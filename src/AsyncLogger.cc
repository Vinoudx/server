#include "AsyncLogger.hpp"

#include <iostream>
namespace furina{

vecBuffer::vecBuffer():capacity(16),remaining_(capacity){
    buffer_.reserve(capacity);
}

void vecBuffer::push(int fd, const std::string& content){
    push(std::make_pair(fd, content));
}

void vecBuffer::push(const std::pair<int, std::string>& item){
    if(!isFull()){
        remaining_--;
        buffer_.emplace_back(std::move(item));
    }
}


AsyncLogger::AsyncLogger():stop_(false){
    fullBuffers_.reserve(8);
    bufferA_ = std::make_unique<vecBuffer>();
    bufferB_ = std::make_unique<vecBuffer>();
    thread_ = std::make_unique<std::thread>([this]{threadFunc();});
}

AsyncLogger::~AsyncLogger(){
    {
        std::unique_lock<std::mutex> l(mtx_);
        stop_ = true;
        if (bufferA_ && !bufferA_->getBuffer().empty()) {
            fullBuffers_.emplace_back(std::move(bufferA_));
            if(bufferB_ != nullptr){
                bufferA_.swap(bufferB_);
            }else{
                bufferA_ = std::make_unique<vecBuffer>();
            }
        }
    }
    cv_.notify_one();
    thread_->join();
}

void AsyncLogger::log(int fd, const std::string& content){
    std::unique_lock<std::mutex> l(mtx_);
    bufferA_->push(fd, content);    
    if(bufferA_->isFull()){
        fullBuffers_.emplace_back(std::move(bufferA_));
        if(bufferB_ != nullptr){
            bufferA_.swap(bufferB_);
        }else{
            bufferA_ = std::make_unique<vecBuffer>();
        }
        cv_.notify_one();
    }
}

void AsyncLogger::flush(){
    {
        std::unique_lock<std::mutex> l(mtx_);
        if (bufferA_ && !bufferA_->getBuffer().empty()) {
            fullBuffers_.emplace_back(std::move(bufferA_));
            if(bufferB_ != nullptr){
                bufferA_.swap(bufferB_);
            }else{
                bufferA_ = std::make_unique<vecBuffer>();
            }
        }
    }
    cv_.notify_one(); 
}


void AsyncLogger::threadFunc(){
    std::unique_ptr<vecBuffer> bufferC_;
    std::unique_ptr<vecBuffer> bufferD_;
    std::vector<vecBuffer::ptr_t> buffers(8);

    while(!(stop_ && fullBuffers_.empty())){
        std::unique_lock<std::mutex> l(mtx_);
        bool isTimeOut = cv_.wait_for(l, std::chrono::milliseconds(200), [this]{
            return !fullBuffers_.empty() || stop_;
        });

        if(isTimeOut){
            // 超时直接搞
            if (bufferA_ && !bufferA_->getBuffer().empty()) {
                fullBuffers_.emplace_back(std::move(bufferA_));
                if(bufferB_ != nullptr){
                    bufferB_.swap(bufferA_);
                }else{
                    bufferA_ = std::make_unique<vecBuffer>();
                }
            }
        }

        // 把C或者D给B
        if(bufferC_ != nullptr){
            bufferB_.swap(bufferC_);
        }
        if(bufferD_ != nullptr){
            bufferC_.swap(bufferD_);
        }
        // 拿到buffers
        buffers.swap(fullBuffers_);
        
        // 解锁，开始输出
        l.unlock();
        for(auto& item: buffers){
            if(!item)continue;
            for(auto& contents: item->getBuffer()){
                ssize_t n = write(contents.first, contents.second.c_str(), contents.second.size());
            }
            item->setEmpty();
            if(bufferC_ == nullptr){
                bufferC_.swap(item);
            }else if(bufferD_ == nullptr){
                bufferD_.swap(item);
            }
        }
        buffers.clear();

    }

}

}