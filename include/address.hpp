#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <memory>

namespace furina{

class InetAddress{
public:
    using ptr = std::shared_ptr<InetAddress>;

    static InetAddress::ptr createAddr(const char* ipv4, uint16_t port);
    static InetAddress::ptr createEmptyAddr();

    InetAddress(const char* ipv4, uint16_t port);
    InetAddress() = default;
    const sockaddr* getAddr() const{return (sockaddr*)&m_addr;}
    socklen_t getLength() const{return m_len;}

    sockaddr* getMutableAddr() {return (sockaddr*)&m_addr;}
    socklen_t* getMutableLength() {return &m_len;}

private:
    struct sockaddr_in m_addr;
    socklen_t m_len;
};

}