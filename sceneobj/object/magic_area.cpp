#include "magic_area.h"
#include "define_func.h"
#include "world_mng.h"
#include "timer.h"

extern int g_frame;

MagicArea::MagicArea()
    :do_delete_(false),
    frame_max_(0),
    frame_count_(0),
    drag_speed_(0),
    owner_id_(NULL_ID),
    target_id_(NULL_ID),
    frame_search_(false),
    last_drag_frame_(0)
{
	settype( OT_MAGIC_AREA );
}

void MagicArea::process_parallel()
{
    if( ++frame_count_ > frame_max_ )
    {
        do_delete_ = true;
        return;
    }

    if ( target_id_ != NULL_ID )
    {
        Ctrl* target = g_worldmng->get_ctrl( target_id_ ); 
        if ( target )
        {
            VECTOR3 pos = target->getpos();
            replace( target->get_worldid(), pos, target->get_plane() );
        }
    }
}

void MagicArea::process_serial()
{
    if( do_delete_ )
    {
        safe_delete();
        return;
    }

    /*
    if ( drag_speed_ > 0 && g_frame - last_drag_frame_ > g_timermng->get_frame_rate() / 2 )
    {
        do_drag();
        last_drag_frame_ = g_frame;
    }
    */

    if ( frame_search_ )
    {
        do_frame_search();
    }
}

void MagicArea::safe_delete()
{
    // call lua
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "do_magic_area_delete" );
    lua_pushnumber( L, this->get_id() );
    if( llua_call( L, 2, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );

    // real delete
    Ctrl::safe_delete();
}

void MagicArea::do_drag()
{
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "do_magic_area_drag" );
    lua_pushnumber( L, this->get_id() );
    if( llua_call( L, 2, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}

void MagicArea::do_frame_search()
{
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "do_magic_area_frame_search" );
    lua_pushnumber( L, this->get_id() );
    if( llua_call( L, 2, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}
/******************************************************************************
for lua
******************************************************************************/

const char MagicArea::className[] = "MagicArea";
const bool MagicArea::gc_delete_ = false;

Lunar<MagicArea>::RegType MagicArea::methods[] =
{	
	LUNAR_OBJ_METHODS,
	LUNAR_CTRL_METHODS,
	LUNAR_MAGIC_AREA_METHODS,
	{NULL, NULL}
};

int MagicArea::c_get_do_delete( lua_State* _state )
{
    lua_pushboolean( _state, do_delete_ );
	return 1;
}

int MagicArea::c_set_do_delete( lua_State* _state )
{
    lcheck_argc( _state, 1 );
	do_delete_ = lua_toboolean( _state, 1 );
	return 0;
}

int MagicArea::c_get_owner_id( lua_State* _state )
{
    lua_pushnumber( _state, owner_id_ );
    return 1;
}

int MagicArea::c_set_owner_id( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    owner_id_ = lua_tointeger( _state, 1 );
    return 0;
}

int MagicArea::c_get_target_id( lua_State* _state )
{
    lua_pushnumber( _state, target_id_ );
    return 1;
}

int MagicArea::c_set_target_id( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    target_id_ = lua_tointeger( _state, 1 );
    return 0;
}

int MagicArea::c_set_frame_max( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    frame_max_ = lua_tointeger( _state, 1 );
    return 0;
}

int MagicArea::c_get_drag_speed( lua_State* _state )
{
    lua_pushnumber( _state, drag_speed_ );
    return 1;
}

int MagicArea::c_set_drag_speed( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    drag_speed_ = lua_tonumber( _state, 1 );
    return 0;
}

int MagicArea::c_set_frame_search( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	frame_search_ = lua_toboolean( _state, 1 );
    return 0;
}

