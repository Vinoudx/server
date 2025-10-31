#include "tcp_server.hpp"
#include "udp_server.hpp"

#include "logger.hpp"

using namespace furina;

void onMsg(Socket::ptr sock, Buffer::ptr buffer, Timestamp time){
    std::string s = buffer->readString();
    LOG_INFO << s;
    std::string responce = "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 13\r\n"
                "\r\n"
                "Hello World!";
    sock->send(responce, 0);
}

void onMsgUdp(Socket::ptr sock, Buffer::ptr buffer, Timestamp time){
    std::string s = buffer->readString();
    LOG_INFO << s;
    sock->sendto(s, 0);
}

int main(){
    InetAddress::ptr addr = InetAddress::createAddr("127.0.0.1", 8889);
    TcpServer s(1, addr);
    // UdpServer s(1, addr);

    s.setMessageCallback(onMsg);
    // s.setKeepAlive(true);
    s.start();
    s.waitingForStop();

    return 0;
}