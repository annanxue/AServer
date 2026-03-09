#include "gm_client_module.h"
#include "log_lua_module.h"
#include "connect_thread.h"
#include "lmisc.h"
#include "msg.h"
#include "lar.h"

const char GMClient::className[] = "GMClient";
const bool GMClient::gc_delete_ = false;

Lunar<GMClient>::RegType GMClient::methods[] =
{   
    {NULL, NULL}
};

GMClient::GMClient()
{
    init_func_map();
}

GMClient::~GMClient()
{
}

void GMClient::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &GMClient::on_connect; 
    func_map_[PT_DISCONNECT] = &GMClient::on_disconnect; 
}

void GMClient::on_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_CONNECT, _dpid );
}

void GMClient::on_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_DISCONNECT, _dpid );

    del_sock( _dpid );
    g_gm_connect_thread->on_conn_disconnect();
}

void GMClient::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (GMClient::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
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
        ERR(2)( "[GMClient](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void GMClient::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( LogLuaSvr::GMCLIENT_MESSAGE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

bool GMClientModule::app_class_init()
{
    char ip[APP_CFG_NAME_MAX] = {0};
    int32_t port = 0;
    int32_t crypt = 0;
    int32_t poll_wait_time = 100;
    int32_t sleep_time = 100;

    APP_GET_TABLE( "GMClient", 2 );
    APP_GET_STRING( "IP", ip );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "SleepTime", sleep_time );
    APP_END_TABLE();

    if( strlen(ip) <= 0 || !port ) 
    {
        ERR(2)( "[GMClientModule](app_class_init)read cfg err!" );
        return false;
    }

    g_gm_connect_thread->init( g_gmclient, ip, port, crypt, poll_wait_time, sleep_time );

    if( g_gm_connect_thread->start() < 0 )
    {
        ERR(2)( "[GMClientModule](app_class_init) start connect thread failed" );  
        return false;
    }

    LOG(2)( "===========GMClient Module Start===========");

    return true;
}

bool GMClientModule::app_class_destroy()
{
    LOG(2)( "===========GMClient Module Destroy===========");
    return true;
}
