#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

void send_task(const char* ip, int port, int thread_id, int duration_sec) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &servaddr.sin_addr);

    char msg[64];
    snprintf(msg, sizeof(msg), "Hello from thread %d", thread_id);

    auto start = std::chrono::steady_clock::now();
    int count = 0;

    while (true) {
        sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
        count++;

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now - start).count() > duration_sec)
            break;
    }

    std::cout << "Thread " << thread_id << " sent " << count << " packets\n";
    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <server_ip> <port> <threads>\n";
        return 0;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int threads = atoi(argv[3]);
    int duration = 10; // 测试10秒

    std::vector<std::thread> workers;
    for (int i = 0; i < threads; i++) {
        workers.emplace_back(send_task, ip, port, i, duration);
    }

    for (auto& t : workers) t.join();

    std::cout << "All threads finished.\n";
    return 0;
}