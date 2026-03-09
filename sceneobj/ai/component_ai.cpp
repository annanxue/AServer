#include "component_ai.h"
#include <cmath>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <sstream>
#include "float.h"
#include "btree.h"
#include "utils.h"
#include "condition.h"
#include "monster.h"
#include "timer.h"
#include "log.h"
#include "load_btree.h"
#include <stdio.h>
#include <stdlib.h>
#include "camp_mng.h"

namespace SGame
{

int32_t AI::proto_counter_ = 0;

AI::AI()
{
	owner_ = NULL; 
	ai_id_ = 0;
	target_id_ = 0;
	target_ = NULL;
	last_target_id_ = 0;
	motion_range_idx_ = 0;
	motion_node_ = NULL;
	frame_counter_ = -1;
	smoother_.init(2);
	target_visible_ = false;
	chase_aoi_dist_ = 0.f;
	vc3_init(speed_vec_, 0, 0, 0);
	vc3_init(spawn_point_, 0, 0, 0);
	spawn_angle_ = 0.f;
	angle_max_ = 0.f;
	angle_min_ = 0.f;
	cannot_turn_ = 0;
	is_good_ = true;
	last_act_type_ = SHARE_ACT_NONE;
	last_turn_type_ = SHARE_TURN_NONE;
	turn_map_.insert(std::make_pair(SHARE_TURN_STILL, SHARE_TURN_STILL));
	turn_map_.insert(std::make_pair(SHARE_TURN_TO_CHAR_ALWAYS, SHARE_TURN_TO_CHAR_ALWAYS));
	turn_map_.insert(std::make_pair(SHARE_TURN_TO_CHAR_ONCE, SHARE_TURN_STILL));
	is_virtual_ = false;
	nobreak_ = false;
	rate_acc_ = 0;
	spawn_flag_ = SHARE_AI_SPAWN_NONE;
	obj_id_ = 0;
	mode_ = SHARE_AI_NORMAL_MODE;
	script_move_ = false;
	reborn_flag_ = false;
	force_in_battle_  = false;
	is_moving_ = false;
	write_pos_ = false;
	walk_speed_ = 0;
	run_speed_ = 0;
	rush_speed_ = 0;
	melee_offset_ = 0;
	rare_print_ = 0;
	verbose_print_ = 0;
	cur_time_ = 0;
	threat_mgr_.set_owner(this);
	resume_time_ = 0;
	extra_skill_id_ = 0;
	section_ = 0;
	ignore_block_ = false;
	leave_fight_ = false;
	not_ot_ = false;
	buff_min_time_ = 0.f;
	buff_mst_dist_ = 0.f;
	buff_mst_num_ = 0;
	buff_del_id_ = 0;
	buff_del_time_ = 0;
    home_dist_square_ = -1.0f;
    is_returning_home_ = false;
    is_rm_combat_npc_ = false;
    remote_fix_rad_ = 0.f;
    rush_master_rate_ = 1.f;
    master_id_ = 0;
    bt_in_range_ = 0.f;
    bt_out_range_ = 0.f;
    nbt_in_range_ = 0.f;
    nbt_out_range_ = 0.f;
    off_angle_ = DEFAULT_OFF_ANGLE;
	is_running_ = false;
	is_positive_ = false;
    only_id_ = 0;
    forward_angle_ = 0.f;
}

AI::~AI()
{

}

const char AI::className[] = "AI";
const bool AI::gc_delete_ = false;

Lunar<AI>::RegType AI::methods[] =
{	
	LUNAR_AI_METHODS,
	{NULL, NULL}
};

bool AI::is_dead_or_error()
{
	return is_dead() || !is_good();
}

bool AI::is_dead() const
{
	Spirit* owner = static_cast<Spirit*>(g_worldmng->get_ctrl(get_id()));

	if(is_invalid_obj(owner))
	{
		return true;
	}

	if(owner->state.is_state(STATE_DEAD) > 0)
	{
		return true;
	}

    return false;
}

bool AI::can_do_continue_skill() const
{
    if (!is_running_ || resume_time_ > 0)
    {
        return false;
    }

	if (is_invalid_obj(owner_))
	{
		return false;
	}

	Spirit* owner = static_cast<Spirit*>(owner_);
    if (owner->state.is_state(STATE_SKILLING) == 0 && owner->state.is_state(STATE_STAND) == 0)
    {
        return false;
    }

    return true;
}

int AI::c_set_suspend(lua_State* _state)
{
	lcheck_argc(_state, 1);
	bool is_suspend = lua_toboolean(_state, 1);
    bool is_run = !is_suspend;

	if (is_run != is_running_)
	{
		is_running_ = is_run;
		if (!is_running_)
		{
			reset(true);
		}
	}
	return 0;
}

int AI::c_set_running( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	bool is_run = lua_toboolean( _state, 1 );
	if (is_run != is_running_)
	{
		is_running_ = is_run;
		if (!is_running_)
		{
            threat_mgr_.clear_all_threats();
			reset(true);
		}
	}
	return 0;
}

void AI::execute()
{
	if (!is_running_)
	{
		return;
	}

	if(is_dead_or_error())
	{
		l_stop();
		return;
	}

	set_cur_time();

	int32_t frame = g_timermng->get_frame_rate()/2;
	bool in_flag = frame_counter_ % frame == 0;
	bool out_flag = frame_counter_ % frame == 0;

	threat_mgr_.execute(can_leave_fight() || in_flag, out_flag);
	
	++frame_counter_;

	if(resume_time_)
	{
		if(cur_time_ > resume_time_)
		{
			resume_time_ = 0;
		}
		else
		{
			return;
		}
	}
	
	del_cond_buff();

	set_steped(false);

	btree_.execute();
}

bool AI::can_broadcast_msg( ACT_TYPE _act_type /*= SHARE_ACT_NONE*/, TURN_TYPE _turn_type /*= SHARE_TURN_NONE*/ )
{
	bool flag = true;

	if (SHARE_ACT_STAND != last_act_type_ || SHARE_ACT_STAND != _act_type ||
		turn_map_[last_turn_type_] != _turn_type)
	{
		flag = true;
	}
	else
	{
		proto_counter_ ++;
		//printf("proto_counter_ %d\n", proto_counter_);
	}

	last_act_type_ = _act_type;
	last_turn_type_ = _turn_type;

	return flag;
}

void AI::init()
{
	vector<float> range_vals;
    bool is_init_motion = false; 
	for (uint32_t  i = 0; i < btree_.get_condition().size(); ++i)
	{
		const ConditionSVO *node = static_cast<const ConditionSVO *>(btree_.get_condition()[i]);
		{
			const string& range_str = node->get_range_str();
			float range_val;
			vector<float> temp_ranges;

			AI_TYPE range_type = RANGE_FLOAT;

			if ("melee" == range_str)
			{
				range_type = RANGE_MELEE;
			}
			else if (range_str[0] == '[')
			{
				range_type = RANGE_LIST;
				get_float_list(range_str, temp_ranges);
			}
			else if (range_str[0] == '(')
			{
				range_type = RANGE_TUPLE;
				get_float_list(range_str, temp_ranges);
			}
			else
			{
				range_val = atof(range_str.c_str());
				temp_ranges.push_back(range_val);
			}
			const_cast<ConditionSVO *>(node)->set_ranges(temp_ranges, range_type);
			
			if(RANGE_LIST == range_type)
			{
				init_motion_range(temp_ranges);
                is_init_motion = true;
				if(verbose_print_)
				{
					for(vector<float>::const_iterator iter = temp_ranges.begin(); iter != temp_ranges.end(); ++iter)
					{
						TRACE(2)("ai_debug MotionRange : %f", *iter);
					}
				}
				temp_ranges.clear();
			}

			if((COND_IN_START < node->get_event()  && node->get_event() < COND_IN_END) ||
				(COND_OUT_START < node->get_event() && node->get_event() < COND_OUT_END))
			{		
				for(uint32_t i = 0; i< temp_ranges.size(); ++i)
				{
					float range_ = temp_ranges[i];

					vector<float>::const_iterator iter = find(range_vals.begin(), range_vals.end(),
						range_);
					if (iter == range_vals.end())
					{
						range_vals.push_back(range_);
					}
				}
			}
		}
	}

	sort(range_vals.begin(), range_vals.end());

	if(verbose_print_)
	{
		for(vector<float>::const_iterator iter = range_vals.begin(); iter != range_vals.end(); ++iter)
		{
			TRACE(2)("ai_debug Range : %f", *iter);
		}
	}

	init_range(range_vals);

	for (uint32_t i = 0; i < btree_.get_condition().size(); ++i)
	{
		ConditionSVO *node = static_cast<ConditionSVO *>(btree_.get_condition()[i]);
		vector<float>::const_iterator iter;
		int32_t idx_start = -1;
		int32_t idx_end = -1;

		switch (node->get_range_type())
		{
		case RANGE_FLOAT:
			iter = find(range_vals.begin(), range_vals.end(), node->get_ranges()[0]);
			if(iter != range_vals.end())
			{
				idx_end = iter - range_vals.begin();
			}
			break;

		case RANGE_TUPLE:
			iter = find(range_vals.begin(), range_vals.end(), node->get_ranges()[0]);
			if(iter != range_vals.end())
			{
				idx_start = iter - range_vals.begin();
			}

			if (node->get_ranges().size() > 1)
			{
				iter = find(range_vals.begin(), range_vals.end(), node->get_ranges()[1]);
				if (iter != range_vals.end())
				{
					idx_end = iter - range_vals.begin();
				}
			}
			break;

		case RANGE_LIST:
			motion_node_ = node;
			motion_node_->reset_chase_radius();
			break;

		case RANGE_MELEE:
			break;

		default:
            node->errorf( "range type not found" );
            break;
		}
		node->set_range_pair(idx_start, idx_end);
	}

    // if not init motion range then read ["view"] in config  CHAR_AI.lua to init motion range
    if( !is_init_motion ) 
    {
        vector<float> view_ranges;
        const string& view_str = get_ai_info("view");
        if( view_str[0] == '[' )
        {
			get_float_list( view_str, view_ranges );
            init_motion_range( view_ranges );
        }
    }

	is_virtual_ = atoi(get_ai_info("virtual_").c_str());
	nobreak_ = atoi(get_ai_info("nobreak_").c_str());
	force_in_battle_ = atoi(get_ai_info("force_in_battle_").c_str());
	melee_offset_ = atof(get_ai_info("melee_offset_").c_str());
	out_range_ = atof(get_ai_info("out_range_").c_str());
	in_range_ = atof(get_ai_info("in_range_").c_str());
	ignore_block_ = (get_ai_info("IgnoreBlock") == "1");
	leave_fight_ = (get_ai_info("LeaveFight") == "1");
	not_ot_ = atoi(get_ai_info("not_OT").c_str());
}

void AI::init_range(const vector<float>& _range_vals)
{
	for(uint32_t i = 0; i< _range_vals.size(); ++i)
	{
		float radius = _range_vals[i] ;
		float buff = radius + RANGE_DIS_BUFF;
		ranges_.push_back(CircleRange(this, "range_", radius, buff));
	}
}

void AI::init_motion_range(const vector<float>& _range_vals)
{
	if(assert_fail(_range_vals.size() >= RANGE_LENGTH))
	{
		ERR(2)("error motion range,ai_id=%d", ai_id_);
		return;
	}

	float view_angle = radians(_range_vals[RANGE_VIEW_ANGLE]/2.f);
	float sight_radius = _range_vals[RANGE_SIGHT];

	float motion_radius = _range_vals[RANGE_MOTION];

	chase_aoi_dist_ = sight_radius > motion_radius ? sight_radius : motion_radius;

	float chase_radius = _range_vals[RANGE_CHASE];

	float dis_buff = RANGE_DIS_BUFF;
	float angle_buff =  radians(RANGE_ANGLE_BUFF);

	SectorRange sight_sector(this, "sight_sector",  sight_radius,
		sight_radius + dis_buff, view_angle, view_angle + angle_buff);
	CircleRange listen_circle(this, "listen_circle", motion_radius, motion_radius + dis_buff);

	motion_sectors_.push_back(SectorCircleRange(this, "motion_range", listen_circle, sight_sector));
	motion_circles_.push_back(CircleRange(this, "chase_range", chase_radius, chase_radius + dis_buff));
}

bool AI::query_common_range( int32_t _range_id ) const
{
	return ranges_[_range_id].is_in_range(tgt_get_pos(NULL));
}

bool AI::query_motion_range(RANGE_TYPE _range_type, bool _is_in)
{
	return query_range(_range_type, _is_in, tgt_get_pos(NULL));
}

bool AI::tgt_query_dist(float _range_dist, bool _is_in)
{
	float dist = tgt_get_dist(NULL);
	bool ret = (dist <= _range_dist);
	if (!_is_in)
	{
		ret = !ret;
	}
	return ret;
}

int AI::c_on_message( lua_State* _state )
{
	lcheck_argc( _state, 5 );
	int32_t msg = lua_tointeger(_state, 1);
	int32_t arg1 = lua_tointeger(_state, 2);
	int32_t arg2 = lua_tointeger(_state, 3);
	int32_t arg3 = lua_tointeger(_state, 4);
	int32_t arg4 = lua_tointeger(_state, 5);

	if(!ai_id_ || !is_good())
	{
		return 0;
	}

    set_cur_time();

	switch(msg)
	{
	case AI_ON_HIT:
		btree_.reset(true);
		resume_time_ = cur_time_ + arg2 * THOUSAND;
		break;

	case AI_ON_RESUME:
		resume_time_ = cur_time_;// + 0.6 * THOUSAND;
		break;

	case AI_ON_DAMAGE:
		threat_mgr_.add_damage(arg1, arg2, arg3, arg4);
		break;
	default:
		btree_.on_message(msg);
        break;
	}
	return 0;
}

float AI::get_range(int32_t _range_idx) const
{
	return ranges_[_range_idx].get_radius();
}

bool AI::set_path( Navmesh::Path &_path, bool _loop, bool _not_find_path, int32_t _ani_type, const vector<PATROL_INFO> *_patrol_info, bool _broadcast )
{
	return btree_.set_patrol_path(_path, _loop, _not_find_path, _ani_type, _patrol_info, _broadcast );
}

void AI::set_cur_time()
{
	cur_time_ = g_timermng->get_ms();
}

float AI::get_melee_range() const//12
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "get_melee_range" );
	lua_pushinteger( L, get_id());

	if( llua_call( L, 2, 1, 0 ) == 0) 
	{
		float rad = lua_tonumber( L, -1 );
		lua_pop( L, 1 );    
		return rad;
	}
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return 3.f;
	}  
}

void AI::cast_skill( int32_t _skill_id, char _use, uint32_t _guid, char _skill_type, const VECTOR3& _pos, int32_t _use_bs) const
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "start_skill" );
	lua_pushinteger( L, get_id());
	lua_pushinteger( L, _skill_id);
	lua_pushinteger( L, _guid);
	lua_pushinteger( L, get_frame_end_time());
	lua_pushboolean( L, _use_bs);

	if( llua_call( L, 6, 0, 0 )) 
	{
		llua_fail( L, __FILE__, __LINE__ );
	}
}

void AI::l_activate_trig(const vector<float>& _chunk_idx) const//12
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "activate_trig" );
	lua_pushinteger( L, get_id());

	lua_newtable(L);
	for (uint32_t i = 0; i < _chunk_idx.size(); ++i)
	{
		lua_pushinteger(L, _chunk_idx[i]);
		lua_rawseti(L, -2, i+1);
	}

	if( llua_call( L, 3, 0, 0 )) 
	{
		llua_fail( L, __FILE__, __LINE__ );
	} 
}

int AI::c_init(lua_State* _state)
{
	lcheck_argc( _state, 7 );
	uint32_t owner_id = lua_tointeger( _state, 1 );
	int32_t ai_id = lua_tointeger( _state, 2 );

	walk_speed_ = lua_tonumber( _state, 3 )/THOUSAND;
	run_speed_ = lua_tonumber( _state, 4 ) /THOUSAND;
	rush_speed_ =  lua_tonumber( _state, 5 ) /THOUSAND;
    master_id_ = lua_tointeger( _state, 6 );
	is_positive_ = lua_toboolean(_state, 7);

	owner_ = g_worldmng->get_ctrl(owner_id);
    if (is_invalid_obj(owner_))
    {
        ERR(2)("[AI](AI::c_init):owner_ is invalid, owner_id = %u", owner_id);
        return 0;
    }

	static_cast<Monster *>(owner_)->set_ai(this);

	ai_id_ = ai_id;
	
	template_name_ = get_ai_info("Template");
	if(assert_fail(!template_name_.empty()))
	{
		ERR(2)("ai id not exist,ai_id=%d", ai_id);
		return 0;
    }

    if (assert_fail(btree_.load(template_name_, this)))
    {
        ERR(2)("xml file load failed,ai_id=%d", ai_id);
        return 0;
    }

    uint32_t is_combat_npc = atoi(get_ai_info("is_combat_npc").c_str()); 
    if (is_combat_npc)
    {
        get_combat_npc_move_ranges();
        is_rm_combat_npc_ = atoi(get_ai_info("is_rm_combat_npc").c_str());
        remote_fix_rad_ = atof(get_ai_info("remote_fix_rad").c_str());
    }

	btree_.init_tree();

	init();
	
    get_tgt_from_master();
    
	if(rare_print_)
	{
		btree_.dump_main_selector();
	}
	return 0;
}

void AI::get_combat_npc_move_ranges()
{
	std::string combat_npc_range_str = get_ai_info("combat_npc_move_range");
    std::vector<float> tmp_vec;
    get_float_list(combat_npc_range_str, tmp_vec);
    bt_in_range_ = tmp_vec[0];
    bt_out_range_ = tmp_vec[1];
    nbt_in_range_ = tmp_vec[2];
    nbt_out_range_ = tmp_vec[3];
}

float AI::tgt_get_dir( const BTLeafNode* _node ) const
{
	if(_node && _node->target_last() && get_last_one())
	{
		return static_cast<Ctrl*>(get_last_one())->get_angle_y();
	}
	else
	{
		if(target_)
		{
			return target_->get_angle_y();
		}
		else
		{
			return 0.f;
		}
	}
}

VECTOR3 AI::get_obj_pos( Obj* _obj ) const
{
	return _obj->getpos();
}

void AI::set_speed_vec(const VECTOR3& _vec, float _deviate_angle )
{
	speed_vec_ = _vec;
	speed_vec_.y = 0;
	float angle = degrees(dir2radian(speed_vec_));
	angle += _deviate_angle;

	set_client_angle(angle);
}

void AI::set_angle(float _angle)
{
	owner_->set_angle_y(_angle);
}

void AI::set_speed_len(float _length)
{
	is_moving_ = _length > 0;
}

void AI::set_target( Obj* _new_tgt ) 
{
	if(target_ != _new_tgt)
	{
		reset();
		tgt_safe_set(_new_tgt);
		if(_new_tgt)
		{
			//targetCombat_start(newTgt)
		}
		else
		{
			self_combat_end();
			if(motion_node_)
			{
				motion_node_->reset_chase_radius();
			}
			btree_.clear_cd();
			rate_acc_ = 0;
			last_act_type_ = SHARE_ACT_NONE;
			last_turn_type_ = SHARE_TURN_NONE;
		}
	}
}

uint32_t AI::tgt_get_id( const BTLeafNode* _node/*=NULL*/, bool _find_last /*= false*/ ) const
{
	if(_node && _node->target_last() && get_last_one())
	{
		return static_cast<Ctrl*>(get_last_one())->get_id();
	}
	else if(_node && _node->target_master() && get_master())
	{
		return get_master()->get_id();
	}
	else if(target_)
	{
		return static_cast<Ctrl*>(target_)->get_id();
	}
	else if(_find_last && is_valid_obj(g_worldmng->get_ctrl(last_target_id_)))
	{
		return last_target_id_;
	}
	return 0;
}

void AI::set_pos( VECTOR3& _pos )
{
	owner_->setpos(_pos);
}

int32_t AI::get_type_id(Obj *_obj) const//11
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "get_type_id" );
	lua_pushinteger( L, static_cast<Ctrl*>(_obj)->get_id());

	if( llua_call( L, 2, 1, 0 ) == 0) 
	{
		float index = lua_tointeger( L, -1 );
		lua_pop( L, 1 );  
		return index;
	}
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return 1.f;
	}  
}

void AI::shout(Obj* _obj)
{
// 	if(ai)
// 	{
// 		ai->setThreat_target(target_);
// 		ai->force_in_battle();
// 	}
}

bool AI::check_obj(Obj* _obj, int _type, bool _alive) const
{
	return _obj->is_type(_type) && is_invalid_target(static_cast<Ctrl*>(_obj)->get_id()) != _alive;
}

bool AI::can_shout(Obj* _obj) const
{
// 	if(ai)
// 	{
// 		if(!ai->is_in_battle() || ai->tgt_get_id() != tgt_get_id())
// 		{
// 			return true;
// 		}
// 	}
	return false;
}

void AI::set_chase_radius(float _radius)
{
	if (motion_node_)
	{
		float range = motion_node_->get_chase_radius();
		motion_node_->set_chase_radius((max(range, _radius)));
	}
}

uint32_t AI::get_id() const
{
	if(owner_)
	{
		return static_cast<Ctrl*>(owner_)->get_id();
	}
	else
	{
		return 0;
	}
}

bool AI::is_fake_battle() const
{
    return threat_mgr_.is_fake_battle(); 
}

void AI::get_tgt_from_master()
{
    Ctrl* master = get_master();
    if (master == NULL)
    {
        return;
    }

    World* world = master->get_world();
    Scene* scene = world->get_scene(); 
    VECTOR3 master_pos = master->getpos();
    float range = scene->get_min_dynamic_land_size() * 2.5;
    FOR_LINKMAP(world, master->plane_, master_pos, range, Obj::LinkDynamic )
    {
        if(check_obj(obj, OT_MONSTER, true))
        {
            threat_mgr_.on_obj_enter(obj);
        }
    }
    END_LINKMAP
}

float AI::obj_dist_of_master(Ctrl* _master, Obj* _obj)
{
    return length2d(_master->getpos() - _obj->getpos());
}

bool AI::is_melee() const
{
    return !is_rm_combat_npc_;
}

float AI::get_remote_fix_rad() const
{
    return remote_fix_rad_;
}

float AI::get_rush_master_rate() const
{
    return rush_master_rate_;
}

bool AI::is_obj_virtual(Obj* _obj) const
{
// 	if(ai)
// 	{
// 		return ai->is_virtual();
// 	}
	return false;
}

bool AI::is_virtual() const
{
	return is_virtual_;
}

Ctrl* AI::get_master() const
{
    Ctrl* master = NULL;
    if (master_id_ != 0)
    {
        master = g_worldmng->get_ctrl(master_id_);
        if (is_invalid_obj(master))
        {
            ERR(2)("[AI](AI::has_master):master is invalid, master_id = %u", master_id_);
            master = NULL;
        }
    }
    return master;
}

bool AI::has_master() const
{
    Ctrl* master = get_master();
    if (master != NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int AI::c_sync_path_to( lua_State* _state )
{
	lcheck_argc( _state, 0 );
// 	if(is_moving_)
// 	{
		lua_pushboolean(_state, btree_.sync_path_to());
	//}
	return 1;
}

void AI::set_spawn_flag(SPAWN_FLAG _flag)
{
	spawn_flag_ = _flag;
	switch(_flag)
	{
	case SHARE_AI_SPAWN_JUMP:
		break;

	case SHARE_AI_SPAWN_PATROL:
		btree_.add_spawn_patrol_node();
		break;

	case SHARE_AI_SPAWN_ACT:
		break;
	}
}

float AI::get_obj_radius(Obj* _obj) const
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "get_obj_radius" );
	lua_pushinteger( L, get_id());
	lua_pushinteger( L, static_cast<Ctrl*>(_obj)->get_id());

	if( llua_call( L, 3, 1, 0 ) == 0) 
	{
		float radius = lua_tonumber( L, -1 );
		lua_pop( L, 1 );    
		return radius;
	}
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return DEFAULT_BOUNDING_RADIUS;
	}  
}

VECTOR3 AI::get_master_pos( Ctrl* _master, bool _not_offset /*= false*/ )
{
 	if (_master != NULL)
 	{
        if (_not_offset)
        {
            return _master->getpos(); 
        }
        else
        {
            if (off_angle_ != DEFAULT_OFF_ANGLE)
            {
                return get_follow_pos(_master, false);
            }
            else
            {
                return _master->getpos(); 
            }
        }
 	}
	return get_pos();
}

const list<Obj*>& AI::get_neighbors()
{
	neighbors_.clear();
	return neighbors_;
}

int AI::tgt_is_type(int _type) const
{
	if(target_)
	{
		return target_->is_type(_type);
	}
	else
	{
		return 0;
	}
}

bool AI::find_path(const VECTOR3& _start, const VECTOR3& _end, Navmesh::Path* _path, unsigned short _door_flags)
{
	VECTOR3 hit_pos;
	float hit_rate;
	_path->clear();

	if(raycast(_start, _end, hit_pos, hit_rate, _door_flags))
	{
		_path->push_back(_start);
		_path->push_back(_end);
		return true;
	}
	else
	{
		return static_cast<Ctrl*>(owner_)->findpath(&_start, &_end, _path, _door_flags);
	}
}

bool AI::find_poly(const VECTOR3& _center, float _min_radius, float _radius,
					std::vector<VECTOR3>& _points, float _min_rate)//12
{
	if(false)
	{
		// 	if(get_scene())
		// 	{
		// 		return get_scene()->find_poly(pos, min_radius, max_radius, centers);
		// 	}
		// 	else
		{
			return false;
		}
	}
	else
	{
		float div = 16.f;
		_points.clear();

		for(float angle = 0.f; angle < 359.f; angle += 360.f/div)
		{
			float a = radians(angle);
			VECTOR3 pos;
			pos.x = _center.x + _radius * cosf(a);
			pos.z = _center.z + _radius * sinf(a);
			VECTOR3 ret_pos;
			float rate;
			raycast(_center, pos, ret_pos, rate);
			if(rate > _min_rate)
			{
				_points.push_back(ret_pos);
			}
		}
		return _points.size() > 0;
	}
}

bool AI::raycast( const VECTOR3& _start, const VECTOR3& _end, VECTOR3& _hit_pos_ptr, float& _rate, unsigned short _door_flags )
{
	return static_cast<Ctrl*>(owner_)->raycast(&_start, &_end, &_hit_pos_ptr, &_rate, _door_flags);//9
}

bool AI::assert_fail( bool _cond ) const
{
	if(!_cond)
	{
		_throw();
	}
	return !_cond;
}

float AI::get_obj_die_time(Obj* _obj)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "get_die_time" );
	lua_pushinteger( L, static_cast<Ctrl*>(_obj)->get_id());

	if( llua_call( L, 2, 1, 0 ) == 0) 
	{
		float time = lua_tonumber( L, -1 );
		lua_pop( L, 1 );    
		return time;
	} 
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return 0;
	}  
}

bool AI::no_break() const
{
	return nobreak_;
}

std::string AI::dump_info()
{
	if(get_ai_id() == 0)
	{
		return "no ai id";
	}

	tgt_check_exist();

	std::string spawn_act;
	switch(spawn_flag_)
	{
	case SHARE_AI_SPAWN_JUMP:
		spawn_act = "jmp";
		break;
	case SHARE_AI_SPAWN_PATROL:
		spawn_act = "pth";
		break;
	case SHARE_AI_SPAWN_ACT:
		spawn_act = "ani";
		break;
	default:
		spawn_act = "no";
	}

	std::string ret;
	std::string moving = is_moving_ ? "inMov" : "noMov";
	std::string returning = false ? "inRet" : "noRet";
	std::string battle = is_in_battle() ? "inBat" : "noBat";
	VECTOR3 tarPos = tgt_get_pos(NULL);
	uint32_t tarGUID = tgt_get_id(NULL);
	VECTOR3 pos = get_pos();
	VECTOR3 spwPos= get_spawn_point();
	char temp[MAX_FILE_NAME];
	float tarDis = length2d(pos - tarPos);
	float spwDis = length2d(pos - spwPos);
	int32_t type_id = get_type_id();
	memset(temp, 0, MAX_FILE_NAME);

// 	ff_snprintf(temp, MAX_FILE_NAME, "scn%lu,pos(%.1f,%.1f)spw(%.1f,%.1f)%s\n", 
// 		get_scene()->get_id(), pos.x, pos.z, spwPos.x, spwPos.z, spawn_act.c_str());

	ret += temp;

	ff_snprintf(temp, MAX_FILE_NAME, "type%d,ai%d,xml:%s\n", \
		type_id, get_ai_id(), get_template_name().c_str());
	ret += temp;

	ff_snprintf(temp, MAX_FILE_NAME, "%s,%s,%s\n", \
		btree_.dump_info().c_str(), moving.c_str(), returning.c_str());
	ret += temp;

	ff_snprintf(temp, MAX_FILE_NAME, "%s,tar_pos(%.1f,%.1f),tarId:%u\n", \
		battle.c_str(), tarPos.x, tarPos.z, tarGUID);

	ret += temp;

	ff_snprintf(temp, MAX_FILE_NAME, "tarDis%.1f,view%s,spwDis%.1f\n", \
		tarDis, get_motion_str().c_str(), spwDis);
	ret += temp;

	ff_snprintf(temp, MAX_FILE_NAME, "frame%d,id:%u", \
		frame_counter_, get_id());

	ret += temp;

	return ret;
}

int32_t AI::get_type_id() const
{
	return 0;
}

std::string AI::get_motion_str() const
{
	if(motion_node_)
	{
		return motion_node_->get_range_str();
	}
	else
	{
		return "no";
	}
}

void AI::set_speed(float _walk_speed, float _run_speed, float _rush_speed)
{
	walk_speed_ = _walk_speed /THOUSAND;
	run_speed_ = _run_speed /THOUSAND;
	rush_speed_ =  _rush_speed /THOUSAND;
}

float AI::get_speed(ANI_TYPE _ani_type)
{
    if (_ani_type == SHARE_ANI_RUN_MASTER || _ani_type == SHARE_ANI_RUSH_MASTER)
    {
        Ctrl* master = get_master();
        if (master != NULL)
        {
            float m_speed = get_master_speed(master);
            if (_ani_type == SHARE_ANI_RUSH_MASTER)
            {
                rush_master_rate_ = get_dis_of_master(master) / get_out_range();
                return (m_speed + 2 * rush_master_rate_ ) / THOUSAND; 
            }
            else
            {
                return (m_speed + 1) / THOUSAND;
            }
        }
    }

	float speed = static_cast<Spirit*>(owner_)->get_speed();
	if(_ani_type < SHARE_ANI_RUN)
	{
		return walk_speed_ * speed;
	}
	else if (_ani_type < SHARE_ANI_RUSH)
	{
		return run_speed_ * speed;
	}
	else
	{
		return rush_speed_ * speed;
	}
}

float AI::get_owner_speed() const
{
    Spirit* owner = static_cast<Spirit*>(owner_);
	return owner->get_speed();
}

void AI::tgt_check_exist()
{
	target_ = target_id_ ? g_worldmng->get_ctrl(target_id_) : NULL;
// 	if (target_id_ && (!target_ || get_scene() != tgt_get_scene()))
// 	{
// 		uint32_t scene_id = 0;
// 		if (get_scene())
// 		{
// 			scene_id = get_scene()->get_id();
// 		}
// 		uint32_t tgt_scene_id = 0;
// 		if(tgt_get_scene())
// 		{
// 			tgt_scene_id = tgt_get_scene()->get_id();
// 		}
// 
// 		E_rR(2)("invalid ai target! tgtId=%lu, tgt=%p, scnId=%lu, tgtScnId=%lu, aiId=%d, template=%s", 
// 			target_id_, target_, scene_id, tgt_scene_id, get_ai_iD(), get_template_name().c_str());
// 
// 		target_id_ = 0;
// 	}
}

void AI::tgt_safe_set(Obj* _target)
{
	last_target_id_ = target_ ? static_cast<Ctrl*>(target_)->get_id() : 0;
	target_ = _target;
	target_id_ = _target ? static_cast<Ctrl*>(_target)->get_id() : 0;
}

float AI::get_bounding_radius() const
{
	if(target_)
	{
		return get_obj_radius(target_);
	}
	else
	{
		return DEFAULT_BOUNDING_RADIUS;
	}
}

float AI::get_facing_radius() const
{
	return DEFAULT_BOUNDING_RADIUS;
}

float AI::get_melee_offset() const
{
	return melee_offset_;
}

bool AI::query_melee_range()
{
	return tgt_get_dist(NULL) <= get_melee_range();
}

bool AI::query_range(RANGE_TYPE _range_type, bool _is_in, const VECTOR3& _tar_pos) const
{
	const CRange *range = NULL;

	switch(_range_type)
	{
	case RANGE_SENSE:
		range = motion_range_idx_ < (int)motion_sectors_.size() ? &motion_sectors_[motion_range_idx_] : NULL;
		break;

	case RANGE_CHASE:
		range = motion_range_idx_ < (int)motion_circles_.size() ? &motion_circles_[motion_range_idx_] : NULL;
		break;

	case RANGE_NEAR_WALK:
		range = motion_range_idx_ < (int)near_walks_.size() ? &near_walks_[motion_range_idx_] : NULL;
		break;
    case RANGE_VIEW_ANGLE:
    case RANGE_SIGHT:
    case RANGE_MOTION:
    case RANGE_LENGTH:
    case RANGE_COMMON:
    case RANGE_LISTEN:
    default:
        break;
	}

	if(range)
	{
		if (_is_in)
		{
			return range->is_in_range(_tar_pos);
		}
		else
		{
			return range->is_out_range(_tar_pos);
		}
	}
	else
	{
		return !_is_in;
	}
}

void AI::set_extra_skills(void* _skills)
{
// 	int size = pyList__size(skills);
// 	extra_skills_.clear();

// 	for (int i = 0; i < size; ++i)
// 	{
// 		PyObject *num = py_sequence__get_item(skills, i);
// 		extra_skills_.push_back(static_cast<uint32_t>(pyLong__asLong(num)));
// 		py__x_dE_c_r_eF(num);
// 	}
}

void AI::get_float_list( const string& _val, std::vector<float>& _values ) /*1 */
{
	std::string value(_val);
	_values.clear();
	std::vector<std::string> string_list;

	if(value.find("(") != string::npos || value.find("[") != string::npos || value.find("{") != string::npos)
	{
		str_split(str_trim_bracket(&value), string_list);
	}
	else
	{
		string_list.push_back(value);
	}

	for(uint32_t i = 0; i < string_list.size(); ++i)
	{
		_values.push_back(atof(string_list[i].c_str()));
	}
}

int AI::c_set_rare_print( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	rare_print_ = lua_tointeger( _state, 1 );
	return 0;
}

int AI::c_set_verbose_print( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	verbose_print_ = lua_tointeger( _state, 1 );
    threat_mgr_.ai_debug_ = verbose_print_ > 0;
	return 0;
}

int AI::c_set_spawn_point( lua_State* _state )
{
	lcheck_argc( _state, 2 );
	spawn_point_.x = lua_tonumber( _state, 1 );
	spawn_point_.z = lua_tonumber( _state, 2 );
	spawn_angle_ = owner_->get_angle_y();
	return 0;
}

int AI::c_set_angle_range( lua_State* _state )
{
	lcheck_argc( _state, 3 );
	angle_max_ = lua_tonumber( _state, 1 );
	angle_min_ = lua_tonumber( _state, 2 );
	cannot_turn_ = lua_tonumber( _state, 3 );
	return 0;
}

int AI::c_leave_master( lua_State* _state)
{
    lcheck_argc( _state, 0 );
    master_id_ = 0;
    return 0;
}

int AI::broadcast_mst_navigation( int _add, uint32_t _obj_id, ANI_TYPE _ani_type, float _dir_angle, const Navmesh::Path &_path, TURN_TYPE _turn_type, float _rush_rate )
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "broadcast_mst_navigation" );
	lua_pushboolean( L, _add);
	lua_pushinteger( L, _obj_id);
	lua_pushinteger( L, _ani_type);
	lua_pushnumber( L, _dir_angle);

	lua_newtable(L);
	for(uint32_t i = 0; i < _path.size(); ++i)
	{
		lua_newtable(L);
		lua_pushnumber(L, _path[i].x);
		lua_rawseti(L, -2, 1);
		lua_pushnumber(L, _path[i].z);
		lua_rawseti(L, -2, 2);
		lua_rawseti(L, -2, i+1);
	}

	lua_pushinteger( L, _turn_type);
    lua_pushnumber( L, _rush_rate);

	if( llua_call( L, 8, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
	return _add;
}

int AI::broadcast_mst_sfx(int32_t _sfx_id, int32_t _is_open, uint32_t _obj_id)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );

	lua_pushstring( L, "broadcast_mst_sfx" );
	lua_pushinteger( L, _obj_id);
	lua_pushinteger( L, _sfx_id);
	lua_pushinteger( L, _is_open );
	if( llua_call( L, 4, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
	return 0;
}

int AI::broadcast_mst_stand( int _add, uint32_t _obj_id, const string& _ani_name, float _x, float _z, ACT_TYPE _act_type, TURN_TYPE _turn_type, uint32_t _tar_id, float _dir_radian)
{
	if (_add == 0 && !can_broadcast_msg(_act_type, _turn_type))//12
	{
		return _add;
	}

	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid());
	lua_pushstring( L, "broadcast_mst_stand" );
	lua_pushboolean( L, _add);
	lua_pushinteger( L, _obj_id);
	lua_pushstring( L, _ani_name.c_str());
	lua_pushnumber( L, _x);
	lua_pushnumber( L, _z);
	lua_pushinteger( L, _act_type);
	lua_pushinteger( L, _turn_type);
	lua_pushinteger( L, _tar_id);
	lua_pushnumber(L, _dir_radian);

	if( llua_call( L, 10, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
	return _add;
}

int AI::sync_mst_patrol_path( uint32_t _obj_id, const Navmesh::Path& _path ) const
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "sync_mst_patrol_path" );
    lua_pushinteger( L, _obj_id );
    lua_newtable(L);
    for (size_t i = 0; i < _path.size(); ++i)
    {
        lua_newtable( L );
        lua_pushnumber( L, _path[i].x);
		lua_rawseti(L, -2, 1);
		lua_pushnumber(L, _path[i].z);
		lua_rawseti(L, -2, 2);
		lua_rawseti(L, -2, i+1);
    }

	if( llua_call( L, 3, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
    return 0;
}


int AI::sync_mst_patrol_index( uint32_t _obj_id, uint32_t _index ) const
{
    LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );

    lua_pushstring( L, "sync_mst_patrol_index" );
    lua_pushinteger( L, _obj_id);
    lua_pushinteger( L, _index );
	if( llua_call( L, 3, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
    return 0;
}

void AI::on_obj_enter( Ctrl* _obj, bool _is_master_pass/*=false*/ )
{
    if (has_master())
    {
        if (!_is_master_pass || _obj->get_id() == static_cast<Ctrl*>(owner_)->get_id())
        {
            return;
        }
    }
	threat_mgr_.on_obj_enter(_obj);
}

int AI::c_on_obj_enter( lua_State* _state )
{
	lcheck_argc( _state, 2 );
	Ctrl * obj = static_cast<Ctrl*>(get_obj(lua_tointeger( _state, 1 )));
	if (is_invalid_obj(obj)) return 0;

    bool is_master_pass = lua_toboolean( _state, 2 ); 
    if (has_master())
    {
        if (!is_master_pass || obj->get_id() == static_cast<Ctrl*>(owner_)->get_id())
        {
            return 0;
        }
    }
    threat_mgr_.on_obj_enter(obj);
	return 0;
}

int AI::c_on_obj_leave( lua_State* _state )
{
	lcheck_argc( _state, 2 );
    uint32_t target_id = lua_tointeger( _state, 1 );
    bool is_master_pass = lua_toboolean( _state, 2 ); 
    if (has_master())
    {
        if (!is_master_pass || target_id == static_cast<Ctrl*>(owner_)->get_id())
        {
            return 0;
        }
    }
    threat_mgr_.on_obj_leave(target_id);
	return 0;
}

int AI::c_is_spawning( lua_State* _state )
{
	lcheck_argc( _state, 0 );
	int result = is_spawning() ? 1 : 0;;
	lua_pushnumber(_state, result);
	return 1;
}

bool AI::is_enemy( Obj *_obj ) const
{
	if(!valid_target(_obj))
	{
		return false;
	}

    if( only_id_ > 0 && is_only_target( _obj ) ) 
    {
        //设置了only_id只能选择宿主为目标
        return true;  
    }

	Spirit* ta = static_cast<Spirit*>(_obj);
	Spirit* ow = static_cast<Spirit*>(owner_);

	return  g_campmng->is_enemy(ta,ow);
}

bool AI::is_friend( Obj* _obj ) const
{
	if(!valid_target(_obj))
	{
		return false;
	}

    if( only_id_ > 0 && !is_only_target( _obj ) ) 
    {
        //设置了only_id只能选择宿主为目标
        return true;  
    }

	Spirit* ta = static_cast<Spirit*>(_obj);
	Spirit* ow = static_cast<Spirit*>(owner_);

	return  g_campmng->is_friend(ta,ow);
}

bool AI::is_only_target( Obj* _obj ) const
{
	if(!_obj->is_player() && !_obj->is_monster()) return false;
    Ctrl* other = (Ctrl*)_obj;
    return only_id_ == other->get_id() ;
}

void AI::try_enlarge_chase( Obj* _target, bool _default_aoi, float _dist /*= 0*/ )
{
	if(_target)
	{
		if(_dist == 0) _dist = dist_of_obj(_target);
		if(_default_aoi && _dist > get_aoi_radius()) return;
		set_chase_radius(_dist + 3 );
	}
}

bool AI::is_invalid_target( uint32_t _obj_id ) const
{
	Spirit *obj = static_cast<Spirit*>(g_worldmng->get_ctrl(_obj_id));

	if(is_invalid_obj(obj))
	{
		return true;
	}

	if(obj->state.is_state(STATE_DEAD) > 0)
	{
		return true;
	}

	if(obj->state.is_state(STATE_CINEMA) > 0)
	{
		return true;
	}

	if(obj->is_stealth())
	{
		return true;
	}

	if(obj->get_worldid() != static_cast<Spirit*>(owner_)->get_worldid() || obj->get_plane() != static_cast<Spirit*>(owner_)->get_plane())
	{
		return true;
	}

	return false;
}

bool AI::is_owner_in_state(uint32_t _state_idx) const 
{
	Spirit* owner = static_cast<Spirit*>(owner_);
	return owner->state.is_state(_state_idx) > 0;
}

bool AI::obj_query_dist( float _range_dist, bool _is_in, Obj* _obj )
{
	if(is_invalid_obj(_obj)) return false;

	bool ret = vc3_len(get_pos() - get_obj_pos(_obj)) <= _range_dist*_range_dist;
	if(!_is_in)
	{
		ret = !ret;
	}
	return ret;
}

Obj* AI::get_obj( uint32_t _obj_pid ) const
{
	return g_worldmng->get_ctrl(_obj_pid);
}

bool AI::obj_out_chase_radius( uint32_t _obj_id )
{
	if (motion_node_)
	{
        return obj_query_dist(motion_node_->get_chase_radius(), false, get_obj(_obj_id));
	}
	else
	{
		return true;
	}
}

bool AI::valid_target( Obj* _obj ) const
{
	if(is_invalid_obj(_obj)) return false;
	if(!_obj->is_player() && !_obj->is_monster()) return false;
	if(is_invalid_target(static_cast<Ctrl*>(_obj)->get_id())) return false;
    return true;
}

bool AI::can_leave_fight() const
{
	return leave_fight_;
}

bool AI::obj_in_sight_radius( Obj* _obj )
{
    VECTOR3 obj_pos = get_obj_pos(_obj);
    return query_range(RANGE_SENSE, true, obj_pos);
}

void AI::find_chase_aoi_enemy()
{
	FOR_LINKMAP( get_world(), get_plane(), get_pos(), get_aoi_radius(), Obj::LinkPlayer )
	{
		if(is_enemy(obj))
		{
			threat_mgr_.add_threat(obj, VIEW_THREAT, false, true);
		}
	}
	END_LINKMAP

	FOR_LINKMAP( get_world(), get_plane(), get_pos(), get_aoi_radius(), Obj::LinkDynamic )
	{
		if(is_enemy(obj))
		{
			threat_mgr_.add_threat(obj, VIEW_THREAT, false, true);
		}
	}
	END_LINKMAP
}

int AI::c_set_spawn_anim( lua_State* _state )
{
	lcheck_argc( _state, 2 );
	const char *anim = lua_tostring( _state, 1 );//todo
	float time = lua_tonumber(_state, 2);
	btree_.set_spawn_anim(anim, time);
	btree_.clear_cd(true);
	return 0;
}

void AI::l_stop()
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "stop_ai" );
	lua_pushinteger( L, get_id());
	lua_pushboolean( L, is_good() ? 0 : 1);
	if( llua_call( L, 3, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}

void AI::l_set_spawn_id(int32_t _spawn_id)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "set_spawn_id" );
	lua_pushinteger( L, get_id());
	lua_pushinteger( L, _spawn_id);

	if( llua_call( L, 3, 0, 0 ) )      llua_fail( L, __FILE__, __LINE__ );
}

int32_t AI::l_get_ani_id( const string& _anim_name )
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "get_ani_id" );
	lua_pushstring( L, _anim_name.c_str());

	if( llua_call( L, 2, 1, 0 ) == 0) 
	{
		float index = lua_tointeger( L, -1 );
		lua_pop( L, 1 );    
		return index;
	} 
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return 1.f;
	}  
}

bool AI::l_get_anim_time( const string& _anim_name, float& _time )
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "ai_get_anim_time" );
	lua_pushinteger( L, get_id());
	lua_pushstring( L, _anim_name.c_str());

	if ( llua_call( L, 3, 2, 0 ) == 0 ) 
	{
		_time = lua_tonumber( L, -1 );
		bool is_succ = lua_toboolean( L, -2 );
		lua_pop( L, 2 ); 
		return is_succ;
	} 
	else 
	{    
		llua_fail( L, __FILE__, __LINE__ );
        _time = 0.f;
        return false;
	} 
}

int32_t AI::l_get_trigger_cur_state( uint32_t _obj_id )
{
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* state = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( state, "get_trigger_cur_state" );
    lua_pushnumber( state, _obj_id );
    
    if (llua_call( state, 2, 1, 0))
    {
        llua_fail( state, __FILE__, __LINE__ ); 
        return TRIGGER_STATE_INVALID;
    }

    int32_t cur_state = lua_tointeger( state, -1 );
    lua_pop( state, 1 );
    return cur_state;
}

bool AI::l_get_ai_board_value( const string& _key, int32_t& _value, int32_t _independ ) 
{
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* state = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( state, "get_ai_board_value" );
	lua_pushinteger( state, get_id() );
    lua_pushstring( state, _key.c_str() );
    lua_pushinteger( state, _independ );
    
    if ( llua_call( state, 4, 2, 0 ) )
    {
        llua_fail( state, __FILE__, __LINE__ ); 
        _value = 0;
        return false;
    }

    _value = lua_tointeger( state, -1 );
    bool is_succ = lua_toboolean( state, -2 );
    lua_pop( state, 2 );
    return is_succ;
}

bool AI::l_set_ai_board_value( const string& _key, int32_t _value, int32_t _independ )
{
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* state = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring( state, "set_ai_board_value" );
	lua_pushinteger( state, get_id() );
    lua_pushstring( state, _key.c_str() );
    lua_pushinteger( state, _value );
    lua_pushinteger( state, _independ );
    
    if ( llua_call( state, 5, 1, 0 ) )
    {
        llua_fail( state, __FILE__, __LINE__ ); 
        return false;
    }

    bool is_succ = lua_toboolean( state, -1 );
    lua_pop( state, 1 );
    return is_succ;
}

int AI::c_add_spawn_node( lua_State* _state )
{
	if(!ai_id_ || !is_good())
	{
		return 0;
	}

	lcheck_argc( _state, 2 );
	const char * anim = lua_tostring(_state, 1);
	float time = lua_tonumber(_state, 2);
	btree_.add_spawn_node(anim, time, false, false);
	return 0;
}

void AI::l_set_skill( int32_t _skill_id, float _skill_cd )
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "set_skill" );
	lua_pushinteger( L, get_id());
	lua_pushinteger( L, _skill_id);
	lua_pushnumber( L, _skill_cd);

	if( llua_call( L, 4, 0, 0 )) 
	{
		llua_fail( L, __FILE__, __LINE__ );
	} 
}

int AI::l_skill_can_use( int32_t _skill_id)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "skill_can_use" );
	lua_pushinteger( L, get_id());
	lua_pushinteger( L, _skill_id);
	lua_pushinteger( L, get_frame_end_time());

	if( llua_call( L, 4, 1, 0 ) == 0) 
	{
		int flag = lua_toboolean( L, -1 );
		lua_pop( L, 1 );    
		return flag;
	} 
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return false;
	}  
}

Obj* AI::tgt_get( const BTLeafNode* _node ) const
{
	if(_node && _node->target_last())
	{
		return get_last_one();
	}
	else
	{
		return target_;
	}
}

int AI::l_get_plane_monster(int _type_id )
{
	int scene_id = get_world()->get_id();
	int plane_id = get_plane();

	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	
	lua_pushstring( L, "get_plane_monster" );
	lua_pushinteger( L, scene_id );
	lua_pushinteger( L, plane_id );
	lua_pushinteger( L, _type_id );

	if( llua_call( L, 4, 1, 0 ) == 0) 
	{
		int obj_id = lua_tointeger( L, -1 );
		lua_pop( L, 1 );
		return obj_id;
	} 
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return false;
	}  
}

int AI::c_tgt_get_id( lua_State* _state )
{
	lcheck_argc( _state, 0 );
	lua_pushnumber(_state, tgt_get_id(NULL));
	return 1;
}

void AI::self_combat_end() const
{
	if(!is_dead())
	{
		LuaServer* lua_svr = Ctrl::get_lua_server();
		lua_State* L = lua_svr->L();
		lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
		lua_pushstring( L, "mst_leave_fight" );
		lua_pushinteger( L, get_id());

		if( llua_call( L, 2, 0, 0 )) 
		{ 
			llua_fail( L, __FILE__, __LINE__ );
		}  
	}
}

void AI::set_client_angle(float _angle)
{
	if (cannot_turn_)
	{
		return;
	}
	if (angle_max_ != 0 || angle_min_ != 0)
	{
		if (_angle > angle_min_ && _angle < angle_max_)
		{
			if (angle_max_ - _angle > _angle - angle_min_)
			{
				_angle = angle_min_;
			}
			else
			{
				_angle = angle_max_;
			}
		}
	}
	owner_->set_angle_y(180-_angle);
}

bool AI::is_spawning()
{
	return (btree_.get_current_node()->get_name() == "spawn");
}

bool AI::l_get_plane_property( const string& _key , int& _value )
{
	string value_str ;
	bool succ = l_get_plane_property(_key,value_str);
	if(succ)
	{
		_value = atoi( value_str.c_str());
	}
	return succ;
}

bool AI::l_get_plane_property( const string& _key , float& _value )
{
	string value_str ;
	bool succ = l_get_plane_property(_key,value_str);
	if(succ)
	{
		_value = atof( value_str.c_str());
	}
	return succ;
}

bool AI::l_get_plane_property( const string& _key , string& _value )
{
	if(_key.empty() )
		return false;

	int scene_id = get_world()->get_id();
	int plane_id = get_plane();


	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "get_plane_property" );
	lua_pushinteger( L, scene_id );
	lua_pushinteger( L, plane_id );
	lua_pushstring( L, _key.c_str() );

	if( llua_call( L, 4, 2, 0 ) == 0) 
	{
		string tmp = lua_tostring( L, -1 );
		bool ok = lua_tointeger( L, -2 ) == 0;
		lua_pop( L, 2 );

		if(ok)
		{
			_value = tmp;
		}
		return ok;
	} 
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return false;
	}  
}



bool AI::l_get_table_field( int& _value , const string& _table , const string& _one , const string& _two/*="" */, const string& _three/*=""*/ )
{
	string value_str ;
	bool succ = l_get_table_field(value_str ,_table,_one,_two,_three );
	if(succ)
	{
		_value = atoi( value_str.c_str());
	}
	return succ;
}

bool AI::l_get_table_field( float& _value, const string& _table , const string& _one , const string& _two/*="" */, const string& _three/*=""*/ )
{
	string value_str ;
	bool succ = l_get_table_field(value_str ,_table,_one,_two,_three );
	if(succ)
	{
		_value = atof( value_str.c_str());
	}
	return succ;
}

bool AI::l_get_table_field( string& _value, const string& _table , const string& _one , const string& _two/*="" */, const string& _three/*=""*/ )
{
	if( _table.empty() || _one.empty() )
		return false;
	
	int input_conut = 3;

	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "get_table_field" );
	lua_pushstring( L, _table.c_str() );
	lua_pushstring( L, _one.c_str() );
	if(! _two.empty())
	{
		lua_pushstring( L, _two.c_str() );
		input_conut = 4;
	}
	if(! _three.empty())
	{
		lua_pushstring( L, _three.c_str() );
		input_conut = 5;
	}

	if( llua_call( L, input_conut, 2, 0 ) == 0) 
	{
		string tmp = lua_tostring( L, -1 );
		bool ok = lua_tointeger( L, -2 ) == 0;
		lua_pop( L, 2 );

		if(ok)
		{
			_value = tmp;
		}
		return ok;
	} 
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return false;
	}
}



bool AI::is_super_armor() const
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "is_super_armor" );
	lua_pushinteger( L, get_id());

	if( llua_call( L, 2, 1, 0 ) == 0) 
	{
		int flag = lua_toboolean( L, -1 );  
		return flag == 1;
	} 
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return false;
	}  
}

void AI::l_do_monster_drop( bool _need_find_player, uint32_t _obj_id, uint32_t _tgt_id, float _drop_dist )
{
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* state = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( state, "do_monster_drop" );
    lua_pushboolean( state, _need_find_player ); 
    lua_pushinteger( state, _obj_id );
    lua_pushinteger( state, _tgt_id );
    lua_pushnumber( state, _drop_dist );

    if ( llua_call( state, 5, 0, 0 ) ) 
    {
		llua_fail( state, __FILE__, __LINE__ );
    }
}

void AI::l_update_mst_fight_attr_fit_pl( uint32_t _monster_id, const vector<uint32_t>& _player_li )
{
    int player_cnt = _player_li.size();
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* state = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( state, "update_mst_fight_attr_fit_pl" );
    lua_pushinteger( state, _monster_id );
    
    for(int i = 0; i < player_cnt; ++i)
    {
        lua_pushinteger( state, _player_li[i] );
    }

    if ( llua_call( state, 2+player_cnt, 0, 0 ) ) 
    {
		llua_fail( state, __FILE__, __LINE__ );
    }
}

void AI::l_update_mst_fight_attr_fit_host( uint32_t _monster_id )
{
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* state = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( state, "update_mst_fight_attr_fit_host" );
    lua_pushinteger( state, _monster_id );
    
    if ( llua_call( state, 2, 0, 0 ) ) 
    {
		llua_fail( state, __FILE__, __LINE__ );
    }
}


void AI::l_set_obj_god_mode( uint32_t _obj_id, bool _is_god )
{
    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* state = lua_svr->L();
    lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( state, "set_obj_god_mode" );
    lua_pushinteger( state, _obj_id );
    lua_pushboolean( state, _is_god ); 

    if ( llua_call( state, 3, 0, 0 ) ) 
    {
		llua_fail( state, __FILE__, __LINE__ );
    }
}

void AI::clear_last()
{
	last_.clear();
}

const std::vector<uint32_t> & AI::get_last() const
{
	return last_;
}

Obj* AI::get_last_one() const
{
	Obj* target = NULL;
	const std::vector<uint32_t> & last = get_last();

	if(!last.empty())
	{
		if(last.size() > 1)
		{
			uint32_t target_id = last[rand_int(0, last.size() - 1)];
			target = g_worldmng->get_ctrl(target_id);
			std::vector<uint32_t> new_last;
			new_last.push_back(target_id);
			set_last(new_last);
		}
		else
		{
			target = g_worldmng->get_ctrl(last[0]);
		}
	}
	return is_valid_obj(target) ? target : tgt_get(NULL);
}

void AI::set_last(const std::vector<uint32_t>& _last) const
{
	last_ = _last;
}

VECTOR3 AI::tgt_get_pos( const BTLeafNode* _node ) const
{
	if(_node && _node->target_last() && get_last_one())
	{
		return get_obj_pos(get_last_one());
	}
	else if(_node && _node->target_master() && get_master())
	{
		return get_master()->getpos();
	}
	else if(target_)
	{
		return target_->getpos();
	}
	else
	{
		Obj *last_target = g_worldmng->get_ctrl(last_target_id_);
		if(is_valid_obj(last_target))
		{
			return last_target->getpos();
		}
		return get_pos();
	}
}

bool AI::is_obj_in_buff( uint32_t _obj_id, const vector<float>& _buff_ids, int32_t _is_buff_id ) const
{
    if (_buff_ids.size() == 0)
       return false;

    LuaServer* lua_svr = Ctrl::get_lua_server();
    lua_State* state = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
    lua_pushstring(state, "is_in_buff");
    lua_pushinteger(state, _obj_id);
    lua_pushinteger(state, _is_buff_id);
    
    lua_newtable(state);
    vector<float>::const_iterator itr;
    uint32_t i = 0;
    for (itr = _buff_ids.begin(); itr != _buff_ids.end(); ++itr)
    {
	    lua_pushnumber(state, ++i);
	    lua_pushnumber(state, (int32_t)*itr);
	    lua_settable(state, -3);
    }

    if (llua_call(state, 4, 1, 0) == 0)
    {
		bool ret = lua_toboolean(state, -1);
		lua_pop(state, 1);    
		return ret;
    }
	else 
	{     
		llua_fail(state, __FILE__, __LINE__);
		return false;
	}
}

int AI::is_in_buff_group( uint32_t _obj_id, int32_t _group_id ) const
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "is_in_buff_group" );
	lua_pushinteger( L, _obj_id);
	lua_pushinteger( L, _group_id);

	if( llua_call( L, 3, 1, 0 ) == 0) 
	{
		int flag = lua_toboolean( L, -1 );
		lua_pop( L, 1 );    
		return flag;
	} 
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return 0;
	}
}

int AI::c_set_patrol_path( lua_State* _state )//1
{
	lcheck_argc( _state, 4 );
	set_spawn_flag(SHARE_AI_SPAWN_PATROL);
	bool loop = lua_toboolean(_state, 1);
	bool not_find_path = lua_toboolean(_state, 2);
	int32_t ani_type = lua_tointeger(_state, 3);
    bool broadcast = lua_toboolean(_state, 4);
    set_path(path_, loop, not_find_path, ani_type, &patrol_info_, broadcast);
	return 0;
}

bool AI::find_path_force( const VECTOR3& _start, const VECTOR3& _end, Navmesh::Path* _path )//12
{
    if (!find_path(_start, _end, _path, DOORFLAG_ALLPASS ) && _path )
	{
		_path->clear();
		_path->push_back(_start);
		_path->push_back(_end);
	}
	return true;
}

void AI::reset_spawn_point()
{
	spawn_point_ = get_pos();
}

float AI::get_in_range() const
{
    if (is_in_battle())
    {
        return bt_in_range_;
    }
    else
    {
        return nbt_in_range_;
    }
}

float AI::get_out_range() const
{
    if (is_in_battle())
    {
        return bt_out_range_;
    }
    else
    {
        return nbt_out_range_;
    }
}

float AI::get_dis_of_master(Ctrl* _master)
{
    return length2d(get_master_pos(_master) - get_pos());
}

//void AI::set_door_flag(unsigned short _door_flags)
//{
//    Ctrl* owner = static_cast<Ctrl*>(owner_);
//    owner->set_door_flags(_door_flags);
//}

void AI::get_move_pos( u_long _world_id, int32_t _plane_id, VECTOR3& _master_pos, float _master_angle, VECTOR3& _hit_pos, bool _gen_off_angle /*= true*/ )
{
    if (_gen_off_angle)
    {
        gen_off_angle();
    }

    float rad = radians(_master_angle + off_angle_);
    float range = get_in_range();

    VECTOR3 pos(_master_pos.x + range * cosf(rad), 0, _master_pos.z + range * sinf(rad));
    float hit_rate;
	World* world = g_worldmng->get_world( _world_id );
	unsigned short door_flags = world->get_door_flags(_plane_id);
    world->get_scene()->raycast(&_master_pos, &pos, &_hit_pos, &hit_rate, door_flags);
    _hit_pos.y = 0;

    if (_gen_off_angle)
    {
       reset_off_angle(); 
    }
}

VECTOR3 AI::get_follow_pos( Ctrl* _master, bool _gen_off_angle /*= true*/ )
{
    VECTOR3 pos;
    Spirit* master = static_cast<Spirit*>(_master);
    u_long world_id = master->get_worldid();
    int32_t plane_id = master->get_plane();
    VECTOR3 m_pos = master->getpos();
    float m_angle = master->get_angle_y();
    get_move_pos(world_id, plane_id, m_pos, m_angle, pos, _gen_off_angle);
    return pos;
}

void AI::gen_off_angle()
{
    uint32_t sign = rand_int(0, 1);
    sign = sign == 0 ? -1 : 1;
    off_angle_ = rand_float(15, 105) * sign;
}

float AI::get_off_angle()
{
    return off_angle_;
}

void AI::reset_off_angle()
{
    off_angle_ = DEFAULT_OFF_ANGLE;
}

float AI::get_master_speed(Ctrl* _master)
{
    Spirit* master = static_cast<Spirit*>(_master); 
    return master->get_speed();
}

int AI::c_set_follow_pos(lua_State *_state)
{ 
	lcheck_argc(_state, 5);
    u_long world_id = (u_long)lua_tointeger(_state, 1); 
    int32_t plane_id = lua_tointeger(_state, 2);
    float x = lua_tonumber(_state, 3);
    float z = lua_tonumber(_state, 4);
    float angle = lua_tonumber(_state, 5);

    VECTOR3 pos;
    VECTOR3 master_pos(x, 0, z);
    get_move_pos(world_id, plane_id, master_pos, angle, pos);
    
    Ctrl* owner = static_cast<Ctrl*>(owner_);
    btree_.reset(true);
	resume_time_ = cur_time_ + THOUSAND;
    owner->replace(world_id, pos, plane_id);

    return 0;
}

int AI::c_set_path( lua_State* _state )
{
	lcheck_argc( _state, 5 );
	float x = lua_tonumber(_state, 1);
	float z = lua_tonumber(_state, 2);
	string anim_name = lua_tostring(_state, 3);
	float anim_time = lua_tonumber(_state, 4);
	int32_t chat_id = lua_tointeger(_state, 5);
	path_.push_back(VECTOR3(x, 0, z));
	if (!anim_name.empty())
	{
		patrol_info_.push_back(PATROL_INFO(path_.back(), anim_name, anim_time, chat_id));
	}
	return 0;
}

int AI::c_set_cond_buff( lua_State* _state )
{
	lcheck_argc( _state, 4 );
	buff_min_time_ = lua_tonumber(_state, 1);
	buff_mst_dist_ = lua_tonumber(_state, 2);
	buff_mst_num_ = lua_tointeger(_state, 3);
	buff_del_id_ = lua_tointeger(_state, 4);
	return 0;
}

int AI::c_get_op_trigger( lua_State* _state )
{
	lcheck_argc( _state, 0 );
	vector<int>& idx = btree_.get_chunk_idx();

	if (idx.size() > 0)
	{
		size_t i = 0;
		for (; i < idx.size(); i++)
		{
			lua_pushnumber(_state, idx[i]);
		}
		return i;
	}
	
	return 0;
}

int AI::c_get_cur_node_name( lua_State* _state)
{
	lcheck_argc( _state, 0 );
	BTNode* node = btree_.get_current_node();
    if(node == NULL)
    {
        lua_pushstring(_state, "");
    }
    else
    {
        lua_pushstring(_state, node->get_name().c_str());
    }
    return 1;
}

void AI::del_cond_buff()
{
	if(buff_del_time_ && cur_time_ > buff_del_time_)
	{
		int32_t num = 0;
		FOR_LINKMAP(get_world(), get_plane(), get_pos(), get_aoi_radius(), Obj::LinkDynamic )//12
		{
			if(check_obj(obj, OT_MONSTER, true))
			{
				if(dist_of_obj(obj) <= buff_mst_dist_)
				{
					num += 1;
				}
			}
		}
		END_LINKMAP

		if(num - 1 <= buff_mst_num_)
		{
			buff_del_time_ = 0;

			LuaServer* lua_svr = Ctrl::get_lua_server();
			lua_State* L = lua_svr->L();
			lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
			lua_pushstring( L, "use_buff" );
			lua_pushinteger( L, get_id());
			lua_pushinteger( L, buff_del_id_);
			lua_pushboolean( L, 0);
			lua_pushnumber(L, 0);

			if( llua_call( L, 5, 0, 0 )) 
			{
				llua_fail( L, __FILE__, __LINE__ );
			}
		}
	}
}

VECTOR3 AI::get_angle_vec() const
{
	return GET_DIR(owner_->get_angle_y());
}

VECTOR3 AI::get_forward_angle_vec() const
{
	return GET_DIR(this->forward_angle_);
}

std::string AI::get_ai_info( const std::string& _key ) const
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "get_ai_info" );
	lua_pushinteger( L, get_ai_id());
	lua_pushstring( L, _key.c_str());
	lua_pushinteger( L, get_id());

	if( llua_call( L, 4, 1, 0 ) == 0) 
	{
		const char *value = lua_tostring( L, -1 );
		lua_pop( L, 1 );  
		return value;
	} 
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return "";
	}
}

std::string AI::get_scene_plane_str() const
{
	int scene_id = get_world()->get_id();
	int plane_id = get_plane();
	std::stringstream ss;
	ss << scene_id;
	ss << "-";
	ss << plane_id;
	return ss.str();
}

int AI::c_is_running( lua_State* _state )
{
	lcheck_argc( _state, 0 );
	lua_pushboolean( _state, is_running_ );
	return 1;
}

int AI::c_get_prop_int( lua_State* _state )
{
	lcheck_argc( _state, 2 );
	int32_t node_id = lua_tointeger( _state, 1 );
	const char* key = lua_tostring( _state, 2 );

	int32_t _value = 0;

	BTNode *node = btree_.get_node(node_id);
	if(node && node->get_property_int(key, _value))
	{
		lua_pushinteger(_state, _value);
		return 1;
	}
	return 0;
}

int AI::c_get_tgt_id( lua_State* _state )//
{
	lcheck_argc( _state, 1 );
	int32_t node_id = lua_tointeger( _state, 1 );

	int32_t target_id = 0;

	BTNode *node = btree_.get_node(node_id);
	if(node)
	{
		BTLeafNode *lead_node = static_cast<BTLeafNode*>(node);
		target_id = tgt_get_id(const_cast<const BTLeafNode*>(lead_node));
	}
	lua_pushinteger( _state, target_id);
	return 1;
}

int AI::c_get_prop_float( lua_State* _state )
{
	lcheck_argc( _state, 2 );
	int32_t node_id = lua_tointeger( _state, 1 );
	const char* key = lua_tostring( _state, 2 );

	float _value = 0;

	BTNode *node = btree_.get_node(node_id);
	if(node && node->get_property_float(key, _value))
	{
		lua_pushnumber( _state, _value);
		return 1;
	}
	return 0;
}

int AI::c_get_prop_float_list( lua_State* _state )//12
{
	lcheck_argc( _state, 2 );
	int32_t node_id = lua_tointeger( _state, 1 );
	const char* key = lua_tostring( _state, 2 );

	std::vector<float> values;

	BTNode *node = btree_.get_node(node_id);
	if(node && node->get_property_float_list(key, values))
	{
// 		lua_newtable(L);
// 		for (uint32_t i = 0; i < _chunk_idx.size(); ++i)
// 		{
// 			lua_pushinteger(L, _chunk_idx[i]);
// 			lua_rawseti(L, -2, i+1);
// 		}
		return 1;
	}
	return 0;
}

int AI::c_assert_fail( lua_State* _state )//12
{
	lcheck_argc( _state, 2 );
	int32_t node_id = lua_tointeger( _state, 1 );
	const char* str = lua_tostring( _state, 2 );

	assert_fail(false);

	BTNode *node = btree_.get_node(node_id);
	if(node)
	{
		printf("c_assert_fail %d %s\n", node_id, str);
		node->errorf(str);
	}
	return 0;
}

int AI::c_set_extra_skill_id( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	extra_skill_id_ = lua_tointeger( _state, 1 );
	return 0;
}

int AI::c_add_cond_buff( lua_State* _state )
{
	lcheck_argc( _state, 0 );
	buff_del_time_ = cur_time_ + buff_min_time_ * 1000;
	return 0;
}

float AI::get_obj_dist( const VECTOR3 &_pos )
{
	return length2d(get_pos() - _pos);
}

int32_t AI::get_obj_camp( Obj *_obj ) const
{
	return static_cast<Spirit*>(_obj)->get_camp();
}

float AI::get_obj_pos_dist( Obj* _obj, const VECTOR3 &_pos )
{
	return length2d(_pos - get_obj_pos(_obj));
}

void AI::action_msg( const std::string& _action, AI_MSG _msg, int32_t _node_id ) const
{
	lua_State* L = ai_msg(_action, _msg, _node_id);
	if( llua_call( L, 5, 0, 0 )) 
	{
		llua_fail( L, __FILE__, __LINE__ );
	} 
}

bool AI::svo_msg( const std::string& _target_str, AI_MSG _msg, int32_t _node_id, const std::string & _event, const std::string & _range, 
	const std::vector<int32_t> &_targets, const std::vector<float> &_ranges ) const
{
	lua_State* L = ai_msg("svo_" + _target_str, _msg, _node_id);
	lua_pushstring( L, _event.c_str());
	lua_pushstring( L, _range.c_str());

	lua_newtable(L);
	for (uint32_t i = 0; i < _targets.size(); ++i)
	{
		lua_pushinteger(L, _targets[i]);
		lua_rawseti(L, -2, i+1);
	}

	lua_newtable(L);
	for (uint32_t i = 0; i < _ranges.size(); ++i)
	{
		lua_pushnumber(L, _ranges[i]);
		lua_rawseti(L, -2, i+1);
	}

	if( llua_call( L, 9, 1, 0 ) == 0) 
	{
		bool ret = lua_toboolean( L, -1 );
		lua_pop( L, 1 );    
		return ret;
	} 
	else 
	{     
		llua_fail( L, __FILE__, __LINE__ );
		return false;
	}  
}

lua_State* AI::ai_msg( const std::string& _action, AI_MSG _msg, int32_t _node_id ) const
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* L = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );
	lua_pushstring( L, "ai_msg" );
	lua_pushinteger( L, get_id());
	lua_pushstring( L, _action.c_str());
	lua_pushinteger( L, _msg);
	lua_pushinteger( L, _node_id);
	return L;
}

int AI::c_set_last( lua_State* _state )
{
	lcheck_argc( _state, 1 );

	last_.clear();
	WHILE_TABLE(_state)
		last_.push_back(lua_tointeger(_state, -1));
	END_WHILE(_state)

	return 0;
}

int AI::c_get_last( lua_State* _state )
{
	lcheck_argc( _state, 0 );

	lua_newtable(_state);
	for (uint32_t i = 0; i < last_.size(); ++i)
	{
		lua_pushinteger(_state, last_[i]);
		lua_rawseti(_state, -2, i+1);
	}
	return 1;
}

int AI::c_get_threat_list( lua_State* _state ) 
{
    lcheck_argc( _state, 0 );
    threat_mgr_.get_threat_list_to_lua( _state ); 
    return 1; 
}

int AI::c_add_threat( lua_State* _state )
{
    lcheck_argc( _state, 3);
    uint32_t target_id = (uint32_t)lua_tonumber(_state, 1);
    int threat_type = (int)lua_tonumber( _state, 2);
    float threat = (float)lua_tonumber( _state, 3);
    Ctrl* target = g_worldmng->get_ctrl( target_id );
    if( target ) 
    {
        threat_mgr_.add_target_threat( target, (enum ThreatType)threat_type, threat );
    }
    return 0;
}

int AI::c_reset_threat( lua_State* _state )
{
    lcheck_argc( _state, 1);
    uint32_t target_id = (uint32_t)lua_tonumber( _state, 1 );
    Ctrl* target = g_worldmng->get_ctrl( target_id );
    if( target ) 
    {
        threat_mgr_.reset_threat( target );
    }
    return 0;
}

int AI::c_set_threat( lua_State* _state )
{
    lcheck_argc( _state, 3 );
    uint32_t target_id = (uint32_t)lua_tonumber(_state, 1);
    int threat_type = (int)lua_tonumber( _state, 2);
    float threat = (float)lua_tonumber( _state, 3);
    Ctrl* target = g_worldmng->get_ctrl( target_id );
    if( target ) 
    {
        threat_mgr_.set_threat( target, (enum ThreatType)threat_type, threat );
    }
    return 0;
}

int AI::c_set_only_target( lua_State* _state )
{
    lcheck_argc( _state, 1 );
    OBJID objid = (OBJID)lua_tonumber(_state, 1);
    only_id_ = objid;
    return 0;
}

int AI::c_set_forward_angle( lua_State* _state )
{
    lcheck_argc( _state, 1 );
    float angle = (float)lua_tonumber( _state, 1 );
    this->forward_angle_ = angle;
    return 0;
}

bool AI::l_has_monster_finished_spawn(uint32_t _obj_id, bool& _is_finished)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* state = lua_svr->L();
	lua_svr->get_lua_ref(Ctrl::get_lua_regid());
	lua_pushstring(state, "has_monster_finished_spawn");
	lua_pushinteger(state, _obj_id);

	if (llua_call(state, 2, 2, 0 ) == 0) 
	{
        _is_finished = lua_toboolean(state, -1);
        bool is_succ = lua_toboolean(state, -2);
		lua_pop(state, 2);    
        return is_succ;
	} 
	else 
	{     
		llua_fail(state, __FILE__, __LINE__);
        return false;
	} 
}

bool AI::l_get_rage_value(float& _rage_value)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* state = lua_svr->L();
	lua_svr->get_lua_ref(Ctrl::get_lua_regid());
	lua_pushstring(state, "get_rage_value");
	lua_pushinteger(state, get_id());

	if (llua_call(state, 2, 2, 0 ) == 0) 
	{
        _rage_value = lua_tonumber(state, -1);
        bool is_succ = lua_toboolean(state, -2);
		lua_pop(state, 2);    
        return is_succ;
	} 
	else 
	{     
		llua_fail(state, __FILE__, __LINE__);
        return false;
	} 
}

bool AI::l_get_continue_skill_id(int32_t _skill_id, int32_t& _continue_skill_id)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* state = lua_svr->L();
	lua_svr->get_lua_ref(Ctrl::get_lua_regid());
	lua_pushstring(state, "get_continue_skill_id");
	lua_pushinteger(state, _skill_id);

	if (llua_call(state, 2, 2, 0 ) == 0) 
	{
        _continue_skill_id = lua_tointeger(state, -1);
        bool has_continue_skill = lua_toboolean(state, -2);
		lua_pop(state, 2);    
        return has_continue_skill;
	} 
	else 
	{     
		llua_fail(state, __FILE__, __LINE__);
        return false;
	} 
}

bool AI::l_get_charge_skill_id(int32_t _skill_id, const vector<int32_t>& _charge_time_range, float& _charge_time, int32_t& _charge_skill_id)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* state = lua_svr->L();
	lua_svr->get_lua_ref(Ctrl::get_lua_regid());
	lua_pushstring(state, "get_charge_skill_id");
	lua_pushinteger(state, _skill_id);
    lua_pushinteger(state, _charge_time_range[0]);
    lua_pushinteger(state, _charge_time_range[1]);

	if (llua_call(state, 4, 3, 0 ) == 0) 
	{
        _charge_skill_id = lua_tointeger(state, -1);
        _charge_time = lua_tonumber(state, -2);
        bool is_succ = lua_toboolean(state, -3);
		lua_pop(state, 3);    
        return is_succ;
	} 
	else 
	{     
		llua_fail(state, __FILE__, __LINE__);
        return false;
	} 
}

void AI::l_play_charge_skill_idle_anim(const VECTOR3 _pos)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* state = lua_svr->L();
	lua_svr->get_lua_ref(Ctrl::get_lua_regid());
	lua_pushstring(state, "play_charge_skill_idle_anim");
	lua_pushinteger(state, get_id());
	lua_pushnumber(state, _pos.x);
	lua_pushnumber(state, _pos.z);

	if (llua_call(state, 4, 0, 0 )) 
	{
		llua_fail(state, __FILE__, __LINE__);
	} 
}

void AI::l_broadcast_turn_to_skill_angle(float _target_angle)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* state = lua_svr->L();
	lua_svr->get_lua_ref(Ctrl::get_lua_regid());
	lua_pushstring(state, "broadcast_turn_to_skill_angle");
	lua_pushinteger(state, get_id());
	lua_pushnumber(state, _target_angle);

	if (llua_call(state, 3, 0, 0 )) 
	{
		llua_fail(state, __FILE__, __LINE__);
	} 
}

bool AI::l_broadcast_chase_skill_move(int32_t _is_serialize, const Navmesh::Path &_path, float _chase_speed)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* state = lua_svr->L();
	lua_svr->get_lua_ref(Ctrl::get_lua_regid());
	lua_pushstring(state, "broadcast_chase_skill_move");
	lua_pushinteger(state, _is_serialize);
	lua_pushinteger(state, get_id());
	lua_pushnumber(state, _chase_speed);

	lua_newtable(state);
	for (uint32_t i = 0; i < _path.size(); ++i)
	{
		lua_newtable(state);
		lua_pushnumber(state, _path[i].x);
		lua_rawseti(state, -2, 1);
		lua_pushnumber(state, _path[i].z);
		lua_rawseti(state, -2, 2);
		lua_rawseti(state, -2, i+1);
	}

	if (llua_call(state, 5, 1, 0 ) == 0) 
	{
        bool is_succ = lua_toboolean(state, -1);
		lua_pop(state, 1);    
        return is_succ;
    }
    else
    {
		llua_fail(state, __FILE__, __LINE__);
        return false;
	} 
}

void AI::l_try_replace_skill_id(int32_t& _skill_id)
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* state = lua_svr->L();
	lua_svr->get_lua_ref(Ctrl::get_lua_regid());
	lua_pushstring(state, "try_replace_skill_id");
	lua_pushinteger(state, get_id());
	lua_pushinteger(state, _skill_id);

	if (llua_call(state, 3, 1, 0 ) == 0) 
	{
        _skill_id = lua_tointeger(state, -1);
		lua_pop(state, 1);    
	} 
	else 
	{     
		llua_fail(state, __FILE__, __LINE__);
	} 
}

}

