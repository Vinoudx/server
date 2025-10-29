#include "timestamp.hpp"

#include <time.h>

namespace furina{

Timestamp::Timestamp(uint64_t now)
    :m_now(now){}

Timestamp Timestamp::now(){
    return Timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count());
}

Timestamp Timestamp::nowAbs(){
    return Timestamp(time(nullptr));
}

uint64_t Timestamp::getNowTime(){
    return m_now;
}

uint64_t Timestamp::nowTimeMs(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
}

}