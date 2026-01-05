#include "fiber_stack_pool.hpp"
#include <iostream>
#include <list>
#include <random>
#include <chrono>

#include "logger.hpp"

using namespace std;
using namespace furina;

void stackPool(){
    list<void*> v;
    auto random_d = std::random_device();
    std::mt19937 gen(random_d());
    std::uniform_int_distribution<int> dis(0, 1);

    int allocate_times = 0;
    int deallocate_times = 0;

    auto clock = std::chrono::high_resolution_clock::now();

    for(int i = 0; i < 1000000; i++){
        int r = dis(gen);
        if(r == 0){
            v.push_front(FiberStackPool::getInstance()->acquireStack());
            ++allocate_times;
        }else if(r == 1 && v.size() > 0){
            FiberStackPool::getInstance()->releaseStack(v.back());
            v.pop_back();
            ++deallocate_times;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - clock);
    std::cout << "Time taken: " << duration.count() << " microseconds, allocate "  << allocate_times << " , deallocate "  << deallocate_times << std::endl;
    std::cout << "mmap times: " << FiberStackPool::getInstance()->m_total_mmap_times << std::endl;
}

void mallocMethod(){
    list<void*> v;
    auto random_d = std::random_device();
    std::mt19937 gen(random_d());
    std::uniform_int_distribution<int> dis(0, 1);

    int allocate_times = 0;
    int deallocate_times = 0;

    auto clock = std::chrono::high_resolution_clock::now();

    for(int i = 0; i < 1000000; i++){
        int r = dis(gen);
        if(r == 0){
            void* p = malloc(64 * 1024 + 8);
            v.push_front(p);
            ++allocate_times;
        }else if(r == 1 && v.size() > 0){
            void* p = v.back();
            free(p);
            v.pop_back();
            ++deallocate_times;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - clock);
    std::cout << "Time taken: " << duration.count() << " microseconds, allocate "  << allocate_times << " , deallocate "  << deallocate_times << std::endl;
    std::cout << "mmap times: " << FiberStackPool::getInstance()->m_total_mmap_times << std::endl;
}

int main(){
    stackPool();
    mallocMethod();

}