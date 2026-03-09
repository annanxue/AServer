#include <signal.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include "log.h"
#include "misc.h"
#include "app_base.h"
#include "logic_thread.h"
#include "app_log_module.h"
#include "app_net_module.h"
#include "app_timer_module.h"
#include "robot_client_module.h"
#include "robot_lua_module.h"
#include "timer.h"
#include "app_curl_module.h"
#include "resource.h"

int g_frame = 0;

/*******************************************************
 *                  Logic Thread
*******************************************************/

class RobotThread: public LogicThread
{
public:
    virtual void lock() { g_luasvr->lock(); }
    virtual void recv_msg();
    virtual void process();
    virtual void run_timer();
    virtual void send_msg();
    virtual void run_gc_step( int32_t _frame_left_time );
    virtual void check_stop();
    virtual void unlock() { g_luasvr->unlock(); }
};

void RobotThread::recv_msg()
{
    g_robotclient->receive_msg();
}

void RobotThread::process()
{
    g_http_mng->process( g_luasvr->L() );
}

void RobotThread::run_timer()
{
    g_timermng->run_timer_list();
}

void RobotThread::send_msg()
{
}

void RobotThread::run_gc_step( int32_t _frame_left_time )
{
    g_luasvr->check_gc( _frame_left_time );
}

void RobotThread::check_stop()
{
    if (pre_stop_ && !pre_stop_tried_)
    {
        pre_stop_tried_ = true;
        
        LOG(2)(" AppBase stop");
        AppBase::active_ = false;
    }
}

/*******************************************************
 *                  CoreApp
*******************************************************/

class RobotApp : public AppBase
{
public:
    bool main_loop();
};

bool RobotApp::main_loop()
{
#ifdef DEBUG
    uint32_t frame_time = msec();
    int32_t wait_time = 0;
    unsigned long noblock = 1;
    ioctl(STDIN_FILENO, FIONBIO, &noblock);
#define IO_BUFFSIZE ( 1024 )
    char buf[IO_BUFFSIZE];    
#endif
    while( active_ ) {
#ifdef DEBUG
        if( !is_daemon() ) {
            int32_t len = read( STDIN_FILENO, buf, IO_BUFFSIZE );
            if( len > 0 )
            {
                buf[len-1] = '\0';  
                // lua debugger
                g_luasvr->lock();
                g_luasvr->debugger_->handle_command( buf );
                g_luasvr->unlock();
            } 
        }

        frame_time += 1000;
        wait_time = frame_time - msec();
        if( wait_time > 0 ) { 
            ff_sleep( wait_time );
        }
#else
        ff_sleep( 3000 );
#endif
    }
    return true;
}

int32_t main( int32_t argc, char** argv )
{
    RobotApp robot_app;

    LogModule log_module;
    TimerModule timer_module;
    NetModule net_module;
    LuaModule lua_module;
    ResourceModule res_module;
    RobotClientModule robot_client_module;
    CurlModule curl_module;
    RobotThread robot_thread;

    if( robot_app.init( argc, argv ) ) {
        robot_app.register_class( &log_module );
        robot_app.register_class( &timer_module );
        robot_app.register_class( &net_module );
        robot_app.register_class( &lua_module );
        robot_app.register_class( &res_module );
        robot_app.register_class( &robot_client_module );
        robot_app.register_class( &curl_module );
        robot_app.register_thread( &robot_thread );
        robot_app.start();
    }
    
    return 0;
}

