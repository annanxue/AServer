#include <sys/ioctl.h> 
#include <string.h>
#include "app_net_module.h"
#include "app_log_module.h"
#include "app_timer_module.h"
#include "log_lua_module.h"
#include "netmng.h"
#include "buffer.h"
#include "commonsocket.h"
#include "log.h"
#include "misc.h"
#include "singleton.h"
#include "log_svr_module.h"
#include "gm_client_module.h"
#include "timer.h"
#include "app_curl_module.h"
#include "logic_thread.h"

int g_frame = 0;
/*******************************************************
 *                  Logic Thread
*******************************************************/

class LogThread: public LogicThread
{
public:
    virtual void lock() { g_luasvr->lock(); }
    virtual void reload_lua();
    virtual void check_stop();
    virtual void recv_msg();
    virtual void process();
    virtual void send_msg();
    virtual void run_timer();
    virtual void run_gc_step( int32_t _frame_left_time );
    virtual void unlock() { g_luasvr->unlock(); }
};

void LogThread::reload_lua()
{
    if (reload_lua_)
    {
        g_luasvr->reload();                      
        reload_lua_ = false;
    }
}

void LogThread::check_stop()
{
    if (pre_stop_ && !pre_stop_tried_)
    {
        pre_stop_tried_ = true;
        g_logsvr->set_pause( true );
        
        LOG(2)(" AppBase stop");
        AppBase::active_ = false;
    }
}

void LogThread::recv_msg()
{
    g_logsvr->receive_msg();
    g_gmclient->receive_msg();
}

void LogThread::process()
{
}

void LogThread::send_msg()
{
}

void LogThread::run_timer()
{
    g_timermng->run_timer_list();
}

void LogThread::run_gc_step( int32_t _frame_left_time )
{
    g_luasvr->check_gc( _frame_left_time );
}

/* 信号处理函数 */
static void sig_action( int _sig )
{
    LOG(2)( "[LOGSERVER](signal) _sig: %d, SIGINT: %d, SIGKILL: %d, SIGUSR1: %d , g_frame: %d, int_time_: %d", _sig, SIGINT, SIGKILL, SIGUSR1, g_frame, AppBase::int_time_ );
    if ( _sig == SIGINT )
    {
        if ( g_frame - AppBase::int_time_ < 10 )
        {
            LogicThread::pre_stop_ = true;
            LogicThread::reload_lua_ = false;
            signal( _sig, SIG_IGN );
        }
        else
        {
            LogicThread::reload_lua_ = true;
        }
    }

    if ( _sig == SIGUSR1 )
    {
        TRACE(2)( "[LOGSERVER](signal) haha! signal value is: %d", SIGUSR1 );
        if ( LogicThread::pre_stop_ )
        {
            AppBase::active_ = false;
        }
        else
        {
            LogicThread::pre_stop_ = true;
        }
    }

    AppBase::int_time_ = g_frame;
    return;
}

/*******************************************************
 *                  CoreApp
*******************************************************/
class LogApp : public AppBase
{
public:
    bool main_loop();
};

bool LogApp::main_loop()
{
#ifdef DEBUG
    uint32_t frame_time = msec();
    int32_t wait_time = 0;
    unsigned long noblock = 1;
    ioctl( STDIN_FILENO, FIONBIO, &noblock );
#define IO_BUFFSIZE ( 1024 )
    char buf[IO_BUFFSIZE];    
#endif

    while ( active_ )
    {
#ifdef DEBUG
        if ( !is_daemon() )
        {
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
        if ( wait_time > 0 )
        { 
            ff_sleep( wait_time );
        }
#else
        ff_sleep( 3000 );
#endif
    }
    return true;
}

int main( int argc, char** argv )
{
    LogApp log_app;

    LogModule log_module;
    NetModule net_module;
    LuaModule lua_module;
    LogSvrModule gsvr_module;
    GMClientModule gmclient_module;
    TimerModule timer_module;

    CurlModule curl_module;

    LogThread log_thread;
    
    if (log_app.init( argc, argv ) )
    {
        log_app.register_class( &log_module );
        log_app.register_class( &net_module );
        log_app.register_class( &timer_module );
        log_app.register_class( &lua_module );
        log_app.register_class( &gsvr_module );
        log_app.register_class( &gmclient_module );

        log_app.register_class( &curl_module );

        log_app.register_thread( &log_thread );

        log_app.register_signal( SIGUSR1, sig_action );
        log_app.register_signal( SIGINT, sig_action );

        log_app.start();
    }
    return 0;
}

