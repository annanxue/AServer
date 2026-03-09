#include "login_client_module.h"
#include "game_lua_module.h"
#include "game_svr_module.h"
#include "connect_thread.h"
#include "lmisc.h"
#include "msg.h"
#include "lar.h"

const char LoginClient::className[] = "LoginClient";
const bool LoginClient::gc_delete_ = false;

Lunar<LoginClient>::RegType LoginClient::methods[] =
{   
	method( LoginClient, c_reset_connect_info ),
    {NULL, NULL}
};

LoginClient::LoginClient()
{
    init_func_map();
}

LoginClient::~LoginClient()
{
}

void LoginClient::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &LoginClient::on_connect; 
    func_map_[PT_DISCONNECT] = &LoginClient::on_disconnect; 
}

void LoginClient::on_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, LG_TYPE_CONNECT, _dpid );
}

void LoginClient::on_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, LG_TYPE_DISCONNECT, _dpid );

    del_sock( _dpid );
    g_login_connect_thread->on_conn_disconnect();
}

void LoginClient::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (LoginClient::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
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
            lua_msg_handle( g_luasvr->L(), _msg, _size, _packet_type, _dpid );
        }
    }
    else
    {
        ERR(2)( "[LoginClient](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void LoginClient::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( GameLuaSvr::LOGINCLIENT_MESSAGE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

int LoginClient::c_reset_connect_info( lua_State* _L )
{
	lcheck_argc( _L, 2 );

    const char* ip = lua_tostring( _L, 1 );
    int port = lua_tointeger( _L, 2 );

    if( ip == NULL )
    {
        ERR(2)( "[LoginClient](c_reset_connect_info)ip is not valid" );
        lua_pushnumber( _L, 1);
        return 1;
    }
    char buff[32];
    if( inet_pton( AF_INET, ip, buff ) <= 0 )
    {
        ERR(2)( "[LoginClient](c_reset_connect_info)ip: %s is not valid", ip );
        lua_pushnumber( _L, 1);
        return 1;
    }
    if( port <= 0 )
    {
        ERR(2)( "[LoginClient](c_reset_connect_info)port: %d is not valid", port );
        lua_pushnumber( _L, 2);
        return 1;
    }

    g_login_connect_thread->reset_connect_info( ip, port );
    lua_pushnumber( _L, 0);

    return 1;
}

bool LoginClientModule::app_class_init()
{
    char ip[APP_CFG_NAME_MAX] = {0};
    int32_t port = 0;
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t sleep_time = 100;

    APP_GET_TABLE( "LoginClient", 2 );
    APP_GET_STRING( "IP", ip );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "SleepTime", sleep_time );
    APP_END_TABLE();

    if( strlen(ip) <= 0 || !port ) 
    {
        ERR(2)( "[LoginClientModule](app_class_init)read cfg err!" );
        return false;
    }

    g_login_connect_thread->init( g_loginclient, ip, port, crypt, poll_wait_time, sleep_time );

    if( g_login_connect_thread->start() < 0 )
    {
        ERR(2)( "start connected thread failed" );  
        return false;
    }

    LOG(2)( "===========LoginClient Module Start===========");

    return true;
}

bool LoginClientModule::app_class_destroy()
{
    LOG(2)( "===========LoginClient Module Destroy===========");

    return true;
}



