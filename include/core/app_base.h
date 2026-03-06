#ifndef ASERVER_APP_BASE_H
#define ASERVER_APP_BASE_H

#include "define.h"
#include "log.h"
#include "thread.h"
#include "singleton.h"
#include "lua_server.h"
#include <vector>
#include <string>
#include <map>
#include <functional>

namespace AServer {

class AppBase;

class AppClassInterface {
public:
    virtual ~AppClassInterface() = default;
    virtual bool app_class_init() { return true; }
    virtual bool app_class_destroy() { return true; }

protected:
    AppBase* app_base_ = nullptr;
    AppClassInterface* next_ = nullptr;
    AppClassInterface* prev_ = nullptr;

    friend class AppBase;
};

class LogicThread;

class AppBase {
public:
    AppBase();
    virtual ~AppBase();

    bool init(int argc, char* argv[]);
    bool register_class(AppClassInterface* cls);
    bool register_signal(int sig, void (*handler)(int));
    bool start();

    virtual bool main_loop() = 0;

    bool is_daemon() const { return is_daemon_; }
    void set_daemon(bool daemon) { is_daemon_ = daemon; }

    static volatile bool active_;
    static volatile int int_time_;

public:
    void set_config_file(const std::string& file) { config_file_ = file; }
    const std::string& get_config_file() const { return config_file_; }

protected:
    virtual void on_signal(int sig);

private:
    bool load_config();

    AppClassInterface class_header_;
    std::string config_file_;
    bool is_daemon_ = false;
    bool initialized_ = false;
    std::vector<AppClassInterface*> classes_;
};

}

#endif
