#include "lar.h"
#include "msg.h"
#include "player_login_svr_module.h"
#include "login_lua_module.h"
#include "timer.h"

const char PlayerLoginSvr::className[] = "PlayerPlayerLoginSvr";
const bool PlayerLoginSvr::gc_delete_ = false;

Lunar<PlayerLoginSvr>::RegType PlayerLoginSvr::methods[] =
{   
    method( PlayerLoginSvr, c_get_android_version ),
    method( PlayerLoginSvr, c_get_ios_version ),
    method( PlayerLoginSvr, c_get_gamesvr_config_url ),
    method( PlayerLoginSvr, c_reset_client_version ),
    method( PlayerLoginSvr, c_get_player_ip ),
    {NULL, NULL}
};


PlayerLoginSvr::PlayerLoginSvr()
    :android_version_(0), ios_version_(0)
{
    init_func_map();
}

PlayerLoginSvr::~PlayerLoginSvr()
{
}

void PlayerLoginSvr::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &PlayerLoginSvr::on_client_connect; 
    func_map_[PT_DISCONNECT] = &PlayerLoginSvr::on_client_disconnect; 
}

void PlayerLoginSvr::on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_CONNECT, _dpid );
}

void PlayerLoginSvr::on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    del_sock( _dpid );
}

void PlayerLoginSvr::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (PlayerLoginSvr::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
    TICK(A);
    if ( _packet_type >= 0 && _packet_type < PT_PACKET_MAX ) 
    {    
        pfn = func_map_[_packet_type];
        if( pfn ) 
        {
            Ar ar( _msg, _size );   
            ( this->*(pfn) )( ar, _dpid, _msg, _size );
        }
        else
        {
            // call lua
            lua_msg_handle( g_luasvr->L(), _msg, _size, _packet_type, _dpid );
        }
    }
    else
    {
        ERR(2)( "[PlayerLoginSvr](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void PlayerLoginSvr::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( LoginLuaSvr::PLAYER_LOGIN_MESSAGE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

int PlayerLoginSvr::c_get_android_version( lua_State* _L )
{
    lua_pushinteger( _L, android_version_ );
    return 1;
}

int PlayerLoginSvr::c_get_ios_version( lua_State* _L )
{
    lua_pushinteger( _L, ios_version_ );
    return 1;
}

int PlayerLoginSvr::c_get_gamesvr_config_url( lua_State* _L )
{
    lua_pushstring( _L, gamesvr_config_url_ );
    return 1;
}

int PlayerLoginSvr::c_reset_client_version( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    int type = lua_tonumber( _L, 1 );
    int version = lua_tonumber( _L, 2 );
    bool ret = true;
    if( type == 1) // android
    {
        set_android_version( version );
        LOG(2)( "[PlayerLoginSvrModule](c_reset_client_version) set android version:%d, success!", version );
    }
    else if( type == 2) // ios
    {
        set_ios_version( version );
        LOG(2)( "[PlayerLoginSvrModule](c_reset_client_version) set ios version:%d, success!", version );

    }
    else
    {
        ERR(2)( "[PlayerLoginSvrModule](c_reset_client_version) unknown type:%d err!", type );
        ret = false;
    }

    lua_pushboolean(_L, ret);
    return 1;
}

int PlayerLoginSvr::c_get_player_ip( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    DPID dpid = (DPID)lua_tonumber( _L, 1 );
    char ip[MAX_IP] = { 0 };
    CommonSocket* sock = get_sock( dpid );
    if ( sock )
    {   
        const EndPoint* ep = sock->get_endpoint();
        ep->GetStringIP( ip );
    }   
    lua_pushstring( _L, ip );
    return 1;
}

bool PlayerLoginSvrModule::app_class_init()
{
    int32_t max_con = 0;
    int32_t port = 0;
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t proc_thread_num = 4;
    int32_t timeout_time = 0;
    int32_t android_version = 0;
    int32_t ios_version = 0;

    APP_GET_NUMBER( "AndroidVersion", android_version ); 
    APP_GET_NUMBER( "IosVersion", ios_version ); 
    APP_GET_STRING( "GameSvrConfigUrl", g_player_login_svr->gamesvr_config_url_ ); 

    APP_GET_TABLE( "PlayerLoginSvr", 2 );
    APP_GET_NUMBER( "MaxCon", max_con );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "ProcThreadNum", proc_thread_num );
    APP_GET_NUMBER( "TimeOutTime", timeout_time );
    APP_END_TABLE();


    if( !max_con || !port ) 
    {
        ERR(2)( "[PlayerLoginSvrModule](app_class_init)read cfg err!" );
        return false;
    }

    g_player_login_svr->set_android_version( android_version );
    g_player_login_svr->set_ios_version( ios_version );

    g_player_login_svr->set_timeout( timeout_time * g_timermng->get_frame_rate() );

    if( g_player_login_svr->start_server( max_con, port, crypt, NULL, poll_wait_time, proc_thread_num ) < 0 )
    {
        ERR(2)( "[PlayerLoginSvrModule](app_class_init)start_server failed. port: %d, max_con: %d", port, max_con );
        return false;
    }

    LOG(2)( "===========PlayerLoginSvr Module Start===========");

    return true;
}

bool PlayerLoginSvrModule::app_class_destroy()
{
    // g_player_login_svr release by auto_ptr

    LOG(2)( "===========PlayerLoginSvr Module Destroy===========");

    return true;
}



