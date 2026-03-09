#include "scene_obj_utils.h"

#include "ctrl.h"
#include "world_mng.h"
#include "define_func.h"
#include "state.h"
#include "3dmath.h"
#include "bullet.h"
#include "load_btree.h"
#include "msg.h"
#include "player.h"

bool g_trace_search = false;

static bool is_y_angle_obj( const VECTOR3* _pos, float _angle_y, float _degree, Ctrl* _obj )
{
    float dst_y_angle = d3d_to_degree( vc3_get_angle( _obj->getpos() - *_pos ) ) ;
    float diff_y_angle = dst_y_angle - _angle_y ; 
    float trans_diff_y_angle = diff_y_angle + _degree/2.0 ; 
    while(trans_diff_y_angle > 360) trans_diff_y_angle -= 360;
    while(trans_diff_y_angle < 0) trans_diff_y_angle += 360;
    return trans_diff_y_angle <= _degree;
}

static bool is_range_obj( const VECTOR3* _pos, float _range, Ctrl* _obj )
{
	VECTOR3 dist = *_pos - _obj->getpos();
	float len = _obj->get_radius() + _range ;
    if( fabsf( dist.x ) <= len && fabsf( dist.y ) <= len && fabsf( dist.z ) <= len ) 
    {
	    return  ( dist.x * dist.x + dist.z * dist.z + dist.y * dist.y < ( len * len  ) );
    }
    return false;
}

static void check_add_obj_degree( Ctrl* _src, Ctrl* _target, lua_State* _state, float _degree, float _range, int& _idx )
{
	VECTOR3 pos = _src->getpos();
    VECTOR3 dist = _target->getpos() - pos;
    float length_sq = dist.x * dist.x + dist.z * dist.z;
    float radius_sq = _src->get_radius() * _src->get_radius();
    if( ( _src->is_search_body() && (length_sq < radius_sq) ) 
            || ( _src->is_y_angle_obj(_target, _degree) && _src->is_range_obj( _target, _range ) ) )
    {
        Ctrl* ctrl = (Ctrl*)_target;
        lua_pushnumber( _state, ++_idx );
        lua_pushnumber( _state, ctrl->get_id() );
        lua_settable( _state, -3 );
    }
}

static void check_add_obj_pos_degree( const VECTOR3* _pos, float _angle_y, float _degree, float _range, Ctrl* _target, lua_State* _state, int& _idx )
{
    if( is_y_angle_obj( _pos, _angle_y, _degree, _target ) && is_range_obj( _pos, _range, _target ) )
    {
        lua_pushnumber( _state, ++_idx );
        lua_pushnumber( _state, _target->get_id() );
        lua_settable( _state, -3 );
    }
}

static void check_add_obj_rect( Ctrl* _src, Ctrl* _target, lua_State* _state, float _width, float _length, int& _idx )
{
	VECTOR3 start_pos = _src->getpos();
    VECTOR3 dist = _target->getpos() - start_pos;
    float length_sq = dist.x * dist.x + dist.z * dist.z;
    float radius_sq = _src->get_radius() * _src->get_radius();
    VECTOR3 end_pos = start_pos + GET_DIR(_src->get_angle_y()) * _length;
    float extra_radius = 0;
    bool is_front_of_start = _src->is_y_angle_obj( _target, 180);
    _src->setpos(end_pos);
    bool is_back_of_end = !_src->is_y_angle_obj( _target, 180); 
    _src->setpos(start_pos);

    if (is_front_of_start && is_back_of_end)
        extra_radius = _width / 2;

    if( ( _src->is_search_body() && (length_sq < radius_sq) )
            || Bullet::get_space().ray_test( start_pos, end_pos, extra_radius, _target) )
    {
        Ctrl* ctrl = (Ctrl*)_target;
        lua_pushnumber( _state, ++_idx );
        lua_pushnumber( _state, ctrl->get_id() );
        lua_settable( _state, -3 );
    }
}

static void check_add_obj_offset_rect( VECTOR3& _center_pos, float _angle_y, Ctrl* _target, lua_State* _state, float _width, float _length, int& _idx )
{
    VECTOR3 end_pos = _center_pos + GET_DIR(_angle_y) * _length;
    float extra_radius = 0;
    bool is_front_of_start = is_y_angle_obj( &_center_pos, _angle_y, 180, _target );
    bool is_back_of_end = !is_y_angle_obj( &end_pos, _angle_y, 180, _target );

    if (is_front_of_start && is_back_of_end)
        extra_radius = _width / 2;

    if( Bullet::get_space().ray_test( _center_pos, end_pos, extra_radius, _target) )
    {
        Ctrl* ctrl = (Ctrl*)_target;
        lua_pushnumber( _state, ++_idx );
        lua_pushnumber( _state, ctrl->get_id() );
        lua_settable( _state, -3 );
    }
}

static void check_add_obj_by_pos( VECTOR3& _pos, Ctrl* _target, lua_State* _state, float _range, int& _idx )
{
    VECTOR3 dist = _target->getpos() - _pos;
    float len = _target->get_radius() + _range;

    if ( fabsf( dist.x ) <= len && fabsf( dist.y ) <= len && fabsf( dist.z ) <= len ) {
        if ( dist.x * dist.x + dist.z * dist.z + dist.y * dist.y < ( len * len  ) ) {
            Ctrl* ctrl = (Ctrl*)_target;
            lua_pushnumber( _state, ++_idx );
            lua_pushnumber( _state, ctrl->get_id() );
            lua_settable( _state, -3 );
        }
    }
}

static void check_add_obj_offset_degree( VECTOR3& _center_pos, float _angle_y, Ctrl* _target, lua_State* _state, float _degree, float _range, int& _idx )
{
    VECTOR3 dist = _target->getpos() - _center_pos;
    float dst_y_angle = d3d_to_degree( vc3_get_angle( dist ) );
    float diff_y_angle = dst_y_angle - _angle_y; 
    float trans_diff_y_angle = diff_y_angle + _degree / 2.0; 
   
    while( trans_diff_y_angle > 360 )
    {
        trans_diff_y_angle -= 360;
    }
    while( trans_diff_y_angle < 0 )
    {
        trans_diff_y_angle += 360;
    }
    if( trans_diff_y_angle > _degree )
    {
        return;
    }
	float len = _target->get_radius() + _range ;
    if( fabsf( dist.x ) <= len && fabsf( dist.y ) <= len && fabsf( dist.z ) <= len ) 
    {
	    if( dist.x * dist.x + dist.z * dist.z + dist.y * dist.y < ( len * len  ) )
        {
            Ctrl* ctrl = (Ctrl*)_target;
            lua_pushnumber( _state, ++_idx );
            lua_pushnumber( _state, ctrl->get_id() );
            lua_settable( _state, -3 );
        }
    }
}

// 简化版的搜索函???按位???// type = 0x0001 player
// type = 0x0010 monster
// type = 0x0100 trigger
int c_get_obj_arroud_with_degree( lua_State* _state )
{
	OBJID srcid = (OBJID)lua_tonumber( _state, 1 );
	int type = (int)lua_tonumber( _state, 2 );
	float range = (float)lua_tonumber( _state, 3 );
	float degree = (float)lua_tonumber( _state, 4 );

	lua_newtable( _state );
	int idx = 0;

	Ctrl* src = g_worldmng->get_ctrl( srcid );
    if( is_invalid_obj(src) ) return 1;
    World* world = src->get_world();
    if( world == NULL ) return 1;
    VECTOR3 pos = src->getpos();

    if ( g_trace_search )
    {
        VECTOR3 pos = src->getpos();
        Ar ar;
        SET_SNAPSHOT_TYPE( ar, ST_TRACE_SEARCH );
        ar << 1;    // degree search
        ar << srcid;
        ar << pos.x;
        ar << pos.z;
        ar << src->get_angle_y();
        ar << range;
        ar << degree;
        BROADCAST( ar, src, NULL );
    }

    if( type & 1 ) {
        FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkPlayer )
            check_add_obj_degree( src, obj, _state, degree, range, idx );
        END_LINKMAP	
        if ( !(type & 2) )
        {
            FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
                if (obj->is_robot()) {
                    check_add_obj_degree( src, obj, _state, degree, range, idx );
                }
            END_LINKMAP
        }
    }

    bool robot_include = type & 1;
    if( type & 2 && type & 4 ) {
        FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
			if (obj->is_monster() || obj->is_attackable_trigger()) {
                if (robot_include || !obj->is_robot()) {
    				check_add_obj_degree( src, obj, _state, degree, range, idx );
                }
			}
        END_LINKMAP	
    }
    else if( type & 2 )
    {
        FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
            if (obj->is_monster()) {
                if (robot_include || !obj->is_robot()) {
                    check_add_obj_degree( src, obj, _state, degree, range, idx );
                }
            }
        END_LINKMAP	
    }
    else if( type & 4 )
    {
        FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
            if (obj->is_attackable_trigger()) {
                check_add_obj_degree( src, obj, _state, degree, range, idx );
            }
        END_LINKMAP	
    }

	return 1;
}

int c_get_obj_around_with_offset_degree( lua_State* _state )
{
	OBJID srcid = (OBJID)lua_tonumber( _state, 1 );
	int type = (int)lua_tonumber( _state, 2 );
	float range = (float)lua_tonumber( _state, 3 );
	float degree = (float)lua_tonumber( _state, 4 );
	float center_x = (float)lua_tonumber( _state, 5 );
	float center_z = (float)lua_tonumber( _state, 6 );
	float angle_y = (float)lua_tonumber( _state, 7 );

	lua_newtable( _state );
	int idx = 0;

	Ctrl* src = g_worldmng->get_ctrl( srcid );
    if( is_invalid_obj(src) ) return 1;
    World* world = src->get_world();
    if( world == NULL ) return 1;
    VECTOR3 center_pos( center_x, 0, center_z );

    if ( g_trace_search )
    {
        Ar ar;
        SET_SNAPSHOT_TYPE( ar, ST_TRACE_SEARCH );
        ar << 1;    // degree search
        ar << srcid;
        ar << center_x;
        ar << center_z;
        ar << angle_y;
        ar << range;
        ar << degree;
        BROADCAST( ar, src, NULL );
    }

    if( type & 1 ) {
        FOR_LINKMAP( world, src->plane_, center_pos, range, Obj::LinkPlayer )
            check_add_obj_offset_degree( center_pos, angle_y, obj, _state, degree, range, idx );
        END_LINKMAP	
        if ( !(type & 2) )
        {
            FOR_LINKMAP( world, src->plane_, center_pos, range, Obj::LinkDynamic )
                if (obj->is_robot()) {
                    check_add_obj_offset_degree( center_pos, angle_y, obj, _state, degree, range, idx );
                }
            END_LINKMAP
        }
    }

    bool robot_include = type & 1;
    if( type & 2 && type & 4 ) {
        FOR_LINKMAP( world, src->plane_, center_pos, range, Obj::LinkDynamic )
			if (obj->is_monster() || obj->is_attackable_trigger()) {
                if (robot_include || !obj->is_robot()) {
                    check_add_obj_offset_degree( center_pos, angle_y, obj, _state, degree, range, idx );
                }
			}
        END_LINKMAP	
    }
    else if( type & 2 )
    {
        FOR_LINKMAP( world, src->plane_, center_pos, range, Obj::LinkDynamic )
            if (obj->is_monster()) {
                if (robot_include || !obj->is_robot()) {
                    check_add_obj_offset_degree( center_pos, angle_y, obj, _state, degree, range, idx );
                }
            }
        END_LINKMAP	
    }
    else if( type & 4 )
    {
        FOR_LINKMAP( world, src->plane_, center_pos, range, Obj::LinkDynamic )
            if (obj->is_attackable_trigger()) {
                check_add_obj_offset_degree( center_pos, angle_y, obj, _state, degree, range, idx );
            }
        END_LINKMAP	
    }

	return 1;
}

// c_get_obj_arround_pos_with_degree( srcid, apply_type, x, z, angle_y, range, degree )
int c_get_obj_arround_pos_with_degree( lua_State* _state )
{
	OBJID srcid   = (OBJID)lua_tonumber( _state, 1 );
	int type      = (int)lua_tonumber( _state, 2 );
	float x       = (float)lua_tonumber( _state, 3 );
	float z       = (float)lua_tonumber( _state, 4 );
	float angle_y = (float)lua_tonumber( _state, 5 );
	float range   = (float)lua_tonumber( _state, 6 );
	float degree  = (float)lua_tonumber( _state, 7 );

	lua_newtable( _state );
	int idx = 0;

	Ctrl* src = g_worldmng->get_ctrl( srcid );
    if( is_invalid_obj( src ) )
    {
        return 1;
    }

    World* world = src->get_world();
    if( world == NULL )
    {
        return 1;
    }

    VECTOR3 pos = VECTOR3( x, 0, z );
    if ( g_trace_search )
    {
        Ar ar;
        SET_SNAPSHOT_TYPE( ar, ST_TRACE_SEARCH );
        ar << 1;    // degree search
        ar << srcid;
        ar << pos.x;
        ar << pos.z;
        ar << angle_y;
        ar << range;
        ar << degree;
        BROADCAST( ar, src, NULL );
    }

    if( type & 1 )
    {
        FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkPlayer )
            check_add_obj_pos_degree( &pos, angle_y, degree, range, obj, _state, idx );
        END_LINKMAP	
        if ( !(type & 2) )
        {
            FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
                if (obj->is_robot())
                {
                    check_add_obj_pos_degree( &pos, angle_y, degree, range, obj, _state, idx );
                }
            END_LINKMAP
        }
    }

    bool robot_include = type & 1;
    if( type & 2 && type & 4 )
    {
        FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
			if (obj->is_monster() || obj->is_attackable_trigger())
            {
                if (robot_include || !obj->is_robot())
                {
                    check_add_obj_pos_degree( &pos, angle_y, degree, range, obj, _state, idx );
                }
			}
        END_LINKMAP	
    }
    else if( type & 2 )
    {
        FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
            if (obj->is_monster())
            {
                if (robot_include || !obj->is_robot())
                {
                    check_add_obj_pos_degree( &pos, angle_y, degree, range, obj, _state, idx );
                }
            }
        END_LINKMAP	
    }
    else if( type & 4 )
    {
        FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
            if (obj->is_attackable_trigger())
            {
                check_add_obj_pos_degree( &pos, angle_y, degree, range, obj, _state, idx );
            }
        END_LINKMAP	
    }

	return 1;
}


// 简化版的搜索函???按位???// type = 0x0001 player
// type = 0x0010 monster
// type = 0x0100 trigger
int c_get_obj_arroud_with_rect( lua_State* _state )
{
	OBJID srcid = (OBJID)lua_tonumber( _state, 1 );
	int type = (int)lua_tonumber( _state, 2 );
	float width = (float)lua_tonumber( _state, 3 );
	float length = (float)lua_tonumber( _state, 4 );

	lua_newtable( _state );
	int idx = 0;

	Ctrl* src = g_worldmng->get_ctrl( srcid );
	if( is_invalid_obj(src) ) return 1;
	World* world = src->get_world();
	if( world == NULL ) return 1;
	VECTOR3 pos = src->getpos();

    if ( g_trace_search )
    {
        VECTOR3 pos = src->getpos();
        Ar ar;
        SET_SNAPSHOT_TYPE( ar, ST_TRACE_SEARCH );
        ar << 2;    // rect search
        ar << srcid;
        ar << pos.x;
        ar << pos.z;
        ar << src->get_angle_y();
        ar << width;
        ar << length;
        BROADCAST( ar, src, NULL );
    }

    float range = width;
    if( length > range ) 
    {
        range = length;
    }
    
	if( type & 1 ) {
		FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkPlayer )
            check_add_obj_rect( src, obj, _state, width, length, idx );
		END_LINKMAP	
        if ( !(type & 2) )
        {
            FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
                if (obj->is_robot()) {
                    check_add_obj_rect( src, obj, _state, width, length, idx );
                }
            END_LINKMAP
        }
	}

    bool robot_include = type & 1;
    if( type & 2 && type & 4 ) {
		FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
			if (obj->is_monster() || obj->is_attackable_trigger()) {
                if (robot_include || !obj->is_robot()) {
                    check_add_obj_rect( src, obj, _state, width, length, idx );
                }
			}
        END_LINKMAP	
    }
    else if( type & 2 )
    {
		FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
            if (obj->is_monster()) {
                if (robot_include || !obj->is_robot()) {
                    check_add_obj_rect( src, obj, _state, width, length, idx );
                }
            }
        END_LINKMAP	
    }
    else if( type & 4 )
    {
		FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
            if (obj->is_attackable_trigger()) {
                check_add_obj_rect( src, obj, _state, width, length, idx );
            }
        END_LINKMAP	
    }

    return 1;
}

int c_get_obj_around_with_offset_rect( lua_State* _state )
{
	OBJID srcid = (OBJID)lua_tonumber( _state, 1 );
	int type = (int)lua_tonumber( _state, 2 );
	float width = (float)lua_tonumber( _state, 3 );
	float length = (float)lua_tonumber( _state, 4 );
	float center_x = (float)lua_tonumber( _state, 5 );
	float center_z = (float)lua_tonumber( _state, 6 );
	float angle_y = (float)lua_tonumber( _state, 7 );

	lua_newtable( _state );
	int idx = 0;

	Ctrl* src = g_worldmng->get_ctrl( srcid );
	if( is_invalid_obj(src) ) return 1;
	World* world = src->get_world();
	if( world == NULL ) return 1;
    VECTOR3 pos( center_x, 0, center_z );

    if ( g_trace_search )
    {
        Ar ar;
        SET_SNAPSHOT_TYPE( ar, ST_TRACE_SEARCH );
        ar << 2;    // rect search
        ar << srcid;
        ar << center_x;
        ar << center_z;
        ar << angle_y;
        ar << width;
        ar << length;
        BROADCAST( ar, src, NULL );
    }

    float range = width;
    if( length > range ) 
    {
        range = length;
    }
    
	if( type & 1 ) {
		FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkPlayer )
            check_add_obj_offset_rect( pos, angle_y, obj, _state, width, length, idx );
		END_LINKMAP	
        if ( !(type & 2) )
        {
            FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
                if (obj->is_robot()) {
                    check_add_obj_offset_rect( pos, angle_y, obj, _state, width, length, idx );
                }
            END_LINKMAP
        }
	}

    bool robot_include = type & 1;
    if( type & 2 && type & 4 ) {
		FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
			if (obj->is_monster() || obj->is_attackable_trigger()) {
                if (robot_include || !obj->is_robot()) {
                    check_add_obj_offset_rect( pos, angle_y, obj, _state, width, length, idx );
                }
			}
        END_LINKMAP	
    }
    else if( type & 2 )
    {
		FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
            if (obj->is_monster()) {
                if (robot_include || !obj->is_robot()) {
                    check_add_obj_offset_rect( pos, angle_y, obj, _state, width, length, idx );
                }
            }
        END_LINKMAP	
    }
    else if( type & 4 )
    {
		FOR_LINKMAP( world, src->plane_, pos, range, Obj::LinkDynamic )
            if (obj->is_attackable_trigger()) {
                check_add_obj_offset_rect( pos, angle_y, obj, _state, width, length, idx );
            }
        END_LINKMAP	
    }

    return 1;
}

int c_get_obj_arroud_by_pos( lua_State* _state )
{
    VECTOR3 pos;
	pos.x = (float)lua_tonumber( _state, 1 );
    pos.y = 0.0f;
	pos.z = (float)lua_tonumber( _state, 2 );
	int type = (int)lua_tonumber( _state, 3 );
	float range = (float)lua_tonumber( _state, 4 );
	int world_id = (int)lua_tonumber( _state, 5 );
	int plane_id = (int)lua_tonumber( _state, 6 );
	OBJID srcid = (OBJID)lua_tonumber( _state, 7 );

	lua_newtable( _state );
	int idx = 0;

	Ctrl* src = g_worldmng->get_ctrl( srcid );
	if( is_invalid_obj(src) ) return 1;
    World* world = g_worldmng->get_world(world_id);
    if( world == NULL ) return 1;

    if ( g_trace_search )
    {
        Ar ar;
        SET_SNAPSHOT_TYPE( ar, ST_TRACE_SEARCH );
        ar << 3;            // pos search
        ar << srcid;
        ar << pos.x;
        ar << pos.z;
        ar << (float)0;     // angle
        ar << range;
        ar << (float)360;   // search degree
        BROADCAST( ar, src, NULL );
    }

    if( type & 1 ) {
        FOR_LINKMAP( world, plane_id, pos, range, Obj::LinkPlayer )
            check_add_obj_by_pos( pos, obj, _state, range, idx );
        END_LINKMAP	
        if ( !(type & 2) )
        {
            FOR_LINKMAP( world, plane_id, pos, range, Obj::LinkDynamic )
                if (obj->is_robot()) {
                    check_add_obj_by_pos( pos, obj, _state, range, idx );
                }
            END_LINKMAP
        }
    }

    bool robot_include = type & 1;
    if( type & 2 && type & 4 ) {
        FOR_LINKMAP( world, plane_id, pos, range, Obj::LinkDynamic )
			if (obj->is_monster() || obj->is_attackable_trigger()) {
                if (robot_include || !obj->is_robot()) {
				    check_add_obj_by_pos( pos, obj, _state, range, idx );
                }
			}
        END_LINKMAP	
    }
    else if( type & 2 )
    {
        FOR_LINKMAP( world, plane_id, pos, range, Obj::LinkDynamic )
            if (obj->is_monster()) {
                if (robot_include || !obj->is_robot()) {
                    check_add_obj_by_pos( pos, obj, _state, range, idx );
                }
            }
        END_LINKMAP	
    }
    else if( type & 4 )
    {
        FOR_LINKMAP( world, plane_id, pos, range, Obj::LinkDynamic )
            if (obj->is_attackable_trigger()) {
                check_add_obj_by_pos( pos, obj, _state, range, idx );
            }
        END_LINKMAP	
    }

	return 1;
}

int c_set_state_msg_retry_max( lua_State* _state ) 
{
    lcheck_argc( _state, 1 ); 
    int max_retry = (int)lua_tonumber( _state, 1 ); 
    State::set_msg_retry_max( max_retry );
    return 1;
}

int c_set_world_door_flags( lua_State *_state ) 
{
	u_long world_id = (u_long) lua_tonumber(_state, 1);
	unsigned int plane_id = (unsigned int) lua_tonumber(_state, 2);
	unsigned short door_flags = (unsigned short) lua_tonumber(_state, 3);

	World* wld = g_worldmng->get_world(world_id);
	if (!wld)
	{
		LOG(2) ("No this world! id: %d", world_id);
	}
	else
	{
		// LOG(2) ("set world door flags, world:%d, plane:%d, flags:%d", world_id, plane_id, door_flags);
		wld->set_door_flags(door_flags, plane_id);
	}
	return 0;
}

int c_get_world_door_flags( lua_State *_state ) 
{
	lcheck_argc( _state, 2 );
	u_long world_id = (u_long) lua_tonumber(_state, 1);
	unsigned int plane_id = (unsigned int) lua_tonumber(_state, 2);
	//unsigned short door_flags = (unsigned short) lua_tonumber(_state, 3);

	World* wld = g_worldmng->get_world(world_id);
	if (wld)
	{
		lua_pushnumber(_state, wld->get_door_flags(plane_id) );
		return 1;
	}
	LOG(2) ("No this world! id: %d", world_id);
	return 0;
}

int c_get_obj_in_aoi( lua_State* _state )//123
{
	VECTOR3 pos;
	pos.x = (float)lua_tonumber( _state, 1 );
	pos.y = 0.0f;
	pos.z = (float)lua_tonumber( _state, 2 );
	int type = (int)lua_tonumber( _state, 3 );
	int world_id = (int)lua_tonumber( _state, 4 );
	int plane_id = (int)lua_tonumber( _state, 5 );

	lua_newtable( _state );
	int idx = 0;

	World* world = g_worldmng->get_world(world_id);
	if( world == NULL ) return 1;

	if( type & 1 ) 
	{
		FOR_LINKMAP_VIEW( world, plane_id, pos, Obj::LinkPlayer, VIEW_RANGE_FULL )
			lua_pushnumber( _state, ++idx );
			lua_pushnumber( _state, obj->get_id() );
			lua_settable( _state, -3 );
		END_LINKMAP_VIEW
        if ( !(type & 2) )
        {
		    FOR_LINKMAP_VIEW( world, plane_id, pos, Obj::LinkDynamic, VIEW_RANGE_FULL )
                if (obj->is_robot()) {
                    lua_pushnumber( _state, ++idx );
                    lua_pushnumber( _state, obj->get_id() );
                    lua_settable( _state, -3 );
                }
		    END_LINKMAP_VIEW
        }
	}

    bool robot_include = type & 1;
	if(type & 2 || type & 4)
	{
		FOR_LINKMAP_VIEW( world, plane_id, pos, Obj::LinkDynamic, VIEW_RANGE_FULL )
			if ((type & 2 && obj->is_monster()) || (type & 4 && obj->is_trigger())) 
			{
                if (robot_include || !obj->is_robot()) {
                    lua_pushnumber( _state, ++idx );
                    lua_pushnumber( _state, obj->get_id() );
                    lua_settable( _state, -3 );
                }
			}
		END_LINKMAP_VIEW	
	}

	return 1;
}

int c_band( lua_State* _state )
{
	int lvalue = (int)lua_tonumber( _state, 1 );
	int rvalue = (int)lua_tonumber( _state, 2 );
	lua_pushnumber(_state, lvalue & rvalue );
    return 1;
}

int c_bor( lua_State* _state )
{
	int lvalue = (int)lua_tonumber( _state, 1 );
	int rvalue = (int)lua_tonumber( _state, 2 );
	lua_pushnumber(_state, lvalue | rvalue );
    return 1;
}

int c_set_bit( lua_State* _state )
{
    lcheck_argc( _state, 2 );
    unsigned int value = (unsigned int)lua_tonumber( _state, 1 );
    unsigned int bit_index = (unsigned int)lua_tonumber( _state, 2 );
    if ( bit_index > 31 ) { //
        ERR(2)("bit index is larger than 32, index:%d", bit_index );
        return 0;
    }
    set_bit(bit_index, &value);
    lua_pushnumber( _state, value );
    return 1;
}

int c_clear_bit( lua_State* _state )
{
    lcheck_argc( _state, 2 );
    unsigned int value = (unsigned int)lua_tonumber( _state, 1 );
    unsigned int bit_index = (unsigned int)lua_tonumber( _state, 2 );
    if ( bit_index > 31 ) { //
        ERR(2)("bit index is larger than 32, index:%d", bit_index );
        return 0;
    }
    clear_bit(bit_index, &value);
    lua_pushnumber( _state, value );
    return 1;
}

int c_test_bit( lua_State* _state )
{
    lcheck_argc( _state, 2 );
    unsigned int value = (unsigned int)lua_tonumber( _state, 1 );
    unsigned int bit_index = (unsigned int)lua_tonumber( _state, 2 );
    if ( bit_index > 31 ) { //
        ERR(2)("bit index is larger than 32, index:%d", bit_index );
        return 0;
    }
    lua_pushboolean( _state, test_bit(bit_index, &value) );
    return 1;
}

int c_open_packet_stats( lua_State* _state )
{
    bool open = lua_toboolean( _state, 1 );
    int snapshot_type = luaL_optint( _state, 2, 0x5007 );
    Ctrl::get_netmng()->set_packet_stats_open( open, snapshot_type );
    return 0;
}

int c_print_packet_stats( lua_State* _state )
{
    lcheck_argc( _state, 0 ); 
    Ctrl::get_netmng()->print_packet_stats();
    return 0;
}


int c_set_trace_search( lua_State* _state )
{
    lcheck_argc( _state, 1 );
    g_trace_search = lua_toboolean( _state, 1 );
    return 0;
}

int c_reload_ai( lua_State* _state )
{
    SGame::clear_doc_cache();
    return 0;
}

int c_findpath( lua_State *_state )
{
    unsigned long world_id = (int)lua_tonumber( _state, 1 );
	int plane_id = (int)lua_tonumber( _state, 2 );
	float sx = (int)lua_tonumber( _state, 3 );
	float sz = (int)lua_tonumber( _state, 4 );
	float ex = (int)lua_tonumber( _state, 5 );
	float ez = (int)lua_tonumber( _state, 6 );

    bool ret = false;
	Navmesh::Path path;
    World* world = g_worldmng->get_world( world_id );
    if( world )
    {
        VECTOR3 start_pos( sx, 0, sz );
        VECTOR3 end_pos( ex, 0, ez );
        unsigned short door_flags = world->get_door_flags( plane_id );
	    ret = world->get_scene()->findpath( &start_pos, &end_pos, &path, door_flags );
    }

	lua_pushboolean( _state, ret );
	lua_newtable( _state );
	for( unsigned int i = 0; i < path.size(); ++i )
	{
		lua_newtable( _state );
		lua_pushnumber( _state, path[i].x );
		lua_rawseti( _state, -2, 1) ;
		lua_pushnumber( _state, path[i].z );
		lua_rawseti( _state, -2, 2 );
		lua_rawseti( _state, -2, i + 1 );
	}
	lua_pushinteger( _state, path.size() );
	return 3;
}

int c_raycast( lua_State *_state )
{
    unsigned long world_id = (int)lua_tonumber( _state, 1 );
	int plane_id = (int)lua_tonumber( _state, 2 );
	float sx = (float)lua_tonumber( _state, 3 );
	float sz = (float)lua_tonumber( _state, 4 );
	float ex = (float)lua_tonumber( _state, 5 );
	float ez = (float)lua_tonumber( _state, 6 );

    VECTOR3 start_pos( sx, 0, sz );
    VECTOR3 end_pos( ex, 0, ez );
    VECTOR3 hit_pos( sz, 0, sz );
    float hit_rate = 0.0f;

    bool ret = false;
    World* world = g_worldmng->get_world( world_id );
    if( world )
    {
        VECTOR3 start_pos( sx, 0, sz );
        VECTOR3 end_pos( ex, 0, ez );
        unsigned short door_flags = world->get_door_flags( plane_id );
        ret = world->get_scene()->raycast( &start_pos, &end_pos, &hit_pos, &hit_rate, door_flags );
    }

    lua_pushboolean( _state, ret );
    lua_pushnumber( _state, hit_pos.x );
    lua_pushnumber( _state, hit_pos.z );
    lua_pushnumber( _state, hit_rate );
    return 4;
}
