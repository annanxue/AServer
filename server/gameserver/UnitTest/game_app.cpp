#include <signal.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include "log.h"
#include "misc.h"
#include "timer.h"
#include "app_base.h"
#include "logic_thread.h"
#include "app_log_module.h"
#include "app_net_module.h"
#include "app_timer_module.h"
#include "game_lua_module.h"
#include "game_svr_module.h"
#include "db_client_module.h"
#include "login_client_module.h"
#include "bf_client_module.h"
#include "gm_client_module.h"
#include "log_client_module.h"
#include "resource.h"
#include "world_mng.h"
#include "player_mng.h"
#include "states.h"
#include "connect_thread.h"
#include "app_curl_module.h"
#include "gm_svr_module.h"

int g_frame = 0;


/*******************************************************
 *                  Logic Thread
*******************************************************/

class GameThread: public LogicThread
{
public:
    virtual void lock() { g_luasvr->lock(); }
    virtual void reload_lua();
    virtual void check_stop();
    virtual void recv_msg();
    virtual void process();
    virtual void do_logic_state_lua();
    virtual void send_msg();
    virtual void run_timer();
    virtual void run_gc_step( int32_t _frame_left_time );
    virtual void log_frame();
    virtual void unlock() { g_luasvr->unlock(); }
};

void GameThread::reload_lua()
{
    if (reload_lua_){
        g_luasvr->reload();                      
        reload_lua_ = false;
    }
}

void GameThread::check_stop()
{
    if (pre_stop_ && !pre_stop_tried_){
        pre_stop_tried_ = true;
        g_gamesvr->set_pause( true );
        g_gamesvr->kick_all_player();
        g_gamesvr->on_server_pre_stop();
        
        if (g_connect_thread->is_connected()){
            LOG(2)("wait db to recv all and close connection");
            g_dbclient->send_last_msg();
        }else{
            LOG(2)(" AppBase stop");
            AppBase::active_ = false;
        }

    }
}

void GameThread::recv_msg()
{
    TICK(A);
    g_gamesvr->receive_msg();
    g_dbclient->receive_msg();
    g_loginclient->receive_msg();
    g_gmclient->receive_msg();
    g_bfclient->receive_msg();
    g_logclient->receive_msg();
    g_gmsvr->receive_msg();
    TICK(B);
    mark_tick( TICK_TOTAL_RECV, A, B );
}

void GameThread::process()
{
    TICK(A);
    g_worldmng->process();
    TICK(B);
    mark_tick( TICK_PROCESS, A, B );

    g_http_mng->process( g_luasvr->L() );
}

void GameThread::do_logic_state_lua()
{
    TICK(A_LUA_MSG)
    lua_State *L = g_luasvr->L();
    int orig_top = lua_gettop( L );
    g_luasvr->get_lua_ref( GameLuaSvr::STATE_MSG );
    list_head tmp;
    INIT_LIST_HEAD(&tmp);

    int t_top = lua_gettop( L );
    while(!list_empty(&Ctrl::lua_msg_q)) {
        list_head *pos = Ctrl::lua_msg_q.next;
        LUA_MSG *node = list_entry(pos, LUA_MSG, link);
        if(node->objid) {
            if(node->frame <= g_frame) {
                lua_pushvalue(L, -1);
                list_del(&node->link);
                lua_pushnumber(L, node->objid);
                lua_pushnumber(L, node->state);
                lua_pushnumber(L, node->msg);
                lua_pushnumber(L, node->p0);
                lua_pushnumber(L, node->p1);
                lua_pushnumber(L, node->p2);
                lua_pushnumber(L, node->p3);
                lua_pushnumber(L, node->p4);

                //TICK(A1);
                if( llua_call(L, 8, 0, 0) )     {
                    llua_fail( L, __FILE__, __LINE__ );
                    ERR(2)("[TOLUA] state: %d, objid: %d, msg: %d, p0: %d, p1: %d, p2: %d, p3: %d, p4: %d",
                            node->state, node->objid, node->msg, node->p0, node->p1, node->p2, node->p3, node->p4 );
                    lua_settop( L, t_top );
                }
                //TICK(B1);
                //mark_tick( TICK_TYPE_STATE + ( node->state<<8 )+node->msg, A1, B1 );

                node->objid = 0;
                int index = node - Ctrl::lua_msg;
                if(index >= 0 && index < MAX_LUA_MSG) {
#ifdef FIX_LUA_MSG
                    list_add_tail(&node->link, &Ctrl::lua_msg_free_q);
#else
                    list_add_tail(&node->link, &Ctrl::lua_msg_q);
#endif
                } else {
                    delete node;
                }
            } else {
                list_del(&node->link);
                list_add(&node->link, &tmp);
            }
        } else {
            break;
        }
    }
    lua_settop(L, orig_top);
    if(!list_empty(&tmp)) {
        list_splice(&tmp, &Ctrl::lua_msg_q);
    }
    TICK(B_LUA_MSG)
    mark_tick( TICK_LUA_MSG, A_LUA_MSG, B_LUA_MSG );
}

void GameThread::send_msg()
{
    TICK(A);
    g_playermng->notify();
    TICK(B);
    mark_tick( TICK_TOTAL_NOTIFY, A, B );
}

void GameThread::run_timer()
{
    TICK(A);
    g_timermng->run_timer_list();
    TICK(B);
    mark_tick( TICK_TIMER, A, B );
}

void GameThread::run_gc_step( int32_t _frame_left_time )
{
    TICK(A);
    g_luasvr->check_gc( _frame_left_time );
    TICK(B);
    mark_tick( TICK_LUA_GC, A, B );
}

void GameThread::log_frame()
{
    do_log_frame( g_gamesvr, g_luasvr->L() );
}

/*******************************************************
 *                  CoreApp
*******************************************************/

class GameApp : public AppBase
{
public:
    bool main_loop();
};

bool GameApp::main_loop()
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

/* 信号处理函数 */
static void sig_action( int _sig )
{
    LOG(2)("[GAMESERVER](signal) _sig: %d, SIGINT: %d, SIGKILL: %d, SIGUSR1: %d , g_frame: %d, int_time_: %d", _sig, SIGINT, SIGKILL, SIGUSR1, g_frame, AppBase::int_time_ );
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
        TRACE(2)("[GAMESERVER](signal) haha! signal value is: %d", SIGUSR1);
        if ( LogicThread::pre_stop_ ){
            AppBase::active_ = false;
        } else {
            LogicThread::pre_stop_ = true;
        }
    }

    AppBase::int_time_ = g_frame;
    return;

}

int32_t main( int32_t argc, char** argv )
{

    GameApp game_app;
    LogModule log_module;
    NetModule net_module;
    LuaModule lua_module;
    GameSvrModule gsvr_module;
    DBClientModule dbclient_module;
    BFClientModule bfclient_module;
    LoginClientModule loginclient_module;
    GMClientModule gmclient_module;
    LogClientModule logclient_module;
    TimerModule timer_module;
    ResourceModule res_module;
    StatesModule states_module;
    CurlModule curl_module;
    GameThread game_thread;
    GmSvrModule gmsvr_module;

    if( game_app.init( argc, argv ) ) {
        game_app.register_class( &log_module );
        game_app.register_class( &timer_module );
        game_app.register_class( &net_module );
        game_app.register_class( &lua_module );
        game_app.register_class( &gsvr_module );
        game_app.register_class( &dbclient_module );
        game_app.register_class( &bfclient_module );
        game_app.register_class( &loginclient_module );
        game_app.register_class( &gmclient_module );
        game_app.register_class( &logclient_module );
        game_app.register_class( &res_module );
        game_app.register_class( &states_module );
        game_app.register_class( &curl_module );
        game_app.register_class( &gmsvr_module );
        game_app.register_thread( &game_thread );

        game_app.register_signal(SIGUSR1, sig_action);
        game_app.register_signal(SIGINT, sig_action);
        
        game_app.start();
    }
    
    return 0;
}

