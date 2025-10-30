#include "socket.hpp"

#include <string.h>

#include "socket_hook.hpp"
#include "logger.hpp"
#include "macro.hpp"

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

ssize_t Socket::read(Buffer::ptr buffer){
    if(m_type == SOCK_DGRAM){
        LOG_ERROR << "cannot apply read on udp socket";
    }
    std::string temp_str;
    char temp[1024] = {0};
    size_t total_Bytes = 0;
    while(true){
        memset(temp, 0, 1024);
        int n = SocketHook::read(m_fd, temp, 1024);
        if(n == 0){
            break;
        }else if(n == -1){
            return -1;
        }else{
            total_Bytes += n;
            temp_str.append(temp);
        }
    }
    buffer->writeString(temp_str);
    return total_Bytes;
}

ssize_t Socket::write(const std::string& buffer){
    if(m_type == SOCK_DGRAM){
        LOG_ERROR << "cannot apply write on udp socket";
    }
    return SocketHook::write(m_fd, buffer.c_str(), buffer.size());
}

ssize_t Socket::recv(Buffer::ptr buffer, int flags){
    if(m_type == SOCK_DGRAM){
        LOG_ERROR << "cannot apply recv on udp socket";
    }
    std::string temp_str;
    char temp[1024] = {0};
    size_t total_Bytes = 0;
    while(true){
        memset(temp, 0, 1024);
        int n = SocketHook::recv(m_fd, temp, 1024, flags);
        if(n == 0){
            break;
        }else if(n == -1){
            return -1;
        }else{
            total_Bytes += n;
            temp_str.append(temp);
        }
    }
    buffer->writeString(temp_str);
    return total_Bytes;
}

ssize_t Socket::send(const std::string& buffer, int flags){
    if(m_type == SOCK_DGRAM){
        LOG_ERROR << "cannot apply send on udp socket";
    }
    return SocketHook::send(m_fd, buffer.c_str(), buffer.size(), flags);
}

ssize_t Socket::recvfrom(Buffer::ptr buffer, int flags){
    
    if(m_type != SOCK_DGRAM){
        LOG_ERROR << "cannot apply sendto on not udp socket";
    }
    std::string temp_str;
    char temp[1024] = {0};
    size_t total_Bytes = 0;
    while(true){
        memset(temp, 0, 1024);
        int n = SocketHook::recvfrom(m_fd, temp, 1024, flags, m_peer_address->getMutableAddr(), m_peer_address->getMutableLength());
        if(n == 0){
            break;
        }else if(n == -1){
            return -1;
        }else{
            total_Bytes += n;
            temp_str.append(temp);
        }
    }
    buffer->writeString(temp_str);
    return total_Bytes;
    
}
ssize_t Socket::sendto(const std::string& buffer, int flags){
    if(m_type != SOCK_DGRAM){
        LOG_ERROR << "cannot apply recvfrom on not udp socket";
    }
    return SocketHook::sendto(m_fd, buffer.c_str(), buffer.size(), flags, m_peer_address->getAddr(), m_peer_address->getLength());    
}

int Socket::connect(InetAddress::ptr address){
    return SocketHook::connect(m_fd, address->getAddr(), address->getLength());
}

Socket::ptr Socket::accept(){
    ASSERT2(false, "NOT IMPL");
}

int Socket::bind(InetAddress::ptr addr){
    m_local_address = addr;
    return SocketHook::bind(m_fd, m_local_address->getAddr(), m_local_address->getLength());
}

int Socket::listen(int backlog){
    return SocketHook::listen(m_fd, backlog);
}

void Socket::close(){
    ASSERT2(false, "NOT IMPL");
}

}