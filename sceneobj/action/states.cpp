#include <string.h>
#include "states.h"
#include "state.h"
#include "player_mng.h"
#include "world_mng.h"
#include "timer.h"
#include "3dmath.h"
#include "log.h"
#include "define_func.h"
#include "bullet.h"
#include "camp_mng.h"

// 0 deny, 1 swap, 2 together 3 delay                                                      
static int table[STATE_MAX][STATE_MAX] = {
      //  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25
      {   1,  1,  0,  1,  1,  3,  1,  1,  3,  0,  3,  1,  3,  2,  0,  3,  2,  0,  1,  3,  1,  0,  0,  0,  0,  0  },      //  0   STATE_STAND
      {   1,  1,  0,  1,  2,  0,  1,  1,  0,  0,  0,  2,  0,  0,  0,  0,  2,  0,  0,  0,  1,  0,  0,  0,  0,  0  },      //  1   STATE_MOVE_TO     
      {   1,  1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  3,  2,  2,  1,  1,  1,  3,  1,  1,  1,  1  },      //  2   STATE_DEAD
      {   1,  1,  0,  1,  2,  0,  1,  1,  0,  0,  0,  1,  0,  2,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0  },      //  3   STATE_RUSH     
      {   1,  1,  0,  1,  1,  0,  1,  1,  0,  0,  0,  1,  0,  2,  0,  0,  2,  0,  0,  0,  1,  0,  0,  0,  0,  0  },      //  4   STATE_SKILLING    
      {   1,  1,  0,  1,  1,  1,  1,  1,  0,  0,  1,  0,  0,  2,  0,  0,  2,  0,  0,  0,  1,  0,  0,  0,  0,  0  },      //  5   STATE_HURT
      {   1,  1,  0,  1,  2,  0,  1,  1,  0,  0,  0,  2,  0,  0,  0,  0,  2,  0,  0,  0,  1,  0,  0,  0,  0,  0  },      //  6   STATE_MOVE_GROUND 
      {   1,  1,  0,  1,  1,  0,  1,  1,  3,  0,  0,  0,  3,  0,  0,  3,  0,  0,  0,  3,  1,  0,  0,  0,  0,  0  },      //  7   STATE_NAVIGATION
      {   1,  1,  0,  1,  1,  1,  1,  1,  1,  2,  1,  0,  0,  2,  0,  1,  0,  0,  0,  0,  1,  1,  0,  0,  2,  0  },      //  8   STATE_HURT_BACK
      {   1,  1,  0,  3,  1,  1,  1,  1,  2,  1,  2,  1,  2,  2,  0,  2,  2,  0,  1,  2,  1,  2,  2,  1,  1,  0  },      //  9   STATE_DAZED
      {   1,  1,  0,  0,  1,  1,  1,  1,  0,  2,  1,  0,  0,  2,  0,  0,  2,  0,  1,  0,  1,  1,  0,  0,  0,  0  },      //  10  STATE_HURT_FLY
      {   1,  1,  0,  1,  2,  0,  1,  1,  0,  0,  0,  1,  0,  2,  0,  0,  2,  0,  0,  0,  1,  0,  0,  0,  0,  0  },      //  11  STATE_MOVE_PERSIST
      {   1,  1,  0,  1,  1,  1,  1,  1,  1,  2,  1,  0,  1,  2,  0,  0,  0,  0,  1,  0,  1,  1,  0,  0,  0,  0  },      //  12  STATE_HURT_BACK_FLY
      {   2,  1,  0,  2,  2,  2,  1,  1,  2,  2,  2,  2,  2,  1,  0,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  0  },      //  13  STATE_HOLD
	  {   1,  1,  0,  0,  1,  0,  1,  1,  0,  0,  0,  1,  0,  2,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  1  },      //  14  STATE_CINEMA
	  {   1,  1,  0,  1,  1,  1,  1,  1,  1,  2,  1,  0,  0,  2,  0,  1,  0,  0,  1,  0,  1,  1,  0,  0,  2,  0  },      //  15  STATE_HURT_HORI
	  {   2,  2,  0,  0,  2,  2,  2,  1,  0,  2,  2,  2,  0,  2,  0,  0,  0,  0,  1,  0,  1,  2,  0,  0,  2,  0  },      //  16  STATE_DRAG
      {   1,  1,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1  },      //  17  STATE_REPLACE
	  {   1,  1,  0,  0,  2,  0,  1,  1,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0  },      //  18  STATE_LEADING
      {   1,  1,  0,  0,  1,  1,  1,  1,  1,  2,  1,  0,  0,  2,  0,  1,  0,  0,  0,  0,  1,  2,  0,  1,  2,  0  },      //  19  STATE_PULL
      {   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  },      //  20  STATE_PICK
      {   1,  1,  0,  1,  1,  1,  1,  1,  2,  2,  1,  0,  2,  2,  0,  0,  2,  0,  0,  2,  1,  1,  0,  1,  2,  0  },      //  21  STATE_HURT_FLOAT
      {   1,  1,  0,  3,  1,  1,  1,  1,  1,  2,  1,  1,  1,  2,  0,  1,  2,  0,  1,  3,  1,  1,  3,  1,  2,  0  },      //  22  STATE_FREEZE
      {   1,  1,  0,  3,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  0,  1,  1,  0,  1,  3,  1,  1,  0,  0,  1,  0  },      //  23  STATE_FEAR
      {   1,  1,  0,  3,  1,  1,  1,  1,  2,  1,  2,  1,  2,  2,  0,  2,  2,  0,  1,  2,  1,  2,  2,  1,  1,  0  },      //  24  STATE_DANCE
      {   1,  1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  0,  1,  1,  1,  1,  1,  1,  1,  0  },      //  25  STATE_CAUGHT
};

#define OBJ ((Spirit*)(state->get_obj()))
#define CLIENT ((Player*)(state->get_obj()))


int state_sample( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            break;
        case MSG_UPDATE:
            break; 
        case MSG_DESTROY:
            break;
        default:
            break;
    }
    return RT_OK;
}

static void state_move_persist_pre( State* state, int index )
{
    int pre_time = state->get_param( index, MOVE_PERSIST_PARAM_PRE_TIME );
    pre_time -= g_timermng->get_ms_one_frame();
    if( pre_time <= 0 )
    {
        state->set_param( index, MOVE_PERSIST_PARAM_STAGE, 0 );
    }
    else
    {
        state->set_param( index, MOVE_PERSIST_PARAM_PRE_TIME, pre_time );
    }
}

static void state_move_persist_process( State* state, int index )
{
    int frame_counter = state->get_param( index, MOVE_PERSIST_PARAM_FRAME_COUNTER );
    //int skill_id      = state->get_param( index, MOVE_PERSIST_PARAM_SKILL_ID );
    int period        = state->get_param( index, MOVE_PERSIST_PARAM_PERIOD );
    int total_time    = state->get_param( index, MOVE_PERSIST_PARAM_TOTAL_TIME );
    if ( frame_counter % period == 0 )
    {
        int skill_id = state->get_param( index, MOVE_PERSIST_PARAM_SKILL_ID );
        OBJ->to_lua( STATE_MOVE_PERSIST, MSG_HIT, skill_id, 0 ); 
    }
    total_time -= g_timermng->get_ms_one_frame();
    state->set_param( index, MOVE_PERSIST_PARAM_TOTAL_TIME, total_time );
    if( total_time < 0 )
    {
        int skill_id = state->get_param( index, MOVE_PERSIST_PARAM_SKILL_ID );
        OBJ->to_lua( STATE_MOVE_PERSIST, MSG_HIT, skill_id, 1 ); 
    }
    ++frame_counter;
    state->set_param( index, MOVE_PERSIST_PARAM_FRAME_COUNTER, frame_counter );
}

static void state_move_persist_cast( State* state, int index )
{
    int cast_time = state->get_param( index, MOVE_PERSIST_PARAM_CAST_TIME );
    cast_time -= g_timermng->get_ms_one_frame();
    if( cast_time <= 0 )
    {
        state->del_state( index );
        state->del_state( STATE_SKILLING );
    }
    else
    {
        state->set_param( index, MOVE_PERSIST_PARAM_CAST_TIME, cast_time );
    }
}

int state_move_persist( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
                state->set_param( index, MOVE_PERSIST_PARAM_SKILL_ID, p0 );
                state->set_param( index, MOVE_PERSIST_PARAM_PERIOD, p2 );
                state->set_param( index, MOVE_PERSIST_PARAM_CAST_TIME, p3 );
                state->set_param( index, MOVE_PERSIST_PARAM_STAGE, 2 );
                state->set_param( index, MOVE_PERSIST_PARAM_HIT_COUNT, 1 );
                state->set_param( index, MOVE_PERSIST_PARAM_FRAME_COUNTER, 0 );
		        TRACE (2) ( "[states](state_move_persist) initial, ctrl_id: %d",  OBJ->get_id() );
            }
            break;
        case MSG_UPDATE:
            {
                switch( state->get_param( index, MOVE_PERSIST_PARAM_STAGE ) )
                {
                    case 2:
                        state_move_persist_pre( state, index );
                        break;
                    case 0:
                        state_move_persist_process( state, index );
                        break;
                    case 1:
                        state_move_persist_cast( state, index );
                        break;
                    default:
                        break;
                }
            }
            break; 
        case MSG_DESTROY:
            {
                int skill_id = state->get_param( index, MOVE_PERSIST_PARAM_SKILL_ID );
                OBJ->to_lua( STATE_MOVE_PERSIST, MSG_DESTROY, skill_id );
		        TRACE (2) ( "[states](state_move_persist) destroy, ctrl_id: %d",  OBJ->get_id() );
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

static int state_dead_move( State *state, int index )
{
    VECTOR3 cur_pos = OBJ->getpos();
    float speed = state->get_param(index, 5) * INT2FLOAT_MUL_FACTOR;    //speed
    float range = state->get_param(index, 6) * INT2FLOAT_MUL_FACTOR;    //range

    int dead_type = state->get_param(index, 0);
    if (dead_type == 2) // DTYPE_FLY
    {
        int fly_time = state->get_param(index, 3);
        int frame_counter = state->get_param(index, 4);                           
        int fly_frame = ceil( fly_time * 0.001 * g_timermng->get_frame_rate() );  
        state->set_param(index, 4, ++frame_counter);                              
        speed = (2*fly_frame - 2*frame_counter + 1) * speed / fly_frame;
    }

    float sx = state->get_param(index, 7) * INT2FLOAT_MUL_FACTOR;       //startx
    float sz = state->get_param(index, 8) * INT2FLOAT_MUL_FACTOR;       //startz
    VECTOR3 start_pos( sx, 0, sz );

    float endx = state->get_param(index, 9) * INT2FLOAT_MUL_FACTOR;     //end x
    float endz = state->get_param(index, 10) * INT2FLOAT_MUL_FACTOR;    //end z
    VECTOR3 end_pos( endx, 0, endz );

    float degree = get_degree( end_pos, start_pos ); 
    VECTOR3 delta = GET_DIR( degree ); 
    vc3_mul(delta, delta, speed);
    vc3_mul(delta, delta, 1.0/g_timermng->get_frame_rate());
    VECTOR3 next_pos = cur_pos + delta;

    if( speed <= 0 || vc3_check_out_range( next_pos, start_pos, range ) )
    {
        VECTOR3 end_pos = start_pos + GET_DIR(degree) * range;
        delta = end_pos - cur_pos;
        OBJ->v_delta_.x = delta.x;
        OBJ->v_delta_.z = delta.z;
        state->set_param(index, 0, 1);  //dead_type
    }
    else
    {
        OBJ->v_delta_.x = delta.x;
        OBJ->v_delta_.z = delta.z;
    }
    return 0;
}

static int state_dead_revive(State *state, int index)
{
	int hold_time = state->get_param(index, 2);
	if (hold_time >= 0)
	{
		hold_time -= g_timermng->get_ms_one_frame();
		state->set_param(index, 2, hold_time);
		if (hold_time <= 0)
		{
			OBJ->to_lua(index, MSG_UPDATE, 10);
		}
		else return 0;
	}

	int act_time = state->get_param(index, 3);
	act_time -= g_timermng->get_ms_one_frame();
	if (act_time > 0)
	{
		state->set_param(index, 3, act_time);
	}
	else
	{
		state->del_state(index);
	}
	return 0;
}

int state_dead( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) 
    {
        case MSG_INITIAL:
            {
                VECTOR3 start_pos = OBJ->getpos();
                VECTOR3 hit_pos;
                float hit_rate;
                float range = state->get_param(index, 6) * INT2FLOAT_MUL_FACTOR;    //range
                if (p3 != 0)                                                         //p3 is hit_out_type
                {
                    bool ignore_block_val = OBJ->get_ignore_block();
                    if (!ignore_block_val)
                    {
                        OBJ->set_ignore_block(true);
                    }
                    OBJ->raycast_by_range(-range, &hit_pos, &hit_rate);
                    OBJ->set_ignore_block(ignore_block_val);
                }
                else
                {
                    OBJ->raycast_by_range(-range, &hit_pos, &hit_rate);
                }
                range *= hit_rate;

                state->set_param(index, 6, range*FLOAT2INT_MUL_FACTOR);             //range
                state->set_param(index, 7, start_pos.x*FLOAT2INT_MUL_FACTOR);       //startx
                state->set_param(index, 8, start_pos.z*FLOAT2INT_MUL_FACTOR);       //startz 
                state->set_param(index, 9, hit_pos.x*FLOAT2INT_MUL_FACTOR);         //targetx
                state->set_param(index, 10, hit_pos.z*FLOAT2INT_MUL_FACTOR);        //targetz
                state->set_param(index, 11, p3);                                    //hit_out_type

                // dead_type, atkid, skill_id
                OBJ->to_lua(STATE_DEAD, MSG_INITIAL, p0, p1, p2);
            }
            break;
        case MSG_UPDATE:
            {
                switch( state->get_param(index, 0) )    //dead_type
                {
                    case 1:
                        break;
                    case 2:
                        state_dead_move( state, index );
                        break;
                    case 3: 
                        state_dead_move( state, index );
                        break;
                    case 4:
                        break;
					case 10:	// revive
						state_dead_revive( state, index );
						break;
                    default:
                        break;
                }
            }
            break; 
        case MSG_DESTROY:
			{
				int tp = state->get_param(index, 0);
				OBJ->to_lua(STATE_DEAD, MSG_DESTROY, tp);
				break;
			}
        default:
            break;
    }
    return RT_OK;
}

//htype, hurt_time, skill_id, atkid
int state_hurt( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
                state->set_param(index, 2, 0);      //frame counter
				OBJ->to_lua(STATE_HURT, MSG_INITIAL, p0, p2, p3);
            }
            break;
        case MSG_UPDATE:
            {
                int end_time = state->get_param(index,1);
                int frame_index =  state->get_param(index, 2); 
                state->set_param(index, 2, ++frame_index); 
                if( frame_index >= g_timermng->conv_ms_2_frame( end_time ) ) 
                {
                    state->del_state(index);
                }
            }
            break; 
        case MSG_DESTROY:
			if(OBJ->is_monster())
			{
				OBJ->to_lua(STATE_HURT, MSG_DESTROY);
			}
            break;
        default:
            break;
    }
    return RT_OK;
}

static void state_hurt_back_process( State *state, int index )
{
    VECTOR3 cur_pos = OBJ->getpos();
    float speed = state->get_param(index, 3) * INT2FLOAT_MUL_FACTOR;    //speed
    float range = state->get_param(index, 4) * INT2FLOAT_MUL_FACTOR;    //range

    float sx = state->get_param(index, 9) * INT2FLOAT_MUL_FACTOR;  
    float sz = state->get_param(index, 10) * INT2FLOAT_MUL_FACTOR;  
    VECTOR3 start_pos( sx, 0, sz );

    float endx = state->get_param(index, 7) * INT2FLOAT_MUL_FACTOR;     //end x
    float endz = state->get_param(index, 8) * INT2FLOAT_MUL_FACTOR;     //end z
    VECTOR3 end_pos( endx, 0, endz );

    float degree = get_degree( end_pos, start_pos ); 
    VECTOR3 delta = GET_DIR( degree ); 
    vc3_mul(delta, delta, speed);
    vc3_mul(delta, delta, 1.0/g_timermng->get_frame_rate());
    VECTOR3 next_pos = cur_pos + delta;

    if( vc3_check_out_range( next_pos, start_pos, range ) )
    {
        delta = end_pos - cur_pos;
        OBJ->v_delta_.x = delta.x;
        OBJ->v_delta_.z = delta.z;
        state->set_param(index, 11, 1);
    }
    else
    {
        OBJ->v_delta_.x = delta.x;
        OBJ->v_delta_.z = delta.z;
    }
}

static void state_hurt_hori_process( State *state, int index )//1
{
	VECTOR3 cur_pos = OBJ->getpos();
	float speed = state->get_param(index, 3) * INT2FLOAT_MUL_FACTOR;    //speed
	float range = state->get_param(index, 4) * INT2FLOAT_MUL_FACTOR;    //range

	float sx = state->get_param(index, 9) * INT2FLOAT_MUL_FACTOR;  
	float sz = state->get_param(index, 10) * INT2FLOAT_MUL_FACTOR;  
	VECTOR3 start_pos( sx, 0, sz );

	float tx = state->get_param(index, 7) * INT2FLOAT_MUL_FACTOR;     //tar x
	float tz = state->get_param(index, 8) * INT2FLOAT_MUL_FACTOR;     //tar z
	VECTOR3 dest_pos( tx, 0, tz );

	float degree = get_degree( dest_pos, start_pos); 
	VECTOR3 delta = GET_DIR( degree ); 
	vc3_mul(delta, delta, speed);
	vc3_mul(delta, delta, 1.0/g_timermng->get_frame_rate());

	if( vc3_check_out_range( cur_pos, start_pos, range ) )
	{
		VECTOR3 end_pos = start_pos + GET_DIR(degree) * range;
		delta = end_pos - cur_pos;
		OBJ->v_delta_.x = delta.x;
		OBJ->v_delta_.z = delta.z;
		state->del_state( index );
	}
	else
	{
		OBJ->v_delta_.x = delta.x;
		OBJ->v_delta_.z = delta.z;
	}
}

static void state_hurt_back_fly_process( State *state, int index )
{
    VECTOR3 cur_pos = OBJ->getpos();
    float speed = state->get_param(index, 3) * INT2FLOAT_MUL_FACTOR;    //speed
    float range = state->get_param(index, 4) * INT2FLOAT_MUL_FACTOR;    //range

    int fly_time = state->get_param(index, 5);
    int frame_counter = state->get_param(index, 6);
    int fly_frame = ceil( fly_time * 0.001 * g_timermng->get_frame_rate() );
    state->set_param(index, 6, ++frame_counter);
    float cur_speed = (2*fly_frame - 2*frame_counter + 1) * speed / fly_frame;

    float sx = state->get_param(index, 9) * INT2FLOAT_MUL_FACTOR;  
    float sz = state->get_param(index, 10) * INT2FLOAT_MUL_FACTOR;  
    VECTOR3 start_pos( sx, 0, sz );

    float endx = state->get_param(index, 7) * INT2FLOAT_MUL_FACTOR;     //end x
    float endz = state->get_param(index, 8) * INT2FLOAT_MUL_FACTOR;     //end z
    VECTOR3 end_pos( endx, 0, endz );

    float degree = get_degree( end_pos, start_pos ); 
    VECTOR3 delta = GET_DIR( degree ); 
    vc3_mul(delta, delta, cur_speed);
    vc3_mul(delta, delta, 1.0/g_timermng->get_frame_rate());
    VECTOR3 next_pos = cur_pos + delta;

    if( cur_speed <= 0 || vc3_check_out_range( next_pos, start_pos, range ) )
    {
        VECTOR3 end_pos = start_pos + GET_DIR(degree) * range;
        delta = end_pos - cur_pos;
        OBJ->v_delta_.x = delta.x;
        OBJ->v_delta_.z = delta.z;
        state->set_param(index, 11, 2);
    }
    else
    {
        OBJ->v_delta_.x = delta.x;
        OBJ->v_delta_.z = delta.z;
    }
}

static void state_hurt_back_fly_pre( State *state, int index )
{
    int pre_time = state->get_param(index, 1);
    pre_time -= g_timermng->get_ms_one_frame();
    if( pre_time <= 0 )
    {
        state->set_param(index, 11, 1);
    }
    else
    {
        state->set_param(index, 1, pre_time);
    }
}

static void state_hurt_back_cast( State *state, int index )
{
    int cast_time = state->get_param(index, 1);
    cast_time -= g_timermng->get_ms_one_frame();
    if( cast_time <= 0 )
    {
        state->del_state( index );
    }
    else
    {
        state->set_param(index, 1, cast_time);
    }
}

static void state_hurt_back_fly_cast( State *state, int index )
{
    int cast_time = state->get_param(index, 2);
    cast_time -= g_timermng->get_ms_one_frame();
    if( cast_time <= 0 )
    {
        state->del_state( index );
    }
    else
    {
        state->set_param(index, 2, cast_time);
    }
}

//htype, hurt_time, skill_id, atkid
int state_hurt_back( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
                VECTOR3 cur_pos = OBJ->getpos();
                state->set_param(index, 9, cur_pos.x*FLOAT2INT_MUL_FACTOR);         //startx
                state->set_param(index, 10, cur_pos.z*FLOAT2INT_MUL_FACTOR);        //startz 
                state->set_param(index, 11, 0);
				OBJ->to_lua(STATE_HURT_BACK, MSG_INITIAL, p0, p2, p3);
            }
            break;
        case MSG_UPDATE:
            {
                switch( state->get_param(index, 11) )
                {
                    case 0:
                        state_hurt_back_process( state, index );
                        break;
                    case 1:
                        state_hurt_back_cast( state, index );
                        break;
                    default:
                        break;
                }
            }
            break; 
        case MSG_DESTROY:
			if(OBJ->is_monster())
			{
				OBJ->to_lua(STATE_HURT_BACK, MSG_DESTROY);
			}
            break;
        default:
            break;
    }
    return RT_OK;
}

//htype, hurt_time, skill_id, atkid
int state_hurt_back_fly( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) 
    {
        case MSG_INITIAL:
        {
            VECTOR3 cur_pos = OBJ->getpos();
            state->set_param(index, 9, cur_pos.x*FLOAT2INT_MUL_FACTOR);         //startx
            state->set_param(index, 10, cur_pos.z*FLOAT2INT_MUL_FACTOR);        //startz 
            state->set_param(index, 11, 1);
			OBJ->to_lua(STATE_HURT_BACK_FLY, MSG_INITIAL, p0, p2, p3);
            break;
        }
        case MSG_UPDATE:
        {
            switch( state->get_param(index, 11) )
            {
                case 0:
                    state_hurt_back_fly_pre( state, index );
                    break;
                case 1:
                    state_hurt_back_fly_process( state, index );
                    break;
                case 2:
                    state_hurt_back_fly_cast( state, index );
                    break;
                default:
                    break;
            }
            break; 
        }
        case MSG_DESTROY:
            if(OBJ->is_monster())
            {
                OBJ->to_lua(STATE_HURT_BACK_FLY, MSG_DESTROY);
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

//p0 - p3: htype, hurt_time, skill_id, atkid
//
int state_hurt_fly( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
                state->set_param(index, 2, 0);      //frame counter
				OBJ->to_lua(STATE_HURT_FLY, MSG_INITIAL, p0, p2, p3);
            }
            break;
        case MSG_UPDATE:
            {
                int end_time = state->get_param(index,1);
                int frame_index =  state->get_param(index, 2); 
                state->set_param(index, 2, ++frame_index); 
                if( frame_index >= g_timermng->conv_ms_2_frame( end_time ) ) 
                {
                    state->del_state(index);
                }
            }
            break; 
        case MSG_DESTROY:
			if(OBJ->is_monster())
			{
				OBJ->to_lua(STATE_HURT_FLY, MSG_DESTROY);
			}
            break;
        default:
            break;
    }
    return RT_OK;
}

//htype, hurt_time, skill_id, atkid
int state_hurt_float( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
				OBJ->to_lua(STATE_HURT_FLOAT, MSG_INITIAL, p0, p2, p3);
            }
            break;
        case MSG_UPDATE:
            {
                int left_time = state->get_param( index, 1 );
                int cast_time = state->get_param( index, 2 );
                int bs_dead = state->get_param( index, 4 );
                left_time -= g_timermng->get_ms_one_frame();
                state->set_param( index, 1, left_time );
                if ( left_time <= cast_time )
                {
                    state->set_param( index, 3, 1 );    // bs handled
                    if ( bs_dead == 1 )
                    {
                        state->del_state(index);
                    }
                }
                if ( left_time <= 0 )
                {
                    state->del_state(index);
                }
            }
            break; 
        case MSG_DESTROY:
			if(OBJ->is_monster())
			{
				OBJ->to_lua(STATE_HURT_FLOAT, MSG_DESTROY);
			}
            break;
        default:
            break;
    }
    return RT_OK;
}

//htype, hurt_time, skill_id, atkid
int state_hurt_hori( State *state, int index, int msg, int p0, int p1, int p2, int p3 )//2
{
	switch(msg) {
	case MSG_INITIAL:
		{
			VECTOR3 cur_pos = OBJ->getpos();
			state->set_param(index, 9, cur_pos.x*FLOAT2INT_MUL_FACTOR);         //startx
			state->set_param(index, 10, cur_pos.z*FLOAT2INT_MUL_FACTOR);        //startz 
			OBJ->to_lua(STATE_HURT_HORI, MSG_INITIAL, p0, p2, p3);
		}
		break;
	case MSG_UPDATE:
		{
			state_hurt_hori_process(state, index);
		}
		break; 
	case MSG_DESTROY:
		if(OBJ->is_monster())
		{
			OBJ->to_lua(STATE_HURT_HORI, MSG_DESTROY);
		}
		break;
	default:
		break;
	}
	return RT_OK;
}

int state_skilling( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
                state->set_param(index, SKILL_PARAM_FRAME_COUNTER, 0); //frame counter
                state->set_param(index, SKILL_PARAM_HIT_COUNTER, 0); //hit counter
                int skill_id = state->get_param(index, SKILL_PARAM_SKILL_ID);
                int target_id = state->get_param(index, SKILL_PARAM_TARGET_ID);
                OBJ->to_lua(STATE_SKILLING, MSG_INITIAL, skill_id, 0, target_id);
            }
            break;
        case MSG_UPDATE:
            {
                int frame_index =  state->get_param(index, SKILL_PARAM_FRAME_COUNTER); 
                state->set_param(index, SKILL_PARAM_FRAME_COUNTER, ++frame_index); 
                int hit_index = state->get_param(index, SKILL_PARAM_HIT_COUNTER);
                int hit_time = state->get_param(index, SKILL_PARAM_HIT_TIME);
                int end_time = state->get_param(index, SKILL_PARAM_END_TIME);
                int target_id = state->get_param(index, SKILL_PARAM_TARGET_ID);
                //int anim_section = state->get_param(index, SKILL_PARAM_ANIM_SECTION);
                int skill_id = state->get_param(index, SKILL_PARAM_SKILL_ID);

                //if reach hit time, only support 1 hit
                if ( hit_index == 0 && frame_index >= g_timermng->conv_ms_2_frame( hit_time ) )
                {
                    OBJ->to_lua(STATE_SKILLING, MSG_HIT, skill_id, frame_index, target_id);

                    hit_index++;
                    state->set_param(index, SKILL_PARAM_HIT_COUNTER, hit_index); //hit counter
                }
                else if ( hit_index > 0 && frame_index >= g_timermng->conv_ms_2_frame( end_time ) )
                {
                    if ( !(state->is_state(STATE_RUSH) || state->is_state(STATE_MOVE_PERSIST) || state->is_state(STATE_LEADING)) )
                    {
                        state->del_state(index);
                    }
                }
            }
            break; 
        case MSG_DESTROY:
            {
                int skill_id = state->get_param(index, SKILL_PARAM_SKILL_ID);
                OBJ->to_lua(STATE_SKILLING, MSG_DESTROY, skill_id, 0, 0);
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

static void check_add_search_target( Ctrl* _self, Ctrl* _target, int _apply_type, VECTOR3 _start_pos, VECTOR3 _end_pos, int _skill_id, float _radius )
{
    if( _apply_type == 1 )
    {
        if( !_target->is_player() ) return;
    }
    else if( _apply_type == 2 )
    {
        if( !_target->is_monster() ) return;
    }
    else if( _apply_type == 4 )
    {
        if( !_target->is_attackable_trigger() ) return;
    }

    bool is_enemy = g_campmng->is_enemy( (Spirit*)_self, (Spirit*)_target );
   
    if( is_enemy )
    {
        if( Bullet::get_space().ray_test( _start_pos, _end_pos, _radius, _target ) )
        {
            _self->to_lua(STATE_RUSH, MSG_HIT, _skill_id, _target->get_id());
        }
    }
}

static void search_hit( Ctrl* _obj, VECTOR3 _end_pos, int _skill_id, float _radius, int _apply_type )
{
    if( _skill_id < 0 )
    {
        return;
    }
    VECTOR3 pos = _obj->getpos();
    if( _apply_type & 1 )
    {
        FOR_LINKMAP( _obj->get_world(), _obj->plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkPlayer )
        {
            check_add_search_target( _obj, obj, 1, pos, _end_pos, _skill_id, _radius );
        }
        END_LINKMAP	
        if ( !(_apply_type & 2) )
        {
            FOR_LINKMAP( _obj->get_world(), _obj->plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
            {
                if (_obj->is_robot()) {
                    check_add_search_target( _obj, obj, 1, pos, _end_pos, _skill_id, _radius );
                }
            }
            END_LINKMAP
        }
    }
    bool robot_include = _apply_type & 1;
    if( _apply_type & 2 && _apply_type & 4 )
    {
        FOR_LINKMAP( _obj->get_world(), _obj->plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
        {
            if (robot_include || !obj->is_robot()) {
                check_add_search_target( _obj, obj, 2, pos, _end_pos, _skill_id, _radius );
                check_add_search_target( _obj, obj, 4, pos, _end_pos, _skill_id, _radius );
            }
        }
        END_LINKMAP	
    }
    else if( _apply_type & 2 )
    {
        FOR_LINKMAP( _obj->get_world(), _obj->plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
        {
            if (robot_include || !obj->is_robot()) {
                check_add_search_target( _obj, obj, 2, pos, _end_pos, _skill_id, _radius );
            }
        }
        END_LINKMAP	
    }
    else if( _apply_type & 4 )
    {
        FOR_LINKMAP( _obj->get_world(), _obj->plane_, pos, MIN_DYNAMIC_LAND_SIZE, Obj::LinkDynamic )
        {
            check_add_search_target( _obj, obj, 4, pos, _end_pos, _skill_id, _radius );
        }
        END_LINKMAP	
    }
}

static void state_rush_pre( State* state, int index )
{
    int hj_time = state->get_param( index, RUSH_PARAM_TIME_TO_PRE_END );

    hj_time -= g_timermng->get_ms_one_frame();
    if( hj_time <= 0 )
    {
        state->set_param( index, RUSH_PARAM_STAGE_ID, 0 );
    }
    else
    {
        state->set_param( index, RUSH_PARAM_TIME_TO_PRE_END, hj_time );
    }
}

static void state_rush_process( State* state, int index )
{
    float speed = state->get_param( index, RUSH_PARAM_SPEED ) * INT2FLOAT_MUL_FACTOR;
    float range = state->get_param( index, RUSH_PARAM_RANGE ) * INT2FLOAT_MUL_FACTOR;
    int skill_id = state->get_param(index, RUSH_PARAM_SKILL_ID );
    float radius = state->get_param( index, RUSH_PARAM_SEARCH_RADIUS ) * INT2FLOAT_MUL_FACTOR; 
    int apply_type = state->get_param( index, RUSH_PARAM_APPLY_TYPE );

    VECTOR3 delta = GET_DIR(OBJ->get_angle_y());
    vc3_mul(delta, delta, speed);
    vc3_mul(delta, delta, 1.0/g_timermng->get_frame_rate());
    VECTOR3 cur_pos = OBJ->getpos();

    float sx = state->get_param( index, RUSH_PARAM_START_X ) * INT2FLOAT_MUL_FACTOR;  
    float sz = state->get_param( index, RUSH_PARAM_START_Z ) * INT2FLOAT_MUL_FACTOR;  
    VECTOR3 start_pos( sx, 0, sz );
    VECTOR3 next_pos = cur_pos + delta;

    if( vc3_check_out_range( next_pos, start_pos, range ) )
    {
        VECTOR3 end_pos = start_pos + GET_DIR(OBJ->get_angle_y()) * range;
        delta = end_pos - cur_pos;
        OBJ->v_delta_.x = delta.x;
        OBJ->v_delta_.z = delta.z;

        search_hit( OBJ, end_pos, skill_id, radius, apply_type );

        int cast_hit = state->get_param( index, RUSH_PARAM_TIME_TO_CAST_HIT );
        if ( cast_hit == 0 )
        {
            state->set_param( index, RUSH_PARAM_STAGE_ID, 2 );
        }
        else
        {
            state->set_param(index, RUSH_PARAM_STAGE_ID, 1 );
        }
    }
    else
    {
        OBJ->v_delta_.x = delta.x;
        OBJ->v_delta_.z = delta.z;
        search_hit( OBJ, next_pos, skill_id, radius, apply_type );
    }
}

static void state_rush_cast_hit( State* state, int index )
{
    int hit_time = state->get_param( index, RUSH_PARAM_TIME_TO_CAST_HIT );

	if (state->is_state(STATE_SKILLING))
	{
		// int cast_time = state->get_param(index, 7);
		int skill_frame_index = state->get_param(STATE_SKILLING, SKILL_PARAM_FRAME_COUNTER);
		int skill_bs_time = state->get_param(STATE_SKILLING, SKILL_PARAM_BS_TIME);
		int skill_remain_frame = g_timermng->conv_ms_2_frame(skill_bs_time) - skill_frame_index;
		int rush_remain_frame = g_timermng->conv_ms_2_frame(hit_time);

		// LOG (2) ( "skill bs time: %d, remain frame: %d, rush_time: %d", skill_bs_time, skill_remain_frame, rush_remain_frame );
		if (skill_remain_frame > rush_remain_frame)
			return;
	}
	
    hit_time -= g_timermng->get_ms_one_frame();
    if( hit_time <= 0 )
    {
        int skill_id = state->get_param( index, RUSH_PARAM_SKILL_ID );
        OBJ->to_lua( STATE_RUSH, MSG_HIT, skill_id, 0 );
        state->set_param( index, RUSH_PARAM_STAGE_ID, 2 );
    }
    else
    {
        state->set_param( index, RUSH_PARAM_TIME_TO_CAST_HIT, hit_time );
    }
}

static void state_rush_cast( State* state, int index )
{
    int cast_time = state->get_param( index, RUSH_PARAM_TIME_TO_CAST_END );

    cast_time -= g_timermng->get_ms_one_frame();
    if( cast_time <= 0 )
    {
        state->del_state( index );
        state->del_state( STATE_SKILLING );
    }
    else
    {
        state->set_param( index, RUSH_PARAM_TIME_TO_CAST_END, cast_time );
    }
}

int state_rush( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
                VECTOR3 cur_pos = OBJ->getpos();
                state->set_param( index, RUSH_PARAM_START_X, cur_pos.x*FLOAT2INT_MUL_FACTOR );
                state->set_param( index, RUSH_PARAM_START_Z, cur_pos.z*FLOAT2INT_MUL_FACTOR );
                OBJ->to_lua( STATE_RUSH, MSG_INITIAL, p0, p1, p2, p3 );
            }
            break;
        case MSG_UPDATE:
            {
                switch( state->get_param( index, RUSH_PARAM_STAGE_ID ) )
                {
                case 3:
                    state_rush_pre( state, index );
                    break;
                case 0:
                    state_rush_process( state, index );
                    break;
                case 1:
                    state_rush_cast_hit( state, index );
                    break;
                case 2:
                    state_rush_cast( state, index );
                    break;
                default:
                    break;
                }
            }
            break; 
        case MSG_DESTROY:
            {
                int skill_id = state->get_param( index, RUSH_PARAM_SKILL_ID );
                OBJ->to_lua( STATE_RUSH, MSG_DESTROY, skill_id, 0 );
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_cinema( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
	switch(msg) {
	case MSG_INITIAL:
		{
            state->set_param(index, 0, p0);         //type 
            state->set_param(index, 1, p1);         //x
            state->set_param(index, 2, p2);         //z
            state->set_param(index, 3, p3);         //angle
            state->set_param(index, 10, 0);         //frame
			OBJ->to_lua(STATE_CINEMA, MSG_INITIAL);
		}
		break;
    case MSG_UPDATE:
        {
            // if cinema used for transfer, force end it when cinema last over 1 seconds
            if( state->get_param(index, 0) != 0 )    
            {
                int frame = state->get_param(index, 10);
                state->set_param(index, 10, ++frame);
                if( frame >= g_timermng->conv_ms_2_frame( 1000 ) ) 
                {
                    state->del_state( index );
                }
            }
        }
		break;
	case MSG_DESTROY:
		{
			OBJ->to_lua(STATE_CINEMA, MSG_DESTROY);
		}
		break;
	default:
		break;
	}
	return RT_OK;
}

static bool check_arrived_point( State *state, int index, VECTOR3& dest)
{
    float radius = OBJ->get_speed() * 1.0 / g_timermng->get_frame_rate();
	VECTOR3 dist = OBJ->getpos() - dest;
    return ( dist.x * dist.x + dist.z * dist.z /*+ dist.y * dist.y*/ <= ( radius * radius  ) );
}

static void check_move_arrive( State *state, int index, VECTOR3& dest, VECTOR3& delta )
{
    int arrive = check_arrived_point( state, index, dest );
    if(arrive) {
        OBJ->v_delta_ = dest - OBJ->get_iapos();
        state->set_param(index, 3, 1);  // arrived
        state->del_state(index);
    } else {
        OBJ->v_delta_.x = delta.x;
        OBJ->v_delta_.z = delta.z;
    }
}

int state_move_to( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    float speed = OBJ->get_speed();
    switch(msg) {
        case MSG_INITIAL:
            {
                state->set_param(index, 0, p0); // x
                state->set_param(index, 1, p1); // z
                state->set_param(index, 2, p2); // angle_y
                state->set_param(index, 3, 0);  // arrived

                float x = p0 * INT2FLOAT_MUL_FACTOR;
                float z = p1 * INT2FLOAT_MUL_FACTOR;
                VECTOR3 dest(x, 0, z);
                VECTOR3 cur_pos = OBJ->getpos();
                cur_pos = (cur_pos + dest) * 0.5f;
                OBJ->setpos( cur_pos );

                OBJ->set_angle_y( get_degree(dest, OBJ->getpos()) );
            }
            break;
        case MSG_UPDATE:
            {
                float x = state->get_param(index, 0) * INT2FLOAT_MUL_FACTOR;
                float z = state->get_param(index, 1) * INT2FLOAT_MUL_FACTOR;
                VECTOR3 dest(x, 0, z);

                OBJ->set_angle_y( get_degree(dest, OBJ->getpos()) );

                VECTOR3 delta = GET_DIR(OBJ->get_angle_y());
                vc3_mul(delta, delta, speed);
                vc3_mul(delta, delta, 1.0/g_timermng->get_frame_rate());
                check_move_arrive( state, index, dest, delta );
            }
            break; 
        case MSG_DESTROY:
            {
                int arrived = state->get_param(index, 3);
                if (arrived)
                {
                    float angle = state->get_param(index, 2) * INT2FLOAT_MUL_FACTOR;
                    OBJ->set_angle_y( angle );
                }
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_stand( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
				OBJ->to_lua(STATE_STAND, MSG_INITIAL, p0);
            }
            break;
        case MSG_UPDATE:
            break; 
        case MSG_DESTROY:
            break;
        default:
            break;
    }
    return RT_OK;
}

static void calc_speed_angle( State *state, int index, float& _speed, float& _angle )
{
    VECTOR3 move_vc( 0, 0, 0 );
    VECTOR3 drag_vc( 0, 0, 0 );
    VECTOR3 fear_vc( 0, 0, 0 );
    VECTOR3 final_vc( 0, 0, 0 );

    if ( state->is_state( STATE_MOVE_GROUND ) )
    {
        float speed = OBJ->get_speed();
        float angle = state->get_param( STATE_MOVE_GROUND , 0 ) * INT2FLOAT_MUL_FACTOR; 
        move_vc = GET_DIR( angle );
        vc3_mul( move_vc, move_vc, speed );
    }

    if ( state->is_state( STATE_FEAR ) )
    {
        float speed = OBJ->get_speed();
        if ( OBJ->is_monster() )
        {
            Monster* monster = (Monster*)OBJ;
            speed *= monster->get_ai()->get_run_speed() * 1000.0;
        }
        float angle = OBJ->get_angle_y(); 
        fear_vc = GET_DIR( angle );
        vc3_mul( fear_vc, fear_vc, speed );
    }

    if ( state->is_state( STATE_DRAG ) )
    {
        float drag_x = state->get_param( STATE_DRAG, DRAG_PARAM_CENTER_X ) * INT2FLOAT_MUL_FACTOR;
        float drag_z = state->get_param( STATE_DRAG, DRAG_PARAM_CENTER_Z ) * INT2FLOAT_MUL_FACTOR;
        float drag_speed = state->get_param( STATE_DRAG, DRAG_PARAM_SPEED ) * INT2FLOAT_MUL_FACTOR;

        vc3_init( drag_vc, drag_x, 0, drag_z );
        vc3_sub( drag_vc, drag_vc, OBJ->getpos() );
        vc3_norm( drag_vc, drag_vc, 1.0f );
        vc3_mul( drag_vc, drag_vc, drag_speed );
    }

    vc3_add( final_vc, move_vc, drag_vc );
    vc3_add( final_vc, final_vc, fear_vc );
    _speed = vc3_lensq( final_vc );
    _angle = d3d_to_degree( vc3_get_angle( final_vc ) );
}

static void step_move( State *state, int index )
{
    float speed = 0.0f;
    float angle = 0.0f;
    calc_speed_angle(state, index, speed, angle);

    VECTOR3 delta = GET_DIR( angle );
    vc3_mul( delta, delta, speed );
    vc3_mul( delta, delta, 1.0/g_timermng->get_frame_rate() );

    VECTOR3 cur_pos = OBJ->getpos();
    if ( state->is_state( STATE_DRAG ) && !state->is_state( STATE_MOVE_GROUND ) && !state->is_state( STATE_FEAR ) )
    {
        float drag_x = state->get_param( STATE_DRAG, DRAG_PARAM_CENTER_X ) * INT2FLOAT_MUL_FACTOR;
        float drag_z = state->get_param( STATE_DRAG, DRAG_PARAM_CENTER_Z ) * INT2FLOAT_MUL_FACTOR;
        float speed = state->get_param( STATE_DRAG, DRAG_PARAM_SPEED ) * INT2FLOAT_MUL_FACTOR;
        VECTOR3 drag_pos( drag_x, 0, drag_z );
        float range = (speed / g_timermng->get_frame_rate()) * (speed / g_timermng->get_frame_rate());
        if ( vc3_len( drag_pos - cur_pos ) < range )
        {
            delta = drag_pos - cur_pos;
        }
    }

	if ( !OBJ->is_ignore_block() )
	{
		float hit_rate;
		VECTOR3 hit_pos;
		VECTOR3 end_pos = cur_pos + delta;

		OBJ->raycast( &cur_pos, &end_pos, &hit_pos, &hit_rate ); 
		if( hit_rate < 1 )
		{
			delta = hit_pos - cur_pos;
		}
	}

    OBJ->v_delta_.x = delta.x;
    OBJ->v_delta_.z = delta.z;
}

int state_move_ground( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    float speed = 0.0f;
    float angle = 0.0f;
    switch(msg) {
        case MSG_INITIAL:
            {
                state->set_param(index, 0, p0); 
                calc_speed_angle(state, index, speed, angle);
                OBJ->set_angle_y( angle );
                if(OBJ->is_player())
                {
                    OBJ->to_lua(STATE_MOVE_GROUND, MSG_INITIAL, p0);
                }
            }
            break;
        case MSG_UPDATE:
            {
                step_move( state, index );
            }
            break; 
        case MSG_DESTROY:
            break;
        default:
            break;
    }
    return RT_OK;
}

static void check_navigation( State *state, int index )
{
    float x = state->get_param(index, 0) * INT2FLOAT_MUL_FACTOR;
    float z = state->get_param(index, 1) * INT2FLOAT_MUL_FACTOR;
    VECTOR3 dest(x, 0, z);
    float speed = OBJ->get_speed();
    float nav_rate = state->get_param(index, 4) * INT2FLOAT_MUL_FACTOR;
    speed *= nav_rate;

    OBJ->set_angle_y( get_degree(dest, OBJ->getpos()) );

    VECTOR3 delta;
    delta = GET_DIR(OBJ->get_angle_y());
    vc3_mul(delta, delta, speed);
    vc3_mul(delta, delta, 1.0/g_timermng->get_frame_rate());

    int arrive = check_arrived_point( state, index, dest );
    if(arrive) {
        CLIENT->pop_nav_points();

        if ( CLIENT->is_nav_points_empty() ){
            OBJ->v_delta_ = dest - OBJ->get_iapos();
            state->del_state(index);
        } else {
            float x, z;
            CLIENT->get_nav_points(x, z);

            state->set_param(index, 0, x * FLOAT2INT_MUL_FACTOR); 
            state->set_param(index, 1, z * FLOAT2INT_MUL_FACTOR);

            VECTOR3 dest(x, 0, z);
            OBJ->set_angle_y( get_degree(dest, OBJ->getpos()) );

            check_navigation( state, index );
        }
    } else {
        OBJ->v_delta_.x = delta.x;
        OBJ->v_delta_.z = delta.z;
    }
}

int state_navigation( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
                if ( CLIENT->is_nav_points_empty() ){
                    state->del_state(index);
                    break;
                }

                // x, z, end_x, end_z
                OBJ->to_lua( STATE_NAVIGATION, MSG_INITIAL, p0, p1, p2, p3 ); 
                float x, z;
                CLIENT->get_nav_points(x, z);

                state->set_param(index, 0, x * FLOAT2INT_MUL_FACTOR); 
                state->set_param(index, 1, z * FLOAT2INT_MUL_FACTOR);

                VECTOR3 dest(x, 0, z);
                OBJ->set_angle_y( get_degree(dest, OBJ->getpos()) );

                check_navigation( state, index );
            }
            break;
        case MSG_UPDATE:
            {
                check_navigation( state, index );
            }
            break; 
        case MSG_DESTROY:
            {
                OBJ->to_lua( STATE_NAVIGATION, MSG_DESTROY );
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_dazed( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
			{
				OBJ->to_lua( STATE_DAZED, MSG_INITIAL, p0 );
			}
            break;
        case MSG_DESTROY:
            {
				OBJ->to_lua( STATE_DAZED, MSG_DESTROY );
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_hold( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
			{
                state->set_param( index, HOLD_PARAM_TIME_IN_MS, p0 );
				OBJ->to_lua( STATE_HOLD, MSG_INITIAL, p0, p1 );
			}
            break;
        case MSG_UPDATE:
            {
                int hold_ms = state->get_param( index, HOLD_PARAM_TIME_IN_MS ) - g_timermng->get_ms_one_frame();
                if ( hold_ms <= 0 )
                {
                    state->del_state( index );
                }
                else
                {
                    state->set_param( index, HOLD_PARAM_TIME_IN_MS, hold_ms );
                }
            }
            break; 
        case MSG_DESTROY:
			OBJ->to_lua( STATE_HOLD, MSG_DESTROY );
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_freeze( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
			{
				OBJ->to_lua( STATE_FREEZE, MSG_INITIAL, p0 );
			}
            break;
        case MSG_DESTROY:
            {
				OBJ->to_lua( STATE_FREEZE, MSG_DESTROY );
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_dance( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
			{
				OBJ->to_lua( STATE_DANCE, MSG_INITIAL, p0 );
			}
            break;
        case MSG_DESTROY:
            {
				OBJ->to_lua( STATE_DANCE, MSG_DESTROY );
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_caught( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
			{
                state->set_param( index, 0, p0 );       // attacker id
                state->set_param( index, 1, p1 );       // cfg id
				OBJ->to_lua( STATE_CAUGHT, MSG_INITIAL, p0, p1 );
			}
            break;
        case MSG_DESTROY:
            {
				OBJ->to_lua( STATE_CAUGHT, MSG_DESTROY );
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_fear( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
			{
                state->set_param( index, 0, p0 );       // attacker id
                state->set_param( index, 1, p1 );       // buff sn
				OBJ->to_lua( STATE_FEAR, MSG_INITIAL, p0, p1 );
			}
            break;
        case MSG_UPDATE:
            {
                step_move( state, index );
            }
            break; 
        case MSG_DESTROY:
            {
				OBJ->to_lua( STATE_FEAR, MSG_DESTROY );
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_drag( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
			{
                state->set_param( index, DRAG_PARAM_CENTER_X,    p0 );
                state->set_param( index, DRAG_PARAM_CENTER_Z,    p1 );
                state->set_param( index, DRAG_PARAM_SPEED,       p2 );
                state->set_param( index, DRAG_PARAM_ATTACKER_ID, p3 );
				OBJ->to_lua( STATE_DRAG, MSG_INITIAL );
			}
            break;
        case MSG_UPDATE:
            {
                if ( !state->is_state( STATE_MOVE_GROUND ) && !state->is_state( STATE_FEAR ) )
                {
                    step_move( state, index );
                }
                int drag_time = state->get_param( index, DRAG_PARAM_TIME_IN_MS );
                if ( drag_time > 0 )    // drag_time <= 0 means infinite
                {
                    drag_time -= g_timermng->get_ms_one_frame();
                    if ( drag_time <= 0 )
                    {
                        state->del_state( index );
                    }
                    else
                    {
                        state->set_param( index, DRAG_PARAM_TIME_IN_MS, drag_time );
                    }
                }
            }
            break; 
        case MSG_DESTROY:
			OBJ->to_lua(STATE_DRAG, MSG_DESTROY);
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_replace( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
            }
            break;
        case MSG_UPDATE:
            {
            }
            break; 
        case MSG_DESTROY:
            {
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

static int state_leading_process( State* state, int index )
{
	int frame_counter = state->get_param(index, LEADING_PARAM_FRAME_COUNTER);
	int skill_id = state->get_param( index, LEADING_PARAM_SKILL_ID );   // skill id
	int tgt_id = state->get_param( index, LEADING_PARAM_TARGET_ID );
	int end_with_die = state->get_param(index, LEADING_PARAM_ENDWITH_DIE);

	Spirit* target = (Spirit*)g_worldmng->get_ctrl(tgt_id);
    if (target == NULL)
    {
		OBJ->to_lua(STATE_LEADING, MSG_HIT, skill_id, 1); 
		return RT_OK;
    }
	bool check_tgt_die = (end_with_die>0) ? target->is_die() : false;
	if (check_tgt_die)
	{
		OBJ->to_lua(STATE_LEADING, MSG_HIT, skill_id, 1); 
		return RT_OK;
	}
	else
	{
		VECTOR3 obj_pos = OBJ->getpos();
		VECTOR3 tgt_pos = target->getpos();
		float range = state->get_param(index, LEADING_PARAM_RANGE) * INT2FLOAT_MUL_FACTOR;
		if (vc3_check_out_range(obj_pos, tgt_pos, range))
		{
			OBJ->to_lua(STATE_LEADING, MSG_HIT, skill_id, 1); 
			return RT_OK;
		}
	}

	if ( frame_counter % g_timermng->get_frame_rate() == 0 )
	{
		OBJ->to_lua(STATE_LEADING, MSG_UPDATE, skill_id ); 
	}

	int period = state->get_param(index, LEADING_PARAM_PERIOD);
	if (frame_counter % period == 0)
	{
		OBJ->to_lua(STATE_LEADING, MSG_HIT, skill_id, 0); 
	}

	int total_time = state->get_param(index, LEADING_PARAM_TOTAL_TIME);
	if( total_time > 0 )
	{
		int one_frame_time = g_timermng->get_ms_one_frame();
		if( total_time - one_frame_time * frame_counter < 0 )
		{
			OBJ->to_lua(STATE_LEADING, MSG_HIT, skill_id, 1); 
		}
	}

	++frame_counter;
	state->set_param(index, LEADING_PARAM_FRAME_COUNTER, frame_counter);
	return RT_OK;
}

static int state_leading_cast( State* state, int index )
{
	int cast_time = state->get_param(index, LEADING_PARAM_CAST_TIME);
	cast_time -= g_timermng->get_ms_one_frame();
	if( cast_time <= 0 )
	{
		state->del_state( index );
		state->del_state( STATE_SKILLING );
	}
	else
	{
		state->set_param(index, LEADING_PARAM_CAST_TIME, cast_time);
	}
	return RT_OK;
}

static int state_leading( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
	switch(msg) {
		case MSG_INITIAL:
			{
				state->set_param( index, LEADING_PARAM_IS_CANCEL, 0 );    // is cast
				state->set_param( index, LEADING_PARAM_FRAME_COUNTER, 0 );   // frame counter
				state->set_param( index, LEADING_PARAM_SKILL_ID, p0 );    // skill id
				state->set_param( index, LEADING_PARAM_PERIOD, p1 );   // period
				state->set_param( index, LEADING_PARAM_CAST_TIME, p2 );   // cast time
				state->set_param( index, LEADING_PARAM_DEC_TIME, p3);	// dec_time
			}
			break;
		case MSG_UPDATE:
			{
				switch( state->get_param( index, LEADING_PARAM_IS_CANCEL ) )
				{
				case 0:
					state_leading_process( state, index );
					break;
				case 1:
					state_leading_cast( state, index );
					break;
				default:
					break;
				}
			}
			break; 
		case MSG_DESTROY:
			{
				OBJ->to_lua( STATE_LEADING, MSG_DESTROY );
			}
			break;
		default:
			break;
	}
	return RT_OK;
}

// 0 tx
// 1 tz
// 2 speed
// 3 range
// 4 startx
// 5 startz
int state_pull( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
                state->set_param(index, 0, p1); // end x
                state->set_param(index, 1, p2); // end z
                state->set_param(index, 2, p3); // speed

                VECTOR3 cur_pos = OBJ->getpos();
                float endx = p1 * INT2FLOAT_MUL_FACTOR;
                float endz = p2 * INT2FLOAT_MUL_FACTOR;
                VECTOR3 end_pos( endx, 0, endz );
                float range = vc3_xzlensq( end_pos - cur_pos );

                state->set_param(index, 3, range*FLOAT2INT_MUL_FACTOR);            //range
                state->set_param(index, 4, cur_pos.x*FLOAT2INT_MUL_FACTOR);        //startx
                state->set_param(index, 5, cur_pos.z*FLOAT2INT_MUL_FACTOR);        //startz 
				OBJ->to_lua(STATE_PULL, MSG_INITIAL, p0, p1, p2, p3 );
            }
            break;
        case MSG_UPDATE:
            {
                VECTOR3 cur_pos = OBJ->getpos();
                float speed = state->get_param(index, 2) * INT2FLOAT_MUL_FACTOR;    //speed
                float range = state->get_param(index, 3) * INT2FLOAT_MUL_FACTOR;    //range

                float sx = state->get_param(index, 4) * INT2FLOAT_MUL_FACTOR;  
                float sz = state->get_param(index, 5) * INT2FLOAT_MUL_FACTOR;  
                VECTOR3 start_pos( sx, 0, sz );

                float endx = state->get_param(index, 0) * INT2FLOAT_MUL_FACTOR;     //end x
                float endz = state->get_param(index, 1) * INT2FLOAT_MUL_FACTOR;     //end z
                VECTOR3 end_pos( endx, 0, endz );

                float degree = get_degree( end_pos, start_pos ); 
                VECTOR3 delta = GET_DIR( degree ); 
                vc3_mul(delta, delta, speed);
                vc3_mul(delta, delta, 1.0/g_timermng->get_frame_rate());
                VECTOR3 next_pos = cur_pos + delta;

                if( vc3_check_out_range( next_pos, start_pos, range ) )
                {
                    delta = end_pos - cur_pos;
                    OBJ->v_delta_.x = delta.x;
                    OBJ->v_delta_.z = delta.z;
                    state->del_state( index );
                }
                else
                {
                    OBJ->v_delta_.x = delta.x;
                    OBJ->v_delta_.z = delta.z;
                }
            }
            break; 
        case MSG_DESTROY:
            break;
        default:
            break;
    }
    return RT_OK;
}

int state_pick( State *state, int index, int msg, int p0, int p1, int p2, int p3 )
{
    switch(msg) {
        case MSG_INITIAL:
            {
                state->set_param( index, 0, p0 );   // ctrl_id of pick_point
                state->set_param( index, 1, p1 );   // pick interrupt msg_id
                OBJ->to_lua( STATE_PICK, MSG_INITIAL ); 
            }
            break;
        case MSG_DESTROY:
            {
				OBJ->to_lua( STATE_PICK, MSG_DESTROY );
            }
            break;
        default:
            break;
    }
    return RT_OK;
}

void init_states()
{
    unsigned int i, j;
    for(i = 0; i < STATE_MAX; ++i) {
        memset(State::get_desc(i), 0, sizeof(StateDesc)); 
        for(j = 0; j < STATE_MAX; ++j) {
            if(table[i][j] == 0) {
                set_bit(j, State::get_desc(i)->deny);
            } else if(table[i][j] == 1) {
                set_bit(j, State::get_desc(i)->swap);
            } else if(table[i][j] == 3) {
                set_bit(j, State::get_desc(i)->dely);
            }
        }
    }

    State::set_state_func(STATE_STAND, state_stand);
    State::set_state_func(STATE_MOVE_TO, state_move_to);
    State::set_state_func(STATE_SKILLING, state_skilling);
    State::set_state_func(STATE_RUSH, state_rush);
	State::set_state_func(STATE_CINEMA, state_cinema);
    State::set_state_func(STATE_DEAD, state_dead);
    State::set_state_func(STATE_HURT, state_hurt);
    State::set_state_func(STATE_MOVE_GROUND, state_move_ground);
    State::set_state_func(STATE_NAVIGATION, state_navigation);
    State::set_state_func(STATE_HURT_BACK, state_hurt_back);
    State::set_state_func(STATE_FREEZE, state_freeze);
    State::set_state_func(STATE_DANCE, state_dance);
    State::set_state_func(STATE_CAUGHT, state_caught);
    State::set_state_func(STATE_FEAR, state_fear);
    State::set_state_func(STATE_DAZED, state_dazed);
    State::set_state_func(STATE_HOLD, state_hold);
    State::set_state_func(STATE_HURT_FLY, state_hurt_fly);
    State::set_state_func(STATE_HURT_FLOAT, state_hurt_float);
    State::set_state_func(STATE_MOVE_PERSIST, state_move_persist);
    State::set_state_func(STATE_HURT_BACK_FLY, state_hurt_back_fly);
	State::set_state_func(STATE_HURT_HORI, state_hurt_hori);
    State::set_state_func(STATE_DRAG, state_drag);
    State::set_state_func(STATE_REPLACE, state_replace);
	State::set_state_func(STATE_LEADING, state_leading);
    State::set_state_func(STATE_PULL, state_pull);
    State::set_state_func(STATE_PICK, state_pick);
}

bool StatesModule::app_class_init(){
    init_states();
    init_t_math();
    LOG(2)( "===========States Module Init===========");
    return true;
}

bool StatesModule::app_class_destroy(){
    LOG(2)( "===========States Module Destroy===========");
    return true;
}




