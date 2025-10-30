#ifndef __BUFFER__
#define __BUFFER__

#include <memory>

namespace furina{

/*
---------------------------------------
26AAAAAAAAAAAAAAAAAAAAAAAAA
^                          ^
|                          |
readBegin               writeBegin
*/

class Buffer{
public:
    using ptr = std::shared_ptr<Buffer>;

    Buffer(size_t defaultCapacity = 4096);
    ~Buffer();

    // uint64 string
    void writeString(const std::string& str);
    std::string readString();

    void writeUint64(uint64_t value);
    uint64_t readUint64();

    size_t readableBytes();
    size_t writeableBytes();

    bool hasReadableBytes(){return m_readBegin != m_writeBegin;}
    bool hasWriteableBytes(){return advance(m_writeBegin, 1) != m_readBegin;}
 
    void readNBytes(char* buffer, size_t n);
    void writeNBytes(const char* buffer, size_t n);

private:

    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);

    size_t advance(size_t ptr, size_t n);

    // 只能往大了resize
    void resize(size_t n);



    char* m_buffer;
    size_t m_capacity;
    size_t m_readBegin;
    size_t m_writeBegin;
};

}


#endif