#pragma once 

#include <memory>

#include "address.hpp"
#include "buffer.hpp"

namespace furina{

class Socket{
public:
    using ptr = std::shared_ptr<Socket>;

    Socket(int family, int type, int protocal);
    ~Socket(){if(m_isValid)close();}

    ssize_t read(Buffer::ptr buffer);
    ssize_t write(const std::string& buffer);

    ssize_t recv(Buffer::ptr buffer, int flags);
    ssize_t send(const std::string& buffer, int flags);
    ssize_t recvfrom(Buffer::ptr buffer, int flags);
    ssize_t sendto(const std::string& buffer, int flags);
    
    int connect(InetAddress::ptr address);
    Socket::ptr accept();

    int bind(InetAddress::ptr addr);
    int listen(int backlog);
    void close();

    InetAddress::ptr getPeerAddr()const{return m_peer_address;}
    InetAddress::ptr getLocalAddr()const{return m_local_address;}

    void setPeerAddr(const InetAddress::ptr& addr){m_peer_address = addr;}

    int getFamily() const{return m_family;}
    int getType() const{return m_type;}
    int getProtocal() const{return m_protocal;}
    int getFd() const{return m_fd;}

private:
    int m_family;
    int m_type;
    int m_protocal;
    int m_fd;
    bool m_isValid;
    InetAddress::ptr m_peer_address;
    InetAddress::ptr m_local_address;
};

}