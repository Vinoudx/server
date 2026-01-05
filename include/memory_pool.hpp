#pragma once

#include <memory>

namespace furina{

class TempMemoryPool{
public:
    static void* allocate(size_t size){
        return ::malloc(size);
    }
    static void deallocate(void* ptr, size_t size){
        ::free(ptr);
    }
    
    template<typename T, typename... Args>
    static void construct(void* ptr, Args&&... args){
        new (ptr) T(std::forward<Args>(args)...);
    }

    template<typename T>
    static void destroy(void* ptr){
        if constexpr (!std::is_trivially_destructible_v<T>) {
        reinterpret_cast<T*>(ptr)->~T();
    }
    }
};

/*
仿照nginx内存池
分小块内存和大块内存

*/

class MemoryPool{
public:
    using ptr = std::shared_ptr<MemoryPool>;
private:

    struct BigBlock{
        size_t size;
        BigBlock* next;
        void* data;
    };

    struct BlockHead{
        size_t block_size;
        BigBlock* big_block_chain;
        BlockHead* next_block;
        void* free_begin;
        void* block_end;
    };

    struct SmallBlock{
        BlockHead head;
        
    };
    

};

}