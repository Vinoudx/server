#pragma once 

#include <memory>
#include <unordered_map>
#include <atomic>

#include "io_scheduler.hpp"
#include "socket.hpp"
#include "socket_manager.hpp"
#include "buffer.hpp"
#include "timestamp.hpp"

namespace furina{

class TcpServer{
public:
    using ptr = std::shared_ptr<TcpServer>;
    using OnConnectionCallback = std::function<void(TcpSocket::ptr, Timestamp)>;
    using OnMessageCallback = std::function<void(TcpSocket::ptr, Buffer::ptr, Timestamp)>;

    TcpServer(size_t num_threads, const InetAddress::ptr& local_address);

    ~TcpServer();

    void setConnectionCallback(OnConnectionCallback cb);
    void setMessageCallback(OnMessageCallback cb);

    void start();
    void stop();

    void setKeepAlive(bool v){m_keepAlive = v;}

    TimerTask::ptr addTimer(uint64_t time_ms, bool isrecurrent,  std::function<void()> cb);
    void delTimer(TimerTask::ptr timer);

    void waitingForStop();

private:

    void handleConnection();
    void handleMessage(TcpSocket::ptr sock);

    std::unordered_map<int, TcpSocket::ptr> m_fd_to_socket;

    InetAddress::ptr m_local_address;
    TcpSocket::ptr m_listen_socket;
    IoScheduler::ptr m_ios;
    size_t m_num_threads;
    std::atomic_bool m_isRunning;
    bool m_keepAlive;

    OnConnectionCallback m_connection_cb;
    OnMessageCallback m_message_cb;

};

}