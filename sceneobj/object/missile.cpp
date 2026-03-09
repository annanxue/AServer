#include "missile.h"
#include "define_func.h"
#include "timer.h"
#include "world_mng.h"
#include "player_mng.h"
#include "log.h"
#include "curve_mng.h"
#include "lua_utils.h"
#include "msg.h"
#include "scene_obj_utils.h"

using namespace Lua;

extern bool g_trace_search;
const char Missile::className[] = "Missile";
const bool Missile::gc_delete_ = false;

Lunar<Missile>::RegType Missile::methods[] =
{
	LUNAR_OBJ_METHODS,
	LUNAR_CTRL_METHODS,
	LUNAR_MISSILE_METHODS,
	{NULL, NULL}
};

Missile::Missile()
{
	settype( OT_MISSILE );

    owner_id_        = NULL_ID;
    target_id_       = NULL_ID;
    old_pos_         = NULL_POS;
    do_delete_       = false;
    apply_type_      = 0;
    target_index_    = 0;
    hit_index_       = 0;
    stages_          = NULL;
    stage_count_     = 0;
    stage_index_     = -1;
    ori_             = NULL_POS;
    is_hit_continue_ = false;
}

Missile::~Missile()
{
    for (int i = 0; i < stage_count_; i++)
    {
        SAFE_DELETE( stages_[i] )
    }
    SAFE_DELETE_ARRAY( stages_ )
}

void Missile::process_parallel()
{
    if ( do_delete_ )
    {
        return;
    }

    MissileStage* stage = get_current_stage();
    if ( !stage )
    {
        return;
    }

    search_target();

    move();

    if ( get_radius() > 0 )
    {
        search_hit();
    }

    if ( !stage->has_predicted_ && stage->is_almost_finished() )
    {
        predict_stage_end();
    }

    if ( stage->is_finished() )
    {
        target_index_ = 0;
        next_stage();
    }
}

void Missile::process_serial()
{
    if( hit_index_ != 0 )
    {
        int idx;
        LuaServer* lua_svr = Ctrl::get_lua_server();
        lua_State* L = lua_svr->L();
        lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
        lua_pushstring( L, "check_missile_hit" );
        lua_pushnumber( L, get_id() );

        lua_newtable( L );
        for ( idx = 0; idx < hit_index_; idx++ )
        {
            lua_pushnumber( L, idx + 1 );
            lua_pushnumber( L, hit_id_array_[idx] );
            lua_settable( L, -3 );
        }

        if( llua_call( L, 3, 0, 0 ) )
        {
            llua_fail( L, __FILE__, __LINE__ );
        }
    }

    if( target_index_ != 0 )
    {
        int idx;
        LuaServer* lua_svr = Ctrl::get_lua_server();
        lua_State* L = lua_svr->L();
        lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
        lua_pushstring( L, "check_missile_target" );
        lua_pushnumber( L, get_id() );

        lua_newtable( L );
        for ( idx = 0; idx < target_index_; idx++ )
        {
            lua_pushnumber( L, idx + 1 );
            lua_pushnumber( L, target_id_array_[idx] );
            lua_settable( L, -3 );
        }

        if( llua_call( L, 3, 0, 0 ) )
        {
            llua_fail( L, __FILE__, __LINE__ );
        }
    }

    if ( do_delete_ )
    {
        safe_delete();
    }
}

int Missile::setpos( VECTOR3& _pos )
{
    int is_succ = Ctrl::setpos( _pos );
    if ( is_succ )
    {
        setpos_y( _pos.y );
    }
    return is_succ;
}

void Missile::predict_stage_end()
{
    MissileStage* cur_stage = get_current_stage();
    if ( !cur_stage )
    {
        return;
    }

    if ( cur_stage->has_predicted_ )
    {
        return;
    }

    MissileStage* next_stage = get_next_stage();
    if ( !next_stage )
    {
        return;
    }

    VECTOR3 end_pos, end_ori;
    if ( !cur_stage->move_by_frames( end_pos, end_ori, cur_stage->max_frame_ ) )
    {
        return;
    }

    float end_angle_y = d3d_to_degree( vc3_get_angle( end_ori ) );
    float stage_persist_time = 1.0 * cur_stage->max_frame_ / g_timermng->get_frame_rate();

    Ar ar;
    SET_SNAPSHOT_TYPE( ar, ST_PREDICT_MISSILE_STAGE_END );
    ar << get_id();
    ar << stage_index_;
    ar << stage_persist_time;
    ar << end_pos.x;
    ar << end_pos.y;
    ar << end_pos.z;
    ar << end_angle_y;
    BROADCAST( ar, this, NULL );

    cur_stage->has_predicted_ = true;
}

void Missile::next_stage()
{
    MissileStage* cur_stage = get_current_stage();

    stage_index_++;
    if ( stage_index_ >= stage_count_ )
    {
        do_delete_ = true;
        return;
    }

    MissileStage* stage = stages_[stage_index_];
    if ( !stage )
    {
        do_delete_ = true;
        return;
    }

    if ( stage->type_ == TRACK && target_id_ == NULL_ID )
    {
        do_delete_ = true;
        return;
    }

    stage->start();

    if ( cur_stage == NULL || !cur_stage->has_predicted_ )
    {
        Ar ar;
        SET_SNAPSHOT_TYPE( ar, ST_MISSILE_STAGE );
        ar << get_id();
        ar << stage_index_;
        ar << stage->type_;
        ar << stage->start_pos_.x;
        ar << stage->start_pos_.y;
        ar << stage->start_pos_.z;
        ar << stage->start_angle_y_;
        BROADCAST( ar, this, NULL );
    }
}

MissileStage* Missile::get_current_stage()
{
    if ( stage_index_ < 0 || stage_index_ >= stage_count_ )
    {
        return NULL;
    }
    return stages_[stage_index_];
}

MissileStage* Missile::get_next_stage()
{
    int next_stage_index = stage_index_ + 1;
    if ( next_stage_index < 0 || next_stage_index >= stage_count_ )
    {
        return NULL;
    }
    return stages_[next_stage_index];
}

void Missile::move()
{
    old_pos_ = getpos();

    MissileStage* stage = get_current_stage();
    if ( !stage )
    {
        return;
    }

    VECTOR3 new_pos;
    if ( stage->move( new_pos, ori_ ) )
    {
        if ( !is_ignore_block() )
        {
            float hit_rate;
            VECTOR3 hit_pos;
            raycast( &old_pos_, &new_pos, &hit_pos, &hit_rate );
            if ( hit_rate < 1 )
            {
                new_pos = hit_pos;
                do_delete_ = true;
            }
        }

        if ( setpos( new_pos ) )
        {
            if ( ori_.x != 0 || ori_.z != 0 )
            {
                float angle_y = d3d_to_degree( vc3_get_angle( ori_ ) );
                set_angle_y( angle_y );
            }
            g_playermng->trace_pos_to_all_players( this );
        }
        else
        {
            do_delete_ = true;
        }
    }
}

BulletSpace& Missile::get_space()
{
    static BulletSpace space;
    return space;
}

void Missile::search_hit_player()
{
    VECTOR3 pos = getpos();
    FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkPlayer )
    {
        if ( !check_valid_hit( obj ) )
        {
            return;
        }
    }
    END_LINKMAP	

    if ( !(apply_type_ & 2) )
    {
        FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
        {
            if ( obj->is_robot() )
            {
                if ( !check_valid_hit( obj ) )
                {
                    return;
                }
            }
        }
        END_LINKMAP
    }
}

void Missile::search_hit_monster()
{
    bool robot_include = apply_type_ & 1; 
    VECTOR3 pos = getpos();

    FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
    {
        if ( !robot_include && obj->is_robot() )
        {
            continue;
        }
        if( obj->is_monster() )
        {
            if ( !check_valid_hit( obj ) )
            {
                return;
            }
        }
    }
    END_LINKMAP	
}

void Missile::search_hit_trigger()
{
    VECTOR3 pos = getpos();

    FOR_LINKMAP( get_world(), plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
    {
        if( obj->is_attackable_trigger() )
        {
            if ( !check_valid_hit( obj ) )
            {
                return;
            }
        }
    }
    END_LINKMAP	
}

bool Missile::check_valid_hit( Ctrl* _obj )
{
    if ( hit_index_ >= MAX_HIT )
    {
        return false;
    }

    VECTOR3 pos = getpos();
    float max_height = _obj->get_height() + get_height() * 0.5f;
    if ( old_pos_.y <= max_height || pos.y <= max_height )
    {
        float radius = get_radius();
        if( get_space().ray_test( old_pos_, pos, radius, _obj ) )
        {
            hit_id_array_[hit_index_++] = _obj->get_id();
        }
    }
    return hit_index_ < MAX_HIT;
}

void Missile::check_valid_target( Ctrl* _target, float _search_angle, float _search_radius )
{
    if ( target_index_ >= MAX_TARGET )
    {
        return;
    }
    if( is_y_angle_obj( _target, _search_angle ) && is_range_obj_ground( _target, _search_radius ) )
    {
        target_id_array_[target_index_++] = _target->get_id();
    }
}

void Missile::search_target()
{
    target_index_ = 0;
    if ( target_id_ != NULL_ID )
    {
        return;
    }

    World* world = get_world();
    if ( world == NULL )
    {
        return;
    }

    MissileStage* stage = get_current_stage();
    if ( !stage )
    {
        return;
    }

    if ( !stage->should_search_target() )
    {
        return;
    }

    VECTOR3 pos = getpos();
    float search_angle = stage->search_angle_;
    float search_radius = stage->search_radius_;
    if ( g_trace_search )
    {
        Ar ar;
        SET_SNAPSHOT_TYPE( ar, ST_TRACE_SEARCH );
        ar << 1;
        ar << get_id();
        ar << pos.x;
        ar << pos.z;
        ar << get_angle_y();
        ar << search_radius;
        ar << search_angle;
        BROADCAST( ar, this, NULL );
    }

    if ( apply_type_ & 1 )
    {
        FOR_LINKMAP( world, plane_, pos, search_radius, Obj::LinkPlayer )
            check_valid_target( obj, search_angle, search_radius );
        END_LINKMAP
        if ( !(apply_type_ & 2) )
        {
            FOR_LINKMAP( world, plane_, pos, search_radius, Obj::LinkDynamic )
                if (obj->is_robot())
                {
                    check_valid_target( obj, search_angle, search_radius );
                }
            END_LINKMAP
        }
    }

    bool robot_include = apply_type_ & 1;
    if ( apply_type_ & 2 && apply_type_ & 4 )
    {
        FOR_LINKMAP( world, plane_, pos, search_radius, Obj::LinkDynamic )
            if (obj->is_monster() || obj->is_attackable_trigger())
            {
                if (robot_include || !obj->is_robot())
                {
                    check_valid_target( obj, search_angle, search_radius );
                }
            }
        END_LINKMAP
    }
    else if ( apply_type_ & 2 )
    {
        FOR_LINKMAP( world, plane_, pos, search_radius, Obj::LinkDynamic )
            if (obj->is_monster())
            {
                if (robot_include || !obj->is_robot())
                {
                    check_valid_target( obj, search_angle, search_radius );
                }
            }
        END_LINKMAP
    }
    else if ( apply_type_ & 4 )
    {
        FOR_LINKMAP( world, plane_, pos, search_radius, Obj::LinkDynamic )
            if (obj->is_attackable_trigger())
            {
                check_valid_target( obj, search_angle, search_radius );
            }
        END_LINKMAP
    }
}

void Missile::search_hit()
{
    hit_index_ = 0;
    if( apply_type_ & 1 )
    {
        search_hit_player();
    }
    if( hit_index_ >= MAX_HIT )
    {
        return;
    }
    if( apply_type_ & 2 )
    {
        search_hit_monster();
    }
    if( hit_index_ >= MAX_HIT )
    {
        return;
    }
    if( apply_type_ & 4 )
    {
        search_hit_trigger();
    }
}

void Missile::safe_delete()
{
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* L = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( L, "do_missile_delete" );
    lua_pushnumber( L, this->get_id() );
    if( llua_call( L, 2, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );

    Ctrl::safe_delete();
}

int Missile::c_set_target_id( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	target_id_ = lua_tointeger( _state, 1 );
    TRACE(2)( "[Missile](c_set_target_id) missile ctrl_id: %d, target ctrl_id: %d", get_id(), target_id_ );

    MissileStage* cur_stage = get_current_stage();
    if ( !cur_stage )
    {
        ERR(2)( "[Missile](c_set_target_id) current stage is NULL" );
        return 0;
    }

    MissileStage* next_stage = get_next_stage();
    if ( !next_stage )
    {
        ERR(2)( "[Missile](c_set_target_id) next stage is NULL" );
        return 0;
    }

    if ( next_stage->type_ != TRACK )
    {
        ERR(2)( "[Missile](c_set_target_id) next stage type is not TRACK, type: %d", next_stage->type_ );
        return 0;
    }

    int max_frame = g_frame + PREDICT_LEAD_FRAME - cur_stage->start_frame_;
    if ( max_frame < cur_stage->max_frame_ )
    {
        cur_stage->max_frame_ = max_frame;
    }

    predict_stage_end();
    return 0;
}

int Missile::c_on_hit( lua_State* _state )
{
    if ( !is_hit_continue_ )
    {
        do_delete_ = true;
    }
    return 0;
}

int Missile::c_set_owner_id( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	owner_id_ = lua_tointeger( _state, 1 );
    return 0;
}

int Missile::c_get_owner_id( lua_State* _state )
{
    lua_pushnumber( _state, owner_id_ );
    return 1;
}

int Missile::c_get_current_stage_index( lua_State* _state )
{
    lua_pushnumber( _state, stage_index_ );
    return 1;
}

int Missile::c_init( lua_State* _state )
{
    lcheck_argc( _state, 1 );
    if( lua_type( _state, -1 ) != LUA_TTABLE )
    {
        lua_pushboolean( _state, false );
        return 1;
    }

    float horizontal_radius = 0;
    float vertical_radius = 0;
    int ignore_block = 0;
    int hit_continue = 0;
    get_table_number( _state, -1, "HorizontalRadius", horizontal_radius );
    get_table_number( _state, -1, "VerticalRadius", vertical_radius );
    get_table_number( _state, -1, "IgnoreBlock", ignore_block );
    get_table_number( _state, -1, "HitContinue", hit_continue );
    get_table_number( _state, -1, "ApplyType", apply_type_ );

    set_radius( horizontal_radius );
    set_height( vertical_radius * 2 );
    set_ignore_block( ignore_block == 1 );
    is_hit_continue_ = hit_continue == 1;
    
    bool ret = init_stages( _state );

    lua_pushboolean( _state, ret );
    return 1;
}

bool Missile::init_stages( lua_State* _state )
{
    lua_pushstring( _state, "Stages" );
    lua_gettable( _state, -2 ); 
    if( lua_type( _state, -1 ) != LUA_TTABLE )
    {
        ERR(2)( "[Missile](init_stages) not a lua table" );
        lua_pop( _state, 1 );
        return false;
    } 

    stage_count_ = lua_objlen( _state, -1 );
    assert( stages_ == NULL );
    stages_ = new MissileStage*[stage_count_];

    int stage_type = 0;
    int missile_ctrl_id = get_id();
    for (int i = 0; i < stage_count_; i++ )
    {
        lua_rawgeti( _state, -1, i + 1 );
        get_table_number_by_index( _state, -1, 1, stage_type );
        switch ( stage_type )
        {
            case CURVE:
                stages_[i] = new CurveMove();
                break;
            case TRACK:
                stages_[i] = new TrackMove();
                break;
            case STAY:
                stages_[i] = new StayMove();
                break;
            default:
                stages_[i] = NULL;
                break;
        }
        if ( stages_[i] == NULL )
        {
            ERR(2)( "[Missile](init_stages) unknown stage type: %d", stage_type );
            break;
        }
        stages_[i]->init( _state, missile_ctrl_id );
        lua_pop( _state, 1 );

        TRACE(2)( "[Missile](init_stages) add stage, missile: 0x%x, type: %d", this, stages_[i]->type_ );
    }

    lua_pop( _state, 1 );
    return true;
}

int Missile::c_fire( lua_State* _state )
{
    if ( stage_index_ == -1 )
    {
        next_stage();
    }
    else
    {
        ERR(2)( "[Missile](c_fire) fired already, stage index: %d", stage_index_ );
    }
    return 0;
}

void MissileStage::init( lua_State* _state, OBJID _missile_ctrl_id )
{
    int max_time = 0, start_search_time = 0, stop_search_time = 0;
    lua_rawgeti( _state, -1, 2 );
    if( lua_type( _state, -1 ) != LUA_TTABLE )
    {
        ERR(2)( "[MissileStage](init) cfg is not a table!" );
        lua_pop( _state, 1 );
        return;
    }

    speed_ = search_angle_ = search_radius_ = 0;
    get_table_number_by_index( _state, -1, 1, speed_ );
    get_table_number_by_index( _state, -1, 2, max_time );
    get_table_number_by_index( _state, -1, 3, search_angle_ );
    get_table_number_by_index( _state, -1, 4, search_radius_ );
    get_table_number_by_index( _state, -1, 5, start_search_time );
    get_table_number_by_index( _state, -1, 6, stop_search_time );
    lua_pop( _state, 1 );

    int frame_rate = g_timermng->get_frame_rate();
    max_frame_ = (int)(max_time * 0.001 * frame_rate);
    start_search_frame_ = (int)(start_search_time * 0.001 * frame_rate);
    stop_search_frame_ = (int)(stop_search_time * 0.001 * frame_rate);
    missile_ctrl_id_ = _missile_ctrl_id;
}

void MissileStage::start()
{
    Missile* missile = (Missile*)g_worldmng->get_ctrl( missile_ctrl_id_ );
    if ( !missile )
    {
        return;
    }
    start_pos_ = missile->getpos();
    start_angle_y_ = missile->get_angle_y();
    start_frame_ = g_frame;
    has_predicted_ = false;
}

bool MissileStage::is_almost_finished()
{
    return PREDICT_LEAD_FRAME + g_frame - start_frame_ >= max_frame_;
}

bool MissileStage::is_finished()
{
    return g_frame - start_frame_ >= max_frame_;
}

bool MissileStage::should_search_target()
{
    int frame_passed = g_frame - start_frame_;
    return search_angle_ > 0 && search_radius_ > 0 && frame_passed >= start_search_frame_ && frame_passed <= stop_search_frame_;
}

bool MissileStage::move_by_frames( VECTOR3& _pos, VECTOR3& _ori, int _frame_count )
{
    ERR(2)( "[MissileStage](move_by_frames) Unsupported operation" );
    return false;
}

void CurveMove::init( lua_State* _state, OBJID _missile_ctrl_id )
{
    MissileStage::init( _state, _missile_ctrl_id );
    lua_rawgeti( _state, -1, 3 );
    if( lua_type( _state, -1 ) != LUA_TTABLE )
    {
        ERR(2)( "[CurveMove](init) cfg is not a table!" );
        lua_pop( _state, 1 );
        return;
    }
    
    int is_finish_continue = 0;
    get_table_number_by_index( _state, -1, 1, curve_id_ );
    get_table_number_by_index( _state, -1, 2, is_finish_continue );
    lua_pop( _state, 1 );
    is_finish_continue_ = is_finish_continue == 1;

    BezierCurve* curve = g_curvemng->get_curve( curve_id_ );
    if ( curve == NULL )
    {
        ERR(2)( "[CurveMove](init) curve not found, curve_id: %d", curve_id_ );
        return;
    }

    if ( !is_finish_continue )
    {
        int max_frame = ceil( curve->get_length() / speed_ * g_timermng->get_frame_rate() );
        if ( max_frame < max_frame_ )
        {
            max_frame_ = max_frame;
        }
    }
}

bool CurveMove::move( VECTOR3& _pos, VECTOR3& _ori )
{
    return move_by_frames( _pos, _ori, g_frame - start_frame_ );
}

bool CurveMove::move_by_frames( VECTOR3& _pos, VECTOR3& _ori, int _frame_count )
{
    BezierCurve* curve = g_curvemng->get_curve( curve_id_ );
    if ( curve == NULL )
    {
        ERR(2)( "[CurveMove](move) curve not found, curve_id: %d", curve_id_ );
        return false;
    }

    float total_len = curve->get_length();
    float passed_len = speed_ * _frame_count / g_timermng->get_frame_rate();
    float t = passed_len / total_len;

    float sin = SIN( start_angle_y_ );
    float cos = COS( start_angle_y_ );
    VECTOR3 curve_pos = curve->get_pos( t );
    _pos.x = curve_pos.x * cos - curve_pos.z * sin + start_pos_.x;
    _pos.z = curve_pos.x * sin + curve_pos.z * cos + start_pos_.z;
    _pos.y = curve_pos.y + start_pos_.y;

    VECTOR3 curve_tangent = curve->get_tangent( t );
    _ori.x = curve_tangent.x * cos - curve_tangent.z * sin;
    _ori.y = curve_tangent.y;
    _ori.z = curve_tangent.x * sin + curve_tangent.z * cos;

    if ( t > 1 && is_finish_continue_ )
    {
        VECTOR3 move_delta;
        vc3_norm( move_delta, _ori, passed_len - total_len );
        _pos = _pos + move_delta;
    }

    return true;
}


void TrackMove::init( lua_State* _state, OBJID _missile_ctrl_id )
{
    MissileStage::init( _state, _missile_ctrl_id );
    lua_rawgeti( _state, -1, 3 );
    if( lua_type( _state, -1 ) != LUA_TTABLE )
    {
        ERR(2)( "[TrackMove](init) cfg is not a table!" );
        lua_pop( _state, 1 );
        return;
    }
    
    get_table_number_by_index( _state, -1, 1, turn_speed_ );
    lua_pop( _state, 1 );
}

bool TrackMove::move( VECTOR3& _pos, VECTOR3& _ori )
{
    Missile* missile = (Missile*)g_worldmng->get_ctrl( missile_ctrl_id_ );
    if ( !missile )
    {
        return false;
    }

    Ctrl* target = g_worldmng->get_ctrl( missile->get_target_id() );
    if ( !target )
    {
        return false;
    }

    VECTOR3 missile_ori = missile->get_ori();
    VECTOR3 target_pos = target->getpos();
    target_pos.y = 1;
    VECTOR3 target_dir = target_pos - missile->getpos();
    vc3_norm( missile_ori, missile_ori );
    vc3_norm( target_dir, target_dir );

    float dot = vc3_dot( missile_ori, target_dir );
    if ( dot > 1.0f )
    {
        dot = 1.0f;
    }
    else if ( dot < -1.0f )
    {
        dot = -1.0f;
    }

    float diff_angle = d3d_to_degree( acos( dot ) );
    float turn_angle = turn_speed_ / g_timermng->get_frame_rate();
    if ( diff_angle <= turn_angle )
    {
        _ori = target_dir;
    }
    else if( abs( diff_angle - 180 ) < 0.001f )
    {
        _ori = target_dir * -1;
    }
    else
    {
        VECTOR3 normal;
        vc3_cross( normal, missile_ori, target_dir );
        vc3_cross( normal, normal, missile_ori );
        _ori = missile_ori * COS( turn_angle ) + normal * SIN( turn_angle );
    }

    float passed_len = speed_ / g_timermng->get_frame_rate();
    vc3_norm( _ori, _ori, passed_len );
    _pos = missile->getpos() + _ori;

    return true;
}

bool TrackMove::is_almost_finished()
{
    return false;
}

bool TrackMove::is_finished()
{
    Missile* missile = (Missile*)g_worldmng->get_ctrl( missile_ctrl_id_ );
    if ( !missile )
    {
        return true;
    }

    Ctrl* target = g_worldmng->get_ctrl( missile->get_target_id() );
    if ( !target )
    {
        return true;
    }
    return false;
}


void StayMove::init( lua_State* _state, OBJID _missile_ctrl_id )
{
    MissileStage::init( _state, _missile_ctrl_id );
    lua_rawgeti( _state, -1, 3 );
    if( lua_type( _state, -1 ) != LUA_TTABLE )
    {
        ERR(2)( "[StayMove](init) cfg is not a table!" );
        lua_pop( _state, 1 );
        return;
    }
    
    get_table_number_by_index( _state, -1, 1, turn_speed_ );
    lua_pop( _state, 1 );
}

bool StayMove::move( VECTOR3& _pos, VECTOR3& _ori )
{
    return move_by_frames( _pos, _ori, g_frame - start_frame_ );
}

bool StayMove::move_by_frames( VECTOR3& _pos, VECTOR3& _ori, int _frame_count )
{
    float turn_angle = turn_speed_ * _frame_count / g_timermng->get_frame_rate();
    _pos = start_pos_;
    _ori = GET_DIR( start_angle_y_ + turn_angle );
    return true;
}

