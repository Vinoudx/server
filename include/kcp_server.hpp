#pragma once

#include <memory>

#include <unordered_map>

#include "socket.hpp"
#include "io_scheduler.hpp"

namespace furina {

class KcpServer{
public:
    using ptr = std::shared_ptr<KcpServer>;
    using OnMessageCallback = std::function<void(KcpSocket::ptr, Buffer::ptr, Timestamp)>;
    KcpServer(size_t num_threads, const InetAddress::ptr& local_address);

    ~KcpServer();

    void setMessageCallback(OnMessageCallback cb);

    void start();
    void stop();

    TimerTask::ptr addTimer(uint64_t time_ms, bool isrecurrent,  std::function<void()> cb);
    void delTimer(TimerTask::ptr timer);

    void waitingForStop();

private:

    void updateAllClients();
    void handleMessage(UdpSocket::ptr sock);
    void handleConnection();

    InetAddress::ptr m_local_address;
    IoScheduler::ptr m_ios;
    size_t m_num_threads;
    std::atomic_bool m_isRunning;

    std::unordered_map<InetAddress, KcpSocket::ptr, furina::InetAddressHash> m_clients;

    OnMessageCallback m_message_cb;
};

}
