#pragma once

#if 0

#include <sys/socket.h>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>

#include "socket.hpp"
#include "utils.hpp"

namespace furina{

class SocketManager: public Singleton<SocketManager>{
    friend class Singleton<SocketManager>;
public:

    Socket::ptr getTcpSocket(int fd);
    Socket::ptr createTcpSocket();

    Socket::ptr getUdpSocket(int fd);
    Socket::ptr createUdpSocket();
    Socket::ptr getSpareUdpSocket();

    void insertSocket(Socket::ptr sock);
    void closeSocket(int fd);

private:
    std::unordered_map<int, Socket::ptr> m_sockets;
    std::shared_mutex m_mtx;
};

}

#endif