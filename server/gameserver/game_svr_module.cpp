#include "game_svr_module.h"
#include "game_lua_module.h"
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
	LUNAR_DECLARE_METHOD( GameSvr, c_get_server_port ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_user_num ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_allow_shops_on_bf ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_allow_wild_cross_zone ),
	LUNAR_DECLARE_METHOD( GameSvr, c_kick_player ),
	LUNAR_DECLARE_METHOD( GameSvr, c_start_server ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_player_ip ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_gamesvr_config_url ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_game_area ),
	LUNAR_DECLARE_METHOD( GameSvr, c_is_china ),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_platform_tag),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_order_query_url),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_order_ack_url),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_order_compensate_url),
	LUNAR_DECLARE_METHOD(GameSvr, c_get_ema_gift_order_query_url),	
	LUNAR_DECLARE_METHOD(GameSvr, c_get_ema_gift_order_url_ack),	
	LUNAR_DECLARE_METHOD( GameSvr, c_get_mipush_tag),
	LUNAR_DECLARE_METHOD( GameSvr, c_get_game_id),
    {NULL, NULL}
};


GameSvr::GameSvr()
{
    init_func_map();

    max_con_ = 0;
    memset( ip_, 0, sizeof(ip_) );
    port_ = 0;
    crypt_ = 1;
    poll_wait_time_ = 100;
    proc_thread_num_ = 4; 
    allow_shops_on_bf_ = false;
    allow_wild_cross_zone_ = false;
    memset( gamesvr_config_url_, 0, sizeof( gamesvr_config_url_ ));
    memset( game_area_, 0, sizeof( game_area_ ));
    memset( platform_tag_, 0, sizeof( platform_tag_ ));
    memset( order_query_url_, 0, sizeof( order_query_url_ ));
    memset( order_ack_url_, 0, sizeof( order_ack_url_ ));
    memset( order_compensate_url_, 0, sizeof( order_compensate_url_ ));
    memset( game_id_, 0, sizeof( game_id_ ));
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

void GameSvr::on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_CONNECT, _dpid );
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
    g_luasvr->get_lua_ref( GameLuaSvr::GAMESERVER_MESSAGE_HANDLER );
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

void GameSvr::lua_set_max_user_num( lua_State* _L, int _max_user_num )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[GameSvr](lua_set_max_user_num) %s:%d lua_gettop() is %d", __FILE__, __LINE__, lua_gettop( _L ) );
        lua_pop( _L, lua_gettop( _L ) );
    }
    g_luasvr->get_lua_ref( GameLuaSvr::SET_MAX_USER_NUM );
    lua_pushnumber( _L, _max_user_num );
    if( llua_call( _L, 1, 0, 0 ) ) 
    {
        llua_fail( _L, __FILE__, __LINE__ );
    }
}

void GameSvr::set_start_param(int32_t _max_con, char* _ip, int32_t _port, int32_t _crypt, int32_t _poll_wait_time, int32_t _proc_thread_num)
{
    max_con_ = _max_con;
    strncpy( ip_, _ip, sizeof(ip_) );
    port_ = _port;
    crypt_ = _crypt;
    poll_wait_time_ = _poll_wait_time;
    proc_thread_num_ = _proc_thread_num;

}

void GameSvr::kick_player(DPID _dpid)
{
    CommonSocket* sock = get_sock( _dpid );
    if ( !sock ) {
        ERR(2)( "(GameSvr::kick_player), _dpid not exist:0x%08X", _dpid );
        return;
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

    g_luasvr->get_lua_ref( GameLuaSvr::KICK_ALL_PLAYER_HANDLER );
    assert( !lua_isnil( L, -1 ) );

    if( llua_call( L, 0, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );

}

void GameSvr::on_server_pre_stop()
{
    lua_State* L = g_luasvr->L();
    if( lua_gettop( L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg)on_server_pre_stop lua_gettop is %d", lua_gettop(L) );
        lua_pop( L, lua_gettop(L) );
    }

    g_luasvr->get_lua_ref( GameLuaSvr::ON_SERVER_PRE_STOP );
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

	lua_pushstring( _L, ip_);
	return 1;
}

int32_t GameSvr::c_get_server_port( lua_State* _L )
{
	lcheck_argc( _L, 0 );

	lua_pushnumber( _L, port_);
	return 1;
}

int32_t GameSvr::c_get_user_num( lua_State* _L )
{
	lcheck_argc( _L, 0 );

	lua_pushnumber( _L, get_user_num());
	return 1;
}

int32_t GameSvr::c_get_allow_shops_on_bf( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushboolean( _L, allow_shops_on_bf_ );
    return 1;
}

int32_t GameSvr::c_get_allow_wild_cross_zone( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushboolean( _L, allow_wild_cross_zone_ );
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

int32_t	GameSvr::c_start_server( lua_State* _L )
{
	lcheck_argc( _L, 0 );
    int ret = start_server( max_con_, port_, crypt_, ip_, poll_wait_time_, proc_thread_num_ ); 
    if ( ret < 0 )
    {
        ERR(2)( "[PlayerLoginSvrModule](app_class_init)start_server failed. port: %d, max_con: %d", port_, max_con_ );
    }
    else
    {
        Ctrl::set_netmng( this );
    }
    lua_pushboolean( _L, ret >= 0 );
	return 1;
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

int GameSvr::c_get_gamesvr_config_url( lua_State* _L )
{
    lua_pushstring( _L, gamesvr_config_url_ );
    return 1;
}

int GameSvr::c_get_game_area( lua_State* _L )
{
    lua_pushstring( _L, game_area_ );
    return 1;
}

int GameSvr::c_is_china(lua_State* _L)
{
	lua_pushboolean(_L, is_china_);
	return 1;
}

int32_t GameSvr::c_get_platform_tag( lua_State* _L )
{
    lua_pushstring( _L, platform_tag_);
    return 1;
}

int32_t GameSvr::c_get_order_query_url( lua_State* _L )
{
    lua_pushstring( _L, order_query_url_ );
    return 1;
}

int32_t GameSvr::c_get_ema_gift_order_query_url(lua_State* _L)
{
	lua_pushstring(_L, ema_gift_order_query_url_);
	return 1;
}

int32_t GameSvr::c_get_ema_gift_order_url_ack(lua_State* _L)
{
	lua_pushstring(_L, ema_gift_order_url_ack_);
	return 1;
}

int32_t GameSvr::c_get_order_ack_url( lua_State* _L )
{
    lua_pushstring( _L, order_ack_url_ );
    return 1;
}

int32_t GameSvr::c_get_order_compensate_url( lua_State* _L )
{
    lua_pushstring( _L, order_compensate_url_ );
    return 1;
}

int32_t GameSvr::c_get_mipush_tag( lua_State* _L )
{
    lua_pushstring( _L, mipush_tag_ );
    return 1;
}

int32_t GameSvr::c_get_game_id( lua_State* _L )
{
    lua_pushstring( _L, game_id_);
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
    int32_t allow_shops_on_bf = 0;
    int32_t allow_wild_cross_zone = 0;

	int32_t is_china = 0;


    char ip[APP_CFG_NAME_MAX] = {0};
    char gamesvr_config_url[APP_CFG_NAME_MAX] = {0};
    char game_area[APP_CFG_NAME_MAX] = {0};
    char platform_tag[APP_CFG_NAME_MAX] = {0};
    char order_query_url[APP_CFG_NAME_MAX] = {0};
    char order_ack_url[APP_CFG_NAME_MAX] = {0};
    char order_compensate_url[APP_CFG_NAME_MAX] = {0};
	char ema_gift_order_query_url[APP_CFG_NAME_MAX] = { 0 };
	char ema_gift_order_url_ack[APP_CFG_NAME_MAX] = { 0 };
    char mipush_tag[APP_CFG_NAME_MAX] = {0};
    char game_id[APP_CFG_NAME_MAX] = {0};

    APP_GET_NUMBER( "AllowShopsOnBF", allow_shops_on_bf );
    APP_GET_NUMBER( "AllowWildCrossZone", allow_wild_cross_zone );
    APP_GET_STRING( "GameSvrConfigUrl", gamesvr_config_url ); 
    APP_GET_STRING( "GameArea", game_area ); 
	APP_GET_NUMBER("IsChina", is_china);
    APP_GET_STRING( "PlatformTag", platform_tag); 
    APP_GET_STRING( "GameID", game_id); 

    APP_GET_STRING( "OrderQueryUrl", order_query_url ); 
    APP_GET_STRING( "OrderAckUrl", order_ack_url ); 
    APP_GET_STRING( "OrderCompensateUrl", order_compensate_url ); 
	APP_GET_STRING("EmaGiftOrderQueryUrl", ema_gift_order_query_url);
	APP_GET_STRING("EmaGiftOrderUrlAck", ema_gift_order_url_ack);

    APP_GET_STRING( "MIPushTag", mipush_tag ); 

    APP_GET_TABLE( "GameSvr", 2 );
    APP_GET_STRING( "IP", ip );
    APP_GET_NUMBER( "MaxCon", max_con );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "ProcThreadNum", proc_thread_num );
	APP_GET_NUMBER_SPLIT( "ServerId", server_id , 8 );
    APP_GET_NUMBER( "TimeOutTime", timeout_time );
    APP_END_TABLE();

    g_gamesvr->set_server_id( server_id );

    if( !max_con || !port || g_gamesvr->get_server_id() < 0 ) 
    {
        ERR(2)( "[GameSvrModule](app_class_init)read cfg err! server_id: %d", g_gamesvr->get_server_id() );
        return false;
    }

    g_gamesvr->set_timeout( timeout_time * g_timermng->get_frame_rate() );

    g_gamesvr->set_start_param( max_con, ip, port, crypt, poll_wait_time, proc_thread_num );

    g_gamesvr->set_allow_shops_on_bf( allow_shops_on_bf != 0 );

    g_gamesvr->set_allow_wild_cross_zone( allow_wild_cross_zone != 0 );

    g_gamesvr->set_gamesvr_config_url( gamesvr_config_url );

    g_gamesvr->set_game_area( game_area );

	g_gamesvr->set_is_china(is_china == 1);

    g_gamesvr->set_platform_tag( platform_tag );

    g_gamesvr->set_order_query_url( order_query_url );
    g_gamesvr->set_order_ack_url( order_ack_url );
    g_gamesvr->set_order_compensate_url( order_compensate_url );
	g_gamesvr->set_ema_gift_order_query_url(ema_gift_order_query_url);
	g_gamesvr->set_ema_gift_order_url_ack(ema_gift_order_url_ack);

    g_gamesvr->set_mipush_tag( mipush_tag );

    g_gamesvr->set_game_id( game_id );

    g_gamesvr->lua_set_max_user_num( g_luasvr->L(), max_con );


    LOG(2)( "===========GameSvr Module Start===========");
    LOG(2)( "===========WAIT GLOBAL SYSTEM DATA FROM DB to START SERVER===========");

    return true;
}

bool GameSvrModule::app_class_destroy()
{
    LOG(2)( "===========GameSvr Module Destroy===========");

    return true;
}


