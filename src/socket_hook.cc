#include "socket_hook.hpp"

#include "io_scheduler.hpp"
#include "fiber.hpp"
#include "logger.hpp"

namespace furina{

static int64_t s_io_timeout_ms = 5000;

template<typename Func, typename... Args>
static ssize_t do_io(Func func, int fd, const char* name, int event, int64_t timeout_ms, Args&& ...args){
    SchedulerThread* ios = getThisThreadSchedulerThread();
    bool is_time_out = false;
    while(true){
        is_time_out = false;
        int n = func(fd, std::forward<Args>(args)...);
        while(n == -1 && errno == EINTR){
            n = func(fd, std::forward<Args>(args)...);
        }
        if(n == -1 && errno == EAGAIN){
            auto this_fiber = getExecFiber(); 
            TimerTask::ptr timer = nullptr;
            if(timeout_ms != -1){
                timer = ios->addTimer(timeout_ms, false, [&this_fiber, &ios, fd, &is_time_out, &event, timeout_ms]{
                        LOG_INFO << "fd = " << fd << " event = " << event << " timeout after " << timeout_ms << "ms";  
                        ios->delEvent(fd, event);
                        errno = ETIMEDOUT;
                        is_time_out = true;
                        this_fiber->resume();
                });
            }
            ios->addEvent(fd, event, [&this_fiber]{
                this_fiber->resume();
            });
            Fiber::YeildToHold();
            if(errno == ECANCELED){
                return 0;
            }

            if(is_time_out){
                return 0;
            }
            if(timer){
                ios->delTimer(timer);
            }
        }else{
            return n;
        }
    }
}

ssize_t SocketHook::read(int fd, void *buf, size_t count){
    return do_io(::read, fd, "read", READ, s_io_timeout_ms, buf, count);
}

ssize_t SocketHook::write(int fd, const void *buf, size_t count){
    return do_io(::write, fd, "write", WRITE, s_io_timeout_ms, buf, count);
}

ssize_t SocketHook::recv(int sockfd, void *buf, size_t len, int flags){
    return do_io(::recv, sockfd, "recv", READ, s_io_timeout_ms, buf, len, flags);
}

ssize_t SocketHook::send(int sockfd, const void *buf, size_t len, int flags){
    return do_io(::send, sockfd, "send", WRITE, s_io_timeout_ms, buf, len, flags);
}

ssize_t SocketHook::sendto(int sockfd, const void *buf, size_t len, int flags,
                        const struct sockaddr *dest_addr, socklen_t addrlen){
    return do_io(::sendto, sockfd, "sendto", WRITE, s_io_timeout_ms, buf, len, flags, dest_addr, addrlen);
}

ssize_t SocketHook::recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen){
    return do_io(::recvfrom, sockfd, "recvfrom", READ, -1, buf, len, flags, src_addr, addrlen);
}

int SocketHook::connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    // return ::connect(sockfd, addr, addrlen);
    return do_io(::connect, sockfd, "connect", READ, s_io_timeout_ms, addr, addrlen);
}

int SocketHook::accept(int s, struct sockaddr *addr, socklen_t *addrlen){
    // return ::accept(s, addr, addrlen);
    return do_io(::accept, s, "accept", READ, -1, addr, addrlen);
}

int SocketHook::listen(int fd, int backlog){
    return ::listen(fd, backlog);
}

int SocketHook::bind(int fd, const sockaddr* addr, socklen_t len){
    return ::bind(fd, addr, len);
}

}