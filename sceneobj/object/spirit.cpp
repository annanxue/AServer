#include "spirit.h"
#include "player_mng.h"
#include "world_mng.h"
#include "define_func.h"
#include "resource.h"
#include "timer.h"
#include "state.h"

#define MAX_SFXTIME		30

Spirit::Spirit()
    :camp_(0)
{
	settype( OT_SPRITE );
	speed_ = DEFAULT_SPEED;
    v_delta_ = NULL_POS;
    state.set_obj(this);
    mk_pos_succ_ = false;
    INIT_LIST_HEAD(&nav_points_.link_);
    stealth_ = false;
}

Spirit::~Spirit()
{
    clear_nav_points();
}

void Spirit::process_parallel()
{
    v_delta_.x = v_delta_.z = 0.0f;
    TICK(A);
    state.process();
    mk_pos_succ_ = mk_pos();
    TICK(B);
    mark_tick(TICK_FSM, A, B);
    // trace pos for debug
    g_playermng->trace_pos_to_all_players( this );
    g_playermng->trace_state( this );
}

void Spirit::set_speed( float _speed )
{ 
	speed_ = _speed; 
}

bool Spirit::mk_pos()
{
    VECTOR3 pos = getpos() + v_delta_;
    return setpos( pos );
}

void Spirit::clear_nav_points()
{
    while ( !list_empty( &nav_points_.link_ ) ){
        pop_nav_points();
    }
}

void Spirit::add_nav_points( NavPoints* _point )
{
    list_add_tail( &_point->link_, &nav_points_.link_ );
}

void Spirit::get_nav_points( float& _x, float& _z )
{
    if (list_empty( &nav_points_.link_ ))
        return ;

    NavPoints* point = list_entry(nav_points_.link_.next, NavPoints, link_);
    _x = point->x_;
    _z = point->z_;
}

bool Spirit::is_nav_points_empty()
{
    return list_empty( &nav_points_.link_ );
}


void Spirit::pop_nav_points()
{
    if (list_empty( &nav_points_.link_ ))
        return ;

    struct list_head * first_node = nav_points_.link_.next;
    NavPoints* point = list_entry( first_node, NavPoints, link_ );
    list_del( first_node );       
    SAFE_DELETE( point );
}


/******************************************************************************
for lua
******************************************************************************/

const char Spirit::className[] = "Spirit";
const bool Spirit::gc_delete_ = false;

Lunar<Spirit>::RegType Spirit::methods[] =
{	
	LUNAR_OBJ_METHODS,
	LUNAR_CTRL_METHODS,
	LUNAR_SPIRIT_METHODS,
	{NULL, NULL}
};

int	Spirit::c_set_speed( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	float speed = (float)lua_tonumber( _state, 1 );
    if( speed < 0 ){
        speed = 0;
    }
	set_speed( speed );
	return 0;
}

int Spirit::c_get_speed( lua_State* _state )
{
    lcheck_argc( _state, 0 );
    lua_pushnumber( _state, get_speed() );
    return 1;
}

int Spirit::send_state_msg( int msg, int p0, int p1, int p2, int p3 )
{
    return state.send_msg(msg, p0, p1, p2, p3);
}

int Spirit::post_state_msg( int msg, int p0, int p1, int p2, int p3 )
{
    return state.post_msg(msg, p0, p1, p2, p3);
}

int Spirit::c_send_state_msg( lua_State *_state )
{
    lcheck_argc(_state, 5);
    int msg, p0,p1,p2, p3;
    msg =lua_tointeger(_state, 1);
    p0 = lua_tointeger(_state, 2);
    p1 = lua_tointeger(_state, 3);
    p2 = lua_tointeger(_state, 4);
    p3 = lua_tointeger(_state, 5);

    int rt = state.send_msg(msg, p0, p1, p2, p3);
	lua_pushinteger(_state, rt);
    return 1;
}

int Spirit::c_post_state_msg( lua_State *_state )
{
    lcheck_argc(_state, 5);
    int msg, p0,p1,p2, p3;
    msg =lua_tointeger(_state, 1);
    p0 = lua_tointeger(_state, 2);
    p1 = lua_tointeger(_state, 3);
    p2 = lua_tointeger(_state, 4);
    p3 = lua_tointeger(_state, 5);

    int rt = state.post_msg(msg, p0, p1, p2, p3);
	lua_pushinteger(_state, rt);
    return 1;
}

int Spirit::c_send_state_fmt_msg( lua_State *_state )
{
    lcheck_argc(_state, 6);
    int msg, p0, p1, p2, p3;
    msg =lua_tointeger(_state, 1);
	const char* format_str = lua_tostring( _state, 2 );
    if ( format_str==NULL ) {
        ERR(2)("c_send_state_fmt_msg() error, format_str is NULL !!!");
        return 0;
    }
    int len = strlen( format_str );
    if( len != 4 ) {
        ERR(2)("c_send_state_fmt_msg() error, len[%d] != narg-1[%d]", len, 4);
        return 0;
    }

    int index = 0;
    p0 = format_str[index++] == 'f' ? (lua_tointeger(_state, 3) * FLOAT2INT_MUL_FACTOR) : lua_tointeger(_state, 3);
    p1 = format_str[index++] == 'f' ? (lua_tointeger(_state, 4) * FLOAT2INT_MUL_FACTOR) : lua_tointeger(_state, 4);
    p2 = format_str[index++] == 'f' ? (lua_tointeger(_state, 5) * FLOAT2INT_MUL_FACTOR) : lua_tointeger(_state, 5);
    p3 = format_str[index++] == 'f' ? (lua_tointeger(_state, 6) * FLOAT2INT_MUL_FACTOR) : lua_tointeger(_state, 6);

    int rt = state.send_msg(msg, p0, p1, p2, p3);
	lua_pushinteger(_state, rt);
    return 1;
}

int Spirit::c_post_state_fmt_msg( lua_State *_state )
{
    lcheck_argc(_state, 6);
    int msg, p0,p1,p2, p3;
    msg = lua_tointeger(_state, 1);
	const char* format_str = lua_tostring( _state, 2 );
    if ( format_str==NULL ) {
        ERR(2)("c_post_state_fmt_msg() error, format_str is NULL !!!");
        return 0;
    }
    int len = strlen( format_str );
    if( len != 4 ) {
        ERR(2)("c_post_state_fmt_msg() error, len[%d] != narg-1[%d]", len, 4);
        return 0;
    }

    int index = 0;
    p0 = format_str[index++] == 'f' ? (lua_tointeger(_state, 3) * FLOAT2INT_MUL_FACTOR) : lua_tointeger(_state, 3);
    p1 = format_str[index++] == 'f' ? (lua_tointeger(_state, 4) * FLOAT2INT_MUL_FACTOR) : lua_tointeger(_state, 4);
    p2 = format_str[index++] == 'f' ? (lua_tointeger(_state, 5) * FLOAT2INT_MUL_FACTOR) : lua_tointeger(_state, 5);
    p3 = format_str[index++] == 'f' ? (lua_tointeger(_state, 6) * FLOAT2INT_MUL_FACTOR) : lua_tointeger(_state, 6);

    int rt = state.post_msg(msg, p0, p1, p2, p3);
	lua_pushinteger(_state, rt);
    return 1;
}


int Spirit::c_is_state( lua_State *_state )
{
    lcheck_argc(_state, 1);
    unsigned int index = (unsigned int)lua_tonumber(_state, 1);
    int rt = state.is_state( index );
	lua_pushnumber(_state, rt);
    return 1;
}

int Spirit::c_get_states( lua_State *_state )
{
    lcheck_argc( _state, 0 );
    lua_newtable( _state );
    for ( int i = 0; i < MAX_BITSET; i++ )
    {
        lua_pushnumber( _state, i + 1 );
        lua_pushnumber( _state, state.run_set[i] );
        lua_settable( _state, -3 );
    }
    return 1;
}

int Spirit::c_get_states_in_post( lua_State *_state )
{
    lcheck_argc( _state, 0 );
    lua_newtable( _state );

    int head = state.qhead;
    int tail = state.qtail;
    int index = 1;

    while(head < tail) 
    {
        int idx = head % MAX_MESSAGE;
        int post_state = state.msg_queue[idx][0];
        post_state = (post_state >> 8) & 0x0000FFFF;
        lua_pushnumber( _state, index );
        lua_pushnumber( _state, post_state );
        lua_settable( _state, -3 );
        head++;
        index++;
    }
    return 1;
}

int Spirit::c_is_state_in_post( lua_State *_state )
{
    lcheck_argc(_state, 1);
    int index = lua_tonumber(_state, 1);
    bool rt = state.is_state_in_post( index );
	lua_pushboolean(_state, rt);
    return 1;
}


int Spirit::c_can_be_state( lua_State *_state )
{
    lcheck_argc(_state, 1);
    unsigned int index = (unsigned int)lua_tonumber(_state, 1);
    int rt = state.can_be_state(index);
    lua_pushnumber(_state, rt);
    return 1;
}

int Spirit::c_can_be_state_late( lua_State *_state )
{
    lcheck_argc(_state, 1);
    unsigned int index = (unsigned int)lua_tonumber(_state, 1);
    bool rt = state.can_be_state_late(index);
    lua_pushboolean(_state, rt);
    return 1;
}

int Spirit::c_serialize_state( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	LAr* lar = Lunar<LAr>::check( _state, 1 );
    state.serialize( *(lar->ar_));
	return 0;
}

int Spirit::c_set_camp( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    int camp = lua_tointeger( _state, 1 );
    set_camp( camp );
    return 0;
}

int Spirit::c_get_camp( lua_State* _state )
{
    lua_pushnumber( _state, get_camp() );
    return 1;
}

int Spirit::c_del_state( lua_State *_state )
{
	lcheck_argc( _state, 1 );
    int index = lua_tointeger( _state, 1 );
    state.del_state( index );
    return 0;
}

int Spirit::c_del_state_safe( lua_State *_state )
{
	lcheck_argc( _state, 1 );
    int index = lua_tointeger( _state, 1 );
    state.del_state( index );
    state.del_state_from_msg_queue( index );
    return 0;
}

int Spirit::c_del_all_state( lua_State *_state )
{
    lcheck_argc( _state, 0 );
    for ( int i = 0; i < STATE_MAX; i++ )
    {
        state.del_state( i );
    }
    state.clear_msg_queue();
    return 0;
}

int Spirit::c_get_param( lua_State *_state )
{
    lcheck_argc(_state, 2);
    int s = (int)lua_tonumber(_state, 1);
    int i = (int)lua_tonumber(_state, 2);

    int data = state.get_param(s, i);
    lua_pushnumber(_state, data);
    return 1;
}

int Spirit::c_set_param( lua_State *_state )
{
    lcheck_argc(_state, 3);
    int s = (int)lua_tonumber(_state, 1);
    int i = (int)lua_tonumber(_state, 2);
    int d = (int)lua_tonumber(_state, 3);
    state.set_param(s, i, d);
    return 0;
}

int Spirit::c_is_in_skill_unbreakable( lua_State* _state )
{
    int ret = 0;
    if (state.is_state( STATE_SKILLING )){
        int frame_index = state.get_param( STATE_SKILLING, SKILL_PARAM_FRAME_COUNTER ); 
        if ( state.is_state( STATE_RUSH ) )
        {
            int bs_time = state.get_param( STATE_RUSH, RUSH_PARAM_CAST_BS_TIME );
            ret = frame_index < g_timermng->conv_ms_2_frame( bs_time );
        }
        else
        {
            int hj_time = state.get_param( STATE_SKILLING, SKILL_PARAM_HJ_TIME );
            int bs_time = state.get_param( STATE_SKILLING, SKILL_PARAM_BS_TIME );
            ret = ( frame_index > g_timermng->conv_ms_2_frame( hj_time ) )
                && ( frame_index < g_timermng->conv_ms_2_frame( bs_time ));
        }
    }

    lcheck_argc(_state, 0);
    lua_pushboolean(_state, ret);
    return 1;
}

int Spirit::c_store_state_param( lua_State* _state )
{
    int argc = lua_gettop( _state );
    if ( argc > MAX_PARAM + 1 || argc == 1 )
    {
        ERR(2)( "fatal error: invalid param num, param_num:%d", argc );
        lua_settop( _state, 0 );
        return 0;
    }
    int index = lua_tointeger(_state, 1); 
    for (int i = 2; i <= argc; i++) {
        state.set_param(index, i - 2, lua_tointeger(_state, i));
    }
    return 0;

}

int Spirit::c_store_state_param_from( lua_State* _state )
{
    int argc = lua_gettop( _state );
    if ( argc <= 2 )
    {
        ERR(2)( "fatal error: invalid param num, param_num:%d", argc );
        lua_settop( _state, 0 );
        return 0;
    }
    int from = lua_tonumber( _state, 2 );
    if( (from + argc - 2) > MAX_PARAM )
    {
        ERR(2)( "fatal error: invalid param num, param_num:%d", argc );
        lua_settop( _state, 0 );
        return 0;
    }
    int index = (int)lua_tonumber(_state, 1); 
    for (int i = 3; i <= argc; i++) {
        state.set_param(index, i - 3, (int)lua_tonumber(_state, i));
    }
    return 0;
}

int Spirit::c_set_nav_points( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	LAr* lar = Lunar<LAr>::check( _state, 1 );

    clear_nav_points();

	if( lar == 0 ) return 0;
    if( lar->flush_flag_ != 0 ) {
		ERR(2)( "[OBJ](player) %s:%d flush flag set, this is very dangerous!!!! pls concat: sodme !!!! ", __FILE__, __LINE__ );
		return 0;
    }

    Ar* ar = lar->ar_;
	unsigned char point_num; 
    (*ar) >> point_num;

    if (point_num == 0)
        return 0;

    float x = 0;
    float z = 0;
    for (int i = 0; i < point_num; i ++)
    {
        NavPoints* point = new NavPoints;
        (*ar) >> x >> z;
        point->x_ = x;
        point->z_ = z;
        add_nav_points( point );
    }

    lua_pushnumber( _state, x );
    lua_pushnumber( _state, z );
	return 2;
}

int Spirit::c_clear_nav_points( lua_State* _state )
{
	lcheck_argc( _state, 0 );
    clear_nav_points();
    return 0;
}

int Spirit::c_add_nav_points( lua_State* _state )
{
	lcheck_argc( _state, 2 );
    NavPoints* point = new NavPoints;
    point->x_ = (float)lua_tonumber(_state, 1);
    point->z_ = (float)lua_tonumber(_state, 2);
    add_nav_points( point );
    return 0;
}

int Spirit::c_pop_nav_points( lua_State* _state )
{
	lcheck_argc( _state, 0 );
    pop_nav_points();
    return 0;
}

int Spirit::c_is_nav_points_empty( lua_State* _state )
{
	lcheck_argc( _state, 0 );
    lua_pushboolean( _state, is_nav_points_empty() );
    return 1;
}

int Spirit::c_is_stealth( lua_State* _state )
{
	lcheck_argc( _state, 0 );
    lua_pushboolean( _state, is_stealth() );
    return 1;
}

int Spirit::c_set_stealth( lua_State* _state )
{

    lcheck_argc( _state, 1 );
	stealth_ = lua_toboolean( _state, 1 );
	return 0;
}

