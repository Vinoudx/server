#include "thread.hpp"

#include "logger.hpp"
#include "utils.hpp"

namespace furina{

Thread::Thread(Callback cb)
        :m_cb(cb){
    sem_init(&m_sem, 0, 0);
    m_thread = std::make_shared<std::thread>(std::bind(&Thread::threadMainFunc, this));
    sem_wait(&m_sem);
}

Thread::~Thread(){
    if(m_thread->joinable()){
        m_thread->join();
    }
    m_cb = nullptr;
    sem_destroy(&m_sem);
    LOG_DEBUG << "thread exit";
}

void Thread::threadMainFunc(){
    m_id = getThreadId();
    sem_post(&m_sem);
    m_cb();
}

};