#pragma once 

#include <memory>

#include "address.hpp"

namespace furina{

class Socket{
public:
    using ptr = std::shared_ptr<Socket>;

    ssize_t recv(void *buf, size_t len, int flags){}
    ssize_t send(const void *buf, size_t len, int flags){}
    ssize_t sendto(const void *buf, size_t len, int flags,
                          const struct sockaddr *dest_addr, socklen_t addrlen){}
    ssize_t recvfrom(void *buf, size_t len, int flags,
                            struct sockaddr *src_addr, socklen_t *addrlen){}
    
    int connect(InetAddress::ptr address){}
    Socket::ptr accept(){}

    int bind(InetAddress::ptr addr){}
    int listen(int backlog){}
    void close(){}

    InetAddress::ptr getPeerAddr()const{return m_peer_address;}
    InetAddress::ptr getLocalAddr()const{return m_local_address;}

    int isValid() const{return m_valid;}
    int getFamily() const{return m_family;}
    int getType() const{return m_type;}
    int getFlag() const{return m_flag;}
    int getFd() const{return m_fd;}

private:
    int m_family;
    int m_type;
    int m_flag;
    int m_fd;
    bool m_valid;
    InetAddress::ptr m_peer_address;
    InetAddress::ptr m_local_address;
};

}