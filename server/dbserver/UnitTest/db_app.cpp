#include <sys/ioctl.h> 
#include <string.h>
#include "app_net_module.h"
#include "app_log_module.h"
#include "db_lua_module.h"
#include "netmng.h"
#include "buffer.h"
#include "commonsocket.h"
#include "database.h"
#include "log.h"
#include "misc.h"
#include "singleton.h"
#include "db_svr_module.h"
#include "gm_client_module.h"
#include "logic_thread.h"
#include "timer.h"
#include "app_timer_module.h"
#include "app_curl_module.h"

int g_frame = 0;
/*******************************************************
 *                  Logic Thread
*******************************************************/

class DbThread: public LogicThread
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
    virtual void log_frame();
    virtual void unlock() { g_luasvr->unlock(); }
};

void DbThread::reload_lua()
{
    if (reload_lua_){
        g_luasvr->reload();                      
        reload_lua_ = false;
    }
}

void DbThread::check_stop()
{
    if (pre_stop_ && !pre_stop_tried_){
        pre_stop_tried_ = true;

        //db没有保存数据的逻辑
        //正常维护时的保存数据必须由gameserver发起
        //gameserver退出后，才退db

        AppBase::active_ = false;
    }
}


void DbThread::recv_msg()
{
    g_dbsvr->receive_msg();
    g_gmclient->receive_msg();
}

void DbThread::process()
{
}

void DbThread::send_msg()
{
}

void DbThread::run_timer()
{
    TICK(A);
    g_timermng->run_timer_list();
    TICK(B);
    mark_tick( TICK_TIMER, A, B );
}

void DbThread::run_gc_step( int32_t _frame_left_time )
{
    g_luasvr->check_gc( _frame_left_time );
}

void DbThread::log_frame()
{
    do_log_frame( g_dbsvr, g_luasvr->L() );
}


/* 信号处理函数 */
static void sig_action( int _sig )
{
    LOG(2)("(signal) _sig: %d, SIGINT: %d, SIGKILL: %d, SIGUSR1: %d , g_frame: %d, int_time_: %d", _sig, SIGINT, SIGKILL, SIGUSR1, g_frame, AppBase::int_time_ );
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

    if ( _sig == SIGUSR1 ) {
        TRACE(2)("(signal) haha! signal value is: %d", SIGUSR1);
        if ( LogicThread::pre_stop_ ){
            AppBase::active_ = false;
        } else {
            LogicThread::pre_stop_ = true;
        }
    }

    AppBase::int_time_ = g_frame;
    return;

}


/*******************************************************
 *                  CoreApp
*******************************************************/

class DBApp:public AppBase{
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


int main( int argc, char** argv )
{
    DBApp db_app;
    LogModule log_module;
    TimerModule timer_module;
    NetModule net_module;
    LuaServerModule luasvr_module;
    DbSvrModule gsvr_module;
    GMClientModule gmclient_module;
    CurlModule curl_module;
    DbThread db_thread;
    
    if (db_app.init( argc, argv ) ){
        db_app.register_class( &log_module );
        db_app.register_class( &timer_module );
        db_app.register_class( &net_module );
        db_app.register_class( &luasvr_module );
        db_app.register_class( &gsvr_module );
        db_app.register_class( &gmclient_module );
        db_app.register_class( &curl_module );

        db_app.register_thread( &db_thread );

        db_app.register_signal(SIGUSR1, sig_action);
        db_app.register_signal(SIGINT, sig_action);

        db_app.start();
    }
    return 0;
}
