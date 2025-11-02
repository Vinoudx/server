#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int BUFFER_SIZE = 1024;

void receiveMessages(int sock) {
    char buffer[BUFFER_SIZE];
    sockaddr_in serverAddr;
    socklen_t addrLen = sizeof(serverAddr);

    while (true) {
        ssize_t recvLen = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0,
                                   (sockaddr*)&serverAddr, &addrLen);
        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            std::cout << "\n[Server] " << buffer << std::endl;
            std::cout << "> "; // 提示符
            std::cout.flush();
        }
    }
}

int main() {
    std::string serverIp = "127.0.0.1";
    int serverPort = 8890;

    std::cout << "Enter server IP: ";
    std::cout << "Enter server port: ";

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);

    // 启动接收线程
    std::thread recvThread(receiveMessages, sock);
    recvThread.detach();

    std::string message;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, message);
        if (message.empty()) break;

        ssize_t sentLen = sendto(sock, message.c_str(), message.size(), 0,
                                 (sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentLen < 0) {
            perror("sendto");
        }
    }

    close(sock);
    return 0;
}
