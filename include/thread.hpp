#pragma once

#include <memory>
#include <thread>
#include <semaphore.h>
#include <functional>

namespace furina{

class Thread{
public:
    using ptr = std::shared_ptr<Thread>;
    using Callback = std::function<void()>;

    explicit Thread(Callback cb);
    ~Thread();

    size_t getId() const{return m_id;}

private:

    void threadMainFunc();

    std::shared_ptr<std::thread> m_thread;
    size_t m_id;
    sem_t m_sem;
    Callback m_cb;
};

}
