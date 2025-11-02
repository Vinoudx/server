#include "address.hpp"

#include <string.h>
#include <unistd.h>
#include <iostream>

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
    return std::make_shared<InetAddress>("0.0.0.0", 0);
}

InetAddress::ptr InetAddress::fromString(const std::string& addr){
    size_t i = addr.find(':');
    if(i == std::string::npos)return nullptr;
    std::string ip = addr.substr(0, i);
    uint16_t port = std::stoi(addr.substr(i+1));
    return InetAddress::createAddr(ip.c_str(), port);
}

std::string InetAddress::dump(){
    std::string str(inet_ntoa(m_addr.sin_addr));
    str.append(":");
    str.append(std::to_string(ntohs(m_addr.sin_port)));
    return str;
}

}