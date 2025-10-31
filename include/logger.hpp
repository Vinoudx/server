#ifndef __LOGGER__
#define __LOGGER__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>

#include "AsyncLogger.hpp"
#include "utils.hpp"

#define GET_LOGGER

#define LOG(level) \
            std::make_unique<furina::LoggerWapper>(furina::LogLevel::level, furina::Logger::ptr_t(new furina::Logger), furina::LogEvent::ptr_t(new furina::LogEvent(__FILE__, __LINE__, furina::getThreadId(), furina::getFiberId(), ::time(NULL))))->getSS()

#define LOG_DEBUG LOG(DEBUG)
#define LOG_INFO LOG(INFO)
#define LOG_WARNING LOG(WARNING)
#define LOG_ERROR LOG(ERROR)
#define LOG_FATAL LOG(FATAL)

#define LOG_FLUSH furina::AsyncLogger::getInstance()->flush();


namespace furina{

enum class LogLevel{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

inline const char* levelToString(LogLevel level){
    switch (level){
        #define X(name) case LogLevel::name: return #name
        X(DEBUG);
        X(INFO);
        X(WARNING);
        X(ERROR);
        X(FATAL);
        #undef X
    }
    return "unkown";
}

// 日志事件
class LogEvent{
public:
    using ptr_t = std::shared_ptr<LogEvent>;
    LogEvent(const char* file, uint32_t line, uint32_t threadid, uint32_t fiberid, time_t time);
    std::string getFile();
    uint32_t getLine(){return line_;}
    uint32_t getThreadId(){return threadid_;}
    uint32_t getFiberId(){return fiberid_;}
    std::string getTime();
    std::string getContent(){return ss_.str();}

    std::stringstream& getSS(){return ss_;}

private:
    const char* file_ = nullptr;
    uint32_t line_ = 0;
    uint32_t threadid_ = 0;
    uint32_t fiberid_ = 0;
    time_t time_ = 0;
    std::stringstream ss_;
};

// 日志格式化器
class LogFormatter{
public:
    using ptr_t = std::shared_ptr<LogFormatter>;
    LogFormatter() = default;
    std::string format(LogLevel level, const LogEvent::ptr_t& event);
};

// 日志输出器基类
class LogAppenderBase{
public:
    using ptr_t = std::shared_ptr<LogAppenderBase>;
    LogAppenderBase();
    virtual ~LogAppenderBase() = default;

    virtual void log(LogLevel level, const LogEvent::ptr_t& event){};
    
protected:
    LogFormatter::ptr_t formatter_;
};

class StdAppender: public LogAppenderBase{
public:
    using ptr_t = std::shared_ptr<StdAppender>;
    StdAppender();
    void log(LogLevel level, const LogEvent::ptr_t& event) override;
private:
};

class FileAppender: public LogAppenderBase{
public:
    using ptr_t = std::shared_ptr<FileAppender>;
    FileAppender(const std::string& filename);
    void log(LogLevel level, const LogEvent::ptr_t& event) override;
    bool reopen();
private:
    std::string filename_;
    int fd_;
};

// 主日志类
class Logger{
public:
    using ptr_t = std::shared_ptr<Logger>;
    Logger(LogLevel level = LogLevel::DEBUG, const std::string& name = "root");
    void log(LogLevel level, const LogEvent::ptr_t& event);

    void debug(const LogEvent::ptr_t& event);
    void info(const LogEvent::ptr_t& event);
    void warning(const LogEvent::ptr_t& event);
    void error(const LogEvent::ptr_t& event);
    void fatal(const LogEvent::ptr_t& event);

    Logger& addAppender(LogAppenderBase::ptr_t appender);
    Logger& removeAppender(LogAppenderBase::ptr_t appender);
    LogLevel getLevel() const{return level_;}
    void setLevel(LogLevel level){level_ = level;}

private:
    std::string name_;
    LogLevel level_;
    std::list<LogAppenderBase::ptr_t> appenders_;
};

class LoggerWapper{
public:
    using ptr_t = std::shared_ptr<LoggerWapper>;
    LoggerWapper(LogLevel level, const Logger::ptr_t& logger, const LogEvent::ptr_t& event);
    ~LoggerWapper();
    std::stringstream& getSS(){return event_->getSS();}
private:
    Logger::ptr_t logger_;
    LogEvent::ptr_t event_;
    LogLevel level_;
};


};

#endif