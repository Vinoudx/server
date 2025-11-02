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

class UdpServer{
public:
    using ptr = std::shared_ptr<UdpServer>;
    using OnMessageCallback = std::function<void(UdpSocket::ptr, Buffer::ptr, Timestamp)>;

    UdpServer(size_t num_threads, const InetAddress::ptr& local_address);

    ~UdpServer();

    void setMessageCallback(OnMessageCallback cb);

    void start();
    void stop();

    TimerTask::ptr addTimer(uint64_t time_ms, bool isrecurrent,  std::function<void()> cb);
    void delTimer(TimerTask::ptr timer);

    void waitingForStop();

private:

    void handleMessage(UdpSocket::ptr sock);

    InetAddress::ptr m_local_address;
    IoScheduler::ptr m_ios;
    size_t m_num_threads;
    std::atomic_bool m_isRunning;

    OnMessageCallback m_message_cb;

};

}