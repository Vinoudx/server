#pragma once 

#include <mutex>
#include <memory>
#include <sys/syscall.h>

namespace furina{

enum Event{
    NONE = 0x000,
    READ = 0x001,
    WRITE = 0x004
};

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

class Noncopyable{
    Noncopyable() = delete;
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};

template<typename T>
class Singleton{
public:
    static std::shared_ptr<T> getInstance(){
        std::call_once(m_flag, []{
            m_instance = std::shared_ptr<T>(new T());
        });
        return m_instance;
    }
private:
    static std::shared_ptr<T> m_instance;
    static std::once_flag m_flag;
};

template<typename T>
std::shared_ptr<T> Singleton<T>::m_instance = nullptr;

template<typename T>
std::once_flag Singleton<T>::m_flag;

size_t getThreadId();
size_t getFiberId();

} 
