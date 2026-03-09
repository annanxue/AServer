#ifndef __LOGIC_THREAD_H__
#define __LOGIC_THREAD_H__

#include <stdint.h>
#include "thread.h"
#include "lunar.h"
#include "netmng.h"

using namespace FF_Network; 

class LogicThread: public Thread
{
public:
    LogicThread():frame_start_time_(0), frame_wait_time_(0), frame_rate_(10){ thread_name_ = "LogicThread"; }
    virtual ~LogicThread() {}
public:
    virtual void lock() {}
    virtual void frame_begin();
    virtual void reload_lua(){};
    virtual void check_stop(){};
    virtual void recv_msg() {}
    virtual void process() {}
    virtual void do_logic_state_lua(){}
    virtual void run_timer() {}
    virtual void send_msg() {}
    virtual void run_gc_step( int32_t _frame_left_time ) {}
    virtual void log_frame() {}
    virtual void frame_end();
    virtual void unlock() {}
public:
    void run();
    void set_frame_rate( int _frame_rate ) { frame_rate_ = _frame_rate; }
    int32_t get_frame_rate() { return frame_rate_; }
    void do_log_frame( NetMng* _net_mng, lua_State* _L );
private:
    unsigned int frame_start_time_;
    int32_t frame_wait_time_;
protected:
    int32_t frame_rate_;

public:
    static volatile bool reload_lua_;
    static volatile bool pre_stop_;
    static volatile bool pre_stop_tried_;

};

#endif

