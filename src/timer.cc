#include "timer.hpp"

#include <sys/timerfd.h>
#include <string.h>

#include "logger.hpp"

namespace furina{

bool TimerTask::TimerCmp::operator()(const TimerTask::ptr& lhs, const TimerTask::ptr& rhs) const{
    return lhs->m_time_point_ms < rhs->m_time_point_ms;
}

TimerTask::TimerTask(uint64_t time_ms, bool isrecurrent, std::function<void()> cb)
    :m_task(cb)
    ,m_duration_ms(time_ms)
    ,m_recurrent(isrecurrent){

    m_time_point_ms = Timestamp::nowTimeMs() + time_ms;
}

void TimerTask::triggerTask(){
    if(m_task)m_task();
}

TimerTask::TimerTask(uint64_t now):m_time_point_ms(now){}

////////////////////////////////////////////////

Timer::Timer(int timerfd)
    :m_timer_fd(timerfd){

    if(m_timer_fd <= 0){
        LOG_FATAL << "Timer::Timer() invalid timer";
    }
    struct itimerspec tsp;
    memset(&tsp, 0, sizeof(tsp));
    if(timerfd_settime(m_timer_fd, 0, &tsp, nullptr)){
        LOG_ERROR << "Timer::Timer() set time fail" << strerror(errno);
    }
}

TimerTask::ptr Timer::addTimer(uint64_t time_ms, bool isrecurrent, std::function<void()> cb){
    TimerTask::ptr tt = std::shared_ptr<TimerTask>(new TimerTask(time_ms, isrecurrent, cb));
    addTimer(tt);
    return tt;
}

void Timer::addTimer(TimerTask::ptr timer){
    std::lock_guard<std::mutex> l(m_mtx);
    m_tasks.emplace(std::move(timer));
    updateTimerfd();    
}

void Timer::delTimer(TimerTask::ptr timer){
    std::lock_guard<std::mutex> l(m_mtx);
    m_tasks.erase(timer);
    updateTimerfd();
}

void Timer::getExpiredTasks(std::vector<TimerTask::ptr>& tasks){
    uint64_t now_ms = Timestamp::nowTimeMs();

    auto it = m_tasks.lower_bound(std::shared_ptr<TimerTask>(new TimerTask(now_ms)));
    if(it != m_tasks.end())it++;

    tasks.insert(tasks.end(), m_tasks.begin(), it);
    m_tasks.erase(m_tasks.begin(), it);

    for(auto& timer: tasks){
        if(timer->isRecurrent()){
            timer->m_time_point_ms = now_ms + timer->m_duration_ms;
            addTimer(timer);
        }
    }

    if(m_tasks.empty()){
        struct itimerspec tsp;
        memset(&tsp, 0, sizeof(tsp));
        if(timerfd_settime(m_timer_fd, 0, &tsp, nullptr)){
            LOG_ERROR << "Timer::getExpiredTasks() set time fail" << strerror(errno);
        }
    }
}

void Timer::updateTimerfd(){
    auto earlist = m_tasks.begin();
    if(earlist == m_tasks.end())return;
    
    struct itimerspec tsp;
    memset(&tsp, 0, sizeof(tsp));
    int64_t diff_ms = (*earlist)->m_time_point_ms - Timestamp::nowTimeMs();
    if (diff_ms <= 0) diff_ms = 1; // 确保不会立刻触发

    tsp.it_value.tv_sec  = diff_ms / 1000;
    tsp.it_value.tv_nsec = (diff_ms % 1000) * 1000000;
    if(timerfd_settime(m_timer_fd, 0, &tsp, nullptr)){
        LOG_ERROR << "Timer::updateTimerfd() set time fail " << strerror(errno); 
    }
}

}
