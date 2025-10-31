#include "fiber.hpp"
#include <thread>
#include "io_scheduler.hpp"

#include "logger.hpp"

using namespace furina;

// void work1(){
//     for(int i=0;i<10;i++){
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//         LOG_INFO << "call work, times = " << i;
//         // Fiber::yeildToReady();
//         // fib->swapOut();
//     }
// }

void work2(){
    LOG_INFO << "work2() before yeild";
    auto fiber = getExecFiber();
    // LOG_DEBUG << "get fiber at " << fiber.get();
    std::thread t([fiber]{
        std::this_thread::sleep_for(std::chrono::seconds(1));
        fiber->resume();
    });
    t.detach();
    LOG_INFO << "work2() working";
    Fiber::YeildToHold();
    LOG_INFO << "work2() after yeild";
}

void work3(){
    LOG_INFO << "work3() before yeild";
    auto fiber = getExecFiber();
    std::thread t([fiber]{
        std::this_thread::sleep_for(std::chrono::seconds(1));
        fiber->resume();
    });
    t.detach();
    LOG_INFO << "work3() working";
    Fiber::YeildToHold();
    LOG_INFO << "work3() after yeild";
}


void run_in_fiber(){
    LOG_INFO<<"fiber begin";
    Fiber::YeildToHold();
    LOG_INFO<<"fiber end";
    Fiber::YeildToHold();
}

void onReady(Fiber::ptr fiber){
    LOG_INFO << fiber->getId() << "ready";
} 

void onHold(Fiber::ptr fiber){
    LOG_INFO << fiber->getId() << "hold";
}

void timerfunc(){
    LOG_INFO << "timer tick";
}

int main(){
    // {    
    // setRootFiber(nullptr);
    // Fiber::ptr main_fiber(new Fiber(run_in_fiber, onReady, onHold));
    // LOG_INFO<<"main begin"; 
    // main_fiber->swapIn();
    // LOG_INFO<<"fiber swapout";
    // main_fiber->swapIn();
    // LOG_INFO<<"main end";
    // main_fiber->swapIn();}
    // LOG_INFO<<"main end2";

    IoScheduler sc(5);
    LOG_INFO << "after start";
    sc.start();    
    sc.schedule(work2);
    sc.schedule(work3);

    // auto timer = sc.addTimer(1000, true, &timerfunc);
    // sc.addTimer(5005, false, [timer, &sc]{
    //     LOG_INFO << "delete timer";
    //     sc.delTimer(timer);
    // });

    // // system("pause");
    std::this_thread::sleep_for(std::chrono::seconds(10));
    LOG_INFO << "stop";
    sc.stop();
    return 0;
}