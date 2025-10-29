#pragma once

#include <chrono>

namespace furina{

using Clock = std::chrono::steady_clock;

// 毫秒
class Timestamp{
public:

    Timestamp(uint64_t now);

    static Timestamp now();
    static Timestamp nowAbs();
    static uint64_t nowTimeMs();
    uint64_t getNowTime();
private:
    uint64_t m_now;
};

}