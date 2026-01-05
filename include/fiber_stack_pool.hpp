#pragma once

#include <memory>
#include <mutex>
#include <sys/mman.h>

#include "memory_pool.hpp"


namespace furina{

class FiberStackPool;

static thread_local std::shared_ptr<FiberStackPool> t_fiber_stack_pool = nullptr;

#define PTR_LENTH sizeof(void*)

static const size_t FIBER_STACK_SIZE = 1024 * 64 + PTR_LENTH; // 64 KB + 8B
static const size_t FIBER_STACK_POOL_SIZE = 1;
static const size_t FIBER_STACK_GROW_SIZE = 16;

class FiberStackAllocatorMmap{
public:
    static void* allocate(size_t size){
        return mmap(nullptr, size, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    }

    static int deallocate(void* ptr, size_t size){
        return munmap(ptr, size);
    }
};


class FiberStackPool {
public:

    using MemoryPool = TempMemoryPool;
    using StackAllocator = FiberStackAllocatorMmap;

    static std::shared_ptr<FiberStackPool> getInstance();

    void* acquireStack();
    void releaseStack(void* stack_ptr);

    ~FiberStackPool();
private:
    FiberStackPool();
    
    FiberStackPool(const FiberStackPool&) = delete;
    FiberStackPool& operator=(const FiberStackPool&) = delete;

    void createSomeStacks();

    struct StackBlock{
        void* stack_ptr;
        StackBlock* next;
    };

    struct ChunkBlock{
        void* chunk_ptr;
        ChunkBlock* next;
    };

    StackBlock* m_used_stack_blocks;
    StackBlock* m_used_stack_blocks_tail;
    StackBlock* m_free_stack_blocks;
    StackBlock* m_free_stack_blocks_tail;
    ChunkBlock* m_chunk_blocks;
    size_t m_chunks;
    size_t m_total_stacks;
    size_t m_used_stacks;

public:
    size_t m_total_mmap_times = 0;

};

}