#include "kcp_server.hpp"

#include <string.h>
#include <csignal>
#include <unistd.h>

#include "utils.hpp"
#include "logger.hpp"
#include "macro.hpp"

namespace furina{

static uint64_t s_kcp_update_ms = 20;

KcpServer::KcpServer(size_t num_threads, const InetAddress::ptr& local_address){
    m_isRunning = false;
    m_local_address = local_address;
    m_num_threads = num_threads;
}

KcpServer::~KcpServer(){
    if(!m_isRunning){
        stop();
    }
}

void KcpServer::setMessageCallback(OnMessageCallback cb){
    m_message_cb = cb;
}

TimerTask::ptr KcpServer::addTimer(uint64_t time_ms, bool isrecurrent,  std::function<void()> cb){
    return m_ios->addTimer(time_ms, isrecurrent, cb);
}

void KcpServer::delTimer(TimerTask::ptr timer){
    m_ios->delTimer(timer);
}

void KcpServer::updateAllClients(){
    for(auto& client: m_clients){
        client.second->update();
    }
}

void KcpServer::start(){
    m_isRunning = true;
    m_ios = IoScheduler::ptr(new IoScheduler(m_num_threads));    
    m_ios->start();
    m_ios->addTimer(s_kcp_update_ms, true, std::bind(&KcpServer::updateAllClients, this));
    for(int i = 0; i < m_num_threads; i++){
        m_ios->schedule(std::bind(&KcpServer::handleConnection, this));
    }
}

void KcpServer::stop(){
    m_isRunning = false;
    m_ios->stop();
}

void KcpServer::handleConnection(){
    UdpSocket::ptr sock = UdpSocket::createUdpSocket();
    sock->setReuseAddr();
    sock->setNonBlock();
    sock->bind(m_local_address);
    handleMessage(sock);
}

void KcpServer::handleMessage(UdpSocket::ptr sock){
    if(sock->isClosed())return;
    auto buffer = Buffer::ptr(new Buffer);
    int n = sock->recvfrom(buffer, 0);
    if(n == -1){
        LOG_ERROR << "KcpServer::handleMessage() recvfrom fail " << strerror(errno)
                  << "from peer: " << sock->getPeerAddr()->dump();
        sock->close();
        return;
    }if(n == 0){
        sock->close();
        return;
    }else{
        KcpSocket::ptr ksock = nullptr;
        InetAddress c_addr = *sock->getPeerAddr();
        std::string message = buffer->readString();
        uint32_t conv = ikcp_getconv(message.c_str());
        if(m_clients.find(c_addr) == m_clients.end()){
            ksock = KcpSocket::createKcpSocket(conv);
            ksock->setPeerAddr(sock->getPeerAddr());
            m_clients.insert({c_addr, ksock});
        }else{
            ksock = m_clients[c_addr];
        }
        ksock->setKcp(message.c_str(), message.size());
        SchedulerThread* s_thread = getThisThreadSchedulerThread();
        n = ksock->recvfrom(buffer, 0);
        if(n < 0){
            LOG_ERROR << "KcpServer::handleMessage() recvfrom fail " << strerror(errno)
                  << "from peer: " << ksock->getPeerAddr()->dump();
        }else if(n > 0){
            if(m_message_cb){
                // 只加到当前线程中
                s_thread->addTask([ksock, this, buffer]{
                    m_message_cb(ksock, buffer, Timestamp::nowAbs());
                });
            }
        }
        s_thread->addTask(std::bind(&KcpServer::handleMessage, this, sock));
    }
    
}

void KcpServer::waitingForStop(){
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &sigset, nullptr) != 0) {
        LOG_ERROR << "KcpServer::waitingForStop() pthread_sigmask";
        stop();
        return;
    }
    int sig;
    sigwait(&sigset, &sig);
    LOG_INFO << "Server closed due to Ctrl+C";
    stop();
}

}