#include "socket_manager.hpp"

namespace furina{

Socket::ptr SocketManager::getTcpSocket(int fd){
    std::shared_lock<std::shared_mutex> l(m_mtx);
    auto it = m_sockets.find(fd);
    if(it != m_sockets.end() && it->second->getType() & SOCK_STREAM){
        return it->second;
    }
    return nullptr;
}

Socket::ptr SocketManager::createTcpSocket(){
    std::unique_lock<std::shared_mutex> l(m_mtx);
    Socket::ptr new_sock = std::shared_ptr<Socket>(new Socket(AF_INET, SOCK_STREAM, 0));
    m_sockets.insert_or_assign(new_sock->getFd(), new_sock);
    return new_sock;
}

Socket::ptr SocketManager::getUdpSocket(int fd){
    std::shared_lock<std::shared_mutex> l(m_mtx);
    auto it = m_sockets.find(fd);
    if(it != m_sockets.end() && it->second->getType() & SOCK_DGRAM){
        return it->second;
    }
    return nullptr;
}

Socket::ptr SocketManager::createUdpSocket(){
    std::unique_lock<std::shared_mutex> l(m_mtx);
    Socket::ptr new_sock = std::shared_ptr<Socket>(new Socket(AF_INET, SOCK_DGRAM, 0));
    m_sockets.insert_or_assign(new_sock->getFd(), new_sock);
    return new_sock;
}

Socket::ptr SocketManager::getSpareUdpSocket(){
    std::shared_lock<std::shared_mutex> l(m_mtx);
    for(auto it = m_sockets.begin(); it != m_sockets.end(); it++){
        if(it->second->getType() & SOCK_DGRAM && it->second.use_count() == 1){
            return it->second;
        }
    }
    l.unlock();
    return createUdpSocket();
}

void SocketManager::insertSocket(Socket::ptr sock){
    std::unique_lock<std::shared_mutex> l(m_mtx);
    m_sockets.insert_or_assign(sock->getFd(), sock);
}

void SocketManager::closeSocket(int fd){
    std::unique_lock<std::shared_mutex> l(m_mtx);
    auto it = m_sockets.find(fd);
    if(unlikely(it == m_sockets.end()))return;
    
    m_sockets.erase(it);
}

}