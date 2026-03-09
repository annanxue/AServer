#include "bf_svr_module.h"
#include "bf_lua_module.h"
#include "lar.h"
#include "lmisc.h"
#include "ctrl.h"

const char BFSvr::className[] = "BFSvr";
const bool BFSvr::gc_delete_ = false;

Lunar<BFSvr>::RegType BFSvr::methods[] =
{   
    method(BFSvr, c_get_bf_id),	
    method(BFSvr, c_get_bf_num),	
    method(BFSvr, c_get_gamesvr_config_url ),
    method(BFSvr, c_get_platform_tag),
    method(BFSvr, c_get_mipush_tag),
    method(BFSvr, c_get_game_id),
    {NULL, NULL}
};


BFSvr::BFSvr()
    :bf_num_(10)
{
    init_func_map();
    memset( mipush_tag_, 0, sizeof(mipush_tag_) );   
}

BFSvr::~BFSvr()
{
}

void BFSvr::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &BFSvr::on_client_connect; 
    func_map_[PT_DISCONNECT] = &BFSvr::on_client_disconnect; 
}

void BFSvr::on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
}

void BFSvr::on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_DISCONNECT, _dpid );
    del_sock( _dpid );
}

void BFSvr::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (BFSvr::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
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
        ERR(2)( "[BFSvr](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void BFSvr::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( BFLuaSvr::BFSVR_MESSAGE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

int BFSvr::c_get_bf_id( lua_State* _L )
{
    lua_pushnumber( _L, g_bfsvr->get_server_id() );
    return 1;
}

int BFSvr::c_get_bf_num( lua_State* _L )
{
    lua_pushnumber( _L, g_bfsvr->get_bf_num() );
    return 1;
}

int BFSvr::c_get_gamesvr_config_url( lua_State* _L )
{
    lua_pushstring( _L, gamesvr_config_url_ );
    return 1;
}

int BFSvr::c_get_platform_tag( lua_State* _L )
{
    lua_pushstring( _L, platform_tag_);
    return 1;
}

int BFSvr::c_get_mipush_tag( lua_State* _L )
{
    lua_pushstring( _L, mipush_tag_);
    return 1;
}

int BFSvr::c_get_game_id( lua_State* _L )
{
    lua_pushstring( _L, game_id_);
    return 1;
}


bool BFSvrModule::app_class_init()
{
    int32_t max_con = 0;
    int32_t port = 0;
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t proc_thread_num = 4;
    int32_t server_id = -1;
    char ip[APP_CFG_NAME_MAX] = {0};
    int bf_num_max = 10;

    APP_GET_STRING( "GameSvrConfigUrl", g_bfsvr->gamesvr_config_url_ ); 
    APP_GET_STRING( "PlatformTag", g_bfsvr->platform_tag_); 
    APP_GET_STRING( "GameID", g_bfsvr->game_id_); 

    APP_GET_STRING( "MIPushTag", g_bfsvr->mipush_tag_); 

    APP_GET_TABLE( "BFSvr", 2 );
    APP_GET_STRING( "IP", ip );
    APP_GET_NUMBER( "MaxCon", max_con );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "ProcThreadNum", proc_thread_num );
	APP_GET_NUMBER_SPLIT( "ServerId", server_id , 8 );
    APP_GET_NUMBER( "BFNumMax", bf_num_max );
    APP_END_TABLE();

    g_bfsvr->set_server_id( server_id );
    g_bfsvr->set_bf_num( bf_num_max );

    if( !max_con || !port || g_bfsvr->get_server_id() < 0 ) 
    {
        ERR(2)( "[BFSvrModule](app_class_init)read cfg err! server_id: %d", g_bfsvr->get_server_id() );
        return false;
    }

    if( g_bfsvr->start_server( max_con, port, crypt, ip, poll_wait_time, proc_thread_num ) < 0 )
    {
        ERR(2)( "[BFSvrModule](app_class_init)start_server failed. port: %d, max_con: %d", port, max_con );
        return false;
    }

    LOG(2)( "===========BFSvr Module Start===========");

    return true;
}

bool BFSvrModule::app_class_destroy()
{
    LOG(2)( "===========BFSvr Module Destroy===========");

    return true;
}


