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

////////////////////////////////////////////////////////////////////////////////////////

KcpSocket::KcpSocket(uint32_t conv_id, int protocal):UdpSocket(protocal){
    m_kcp = ikcp_create(conv_id, (void*)this);
    m_kcp->output = KcpSocket::kcp_send_callback;
    ikcp_nodelay(m_kcp, 1, 10, 2, 1);   // 极速模式
    ikcp_wndsize(m_kcp, 128, 128);      // 发送窗口、接收窗口
    ikcp_setmtu(m_kcp, 1400);           // MTU 一般 1400（留给 UDP/IP 头）
    m_kcp->rx_minrto = 10;              // 最小RTO 10ms（默认100ms太慢）
}

KcpSocket::~KcpSocket(){
    if(m_isValid){
        this->close();  
    }
    ::close(m_fd);
    m_isValid = false;
}

KcpSocket::ptr KcpSocket::createKcpSocket(uint32_t conv, int protocal){
    return KcpSocket::ptr(new KcpSocket(conv, protocal));
}

uint64_t KcpSocket::getMilisecondForUpdate(){
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void KcpSocket::update(){
    ikcp_update(m_kcp, KcpSocket::getMilisecondForUpdate());
}

ssize_t KcpSocket::recvfrom(Buffer::ptr buffer, int flags){
    char temp[4096] = {0};
    std::string t;
    int hr = 0;
    while(true){
        hr = ikcp_recv(m_kcp, temp, 4096);
        if(hr <= 0)break;
        t.append(temp, hr);
    }
    buffer->writeString(t);
    return t.size();
}

void KcpSocket::setKcp(const char* buf, size_t len){
    ikcp_input(m_kcp, buf, len);
}

ssize_t KcpSocket::sendto(const std::string& buffer, int flags){
    m_flag_context = flags;
    return ikcp_send(m_kcp, buffer.c_str(), buffer.size());
}

void KcpSocket::close(){
    if(!m_isValid)return;
    ikcp_release(m_kcp);
    m_isValid = false;
    // SchedulerThread* ios = getThisThreadSchedulerThread();
    // if(ios)ios->delAllEvents(m_fd);
}

int KcpSocket::kcp_send_callback(const char* buf, int len, ikcpcb* kcp, void* user){
    KcpSocket* sock = static_cast<KcpSocket*>(user);
    return SocketHook::sendto(sock->m_fd, buf, len, sock->m_flag_context, sock->m_peer_address->getAddr(), sock->m_peer_address->getLength());
}


}