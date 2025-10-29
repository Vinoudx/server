#pragma once

#include <memory>
#include <functional>
#include <vector>

#include "scheduler_thread.hpp"
#include "fiber.hpp"
#include "timer.hpp"

namespace furina{

class IoScheduler{
public:
    using ptr = std::shared_ptr<IoScheduler>;

    IoScheduler(size_t num_threads = 1);

    void schedule(std::function<void()> task);
    void schedule(Fiber::ptr fiber);

    void addEvent(int fd, int event, std::function<void()> cb);
    void delEvent(int fd, int event);
    void delAllEvents(int fd);

    TimerTask::ptr addTimer(uint64_t time_ms, bool isrecurrent,  std::function<void()> cb);
    void delTimer(TimerTask::ptr timer);

    void stop();

    bool isClosed(){return m_closed;}

private:

    size_t getNextIndex();

    std::vector<SchedulerThread::uptr> m_threads;
    size_t m_num_threads;
    bool m_closed;
    size_t m_roundrobin_next_index;
};

void setThisThreadScheduler(IoScheduler* scheduler);
IoScheduler* getThisThreadScheduler();

}