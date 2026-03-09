#include "robot_client_module.h"
#include "robot_lua_module.h"
#include "connect_thread.h"
#include "lmisc.h"
#include "lar.h"

const char RobotClient::className[] = "RobotClient";
const bool RobotClient::gc_delete_ = false;

Lunar<RobotClient>::RegType RobotClient::methods[] =
{   
	LUNAR_DECLARE_METHOD( RobotClient, c_get_gamesvr_id ),
	LUNAR_DECLARE_METHOD( RobotClient, c_connect_gamesvr ),
	LUNAR_DECLARE_METHOD( RobotClient, c_connect_bfsvr ),
	LUNAR_DECLARE_METHOD( RobotClient, c_disconnect ),
    LUNAR_DECLARE_METHOD( RobotClient, c_add_nonreconnect_dpid ),
    LUNAR_DECLARE_METHOD( RobotClient, c_get_login_type ),
    {NULL, NULL}
};

RobotClient::RobotClient()
{
    init_func_map();
}

RobotClient::~RobotClient()
{
    memset( loginsvr_ip_, 0, sizeof(loginsvr_ip_) );
    loginsvr_port_ = 0;
    memset( gamesvr_ip_, 0, sizeof(gamesvr_ip_) );
    gamesvr_port_ = 0;
}

void RobotClient::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &RobotClient::on_connect; 
    func_map_[PT_DISCONNECT] = &RobotClient::on_disconnect; 
}

int RobotClient::init_multi_client( int _max_con, int _timeval )
{
    nonreconnect_dpids_.init( _max_con, _max_con, "RobotClient" );
    return NetMng::init_multi_client( _max_con, _timeval );
}

void RobotClient::set_loginsvr_addr( const char* _ip, int32_t _port )
{
    strcpy( loginsvr_ip_, _ip );
    loginsvr_port_ = _port;
}

void RobotClient::set_gamesvr_id( int32_t _gamesvr_id )
{
    gamesvr_id_ = _gamesvr_id;
}

void RobotClient::set_gamesvr_addr( const char* _ip, int32_t _port )
{
    strcpy( gamesvr_ip_, _ip );
    gamesvr_port_ = _port;
}

void RobotClient::on_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    CommonSocket* sock = get_sock( _dpid );
    if ( sock )
    {
        const EndPoint* ep = sock->get_endpoint();
        char ip[MAX_IP] = { 0 };
        ep->GetStringIP( ip );
        int32_t port = ep->GetPort();

        if ( strcmp( ip, loginsvr_ip_ ) == 0 && port == loginsvr_port_ )
        {
            lua_on_loginsvr_connect( g_luasvr->L(), _dpid );
        }
        else if ( strcmp( ip, gamesvr_ip_ ) == 0 && port == gamesvr_port_ )
        {
            lua_on_gamesvr_connect( g_luasvr->L(), _dpid );
        }
        else
        {
            lua_on_bfsvr_connect( g_luasvr->L(), _dpid, ip, port );
        }
    }
}

void RobotClient::lua_on_loginsvr_connect( lua_State* _L, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[ROBOT](lua_on_loginsvr_connect) %s:%d, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( RobotLuaSvr::ON_LOGINSVR_CONNECT );
    lua_pushnumber( _L, _dpid );
    assert( !lua_isnil( _L, -2 ) );

    if( llua_call( _L, 1, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

void RobotClient::lua_on_gamesvr_connect( lua_State* _L, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[ROBOT](lua_on_gamesvr_connect) %s:%d, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( RobotLuaSvr::ON_GAMESVR_CONNECT );
    lua_pushnumber( _L, _dpid );
    assert( !lua_isnil( _L, -2 ) );

    if( llua_call( _L, 1, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

void RobotClient::lua_on_bfsvr_connect( lua_State* _L, DPID _dpid, const char* _ip, int32_t _port )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[RobotClient](lua_on_bfsvr_connect) %s:%d, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( RobotLuaSvr::ON_BFSVR_CONNECT );
    lua_pushnumber( _L, _dpid );
    lua_pushstring( _L, _ip );
    lua_pushnumber( _L, _port );
    assert( !lua_isnil( _L, -4 ) );

    if( llua_call( _L, 3, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

void RobotClient::set_user_id_begin_num( int _begin_num )
{
    lua_State* L = g_luasvr->L();
    g_luasvr->get_lua_ref( RobotLuaSvr::ROBOT_USER_ID_HANDLER );
    lua_pushinteger( L, _begin_num );
    assert( !lua_isnil( L, -2 ) );
    if( llua_call( L, 1, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}

void RobotClient::set_robot_type( int _type )
{
    lua_State* L = g_luasvr->L();
    g_luasvr->get_lua_ref( RobotLuaSvr::SET_ROBOT_TYPE );
    lua_pushinteger( L, _type );
    assert( !lua_isnil( L, -2 ) );
    if( llua_call( L, 1, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}

void RobotClient::on_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    del_sock( _dpid );
    bool reconnect = !nonreconnect_dpids_.del( _dpid );
    g_multi_connect_thread->on_conn_disconnect( _dpid, reconnect );
    // lua_msg_handle调用必须放在g_multi_connect_thread->on_conn_disconnect之后，
    // 因为lua_msg_handle中可能会发起新的连接，此时需保证连接数不超过最大值。
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_DISCONNECT, _dpid );  
}

void RobotClient::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (RobotClient::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
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
        ERR(2)( "[RobotClient](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void RobotClient::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( RobotLuaSvr::ROBOT_MESSAGE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

int32_t RobotClient::c_get_gamesvr_id( lua_State* _L )
{
	lcheck_argc( _L, 0 );
    lua_pushnumber( _L, gamesvr_id_ );
    return 1;
}

int32_t RobotClient::c_connect_gamesvr( lua_State* _L )
{
	lcheck_argc( _L, 0 );
    g_multi_connect_thread->add_conn( gamesvr_ip_, static_cast<uint16_t>( gamesvr_port_ ), false );
    return 0;
}

int32_t RobotClient::c_connect_bfsvr( lua_State* _L )
{
	lcheck_argc( _L, 2 );
    const char *ip = lua_tostring( _L, 1 );
    int32_t port = lua_tonumber( _L, 2 );
    g_multi_connect_thread->add_conn( ip, static_cast<uint16_t>( port ), false );
    return 0;
}

int32_t RobotClient::c_disconnect( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	DPID dpid = (DPID)lua_tonumber( _L, 1 );
    nonreconnect_dpids_.set( dpid, 1 );  // tell MultiConnectThread not to reconnect
    static Ar ar; //useless
    lua_settop(_L, 0); //recall lua again in on_disconnect
    on_disconnect( ar, dpid, NULL, 0 );
    LOG(2)( "[RobotClient](c_disconnect) dpid = %d", dpid );
    return 0;
}

int32_t RobotClient::c_add_nonreconnect_dpid( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	DPID dpid = (DPID)lua_tonumber( _L, 1 );
    nonreconnect_dpids_.set( dpid, 1 );  // tell MultiConnectThread not to reconnect
    return 0;
}

int32_t RobotClient::c_get_login_type( lua_State* _L )
{
    lua_pushstring( _L, login_type_ );
    return 1;
}

bool RobotClientModule::app_class_init()
{
    char loginsvr_ip[APP_CFG_NAME_MAX] = {0};
    int32_t loginsvr_port = 0;
    int32_t gamesvr_id = 1;
    char gamesvr_ip[APP_CFG_NAME_MAX] = {0};
    int32_t gamesvr_port = 0;
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t sleep_time = 100;
    int32_t player_num = 3000;
    int32_t robot_type = 0;
    int32_t begin_num = 0;
    char login_type[APP_CFG_NAME_MAX] = {0};

    APP_GET_TABLE( "RobotClient", 2 );
    APP_GET_NUMBER( "PlayerNum", player_num );
    APP_GET_NUMBER( "RobotType", robot_type );
    APP_GET_STRING( "LoginSvrIP", loginsvr_ip );
    APP_GET_NUMBER( "LoginSvrPort", loginsvr_port );
    APP_GET_NUMBER( "GameSvrID", gamesvr_id );
    APP_GET_STRING( "GameSvrIP", gamesvr_ip );
    APP_GET_NUMBER( "GameSvrPort", gamesvr_port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "SleepTime", sleep_time );
    APP_GET_NUMBER( "UserIdBeginNum", begin_num );
    APP_GET_STRING( "LoginType", login_type );
    APP_END_TABLE();

    if( strlen(loginsvr_ip) <= 0 || !loginsvr_port || strlen(gamesvr_ip) <= 0 || !gamesvr_port ) 
    {
        ERR(2)( "[RobotClientModule](app_class_init) ip:port err, read cfg err!" );
        return false;
    }

    g_robotclient->set_login_type( login_type );
    g_robotclient->set_user_id_begin_num( begin_num );
    g_robotclient->set_robot_type( robot_type );

    g_robotclient->set_loginsvr_addr( loginsvr_ip, loginsvr_port );
    g_robotclient->set_gamesvr_id( gamesvr_id );
    g_robotclient->set_gamesvr_addr( gamesvr_ip, gamesvr_port );

    g_robotclient->init_multi_client( player_num, poll_wait_time );
    g_multi_connect_thread->init( g_robotclient, player_num, crypt, sleep_time );
    for( int32_t i = 0; i < player_num; i++ )
    {
        g_multi_connect_thread->add_conn( loginsvr_ip, loginsvr_port );
    }

    if( g_multi_connect_thread->start() < 0 )
    {
        ERR(2)( "start multi-connect thread failed" );  
        return false;
    }

    LOG(2)( "===========RobotClient Module Start===========");

    return true;
}

bool RobotClientModule::app_class_destroy()
{
    LOG(2)( "===========RobotClient Module Destroy===========");

    return true;
}



