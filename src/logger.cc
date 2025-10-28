#include "logger.hpp"

#include <cstring>

namespace furina{

    std::string LogEvent::getFile(){
        const char* ptr = strrchr(file_, '/');
        return std::string(ptr + 1, strlen(file_) - (ptr - file_));
    }

    std::string LogEvent::getTime(){
        struct tm *tm_info = localtime(&time_);
        char temp[64] = {0};
        snprintf(temp, 63, "%04d-%02d-%02d %02d:%02d:%02d",
           tm_info->tm_year + 1900,  
           tm_info->tm_mon + 1,      
           tm_info->tm_mday,         
           tm_info->tm_hour,         
           tm_info->tm_min,          
           tm_info->tm_sec);       
        return std::string(temp, strlen(temp));
    }

    Logger::Logger(LogLevel level, const std::string& name):name_(name),level_(level){
        appenders_.push_back(furina::LogAppenderBase::ptr_t(new furina::StdAppender()));
    }

    LogEvent::LogEvent(const char* file, uint32_t line, uint32_t threadid, uint32_t fiberid, time_t time):
        file_(file),
        line_(line),
        threadid_(threadid),
        fiberid_(fiberid),
        time_(time){}

    void Logger::log(LogLevel level, const LogEvent::ptr_t& event){
        for(auto& appender: appenders_){
            if(level >= level_){
                appender->log(level, event);
            }
        }
    }

    void Logger::debug(const LogEvent::ptr_t& event){
        log(LogLevel::DEBUG, event);
    }
    void Logger::info(const LogEvent::ptr_t& event){
        log(LogLevel::INFO, event);
    }
    void Logger::warning(const LogEvent::ptr_t& event){
        log(LogLevel::WARNING, event);
    }
    void Logger::error(const LogEvent::ptr_t& event){
        log(LogLevel::ERROR, event);
    }
    void Logger::fatal(const LogEvent::ptr_t& event){
        log(LogLevel::FATAL, event);
    }

    Logger& Logger::addAppender(LogAppenderBase::ptr_t appender){
        appenders_.push_back(appender);
        return *this;
    }
    Logger& Logger::removeAppender(LogAppenderBase::ptr_t appender){
        for(auto it = appenders_.begin(); it != appenders_.end(); ++it){
            if(*it == appender){
                appenders_.erase(it);
                return *this;
            }
        }
        return *this;
    }


    LogAppenderBase::LogAppenderBase():formatter_(std::make_shared<LogFormatter>()){}

    StdAppender::StdAppender():LogAppenderBase(){}

    void StdAppender::log(LogLevel level, const LogEvent::ptr_t& event){
        AsyncLogger::getInstance()->log(STDOUT_FILENO, formatter_->format(level, event));
    }

    FileAppender::FileAppender(const std::string& filename):LogAppenderBase(),filename_(filename){
        fd_ = open(filename_.c_str(), O_APPEND);
        if(fd_ < 0){
            perror("open");
            fd_ = STDOUT_FILENO;
        }
    }

    void FileAppender::log(LogLevel level, const LogEvent::ptr_t& event){
        if(reopen()){
            AsyncLogger::getInstance()->log(fd_, formatter_->format(level, event));
        }
    }

    bool FileAppender::reopen(){
        if(fcntl(fd_, F_GETFD) != -1 || errno != EBADF){
            fd_ = open(filename_.c_str(), O_APPEND);
            if(fd_ < 0){
                perror("open");
                fd_ = STDOUT_FILENO;
            }
        }
        return fcntl(fd_, F_GETFD) != -1 || errno != EBADF;
    }

    std::string LogFormatter::format(LogLevel level, const LogEvent::ptr_t& event){
        char temp[1024] = {0};
        snprintf(temp, 1023, "%s\t[%s]\t[%u]\t[%u]\t%s:%u: %s\n",
            event->getTime().c_str(),            
            levelToString(level),
            event->getThreadId(),
            event->getFiberId(),
            event->getFile().c_str(),            
            event->getLine(),
            event->getContent().c_str()
        );
        return std::string(temp, strlen(temp));
    }

    LoggerWapper::LoggerWapper(LogLevel level, const Logger::ptr_t& logger, const LogEvent::ptr_t& event):
        level_(level),    
        logger_(logger),
        event_(event){}
    
    LoggerWapper::~LoggerWapper(){
        switch (level_)
        {
        case LogLevel::DEBUG:
            logger_->debug(event_);
            break;
        case LogLevel::INFO:
            logger_->info(event_);
            break;
        case LogLevel::WARNING:
            logger_->warning(event_);
            break;
        case LogLevel::ERROR:
            logger_->error(event_);
            break;
        case LogLevel::FATAL:
            logger_->fatal(event_);
            break;
        default:
            break;
        }
    }

}