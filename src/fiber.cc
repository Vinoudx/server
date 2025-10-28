#include "fiber.hpp"

#include <string.h>

#include "logger.hpp"
#include "utils.hpp"
#include "macro.hpp"

namespace furina{

static thread_local Fiber::ptr t_this_thread_root_fiber = nullptr;
static thread_local Fiber::ptr t_executing_fiber = nullptr;

// static thread_local Fiber* t_this_thread_root_fiber = nullptr;
// static thread_local Fiber* t_executing_fiber = nullptr;

static size_t s_stack_size = 1024 * 1024;

Fiber::ptr getRootFiber(){
    return t_this_thread_root_fiber;
}

void setRootFiber(Fiber::ptr fiber){
    if(likely(fiber != nullptr)){
        t_this_thread_root_fiber = fiber;
    }else{
        t_this_thread_root_fiber = std::shared_ptr<Fiber>(new Fiber());
        t_executing_fiber = t_this_thread_root_fiber;
    }
}

void destroyFiberWhenThreadEnd(){
    t_this_thread_root_fiber.reset();
    t_executing_fiber.reset();
}

Fiber::ptr getExecFiber(){
    return t_executing_fiber;
}

void setExecFiber(Fiber::ptr fiber){
    t_executing_fiber = fiber;
}

Fiber::Fiber():m_state(READY),m_stack(nullptr){
    m_id = s_fiberid_generator++;
    if(getcontext(&m_context)){
        LOG_ERROR << "Fiber::Fiber() " << strerror(errno);
    }
    LOG_DEBUG << "Fiber::Fiber() main fiber " << m_id; 
}

Fiber::Fiber(std::function<void()> cb, StateListenerCallback rcb, StateListenerCallback bcb)
            :m_cb(cb)
            ,m_on_block_listenercb(bcb)
            ,m_on_ready_listenercb(rcb)
            ,m_id(s_fiberid_generator++)
            ,m_state(INIT)
            ,m_stack_size(s_stack_size){
        
    m_stack = malloc(m_stack_size);
    
    if(getcontext(&m_context)){
        LOG_ERROR << "Fiber::Fiber() " << strerror(errno);
    }

    m_context.uc_stack.ss_size = m_stack_size;
    m_context.uc_stack.ss_sp = m_stack;
    m_context.uc_link = nullptr;

    makecontext(&m_context, &Fiber::mainFunc, 0);
    LOG_DEBUG << "Fiber " << m_id << " created and start";
    changeState(READY);
}

Fiber::~Fiber(){
    changeState(TERM);
    free(m_stack);
    m_cb = nullptr;
    m_on_block_listenercb = nullptr;
    m_on_ready_listenercb = nullptr;
    LOG_DEBUG << "Fiber::~Fiber() id = " << m_id;
}

void Fiber::swapIn(){
    Fiber::ptr root_fiber = getRootFiber();
    Fiber* row_ptr = root_fiber.get();
    root_fiber.reset();
    setExecFiber(shared_from_this());
    changeState(EXEC);
    swapcontext(&row_ptr->m_context, &m_context);
}

void Fiber::swapOut(){
    ASSERT2(t_this_thread_root_fiber != nullptr, "we have no root fiber yet");
    if(m_state == EXEC){
        changeState(READY);
    }
    Fiber::ptr root_fiber = getRootFiber();
    Fiber* row_ptr = root_fiber.get();
    setExecFiber(root_fiber);
    root_fiber.reset();
    swapcontext(&m_context, &row_ptr->m_context);
}

void Fiber::resume(){
    if(m_state != TERM && m_state != EXCE){
        changeState(READY);
    }
}

void Fiber::reset(std::function<void()> cb, StateListenerCallback rcb, StateListenerCallback bcb){
    m_cb = cb;
    m_on_block_listenercb = bcb;
    m_on_ready_listenercb = rcb;
    if(!getcontext(&m_context)){
        LOG_ERROR << "Fiber::Fiber() " << strerror(errno);
    }

    m_context.uc_stack.ss_size = m_stack_size;
    m_context.uc_stack.ss_sp = m_stack;
    m_context.uc_link = nullptr;

    makecontext(&m_context, &Fiber::mainFunc, 0);
    LOG_DEBUG << "Fiber " << m_id << " reset";
    changeState(READY);
}

void Fiber::YeildToHold(){
    Fiber::ptr now_fiber = getExecFiber();
    Fiber* row_ptr = now_fiber.get();
    now_fiber.reset();
    now_fiber->changeState(HOLD);
    row_ptr->swapOut();
}

void Fiber::YeildToReady(){
    Fiber::ptr now_fiber = getExecFiber();
    Fiber* row_ptr = now_fiber.get();
    now_fiber->changeState(READY);
    row_ptr->swapOut();
}

void Fiber::mainFunc(){
    Fiber::ptr now_fiber = getExecFiber();
    now_fiber->changeState(EXEC);
    try{
        if(now_fiber->m_cb){
            now_fiber->m_cb();
        }
        now_fiber->m_cb = nullptr;
    }catch(std::exception& e){
        now_fiber->changeState(EXCE);
        LOG_ERROR << "Fiber::mainFunc() unhandled exception " << e.what();
        throw e;
    }catch(...){
        now_fiber->changeState(EXCE);
        LOG_FATAL << "Fiber::mainFunc() unhandled unkown except";
        throw;
    }
    now_fiber->changeState(TERM);
    Fiber* row_ptr = now_fiber.get();
    now_fiber.reset();
    row_ptr->swapOut();
}

void Fiber::changeState(State state){
    State pre_state = m_state;
    m_state = state;
    // LOG_DEBUG << "pre state = " << pre_state << " now state = " << m_state;
    if(pre_state == HOLD && m_state == READY && m_on_ready_listenercb){
        m_on_ready_listenercb(shared_from_this());
    }
    if(m_state == HOLD && m_on_block_listenercb){
        m_on_block_listenercb(shared_from_this());
    }
}


}