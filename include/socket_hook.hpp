#pragma once 

#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace furina{

class SocketHook{
public:

    static ssize_t read(int fd, void *buf, size_t count);
    static ssize_t write(int fd, const void *buf, size_t count);

    static ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    static ssize_t send(int sockfd, const void *buf, size_t len, int flags);
    static ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                          const struct sockaddr *dest_addr, socklen_t addrlen);
    static ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                            struct sockaddr *src_addr, socklen_t *addrlen);
    
    static int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    static int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
    static int listen(int fd, int backlog);
    static int bind(int fd, const sockaddr* addr, socklen_t len);
private:
    enum Event{
        READ = 0x001,
        WRITE = 0x004
    };
};

}