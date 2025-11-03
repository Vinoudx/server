#include "buffer.hpp"

#include <string.h>
#include <string>

#include "logger.hpp"

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
    temp.resize(len);
    read(temp.data(), len, m_readBegin);
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
        read((char*)&temp, 1, m_readBegin);
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

uint64_t Buffer::peekUint64(){
    m_peekBegin = m_readBegin;
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7){
        uint8_t temp = 0;
        read((char*)&temp, 1, m_peekBegin);
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

void Buffer::readNBytes(char* buffer, size_t n){
    read(buffer, n, m_readBegin);
}

void Buffer::writeNBytes(const char* buffer, size_t n){
    write(buffer, n);
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

std::string Buffer::peekString(){
    m_peekBegin = m_readBegin;
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7){
        uint8_t temp = 0;
        read((char*)&temp, 1, m_peekBegin);
        if(temp < 0x80){
            result |= ((uint64_t)temp) << i;
            break;
        }else{
            result |= ((uint64_t)temp & 0x7f) << i;
        }
    }
    std::string temp;
    temp.resize(result);
    read(temp.data(), result, m_peekBegin);
    return temp;
}

void Buffer::read(void* buf, size_t size, size_t& read_begin){
    if(readableBytes() < size)[[unlikely]]{
        LOG_ERROR << "has " << readableBytes() << " Bytes, but need " << size << " Bytes";
        throw std::logic_error("no enough data");
    }
    if(m_capacity - read_begin >= size){
        // 到结尾读的完
        memcpy(buf, m_buffer + read_begin, size);
        read_begin = advance(read_begin, size);
    }else{
        // 到结尾读不完
        size_t readBytes = m_capacity - read_begin;
        memcpy(buf, m_buffer + read_begin, readBytes);
        read_begin = advance(read_begin, readBytes);
        memcpy((char*)buf + readBytes, m_buffer + read_begin, size - readBytes);
        read_begin = advance(read_begin, size - readBytes);
    }
}



}