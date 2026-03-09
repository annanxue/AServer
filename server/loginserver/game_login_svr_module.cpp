#include "lar.h"
#include "msg.h"
#include "game_login_svr_module.h"
#include "login_lua_module.h"

const char GameLoginSvr::className[] = "GameLoginSvr";
const bool GameLoginSvr::gc_delete_ = false;

Lunar<GameLoginSvr>::RegType GameLoginSvr::methods[] =
{   
	LUNAR_DECLARE_METHOD( GameLoginSvr, c_get_server_ip ),
	LUNAR_DECLARE_METHOD( GameLoginSvr, c_get_server_port ),
	LUNAR_DECLARE_METHOD( GameLoginSvr, c_get_platform_tag),
	LUNAR_DECLARE_METHOD( GameLoginSvr, c_get_game_id),
    {NULL, NULL}
};


GameLoginSvr::GameLoginSvr()
{
    init_func_map();
}

GameLoginSvr::~GameLoginSvr()
{
}

void GameLoginSvr::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &GameLoginSvr::on_client_connect; 
    func_map_[PT_DISCONNECT] = &GameLoginSvr::on_client_disconnect; 
}


void GameLoginSvr::on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
}

void GameLoginSvr::on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, GL_TYPE_LOGOUT, _dpid );

    del_sock( _dpid );
}

void GameLoginSvr::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (GameLoginSvr::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
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
        ERR(2)( "[GameLoginSvr](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void GameLoginSvr::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( LoginLuaSvr::GAME_LOGIN_MESSAGE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

int32_t GameLoginSvr::c_get_server_ip( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushstring( _L, get_ip() );
	return 1;
}

int32_t GameLoginSvr::c_get_server_port( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	const EndPoint* addr = get_remote_addr();
	lua_pushnumber( _L, addr->GetPort() );
	return 1;
}

int32_t GameLoginSvr::c_get_platform_tag( lua_State* _L )
{
    lua_pushstring( _L, platform_tag_);
    return 1;
}

int32_t GameLoginSvr::c_get_game_id( lua_State* _L )
{
    lua_pushstring( _L, game_id_);
    return 1;
}


bool GameLoginSvrModule::app_class_init()
{
    int32_t max_con = 0;
    int32_t port = 0;
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t proc_thread_num = 4;
    int32_t server_id = -1;

    APP_GET_STRING( "PlatformTag", g_game_login_svr->platform_tag_);
    APP_GET_STRING( "GameID", g_game_login_svr->game_id_);

    APP_GET_TABLE( "GameLoginSvr", 2 );
    APP_GET_NUMBER( "MaxCon", max_con );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "ProcThreadNum", proc_thread_num );
    APP_GET_NUMBER_SPLIT( "ServerId", server_id , 8 );
    APP_END_TABLE();

    g_game_login_svr->set_server_id( server_id );

    if( !max_con || !port || g_game_login_svr->get_server_id() < 0 ) 
    {
        ERR(2)( "[GameLoginSvrModule](app_class_init)read cfg err! server_id: %d", g_game_login_svr->get_server_id() );
        return false;
    }

    if( g_game_login_svr->start_server( max_con, port, crypt, NULL, poll_wait_time, proc_thread_num ) < 0 )
    {
        ERR(2)( "[GameLoginSvrModule](app_class_init)start_server failed. port: %d, max_con: %d", port, max_con );
        return false;
    }

    LOG(2)( "===========GameLoginSvr Module Start===========");

    return true;
}

bool GameLoginSvrModule::app_class_destroy()
{
    // g_game_login_svr release by auto_ptr

    LOG(2)( "===========GameLoginSvr Module Destroy===========");

    return true;
}



