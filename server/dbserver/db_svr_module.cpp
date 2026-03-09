#include "db_svr_module.h"
#include "db_lua_module.h"
#include "lar.h"
#include "msg.h"

const char DbSvr::className[] = "DbSvr";
const bool DbSvr::gc_delete_ = false;

Lunar<DbSvr>::RegType DbSvr::methods[] =
{   
	LUNAR_DECLARE_METHOD( DbSvr, c_get_server_id ),
    {NULL, NULL}
};


DbSvr::DbSvr()
{
    init_func_map();
}

DbSvr::~DbSvr()
{
}

void DbSvr::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &DbSvr::on_client_connect; 
    func_map_[PT_DISCONNECT] = &DbSvr::on_client_disconnect; 
    func_map_[DB_TYPE_REG_SERVERID] = &DbSvr::on_register_server_id;
    func_map_[DB_TYPE_LAST_MSG] = &DbSvr::on_gameserver_last_msg;
}

int32_t DbSvr::c_get_server_id( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, get_server_id());
	return 1;
}

void DbSvr::on_gameserver_last_msg( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    post_disconnect_msg( _dpid, 0, -1 );
    LOG(2)( "[DbSvr](on_gameserver_last_msg)" );
}

void DbSvr::on_register_server_id( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    int server_id;
    _ar >> server_id;
    if( server_id != g_dbsvr->get_server_id() )
    {
        post_disconnect_msg( _dpid, 0, -1 );
        ERR(2)( "[DbSvr](on_register_server_id)game serverid: %d != db serverid: %d", server_id, g_dbsvr->get_server_id() );
    }
    else
    {
        g_luasvr->set_game_dpid( _dpid );
    }
}

void DbSvr::on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
}

void DbSvr::on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_DISCONNECT, _dpid );
    del_sock( _dpid );
}

void DbSvr::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (DbSvr::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
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
        ERR(2)( "[DbSvr](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void DbSvr::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( DBLuaSvr::DB_MESSAGE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}


bool DbSvrModule::app_class_init()
{
    int32_t max_con = 0;
    int32_t port = 0;
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t proc_thread_num = 4;
    int32_t server_id = -1;

    APP_GET_TABLE( "DbSvr", 2 );
    APP_GET_NUMBER( "MaxCon", max_con );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "ProcThreadNum", proc_thread_num );
    APP_GET_NUMBER_SPLIT( "ServerId", server_id , 8 );
    APP_END_TABLE();

    g_dbsvr->set_server_id( server_id );

    if( !max_con || !port || g_dbsvr->get_server_id() < 0 ) 
    {
        ERR(2)( "[DbSvrModule](app_class_init)read cfg err! server_id: %d", g_dbsvr->get_server_id() );
        return false;
    }

    if( g_dbsvr->start_server( max_con, port, crypt, NULL, poll_wait_time, proc_thread_num ) < 0 )
    {
        ERR(2)( "[DbSvrModule](app_class_init)start_server failed. port: %d, max_con: %d", port, max_con );
        return false;
    }

    LOG(2)( "===========DbSvr Module Start===========");

    return true;
}

bool DbSvrModule::app_class_destroy()
{
    // g_dbsvr release by auto_ptr

    LOG(2)( "===========DbSvr Module Destroy===========");

    return true;
}



