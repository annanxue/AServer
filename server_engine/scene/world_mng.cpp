#include "world_mng.h"
#include "log.h"
#include "obj.h"
#include "resource.h"

WorldMng::WorldMng()
{
	remove_world_cnt_	= 0;
	ctrl_index_			= 1; 

    world_map_.init( 16, 32, "world_map_:world_mng.cpp" );
    ctrl_map_.init(1024, (1<<16), "ctrl_map_:world_mng.cpp" );
}


WorldMng::~WorldMng()
{
	World* world = NULL;
    Node* node = world_map_.first();
    while( node ) {
        world = (World*)(node->val);
		SAFE_DELETE( world );
        node = world_map_.next( node );
    }
}


void WorldMng::add_world( World* _world )
{
	assert(_world);
	world_map_.set( _world->get_id(), (intptr_t)_world );
	TRACE(2)("[SENCE](world) Add world( id:0x%08X )", _world->get_id());
}


World* WorldMng::get_world( u_long _world_id ) 
{
	World* world = NULL;
	world_map_.find( _world_id, (intptr_t&)world );
	if( world && world->is_delete() ) return NULL;
	return world;
}


void WorldMng::remove_world( World* _world )
{
	remove_world_ary_[remove_world_cnt_++] = _world;
}

void WorldMng::remove()
{
	for( int i = 0; i < remove_world_cnt_; i++ ) {
		World* world = remove_world_ary_[i];
		if( !world ) {
			continue;
		}

		TRACE(2)("[SENCE](world) Remove world(id:%d)", world->get_id());
		world_map_.del( world->get_id() );
		SAFE_DELETE( world );
	}
	remove_world_cnt_ = 0;
}


void WorldMng::process()
{
    World* world = NULL;
    Node* node = world_map_.first();
    while( node ) {
        world = (World*)(node->val);
        if( world ) {
			world->process();
        }
        node = world_map_.next( node );
    }
    remove();
}


void WorldMng::add_ctrl( Ctrl* _ctrl )
{
	assert(_ctrl);

	_ctrl->set_id( ctrl_index_ );
    ctrl_map_.set( ctrl_index_, (intptr_t)_ctrl ); 
    ctrl_index_ = ctrl_index_ + 1;
}


void WorldMng::remove_ctrl( Ctrl* _ctrl )
{
	assert(_ctrl);
    ctrl_map_.del( _ctrl->get_id() );
}

Ctrl* WorldMng::get_ctrl( u_long _ctrlid )
{
	Ctrl* ret = NULL;
    if( ctrl_map_.find( _ctrlid, (intptr_t&)ret ) ) {
		return ( ret->is_ctrl() ? ret : NULL );
    } else {
        return NULL;
    }
}

Spirit* WorldMng::get_spirit( u_long _ctrlid ) 
{
	Ctrl* ret = NULL;

    if( ctrl_map_.find( _ctrlid, (intptr_t&)ret ) ) {
		return ( ret->is_spirit() ? (Spirit*)ret : NULL );
    } else {
        return NULL;
    }
}

Player* WorldMng::get_player( u_long _ctrlid )
{
	Ctrl* ret = NULL;
    if( ctrl_map_.find( _ctrlid, (intptr_t&)ret ) ) {
		return ( ret->is_player() ? (Player*)ret : NULL );
    } else {
        return NULL;
    }
}

const char WorldMng::className[] = "WorldMng";
const bool WorldMng::gc_delete_ = false;

Lunar<WorldMng>::RegType WorldMng::methods[] =
{
	LUNAR_WORLDMNG_METHODS,
	{NULL, NULL}
};

int WorldMng::c_in_world( lua_State* _L )
{
    lcheck_argc( _L, 3 );
    u_long world_id = (u_long)lua_tonumber( _L, 1 );
    float x = (float)lua_tonumber( _L, 2 );
    float z = (float)lua_tonumber( _L, 3 );
    World* pworld = NULL;
    pworld = this->get_world( world_id );
    if( pworld )
    {
        if( pworld->in_world( x, z ) )
        {
            lua_pushboolean( _L, true );
        }
        else
        {
            lua_pushboolean( _L, false );
        }
    }
    else
    {
        lua_pushboolean( _L, false );
    }
    return 1;
}

int WorldMng::c_add_world( lua_State *_L)
{
    lcheck_argc(_L, 1);
    int sceneid = (int)lua_tonumber(_L, 1);
    Scene *scene = g_resmng->get_scene(sceneid);

    u_long world_id = (u_long)-1;
    if (scene) {
        World* world = scene->add_world();
        if( world ) {
            world_id = world->get_id();
        }
    }

    lua_pushnumber( _L, world_id );
    return 1;
}

int WorldMng::c_remove_world( lua_State *_L)
{
    lcheck_argc(_L, 2);
    unsigned long sceneid  = (unsigned long)lua_tonumber(_L, 1);
    unsigned long worldidx = (unsigned long)lua_tonumber(_L, 2);
    unsigned long worldid = (worldidx << 16) + sceneid;

    World *world = g_worldmng->get_world(worldid);
    if (world) {
        Scene *scene = g_resmng->get_scene(sceneid);
        if (scene) {
            scene->remove_world(world);    
        }
    }
    return 0;
}

int WorldMng::c_is_valid_sceneid( lua_State *_L)
{
    lcheck_argc(_L, 1);
    int sceneid = (int)lua_tonumber(_L, 1);
    Scene *scene = g_resmng->get_scene(sceneid);
    if ( scene ) 
        lua_pushboolean( _L, true );
    else
        lua_pushboolean( _L, false );
    return 1;
}

int WorldMng::c_get_world_player_count( lua_State *_L )
{
	lcheck_argc( _L, 1 );
    int world_id =  (int)lua_tonumber( _L, 1 );
    World* world = get_world( world_id );
    int count = 0;
    if( world ){
        count = world->get_player_count();
    }
    lua_pushnumber( _L, count );
    return 1;
}



