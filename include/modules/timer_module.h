#ifndef ASERVER_TIMER_MODULE_H
#define ASERVER_TIMER_MODULE_H

#include "define.h"
#include "app_base.h"
#include "singleton.h"
#include "thread.h"
#include <map>
#include <vector>
#include <functional>
#include <chrono>

namespace AServer {

using TimerCallback = std::function<void()>;

struct TimerItem {
    int timer_id;
    int interval;
    int64_t next_tick;
    TimerCallback callback;
    bool repeat;
    bool valid;
};

class TimerMng : public Singleton<TimerMng> {
public:
    TimerMng();
    ~TimerMng();

    int add_timer(int interval, TimerCallback callback, bool repeat = true);
    void remove_timer(int timer_id);
    void run_timer_list();

private:
    int next_timer_id_ = 1;
    std::map<int, TimerItem> timers_;
    Mutex mutex_;
};

class TimerModule : public AppClassInterface {
public:
    bool app_class_init() override;
    bool app_class_destroy() override;
};

#define g_timermng AServer::TimerMng::instance_ptr()

}

#endif
