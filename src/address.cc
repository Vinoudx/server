#include "address.hpp"

#include <string.h>

namespace furina{

InetAddress::InetAddress(const char* ipv4, uint16_t port){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = inet_addr(ipv4);
    m_addr.sin_port = htons(port);

    m_len = sizeof(m_addr);
}

InetAddress::ptr InetAddress::createAddr(const char* ipv4, uint16_t port){
    return std::make_shared<InetAddress>(ipv4, port);
}

InetAddress::ptr InetAddress::createEmptyAddr(){
    return std::make_shared<InetAddress>();
}

}