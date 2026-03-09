#include "trigger.h"
#include "define_func.h"
#include "player_mng.h"
#include "world_mng.h"
#include "log.h"
#include "3dmath.h"
#include "bullet.h"

static const int CHECK_FRAME_RATE = 1;
static const int AOI_CIRCLE_SHAPE = 0;
static const int AOI_POLY_SHAPE = 1;

/*
 *
 * http://alienryderflex.com/polygon/
 * https://en.wikipedia.org/wiki/Point_in_polygon
 *
*/
inline bool is_point_in_polygon( const VECTOR3& p, const VECTOR3* _polygon, int _point_num )
{
    int cross_num = 0;
    for (int i = 0; i < _point_num; ++i) 
    {
        const VECTOR3& p1 = _polygon[i];
        const VECTOR3& p2 = _polygon[(i+1) % _point_num];

        if ( p1.z == p2.z )
            continue;

        if ( p.z < min(p1.z, p2.z) )
            continue;

        if ( p.z >= max(p1.z, p2.z) )
            continue;

        float x = (p.z - p1.z) * (p2.x - p1.x) / (p2.z - p1.z) + p1.x;

        if ( x > p.x )
        {
            cross_num++;
        }
    }
    return (cross_num % 2 == 1);
}

static void get_rectangle_corners( Ctrl* _owner, float _radius, float _lw_percent, VECTOR3* _corners  )
{
    if( _owner == NULL || _corners == NULL )
    {
        return;
    }
    VECTOR3 dist = GET_DIR(_owner->get_angle_y()) * _radius;
    VECTOR3 mid_a = _owner->getpos() + dist;
    VECTOR3 mid_b = _owner->getpos() - dist;

    float width = _lw_percent * _radius;
    VECTOR3 dist_2 = GET_DIR(_owner->get_angle_y() + 90 ) * width;

    _corners[0] = mid_a + dist_2;
    _corners[1] = mid_a - dist_2;
    _corners[2] = mid_b - dist_2;
    _corners[3] = mid_b + dist_2;
}


AoiTrigger::AoiTrigger( OBJID _owner_id, int _shape_type, float _rad, float _lw_per ) 
    : shape_type_(_shape_type),
    radius_(_rad), 
    lw_percenet_(_lw_per),
    owner_id_(_owner_id), 
    is_check_dynamic_(false),
    aoi_shape_(NULL)
{
}

AoiTrigger::~AoiTrigger()
{
    if( aoi_shape_ != NULL )
    {
        cpShapeFree( aoi_shape_ );
        aoi_shape_ = NULL;
    }
}

void AoiTrigger::create_aoi_shape( )
{
    if( aoi_shape_ != NULL )
    {
        return;
    }
    switch( shape_type_ )
    {
        case AOI_CIRCLE_SHAPE:
            create_circle_shape_aoi();
            break;
        case AOI_POLY_SHAPE:
            create_rectangle_shape_aoi();
            break;
        default:
            LOG(2)( "[trigger](create_aoi_shape) not support aoi type %d", shape_type_ );
    }
}

void AoiTrigger::create_circle_shape_aoi()
{
	Ctrl* owner = g_worldmng->get_ctrl( owner_id_ );
	if (owner == NULL) 
    {
        return;
    }

    VECTOR3 pos = owner->getpos();
    aoi_shape_ = cpCircleShapeNew( NULL, 1, cpvzero );
    ((cpCircleShape*)aoi_shape_)->r = radius_;
    ((cpCircleShape*)aoi_shape_)->tc = cpv(pos.x, pos.z);
}
    
/*
 *     p0 +--------- mid_a ----------+ p1
 *                    /\
 *                    ||
 *
 *     p3 +--------- mid_b ----------+ p2
 */

void AoiTrigger::create_rectangle_shape_aoi()
{
    Ctrl* owner = g_worldmng->get_ctrl( owner_id_ );
    if (owner == NULL) 
    {
        return;
    }

    get_rectangle_corners( owner, radius_, lw_percenet_, corners_ );

    cpVect verts[RECTANGLE_CORNER_NUM];
    for( int i = 0; i < RECTANGLE_CORNER_NUM; ++i )
    {
        verts[i] = cpv( corners_[i].x, corners_[i].z );
    }

    aoi_shape_ = cpPolyShapeNew( NULL, RECTANGLE_CORNER_NUM, verts, cpvzero );
}

bool AoiTrigger::is_in_aoi( Ctrl* _target )
{
    switch( shape_type_ )
    {
        case AOI_CIRCLE_SHAPE:
            return is_in_circle_aoi( _target );
        case AOI_POLY_SHAPE: 
            return is_point_in_polygon( _target->getpos(), corners_, RECTANGLE_CORNER_NUM );   
        default:
            LOG(2)( "[trigger](create_aoi_shape) not support aoi type %d", shape_type_ );
    }
    return false;
}

bool AoiTrigger::is_in_circle_aoi( Ctrl* _target )
{
    VECTOR3 pos = _target->getpos();
    cpNearestPointQueryInfo info;
    cpShapeNearestPointQuery (aoi_shape_, cpv(pos.x, pos.z), &info);
    return info.d <= 0.0;
}

bool AoiTrigger::ray_test( Ctrl* _target )
{
	Ctrl* owner = g_worldmng->get_ctrl( owner_id_ );
	if (owner == NULL) 
    {
		return false;
    }

	if (owner->get_world()->get_id() !=  _target->get_world()->get_id())
    {
		return false;
    }

    if( aoi_shape_ == NULL )
    {
        create_aoi_shape();
    }

    if( aoi_shape_ != NULL )
    {
        return is_in_aoi( _target );
    }

    return false;
}

bool AoiTrigger::ray_test( OBJID _obj_id )
{
	Ctrl* target = g_worldmng->get_ctrl(_obj_id);
	if (target)
    {
		return ray_test(target);
    }
	return false;
}

Trigger::Trigger()
{
	settype( OT_TRIGGER );
	aoi_ = NULL;
	is_active_ = true;
	cur_frame_count = 0;
	is_attackable_ = false;

	obj_in_aoi_map_.init(64, 128, "trigger.cpp");
	obj_enter_ary_.clear();
	obj_leave_ary_.clear();
}

Trigger::~Trigger()
{
	SAFE_DELETE(aoi_);
}

void Trigger::process_parallel()
{
    Spirit::process_parallel();

	if ( (NULL == aoi_) || !is_active_ || (++cur_frame_count < CHECK_FRAME_RATE)) 
    {
		return;
    }

	cur_frame_count = 0;
	obj_enter_ary_.clear();
	obj_leave_ary_.clear();

    // 1: check last in 
    if (!obj_in_aoi_map_.isempty()) 
    {
        Node* pNode = obj_in_aoi_map_.first();
        while(pNode) 
        {
            OBJID obj_id = (OBJID)pNode->key;
            if (!aoi_->ray_test(obj_id)) 
            {
                obj_leave_ary_.push_back(obj_id);
            }

            pNode = obj_in_aoi_map_.next(pNode);
        }
    }

	// 2: check new in
	VECTOR3 pos = getpos();
	FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkPlayer )
	{
		OBJID obj_id = obj->get_id();
		int vtype = 0;
		
		// last not in and check succ
		if ((!obj_in_aoi_map_.find( obj_id, (intptr_t&)vtype)) && aoi_->ray_test(obj))
		{
			obj_enter_ary_.push_back(obj_id);
			obj_in_aoi_map_.set(obj_id, 1);
		}
	}
	END_LINKMAP	

	if (aoi_->get_check_dynamic())
	{
		FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
		{
			OBJID obj_id = obj->get_id();
			int vtype = 0;

			if ((!obj_in_aoi_map_.find( obj_id, (intptr_t&)vtype)) && aoi_->ray_test(obj))
			{
				obj_enter_ary_.push_back(obj_id);
				obj_in_aoi_map_.set(obj_id, 1);
			}
		}
		END_LINKMAP	
	}

	// merge list
	for (size_t i = 0; i < obj_leave_ary_.size(); i++)
	{
		obj_in_aoi_map_.del(obj_leave_ary_[i]);
	}
}

void Trigger::process_serial()
{
    if( aoi_ == NULL ) return;

	for (size_t i = 0; i < obj_enter_ary_.size(); i++)
	{
		player_enter(obj_enter_ary_[i]);
	}
	obj_enter_ary_.clear();

	for (size_t i = 0; i < obj_leave_ary_.size(); i++)
	{
		player_leave(obj_leave_ary_[i]);
	}
	obj_leave_ary_.clear();
}

void Trigger::clear_aoi_map( )
{
	obj_in_aoi_map_.clean();
	obj_enter_ary_.clear();
	obj_leave_ary_.clear();
}

void Trigger::set_active(bool _active)
{
	is_active_ = _active;
    if( _active == false )
    {
        clear_aoi_map();
    }
}

int	Trigger::view_add( Ctrl* _obj )
{
    int result = Ctrl::view_add( _obj );
    return result;
}

int	Trigger::view_remove( Ctrl* _obj )
{
    int result = Ctrl::view_remove( _obj );
    return result;
}

bool Trigger::check_in_aoi(OBJID _obj_id, bool is_modify)
{
	return false;
}

void Trigger::add_aoi( int _shape_type, float _raduis, float _lw_percent )
{
	if (aoi_ == NULL)
	{
		aoi_ = new AoiTrigger( get_id(), _shape_type, _raduis, _lw_percent );
	}

	aoi_->set_radius(_raduis);
}

void Trigger::player_enter( OBJID _player_id )
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "do_player_enter_aoi" );
    lua_pushnumber( L, _player_id );
    lua_pushnumber( L, get_id() );

    if( llua_call( L, 3, 0, 0 ) )      
    {
		llua_fail( L, __FILE__, __LINE__ );
    }
}

void Trigger::player_move( OBJID _player_id, VECTOR3 _old_pos, VECTOR3 _cur_pos )
{
	return;
}

void Trigger::player_leave( OBJID _player_id )
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "do_player_leave_aoi" );
    lua_pushnumber( L, _player_id );
    lua_pushnumber( L, get_id() );

    if( llua_call( L, 3, 0, 0 ) )      
    {
		llua_fail( L, __FILE__, __LINE__ );
    }
}

int Trigger::c_do_set_angle_y( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	float angle = lua_tonumber( _state, 1 );
	set_angle_y( angle );
	return 0;
}

int Trigger::c_replace_pos( lua_State* _state )
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

	TRACE(2)( "[OBJ](Trigger) c_replace_pos, x = %f, y = %f, z = %f, scene = %d, worldidx = %d", x, y, z, (worldid & 0x0F), (worldid >> 8) );

	return 0;
}

int Trigger::c_add_aoi( lua_State* _state )
{
	lcheck_argc( _state, 3 );
	int shape_type = lua_tonumber( _state, 1 );
	float radius = lua_tonumber( _state, 2 );
	float lw_percent = lua_tonumber( _state, 3 );
	add_aoi( shape_type, radius, lw_percent );

	return 0;
}

int Trigger::c_set_active( lua_State* _state )
{
    lcheck_argc( _state, 1 );
    bool active = lua_toboolean(_state,1 );
    set_active(active);

    return 0;
}

int Trigger::c_get_neighbors( lua_State* _state )
{
	lua_newtable( _state );
	Node* pNode = obj_in_aoi_map_.first();
	int i = 1;
	while(pNode) {
		OBJID obj_id = (OBJID)pNode->key;
		lua_pushnumber( _state, i++ );
		lua_pushnumber( _state, (int)obj_id );
		lua_settable( _state, -3 );
		pNode = obj_in_aoi_map_.next(pNode);
	}

	return 1;
}

int Trigger::c_set_is_attackable( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	bool is_attackable = lua_toboolean(_state, 1);
	set_attackable(is_attackable);

	return 0;
}

int Trigger::c_set_check_dynamic( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	bool flag = lua_toboolean(_state, 1);
	if (aoi_ != NULL)
	{
		aoi_->set_check_dynamic(flag);
	}
	
	return 0;
}

int Trigger::c_rectangle_aoi_check( lua_State* _state )
{
    lcheck_argc(_state, 3);
    int target_id   = (int)lua_tonumber(_state, 1);
    float radius    = (float)lua_tonumber(_state, 2);
    float lw_percent= (float)lua_tonumber(_state, 3);

    Ctrl* target = g_worldmng->get_ctrl( target_id );
    if (target == NULL) 
    {
        return 0;
    }

    bool is_in_aoi = false;
    VECTOR3 corners[RECTANGLE_CORNER_NUM];
    get_rectangle_corners( this, radius, lw_percent, corners );
    is_in_aoi = is_point_in_polygon( target->getpos(), corners, RECTANGLE_CORNER_NUM );   
 
    lua_pushboolean( _state, is_in_aoi );
    return 1;
}


const char Trigger::className[] = "Trigger";
const bool Trigger::gc_delete_ = false;

Lunar<Trigger>::RegType Trigger::methods[] =
{
	LUNAR_OBJ_METHODS,
	LUNAR_CTRL_METHODS,
	LUNAR_SPIRIT_METHODS,
	LUNAR_TRIGGER_METHODS,
	{NULL, NULL}
};

