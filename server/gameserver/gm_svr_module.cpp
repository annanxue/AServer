#include "lar.h"
#include "msg.h"
#include "gm_svr_module.h"
#include "game_lua_module.h"
#include "timer.h"
#include "lmisc.h"

const char GmSvr::className[] = "GmSvr";
const bool GmSvr::gc_delete_ = false;

Lunar<GmSvr>::RegType GmSvr::methods[] =
{   
    {NULL, NULL}
};


GmSvr::GmSvr()
{
    init_func_map();
}

GmSvr::~GmSvr()
{
}

void GmSvr::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_) );   
    func_map_[PT_CONNECT] = &GmSvr::on_client_connect; 
    func_map_[PT_DISCONNECT] = &GmSvr::on_client_disconnect; 
}


void GmSvr::on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
}

void GmSvr::on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_DISCONNECT, _dpid );

    del_sock( _dpid );
}

void GmSvr::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (GmSvr::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
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
        ERR(2)( "[GmSvr](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void GmSvr::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( GameLuaSvr::GM_SVR_MESSAHE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

bool GmSvrModule::app_class_init()
{
    int32_t max_con = 0;
    int32_t port = 0;
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t proc_thread_num = 4;
    int32_t timeout_time = 0;

    APP_GET_TABLE( "GmSvr", 2 );
    APP_GET_NUMBER( "MaxCon", max_con );
    APP_GET_NUMBER( "Port", port );
    APP_GET_NUMBER( "Crypt", crypt );
    APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
    APP_GET_NUMBER( "ProcThreadNum", proc_thread_num );
    APP_GET_NUMBER( "TimeOutTime", timeout_time );
    APP_END_TABLE();

    if( !max_con || !port ) 
    {
        ERR(2)( "[GmSvrModule](app_class_init)read cfg err!");
        return false;
    }

    g_gmsvr->set_timeout( timeout_time * g_timermng->get_frame_rate() );
    if( g_gmsvr->start_server( max_con, port, crypt, NULL, poll_wait_time, proc_thread_num ) < 0 )
    {
        ERR(2)( "[GmSvrModule](app_class_init)start_server failed. port: %d, max_con: %d", port, max_con );
        return false;
    }

    LOG(2)( "===========GmSvr Module Start===========");

    return true;
}

bool GmSvrModule::app_class_destroy()
{
    // g_gm_svr release by auto_ptr

    LOG(2)( "===========GmSvr Module Destroy===========");

    return true;
}



