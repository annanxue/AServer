#include "logic_thread.h"
#include "app_base.h"
#include "log.h"
#include <chrono>
#include <thread>

using AServer::AppBase;

namespace AServer {

bool LogicThread::pre_stop_ = false;
bool LogicThread::pre_stop_tried_ = false;
bool LogicThread::reload_lua_ = false;

LogicThread::LogicThread()
    : frame_time_(100)
    , frame_count_(0)
    , running_(false) {
    set_thread_name("LogicThread");
}

LogicThread::~LogicThread() {
    running_ = false;
}

void LogicThread::run() {
    INFO("LogicThread started");
    running_ = true;

    while (running_ && AppBase::active_) {
        auto frame_start = std::chrono::steady_clock::now();

        lock();
        try {
            if (pre_stop_) {
                check_stop();
            } else {
                recv_msg();
                process();
                do_logic_state_lua();
                send_msg();
                run_timer();
                run_gc_step(frame_time_);
                log_frame();
            }
        } catch (const std::exception& e) {
            (void)e;
            ERROR("LogicThread exception: %s", e.what());
        } catch (...) {
            ERROR("LogicThread unknown exception");
        }
        unlock();

        frame_count_++;

        auto frame_end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start).count();
        auto sleep_time = frame_time_ - elapsed;

        if (sleep_time > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
    }

    INFO("LogicThread stopped, total frames: %d", frame_count_);
}

}
