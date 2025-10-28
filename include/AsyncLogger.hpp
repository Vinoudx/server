#ifndef __ASYNC_LOGGER__
#define __ASYNC_LOGGER__

#include <vector>
#include <mutex>
#include <condition_variable>
#include <string>
#include <memory>
#include <thread>

#include "utils.hpp"

namespace furina{
class vecBuffer{
public:
    using ptr_t = std::unique_ptr<vecBuffer>;
    vecBuffer();
    void push(int fd, const std::string& content);
    void push(const std::pair<int, std::string>& item);
    bool isFull(){return buffer_.size() >= capacity;}
    void setEmpty(){remaining_ = capacity;buffer_.clear();buffer_.reserve(capacity);}
    std::vector<std::pair<int, std::string>>& getBuffer(){return buffer_;}
private:
    size_t remaining_;
    size_t capacity;
    std::vector<std::pair<int, std::string>> buffer_;
};

class AsyncLogger: public Singleton<AsyncLogger>{
    friend class Singleton<AsyncLogger>;
public:    
    void log(int fd, const std::string& content);
    void threadFunc();
    void flush();
    ~AsyncLogger();
private:
    AsyncLogger();
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;
    AsyncLogger(AsyncLogger&&) = delete;

    bool stop_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::vector<vecBuffer::ptr_t> fullBuffers_;
    std::unique_ptr<std::thread> thread_;

    vecBuffer::ptr_t bufferA_;
    vecBuffer::ptr_t bufferB_;
};

}
#endif