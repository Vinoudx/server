#include "udp_server.hpp"

#include <string.h>
#include <csignal>
#include <unistd.h>

#include "utils.hpp"
#include "logger.hpp"
#include "macro.hpp"

namespace furina{

UdpServer::UdpServer(size_t num_threads, const InetAddress::ptr& local_address){
    m_isRunning = false;
    m_local_address = local_address;
    m_num_threads = num_threads;
}

UdpServer::~UdpServer(){
    if(!m_isRunning){
        stop();
    }
}

void UdpServer::setMessageCallback(OnMessageCallback cb){
    m_message_cb = cb;
}

TimerTask::ptr UdpServer::addTimer(uint64_t time_ms, bool isrecurrent,  std::function<void()> cb){
    return m_ios->addTimer(time_ms, isrecurrent, cb);
}

void UdpServer::delTimer(TimerTask::ptr timer){
    m_ios->delTimer(timer);
}

void UdpServer::start(){
    m_isRunning = true;
    m_ios = IoScheduler::ptr(new IoScheduler(m_num_threads));    
    m_ios->start();
    for(int i = 0; i < m_num_threads; i++){
        UdpSocket::ptr sock = UdpSocket::createUdpSocket();
        sock->setReuseAddr();
        sock->setNonBlock();
        sock->bind(m_local_address);
        m_ios->addEvent(sock->getFd(), READ, std::bind(&UdpServer::handleMessage, this, sock));
    }
}

void UdpServer::stop(){
    m_isRunning = false;
    m_ios->stop();
}

void UdpServer::handleMessage(UdpSocket::ptr sock){
    if(sock->isClosed())return;
    auto buffer = Buffer::ptr(new Buffer);
    int n = sock->recvfrom(buffer, 0);
    LOG_DEBUG << n << " from " << sock->getPeerAddr()->dump();
    if(n == -1){
        LOG_ERROR << "UdpServer::handleMessage() recvfrom fail " << strerror(errno)
                  << "from peer: " << sock->getPeerAddr()->dump();
        return;
    }else{
        if(m_message_cb){
            // 只加到当前线程中
            SchedulerThread* s_thread = getThisThreadSchedulerThread();
            s_thread->addTask([s_thread, sock, this, buffer]{
                m_message_cb(sock, buffer, Timestamp::nowAbs());
                s_thread->addEvent(sock->getFd(), READ, std::bind(&UdpServer::handleMessage, this, sock));
            });
        }
    }
}

void UdpServer::waitingForStop(){
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &sigset, nullptr) != 0) {
        LOG_ERROR << "UdpServer::waitingForStop() pthread_sigmask";
        stop();
        return;
    }
    int sig;
    sigwait(&sigset, &sig);
    LOG_INFO << "Server closed due to Ctrl+C";
    stop();
}

}