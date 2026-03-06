#include "timer_module.h"
#include "log.h"

namespace AServer {

TimerMng::TimerMng() {
}

TimerMng::~TimerMng() {
}

int TimerMng::add_timer(int interval, TimerCallback callback, bool repeat) {
    std::lock_guard<Mutex> lock(mutex_);

    TimerItem item;
    item.timer_id = next_timer_id_++;
    item.interval = interval;
    item.next_tick = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count() + interval;
    item.callback = callback;
    item.repeat = repeat;
    item.valid = true;

    timers_[item.timer_id] = item;
    INFO("Added timer %d, interval: %dms", item.timer_id, interval);
    return item.timer_id;
}

void TimerMng::remove_timer(int timer_id) {
    std::lock_guard<Mutex> lock(mutex_);

    auto it = timers_.find(timer_id);
    if (it != timers_.end()) {
        it->second.valid = false;
        timers_.erase(it);
        INFO("Removed timer %d", timer_id);
    }
}

void TimerMng::run_timer_list() {
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    std::lock_guard<Mutex> lock(mutex_);

    std::vector<int> to_remove;
    for (auto& pair : timers_) {
        TimerItem& item = pair.second;
        if (!item.valid) continue;

        if (now >= item.next_tick) {
            if (item.callback) {
                try {
                    item.callback();
                } catch (const std::exception& e) {
                    (void)e;
                    ERR("Timer callback exception: %s", e.what());
                }
            }

            if (item.repeat) {
                item.next_tick = now + item.interval;
            } else {
                to_remove.push_back(item.timer_id);
            }
        }
    }

    for (int timer_id : to_remove) {
        timers_.erase(timer_id);
    }
}

bool TimerModule::app_class_init() {
    INFO("TimerModule initializing...");
    return true;
}

bool TimerModule::app_class_destroy() {
    INFO("TimerModule destroying...");
    return true;
}

}
