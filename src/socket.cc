#include "socket.hpp"

#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 

#include "socket_hook.hpp"
#include "logger.hpp"
#include "macro.hpp"
#include "socket_manager.hpp"
#include "io_scheduler.hpp"

namespace furina{

static size_t s_max_read = 4096;

Socket::Socket(int family, int type, int protocal)
    :m_family(family)
    ,m_type(type)
    ,m_protocal(protocal){
    m_fd = ::socket(family, type | SOCK_NONBLOCK, protocal);
    if(m_fd <= 0){
        LOG_ERROR << "Socket::Socket() socket fail " << strerror(errno);
    }
    m_isValid = true;
}

Socket::Socket(int fd, const InetAddress::ptr& peer_addr){
    m_fd = fd;
    m_peer_address = peer_addr;
    m_isValid = false;
}

bool Socket::setBlock(){
    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags == -1) {
        LOG_ERROR << "Socket::setBlock() fail" << strerror(errno);
        return false;
    }
    if (fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        LOG_ERROR << "Socket::setBlock() fail" << strerror(errno);
        return false;
    }
    return true;
}

void Socket::setNoDelay(){
    int flag = 1;
    int ret = setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    if (ret == -1) {
        LOG_ERROR << "Socket::setNoDelay() fail" << strerror(errno);
        return;
    }
}

bool Socket::setNonBlock(){
    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags == -1) {
        LOG_ERROR << "Socket::setBlock() fail" << strerror(errno);
        return false;
    }
    if (fcntl(m_fd, F_SETFL, flags & O_NONBLOCK) == -1) {
        LOG_ERROR << "Socket::setBlock() fail" << strerror(errno);
        return false;
    }
    return true;
}

void Socket::setReuseAddr(){
    int opt = 1;
    setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

ssize_t Socket::read(Buffer::ptr buffer){
    if(m_type & SOCK_DGRAM | !m_isValid){
        LOG_ERROR << "cannot apply read on udp socket or closed socket";
        return -1;
    }
    char temp[4096] = {0};
    int n = SocketHook::read(m_fd, temp, 4096);
    if(n == -1 || n == 0)return n;
    buffer->writeString(std::string(temp, n));
    return n;
}

ssize_t Socket::write(const std::string& buffer){
    if(m_type & SOCK_DGRAM | !m_isValid){
        LOG_ERROR << "cannot apply write on udp socket or closed socket";
        return -1;
    }
    return SocketHook::write(m_fd, buffer.c_str(), buffer.size());
}

ssize_t Socket::recv(Buffer::ptr buffer, int flags){
    if(m_type & SOCK_DGRAM | !m_isValid){
        LOG_ERROR << "cannot apply recv on udp socket or closed socket";
        return -1;
    }

    char temp[4096] = {0};
    int n = SocketHook::recv(m_fd, temp, 4096, flags);
    if(n == -1 || n == 0)return n;
    buffer->writeString(std::string(temp, n));
    // LOG_DEBUG << "come " << n << " Bytes, buffer have " << buffer->readableBytes();
    return n;
}

ssize_t Socket::send(const std::string& buffer, int flags){
    if(m_type & SOCK_DGRAM | !m_isValid){
        LOG_ERROR << "cannot apply send on udp socket or closed socket";
        return -1;
    }
    return SocketHook::send(m_fd, buffer.c_str(), buffer.size(), flags);
}

ssize_t Socket::recvfrom(Buffer::ptr buffer, int flags){
    
    if(!(m_type & SOCK_DGRAM) | !m_isValid){
        LOG_ERROR << "cannot apply sendto on not udp socket or closed socket";
        return -1;
    }
    char temp[4096] = {0};
    int n = SocketHook::recvfrom(m_fd, temp, 4096, 0,  m_peer_address->getMutableAddr(), m_peer_address->getMutableLength());
    if(n == -1 || n == 0)return n;
    buffer->writeString(std::string(temp, n));
    return n;
}
ssize_t Socket::sendto(const std::string& buffer, int flags){
    if(!(m_type & SOCK_DGRAM) | !m_isValid){
        LOG_ERROR << "cannot apply recvfrom on not udp socket or closed socket";
        return -1;
    }
    return SocketHook::sendto(m_fd, buffer.c_str(), buffer.size(), flags, m_peer_address->getAddr(), m_peer_address->getLength());    
}

int Socket::connect(InetAddress::ptr address){
    if(!m_isValid){
        LOG_ERROR << "socket closed";
        return -1;
    }
    return SocketHook::connect(m_fd, address->getAddr(), address->getLength());
}

Socket::ptr Socket::accept(){
    if(!m_isValid){
        LOG_ERROR << "socket closed";
        return nullptr;
    }
    InetAddress::ptr addr = InetAddress::createEmptyAddr();
    int new_fd = SocketHook::accept(m_fd, addr->getMutableAddr(), addr->getMutableLength());
    // LOG_DEBUG << new_fd << " " << addr->dump();
    Socket::ptr new_sock = std::shared_ptr<Socket>(new Socket(new_fd, addr));
    new_sock->setNonBlock();
    new_sock->setNoDelay();
    // LOG_DEBUG << addr->getAddr()->sa_family;
    new_sock->m_local_address = m_local_address;
    new_sock->m_family = m_family;
    new_sock->m_type = m_type;
    new_sock->m_protocal = m_protocal;
    new_sock->m_isValid = true;
    SocketManager::getInstance()->insertSocket(new_sock);
    return new_sock;
}

int Socket::bind(InetAddress::ptr addr){
    if(!m_isValid){
        LOG_ERROR << "socket closed";
        return -1;
    }
    m_local_address = addr;
    return SocketHook::bind(m_fd, m_local_address->getAddr(), m_local_address->getLength());
}

int Socket::listen(int backlog){
    if(!m_isValid){
        LOG_ERROR << "socket closed";
        return -1;
    }
    return SocketHook::listen(m_fd, backlog);
}

void Socket::close(){
    if(!m_isValid)return;
    LOG_DEBUG << "Socket::close() on fd " << m_fd;
    IoScheduler* ios = getThisThreadScheduler();
    ios->delAllEvents(m_fd);
    SocketManager::getInstance()->closeSocket(m_fd);
    m_isValid = false;
    ::close(m_fd);
}

}