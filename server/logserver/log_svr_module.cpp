#include "log_svr_module.h"
#include "log_lua_module.h"
#include "lar.h"
#include "lmisc.h"
#include "msg.h"

const char LogSvr::className[] = "LogSvr";
const bool LogSvr::gc_delete_ = false;
char LogSvr::log_str_[65535];

Lunar<LogSvr>::RegType LogSvr::methods[] =
{
	LUNAR_DECLARE_METHOD(LogSvr, c_get_server_ip),
	LUNAR_DECLARE_METHOD(LogSvr, c_get_server_port),
    {NULL, NULL}
};


LogSvr::LogSvr()
{
    init_func_map();
}

LogSvr::~LogSvr()
{
}

void LogSvr::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &LogSvr::on_client_connect; 
    func_map_[PT_DISCONNECT] = &LogSvr::on_client_disconnect; 
    func_map_[LG_TYPE_CERTIFY] = &LogSvr::on_client_certify; 
    func_map_[LG_TYPE_LOG] = &LogSvr::on_log; 
    func_map_[LG_TYPE_ONLINE] = &LogSvr::on_online; 
}

void LogSvr::on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
}

void LogSvr::on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    del_sock( _dpid );
}

void LogSvr::on_client_certify( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    char client_certify_code[MAX_CERTIFY_CODE];
    _ar.read_string( client_certify_code, MAX_CERTIFY_CODE );
    if( strcmp( client_certify_code, certify_code_ ) )
    {
        post_disconnect_msg( _dpid, 0, -1 );
        ERR(2)( "[LogSvr](on_client_certify) certify failed, svr code: %s, client code: %s", certify_code_, client_certify_code );
    }
    else
    {
        LOG(2)( "[LogSvr](on_client_certify) certify success" );
    }
}

void LogSvr::on_log( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    int server_id = -1;
    _ar >> server_id;
    _ar.read_string( log_str_, 65535 );

    lua_State* L = g_luasvr->L();
    if( lua_gettop( L ) != 0 )
    {
        LOG(1)( "[LogSvr](on_log) %s:%d on_log, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(L) );
        lua_pop( L, lua_gettop(L) );
    }
    g_luasvr->get_lua_ref( LogLuaSvr::ON_LOG );
    lua_pushnumber( L, server_id );
    lua_pushstring( L, log_str_ );

    if( llua_call( L, 2, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}

void LogSvr::on_online( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    int server_id = -1;
    int online_num = -1;
    _ar >> server_id;
    _ar >> online_num;

    lua_State* L = g_luasvr->L();
    if( lua_gettop( L ) != 0 )
    {
        LOG(1)( "[LogSvr](on_online) %s:%d on_online, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(L) );
        lua_pop( L, lua_gettop(L) );
    }
    g_luasvr->get_lua_ref( LogLuaSvr::ON_ONLINE);
    lua_pushnumber( L, server_id );
    lua_pushnumber( L, online_num);

    if( llua_call( L, 2, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}


void LogSvr::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (LogSvr::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
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
            ERR(2)( "[LogSvr](msg_handle) unknown packet, packet_type: 0x%08x, dpid: %08x", _packet_type, _dpid );
        }
    }
    else
    {
        ERR(2)( "[LogSvr](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

int32_t LogSvr::c_get_server_ip(lua_State* _L)
{
	lcheck_argc(_L, 0);
	lua_pushstring(_L, get_ip());
	return 1;
}

int32_t LogSvr::c_get_server_port(lua_State* _L)
{
	lcheck_argc(_L, 0);
	const EndPoint* addr = get_remote_addr();
	lua_pushnumber(_L, addr->GetPort());
	return 1;
}


bool LogSvrModule::app_class_init()
{
    int32_t max_con = 0;
    int32_t port = 0;
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t proc_thread_num = 4;
    int32_t server_id = -1;

    APP_GET_TABLE( "LogSvr", 2 );
    APP_GET_NUMBER( "MaxCon", max_con );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "ProcThreadNum", proc_thread_num );
    APP_GET_NUMBER_SPLIT( "ServerId", server_id , 8 );
    APP_GET_STRING( "CertifyCode", g_logsvr->certify_code_ );
    APP_END_TABLE();

    g_logsvr->set_server_id( server_id );

    if( !max_con || !port || g_logsvr->get_server_id() < 0 ) 
    {
        ERR(2)( "[LogSvrModule](app_class_init)read cfg err! server_id: %d", g_logsvr->get_server_id() );
        return false;
    }

    if( g_logsvr->start_server( max_con, port, crypt, NULL, poll_wait_time, proc_thread_num ) < 0 )
    {
        ERR(2)( "[LogSvrModule](app_class_init)start_server failed. port: %d, max_con: %d", port, max_con );
        return false;
    }

    LOG(2)( "===========LogSvr Module Start===========");

    return true;
}

bool LogSvrModule::app_class_destroy()
{
    LOG(2)( "===========LogSvr Module Destroy===========");

    return true;
}
