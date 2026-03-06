#ifndef ASERVER_LOGIC_THREAD_H
#define ASERVER_LOGIC_THREAD_H

#include "define.h"
#include "thread.h"
#include "app_base.h"
#include "singleton.h"
#include <atomic>
#include <condition_variable>

namespace AServer {

class LogicThread : public Thread {
public:
    LogicThread();
    virtual ~LogicThread();

    virtual void lock() {}
    virtual void unlock() {}

    virtual void reload_lua() {}
    virtual void check_stop() {}
    virtual void recv_msg() {}
    virtual void process() {}
    virtual void do_logic_state_lua() {}
    virtual void send_msg() {}
    virtual void run_timer() {}
    virtual void run_gc_step(int32_t frame_left_time) {}
    virtual void log_frame() {}

    void set_frame_time(int32_t ms) { frame_time_ = ms; }
    int32_t get_frame_time() const { return frame_time_; }

    static bool pre_stop_;
    static bool pre_stop_tried_;
    static bool reload_lua_;

protected:
    virtual void run() override;

    int32_t frame_time_;
    int32_t frame_count_;
    std::atomic<bool> running_;
    std::condition_variable cv_;
    std::mutex mutex_;
};

}

#endif
