#include "logger.hpp"
#include "macro.hpp"

int main(){
    LOG_DEBUG << "debug";
    ASSERT2(0, "1243");
}