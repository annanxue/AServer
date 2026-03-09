#include "bullet.h"
#include "define_func.h"
#include "timer.h"
#include "world_mng.h"
#include "log.h"
#include "camp_mng.h"

BulletSpace::BulletSpace()
    :cp_circle_(NULL)
{
    cp_circle_ = cpCircleShapeNew( NULL, 1, cpv(0, 0) );
}

BulletSpace::~BulletSpace()
{
    cpShapeFree( cp_circle_ );
}

bool BulletSpace::ray_test( VECTOR& start, VECTOR& end, Ctrl* target )
{
    VECTOR3 pos = target->getpos();
	cpSegmentQueryInfo info;
    mutex_.Lock();
    ((cpCircleShape*)cp_circle_)->r = target->get_radius();
    ((cpCircleShape*)cp_circle_)->tc = cpv(pos.x, pos.z);
    cpShapeSegmentQuery( cp_circle_, cpv(start.x, start.z), cpv(end.x, end.z), &info );
    mutex_.Unlock();
    if( info.shape != NULL )
    {
        //LOG(2)( "[Bullet]hit: %d, (sx: %f, sz: %f)--->(tx: %f, tz: %f), target(x: %f, z: %f, radius: %f)", 1, start.x, start.z, end.x, end.z, pos.x, pos.z, target->get_radius() );
        return true;
    }
    //LOG(2)( "[Bullet]hit: %d, (sx: %f, sz: %f)--->(tx: %f, tz: %f), target(x: %f, z: %f, radius: %f)", 0, start.x, start.z, end.x, end.z, pos.x, pos.z, target->get_radius() );
    return false;
}

bool BulletSpace::ray_test( VECTOR& start, VECTOR& end, float radius, Ctrl* target )
{
    VECTOR3 pos = target->getpos();
	cpSegmentQueryInfo info;
    mutex_.Lock();
    ((cpCircleShape*)cp_circle_)->r = radius + target->get_radius();
    ((cpCircleShape*)cp_circle_)->tc = cpv(pos.x, pos.z);
    cpShapeSegmentQuery( cp_circle_, cpv(start.x, start.z), cpv(end.x, end.z), &info );
    mutex_.Unlock();
    if( info.shape != NULL )
    {
        //LOG(2)( "[Bullet]hit: %d, (sx: %f, sz: %f)--->(tx: %f, tz: %f), target(x: %f, z: %f, radius: %f)", 1, start.x, start.z, end.x, end.z, pos.x, pos.z, radius );
        return true;
    }
    //LOG(2)( "[Bullet]hit: %d, (sx: %f, sz: %f)--->(tx: %f, tz: %f), target(x: %f, z: %f, radius: %f)", 0, start.x, start.z, end.x, end.z, pos.x, pos.z, radius );
    return false;
}


const char Bullet::className[] = "Bullet";
const bool Bullet::gc_delete_ = false;

Lunar<Bullet>::RegType Bullet::methods[] =
{
	LUNAR_OBJ_METHODS,
	LUNAR_CTRL_METHODS,
	LUNAR_SPIRIT_METHODS,
	LUNAR_BULLET_METHODS,
	{NULL, NULL}
};


Bullet::Bullet()
    :shoot_range_(10), 
    hit_index_(0), 
    is_emit_(false), 
    alive_max_(20.0f), 
    frame_count_(0), 
    owner_id_(NULL_ID), 
    do_delete_(false), 
    search_type_(0), 
    hit_continue_(0),
    path_type_(SEGMENT_LINE),
    last_hit_id_(NULL_ID),
	is_back_(0),
	back_to_cur_pos_(0),
	out_stealth_finish_stay_(0),
	back_stay_time_(0),
	move_status_(FRONT),
	back_time_(0),
    target_id_(NULL_ID),
    trace_type_(MUST_HIT),
    trace_time_(0.0f),
    trace_angle_(0.0f),
    trace_radius_(0.0f),
    trace_turn_speed_(0.0f)
{
	settype( OT_BULLET );
}

void Bullet::process_parallel()
{
    hit_index_ = 0;
    if( ( ++frame_count_ * (1.0/g_timermng->get_frame_rate()) ) > alive_max_ )
    {
        do_delete_ = true;
        return;
    }

    if ( !is_emit_ )
    {
        return;
    }

    if (path_type_ == TRACE_TARGET)
    {
        if (!need_process_trace_move())
        {
            return;
        }
    }

    if (move_status_ != STAY)
    {
        old_pos_ = getpos();
        Spirit::process_parallel();
        VECTOR3 cur_pos = getpos();

        if( path_type_ != TRACE_TARGET && vc3_check_out_range( start_pos_, cur_pos, shoot_range_-0.2 ) )
        {
            if(is_back_ == 1 && move_status_ == FRONT)
            {
                move_status_ = STAY;
                back_time_ = g_timermng->get_ms() + back_stay_time_;
            }
            else
            {
                do_delete_ = true;
            }
        }
        else if( !is_mk_pos_succ() )
        {
            do_delete_ = true;
        }
        else if( trace_type_ == DYNAMIC_TRACE && old_pos_ == cur_pos )
        {
            do_delete_ = true;
        }
        else if ( need_search_hit_when_move() )
        {
            search_hit();
        }
    }
    else
    {
        bool flag = false;
        if(out_stealth_finish_stay_)
        {
            Ctrl* owner = g_worldmng->get_ctrl( owner_id_ );
            if(owner && !static_cast<Spirit*>(owner)->is_stealth())
            {
                flag = true;
            }
        }

        if(g_timermng->get_ms() >= back_time_ || flag)
        {
            move_status_ = BACK;

            VECTOR3 src_pos = getpos();
            target_pos_ = start_pos_;

            if(back_to_cur_pos_)
            {
                Ctrl* owner = g_worldmng->get_ctrl( owner_id_ );
                if( owner )
                {
                    target_pos_ = owner->getpos();
                    shoot_range_ = vc3_lensq(target_pos_ - src_pos);
                }
            }
            
            float angle = get_degree( target_pos_, src_pos );
            start_emit( src_pos.x, src_pos.z, angle, shoot_range_, get_speed() );
        }

        if( path_type_ == SEGMENT_LINE )
        {
            search_hit();
        }
    }
}

void Bullet::process_serial()
{
    if ( path_type_ == TRACE_TARGET && trace_type_ == DYNAMIC_TRACE )
    {
        Ctrl* trace_target = g_worldmng->get_ctrl( target_id_ );
        if ( !trace_target )
        {
            LuaServer* lua_svr = Ctrl::get_lua_server();
            lua_State* L = lua_svr->L();
            lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
            lua_pushstring( L, "do_bullet_trace" );
            lua_pushnumber( L, get_id() );
            if( llua_call( L, 2, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
        }
    }

    if( hit_index_ != 0 )
    {
        int idx;
        LuaServer* lua_svr = Ctrl::get_lua_server();
        lua_State* L = lua_svr->L();
        lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
        lua_pushstring( L, "do_bullet_hit" );
        lua_pushnumber( L, owner_id_ );

        lua_newtable( L );
        for ( idx = 0; idx < hit_index_; idx++ )
        {
            lua_pushnumber( L, idx + 1 );
            lua_pushnumber( L, hit_id_[idx] );
            lua_settable( L, -3 );
        }

        lua_pushnumber( L, get_id() );
        if( llua_call( L, 4, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );

        if( hit_continue_ == 0 )
        {
            safe_delete();
        }
    }
    if( do_delete_ )
    {
        safe_delete();
    }
}

void Bullet::search_hit_player( Ctrl* _owner )
{
    VECTOR3 pos = getpos();
    FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkPlayer )
    {
        bool target_is_dead = ((Spirit*)obj)->is_die();
        OBJID target_id = ((Spirit*)obj)->get_id();
        if( !target_is_dead && target_id != last_hit_id_ && target_id != owner_id_ )
        {
            if( get_space().ray_test( old_pos_, pos, get_radius(), obj ) )
            {
                if ( hit_index_ < MAX_HIT )
                {
                    hit_id_[hit_index_++] = obj->get_id();
                }
                else
                {
                    return;
                }
            }
        }
    }
    END_LINKMAP	
    if ( !(search_type_ & 2) )
    {
        FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
        {
            if (obj->is_robot()) {
                bool target_is_dead = ((Spirit*)obj)->is_die();
                OBJID target_id = ((Spirit*)obj)->get_id();
                if( !target_is_dead && target_id != last_hit_id_ && target_id != owner_id_ )
                {
                    if( get_space().ray_test( old_pos_, pos, get_radius(), obj ) )
                    {
                        if ( hit_index_ < MAX_HIT )
                        {
                            hit_id_[hit_index_++] = obj->get_id();
                        }
                        else
                        {
                            return;
                        }
                    }
                }
            }
        }
        END_LINKMAP
    }
}

void Bullet::search_hit_monster( Ctrl* _owner )
{
    bool robot_include = search_type_ & 1; 
    VECTOR3 pos = getpos();
    FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
    {
        bool target_is_dead = ((Spirit*)obj)->is_die();
        OBJID target_id = ((Spirit*)obj)->get_id();
        if (!robot_include && obj->is_robot()) {
            continue;
        }
        if( obj->is_monster() && !target_is_dead && target_id != last_hit_id_ && target_id != owner_id_ )
        {
            if( get_space().ray_test( old_pos_, pos, get_radius(), obj ) )
            {
                if ( hit_index_ < MAX_HIT )
                {
                    hit_id_[hit_index_++] = obj->get_id();
                }
                else
                {
                    return;
                }
            }
        }
    }
    END_LINKMAP	
}

void Bullet::search_hit_trigger( Ctrl* _owner )
{
    VECTOR3 pos = getpos();
    FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
    {
        if( obj->is_trigger() )
        {
            OBJID target_id = ((Spirit*)obj)->get_id();
            if( obj->is_attackable_trigger() && target_id != last_hit_id_ && target_id != owner_id_ )
            {
                if( get_space().ray_test( old_pos_, pos, get_radius(), obj ) )
                {
                    if ( hit_index_ < MAX_HIT )
                    {
                        hit_id_[hit_index_++] = obj->get_id();
                    }
                    else
                    {
                        return;
                    }
                }
            }
        }
    }
    END_LINKMAP	
}

void Bullet::search_hit()
{
    Ctrl* owner = g_worldmng->get_ctrl( owner_id_ );
    if( owner )
    {
        //OBJID search_id = NULL_ID;
        // search player
        if( search_type_ & 1 )
        {
            search_hit_player( owner );
        }
        if( hit_index_ >= MAX_HIT )
        {
            return;
        }
        else
        {
            // search monster and trigger
            if( (search_type_ & 2) && (search_type_ & 4) )
            {
                search_hit_monster( owner );
                if( hit_index_ < MAX_HIT )
                {
                    return search_hit_trigger( owner );
                }
                else
                {
                    return;
                }
            }
            else if( search_type_ & 2 )
            {
                return search_hit_monster( owner );
            }
            else if( search_type_ & 4 )
            {
                return search_hit_trigger( owner );
            }
        }
    }
}

BulletSpace& Bullet::get_space()
{
    static BulletSpace space;
    return space;
}

void Bullet::start_emit( float _x, float _z, float _angle, float _range, float _speed )
{
    VECTOR3 pos( _x, 0, _z );
    setpos( pos ); 
    set_angle_y( _angle );
    set_shoot_range( _range );
    set_speed( _speed );
    start_pos_ = pos;
    is_emit_ = true;
    post_state_msg( MK_MSG(SYS_SET_STATE, STATE_MOVE_GROUND), _angle*FLOAT2INT_MUL_FACTOR, 0, 0, 0 );
}

void Bullet::safe_delete()
{
    // call lua
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "do_bullet_delete" );
    lua_pushnumber( L, this->get_id() );
    if( llua_call( L, 2, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );

    // real delete
    Ctrl::safe_delete();
}

bool Bullet::need_search_hit_when_move()
{
    if (path_type_ == SEGMENT_LINE)
    {
        return true;
    }
    else if (path_type_ == TRACE_TARGET)
    {
        if (trace_type_ == PROBABLY_HIT)
        {
            return true;
        }
        if (trace_type_ == DYNAMIC_TRACE)
        {
            return true;
        }
    }
    return false;
}

bool Bullet::need_process_trace_move()
{
    if (trace_type_ == PROBABLY_HIT)
    {
        trace_time_ = trace_time_ - (1.0f / g_timermng->get_frame_rate());
        Obj* target = g_worldmng->get_ctrl(target_id_);
        if (trace_time_ < 0 || target == NULL || ((Spirit*)target)->is_die())
        {
            do_delete_ = true;
            return false;
        }
        else
        {
            float angle = get_degree(target->getpos(), getpos());
            state.set_param(STATE_MOVE_GROUND, 0, angle * FLOAT2INT_MUL_FACTOR);
            return true;
        }
    }
    else if (trace_type_ == DYNAMIC_TRACE)
    {
        // trace timeout
        trace_time_ = trace_time_ - (1.0f / g_timermng->get_frame_rate());
        if (trace_time_ < 0)
        {
            do_delete_ = true;
            return false;
        }

        // adjust the angle
        Ctrl* target = g_worldmng->get_ctrl( target_id_ );
        if ( target != NULL )
        {
            float angle = get_degree( target->getpos(), getpos() );
            float cur_angle_y = get_angle_y();
            float diff_angle =  angle - cur_angle_y;
            if ( diff_angle < 0 )
            {
                diff_angle = -diff_angle;
            }
            int sign = 1;
            if ( diff_angle > 180.0f )
            {
                sign *= -1;
                diff_angle -= 180.0f;
            }
            if ( angle < cur_angle_y )
            {
                sign *= -1;
            }
            float angle_per_frame = trace_turn_speed_ / g_timermng->get_frame_rate();
            if ( diff_angle <= angle_per_frame )
            {
                cur_angle_y = angle;
            }
            else
            {
                cur_angle_y += angle_per_frame * sign;
            }
            set_angle_y( cur_angle_y );
            cur_angle_y = get_angle_y();
            state.set_param( STATE_MOVE_GROUND, 0, cur_angle_y * FLOAT2INT_MUL_FACTOR );
        }
        return true;
    }

    return false;
}

int Bullet::c_emit_to_pos( lua_State* _state )
{
    Ctrl* owner = g_worldmng->get_ctrl( owner_id_ );
    if( owner )
    {
        VECTOR3 src_pos = owner->getpos();
        float angle = get_degree( target_pos_, src_pos );
        start_emit( src_pos.x, src_pos.z, angle, shoot_range_, get_speed() );
    }
    return 0;
}

int Bullet::c_set_owner_id( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    owner_id_ = lua_tointeger( _state, 1 );
    return 0;
}

int Bullet::c_get_owner_id( lua_State* _state )
{
    lua_pushnumber( _state, owner_id_ );
    return 1;
}

int Bullet::c_set_target_id( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    target_id_ = lua_tointeger( _state, 1 );
    return 0;
}

int Bullet::c_get_target_id( lua_State* _state )
{
    lua_pushnumber( _state, target_id_ );
    return 1;
}

int Bullet::c_get_target_pos( lua_State* _state )
{
    lua_pushnumber( _state, target_pos_.x );
    lua_pushnumber( _state, target_pos_.z );
    return 2;
}

int Bullet::c_set_target_pos( lua_State* _state )
{
	lcheck_argc( _state, 2 );
    target_pos_.x = lua_tonumber( _state, 1 );
    target_pos_.z = lua_tonumber( _state, 2 );
    target_pos_.y = 0;
    return 0;
}

int Bullet::c_get_is_emit( lua_State* _state )
{
    int is_emit = (int)is_emit_;
    lua_pushinteger( _state, is_emit );
    return 1;
}

int Bullet::c_set_search_type( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    search_type_ = lua_tointeger( _state, 1 );
    return 0;
}

int Bullet::c_get_search_type( lua_State* _state )
{
    lua_pushinteger( _state, search_type_ );
    return 1;
}

int Bullet::c_set_shoot_range( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    float range = lua_tonumber( _state, 1 );
    set_shoot_range( range );
    return 0;
}

int Bullet::c_set_hit_continue( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    hit_continue_ = lua_tointeger( _state, 1 );
    return 0;
}

int Bullet::c_set_path_type( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    path_type_ = lua_tointeger( _state, 1 );
    return 0;
}

int Bullet::c_get_path_type( lua_State* _state)
{
    lua_pushinteger( _state, path_type_);
    return 1;
}

int Bullet::c_set_last_hit( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    last_hit_id_ = lua_tointeger( _state, 1 );
    return 0;
}

int Bullet::c_set_back_time( lua_State* _state )
{
	lcheck_argc( _state, 4 );
	is_back_ = lua_tointeger( _state, 1 );
	back_stay_time_ = lua_tonumber( _state, 2 );
	back_to_cur_pos_ = lua_tonumber( _state, 3 );
	out_stealth_finish_stay_ = lua_tonumber( _state, 4 );
	return 0;
}

int Bullet::c_get_move_status( lua_State* _state )
{
	lcheck_argc( _state, 0 );
    lua_pushinteger( _state, move_status_ );
    return 1;
}

int Bullet::c_set_trace_type( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	int trace_type = lua_tointeger( _state, 1 );
    trace_type_ = (TraceType)trace_type;
    return 0;
}

int Bullet::c_set_trace_time( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	trace_time_ = lua_tonumber( _state, 1 );
    return 0;
}

int Bullet::c_set_trace_params( lua_State* _state )
{
	lcheck_argc( _state, 5 );
	int trace_type = lua_tointeger( _state, 1 );
    trace_type_ = (TraceType)trace_type;
	trace_time_ = lua_tonumber( _state, 2 );
	trace_angle_ = lua_tonumber( _state, 3 );
	trace_radius_ = lua_tonumber( _state, 4 );
	trace_turn_speed_ = lua_tonumber( _state, 5 );
    return 0;
}
