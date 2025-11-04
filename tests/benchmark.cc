#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <vector>
#include <unistd.h>
#include <atomic>
#include <mutex>
#include <algorithm>

using namespace std;
using namespace std::chrono;

// 配置参数
const int MSG_SIZE = 10;          // 每条消息大小
const int NUM_CONN = 12;         // 并发连接数
const int DURATION_SEC = 10;      // 测试时长（秒）
const string SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8890;

// 全局计数器
atomic<long> g_count{0};
mutex latency_mutex;
vector<double> g_latencies; // 存放每次请求延迟（毫秒）

// 单个 worker
void worker(int id) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return; }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &serv_addr.sin_addr);

    if (connect(fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect"); close(fd); return;
    }

    char msg[MSG_SIZE];
    memset(msg, 'A' + id % 26, MSG_SIZE);
    char buf[MSG_SIZE];

    auto start_time = high_resolution_clock::now();
    while (duration_cast<seconds>(high_resolution_clock::now() - start_time).count() < DURATION_SEC) {
        auto req_start = high_resolution_clock::now();

        if (send(fd, msg, MSG_SIZE, 0) != MSG_SIZE) { perror("send"); break; }
        if (recv(fd, buf, MSG_SIZE, MSG_WAITALL) != MSG_SIZE) { perror("recv"); break; }

        auto req_end = high_resolution_clock::now();
        double latency_ms = duration<double, milli>(req_end - req_start).count();

        {
            lock_guard<mutex> lock(latency_mutex);
            g_latencies.push_back(latency_ms);
        }

        g_count++;
    }

    close(fd);
}

int main() {
    vector<thread> threads;

    auto test_start = high_resolution_clock::now();

    for (int i = 0; i < NUM_CONN; i++) {
        threads.emplace_back(worker, i);
    }

    for (auto &t : threads) t.join();

    auto test_end = high_resolution_clock::now();
    double elapsed_sec = duration<double>(test_end - test_start).count();
    double qps = g_count.load() / elapsed_sec;

    cout << "Total requests: " << g_count.load() << endl;
    cout << "Elapsed time: " << elapsed_sec << " s" << endl;
    cout << "QPS: " << qps << endl;

    if (!g_latencies.empty()) {
        sort(g_latencies.begin(), g_latencies.end());
        double sum = 0;
        for (double v : g_latencies) sum += v;
        cout << "Avg latency: " << sum / g_latencies.size() << " ms" << endl;
        cout << "P50 latency: " << g_latencies[g_latencies.size() * 50 / 100] << " ms" << endl;
        cout << "P99 latency: " << g_latencies[g_latencies.size() * 99 / 100] << " ms" << endl;
        cout << "Max latency: " << g_latencies.back() << " ms" << endl;
    }

    return 0;
}
