#include "monster.h"
#include "player_mng.h"
#include "world_mng.h"

Monster::Monster()
{
	settype( OT_MONSTER );
	ai_ = NULL;
    npc_type_ = 0;
}

Monster::~Monster()
{
	SAFE_DELETE(ai_);
}

void Monster::process_serial()
{
	if (ai_)
	{
		ai_->execute();
	}
}

int	Monster::view_add( Ctrl* _obj )
{
    Ctrl::view_add( _obj );
    if( _obj != this )
    {
        on_view_add( _obj );
    }

    return TRUE;
}

int	Monster::view_remove( Ctrl* _obj )
{
    Ctrl::view_remove( _obj );
    if( _obj != this ) 
    {
        on_view_remove( _obj );
    }
    return TRUE;
}

int Monster::c_do_set_angle_y( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	float angle = lua_tonumber( _state, 1 );
	set_angle_y( angle );
	return 0;
}

int Monster::c_replace_pos( lua_State* _state )
{
	lcheck_argc( _state, 4 );
	u_long worldid = (u_long)lua_tointeger( _state, -4 );
	float x = lua_tonumber( _state, -3 );
	float y = lua_tonumber( _state, -2 );
	float z = lua_tonumber( _state, -1 );

	//VECTOR3 old_pos = getpos();

	VECTOR3 pos;
	pos.x = x;
	pos.y = y;
	pos.z = z;
	replace( worldid, pos );

	TRACE(2)( "[OBJ](monster) c_replace_pos, x = %f, y = %f, z = %f, scene = %d, worldidx = %d", x, y, z, (worldid & 0x0F), (worldid >> 8) );

	return 0;
}

int Monster::c_set_npc_type( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    npc_type_ = lua_tointeger( _state, 1 );
    if ( npc_type_ == 2 )
    {
	    settype( OT_ROBOT );
    }
	return 0;
}

const char Monster::className[] = "Monster";
const bool Monster::gc_delete_ = false;

Lunar<Monster>::RegType Monster::methods[] =
{
	LUNAR_OBJ_METHODS,
	LUNAR_CTRL_METHODS,
	LUNAR_SPIRIT_METHODS,
	LUNAR_MONSTER_METHODS,
	{NULL, NULL}
};

void Monster::on_view_add( Ctrl* _obj )
{
	if( is_delete() ) return; 

	if(ai_)
	{
		ai_->on_obj_enter(_obj);
	}
	else
	{
		// just call lua
		LuaServer* lua_svr = Ctrl::get_lua_server();
		lua_State* L = lua_svr->L();
		lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
		lua_pushstring( L, "on_obj_enter_view" );
		lua_pushnumber( L, this->get_id() );
		lua_pushnumber( L, _obj->get_id() );

		if( llua_call( L, 3, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
	}
}

void Monster::on_view_remove( Ctrl* _obj )
{
	// just call lua
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "on_obj_leave_view" );
	lua_pushnumber( L, this->get_id() );
	lua_pushnumber( L, _obj->get_id() );

	if( llua_call( L, 3, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}

