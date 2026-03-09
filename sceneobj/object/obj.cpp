#include "obj.h"
#include "world.h"
#include "log.h"
#include "3dmath.h"
#include "lmisc.h"
#include "player.h"

extern int g_frame;

#ifdef DEBUG
Obj::SerializeStat Obj::serialize_stats_[MAX_S_TYPE];

void Obj::print_serialize_stats()
{
    for( int i = 1; i < MAX_S_TYPE; i++ ) {
        SerializeStat& ss = serialize_stats_[i];
        if( ss.count_ > 0 )
            PROF( 0 )( "[SerializeStat] type: %d, count: %d, time: %d, avg: %f", i, ss.count_, get_msec( ss.time_ ), float( get_msec( ss.time_ ) )/ss.count_ );
    }

    memset( &serialize_stats_, 0, sizeof( serialize_stats_ ) );
}
#endif

Obj::Obj()
: first_time_( true ), sometimes_change_( true ), sometimes_change_offset_( 0 ), always_change_offset_( 0 ), data_ver_( 0 ), search_body_(true)
{
	pos_			= VECTOR3( 0, 0, 0 );
	type_			= OT_OBJ;
	prev_			= NULL;
	next_			= NULL;
	delete_	 		= FALSE;
	link_level_		= 0;
    radius_         = -1.0f;
    height_         = -1.0f;
    attack_radius_  = -1.0f;
    angle_			= VECTOR3( 0, 0, 0 );
   	scale_		    = VECTOR3( 1, 1, 1 );
    x_ = 0;
    z_ = 0;
    index_          = 0;
}

Obj::~Obj()
{
}

void Obj::serialize_unchange_ar( Ar& _ar )
{
}

void Obj::serialize_sometimes_change_ar( Ar& _ar )
{
}

void Obj::serialize_always_change_ar( Ar& _ar, bool _active )
{
}

int Obj::setpos( VECTOR3& _pos )
{
	pos_ = _pos;
    pos_.y = 0;
	return TRUE;
}

int Obj::setpos_y( float _y )
{
    pos_.y = _y;
    return TRUE;
}

int Obj::get_linktype() const
{
    if(is_player()) return LinkPlayer;
    else if(is_static()) return LinkStatic;
    else return LinkDynamic;
}

void Obj::ins_next_node( Obj* _obj )
{
	assert(_obj);

	_obj->set_prev_node( this );
	_obj->set_next_node( next_ );
	if( next_ )
		next_->set_prev_node( _obj );
	next_ = _obj;
}

void Obj::del_node()
{
	if( prev_ )
		prev_->set_next_node( next_ );
	if( next_ )
		next_->set_prev_node( prev_ );
	next_ = prev_ = NULL;
}

void Obj::set_angle_y( float _angle )
{
    if( _angle > 360.0f )
	{
        _angle = fmod( _angle, 360.0f );
	}
    else if( _angle < 0.0f )
    {
        _angle = fmod( _angle, 360.0f );
        _angle += 360.0f;
    }

    angle_.y = _angle;

}

void Obj::set_angle_x( float _angle )
{
    if( _angle > 360.0f )
	{
        _angle = fmod( _angle, 360.0f );
	}
    else if( _angle < 0.0f )
    {
        _angle = fmod( _angle, 360.0f );
        _angle += 360.0f;
    }

    angle_.x = _angle;

}

void Obj::set_angle_z( float _angle )
{
    if( _angle > 360.0f )
	{
        _angle = fmod( _angle, 360.0f );
	}
    else if( _angle < 0.0f )
    {
        _angle = fmod( _angle, 360.0f );
        _angle += 360.0f;
    }

    angle_.z = _angle;

}

void Obj::add_angle_y( float _angle )
{
    float angle = angle_.y + _angle;
    set_angle_y( angle );
}

void Obj::add_angle_x( float _angle )
{
    float angle = angle_.x + _angle;
    set_angle_x( angle );
}

bool Obj::is_range_obj_ground( Obj* _obj, float _range )
{
	VECTOR3 dist = getpos() - _obj->getpos();
	float len = get_radius() + _obj->get_radius() + _range ;
    if( fabsf( dist.x ) <= len && fabsf( dist.z ) <= len ) {
	    return  ( dist.x * dist.x + dist.z * dist.z < ( len * len  ) );
    }
    return false;
}

float Obj::get_height()
{
    if( height_ < 0.0f )
    {
        return 0.5f * scale_.x;
    }
    else
    {
        return height_ * scale_.x;
    }
}

float Obj::get_radius()
{
    if( radius_ < 0.0f )
        return 0.5f * scale_.x;
    else
        return radius_ * scale_.x;
}

float Obj::get_attack_radius()
{
    if( attack_radius_ < 0.0f )
        return 0.5f * scale_.x;
    else
        return attack_radius_ * scale_.x;
}
/******************************************************************************
for lua
******************************************************************************/

const char Obj::className[] = "Obj";
const bool Obj::gc_delete_ = false;


Lunar<Obj>::RegType Obj::methods[] =
{
	LUNAR_OBJ_METHODS,
	{NULL, NULL}
};


int Obj::c_gettype( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, gettype() );
	return 1;
}

int Obj::c_get_index( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, get_index() );
	return 1;
}


int Obj::c_istype( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	int type = (int)lua_tonumber( _L, 1 );
	lua_pushboolean( _L, is_type(type) );
	return 1;
}

int Obj::c_settype( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	int type = (int)lua_tonumber( _L, 1 );

	switch( type )
	{
	case OT_OBJ:
		break;

	case OT_STATIC:
		type |= OT_OBJ;
		break;

	case OT_CTRL:
		type |= OT_OBJ;
		break;

	case OT_COMMON:
		type |= OT_OBJ | OT_CTRL;
		break;

	case OT_ITEM:
		type |= OT_OBJ | OT_CTRL;
		break;

	case OT_SFX:
		type |= OT_OBJ | OT_CTRL;
		break;

	case OT_SPRITE:
		type |= OT_OBJ | OT_CTRL;
		break;

	case OT_PLAYER:
		type |= OT_OBJ | OT_CTRL | OT_SPRITE;
		break;

	case OT_MONSTER:
		type |= OT_OBJ | OT_CTRL | OT_SPRITE;
		break;

	case OT_NPC:
		type |= OT_OBJ | OT_CTRL | OT_SPRITE | OT_MONSTER;
		break;

	case OT_PET:
		type |= OT_OBJ | OT_CTRL | OT_SPRITE;
		break;
	}

	settype( type );
	return 0;
}

int Obj::c_set_index( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	int index;
	index = (int)lua_tonumber( _L, 1 );

    int old_index = get_index();
    if( old_index && old_index != index )
    ERR( 0 )( "[OBJ]Set obj index from script, old index: %d, new index: %d", old_index, index );

	set_index( index );
	return 0;
}

int Obj::c_getpos( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	VECTOR3 pos = getpos();
	lua_pushnumber( _L, pos.x );
	lua_pushnumber( _L, pos.y );
	lua_pushnumber( _L, pos.z );
	return 3;
}


int Obj::c_get_angle( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, get_angle_x() );
	lua_pushnumber( _L, get_angle_y() );
	lua_pushnumber( _L, get_angle_z() );
	return 3;
}


int Obj::c_get_angle_x( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, get_angle_x() );
	return 1;
}

int Obj::c_get_angle_y( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, get_angle_y() );
	return 1;
}

int Obj::c_get_angle_z( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, get_angle_z() );
	return 1;
}

int Obj::c_set_angle_x( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	angle_.x = (float)lua_tonumber( _L, 1 );
	return 0;
}

int Obj::c_set_angle_y( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	set_angle_y( (float)lua_tonumber( _L, 1 ) );
	return 0;
}

int Obj::c_set_angle_z( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	angle_.z = (float)lua_tonumber( _L, 1 );
	return 0;
}

int Obj::c_set_angle( lua_State* _L )
{
	lcheck_argc( _L, 3 );
	angle_.x = (float)lua_tonumber( _L, 1 );
	angle_.y = (float)lua_tonumber( _L, 2 );
	angle_.z = (float)lua_tonumber( _L, 3 );
	return 0;
}


int Obj::c_set_scale( lua_State* _L )
{
	lcheck_argc( _L, 3 );
	scale_.x = (float)lua_tonumber( _L, 1 );
	scale_.y = (float)lua_tonumber( _L, 2 );
	scale_.z = (float)lua_tonumber( _L, 3 );

    sometimes_change_ = true;

	return 0;
}


int Obj::c_get_scale( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, scale_.x );
	lua_pushnumber( _L, scale_.y );
	lua_pushnumber( _L, scale_.z );
	return 3;
}

int Obj::c_set_height( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	height_ = (float)lua_tonumber( _L, 1 );
	return 0;
}

int Obj::c_set_radius( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	radius_ = (float)lua_tonumber( _L, 1 );
	return 0;
}

int Obj::c_set_attack_radius( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	attack_radius_ = (float)lua_tonumber( _L, 1 );
	return 0;
}

int Obj::c_get_radius( lua_State* _L )
{
    lua_pushnumber( _L, radius_ * scale_.x );
    return 1;
}

int Obj::c_get_attack_radius( lua_State* _L )
{
    lua_pushnumber( _L, attack_radius_ * scale_.x);
    return 1;
}

int Obj::c_set_search_body( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	set_search_body(lua_toboolean( _L, 1 ));
	return 0;
}

int Obj::c_is_search_body( lua_State* _L )
{
    lua_pushboolean( _L, is_search_body());
    return 1;
}

int Obj::c_setpos( lua_State* _L )
{
	lcheck_argc( _L, 3 );
	VECTOR3 pos;
	pos.x = (float)lua_tonumber( _L, 1 );
	pos.y = (float)lua_tonumber( _L, 2 );
	pos.z = (float)lua_tonumber( _L, 3 );
    CHECK_NULL_POS( pos );
	setpos( pos );
	return 0;
}

int Obj::c_is_delete( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, this->is_delete() );
	return 1;
}


int Obj::c_get_dest_angle( lua_State* _L )
{
	lcheck_argc( _L, 2 );
	VECTOR3 dest_pos;
	dest_pos.x = (float)lua_tonumber( _L, 1 );
	dest_pos.y = 0.0f;
	dest_pos.z = (float)lua_tonumber( _L, 2 );

	VECTOR3 pos = getpos();
	pos.y = 0.0f;

	VECTOR3 dir1( 0.0f, 0.0f, -1.0f );
	VECTOR3 dir2 = dest_pos - pos;

	vc3_norm( dir2, dir2 );

	float dot = vc3_dot( dir1, dir2 );
	float radian = acos( dot );
	float degree = d3d_to_degree( radian );
	if( dir2.x < 0.0f )
	{
		degree = 360.0f - degree;
	}
	lua_pushnumber( _L, degree );
	return 1;
}

int Obj::c_get_this( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, (unsigned long)this );
	return 1;
}

int Obj::c_set_change( lua_State* _L )
{
    sometimes_change_ = true;
    return 0;
}

int Obj::c_get_angle_y_vec( lua_State* _state )
{
    lcheck_argc( _state, 0 );
    VECTOR3 vec = GET_DIR( get_angle_y() );
	lua_pushnumber( _state, vec.x );
	lua_pushnumber( _state, vec.z );
	return 2;
}
