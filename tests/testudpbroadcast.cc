#include "udp_server.hpp"
#include "logger.hpp"
#include "address.hpp"
#include "socket_manager.hpp"

#include <unordered_set>

using namespace furina;

class BroadCastServer{
public:
    BroadCastServer(int num_thread, InetAddress::ptr addr){
        m_addr = addr;
        m_server = UdpServer::ptr(new UdpServer(num_thread, addr));
        m_server->setMessageCallback(std::bind(&BroadCastServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    void start(){
        m_server->start();
    }

    void onMessage(UdpSocket::ptr sock, Buffer::ptr buffer, Timestamp time){
        std::string s = buffer->readString();
        LOG_INFO << s << " from " << sock->getPeerAddr()->dump();
        m_clients.insert(sock->getPeerAddr()->dump());
        broadcast(s);
    }

    void wait(){
        m_server->waitingForStop();
    }

private:

    void broadcast(const std::string& message){
        LOG_INFO << "has " << m_clients.size() << " clients";
        UdpSocket::ptr sock = UdpSocket::createUdpSocket();
        for(auto& client: m_clients){
            sock->sendto(message, InetAddress::fromString(client));
        }
    }

    std::unordered_set<std::string> m_clients;

    UdpServer::ptr m_server;
    InetAddress::ptr m_addr;
};



int main(){
    InetAddress::ptr addr = InetAddress::createAddr("127.0.0.1", 8891);
    BroadCastServer s(3, addr);
    s.start();
    s.wait();
}