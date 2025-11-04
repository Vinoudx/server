#include "logger.hpp"
#include "macro.hpp"
#include <string>

#include <chrono>

using namespace furina;


int main(){

    FileAppender::ptr_t fa = FileAppender::ptr_t(new FileAppender("../tests/log"));
    StdAppender::ptr_t sa = StdAppender::ptr_t(new StdAppender());

    Logger::ptr_t logger = Logger::ptr_t(new Logger());
    logger->addAppender(fa);
    logger->addAppender(sa);

    LogEvent::ptr_t event = LogEvent::ptr_t(new LogEvent("A", 0, 1, 1, 0));
    const std::string content = "AAAAA";

    event->getSS() << content;

    auto timebegin = std::chrono::high_resolution_clock().now();

    uint64_t count = 0;

    while(true){
        auto timenow = std::chrono::high_resolution_clock().now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(timenow - timebegin).count() >= 10000)break;
        ++count;
        logger->debug(event);
    }


    LOG_DEBUG << "end";
    return 0;
}