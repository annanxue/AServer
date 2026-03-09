#include <sys/ioctl.h> 
#include <string.h>
#include "netmng.h"
#include "buffer.h"
#include "commonsocket.h"
#include "database.h"
#include "log.h"
#include "misc.h"
#include "singleton.h"
#include "logic_thread.h"
#include "timer.h"

#include "app_net_module.h"
#include "app_timer_module.h"
#include "app_log_module.h"
#include "login_lua_module.h"
#include "game_login_svr_module.h"
#include "player_login_svr_module.h"
#include "gm_client_module.h"
#include "app_curl_module.h"
#include "log_client_module.h"

int g_frame = 0;
/*******************************************************
 *                  Logic Thread
*******************************************************/

class LoginThread: public LogicThread
{
public:
    virtual void lock() { g_luasvr->lock(); }
    virtual void reload_lua();
    virtual void check_stop();
    virtual void recv_msg();
    virtual void process();
    virtual void run_timer();
    virtual void send_msg();
    virtual void run_gc_step( int32_t _frame_left_time );
    virtual void log_frame();
    virtual void unlock() { g_luasvr->unlock(); }
};

void LoginThread::reload_lua()
{
    if ( reload_lua_ )
    {
        g_luasvr->reload();                      
        reload_lua_ = false;
    }
}

void LoginThread::check_stop()
{
    if (pre_stop_ && !pre_stop_tried_)
    {
        pre_stop_tried_ = true;
        g_game_login_svr->set_pause( true );
        g_player_login_svr->set_pause( true );
        
        LOG(2)(" AppBase stop");
        AppBase::active_ = false;
    }
}

void LoginThread::recv_msg()
{
    g_game_login_svr->receive_msg();
    g_player_login_svr->receive_msg();
    g_gmclient->receive_msg();
    g_logclient->receive_msg();
}

void LoginThread::process()
{
    g_http_mng->process( g_luasvr->L() );
}

void LoginThread::run_timer()
{
    g_timermng->run_timer_list();
}

void LoginThread::send_msg()
{
}

void LoginThread::run_gc_step( int32_t _frame_left_time )
{
    g_luasvr->check_gc( _frame_left_time );
}

void LoginThread::log_frame()
{
    do_log_frame( g_player_login_svr, g_luasvr->L() );
}


/*******************************************************
 *                  CoreApp
*******************************************************/

class LoginApp:public AppBase{
public:
    bool main_loop(){
        uint32_t frame_time = msec();
        int32_t wait_time = 0;

#ifdef DEBUG            
#define IO_BUFFSIZE ( 1024 )
        unsigned long noblock = 1;
        ioctl(STDIN_FILENO, FIONBIO, &noblock);
        char buf[IO_BUFFSIZE];    
#endif

        while( active_ ){
            LOG_PROCESS_TIMER();

#ifdef DEBUG
            if( !is_daemon() ) {
                int len = read( STDIN_FILENO, buf, IO_BUFFSIZE );
                if( len > 0 )
                {
                    buf[len-1] = '\0';  //drop the last '\n'
                    g_luasvr->lock();
                    g_luasvr->debugger_->handle_command( buf );
                    g_luasvr->unlock();
                } 
            }
#endif

            frame_time += 1000;
            wait_time = frame_time - msec();
            if( wait_time > 0 ) 
                ff_sleep( wait_time );
            else
                TRACE(2)("[CPU] WAIT TIME %d", wait_time );
        }
        return true;
    }
};

/* 信号处理函数 */
static void sig_action( int _sig )
{
    LOG(2)( "[LOGINSERVER](signal) _sig: %d, SIGINT: %d, SIGKILL: %d, SIGUSR1: %d , g_frame: %d, int_time_: %d", _sig, SIGINT, SIGKILL, SIGUSR1, g_frame, AppBase::int_time_ );
    if( _sig == SIGINT )
    {
        if( g_frame - AppBase::int_time_ < 10 )
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
        TRACE(2)( "[LOGINSERVER](signal) haha! signal value is: %d", SIGUSR1 );
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

int main( int argc, char** argv )
{
    LoginApp login_app;
    LogModule log_module;
    TimerModule timer_module;
    NetModule net_module;
    LuaServerModule luasvr_module;
    GameLoginSvrModule game_login_svr_module;
    PlayerLoginSvrModule player_login_svr_module;
    GMClientModule gmclient_module;
    CurlModule curl_module;
    LoginThread login_thread;
    LogClientModule logclient_module;
    
    if (login_app.init( argc, argv ) ){
        login_app.register_class( &log_module );
        login_app.register_class( &timer_module );
        login_app.register_class( &net_module );
        login_app.register_class( &luasvr_module );
        login_app.register_class( &game_login_svr_module );
        login_app.register_class( &player_login_svr_module );
        login_app.register_class( &gmclient_module );
        login_app.register_class( &curl_module );
        login_app.register_class( &logclient_module );

        login_app.register_thread( &login_thread );

        login_app.register_signal(SIGUSR1, sig_action);
        login_app.register_signal(SIGINT, sig_action);

        login_app.start();
    }
    return 0;
}
