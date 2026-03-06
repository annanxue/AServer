#ifndef ASERVER_LOG_MODULE_H
#define ASERVER_LOG_MODULE_H

#include "define.h"
#include "app_base.h"
#include "singleton.h"

namespace AServer {

class LogModule : public AppClassInterface {
public:
    bool app_class_init() override;
    bool app_class_destroy() override;

    void set_log_level(int level);
    void set_log_console(bool enable);

private:
    int log_level_ = LOG_LEVEL_INFO;
    bool console_enable_ = true;
};

}

#endif
