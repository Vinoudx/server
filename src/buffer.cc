#include "buffer.hpp"

#include <string.h>
#include <string>

namespace furina{

Buffer::Buffer(size_t defaultCapacity)
    :m_capacity(defaultCapacity)
    ,m_buffer(new char[defaultCapacity])
    ,m_readBegin(0)
    ,m_writeBegin(0){

}

Buffer::~Buffer(){
    delete[] m_buffer;
    m_readBegin = 0;
    m_writeBegin = 0;
}

void Buffer::resize(size_t n){
    size_t nBytes = readableBytes();
    char* temp = new char[n];
    memcpy(temp, m_buffer, m_capacity);
    m_capacity = n;
    m_writeBegin = advance(m_readBegin, nBytes);
}

size_t Buffer::advance(size_t ptr, size_t n){
    if(n < 0)return ptr;
    return (ptr + n) % m_capacity;
}

void Buffer::writeString(const std::string& str){
    writeUint64((uint64_t)str.size());
    write(str.c_str(), str.size());
}

std::string Buffer::readString(){
    uint64_t len = readUint64();
    std::string temp;
    temp.resize(len + 1);
    read(temp.data(), len);
    return temp;
}

void Buffer::writeUint64(uint64_t value){
    // value = byteswapToNetEndian(value);
    char temp[10];
    int i = 0;
    while(value >= 0x80){
        temp[i++] = ((value & 0x7f) | 0x80);
        value >>= 7;
    }
    temp[i++] = value & 0x7f;
    write(temp, i);
}

uint64_t Buffer::readUint64(){
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7){
        uint8_t temp = 0;
        read((char*)&temp, 1);
        if(temp < 0x80){
            result |= ((uint64_t)temp) << i;
            break;
        }else{
            result |= ((uint64_t)temp & 0x7f) << i;
        }
    }
    // return byteswapToHostEndian(result);
    return result;
}

size_t Buffer::readableBytes(){
    if(m_readBegin <= m_writeBegin){
        return m_writeBegin - m_readBegin;
    }else{
        return (m_writeBegin + m_capacity) - m_readBegin;
    }
}

size_t Buffer::writeableBytes(){
    return m_capacity - readableBytes() - 1;
}

void Buffer::write(const void* buf, size_t size){
    if(writeableBytes() < size)[[unlikely]]{
        // 以1kB为单位加容量
        resize(m_capacity + ((size % 1024) + 1) * 1024);
    }
    if(m_capacity - m_writeBegin >= size){
        // 到结尾写的下
        memcpy(m_buffer + m_writeBegin, buf, size);
        m_writeBegin = advance(m_writeBegin, size);
    }else{
        // 到结尾写不下
        size_t toEndSpace = m_capacity - m_writeBegin;

        // 先写直到结尾
        memcpy(m_buffer + m_writeBegin, buf, toEndSpace);
        m_writeBegin = advance(m_writeBegin, toEndSpace);
        // 再写剩下的
        memcpy(m_buffer + m_writeBegin, (const char*)buf + toEndSpace, size - toEndSpace);
        m_writeBegin = advance(m_writeBegin, size - toEndSpace);
    }
}

void Buffer::read(void* buf, size_t size){
    if(readableBytes() < size)[[unlikely]]{
        throw std::logic_error("no enough data");
    }
    if(m_capacity - m_readBegin >= size){
        // 到结尾读的完
        memcpy(buf, m_buffer + m_readBegin, size);
        m_readBegin = advance(m_readBegin, size);
    }else{
        // 到结尾读不完
        size_t readBytes = m_capacity - m_readBegin;
        memcpy(buf, m_buffer + m_readBegin, readBytes);
        m_readBegin = advance(m_readBegin, readBytes);
        memcpy((char*)buf + readBytes, m_buffer + m_readBegin, size - readBytes);
        m_readBegin = advance(m_readBegin, size - readBytes);
    }
}



}