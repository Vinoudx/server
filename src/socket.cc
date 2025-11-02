#include "socket.hpp"

#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 

#include "socket_hook.hpp"
#include "logger.hpp"
#include "macro.hpp"
#include "scheduler_thread.hpp"
#include "io_scheduler.hpp"

namespace furina{

static size_t s_max_read = 4096;

////////////////////////////////////////////////////////////////////////////////////////////

Socket::Socket(int fd):m_fd(fd),m_isValid(true){
    if(m_fd <= 0){
        LOG_ERROR << "Socket::Socket() socket fail " << strerror(errno);
    }
    m_peer_address = InetAddress::createEmptyAddr();
    m_local_address = InetAddress::createEmptyAddr();
}

bool Socket::setBlock(){
    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags == -1) {
        LOG_ERROR << "TcpSocket::setBlock() fail" << strerror(errno);
        return false;
    }
    if (fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        LOG_ERROR << "TcpSocket::setBlock() fail" << strerror(errno);
        return false;
    }
    return true;
}

void Socket::setNoDelay(){
    int flag = 1;
    int ret = setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    if (ret == -1) {
        LOG_ERROR << "TcpSocket::setNoDelay() fail" << strerror(errno);
        return;
    }
}

bool Socket::setNonBlock(){
    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags == -1) {
        LOG_ERROR << "TcpSocket::setNonBlock() fail" << strerror(errno);
        return false;
    }
    if (fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_ERROR << "TcpSocket::setNonBlock() fail" << strerror(errno);
        return false;
    }
    return true;
}

void Socket::setReuseAddr(){
    int opt = 1;
    if(setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        LOG_ERROR << "Socket::setReuseAddr() setsockopt fail " << strerror(errno);
    }
}

void Socket::setReusePort(){
    int opt = 1;
    if(setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))){
        LOG_ERROR << "Socket::setReuseAddr() setsockopt fail " << strerror(errno);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

TcpSocket::TcpSocket(int protocal):Socket(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, protocal)){
    m_protocal = protocal;
}

TcpSocket::TcpSocket(int fd, const InetAddress::ptr& peer_addr):Socket(fd){
    m_peer_address = peer_addr;
    m_protocal = 0;
    if(getsockname(m_fd, m_local_address->getMutableAddr(), m_local_address->getMutableLength())){
        LOG_ERROR << "TcpSocket::TcpSocket() getsockname fail " << strerror(errno);
    }
}

TcpSocket::~TcpSocket(){
    this->close();
    ::close(m_fd);
    m_isValid = false;
}

TcpSocket::ptr TcpSocket::createTcpSocket(int protocal){
    return TcpSocket::ptr(new TcpSocket(protocal));
}

ssize_t TcpSocket::read(Buffer::ptr buffer){
    if(!m_isValid){
        LOG_ERROR << "TcpSocket closed";
        return -1;
    }
    char temp[4096] = {0};
    int n = SocketHook::read(m_fd, temp, 4096);
    if(n == -1 || n == 0)return n;
    buffer->writeString(std::string(temp, n));
    return n;
}

ssize_t TcpSocket::write(const std::string& buffer){
    if(!m_isValid){
        LOG_ERROR << "TcpSocket closed";
        return -1;
    }
    return SocketHook::write(m_fd, buffer.c_str(), buffer.size());
}

ssize_t TcpSocket::recv(Buffer::ptr buffer, int flags){
    if(!m_isValid){
        LOG_ERROR << "TcpSocket closed";
        return -1;
    }
    char temp[4096] = {0};
    int n = SocketHook::recv(m_fd, temp, 4096, flags);
    if(n == -1 || n == 0)return n;
    buffer->writeString(std::string(temp, n));
    return n;
}

ssize_t TcpSocket::send(const std::string& buffer, int flags){
    if(!m_isValid){
        LOG_ERROR << "TcpSocket closed";
        return -1;
    }
    return SocketHook::send(m_fd, buffer.c_str(), buffer.size(), flags);
}

int TcpSocket::connect(InetAddress::ptr address){
    if(!m_isValid){
        LOG_ERROR << "TcpSocket closed";
        return -1;
    }
    return SocketHook::connect(m_fd, address->getAddr(), address->getLength());
}

TcpSocket::ptr TcpSocket::accept(){
    if(!m_isValid){
        LOG_ERROR << "TcpSocket closed";
        return nullptr;
    }
    InetAddress::ptr addr = InetAddress::createEmptyAddr();
    int new_fd = SocketHook::accept(m_fd, addr->getMutableAddr(), addr->getMutableLength());
    if(new_fd == 0 || new_fd == -1)return nullptr;
    TcpSocket::ptr new_sock = std::shared_ptr<TcpSocket>(new TcpSocket(new_fd, addr));
    new_sock->setNonBlock();
    new_sock->setNoDelay();
    return new_sock;
}

int TcpSocket::bind(InetAddress::ptr addr){
    if(!m_isValid){
        LOG_ERROR << "TcpSocket closed";
        return -1;
    }
    m_local_address = addr;
    return SocketHook::bind(m_fd, m_local_address->getAddr(), m_local_address->getLength());
}

int TcpSocket::listen(int backlog){
    if(!m_isValid){
        LOG_ERROR << "TcpSocket closed";
        return -1;
    }
    return SocketHook::listen(m_fd, backlog);
}

void TcpSocket::close(){
    if(!m_isValid)return;
    m_isValid = false;
    SchedulerThread* ios = getThisThreadSchedulerThread();
    if(ios)ios->delAllEvents(m_fd);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

UdpSocket::UdpSocket(int protocal):Socket(::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, protocal)){
    m_protocal = protocal;
}

UdpSocket::~UdpSocket(){
    this->close();
    ::close(m_fd);
    m_isValid = false;
}

UdpSocket::ptr UdpSocket::createUdpSocket(int protocal){
    return UdpSocket::ptr(new UdpSocket(protocal));
}

ssize_t UdpSocket::recvfrom(Buffer::ptr buffer, int flags){
    if(!m_isValid){
        LOG_ERROR << "UdpSocket closed";
        return -1;
    }
    char temp[4096] = {0};
    int n = SocketHook::recvfrom(m_fd, temp, 4096, 0,  m_peer_address->getMutableAddr(), m_peer_address->getMutableLength());
    if(n == -1 || n == 0)return n;
    buffer->writeString(std::string(temp, n));
    return n;
}

ssize_t UdpSocket::sendto(const std::string& buffer, int flags){
    if(!m_isValid){
        LOG_ERROR << "UdpSocket closed";
        return -1;
    }
    return SocketHook::sendto(m_fd, buffer.c_str(), buffer.size(), flags, m_peer_address->getAddr(), m_peer_address->getLength());    
}

ssize_t UdpSocket::sendto(const std::string& buffer, const InetAddress::ptr& addr, int flags){
    m_peer_address = addr;
    return sendto(buffer, flags);   
}

int UdpSocket::bind(InetAddress::ptr addr){
    if(!m_isValid){
        LOG_ERROR << "UdpSocket closed";
        return -1;
    }
    m_local_address = addr;
    return SocketHook::bind(m_fd, m_local_address->getAddr(), m_local_address->getLength());
}

void UdpSocket::close(){
    if(!m_isValid)return;
    m_isValid = false;
    SchedulerThread* ios = getThisThreadSchedulerThread();
    if(ios)ios->delAllEvents(m_fd);
}

}