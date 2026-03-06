#include "log_module.h"
#include "log.h"

namespace AServer {

bool LogModule::app_class_init() {
    INFO("LogModule initializing...");

    Log::instance().init("./log", "AServer");
    Log::instance().set_level((LogLevel)log_level_);
    Log::instance().set_console(console_enable_);

    INFO("LogModule initialized");
    return true;
}

bool LogModule::app_class_destroy() {
    INFO("LogModule destroying...");
    Log::instance().flush();
    return true;
}

void LogModule::set_log_level(int level) {
    log_level_ = level;
    Log::instance().set_level((LogLevel)level);
}

void LogModule::set_log_console(bool enable) {
    console_enable_ = enable;
    Log::instance().set_console(enable);
}

}
