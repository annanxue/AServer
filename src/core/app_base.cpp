#include "app_base.h"
#include "logic_thread.h"
#include <iostream>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#endif

namespace AServer {

volatile bool AppBase::active_ = true;
volatile int AppBase::int_time_ = 0;

AppBase::AppBase() {
    class_header_.next_ = &class_header_;
    class_header_.prev_ = &class_header_;
    config_file_ = "config.lua";
}

AppBase::~AppBase() {
}

bool AppBase::init(int argc, char* argv[]) {
    if (initialized_) {
        return true;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--daemon") {
            is_daemon_ = true;
        } else if (arg == "-c" && i + 1 < argc) {
            config_file_ = argv[++i];
        }
    }

    if (!is_daemon_) {
        Log::instance().init("./log", "AServer");
        Log::instance().set_level(LOG_LEVEL_DEBUG);
        Log::instance().set_console(true);
    } else {
        Log::instance().init("./log", "AServer");
        Log::instance().set_level(LOG_LEVEL_INFO);
        Log::instance().set_console(false);
    }

    INFO("AServer initializing...");
    INFO("Config file: %s", config_file_.c_str());
    INFO("Daemon mode: %s", is_daemon_ ? "yes" : "no");

    initialized_ = true;
    return true;
}

bool AppBase::register_class(AppClassInterface* cls) {
    if (!cls) return false;

    cls->app_base_ = this;
    cls->next_ = &class_header_;
    cls->prev_ = class_header_.prev_;
    class_header_.prev_->next_ = cls;
    class_header_.prev_ = cls;

    classes_.push_back(cls);
    return true;
}

bool AppBase::register_signal(int sig, void (*handler)(int)) {
#ifdef _WIN32
    signal(sig, handler);
#else
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, nullptr);
#endif
    return true;
}

bool AppBase::start() {
    INFO("Starting AServer...");

    for (auto cls : classes_) {
        if (!cls->app_class_init()) {
            ERR("Failed to initialize class");
            return false;
        }
    }

    INFO("AServer started successfully");
    return main_loop();
}

void AppBase::on_signal(int sig) {
    INFO("Received signal: %d", sig);
    active_ = false;
}

}
