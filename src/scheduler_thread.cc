#include "scheduler_thread.hpp"

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <string.h> 

#include "io_scheduler.hpp"
#include "logger.hpp"
#include "macro.hpp"

namespace furina{

static size_t s_epoll_max_events = 8;
static uint8_t s_epoll_event_expand_ratio = 2;

static thread_local SchedulerThread* s_this_thread_scheduler = nullptr;

SchedulerThread* getThisThreadSchedulerThread(){
    if(unlikely(s_this_thread_scheduler == nullptr)){
        LOG_FATAL << "getThisThreadSchedulerThread() why use this way???";
    }
    return s_this_thread_scheduler;
}

SchedulerThread::SchedulerThread(IoScheduler* scheduler)
        :m_stopped(false)
        ,m_num_ready_fibers(0)
        ,m_num_hold_fibers(0)
        ,m_scheduler(scheduler)
        ,Timer(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)){
    
    if(pipe(m_pipe)){
        LOG_FATAL << "SchedulerThread::SchedulerThread() pipe fail " << strerror(errno);
    }

    m_epoll = epoll_create(1013);
    if(m_epoll <= 0){
        LOG_FATAL << "SchedulerThread::SchedulerThread() epoll_create fail " << strerror(errno);
    }

    // tickle fd
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    event.data.fd = m_pipe[1];
    if(fcntl(m_pipe[0], F_SETFL, O_NONBLOCK | O_CLOEXEC)){
        LOG_FATAL << "SchedulerThread::SchedulerThread() pipe[0] fcntl fail " << strerror(errno);
    }
    if(epoll_ctl(m_epoll, EPOLL_CTL_ADD, m_pipe[0], &event)){
        LOG_FATAL << "SchedulerThread::SchedulerThread() epoll_ctl fail " << strerror(errno);
    }

    // timer fd
    struct epoll_event tevent;
    memset(&tevent, 0, sizeof(tevent));
    tevent.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    tevent.data.fd = m_timer_fd;
    if(epoll_ctl(m_epoll, EPOLL_CTL_ADD, m_timer_fd, &tevent)){
        LOG_FATAL << "SchedulerThread::SchedulerThread() epoll_ctl fail " << strerror(errno);
    }

    m_thread = std::shared_ptr<Thread>(new Thread(std::bind(&SchedulerThread::run, this)));
}

SchedulerThread::~SchedulerThread(){
    stop();
}

void SchedulerThread::onFiberReady(Fiber::ptr fiber){
    // LOG_DEBUG << "on fiber ready " << fiber.get();
    std::lock_guard<std::mutex> l(m_mtx);
    // 将fiber移动到ready队列
    for(auto it = m_hold_queue.begin(); it != m_hold_queue.end(); ++it){
        if(*it == fiber){
            // LOG_DEBUG << "find hold fiber at " << *it;
            m_ready_queue.emplace_back(std::move(*it));
            m_hold_queue.erase(it);
            m_num_ready_fibers++;
            m_num_hold_fibers--;
            break;
        }
    }
    tickle();
}

void SchedulerThread::onFiberHold(Fiber::ptr fiber){
    // // LOG_DEBUG << "on fiber hold " << fiber.get();
    // std::lock_guard<std::mutex> l(m_mtx);
    // // 将fiber移动到hold队列
    // for(auto it = m_ready_queue.begin(); it != m_ready_queue.end(); ++it){
    //     if(*it == fiber){
    //         m_hold_queue.emplace_back(std::move(*it));
    //         m_ready_queue.erase(it);
    //         m_num_ready_fibers--;
    //         m_num_hold_fibers++;
    //         break;
    //     }
    // }
}

void SchedulerThread::addTask(Fiber::ptr fiber){
    // LOG_DEBUG << "addTask";
    // 在不同线程下的执行，需要加锁
    std::lock_guard<std::mutex> l(m_mtx);
    m_ready_queue.push_back(fiber);
    ++m_num_ready_fibers;
    tickle();
}

void SchedulerThread::addTask(std::function<void()> cb){
    Fiber::ptr fiber = std::shared_ptr<Fiber>(new Fiber(cb, 
        std::bind(&SchedulerThread::onFiberReady, this, std::placeholders::_1), 
        std::bind(&SchedulerThread::onFiberHold, this, std::placeholders::_1)));
    addTask(fiber);
}

void SchedulerThread::run(){
    setRootFiber(nullptr);
    s_this_thread_scheduler = this;
    LOG_DEBUG << "RUN";
    setThisThreadScheduler(m_scheduler);
    m_stopped = false;
    Fiber::ptr worker_fiber = nullptr;
    Fiber::ptr idle_fiber = std::shared_ptr<Fiber>(new Fiber(std::bind(&SchedulerThread::idle, this)));
    while(!stopping()){
        // LOG_DEBUG << "ready queue size " << m_ready_queue.size();
        for(auto it = m_ready_queue.begin(); it != m_ready_queue.end(); ++it){
            if((*it)->getState() == Fiber::State::TERM || 
               (*it)->getState() == Fiber::State::EXCE){
                
                m_ready_queue.erase(it);
                continue;
            }else if((*it)->getState() == Fiber::State::EXEC){
                continue;
            }else if((*it)->getState() == Fiber::State::READY){
                worker_fiber.swap(*it);
                m_ready_queue.erase(it);
                break;
            }
        }
        if(worker_fiber != nullptr){
            // LOG_DEBUG << "get task at " << worker_fiber.get();
            worker_fiber->swapIn();
            if(worker_fiber->getState() == Fiber::State::TERM || 
                worker_fiber->getState() == Fiber::State::EXCE){
                
                m_num_ready_fibers--;
                // 什么也不做
            }else if(worker_fiber->getState() == Fiber::State::EXEC){
                // 再加入ready队列中
                worker_fiber->changeState(Fiber::State::READY);
                m_ready_queue.emplace_back(std::move(worker_fiber));
            }else if(worker_fiber->getState() == Fiber::State::READY){
                m_ready_queue.emplace_back(std::move(worker_fiber));
            }else if(worker_fiber->getState() == Fiber::State::HOLD){
                m_hold_queue.emplace_back(std::move(worker_fiber));
            }
            worker_fiber = nullptr;
        }else{
            idle_fiber->swapIn();
            if(idle_fiber->getState() == Fiber::State::EXCE ||
                idle_fiber->getState() == Fiber::State::TERM){

                stop();
            }else if(idle_fiber->getState() == Fiber::State::READY || 
                    idle_fiber->getState() == Fiber::State::EXEC){
            }
        }
    }
    idle_fiber->swapIn();
    idle_fiber.reset();
    worker_fiber.reset();
    destroyFiberWhenThreadEnd();
}

void SchedulerThread::tickle(){
    int n = write(m_pipe[1], "a", 1);
    if(n <= 0){
        LOG_ERROR << "SchedulerThread::tickle() ticlke fail " << strerror(errno);
    }
}

void SchedulerThread::idle(){
    // LOG_DEBUG << "idle";
    std::vector<struct epoll_event> events;
    events.resize(s_epoll_max_events);
    while(!stopping()){
        int n = epoll_wait(m_epoll, &(*events.begin()), events.size(), -1);
        if(n == -1){
            LOG_ERROR << "SchedulerThread::idle() epoll_wait fail " << strerror(errno);
            continue;
        }
        for(int i = 0; i < n; i++){

            if(events[i].data.fd == m_pipe[1]){
                char temp;
                while (true) {
                    ssize_t n = read(m_pipe[0], &temp, sizeof(char));
                    if (n > 0) {
                        // 处理 n 字节
                        break;
                    } else if (n == 0) {
                        // 对端关闭
                        LOG_ERROR << "write side close";
                        break;
                    } else {
                        if (errno == EINTR)
                            continue; // 信号中断重试
                        else if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // 缓冲区读完
                        else {
                            LOG_ERROR << "SchedulerThread::idle() read pipe fail " << strerror(errno);
                            break;
                        }
                    }
                }
                Fiber::YeildToReady();
                continue;
            }
            if(events[i].data.fd == m_timer_fd){
                uint64_t temp;
                while (true) {
                    ssize_t n = read(m_timer_fd, &temp, sizeof(uint64_t));
                    if (n > 0) {
                        break;
                    } else if (n == 0) {
                        // 对端关闭
                        LOG_ERROR << "fd close";
                        break;
                    } else {
                        if (errno == EINTR)
                            continue; // 信号中断重试
                        else if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // 缓冲区读完
                        else {
                            LOG_ERROR << "SchedulerThread::idle() read pipe fail " << strerror(errno);
                            break;
                        }
                    }
                }
                std::vector<TimerTask::ptr> tasks;
                getExpiredTasks(tasks);
                // LOG_DEBUG << "get " << tasks.size() << " timers";
                for(auto& task: tasks){
                    addTask(std::bind(&TimerTask::triggerTask, task));
                }
                continue;
            }
            
            int fd = events[i].data.fd;
            auto event_it = m_events.find(fd);
            if(event_it == m_events.end()){
                LOG_ERROR << "why not found???";
                continue;
            }
            event_it->second->setActualEvents(events[i].events);
            event_it->second->handleEvent();
        }
        if(n == events.size()){
            events.resize(n * s_epoll_event_expand_ratio);
        }
    }
    close(m_epoll);
    close(m_pipe[0]);
    close(m_pipe[1]);
    close(m_timer_fd);
}

void SchedulerThread::stop(){
    std::lock_guard<std::mutex> l(m_mtx);
    if(m_stopped){
        return;
    }
    std::vector<int> keys;
    for (auto& kv : m_events) {
        keys.push_back(kv.first);
    }

    for (int key : keys) {
        delAllEvents(key);
    }
    m_stopped = true;
    tickle();
}

bool SchedulerThread::stopping(){
    // return m_stopped == true && m_ready_queue.empty() && m_hold_queue.empty() && m_events.empty();
    return m_stopped;
}

void SchedulerThread::addEvent(int fd, int event, std::function<void()> cb){
    auto it = m_events.find(fd);
    EventContext::ptr ctx = nullptr;
    int epoll_operation = 0;
    if(it == m_events.end()){
        ctx = std::shared_ptr<EventContext>(new EventContext(fd, this));
        m_events.insert_or_assign(fd, ctx);
        epoll_operation = EPOLL_CTL_ADD;
    }else{
        ctx = it->second;
        epoll_operation = EPOLL_CTL_MOD;
    }

    struct epoll_event epollevent;
    memset(&epollevent, 0, sizeof(epollevent));
    epollevent.events = ctx->registerd_events | event | EPOLLET;
    epollevent.data.fd = fd;
    if(epoll_ctl(m_epoll, epoll_operation, fd, &epollevent)){
        LOG_ERROR << "SchedulerThread::addEvent() epoll_ctl fail " << strerror(errno) << " fd=" << fd;
    }else{
        if(event & EventContext::Event::READ){
            ctx->read_task = cb;
        }else if(event & EventContext::Event::WRITE){
            ctx->write_task = cb;
        }
        ctx->registerd_events |= event;
        ctx->fd = fd;
    }
}

void SchedulerThread::delEvent(int fd, int event){
    auto it = m_events.find(fd);
    if(it == m_events.end()){
        return;
    }
    EventContext::ptr ctx = it->second;
    if((ctx->registerd_events & event) == 0){
        return;
    }

    int epoll_operation = 0;
    if((ctx->registerd_events & ~event) == 0){
        epoll_operation = EPOLL_CTL_DEL;
        m_events.erase(fd);
    }else{
        epoll_operation = EPOLL_CTL_MOD;
    }

    struct epoll_event epollevent;
    memset(&epollevent, 0, sizeof(epollevent));
    epollevent.events = (ctx->registerd_events & ~event) | EPOLLET;
    epollevent.data.fd = ctx->fd;
    
    if(epoll_ctl(m_epoll, epoll_operation, ctx->fd, &epollevent)){
        LOG_ERROR << "SchedulerThread::delEvent() epoll_ctl fail " << strerror(errno);
    }else{
        if(event & EventContext::Event::READ){
            ctx->read_task = nullptr;
        }else if(event & EventContext::Event::WRITE){
            ctx->write_task = nullptr;
        }
        ctx->registerd_events &= ~event;
    }
}

void SchedulerThread::delAllEvents(int fd){
    delEvent(fd, EventContext::Event::READ);
    delEvent(fd, EventContext::Event::WRITE);
}


//////////////////////////////////////////////////////////////////////////////////

SchedulerThread::EventContext::EventContext(int fd_, SchedulerThread* thread_)
    :fd(fd_)
    ,thread(thread_){}

void SchedulerThread::EventContext::setActualEvents(int events){
    actual_events = events;
}

void SchedulerThread::EventContext::handleEvent(){
    LOG_DEBUG << "event " << actual_events << " on fd = " << fd; 
    if((actual_events & READ) && (registerd_events & READ)){
        thread->addTask(read_task);
        thread->delEvent(fd, READ);
    }
    if((actual_events & WRITE) && (registerd_events & WRITE)){
        thread->addTask(write_task);
        thread->delEvent(fd, WRITE);
    }
    if((actual_events & EPOLLERR) | (actual_events & EPOLLHUP)){
        thread->delAllEvents(fd);
    }
}

}