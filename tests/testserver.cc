#include "tcp_server.hpp"
#include "udp_server.hpp"
#include "kcp_server.hpp"

#include "logger.hpp"

#include <sys/sendfile.h>
#include <sys/stat.h>

using namespace furina;

void onMsg(TcpSocket::ptr sock, Buffer::ptr buffer, Timestamp time){
    std::string s = buffer->readString();
    LOG_INFO << s;

    int f = open("../tests/index.html", O_RDONLY);
    struct stat fileStat;
    fstat(f,&fileStat);
    std::string responce = "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " + std::to_string(fileStat.st_size) + "\r\n"
                "\r\n";
    sock->send(responce, 0);
    sendfile(sock->getFd(), f, 0, fileStat.st_size);

}

void onMsgUdp(UdpSocket::ptr sock, Buffer::ptr buffer, Timestamp time){
    std::string s = buffer->readString();
    LOG_INFO << s;
    sock->sendto(s, 0);
}

void onMsgKcp(KcpSocket::ptr sock, Buffer::ptr buffer, Timestamp time){
    std::string s = buffer->readString();
    LOG_INFO << s;
    sock->sendto(s, 0);
}

int main(){
    InetAddress::ptr addr = InetAddress::createAddr("127.0.0.1", 8891);
    TcpServer s(1, addr);
    // UdpServer s(1, addr);
    // KcpServer s(1, addr);

    s.setMessageCallback(onMsg);
    s.setKeepAlive(false);
    s.start();
    s.waitingForStop();

    return 0;
}

// 