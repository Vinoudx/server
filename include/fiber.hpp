#pragma once

#include <ucontext.h>
#include <memory>
#include <functional>
#include <atomic>

namespace furina{

class Fiber;

static std::atomic<size_t> s_fiberid_generator = 0;

class Fiber: public std::enable_shared_from_this<Fiber>{
public:
    using ptr = std::shared_ptr<Fiber>;
    using StateListenerCallback = std::function<void(Fiber::ptr)>;

    enum State{
        INIT = 1,   
        READY = 2,
        EXEC = 3,   
        HOLD = 4,
        TERM = 5,
        EXCE = 6,   // 异常
    };
    Fiber();
    explicit Fiber(std::function<void()> cb, StateListenerCallback rcb = nullptr, StateListenerCallback bcb = nullptr);
    ~Fiber();
    void swapIn();
    void swapOut();
    void reset(std::function<void()> cb, StateListenerCallback rcb, StateListenerCallback bcb);

    size_t getId(){return m_id;}

    static void YeildToHold();
    static void YeildToReady();

    static void mainFunc();

    void changeState(State state);
    State getState(){return m_state;}
    void resume();

private:
    ucontext_t m_context;
    size_t m_id;
    State m_state;
    std::function<void()> m_cb;
    StateListenerCallback m_on_block_listenercb;
    StateListenerCallback m_on_ready_listenercb;
    void* m_stack;
    size_t m_stack_size;
};

Fiber::ptr getRootFiber();
void setRootFiber(Fiber::ptr fiber);
Fiber::ptr getExecFiber();
void setExecFiber(Fiber::ptr Fiber);
void destroyFiberWhenThreadEnd();

};