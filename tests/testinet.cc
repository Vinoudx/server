#include "address.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include "logger.hpp"
#include "macro.hpp"
#include "io_scheduler.hpp"
#include "socket_hook.hpp"
#include "socket.hpp"
#include "socket_manager.hpp"

using namespace furina;

void serverfunc(){
    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    
    int rt = 0;

    auto addr = InetAddress::createAddr("127.0.0.1", (uint16_t)8080);

    rt = SocketHook::bind(sock, addr->getAddr(), addr->getLength());
    // ASSERT2(rt, strerror(errno));
    rt = SocketHook::listen(sock, 1024);
    // ASSERT2(rt, strerror(errno));

    auto client_addr = InetAddress::createEmptyAddr();
    int client = SocketHook::accept(sock, client_addr->getMutableAddr(), client_addr->getMutableLength());
    LOG_INFO << "connet from " << client_addr->dump() << ' ' << client_addr->dump();
    int flags = fcntl(client, F_GETFL, 0);    // 获取当前标志位
    fcntl(client, F_SETFL, flags | O_NONBLOCK);
    while(true){

        char buf[1024] = {0};

        int n = SocketHook::recv(client, buf, 1024, 0);
        if(n == -1)break;
        LOG_INFO << buf;

        SocketHook::send(client, buf, n, 0);

    }
    close(client);
    close(sock);
}

void serverfunc1(){
    // int sock = socket(AF_INET, SOCK_STREAM, 0);
    auto sock = SocketManager::getInstance()->createTcpSocket();
    int rt = 0;
    sock->setReuseAddr();
    auto addr = InetAddress::createAddr("127.0.0.1", (uint16_t)8123);

    // rt = SocketHook::bind(sock, addr->getAddr(), addr->getLength());
    rt = sock->bind(addr);
    ASSERT2(rt == 0, strerror(errno));
    // rt = SocketHook::listen(sock, 1024);
    rt = sock->listen(1024);
    ASSERT2(rt == 0, strerror(errno));
    
    // auto client_addr = InetAddress::createEmptyAddr();
    // int client = SocketHook::accept(sock, client_addr->getMutableAddr(), client_addr->getMutableLength());
    auto client_sock = sock->accept();
    // int flags = fcntl(client, F_GETFL, 0);    // 获取当前标志位
    // fcntl(client, F_SETFL, flags | O_NONBLOCK);
    LOG_INFO << "connet from " << client_sock->getPeerAddr()->dump() << ' ' << client_sock->getLocalAddr()->dump();
    auto buffer = Buffer::ptr(new Buffer);    
    while(true){
        int n = client_sock->recv(buffer, 0);
        if(n == -1 || n == 0)break;
        std::string str = buffer->readString();
        // LOG_INFO << str;
        client_sock->send(str, 0);
    }
    LOG_INFO << "server stop";
}

void clientsocket(){
    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int rt = 0;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("153.3.238.127");
    addr.sin_port = htons(80);
    rt = connect(sock, (sockaddr*)&addr, sizeof(addr));

    char content[] = "GET / HTTP/1.0\r\n\r\n";
    int n = SocketHook::send(sock, content, sizeof content, 0);
    while(true){
        char buf[4096] = {0};
        n = SocketHook::recv(sock, buf, 4095, 0);
        if(n == 0)break;
        LOG_INFO << buf;
    }
}

void clientsocket1(){
    // int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    auto sock = SocketManager::getInstance()->createTcpSocket();
    int rt = 0;
    // struct sockaddr_in addr;
    // memset(&addr, 0, sizeof(addr));
    // addr.sin_family = AF_INET;
    // addr.sin_addr.s_addr = inet_addr("153.3.238.127");
    // addr.sin_port = htons(80);
    // rt = connect(sock, (sockaddr*)&addr, sizeof(addr));
    auto addr = InetAddress::createAddr("153.3.238.127", 80);
    rt = sock->connect(addr);

    std::string str = "GET / HTTP/1.0\r\n\r\n";
    // int n = SocketHook::send(sock, content, sizeof content, 0);
    int n = sock->send(str, 0);
    auto buffer = Buffer::ptr(new Buffer);
    while(true){
        int n = sock->recv(buffer, 0);
        if(n == -1 || n == 0)break;
        std::string str = buffer->readString();
        LOG_INFO << str;
    }
    LOG_INFO << "client stop";
}

int main(){

    IoScheduler iom(4);
    iom.start();
    iom.schedule(serverfunc1);
    iom.schedule(serverfunc1);
    iom.schedule(serverfunc1);
    iom.schedule(serverfunc1);
    // iom.schedule(clientsocket1);


    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
    iom.stop();
    return 0;
}
