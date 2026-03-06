#include "log.h"
#include <chrono>
#include <iomanip>

namespace AServer {

Log& Log::instance() {
    static Log inst;
    return inst;
}

Log::Log()
    : min_level_(LOG_LEVEL_TRACE)
    , console_enable_(true)
    , initialized_(false) {
}

Log::~Log() {
    if (file_stream_.is_open()) {
        file_stream_.flush();
        file_stream_.close();
    }
}

void Log::init(const std::string& log_dir, const std::string& prefix) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_dir_ = log_dir;
    prefix_ = prefix;
    
#ifdef _WIN32
    system(("mkdir " + log_dir_).c_str());
#else
    system(("mkdir -p " + log_dir_).c_str());
#endif
    
    std::string filename = log_dir_ + "/" + prefix_ + ".log";
    file_stream_.open(filename, std::ios::app);
    initialized_ = true;
}

void Log::set_level(LogLevel level) {
    min_level_ = level;
}

void Log::set_console(bool enable) {
    console_enable_ = enable;
}

std::string Log::get_level_str(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_TRACE: return "TRACE";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO ";
        case LOG_LEVEL_WARN: return "WARN ";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Log::get_time_str() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void Log::log(LogLevel level, const char* file, int line, const char* format, ...) {
    if (level < min_level_) return;

    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    std::stringstream ss;
    ss << "[" << get_time_str() << "] ";
    ss << "[" << get_level_str(level) << "] ";
    ss << "[" << file << ":" << line << "] ";
    ss << buffer;

    std::string msg = ss.str();

    std::lock_guard<std::mutex> lock(mutex_);
    
    if (console_enable_) {
        std::cout << msg << std::endl;
    }
    
    if (file_stream_.is_open()) {
        file_stream_ << msg << std::endl;
        file_stream_.flush();
    }
}

void Log::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.flush();
    }
}

}
