#pragma once

#include <memory>
#include <list>
#include <atomic>
#include <functional>
#include <map>

#include "thread.hpp"
#include "fiber.hpp"
#include "timer.hpp"

namespace furina{

class IoScheduler;

class SchedulerThread: public Timer{
public:
    using ptr = std::shared_ptr<SchedulerThread>;
    using uptr = std::unique_ptr<SchedulerThread>;

    explicit SchedulerThread(IoScheduler* scheduler);
    ~SchedulerThread();

    void onFiberReady(Fiber::ptr fiber);
    void onFiberHold(Fiber::ptr fiber);

    void addTask(Fiber::ptr fiber);
    void addTask(std::function<void()> cb);

    void run();
    void tickle();
    void idle();

    void stop();
    bool stopping();

    void addEvent(int fd, int event, std::function<void()> cb);
    // 如果没事件了要把这个东西删掉
    void delEvent(int fd, int event);
    void delAllEvents(int fd);
    // 在epoll关闭时让所有fd上的回调触发一下
    void cancelEvent(int fd, int event);
    void cancelAllEvents(int fd);

private:

    struct EventContext{
        using ptr = std::shared_ptr<EventContext>;
        enum Event{
            NONE = 0,
            READ = 0x001,
            WRITE = 0x004
        };

        EventContext(int fd, SchedulerThread* thread);

        void setActualEvents(int events);
        void handleEvent();

        std::function<void()> read_task;
        std::function<void()> write_task;
        int fd = 0;
        uint32_t registerd_events = 0;
        uint32_t actual_events = 0;
        SchedulerThread* thread;
    };

    std::list<Fiber::ptr> m_ready_queue;
    std::list<Fiber::ptr> m_hold_queue;
    std::atomic<bool> m_stopped;
    size_t m_num_ready_fibers;
    size_t m_num_hold_fibers;
    Thread::ptr m_thread;
    std::mutex m_mtx;
    int m_pipe[2];
    int m_epoll;
    std::map<int, EventContext::ptr> m_events;

    IoScheduler* m_scheduler;
};

SchedulerThread* getThisThreadSchedulerThread();

}