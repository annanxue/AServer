#include "player.h"
#include "log.h"
#include "define_func.h"
#include "world_mng.h"
#include "player_mng.h"
#include "llog.h"
#include "monster.h"
#include "resource.h"
#include "tickcount.h"
#include "msg.h"

#define DEFAULT_PLAYER_RADIUS	0.5

int Player::view_add_num_ = 0;
int Player::view_rm_num_ = 0;
int Player::view_repeat_num_ = 0;

const int MAX_SNAPSHOT_SIZE = 256;

Player::Player()
{
	settype( OT_PLAYER );
	dpid_ = DPID_UNKNOWN;
	set_radius( DEFAULT_PLAYER_RADIUS );

    player_id_ = NULL_ID;
    dpid_ = NULL_ID;
    
    dest_tele_plane_ = 0;
}

Player::~Player()
{
}

void Player::process_parallel()
{
    Spirit::process_parallel();
    process_region();
}

void Player::process_serial()
{
}

void Player::serialize_active( Ar* _ar )
{
}

/*************************************************
	视野管理
*************************************************/

void Player::process_region()
{
}

int Player::view_add( Ctrl* _obj )
{
    assert(_obj);

    if( _obj->is_server_only() && _obj != this )
    {
        return FALSE;
    }

    add_addobj( _obj );

    return TRUE;
}

int Player::view_remove( Ctrl* _obj )
{
    assert(_obj);

    if( _obj == this ) 
    {
        return FALSE;
    }

    if( _obj->is_server_only() )
    {
        return FALSE;
    }

    add_removeobj( _obj );

    return TRUE;
}

void Player::before_replace( World* _old, World* _new )
{
    Spirit::before_replace( _old, _new );
    // notify lua
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "before_replace" );
    lua_pushnumber( L, this->get_id() );
    lua_pushnumber( L, _new->get_scene()->get_id() );
    if( llua_call( L, 3, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}

void Player::after_replace( World* _old, World* _new )
{
    Spirit::after_replace( _old, _new );
    // notify lua
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "after_replace" );
    lua_pushnumber( L, this->get_id() );
    lua_pushnumber( L, _new->get_scene()->get_id() );
    if( llua_call( L, 3, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}

void Player::after_add( World* _world )
{
    Spirit::after_add( _world );
}

/*************************************************
	收发包
*************************************************/

void Player::notify()
{
    if( is_delete() ) {
        return;
    }

    snapshot_.lock();

    if( snapshot_.cb_ > 0 )
    {    
        bool complete = false;
        CommonSocket* sock = Ctrl::get_netmng()->get_sock( dpid_ );
        if( sock ) {
            snapshot_.set_header( dpid_ );
            Ctrl::get_netmng()->add_send_msg( snapshot_.ar_.buffer_ );
            snapshot_.reset();
            complete = true;
        }    

        if( complete == false ) {
            SAFE_DELETE( snapshot_.ar_.buffer_ );   
            snapshot_.reset();
        }    
    }    

    snapshot_.unlock();
}

void Player::check_ar()
{
    int buflen = 0; 
    snapshot_.ar_.get_buffer(&buflen);
    if (buflen > MAX_SNAPSHOT_SIZE) notify();
}

void Player::add_block( const char* buf, int len )
{
    if( is_delete() ) 
    {
        return;
    }

    check_ar();

    unsigned char packet_type = *((unsigned char*)buf);

    snapshot_.lock();

    if( snapshot_.is_packet_can_combine( packet_type, buf ) )
    {
        snapshot_.ar_.buf_cur_ -= snapshot_.last_send_packet_length_;
        snapshot_.log_combine( get_id() );
    }
    else
    {
        snapshot_.cb_++;
    }

    AR_TRACE_BEGIN( packet_type );
    snapshot_.ar_.write( buf, len );
    AR_TRACE_END();

    snapshot_.unlock();
}

void Player::add_addobj( Ctrl* _obj )
{
    if( is_delete() ) return; 

    // just call lua
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "add_addobj" );
    lua_pushnumber( L, this->get_id() );
    lua_pushnumber( L, _obj->get_id() );

    if( llua_call( L, 3, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}


void Player::add_removeobj( Ctrl* _obj )
{
    // just call lua
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "add_removeobj" );
    lua_pushnumber( L, this->get_id() );
    lua_pushnumber( L, _obj->get_id() );

    if( llua_call( L, 3, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}


void Player::add_replace( u_long _scene_id, VECTOR3& _pos )
{
    if( is_delete() )       return;
    check_ar();
    snapshot_.lock();
    snapshot_.cb_++;

    AR_TRACE_BEGIN( ST_REPLACE ); 
    SET_SNAPSHOT_TYPE( snapshot_.ar_, ST_REPLACE );
    snapshot_.ar_ << _scene_id;
    snapshot_.ar_ << _pos.x;   
    snapshot_.ar_ << _pos.z;   
    AR_TRACE_END(); 

    snapshot_.unlock(); 
}

void Player::add_setpos( VECTOR3& _pos )
{
    snapshot_.lock();
    snapshot_.cb_++;

    AR_TRACE_BEGIN( ST_SETPOS ); 
    SET_SNAPSHOT_TYPE( snapshot_.ar_, ST_SETPOS );
    snapshot_.ar_ << get_id();
    snapshot_.ar_ << _pos.x;   
    snapshot_.ar_ << _pos.z;   
    AR_TRACE_END(); 

    snapshot_.unlock();
}

/******************************************************************************
for lua
******************************************************************************/


const char Player::className[] = "Player";
const bool Player::gc_delete_ = false;

Lunar<Player>::RegType Player::methods[] =
{
	LUNAR_OBJ_METHODS,
	LUNAR_CTRL_METHODS,
	LUNAR_SPIRIT_METHODS,
	LUNAR_PLAYER_METHODS,
	{NULL, NULL}
};

int Player::c_set_dest_tele_plane( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    int plane = (int)lua_tonumber( _L, 1 );
    int old = dest_tele_plane_;
    dest_tele_plane_ = plane;
    TRACE(2)( "[c_set_dest_tele_plane] old = %d new = %d ", old, plane  );
    return 0;
}

int Player::c_get_dest_tele_plane( lua_State* _L )
{
    lcheck_argc( _L, 0 );
    lua_pushnumber( _L, dest_tele_plane_ );
    return 1;
}

int	Player::c_set_dpid( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	DPID dpid = (DPID)lua_tonumber( _L, 1 );
	set_dpid( dpid );
	return 0;
}



int	Player::c_get_dpid( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, get_dpid() );
	return 1;
}

int Player::c_get_id( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, get_id());
	return 1;
}

int	Player::c_get_player_id( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, get_playerid() );
	TRACE(2)( "[OBJ](player) Player::c_get_player_id, id = %d", get_playerid() );
	return 1;
}

int Player::c_set_player_id( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	player_id_ = (u_long)lua_tonumber( _L, 1 );
	return 0;	
}

int Player::c_add_block_one_ar( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	LAr* lar = Lunar<LAr>::check( _L, 1 );
	if( lar == 0 ) return 0;

    if( lar->flush_flag_ != 1 ) {
		ERR(2)( "[LUAWRAPPER](lar) %s:%d flush flag not set, this is very dangerous!!!! pls concat: sodme !!!! ", __FILE__, __LINE__ );
		return 0;
    }

	int buf_size;
	char* buf = lar->ar_->get_buffer( &buf_size );
	if ( buf_size != 0 )
	{
		add_block( buf, buf_size );
	}
	//lar->destroy();
	return 0;
}

int Player::c_add_block( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	LAr* lar = Lunar<LAr>::check( _L, 1 );
	if( lar == 0 ) return 0;

	int buf_size;
	char* buf = lar->ar_->get_buffer( &buf_size );
	if ( buf_size != 0 )
	{
		add_block( buf, buf_size );
	}
	lar->destroy();
	return 0;
}

int Player::c_serialize( lua_State* _L )
{
	lcheck_argc( _L, 2 );
	LAr* lar = Lunar<LAr>::check( _L, 1 );
	bool flag = lua_toboolean( _L, 2 );
	serialize( * (lar->ar_), flag );
	return 0;
}

int Player::c_replace_pos( lua_State* _L )
{
	lcheck_argc( _L, 4 );
	u_long worldid = (u_long)lua_tointeger( _L, -4 );
	float x = lua_tonumber( _L, -3 );
	float y = lua_tonumber( _L, -2 );
	float z = lua_tonumber( _L, -1 );

	//VECTOR3 old_pos = getpos();

	VECTOR3 pos;
	pos.x = x;
	pos.y = y;
	pos.z = z;
	int ret = replace( worldid, pos );

	TRACE(2)( "[OBJ](player) c_replace_pos, ret = %d, x = %f, y = %f, z = %f, scene = %d, worldidx = %d", ret, x, y, z, (worldid & 0x0F), (worldid >> 8) );

    lua_pushnumber( _L, ret );
	return 1;
}

int Player::c_refresh_view( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    u_long world_id = (u_long)lua_tointeger( _L, -2 );
    bool reset_snapshot = lua_toboolean( _L, -1 ); 

    World* world = g_worldmng->get_world( world_id );
    if( !world )
    {
        ERR(2)( "[PLAYER](c_add_to_view) can not find world by world_id: %u", world_id );
        c_bt( _L );
        lua_pushboolean( _L, false );
        return 1;
    }

    if( reset_snapshot ) 
    {
        snapshot_.lock();
        snapshot_.reset();
        snapshot_.unlock();
    }
    
    if( world->add_to_view( this, true ) < 0 )
    {
        lua_pushboolean( _L, false );
    }
    else
    {
        lua_pushboolean( _L, true );
    }
    return 1;
}

