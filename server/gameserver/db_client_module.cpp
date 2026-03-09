#include "db_client_module.h"
#include "game_lua_module.h"
#include "game_svr_module.h"
#include "connect_thread.h"
#include "lmisc.h"
#include "msg.h"
#include "lar.h"
#include "app_base.h"

const char DBClient::className[] = "DBClient";
const bool DBClient::gc_delete_ = false;

Lunar<DBClient>::RegType DBClient::methods[] =
{   
	LUNAR_DECLARE_METHOD( DBClient, c_is_connected ),
    {NULL, NULL}
};

DBClient::DBClient()
{
    init_func_map();
}

DBClient::~DBClient()
{
}

void DBClient::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &DBClient::on_connect; 
    func_map_[PT_DISCONNECT] = &DBClient::on_disconnect; 
}

void DBClient::send_register_server_id()
{
    BEFORESEND( ar, DB_TYPE_REG_SERVERID );
    ar << g_gamesvr->get_server_id();
    SEND( ar, g_dbclient, 0 );
}

void DBClient::send_last_msg()
{
    BEFORESEND( ar, DB_TYPE_LAST_MSG );
    SEND( ar, g_dbclient, 0 );
}

void DBClient::on_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    g_connect_thread->stop();  // 一旦连接上db，之后如果和db的连接断开，则game应停机，而不应再重连db。
    send_register_server_id();
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_CONNECT, _dpid );
}

void DBClient::on_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    g_connect_thread->on_conn_disconnect();
    del_sock( _dpid );
    LOG(2)( "[DBClient](on_disconnect)exit server as disconnected from db." );
    AppBase::active_ = false;
}

void DBClient::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (DBClient::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
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
        ERR(2)( "[DBClient](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void DBClient::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( GameLuaSvr::DBCLIENT_MESSAGE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

int32_t DBClient::c_is_connected( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushboolean( _L, g_connect_thread->is_connected() );
	return 1;
}

bool DBClientModule::app_class_init()
{
    char ip[APP_CFG_NAME_MAX] = {0};
    int32_t port = 0;
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t sleep_time = 100;

    APP_GET_TABLE( "DBClient", 2 );
    APP_GET_STRING( "IP", ip );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "SleepTime", sleep_time );
    APP_END_TABLE();

    if( strlen(ip) <= 0 || !port ) 
    {
        ERR(2)( "[DBClientModule](app_class_init)read cfg err!" );
        return false;
    }

    g_connect_thread->init( g_dbclient, ip, port, crypt, poll_wait_time, sleep_time );

    if( g_connect_thread->start() < 0 )
    {
        ERR(2)( "start connected thread failed" );  
        return false;
    }

    LOG(2)( "===========DBClient Module Start===========");

    return true;
}

bool DBClientModule::app_class_destroy()
{
    LOG(2)( "===========DBClient Module Destroy===========");

    return true;
}



