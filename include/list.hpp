#pragma once

#include "memory_pool.hpp"

namespace furina{

template<typename T, typename Allocator = TempMemoryPool>
class FiberStackList{
public:

    List():m_data(nullptr),m_tail(nullptr),m_size(0){}

    template<typename... Args>
    const void* emplace_back(Args&&... data){
        Node* node = Allocator::allocate(sizeof(Node));
        Allocator::construct<Node>(node, m_data, std::forward<Args>(args)...);
        if(m_data == nullptr){
            node->next = node;
            m_data = node;
        }else{
            node->next = m_data;
            m_tail->next = node;
        }
        m_tail = node;
        ++m_size;
        return (void*)node;
    }

    T pop_back(){
        if(m_size == 0)return T();
        if(m_size == 1){
            T temp = m_data->data;
            Allocator::deallocate<Node>(m_data);
        }else{

        }
    }

    void pop_at(void* node){

    }
    

private:

    struct Node{
        Node* next;        
        T data;
    };

    Node* m_data;
    Node* m_tail;
    size_t m_size;
};

}