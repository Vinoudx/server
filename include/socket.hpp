#pragma once 

#include <memory>

#include "address.hpp"
#include "buffer.hpp"

namespace furina{

// 只保管fd， fd具体的创建在TcpSocket，UdpSocket，KcpSocket中
class Socket{
public:
    using ptr = std::shared_ptr<Socket>;

    ~Socket() = default;

    InetAddress::ptr getPeerAddr()const{return m_peer_address;}
    InetAddress::ptr getLocalAddr()const{return m_local_address;}

    int getFd() const{return m_fd;}
    bool isClosed() const{return !m_isValid;}
    int getProtocal() const{return m_protocal;}

    bool setBlock();
    bool setNonBlock();
    void setReuseAddr();
    void setReusePort();
    void setNoDelay();

protected:

    Socket(int fd);

    int m_fd;
    bool m_isValid;
    int m_protocal;
    InetAddress::ptr m_peer_address;
    InetAddress::ptr m_local_address;

};


class TcpSocket: public Socket{
public:
    using ptr = std::shared_ptr<TcpSocket>;

    TcpSocket(int protocal = 0);
    ~TcpSocket();

    static TcpSocket::ptr createTcpSocket(int protocal = 0);

    ssize_t read(Buffer::ptr buffer);
    ssize_t write(const std::string& buffer);

    ssize_t recv(Buffer::ptr buffer, int flags = 0);
    ssize_t send(const std::string& buffer, int flags = 0);

    int connect(InetAddress::ptr address);
    TcpSocket::ptr accept();

    int bind(InetAddress::ptr addr);
    int listen(int backlog);
    void close();

private:
    // for accept only
    TcpSocket(int fd, const InetAddress::ptr& peer_addr);
};

class UdpSocket: public Socket{
public:
    using ptr = std::shared_ptr<UdpSocket>;

    UdpSocket(int protocal = 0);
    ~UdpSocket();

    static UdpSocket::ptr createUdpSocket(int protocal = 0);

    ssize_t recvfrom(Buffer::ptr buffer, int flags = 0);
    ssize_t sendto(const std::string& buffer, int flags = 0);
    ssize_t sendto(const std::string& buffer, const InetAddress::ptr& addr, int flags = 0);

    int bind(InetAddress::ptr addr);

    void close();
};


}