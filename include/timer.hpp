#pragma once

#include <chrono>
#include <set>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>

#include "timestamp.hpp"

namespace furina{

class Timer;

class TimerTask{
public:   
    friend class Timer;
    using ptr = std::shared_ptr<TimerTask>;
    TimerTask(uint64_t time_ms, bool isrecurrent, std::function<void()> cb);

    void triggerTask();
    bool isRecurrent() {return m_recurrent;}

private:
    // 给lower_bound用的
    TimerTask(uint64_t now);

    struct TimerCmp{
        bool operator()(const TimerTask::ptr& lhs, const TimerTask::ptr& rhs) const;
    };

    std::function<void()> m_task;
    uint64_t m_duration_ms;
    uint64_t m_time_point_ms;
    bool m_recurrent;
};

class Timer{
public:
    using ptr = std::shared_ptr<Timer>;

    Timer(int timerfd);

    TimerTask::ptr addTimer(uint64_t time_ms, bool isrecurrent, std::function<void()> cb);
    void delTimer(TimerTask::ptr timer);

protected:

    void getExpiredTasks(std::vector<TimerTask::ptr>& tasks);
    int m_timer_fd;
    void addTimer(TimerTask::ptr timer);

private:

    void updateTimerfd();
    std::set<TimerTask::ptr, TimerTask::TimerCmp> m_tasks;
    std::mutex m_mtx;
};


}
