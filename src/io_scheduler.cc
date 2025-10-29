#include "io_scheduler.hpp"

#include "logger.hpp"

namespace furina{

static thread_local IoScheduler* s_this_thread_scheduler = nullptr;

void setThisThreadScheduler(IoScheduler* scheduler){
    s_this_thread_scheduler = scheduler;
}

IoScheduler* getThisThreadScheduler(){
    if(unlikely(s_this_thread_scheduler == nullptr)){
        LOG_FATAL << "please do not execute I/O in your main thread, otherwise what's the meaning of \
            buliding an I/O multiplexing system?";
    }
    return s_this_thread_scheduler;
}

IoScheduler::IoScheduler(size_t num_threads)
    :m_closed(true)
    ,m_roundrobin_next_index(0){
    
    setThisThreadScheduler(this);
    m_num_threads = num_threads;
}

void IoScheduler::start(){
    m_closed = false;
    if(m_num_threads > 0){
        m_threads.reserve(m_num_threads);
        for(int i = 0; i < m_num_threads; i++){
            m_threads.emplace_back(new SchedulerThread(this));
        }
    }
}

size_t IoScheduler::getNextIndex(){
    size_t now_index = m_roundrobin_next_index;
    m_roundrobin_next_index = (m_roundrobin_next_index + 1) % m_num_threads;
    return now_index;
}

void IoScheduler::schedule(std::function<void()> task){
    size_t index = getNextIndex();
    m_threads[index]->addTask(task);
}

void IoScheduler::schedule(Fiber::ptr fiber){
    size_t index = getNextIndex();
    m_threads[index]->addTask(fiber);
}

void IoScheduler::addEvent(int fd, int event, std::function<void()> cb){
    size_t index = getNextIndex();
    // LOG_DEBUG << "add event at index " << index;
    m_threads[index]->addEvent(fd, event, cb);
    m_fd_to_index[fd] = index;
}

void IoScheduler::delEvent(int fd, int event){
    size_t index = m_fd_to_index.at(fd);
    // LOG_DEBUG << "del event at index " << index;
    m_threads[index]->delEvent(fd, event);
}

void IoScheduler::delAllEvents(int fd){
    size_t index = getNextIndex();
    m_threads[index]->delAllEvents(fd);
}

TimerTask::ptr IoScheduler::addTimer(uint64_t time_ms, bool isrecurrent, std::function<void()> cb){
    size_t index = getNextIndex();
    // LOG_DEBUG << "add timer at index " << index;
    return m_threads[index]->addTimer(time_ms, isrecurrent, cb);
}

void IoScheduler::delTimer(TimerTask::ptr timer){
    for(auto& thread: m_threads){
        thread->delTimer(timer);
    }
}

void IoScheduler::stop(){
    m_closed = true;
    for(auto& thread: m_threads){
        thread->stop();
    }
}

}