#ifndef ASERVER_LOG_H
#define ASERVER_LOG_H

#include "define.h"
#include <string>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <vector>
#include <cstdarg>

namespace AServer {

enum LogLevel {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_WARN = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_FATAL = 5
};

class Log {
public:
    static Log& instance();

    void init(const std::string& log_dir = "./log", const std::string& prefix = "AServer");
    void set_level(LogLevel level);
    void set_console(bool enable);

    void log(LogLevel level, const char* file, int line, const char* format, ...);
    void flush();

private:
    Log();
    ~Log();
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    std::string get_level_str(LogLevel level);
    std::string get_time_str();

    LogLevel min_level_;
    std::string log_dir_;
    std::string prefix_;
    bool console_enable_;
    std::mutex mutex_;
    std::ofstream file_stream_;
    bool initialized_;
};

#define LOG_TRACE(fmt, ...) AServer::Log::instance().log(AServer::LOG_LEVEL_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) AServer::Log::instance().log(AServer::LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) AServer::Log::instance().log(AServer::LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) AServer::Log::instance().log(AServer::LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) AServer::Log::instance().log(AServer::LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) AServer::Log::instance().log(AServer::LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define TRACE(...) LOG_TRACE(__VA_ARGS__)
#define DEBUG(...) LOG_DEBUG(__VA_ARGS__)
#define INFO(...) LOG_INFO(__VA_ARGS__)
#define WARN(...) LOG_WARN(__VA_ARGS__)
#define ERR(...) LOG_ERROR(__VA_ARGS__)
#ifndef _WIN32
#define ERROR(...) LOG_ERROR(__VA_ARGS__)
#endif

}

#endif
