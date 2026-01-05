#include "fiber_stack_pool.hpp"

#include <sys/mman.h>
#include <string.h>

#include "logger.hpp"

namespace furina{

std::shared_ptr<FiberStackPool> FiberStackPool::getInstance() {
    if(t_fiber_stack_pool == nullptr)[[unlikely]]{
        t_fiber_stack_pool = std::shared_ptr<FiberStackPool>(new FiberStackPool());
    }
    return t_fiber_stack_pool;
}

FiberStackPool::FiberStackPool()
    : m_used_stack_blocks(nullptr),
      m_free_stack_blocks(nullptr),
      m_free_stack_blocks_tail(nullptr),
      m_used_stack_blocks_tail(nullptr),
      m_chunk_blocks(nullptr),
      m_chunks(0),
      m_total_stacks(0),
      m_used_stacks(0) {

    for(size_t i = 0; i < FIBER_STACK_POOL_SIZE; ++i){
        void* stack_chunk = StackAllocator::allocate(FIBER_STACK_SIZE * FIBER_STACK_GROW_SIZE);
        m_total_mmap_times++;
        if(stack_chunk == nullptr)[[unlikely]]{
            LOG_ERROR << "FiberStackPool::FiberStackPool() Failed to allocate fiber stack";
            continue;  
        }
        ChunkBlock* chunk = (ChunkBlock*)MemoryPool::allocate(sizeof(ChunkBlock));
        MemoryPool::construct<ChunkBlock>(chunk, stack_chunk, m_chunk_blocks);
        m_chunk_blocks = chunk;
        ++m_chunks;

        for(size_t j = 0; j < FIBER_STACK_GROW_SIZE * FIBER_STACK_SIZE; j += FIBER_STACK_SIZE){
            void* stack = (void*)((char*)stack_chunk + j);
            StackBlock* block = (StackBlock*)MemoryPool::allocate(sizeof(StackBlock));
            MemoryPool::construct<StackBlock>(block, ((char*)stack + PTR_LENTH), m_free_stack_blocks);
            if(m_free_stack_blocks == nullptr){
                block->next = block;
                m_free_stack_blocks = block;
            }else{
                m_free_stack_blocks_tail->next = block;
            }
            m_free_stack_blocks_tail = block;
            ++m_total_stacks;
            // *(StackBlock**)((char*)block->stack_ptr - PTR_LENTH) = block;
            *reinterpret_cast<StackBlock**>(stack) = block;
        }

    }

}

FiberStackPool::~FiberStackPool() {
    StackBlock* current = m_free_stack_blocks;
    StackBlock* previous = nullptr;
    int rt = 0;
    while(previous != m_free_stack_blocks_tail){
        previous = current;
        current = current->next;
        MemoryPool::destroy<StackBlock>(previous);
        MemoryPool::deallocate(previous, sizeof(StackBlock));
    }
    current = m_used_stack_blocks;
    previous = nullptr;
    while(previous != m_used_stack_blocks_tail){
        previous = current;
        current = current->next;
        MemoryPool::destroy<StackBlock>(previous);
        MemoryPool::deallocate(previous, sizeof(StackBlock));
    }

    ChunkBlock* cur = m_chunk_blocks;
    while(cur != nullptr){
        StackAllocator::deallocate(cur->chunk_ptr, FIBER_STACK_GROW_SIZE * FIBER_STACK_SIZE);
        ChunkBlock* temp = cur;
        cur = cur->next;
        MemoryPool::destroy<ChunkBlock>(temp);
        MemoryPool::deallocate(temp, sizeof(ChunkBlock));
    }
}

void* FiberStackPool::acquireStack() {
    if(m_total_stacks - m_used_stacks == 0 || m_free_stack_blocks == nullptr){
        createSomeStacks();
    }
    StackBlock* temp = m_free_stack_blocks;
    if(m_free_stack_blocks == m_free_stack_blocks_tail){
        m_free_stack_blocks = nullptr;
        m_free_stack_blocks_tail = nullptr;
    }else{
        m_free_stack_blocks = temp->next;
        m_free_stack_blocks_tail->next = m_free_stack_blocks;
    }
    if(m_used_stack_blocks == nullptr){
        temp->next = temp;
        m_used_stack_blocks = temp;
        m_used_stack_blocks_tail = temp;
    }else{
        temp->next = m_used_stack_blocks;
        m_used_stack_blocks_tail->next = temp;
        m_used_stack_blocks_tail = temp;
    }
    ++m_used_stacks;
    return temp->stack_ptr;
}

void FiberStackPool::releaseStack(void* stack_ptr) {
    StackBlock* current_block = *(StackBlock**)((char*)stack_ptr - PTR_LENTH);
    StackBlock* block_to_move = nullptr;
    if(m_used_stacks == 1){
        // if(m_used_stack_blocks->stack_ptr != current_block->stack_ptr){
        //     LOG_ERROR << "FiberStackPool::releaseStack() Tried to release a stack that is not in use.";
        //     return;
        // }
        block_to_move = m_used_stack_blocks;
        m_used_stack_blocks = nullptr;
        m_used_stack_blocks_tail = nullptr;
    }else{
        block_to_move = current_block->next;
        std::swap(current_block->stack_ptr, current_block->next->stack_ptr);
        current_block->next = current_block->next->next;
        if(m_used_stack_blocks_tail == current_block){
            m_used_stack_blocks = m_used_stack_blocks_tail->next;
        }
        if(m_used_stack_blocks_tail == block_to_move){
            m_used_stack_blocks_tail = current_block;
        }
    }

    if(m_free_stack_blocks == nullptr){
        block_to_move->next = block_to_move;
        m_free_stack_blocks = block_to_move;
        m_free_stack_blocks_tail = block_to_move;
    }else{
        block_to_move->next = m_free_stack_blocks;
        m_free_stack_blocks_tail->next = block_to_move;
        m_free_stack_blocks_tail = block_to_move;
    }
    --m_used_stacks;
}

void FiberStackPool::createSomeStacks() {
    void* stack_chunk = StackAllocator::allocate(FIBER_STACK_SIZE * FIBER_STACK_GROW_SIZE);
    m_total_mmap_times++;
    if(stack_chunk == nullptr)[[unlikely]]{
        LOG_FATAL << "FiberStackPool::FiberStackPool() Failed to allocate fiber stack";
        abort();
    }
    ChunkBlock* chunk = (ChunkBlock*)MemoryPool::allocate(sizeof(ChunkBlock));
    MemoryPool::construct<ChunkBlock>(chunk, stack_chunk, m_chunk_blocks);
    m_chunk_blocks = chunk;
    ++m_chunks;

    for(size_t j = 0; j < FIBER_STACK_GROW_SIZE * FIBER_STACK_SIZE; j += FIBER_STACK_SIZE){
        void* stack = (void*)((char*)stack_chunk + j);
        StackBlock* block = (StackBlock*)MemoryPool::allocate(sizeof(StackBlock));
        MemoryPool::construct<StackBlock>(block, ((char*)stack + PTR_LENTH), m_free_stack_blocks);
        if(m_free_stack_blocks == nullptr){
            block->next = block;
            m_free_stack_blocks = block;
        }else{
            m_free_stack_blocks_tail->next = block;
        }
        m_free_stack_blocks_tail = block;
        ++m_total_stacks;
        // *(StackBlock**)((char*)block->stack_ptr - PTR_LENTH) = block;
        *reinterpret_cast<StackBlock**>(stack) = block;
    }
}

}
