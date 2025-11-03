#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <chrono>
#include "ikcp.h"

uint32_t iclock(){
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// UDP输出函数，KCP每次需要发包时会调用这个回调
int udp_output(const char *buf, int len, ikcpcb *kcp, void *user) {
    int sock = *(int*)user;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8890);   // 服务端端口
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sendto(sock, buf, len, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    return 0;
}

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // KCP会话ID（客户端和服务器必须一致）
    IUINT32 conv = 0;
    ikcpcb *kcp = ikcp_create(conv, (void*)&sock);
    kcp->output = udp_output;

    // 常用配置：快速模式（低延迟）
    ikcp_nodelay(kcp, 1, 10, 2, 1);
    ikcp_wndsize(kcp, 128, 128);
    kcp->rx_minrto = 10;
    kcp->fastresend = 1;

    printf("KCP echo client start...\n");

    struct sockaddr_in serv_addr;
    socklen_t addr_len = sizeof(serv_addr);
    char buffer[2048];

    // 向服务器发一条消息
    const char *msg = "hello kcp";
    ikcp_send(kcp, msg, strlen(msg));

    // 模拟时钟
    IUINT32 current = iclock();
    IUINT32 slap = current + 20;

    while (1) {
        usleep(1000);  // 1ms tick
        current = iclock();
        ikcp_update(kcp, current);

        // 从UDP接收数据
        int n = recvfrom(sock, buffer, sizeof(buffer), MSG_DONTWAIT,
                         (struct sockaddr*)&serv_addr, &addr_len);
        if (n > 0) {
            ikcp_input(kcp, buffer, n);
        }

        // 从KCP读取完整数据
        char recv_buf[1024];
        int hr = ikcp_recv(kcp, recv_buf, sizeof(recv_buf));
        if (hr > 0) {
            recv_buf[hr] = '\0';
            printf("recv: %s\n", recv_buf);
            break;  // 收到echo就退出
        }
    }

    ikcp_release(kcp);
    close(sock);
    return 0;
}
