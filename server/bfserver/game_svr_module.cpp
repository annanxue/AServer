#include "game_svr_module.h"
#include "bf_lua_module.h"
#include "lar.h"
#include "lmisc.h"
#include "ctrl.h"
#include "timer.h"

const char GameSvr::className[] = "GameSvr";
const bool GameSvr::gc_delete_ = false;

Lunar<GameSvr>::RegType GameSvr::methods[] =
{   
	LUNAR_DECLARE_METHOD( GameSvr, c_get_server_id ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_server_ip ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_server_ip_to_client ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_server_port ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_user_num ),
	LUNAR_DECLARE_METHOD( GameSvr, c_kick_player ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_player_ip),
    {NULL, NULL}
};


GameSvr::GameSvr()
{
    init_func_map();
}

GameSvr::~GameSvr()
{
}

void GameSvr::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &GameSvr::on_client_connect; 
    func_map_[PT_DISCONNECT] = &GameSvr::on_client_disconnect; 
}

void GameSvr::set_server_ip_to_client( const char* _ip_to_client )
{
    strcpy( server_ip_to_client_, _ip_to_client );
}

void GameSvr::on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
}

void GameSvr::on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_DISCONNECT, _dpid );
    del_sock( _dpid );
}

void GameSvr::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (GameSvr::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
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
        ERR(2)( "[GameSvr](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void GameSvr::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( BFLuaSvr::GAMESERVER_MESSAGE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )
    { 
        ERR(2)("[HANDLE](msg) lua failed packet_type:0x%x, dpid:0x%x", _packet_type, _dpid);
        llua_fail( _L, __FILE__, __LINE__ );
    }
}

void GameSvr::kick_player(DPID _dpid)
{
    int fd = _dpid & 0x0000FFFF;
    if (fd != 0) //fd == 0 is for auto_player with ai, without socket
    {
        CommonSocket* sock = get_sock( _dpid );
        if ( !sock ) {
            ERR(2)( "(GameSvr::kick_player), _dpid not exist:0x%08X", _dpid );
            return;
        }
    }

    LOG(2)( "[HANDLE](kick) kick_player _dpid = 0x%08X", _dpid );
    static Ar ar; //useless
    on_client_disconnect( ar, _dpid, NULL, 0 );
}

void GameSvr::kick_all_player()
{
    lua_State* L = g_luasvr->L();
    if( lua_gettop( L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg)kick_all_player lua_gettop is %d", lua_gettop(L) );
        lua_pop( L, lua_gettop(L) );
    }

    g_luasvr->get_lua_ref( BFLuaSvr::KICK_ALL_PLAYER_HANDLER );
    assert( !lua_isnil( L, -1 ) );

    if( llua_call( L, 0, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );

}

int32_t GameSvr::c_get_server_id( lua_State* _L )
{
	lcheck_argc( _L, 0 );

	lua_pushnumber( _L, get_server_id());
	return 1;
}


int32_t GameSvr::c_get_server_ip( lua_State* _L )
{
	lcheck_argc( _L, 0 );

    char ip[APP_CFG_NAME_MAX] = {0};
    get_remote_addr()->GetStringIP(ip);
	lua_pushstring( _L, ip);
	return 1;
}

int32_t GameSvr::c_get_server_ip_to_client( lua_State* _L )
{
	lcheck_argc( _L, 0 );
    lua_pushstring( _L, server_ip_to_client_ );
    return 1;
}

int32_t GameSvr::c_get_server_port( lua_State* _L )
{
	lcheck_argc( _L, 0 );

	lua_pushnumber( _L, get_remote_addr()->GetPort());
	return 1;
}

int32_t GameSvr::c_get_user_num( lua_State* _L )
{
	lcheck_argc( _L, 0 );

	lua_pushnumber( _L, get_user_num());
	return 1;
}

int32_t GameSvr::c_kick_player( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	DPID dpid = (DPID)lua_tonumber( _L, 1 );
    lua_settop(_L, 0); //recall lua again in kick_player
    kick_player(dpid);    
    return 0;
}

int32_t GameSvr::c_get_player_ip( lua_State* _L )
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


bool GameSvrModule::app_class_init()
{
    int32_t max_con = 0;
    int32_t port = 0;
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t proc_thread_num = 4;
    int32_t server_id = -1;
    int32_t timeout_time = 0;
    char ip[APP_CFG_NAME_MAX] = {0};
    char ip_to_client[APP_CFG_NAME_MAX] = {0};

    APP_GET_TABLE( "GameSvr", 2 );
    APP_GET_STRING( "IP", ip );
    APP_GET_STRING( "IPToClient", ip_to_client );
    APP_GET_NUMBER( "MaxCon", max_con );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "ProcThreadNum", proc_thread_num );
    APP_GET_NUMBER_SPLIT( "ServerId", server_id , 8 );
    APP_GET_NUMBER( "TimeOutTime", timeout_time );
	APP_END_TABLE();

    g_gamesvr->set_server_id( server_id );
    g_gamesvr->set_server_ip_to_client( ip_to_client );

    if( !max_con || !port || g_gamesvr->get_server_id() < 0 ) 
    {
        ERR(2)( "[GameSvrModule](app_class_init)read cfg err! server_id: %d", g_gamesvr->get_server_id() );
        return false;
    }

    g_gamesvr->set_timeout( timeout_time * g_timermng->get_frame_rate() );

    if( g_gamesvr->start_server( max_con, port, crypt, ip, poll_wait_time, proc_thread_num ) < 0 )
    {
        ERR(2)( "[GameSvrModule](app_class_init)start_server failed. port: %d, max_con: %d", port, max_con );
        return false;
    }

    Ctrl::set_netmng( g_gamesvr );

    LOG(2)( "===========GameSvr Module Start===========");

    return true;
}

bool GameSvrModule::app_class_destroy()
{
    LOG(2)( "===========GameSvr Module Destroy===========");

    return true;
}


