#include "item.h"
#include "define_func.h"

extern int g_frame;

static const float AOI_RADIUS = 3 * 3 ;

Item::Item()
{
	settype( OT_ITEM );
    set_radius( 0.5 );
}


Item::~Item()
{
}

void Item::process_serial()
{
    VECTOR3 pos = getpos();
	FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkPlayer )
	{
		OBJID obj_id = obj->get_id();
        VECTOR3 tar_pos = obj->getpos();
        float len = vc3_xzlen ( tar_pos - pos );
        if( len <= AOI_RADIUS )
        {
            player_enter( obj_id );
        }
	}
	END_LINKMAP	
}

void Item::player_enter( OBJID _player_ctrl_id )
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "do_player_enter_aoi" );
    lua_pushnumber( L, _player_ctrl_id );
    lua_pushnumber( L, get_id() );

    if( llua_call( L, 3, 0, 0 ) )      
		llua_fail( L, __FILE__, __LINE__ );
}

/************************************************************
 * for lua
 ***********************************************************/
 
const char Item::className[] = "Item";
const bool Item::gc_delete_ = false;

Lunar<Item>::RegType Item::methods[] =
{	
	LUNAR_OBJ_METHODS,
	LUNAR_CTRL_METHODS,
	LUNAR_ITEM_METHODS,
	{NULL, NULL}
};


int Item::c_test_func( lua_State* _state )
{
	lcheck_argc( _state, 0 );
	return 0;
}


