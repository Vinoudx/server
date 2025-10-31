#include "tcp_server.hpp"

#include <string.h>
#include <csignal>
#include <unistd.h>

#include "utils.hpp"
#include "logger.hpp"
#include "macro.hpp"

namespace furina{

TcpServer::TcpServer(size_t num_threads, const InetAddress::ptr& local_address){
    m_isRunning = false;
    m_local_address = local_address;
    m_num_threads = num_threads;
    m_keepAlive = false;
}

TcpServer::~TcpServer(){
    if(!m_isRunning){
        stop();
    }
}

void TcpServer::setConnectionCallback(OnConnectionCallback cb){
    m_connection_cb = std::move(cb);
}

void TcpServer::setMessageCallback(OnMessageCallback cb){
    m_message_cb = std::move(cb);
}

TimerTask::ptr TcpServer::addTimer(uint64_t time_ms, bool isrecurrent,  std::function<void()> cb){
    return m_ios->addTimer(time_ms, isrecurrent, cb);
}

void TcpServer::delTimer(TimerTask::ptr timer){
    m_ios->delTimer(timer);
}

void TcpServer::start(){
    m_isRunning = true;
    m_ios = IoScheduler::ptr(new IoScheduler(m_num_threads));    
    m_ios->start();
    m_listen_socket = SocketManager::getInstance()->createTcpSocket();
    m_listen_socket->setNonBlock();
    m_ios->schedule(std::bind(&TcpServer::handleConnection, this));
}

void TcpServer::stop(){
    m_isRunning = false;
    m_ios->stop();
}

void TcpServer::handleConnection(){
    int rt = 0;
    if(m_listen_socket->bind(m_local_address)){
        LOG_ERROR << "TcpServer::handleConnection() bind fail " << strerror(errno);
        ASSERT(false);
    }

    if(m_listen_socket->listen(1024)){
        LOG_ERROR << "TcpServer::handleConnection() listen fail " << strerror(errno);
        ASSERT(false);
    }

    while(m_isRunning){
        InetAddress::ptr client = InetAddress::createEmptyAddr();
        Socket::ptr client_socket = m_listen_socket->accept();    
        LOG_INFO << "TcpServer::handleConnection() a new connetion established. "
                 << "peer addreess: " << client_socket->getPeerAddr()->dump()
                 << " fd = " << client_socket->getFd();
        m_ios->addEvent(client_socket->getFd(), READ, std::bind(&TcpServer::handleMessage, this, client_socket));
        if(m_connection_cb){
            m_ios->schedule([client_socket, this]{m_connection_cb(std::move(client_socket), Timestamp::nowAbs());});
        }
    }
}

void TcpServer::handleMessage(Socket::ptr sock){
    if(sock->isClosed())return;
    auto buffer = Buffer::ptr(new Buffer);
    int n = sock->recv(buffer, 0);
    if(n == -1){
        LOG_ERROR << "TcpServer::handleMessage() recv fail " << strerror(errno)
                  << "from peer: " << sock->getPeerAddr()->dump();
        return;
    }else if(n == 0){
        LOG_INFO << "TcpServer::handleMessage() a connetion disconnected. "
                 << "peer addreess: " << sock->getPeerAddr()->dump();
        if(m_connection_cb){
            m_ios->schedule([sock, this]{m_connection_cb(std::move(sock), Timestamp::nowAbs());});
        }          
    }else{
        if(m_message_cb){
            // 只加到当前线程中
            SchedulerThread* s_thread = getThisThreadSchedulerThread();
            s_thread->addTask([s_thread, sock, this, buffer]{
                m_message_cb(sock, buffer, Timestamp::nowAbs());
                if(!m_keepAlive)sock->close();
                if(m_keepAlive){
                    s_thread->addEvent(sock->getFd(), READ, std::bind(&TcpServer::handleMessage, this, sock));
                }
            });
        }
    }
}

void TcpServer::waitingForStop(){
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    int sig;
    sigwait(&sigset, &sig);
    LOG_INFO << "Server closed due to Ctrl+C";
    stop();
}

}