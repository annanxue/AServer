#include "action.h"
#include <cmath>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include "common.h"
#include "define_func.h"
#include "lua_utils.h"
#include <sstream>

class Obj;

namespace SGame
{

ActionSkill::ActionSkill(BTNode* _parent, AI* _id)
	: BTLeafNode(_parent, _id)
{
	skill_id_ = 0;
    last_skill_id_ = 0;
    last_skill_frame_ = 0;
	no_cd_ = 0;
	in_ai_delay_ = false;
	ai_delay_time_ = 0.6f;
	max_time_ = 30.f;
	target_type_ = TARGET_FIRST;
	deviate_angle_ = 0.0f;
	use_external_skillid_ = false;
	force_angle_ = -1.0f;
	spawn_angle_ = 0;
	no_turn_ = 0;
	chunk_idx_.clear();
	bs_ = 0;
	skill_cd_ = -1;
}

NodeStatus ActionSkill::step()
{
	uint32_t cur_time_ = ai_->get_frame_end_time();
	if (in_ai_delay_)
	{
		if(time_tracker_.is_ready(cur_time_))
		{
			node_status_ = SUCCESS;
			return node_status_;
		}
	}
	if (max_time_tracker_.is_ready(cur_time_))
	{
		node_status_ = SUCCESS;
		return node_status_;
	}
	return node_status_;
}

void ActionSkill::init()
{
	std::string ai_delay = get_ai_info("AIDelay");
	ai_delay_time_ = atof(ai_delay.c_str());
    ai_delay_time_ = get_property_rand_float( "ai-delay", ai_delay_time_ );

	BTLeafNode::init();
	set_time(get_default_time());
	get_property_float("max-time", max_time_);

	get_property_int("bs", bs_);
	max_time_tracker_.init(Tracker::TYPE_PERIOD, max_time_);
	
	get_property_float("force-angle", force_angle_);
	get_property_int("spawn-angle", spawn_angle_);

	get_property_ai_type("target", (unsigned char&)target_type_);

	get_property_float("deviate-angle", deviate_angle_);

	get_property_int("no-cd", no_cd_);

	get_property_int("no-turn", no_turn_);

	get_property_float("skill-cd", skill_cd_);

	get_property_int_list("chunk-idx", chunk_idx_);

	get_property_float_list("turn-pos", turn_pos_);

	float temp;
	use_external_skillid_ = get_property_float("external-skill",temp);

	bool ret = get_property_int("skill-id", skill_id_);
	if(ai_->assert_fail(ret && skill_id_))
	{
		errorf("no skill id");
		return;
	}

    ai_->l_try_replace_skill_id(skill_id_);
	ai_->l_set_skill(skill_id_, skill_cd_);
}

void ActionSkill::set_angle()
{
	if(spawn_angle_)
	{
		ai_->set_angle(ai_->get_spawn_angle());
	}
	else if(no_turn_)
	{
	}
	else if(turn_pos_.size() > 1)
    {
		VECTOR3 turn_pos(turn_pos_[0], 0, turn_pos_[1]);
        float angle = get_degree( turn_pos, ai_->get_pos() );
        ai_->set_angle( angle );
    }
	else if(force_angle_ == -1.f)
	{
		ai_->speed_vec_to_tgt(this, deviate_angle_);
	}
    else
	{
		ai_->set_client_angle(force_angle_);
	}
}

void ActionSkill::use_skill(int32_t _skill_id)
{
	uint32_t other_tgt_id = get_target_info();
    last_skill_id_ = _skill_id;
    last_skill_frame_ = ai_->get_frame_counter();
	ai_->cast_skill(_skill_id, 1, other_tgt_id, 0, get_skill_pos(), bs_);
}

void ActionSkill::activate()
{
	if( use_external_skillid_ )
	{
		int skill_id = ai_->get_extra_skill_id();

		if(skill_id != 0)
		{
			skill_id_ = skill_id;
            LOG(2)("use external skill %d",skill_id);
			ai_->l_set_skill(skill_id_, skill_cd_);
		}		
	}

	update_cd();
    set_angle();
	ai_->set_speed_len(0);
	ai_->clear_smoother();

	in_ai_delay_ = false; 
    use_skill(skill_id_);

    uint32_t activate_time = ai_->get_frame_begin_time();
	rule_tracker_.activate(activate_time);
	max_time_tracker_.activate(activate_time);
}

void ActionSkill::on_message(MSG_TYPE _msg)
{
    if (_msg != AI_SKILL_END)
    {
        return;
    }

    if (ai_->can_do_continue_skill())
    {
        int32_t current_frame = ai_->get_frame_counter();
        if (last_skill_frame_ != current_frame) 
        {
            int32_t continue_skill_id = 0;
            bool has_continue_skill = ai_->l_get_continue_skill_id(last_skill_id_, continue_skill_id);
            if (has_continue_skill)
            {
                use_skill(continue_skill_id);
                return;
            }
        }
    }

	if (!in_ai_delay_)
	{
		time_tracker_.activate(ai_->get_frame_begin_time());
		in_ai_delay_ = true;
	}
}

float ActionSkill::check_endure_time(float _time)
{
	return max(_time, 0.f);
}

bool ActionSkill::can_use()
{
	if(no_cd_)
	{
		return true;
	}

	if(!BTLeafNode::can_use())
	{
		return false;
	}

	return ai_->l_skill_can_use(skill_id_);
}

SKILL_TGT_TYPE ActionSkill::get_skill_type() const
{
	if(TARGET_FRONT == target_type_)
	{
		return POS_SKILL;
	}
	else
	{
		return TGT_SKILL;
	}
}

VECTOR3 ActionSkill::get_skill_pos()
{
	if(TARGET_FRONT == target_type_)
	{
		return ai_->get_pos() + ai_->get_speed_vec();
	}
	else
	{
		return ai_->get_pos();
	}
}

uint32_t ActionSkill::get_target_info() const
{
    SKILL_TGT_TYPE  tgt_type = get_skill_type();
    if ( tgt_type == POS_SKILL ) {
        return 0;
    }
    else
	    return ai_->tgt_get_id(this, true);
}


ActionChase::ActionChase(BTNode* _parent, AI* _id, AI_TYPE _action_type/* = ACTION_CHASE*/)
	: BTLeafNode(_parent, _id)
{
	action_type_ = _action_type;
	clip_time_ = 500.f;
	override_ani_type_ = false;
	set_ani_type(SHARE_ANI_RUN);
	turn_type_ = SHARE_TURN_TO_PATH;
	is_stand_ = false;
	in_near_walk_ = false;
	dir_radian_ = 0;
	min_rate_ = 0.9;
	already_stand_time_ = 0;
	raycast_rate_ = 0.f;
	chase_master_ = false;
    path_counter_ = 0 ; 
}

void ActionChase::activate() 
{
	uint32_t now_t = ai_->get_frame_end_time();
	if(is_spec_rush() && rule_tracker_.is_ready(now_t))
	{
		if (check_endure_cond())
		{
			node_status_ = SUCCESS;
			return;
		}
	}

	if(init_action_)  // the_first_time
	{
		float step = 0;
		if(is_spec_rush())
		{
			step = clip_time_/THOUSAND;
		}
		float time = get_property_rand_float("time", get_default_time(), step);
		time = check_endure_time(time);
		set_time(time);
		BTLeafNode::activate();
	}

	chase_dist_ = get_speed() * clip_time_;
	init_path();
	select_path_type();
}

NodeStatus ActionChase::step()
{
	if (!is_stand_)
	{
		chase_with_path();
	}
	reactivate_on_success();

	uint32_t cur_time_ = ai_->get_frame_end_time();
	if(time_tracker_.is_ready(cur_time_))
	{
		node_status_ = SUCCESS;
		return node_status_;
	}

	if(!is_spec_rush() && rule_tracker_.is_ready(cur_time_))
	{
		if (check_endure_cond())
		{
			node_status_ = SUCCESS;
			return node_status_;
		}
	}
	return node_status_;
}

void ActionChase::chase_with_path()
{
    uint32_t now_t = ai_->get_frame_end_time();

	if(can_move())
	{
		now_t -= already_stand_time_;
		VECTOR3 now_pos;
		uint32_t start_t = path_timer_[path_counter_];
		uint32_t end_t = path_timer_[path_counter_ + 1];
		VECTOR3 end_pos = path_[path_counter_ +1];

		if (now_t >= end_t)
		{
			now_pos = end_pos;
			path_counter_ += 1;

			if (path_counter_ >= static_cast<int32_t>(path_.size() - 1))
			{
				node_status_ = SUCCESS;
				set_patrol_index(true);
			}
			else
			{
				set_patrol_index(false);
				VECTOR3 start_pos = path_[path_counter_];
				end_pos = path_[path_counter_ +1];
				ai_->set_speed_vec(end_pos - start_pos);
			}
		}
		else
		{
			float u = static_cast<float>(now_t - start_t)/(end_t - start_t);
			const VECTOR3& start_pos = path_[path_counter_];
			vc3_intrp(now_pos, start_pos, end_pos, u);
		}
		if(ai_->verbose_print_)
		{
			TRACE(2)("ai_debug now_pos %f %f %f\n", now_pos.x, now_pos.y, now_pos.z);
        }
        ai_->set_pos(now_pos);
    }
}

bool ActionChase::calcu_path(AI_TYPE _path_type)
{
    if (PATH_DETOUR == _path_type)
    {
        if (path_.size() < 2)
        {
            _path_type = PATH_DIRECT;
			init_path();
		}
		else
		{
			clip_detour_path();
		}
	}

	if (PATH_DIRECT == _path_type)
	{
		if(!clip_direct_path())
		{
			return true;
		}
	}

	path_counter_ = 0;
	path_timer_.clear();

	float speed = get_speed();
    ai_->set_speed_vec(path_[1] - path_[0]);
	double now = static_cast<double>(ai_->get_frame_begin_time());

	float t;
	VECTOR3 last = path_[0];

	for(uint32_t i = 0; i < path_.size(); ++i)
	{
		t = length2d(path_[i] - last)/speed;
		last = path_[i];
		now += t;
		path_timer_.push_back(static_cast<uint32_t>(now));
	}

	if(send_path())
	{
        broadcast_path();
	}
	return true;
}

void ActionChase::broadcast_path()
{
	ai_->set_speed_len(get_speed());
	
    Navmesh::Path path;
	get_msg_path(&path);
	ai_->broadcast_mst_navigation(0, ai_->get_id(), get_ani_type(), dir_radian_, path, turn_type_, ai_->get_rush_master_rate());
}

void ActionChase::check_path() const
{
}

void ActionChase::process_arg()
{
	if(ai_->verbose_print_)
	{
		TRACE(2)("ai_debug", "ActionChase::process_arg()");
	}
}

void ActionChase::clip_path( float _max_dist, bool _intrp_last_point )
{
	float length = 0, old_length, seg_length;

	for (uint32_t i = 1; i < path_.size(); ++i)
	{
		old_length = length;
		seg_length = length2d(path_[i] - path_[i-1]);
		length += seg_length;
		if (length > _max_dist)
		{
			Navmesh::Path new_path;
			for(uint32_t j = 0; j < i; ++j)
			{
				new_path.push_back(path_[j]);
			}

            if ( _intrp_last_point )
            {
                VECTOR3 end_point;
                vc3_intrp(end_point, path_[i-1], path_[i], (_max_dist - old_length)/seg_length);
                new_path.push_back(end_point);
            }
            else
            {
                new_path.push_back(path_[i]);
            }
			path_ = new_path;
			check_path();
			return;
		}
	}
}

void ActionChase::init_path()
{
	path_.clear();
	path_.push_back(ai_->get_pos());
	path_.push_back(get_seek_pos());
}

bool ActionChase::clip_direct_path()
{
	VECTOR3 speed(0, 0, 0);

	VECTOR3 tar_force = path_[0] - path_[1];
	float dist = length2d(tar_force);
	if (dist > 0.1)
	{
		float A = 80000;
		// float B = 10000;
		float n = 1;
		// float m = 2;
		float tar_rad = ai_->get_bounding_radius();

		float d = dist / tar_rad;
		float tar_force_len =  -A / pow(d, n);  // # + B/pow(d, m)

		vc3_norm(tar_force, tar_force, tar_force_len);
		speed = speed + tar_force;
	}

	int32_t num = 0;

	FOR_LINKMAP( ai_->get_world(),ai_->get_plane(), ai_->get_pos(), ai_->get_aoi_radius(), Obj::LinkDynamic )
	{
		if(ai_->check_obj(obj, OT_MONSTER, true) && !ai_->is_obj_virtual(obj))
		{
			VECTOR3 npc_pos(ai_->get_obj_pos(obj));
			VECTOR3 npc_force = path_[0] - npc_pos;
			npc_force.y = 0;
			float dist = length2d(npc_force);
			if(dist > 0.1 && dist < 4)
			{
				float A = 2000;
				float B = 10000;
				float n = 1;
				float m = 2;
				float npc_rad = ai_->get_obj_radius(obj);
				float d = dist/npc_rad;
				float npc_force_len = -A/pow(d, n) + B/pow(d, m);
				vc3_norm(npc_force, npc_force, npc_force_len);
				speed = speed + npc_force;
				if(++num >= 4)
				{
					break;
				}
			}
		}
	}
	END_LINKMAP

	speed = ai_->smooth(speed);

	normalize2d(speed, chase_dist_);

	VECTOR3 tar_pos;
	float rate;
	if(ai_->raycast( path_[0], path_[0] + speed, tar_pos, rate ) || rate > min_rate_)
	{
		path_[1] = tar_pos;
	}
	else
	{
		speed = path_[1] - path_[0];
		speed.y = 0;
		if (length2d(speed) > chase_dist_)
		{
			vc3_norm(speed, speed, chase_dist_);
			path_[1] = path_[0] + speed;
		}
	}
	check_path();
	return true;
}

void ActionChase::init()
{
	BTLeafNode::init();

	get_property_ai_type("turn-type", turn_type_);
	if(get_property_float("clip-time", clip_time_))
	{
		clip_time_ *= THOUSAND;
	}

	ANI_TYPE global_ani_type = tree_->get_global_ani_type();
	if(global_ani_type != SHARE_ANI_NONE)
	{
		set_ani_type(global_ani_type, true);
	}

	ANI_TYPE ani_type;
	if(get_property_ai_type("ani-type", ani_type))
	{
		set_ani_type(ani_type, true);
	}
    int check_valid = 0;
    get_property_int( "check-valid", check_valid );
    check_valid_pos_ = check_valid > 0;
}

bool ActionChase::is_direct_path()
{
	return ai_->raycast(path_[0], path_[1], raycast_hitpos_, raycast_rate_) && !chase_master_;
}

void ActionChase::select_path_type_combat_npc()
{
	const VECTOR3& start = path_[0];
	const VECTOR3& end = path_[1];
	bool res = ai_->find_path_force( start, end, &path_);
	if (res == true)
	{
		if (length2d(path_.back() - end) > SCENE_CELL)
		{
			VECTOR3& dest = path_.back();
			dest = end;
		}
		calcu_path(PATH_DETOUR);
	}
	else
	{
		node_status_ = FAILURE;
	}        
}

void ActionChase::select_path_type()
{
	if (ai_->has_master() && (action_type_ == ACTION_FOLLOW || action_type_ == ACTION_CHASE))
	{
		select_path_type_combat_npc();
		return;
	}

    VECTOR3 target_pos = get_target_pos2d();
	Navmesh::Path temp_path;
	if(path_need_target())
	{
        VECTOR3 owner_pos = ai_->get_pos(); 
        float dist1 = length2d( owner_pos - target_pos );
        float dist2 = 1000;  
		if( ai_->find_path( owner_pos, target_pos, &temp_path, 0x0 ) )
        {
            dist2 = length2d( temp_path.back() - target_pos );
        }

        if( dist1 > 1.0f && dist2 > 2.0f ) 
		{
			ai_->set_target(NULL);
			node_status_ = SUCCESS;
			return;
		}
	}

	if (is_direct_path())
	{
		if (!calcu_path(PATH_DIRECT))
		{
			node_status_ = FAILURE;
			return;
		}
	}
	else
	{
		bool result = ai_->find_path(path_[0], path_[1], &temp_path, 0x0);
		if(result)
		{
			path_ = temp_path;
		}
		else if (raycast_rate_ > 0.1)
		{
			path_[1] = raycast_hitpos_;
		}
		else
		{
			path_[1] = target_pos;
			ai_->find_path(path_[0], path_[1], &path_, 0x0);
		}
		calcu_path(PATH_DETOUR);
	}
}

void ActionChase::get_msg_path(Navmesh::Path* _path) const
{
	*_path = path_;
}

bool ActionChase::can_move()
{
	return true;
}


void ActionChase::set_ani_type(ANI_TYPE _ani_type, bool _override /*= false*/)
{
	if(_override)
	{
		ani_type_ = _ani_type;
		override_ani_type_ = true;
	}
	else if (!override_ani_type_)
	{
		ani_type_ = _ani_type;
	}
}

float ActionChase::get_speed() 
{
	float speed = ai_->get_speed(get_ani_type());

	if(ai_->assert_fail(speed > 0))
	{
		errorf("zero speed! check npc xlsx");
		return 0.1f;
	}
	return speed;
}

int ActionChase::sync_path_to() const
{
	if(RUNNING == node_status_)
	{
		Navmesh::Path path;
		get_cur_path(&path);
		if(path.size() > 1)
		{	
			return ai_->broadcast_mst_navigation(1, ai_->get_id(), get_ani_type(), dir_radian_, path, turn_type_, ai_->get_rush_master_rate());
		}
	}
	return 0;
}

void ActionChase::get_cur_path( Navmesh::Path* _path ) const
{
	_path->clear();
	_path->push_back(ai_->get_pos());
    if (path_counter_ + 1 < (int)path_.size())
    {
        _path->insert(_path->end(), path_.begin()+path_counter_+1, path_.end());
    }
}

bool ActionChase::path_need_target()
{
	return !chase_master_;
}

uint32_t ActionChase::tgt_get_id() const
{
	return ai_->tgt_get_id(this);
}

bool ActionChase::send_path() const
{
	return true;
}

VECTOR3 ActionChase::get_seek_pos() const
{
	if(chase_master_)
	{
		Ctrl* master = ai_->get_master();
		return ai_->get_master_pos(master);
	}
    else if( check_valid_pos_ ) 
    {
        VECTOR3 pos = ai_->tgt_get_pos(this);
        VECTOR3 result_pos;
        Ctrl* target = static_cast<Ctrl*>(ai_->owner_);
        bool result = target->get_valid_pos( &pos, CHECK_VALID_POS_RADIUS, &result_pos );
        if( result )
        {
            result_pos.y = 0;
            return result_pos;
        }
        return  pos;
    }
	else
	{
		return ai_->tgt_get_pos(this);
	}
}

VECTOR3 ActionChase::get_target_pos2d() const
{
	if(chase_master_)
	{
		Ctrl* master = ai_->get_master();
		return ai_->get_master_pos(master);
	}
    else if( check_valid_pos_ ) 
    {
        VECTOR3 pos = ai_->tgt_get_pos(this);
        VECTOR3 result_pos;
        Ctrl* target = static_cast<Ctrl*>(ai_->owner_);
        bool result = target->get_valid_pos( &pos, CHECK_VALID_POS_RADIUS, &result_pos );
        if( result )
        {
            result_pos.y = 0;
            return result_pos;
        }
        return  pos;
    }
	else
	{
		return ai_->tgt_get_pos(this);
	}
}

void ActionHold::do_stand()
{
	ai_->set_speed_len(0);
	ai_->clear_smoother();
	ai_->broadcast_mst_stand(0, ai_->get_id(), "guard", ai_->get_pos().x, ai_->get_pos().z, SHARE_ACT_STAND, SHARE_TURN_STILL, ai_->tgt_get_id(this), 0);
}


ActionExplore::ActionExplore( BTNode* _parent, AI* _id ) 
	: ActionChase(_parent, _id, ACTION_EXPLORE), ani_name_("guard"), act_type_(SHARE_ACT_LOOP)
{
	move_rate_ = 0.f;
	move_time_ = 0.f;
	stand_time_ = 0.f;
	min_angle_ = 0.f;
	max_angle_ = 0.f;
	radius_ = 0.f;
	total_move_time_ = 0.f;
	move_counter_ = 0;
	sign_ = 1;
	force_explore_ = 0;
	back_rate_ = 0.5f;
	loop_ = false;
	patrol_ani_type_ = PATROL_ANI_NONE;
	patrol_path_id_ = 0;
	patrol_path_index_ = 1;
	has_patrol_path_ = false;
    start_with_nearest_ = 0;
    broadcast_ = true;
}

NodeStatus ActionExplore::step()
{
	if(!is_stand_)
	{
		chase_with_path();
	}
	
    if(SUCCESS == node_status_)
	{
		if(has_patrol_path_ && !patrol_path_.empty() && !loop_)
		{
			patrol_path_.clear();
			ai_->reset_spawn_point();
		}
        return node_status_;
	}

	uint32_t cur_time_ = ai_->get_frame_end_time();
	if(rule_tracker_.is_ready(cur_time_))
	{
		if (check_endure_cond())
		{
			node_status_ = SUCCESS;
			return node_status_;
		}
	}

	if(time_tracker_.is_ready(cur_time_))
	{
		node_status_ = SUCCESS;
		return node_status_;
	}
	
	return node_status_;
}

void ActionExplore::init()
{
	ActionChase::init();
	get_property_string("ani", ani_name_);
	if(ani_name_ == "idle" || ani_name_ == "guard")
	{
		act_type_ = SHARE_ACT_STAND;
	}

	bool ret;
	ret = get_property_float("move-rate", move_rate_);
	if(ret)
	{
		std::vector<float> dirs;
		ret = get_property_float_list("dir", dirs);
		if(ai_->assert_fail(ret))
		{
			errorf("no explore dir");
			return;
		}
		min_angle_ = dirs[0];
		max_angle_ = dirs[1];

		ret = get_property_float("move-time", move_time_);
		if(ai_->assert_fail(ret))
		{
			errorf("no explore move time");
			return;
		}
		move_time_ *= THOUSAND;

		ret = get_property_float("stand-time", stand_time_);
		if(ai_->assert_fail(ret))
		{
			errorf("no explore stand time");
			return;
		}
		stand_time_ *= THOUSAND;

		ret = get_property_float("rad", radius_);
		if(ai_->assert_fail(ret))
		{
			errorf("no explore rad");
			return;
		}
	}
	get_property_float("back-rate", back_rate_);
	get_property_int("force-explore", force_explore_);
	get_property_int("path-id", patrol_path_id_);
	get_property_int("start-with-nearest", start_with_nearest_);
}

void ActionExplore::set_move_list()
{
	total_move_time_ = 0.f;

	if (move_rate_ > 0.f)
	{
		move_list_flag_.clear();
		move_list_time_.clear();
		move_timer_flag_.clear();
		move_timer_time_.clear();

		float full_time = get_time();
		float the_time;

		while(full_time > 0)
		{
			the_time = full_time;
			float rate = rand_float(0, 1);
			if(rate < move_rate_)
			{
				full_time -= move_time_;
				if(full_time > 0)
				{
					the_time = move_time_;
				}

				if(move_list_flag_.size() && move_list_flag_.back())
				{
					move_list_time_.back() += the_time;
				}
				else
				{
					move_list_flag_.push_back(true);
					move_list_time_.push_back(the_time);
				}
			}
			else
			{
				full_time -= stand_time_;
				if(full_time > 0)
				{
					the_time = stand_time_;
				}
				if(move_list_flag_.size() && !move_list_flag_.back())
				{
					move_list_time_.back() += the_time;
				}
				else
				{
					move_list_flag_.push_back(false);
					move_list_time_.push_back(the_time);
				}
			}
		}

		the_time = 0;

		for(uint32_t i = 0; i < move_list_flag_.size(); ++i)
		{
			bool is_move = move_list_flag_[i];
			float time = move_list_time_[i];
			if(is_move)
			{
				total_move_time_ += time;
			}
			move_timer_flag_.push_back(is_move);
			move_timer_time_.push_back(the_time);
			the_time += time;
		}
		move_timer_flag_.push_back(false);
		move_timer_time_.push_back(the_time + THOUSAND);

		if(ai_->assert_fail(total_move_time_ <= get_time()))
		{
			errorf("total move time error");
			is_stand_ = true;
            return;
		}
	}

	if(total_move_time_ == 0.f)
	{
		is_stand_ = true;
	}
}

void ActionExplore::get_msg_path(Navmesh::Path* _path) const
{
	_path->clear();
	_path->push_back(ai_->get_pos());
    
    if(move_counter_  >= static_cast<int32_t>(move_timer_flag_.size()- 1))
    {
        return;
    }
    uint32_t move_end_time = move_timer_time_[move_counter_ + 1];
    int move_time = move_end_time - ai_->get_frame_end_time();
    if (move_time < 0) move_time = 0;
    float can_move_dist =  ai_->get_speed(get_ani_type()) * move_time;
   
	float length = 0;
    // [path_counter, path_counter+1] 是之前走的路
    // 从+2开始算，最终路径把之前的走路的终点算上
	for (uint32_t i = path_counter_ + 2; i < path_.size(); ++i)
	{
		float len = length2d(path_[i] - path_[i-1]);
		length += len;
		if (length > can_move_dist)
		{
			for(uint32_t j = path_counter_ + 1; j <= i; ++j)
			{
				_path->push_back(path_[j]);
			}
			return;
		}
	}
	for(uint32_t j = path_counter_ + 1; j < path_.size(); ++j)
	{
		_path->push_back(path_[j]);
	}
}

void ActionExplore::chase_with_path()
{
	uint32_t now_t = ai_->get_frame_end_time();

	uint32_t move_end_t = move_timer_time_[move_counter_ + 1];
    if (now_t >= move_end_t)
	{
		move_counter_ += 1;
		if(move_counter_ >= static_cast<int32_t>(move_timer_flag_.size()- 1))
		{
			node_status_ = SUCCESS;
		}
		else
		{
			bool is_move = move_timer_flag_[move_counter_];
			if (is_move)
			{
                broadcast_path();
			}
			else
			{
                bool is_last_time_stand = (move_counter_ > 0) ? !move_timer_flag_[move_counter_ - 1] : false; 
                if ( !is_last_time_stand )
                {
                    do_stand();
                }
				already_stand_time_ += move_list_time_[move_counter_];
			}
		}
	}

	ActionChase::chase_with_path();
}

size_t ActionExplore::find_nearest_to_start( Navmesh::Path& _path )
{
    VECTOR3 pos = ai_->get_pos();
    int p_size = _path.size();
    if( p_size < 1 )
    {
        return 0;
    }

    size_t nearest = 0;
    float min_length = -1;
    for( int i = 0; i < p_size; ++ i)
    {
        VECTOR3 diff = pos - _path[i];
        float length = length2d(diff);
        if( min_length < 0 || length < min_length )
        {
            nearest = i;
            min_length = length;
        }
    }
    if(min_length < 0.001)
    {
        _path.erase(_path.begin()+nearest);
        nearest = nearest % _path.size();
    }

    return nearest;
}

void ActionExplore::broadcast_path() 
{
    if (broadcast_)
    {
        ActionChase::broadcast_path();
    }
    else
    {
	    ai_->set_speed_len(get_speed());
    }
}

bool ActionExplore::calcu_path(AI_TYPE _path_type)
{
	ActionChase::calcu_path(_path_type);
	move_counter_ = -1; 
    already_stand_time_ = 0;

    if(has_patrol_path_ && patrol_info_.size() > 0)
	{
		set_patrol_move_list();
	}

	uint32_t start_t = ai_->get_frame_begin_time();
	for(uint32_t i = 0; i < move_timer_time_.size(); ++i)
	{
		move_timer_time_[i] += start_t;
	}

	return true;
}

void ActionExplore::set_patrol_index(bool _is_end)
{
	if(!has_patrol_path_) return;

	if(_is_end)
	{
		patrol_path_index_ = 1;
	}
	else
	{
		VECTOR3 pos = path_[path_counter_];

        size_t size = patrol_path_.size();
        for (size_t i = 0; i < size; ++i)
        {
            size_t index = ( patrol_path_index_ + i ) % size;
			VECTOR3 point = patrol_path_[index];
			VECTOR3 dif = point - pos;
			dif.y = 0;
			if(vc3_iszero(dif))
			{
				patrol_path_index_ = index + 1;
				break;
			}
        }
	}

    ai_->sync_mst_patrol_index( ai_->get_id(), patrol_path_index_ );
}

void ActionExplore::do_stand()
{
	ai_->set_speed_len(0);
	ai_->clear_smoother();

    if (!broadcast_) 
    {
        return;
    }
    
    string ani_name = ani_name_;
	if(has_patrol_path_ && patrol_info_.size() > 0)
	{
		for (size_t i = 0; i < patrol_info_.size(); ++i)
		{
			VECTOR3 dif = patrol_info_[i].pos_ - ai_->get_pos();
			if(vc3_xzlen(dif) < 1)
			{
				ani_name = patrol_info_[i].anim_name_;
				int32_t chat_id = patrol_info_[i].chat_id_;
				if(chat_id > 0)
				{
					ai_->broadcast_mst_sfx(chat_id, 1, ai_->get_id());
                }
				break;
			}
		}
	}
	ai_->broadcast_mst_stand(0, ai_->get_id(), ani_name, ai_->get_pos().x, ai_->get_pos().z, act_type_, SHARE_TURN_STILL, ai_->tgt_get_id(this), 0);
}

bool ActionExplore::clip_direct_path()
{
	if(has_patrol_path_)
	{
		get_patrol_path();
		return true;
	}

	VECTOR3 pos;
	VECTOR3 src = ai_->get_pos();
	bool is_in_battle = ai_->is_in_battle();
	Ctrl* master = ai_->get_master();
	if(master != NULL)
	{
		pos = ai_->get_master_pos(master);
	}
	else if(is_in_battle || force_explore_)
	{
		pos = src;
	}
	else
	{
		pos = ai_->get_spawn_point();
	}

	std::vector<VECTOR3> points;
	ai_->find_poly(pos, 0, radius_, points);
	if(!points.size())
	{
		is_stand_ = true;
		return false;
	}

	points.push_back(pos);
	float dist = get_speed() * (total_move_time_ + THOUSAND);
	Navmesh::Path new_path;
	new_path.push_back(src);

	while(dist > 0)
	{
		VECTOR3 old_dir;
		get_old_dir(is_in_battle, old_dir, new_path);

		Navmesh::Path temp_path;
		if(!find_by_ray(is_in_battle, old_dir, new_path, &temp_path, points.size() < 3))
		{
			if (!find_by_poly(is_in_battle, old_dir, &points, new_path, temp_path))
			{
				break;
			}
		}

		if(!add_new_path(&dist, &new_path, temp_path))
		{
			break;
		}
	}

	if (new_path.size() < 2)
	{
		is_stand_ = true;
		return false;
	}
	else
	{
		path_ = new_path;
		return true;
	}
}

void ActionExplore::get_patrol_path()
{
    ai_->find_path_force(ai_->get_pos(), patrol_path_[patrol_path_index_], &path_);
    if(patrol_path_index_ < static_cast<int32_t>(patrol_path_.size()-1))
	{
		path_.insert(path_.end(), patrol_path_.begin()+patrol_path_index_+1, patrol_path_.end());
	}
}

bool ActionExplore::add_new_path(float* _dist, Navmesh::Path* _new_path, const Navmesh::Path& _temp_path)
{
	for(Navmesh::Path::const_iterator pt = _temp_path.begin()+1; pt != _temp_path.end(); ++pt)
	{
		float len_seg = length2d(_new_path->back() - *pt);
		(*_dist) -= len_seg;
		_new_path->push_back(*pt);
		if(_new_path->size() > 20)
		{
			*_dist = 0;
			return false;
		}
		if(*_dist <= 0)
		{
			return false;
		}
	}
	return true;
}

bool ActionExplore::find_by_poly(bool _is_in_battle, const VECTOR3& _old_dir, \
								 std::vector<VECTOR3>* _points, const Navmesh::Path& _new_path, Navmesh::Path& _temp_path)
{ 
	float dif_angle = 180.f;
	VECTOR3 nearest(0, 0, 0);
	random_shuffle(_points->begin(), _points->end());
	for(std::vector<VECTOR3>::iterator pt = _points->begin(); pt != _points->end(); ++pt)
	{
		if (length2d(_new_path.back() - *pt) == 0.f)
		{
			continue;
		}

		if(_new_path.size() < 2 && !force_explore_)
		{
			nearest = *pt;
			break;
		}
		VECTOR3 new_dir;
		if(_is_in_battle)
		{
			new_dir = *pt - _new_path[0];
		}
		else
		{
			new_dir = *pt - _new_path.back();
		}

		float angle = degrees(dot_angle(_old_dir, new_dir));
		if(angle >= min_angle_ && angle <= max_angle_)
		{
			nearest = *pt;
			break;
		}
		else
		{
			if(angle < min_angle_)
			{
				if(min_angle_ - angle < dif_angle)
				{
					dif_angle = min_angle_ - angle;
					nearest = *pt;
				}
			}
			else
			{
				if(angle - max_angle_ < dif_angle)
				{
					dif_angle = angle - max_angle_;
					nearest = *pt;
				}
			}
		}
	}

	if(vc3_iszero(nearest))
	{
		return false;
	}
	bool ret = ai_->find_path(_new_path.back(), nearest, &_temp_path);
	if(!ret)
	{
		return false;
	}
	return true;
}

void ActionExplore::activate()
{
	if(patrol_path_id_ > 0 && !has_patrol_path_)
	{
		l_get_patrol_path();
	}

	if(PATROL_ANI_RUSH == patrol_ani_type_)
	{
		set_ani_type(SHARE_ANI_RUSH);
	}
	else if(PATROL_ANI_RUN == patrol_ani_type_)
	{
		set_ani_type(SHARE_ANI_RUN);
	}
	else if(PATROL_ANI_WALK == patrol_ani_type_)
	{
		set_ani_type(SHARE_ANI_WALK);
	}
	else if(ai_->is_in_battle())
	{
		set_ani_type(SHARE_ANI_RUN);
	}
	else
	{
		set_ani_type(SHARE_ANI_WALK);
	}

	is_stand_ = false;
	set_move_list();
	ActionChase::activate();
	if(is_stand_)
	{
        do_stand();
	}
	else
	{
		move_msg_ = true;
	}
	if(rand_float(0, 1) < back_rate_)
	{
		sign_ = -sign_;
	}
}

void ActionExplore::l_get_patrol_path()
{
	LuaServer* lua_svr = Ctrl::get_lua_server();
	lua_State* state = lua_svr->L();
	lua_svr->get_lua_ref( Ctrl::get_lua_regid() );

	lua_pushstring( state, "get_mst_patrol_path" );
	lua_pushinteger( state, patrol_path_id_ );
	if( llua_call( state, 2, 4, 0 ) )    
	{
		llua_fail( state, __FILE__, __LINE__ );
		return;
	}

	int ret = lua_tointeger( state, -1 );
	int loop = lua_tointeger( state, -2 );
	int is_run = lua_tointeger( state, -3 );
	lua_pop( state, 3 );

	if ( ret == 0 )  //fail to get the path info of id
	{
		lua_pop(state, 1);
		return;
	}

	//get path info
	Navmesh::Path patrol_path;
	if( lua_type( state, -1 ) != LUA_TTABLE )
	{
		ERR(2)("[AI]Lua::GetTable, get patrol path error");
		return;
	}
	WHILE_TABLE( state )
		load_path( state, patrol_path );
	END_WHILE( state )
		lua_pop( state, 1 );

	//set path
	set_path( patrol_path, loop, false, is_run, NULL);
}

void ActionExplore::load_path( lua_State* _state, Navmesh::Path& _path )
{
	float x = 0, z = 0; 
	Lua::get_table_number_by_index( _state, -1, 1, x );
	Lua::get_table_number_by_index( _state, -1, 2, z );
	_path.push_back( VECTOR3(x, 0, z) );
}

bool ActionExplore::set_path( Navmesh::Path &_path, bool _loop, bool _not_find_path, int32_t _ani_type, const vector<PATROL_INFO> *_patrol_info, bool _broadcast )
{
	if (has_patrol_path_)
	{
		return true;
	}

	patrol_path_.clear();
	patrol_path_.push_back(ai_->get_pos());
	patrol_ani_type_ = _ani_type;

    size_t start = 0;
    if( start_with_nearest_ )
    {
        start = find_nearest_to_start(_path);
    }

	if(_loop)
	{
        for(size_t i = 0; i < start ; ++i)
        {
            _path.push_back(_path[i]);
        }
		_path.push_back(patrol_path_[0]);//1
	}
	loop_ = _loop;

	Navmesh::Path temp_path;

    for(size_t i = start; i < _path.size(); ++i)
	{
		if(_not_find_path)
		{
			patrol_path_.push_back(_path[i]);
		}
		else
		{
			ai_->find_path_force(patrol_path_.back(), _path[i], &temp_path);
			for (size_t j = 1; j < temp_path.size(); ++j)
			{
				patrol_path_.push_back(temp_path[j]);
			}
		}
	}

    ai_->sync_mst_patrol_path( ai_->get_id(), patrol_path_ ); 
    
    has_patrol_path_ = true;
    
    if (_patrol_info)
	{
		patrol_info_ = *_patrol_info;
	}
    broadcast_ = _broadcast; 
    return true;
}

bool ActionExplore::find_by_ray(bool _is_in_battle, const VECTOR3& _old_dir, \
								const Navmesh::Path& _new_path, Navmesh::Path* _temp_path, bool _force_ray)
{
	VECTOR3 nearest(0, 0, 0);
	if(_force_ray || (!vc3_iszero(_old_dir) && _is_in_battle) || force_explore_)
	{
		float angle = rand_float(min_angle_, max_angle_);
		if(_force_ray)
		{
			angle = rand_float(90, 150);
		}
		angle = fabs(angle);
		angle *= sign_;

		VECTOR3 my_dir(_old_dir);
		normalize2d(my_dir, radius_);
		rotate_vec_y_axis(&my_dir, angle);

		if(_is_in_battle)
		{
			nearest = my_dir + _new_path[0];
		}
		else
		{
			nearest = my_dir + _new_path.back();
			if(length2d(nearest - _new_path[0]) > radius_)
			{
				my_dir = _old_dir;
				normalize2d(my_dir, radius_);
				rotate_vec_y_axis(&my_dir, - angle);
				nearest = my_dir + _new_path.back();
			}
		}

		bool ret = ai_->find_path(_new_path.back(), nearest, _temp_path);
		if(!ret)
		{
			if(force_explore_)
			{
				VECTOR3 temp;
				float rate;
				ret = ai_->raycast(_new_path.back(), nearest, temp, rate);
				_temp_path->push_back(_new_path.back());
				_temp_path->push_back(nearest);
				if(!ret)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}
	return true;
}

void ActionExplore::get_old_dir(bool _is_in_battle, VECTOR3& _old_dir, const Navmesh::Path& _new_path)
{
	if(_is_in_battle)
	{
		_old_dir = ai_->tgt_get_pos(this) - _new_path[0];
		sign_ = -sign_;
	}
	else
	{
		if(rand_float(0, 1) < back_rate_)
		{
			sign_ = -sign_;
		}
		if (_new_path.size() >= 2)
		{
			_old_dir = _new_path.back() - _new_path[_new_path.size() - 2];
		}
		else
		{
			_old_dir = ai_->get_speed_vec();
			if (vc3_iszero(_old_dir))
			{
				_old_dir = VECTOR3(1, 0, 0);
			}
		}
	}
}

int ActionExplore::sync_path_to() const
{
	if(RUNNING == node_status_)
	{
        if (move_counter_ < 0 || move_counter_ >= (int)move_timer_flag_.size())
        {
            return 0;
        }

		if (move_timer_flag_[move_counter_])
		{
			return ActionChase::sync_path_to();
		}
	}
	return 0;
}

bool ActionExplore::can_use()
{
	if(!ActionChase::can_use())
	{
		return false;
	}

	if(has_patrol_path_ && patrol_path_.empty())
	{
		return false;
	}
    
	return true;
}

void ActionExplore::init_path()
{
}

bool ActionExplore::can_move()
{
    if( move_counter_ < 0 || move_counter_ >= (int)move_timer_flag_.size() )
    {
        return false;
    }
    return move_timer_flag_[move_counter_];
}

void ActionExplore::set_patrol_move_list()//123
{
	move_list_flag_.clear();
	move_list_time_.clear();
	move_timer_flag_.clear();
	move_timer_time_.clear();

	VECTOR3 last = path_[0];
	float speed = get_speed();
	for(uint32_t i = 0; i < path_.size(); ++i)
	{
		float move_time = length2d(path_[i] - last)/speed;
        last = path_[i];

		float stand_time = 0;
		for (size_t j = 0; j < patrol_info_.size(); ++j)
		{
			VECTOR3 dif = patrol_info_[j].pos_ - path_[i];
			if(vc3_xzlen(dif) < 1)
			{
				stand_time = patrol_info_[j].anim_time_ * THOUSAND;
				break;
			}
		}

		if (move_time > 0)
		{
			if(move_list_flag_.size() && move_list_flag_.back())
			{
				move_list_time_.back() += move_time;
			}
			else
			{
				move_list_flag_.push_back(true);
				move_list_time_.push_back(move_time);
			}
		}

		if(stand_time > 0)
		{
			if(move_list_flag_.size() && !move_list_flag_.back())
			{
				move_list_time_.back() += stand_time;
			}
			else
			{
				move_list_flag_.push_back(false);
				move_list_time_.push_back(stand_time);
			}
		}
	}

	float the_time = 0;

	for(uint32_t i = 0; i < move_list_flag_.size(); ++i)
	{
		bool is_move = move_list_flag_[i];
		float time = move_list_time_[i];
		move_timer_flag_.push_back(is_move);
		move_timer_time_.push_back(the_time);
		the_time += time;
	}
	move_timer_flag_.push_back(false);
	move_timer_time_.push_back(the_time + THOUSAND);

	set_time(the_time/THOUSAND);
	time_tracker_.activate(ai_->get_frame_begin_time());
}

ActionEvasion::ActionEvasion( BTNode* _parent, AI* _id ) 
	: ActionChase(_parent, _id, ACTION_EVASION)
{

}

void ActionEvasion::calcu_escape_path()
{
	std::vector<VECTOR3> centers;
	float dist = get_speed() * time_tracker_.get_period();
	ai_->find_poly(path_[0], dist, dist*2, centers);

	VECTOR3 speed(0, 0, 0);
	float angle = PI;
	VECTOR3 path_vec = path_[0] - path_[1];
	Navmesh::Path temp_path;

	if(centers.size())
	{
		for(uint32_t i = 0; i < centers.size(); ++i)
		{
			VECTOR3 temp_speed = centers[i] - path_[0];
			if(length2d(temp_speed) >= dist)
			{
				float the_angle = dot_angle(temp_speed, path_vec);
				if(the_angle < angle)
				{
					if(ai_->find_path(path_[0], centers[i], &temp_path) &&
						length2d(temp_path[1] - centers[i]) <1.0f)
					{
						speed = temp_speed;
						angle = the_angle;
					}
				}
			}
		}
	}

	if(length2d(speed))
	{
		path_[1] = path_[0] + speed;
	}
	else
	{
		normalize2d(path_vec, dist);
		float rate;
		ai_->raycast(path_[0], path_[0] + path_vec, path_[1], rate);
	}
}


ActionSwing::ActionSwing( BTNode* _parent, AI* _id ) 
	: ActionChase(_parent, _id, ACTION_SWING),\
	ani_name_("idle"), angle_min_(0), angle_max_(0), sign_(1)
{
}

VECTOR3 ActionSwing::get_seek_pos() const
{
	sign_ = -sign_;
	VECTOR3 dir = ai_->tgt_get_pos(this) - ai_->get_pos();
	if (length2d(dir) == 0)
	{
		std::vector<VECTOR3> points;
		ai_->find_poly(ai_->tgt_get_pos(this), 0, chase_dist_, points, 0.9f);
		if(points.size() > 0)
		{
			return points[0];
		}
	}

	normalize2d(dir, chase_dist_);
	float angle = rand_float(angle_min_, angle_max_);
	angle *= sign_;
	rotate_vec_y_axis(&dir, angle);
	VECTOR3 ret = ai_->get_pos() + dir;
	VECTOR3 temp;
	float rate;
	ai_->raycast(ai_->get_pos(), ret, temp, rate);
	if(rate < 0.3)
	{
		std::vector<VECTOR3> points;
		ai_->find_poly(ai_->tgt_get_pos(this), 0, chase_dist_, points, 0.9f);
		if(points.size() > 0)
		{
			return points[0];
		}
	}
	sign_ = -sign_;
	return temp;
}

void ActionSwing::init()
{
	ActionChase::init();

	bool ret = get_property_float("angle-max", angle_max_);
	if(ai_->assert_fail(ret && angle_max_))
	{
		errorf("no swing angle max");
		return;
	}
	ret = get_property_float("angle-min", angle_min_);
	if(ai_->assert_fail(ret && angle_min_))
	{
		errorf("no swing angle min");
		return;
	}

	clip_time_ = 0.5f;
	get_property_float("clip-time", clip_time_);
	clip_time_ *= THOUSAND;
}

void ActionSwing::activate()
{
	if(init_action_)
	{
		if (is_spec_rush())
		{
			sign_ = -1;
		}
		else
		{
			sign_ = rand_int(0, 1) * 2 - 1;
		}
	}
	ActionChase::activate();
}


ActionFollowMaster::ActionFollowMaster( BTNode* _parent, AI* _id )
	: ActionChase(_parent, _id, ACTION_FOLLOW)
{

}


ActionAct::ActionAct( BTNode* _parent, AI* _id ) : 
    BTLeafNode(_parent, _id), 
    ani_name_("guard"),
	act_type_(SHARE_ACT_LOOP), 
    turn_type_(SHARE_TURN_STILL)
{
	turn_rate_ = 0.7f;
	dir_radian_ = 0.f;
	is_spawn_ = false;
}

void ActionAct::activate()
{
	BTLeafNode::activate();
	ai_->set_speed_len(0);
	ai_->clear_smoother();

	TURN_TYPE turn_type = turn_type_;
	if(SHARE_TURN_TO_CHAR_ALWAYS == turn_type && rand_float(0, 1) < turn_rate_)
	{
		turn_type = SHARE_TURN_TO_CHAR_ONCE;
	}

	if(turn_type == SHARE_TURN_TO_CHAR_ONCE || turn_type == SHARE_TURN_TO_CHAR_ALWAYS)
	{
		ai_->speed_vec_to_tgt(this);
	}
	ai_->broadcast_mst_stand(0, ai_->get_id(), ani_name_, ai_->get_pos().x, ai_->get_pos().z, act_type_, turn_type_, ai_->tgt_get_id(this), get_dir_radian());
}

NodeStatus ActionAct::step()
{
	uint32_t cur_time_ = ai_->get_frame_end_time();

	if(time_tracker_.is_ready(cur_time_))
	{
		node_status_ = SUCCESS;
		return node_status_;
	}

	if(rule_tracker_.is_ready(cur_time_))
	{
		if (check_endure_cond())
		{
			node_status_ = SUCCESS;
			return node_status_;
		}
	}

	return node_status_;
}

void ActionAct::init()
{
	BTLeafNode::init();

	get_property_string("ani", ani_name_, true);
	if(ani_name_ == "idle" || ani_name_ == "guard")
	{
		act_type_ = SHARE_ACT_STAND;
	}

    bool is_succ = init_anim_time( ani_name_ );
    if (ai_->assert_fail(is_succ))
    {
        std::string err_msg = "fail get anim time, ani_name: " + ani_name_; 
        errorf(err_msg.c_str());
        return;
    }

	get_property_ai_type("act-type", act_type_);
	get_property_ai_type("turn-type", turn_type_);
	get_property_float("turn-rate", turn_rate_);

	get_property_float_list("turn-pos", turn_pos_);
	if (turn_pos_.size() > 0)
	{
		turn_type_ = SHARE_TURN_TO_DIR;
	}
}

//loop anim, get time from ai node param
//non-loop anim, get time from MODEL_INFO
bool ActionAct::init_anim_time(const std::string& _anim_name) 
{
    if (_anim_name.empty() || _anim_name == "guard")
    {
        return true;
    }

    float anim_time = 0.f;
    bool is_succ = ai_->l_get_anim_time(_anim_name, anim_time);
    if (is_succ)
    {
        if (anim_time >= 0)
        {
            set_time(anim_time);
        }
    }
    return is_succ;
}

int ActionAct::sync_path_to() const
{
	return ai_->broadcast_mst_stand(1, ai_->get_id(), ani_name_, ai_->get_pos().x, ai_->get_pos().z, act_type_, turn_type_, ai_->tgt_get_id(this), get_dir_radian());
}

float ActionAct::get_dir_radian() const
{
	if(turn_pos_.size() > 0)
	{
		VECTOR3 turn_pos(turn_pos_[0], 0, turn_pos_[1]);
		dir_radian_ = dir2radian(turn_pos - ai_->get_pos());
	}
	return dir_radian_;
}

void ActionAct::set_anim( const std::string& _anim, float _dir )
{
	act_type_ = SHARE_ACT_LOOP;
	turn_type_ = SHARE_TURN_TO_DIR;
	ani_name_ = _anim;
	dir_radian_ = _dir;
	is_spawn_ = true;
	init();
}


ActionShuffle::ActionShuffle( BTNode* _parent, AI* _id ) 
	: ActionAct(_parent, _id), min_deviate_angle_(15.f)
{
	turn_type_ = SHARE_TURN_TO_CHAR_ONCE;
	ani_name_ = "shuffle_right";
}

void ActionShuffle::init()
{
	ActionAct::init();
	get_property_float("min-angle", min_deviate_angle_);
}

void ActionShuffle::activate()
{
	float angle = ai_->tgt_get_angle(this);
    float time = time_tracker_.get_period();
    if (time <= 0)
    {
        time = angle / 600 + 0.5;
        set_time(time);
    }

	if (angle < min_deviate_angle_)
	{
		ani_name_ = "guard";
		act_type_ = SHARE_ACT_STAND;
	}
	else if (ai_->tgt_is_right_side())
	{
		ani_name_ = "shuffle_right";
		act_type_ = SHARE_ACT_LOOP;
	}
	else
	{
		ani_name_ = "shuffle_left";
		act_type_ = SHARE_ACT_LOOP;
	}

	ActionAct::activate();
	ai_->speed_vec_to_tgt(this);
}


void ActionHold::set_ani_type( ANI_TYPE _ani_type, bool _override /*= false*/ )
{
	if(_override)
	{
		base_ani_type_ = _ani_type;
	}
	else
	{
		ani_type_ = base_ani_type_ + _ani_type;
	}
}

void ActionHold::activate()
{
	if (init_action_)
	{
		std::vector<float> rates;
		rates.push_back(front_ ? 100 : 0);
		rates.push_back(left_ ? 100 : 0);
		rates.push_back(right_ ? 100 : 0);
		rates.push_back(back_ ? 100 : 0);

		float total_sum = 0;
		for (uint32_t i = 0; i < rates.size(); ++i)
		{
			total_sum += rates[i];
		}

		float chosen = rand_float(0, total_sum);
		float sum = 0;
		int idx = -1;
		for(uint32_t i = 0; i < rates.size(); ++i)
		{
			sum += rates[i];
			if(sum >= chosen)
			{
				idx = i;
				break;
			}
		}
		set_ani_type(idx);
	}
	ActionChase::activate();
}

ActionHold::ActionHold(BTNode* _parent, AI* _id) :
	ActionChase(_parent, _id, ACTION_HOLD)
{
	clip_time_ = 1.f;
	turn_type_ = SHARE_TURN_TO_DIR_PATH;
	min_rate_ = 0.1;
	base_ani_type_ = SHARE_ANI_WALK;
	set_ani_type(ANI_FRONT_INDEX);

	front_ = 1;
	left_ = 1;
	right_ = 1;
	back_ = 1;
}

VECTOR3 ActionHold::get_seek_pos() const
{
	VECTOR3 dir = ai_->tgt_get_pos(this) - ai_->get_pos();
	normalize2d(dir, chase_dist_);
	static float angle_array[] =
	{0, 270, 90, 180};
	float angle = angle_array[static_cast<int32_t>(get_ani_type())-base_ani_type_];
	if(angle > 0)
	{
		rotate_vec_y_axis(&dir, angle);
	}
	return ai_->get_pos() + dir;
}

void ActionHold::init()
{
	ActionChase::init();
	if(SHARE_ANI_RUN == base_ani_type_)
	{
		clip_time_ = 0.5;
	}
	get_property_float("clip-time", clip_time_);
	clip_time_ *= THOUSAND;

	get_property_int("front", front_);
	get_property_int("left", left_);
	get_property_int("right", right_);
	get_property_int("back", back_);

	//LOG(2)("front:%d, left:%d, right:%d, back:%d", front_, left_, right_, back_ );
}

void ActionHold::broadcast_path()
{
	dir_radian_ = dir2radian(path_[1] - path_[0]);

	switch(get_ani_type())
	{
	case SHARE_ANI_WALK_LEFT:
	case SHARE_ANI_RUN_LEFT:
		dir_radian_ += PI/2;
		break;

	case SHARE_ANI_WALK_RIGHT:
	case SHARE_ANI_RUN_RIGHT:
		dir_radian_ -= PI/2;
		break;

	case SHARE_ANI_WALK_BACK:
	case SHARE_ANI_RUN_BACK:
		dir_radian_ = PI + dir_radian_;
		break;
	}
	//LOG(2)("aiId: %d, anim: %d, Rad:%f, type:%d", ai_->get_id(), get_ani_type(), dir_radian_, turn_type_);	
	ActionChase::broadcast_path();
}

void ActionHold::select_path_type()
{
	Navmesh::Path temp_path;
	if(path_need_target())
	{
        VECTOR3 target_pos = get_target_pos2d();
		bool result = ai_->find_path(ai_->get_pos(), target_pos, &temp_path);
		if((length2d(ai_->get_pos() - target_pos ) < 1.f) ||
			(result && length2d(temp_path.back() - target_pos ) < 1.f))
		{
		}
		else
		{
			ai_->set_target(NULL);
			node_status_ = SUCCESS;
			return;
		}
	}
	
	is_direct_path();

	is_stand_ = false;
	bool result = ai_->find_path(path_[0], path_[1], &temp_path);
	if(result && !vc2_iszero(temp_path[0] - temp_path[1]))
	{
		path_[1] = temp_path[1];
		calcu_path(PATH_DETOUR);
	}
	else if (raycast_rate_ > 0.1)
	{
		path_[1] = raycast_hitpos_;
		calcu_path(PATH_DETOUR);
	}
	else
	{
		is_stand_ = true;
		do_stand();
	}
}

void ActionFollowMaster::init()
{
	ActionChase::init();
    clip_time_ = 500.f;
	chase_master_ = true;
}

void ActionFollowMaster::activate()
{
    Ctrl* master = ai_->get_master();
    if (master != NULL)
    {
        if (init_action_)
        {
            if (!ai_->is_fake_battle())
            {
                ai_->gen_off_angle();
            }
        }
        else
        {
            if (!ai_->is_fake_battle() && ai_->get_off_angle() == DEFAULT_OFF_ANGLE)
            {
                ai_->gen_off_angle();
            }
        }
        
        float dis_of_master = ai_->get_dis_of_master(master);
        if(dis_of_master > ai_->get_out_range())
        {
            set_ani_type(SHARE_ANI_RUSH_MASTER);
        }
        else
        {
            set_ani_type(SHARE_ANI_RUN_MASTER);
        }
	    ActionChase::activate();
    }
    else
    {
        node_status_ = FAILURE;
    }
}


NodeStatus ActionFollowMaster::step()
{
    Ctrl* master = ai_->get_master();
    if (master != NULL)
    {
        chase_with_path();
        reactivate_on_success();

        float dis_of_master = ai_->get_dis_of_master(master);
        VECTOR3 master_pos = ai_->get_master_pos(master, true);
        VECTOR3 hit_pos;
        float rate;

        if (ai_->is_fake_battle())
        {
            if ((dis_of_master < ai_->get_in_range()) && ai_->raycast(ai_->get_pos(), master_pos, hit_pos, rate))
            {
                node_status_ = SUCCESS;
                ai_->reset_off_angle();
            }
        }
        else
        {
            if ((dis_of_master < SCENE_CELL) && ai_->raycast(ai_->get_pos(), master_pos, hit_pos, rate))
            {
                node_status_ = SUCCESS; 
                ai_->reset_off_angle();  
            }
        }
    }
    else
    {
        node_status_ = FAILURE;
    }
    return node_status_;
}

void ActionFollowMaster::select_path_type()
{
    select_path_type_combat_npc();
}

ActionVaryingChase::ActionVaryingChase(BTNode* _parent, AI* _id)
	: ActionChase(_parent, _id, ACTION_VARYING_CHASE)
{
	clip_time_ = THOUSAND;
	size_ = 0;
}

void ActionVaryingChase::activate()
{
	if(init_action_ || clip_time_tracker_.is_ready(ai_->get_frame_end_time()))
	{
		float total = 0;
		for(uint32_t i = 0; i < rates_.size(); ++i)
		{
			total += rates_[i];
		}

		float chosen = rand_float(0, total);
		float sum = 0;

		int32_t idx = 0;
		for(uint32_t i = 0; i < size_; ++i)
		{
			sum += rates_[i];
			if(sum >= chosen)
			{
				idx = i;
				break;
			}
		}

		set_ani_type(ani_types_[idx], true);
		//clip_time_ = clip_times_[idx]*THOUSAND;
		clip_time_tracker_.set_period(times_[idx]);
		clip_time_tracker_.activate(ai_->get_frame_begin_time());
	}

	ActionChase::activate();
}

void ActionVaryingChase::init()
{
	ActionChase::init();

	get_property_ai_type_list("ani-types", ani_types_);
	size_ = ani_types_.size();

	get_property_float_list("times", times_);
	get_property_float_list("rates", rates_);
	get_property_float_list("clip-times", clip_times_);

	// if (clip_times_.empty())
	// {
	// 	clip_times_.resize(size_, clip_time_);
	// }
	if(ai_->assert_fail(size_ > 0 &&\
		times_.size() == size_ &&\
		rates_.size() == size_))
		//clip_times_.size() == size_))
	{
		errorf("all arg counts are not equal!");
		return;
	}

	clip_time_tracker_.init(Tracker::TYPE_PERIOD, 0.f);
}

ActionSkillChase::ActionSkillChase(BTNode* _parent, AI* _id)
	: ActionChase(_parent, _id, ACTION_ENCIRCLE)
{
	min_range_ = 0.f;
	max_range_ = 1000.f;
	skill_id_ = 0;
}

void ActionSkillChase::init()
{
	ActionChase::init();

	bool ret = get_property_int("skill-id", skill_id_);
	if(ai_->assert_fail(ret && skill_id_))
	{
		errorf("no skillchase skill id");
		return;
	}

	get_property_float("min-range", min_range_);
	get_property_float("range", max_range_);

	turn_type_ = SHARE_TURN_TO_PATH;
}

NodeStatus ActionSkillChase::step()
{
	chase_with_path();
	reactivate_on_success();

	float dist = ai_->tgt_get_dist(this);
	if(dist >= min_range_ && dist <= max_range_)
	{
		node_status_ = SUCCESS;
		return node_status_;
	}

	uint32_t cur_time_ = ai_->get_frame_end_time();
    if(time_tracker_.is_ready(cur_time_))
    {
        node_status_ = SUCCESS;
        return node_status_;
    }

	if(rule_tracker_.is_ready(cur_time_))
	{
		if (check_endure_cond())
		{
			node_status_ = FAILURE;
			return node_status_;
		}
	}

	return node_status_;
}

VECTOR3 ActionSkillChase::get_seek_pos() const
{
	float dist = ai_->tgt_get_dist(this);
	if(dist < min_range_)
	{
		VECTOR3 dir =  ai_->get_pos() - ai_->tgt_get_pos(this);
		if(vc3_iszero(dir))
		{
			dir = VECTOR3(1, 0, 0);
		}
		vc3_norm(dir, dir, min_range_ - dist+1);
		return ai_->get_pos() + dir;
	}
	else
	{
		return ai_->tgt_get_pos(this);
	}
}

void ActionSkillChase::activate()
{
	float dist = ai_->tgt_get_dist(this);
	if(dist < min_range_)
	{
		set_ani_type(SHARE_ANI_WALK);
	}
	else if (dist > max_range_)
	{
		set_ani_type(SHARE_ANI_RUN);
	}
	else
	{
		node_status_ = SUCCESS;
		return;
	}

	if(time_tracker_.get_period() == 0)
	{
		node_status_ = SUCCESS;
		return;
	}

	ActionChase::activate();
}

bool ActionSkillChase::is_direct_path()
{
	ai_->raycast(path_[0], path_[1], raycast_hitpos_, raycast_rate_);
	return false;
}

float ActionSkillChase::check_endure_time(float _time)
{
	return max(_time, 0.f);
}

bool ActionShout::can_use()
{
	if(!ActionAct::can_use())
	{
		return false;
	}

	targets_.clear();

	const std::list<Obj*>& entities = ai_->get_neighbors();
	for(list<Obj*>::const_iterator iter = entities.begin(); iter != entities.end(); ++iter)
	{
		if(ai_->check_obj(*iter, OT_MONSTER, true))
		{
			float dist = ai_->dist_of_obj(*iter);
			if(dist <= range_)
			{
				if (find(ai_ids_.begin(), ai_ids_.end(), ai_->get_type_id(*iter)) != ai_ids_.end())
				{
					if(ai_->can_shout(*iter))
					{
						targets_.push_back(*iter);
					}
				}
			}
		}
	}

	return !targets_.empty();
}

void ActionShout::init()
{
	get_property_float("range", range_);

	std::vector<float> temp;
	/*bool ret = */get_property_float_list("target", temp);
// 	if(ai_->assert_fail(ret && temp.size() == 2) )
// 	{
// 		errorf("no shout target ai ids");
// 		return;
// 	}

	for(uint32_t i = 0; i < temp.size(); ++i)
	{
		ai_ids_.push_back(static_cast<int32_t>(temp[i]));
	}

	get_property_string("ani", ani_name_);
	ActionAct::init();
}

void ActionShout::activate()
{
	if(ani_name_.size())
	{
		ActionAct::activate();
	}
	else
	{
		BTLeafNode::activate();
		node_status_ = SUCCESS;
	}

	for(uint32_t i = 0; i < targets_.size(); ++i)
	{
		ai_->shout(targets_[i]);
	}
}

float ActionShout::get_default_time() const
{
	if(ani_name_.size())
	{
		return -1.f;
	}
	else
	{
		return 0.f;
	}
}

float ActionShout::check_endure_time(float _time)
{
	if(ai_->assert_fail(_time >= 0))
	{
		errorf("no default time");
	}
	return _time;
}

ActionShout::ActionShout( BTNode* _parent, AI* _id ) 
	: ActionAct(_parent, _id)
{
	act_type_ = SHARE_ACT_ONCE_STAND;
	range_ = 10.f;
	ani_name_.clear();
}


ActionSfx::ActionSfx( BTNode* _parent, AI* _id ) 
	: ActionAct(_parent, _id)
{
	sfx_id_ = 0;
	is_open_ = 1;
	delay_time_ = 0.f;
	is_self_play_ = true;
}

void ActionSfx::init()
{
	ActionAct::init();
	bool ret = get_property_int("sfx", sfx_id_);
	if(ai_->assert_fail(ret))
	{
		errorf("no sfx id");
		return;
	}

	get_property_int("open", is_open_);
	get_property_float("delay", delay_time_);

    if (COND_TARGET == target_type_)
    {
        is_self_play_ = false;
    }
}

void ActionSfx::activate()
{
	update_cd();
	if(sfx_id_ > 0 && is_open_ >= 0 && is_open_ <= MAX_SFX_OPEN_FLAG)
	{
        uint32_t obj_id = ai_->get_id(); 
        if (!is_self_play_)
        {
            obj_id = ai_->tgt_get_id(this, true);
        } 
		ai_->broadcast_mst_sfx(sfx_id_, is_open_, obj_id);
	}
	node_status_ = SUCCESS;
}

ActionSpawn::ActionSpawn( BTNode* _parent, AI* _id )
	: ActionAct(_parent, _id)
{
	act_type_ = SHARE_ACT_SPAWN;
	reborn_ = false;
	spawn_buff_ = false;
}


void ActionSpawn::clear_cd( bool _force )
{
	string inst_id ;
	int inst_type;

	if ( ai_->l_get_plane_property("inst_id_",inst_id) == false)
	{
		//LOG(2)("can't get plane property inst_id_");
		return;
	}

	if( ai_->l_get_table_field(inst_type , "INSTANCE_CFG",inst_id,"InstType") == false)
	{
		LOG(2)("can't get INSTANCE_CFG %s inst_type",inst_id.c_str());
		return;
	}

	if( inst_type == 1)
	{
		return;
	}

	if(_force)
	{
		ActionAct::clear_cd(_force);
	}
}

bool ActionSpawn::can_use()
{
	if(reborn_)
	{
		if (ai_->get_reborn_flag())
		{
			ai_->set_reborn_flag(false);
			return true;
		}
		return false;
	}
	else
	{
		bool flag = ActionAct::can_use();
		if(flag && spawn_buff_)
		{
			//ai_->use_buff(SPAWN_BUFF, 1);
		}
		return flag;
	}
}


ActionSetBoard::ActionSetBoard( BTNode* _parent, AI* _id ) 
	: ActionAct(_parent, _id)
{
	range_ = 0;
	add_ = 0;
    independ_ = 0;
}

void ActionSetBoard::init()
{
	ActionAct::init();
	get_property_string("tgt", tgt_);
	get_property_int("range", range_);
	get_property_int("add", add_);
	get_property_int("independ", independ_);
}

void ActionSetBoard::activate()
{
	update_cd();

	int32_t value = range_;
	if ( add_ > 0 )
	{
        int32_t old_value = 0;
        ai_->l_get_ai_board_value( tgt_, old_value, independ_ ); 
        value = old_value + range_;
	}

    ai_->l_set_ai_board_value( tgt_, value, independ_ );
	node_status_ = SUCCESS;
}

ActionDefault::ActionDefault( BTNode* _parent, AI* _id ) 
	: ActionAct(_parent, _id)
{
}

void ActionDefault::activate()
{
	update_cd();
	action_msg_default(MSG_ACTIVATE);
	node_status_ = SUCCESS;
}

void ActionDefault::init()
{
	ActionAct::init();
	action_msg_default(MSG_INIT);
}

ActionDrop::ActionDrop( BTNode* _parent, AI* _id )
	: BTLeafNode( _parent, _id )
{
    drop_to_enemy_ = 1;
    drop_dist_ = 40;
}


void ActionDrop::init()
{
    BTLeafNode::init();
	get_property_int( "drop-to-enemy", drop_to_enemy_ );
    get_property_float( "drop-dist", drop_dist_ );
}

void ActionDrop::activate()
{
    BTLeafNode::activate();

    if ( drop_to_enemy_ > 0 )
    {
        if ( ai_->tgt_is_type(OT_PLAYER) )
        {
            ai_->l_do_monster_drop( false, ai_->get_id(), ai_->tgt_get_id(NULL), drop_dist_ );
        }
    }
    else
    {
        ai_->l_do_monster_drop( true, ai_->get_id(), 0, drop_dist_ );
    }

	node_status_ = SUCCESS;
}

ActionUpdateFightAttr::ActionUpdateFightAttr( BTNode* _parent, AI* _id )
    : BTLeafNode( _parent, _id )
{
    check_range_ = 0;
    exclude_same_camp_ = 1;
    by_host_ = 0;
}

void ActionUpdateFightAttr::init()
{
    BTLeafNode::init();
    get_property_float( "check-player-range", check_range_);
    get_property_int( "exclude-same-camp", exclude_same_camp_);
    get_property_int( "by-host", by_host_);
}

void ActionUpdateFightAttr::activate()
{
    BTLeafNode::activate();

    if(ai_->verbose_print_)
    {
        TRACE(2)("ActionUpdateFightAttr, come in activate: obj:%d \n", ai_->get_id());
    }

    if( by_host_ == 1)
    {
        if(ai_->verbose_print_)
        {
            TRACE(2)("ActionUpdateFightAttr, update mst fight attr in activate by_host: obj:%d \n", ai_->get_id());
        }
        ai_->l_update_mst_fight_attr_fit_host(ai_->get_id());
        node_status_ = SUCCESS;
        return;
    }

    if(check_range_ > 0 )
    {
        int32_t _obj_type = Obj::LinkPlayer;
        std::vector<uint32_t> list;
        VECTOR3 pos = ai_->get_pos();
        FOR_LINKMAP( ai_->get_world(), ai_->get_plane(), pos, check_range_, _obj_type)
        {
            if(ai_->check_obj(obj, _obj_type, true) &&
                ai_->get_obj_pos_dist(obj, pos) < check_range_)
            {
                if(exclude_same_camp_ && ai_->is_enemy(obj))
                {
                    list.push_back(obj->get_id());
                }
                else if(!exclude_same_camp_)
                {
                    list.push_back(obj->get_id());
                }
            }
        }
        END_LINKMAP

        if(list.size() > 0 )
        {
            //lua call update owner's attr, with list info
            ai_->l_update_mst_fight_attr_fit_pl(ai_->get_id(), list);
            if(ai_->verbose_print_)
            {
                TRACE(2)("ActionUpdateFightAttr, update mst fight attr in activate: obj:%d \n", ai_->get_id());
            }
        }
    }
	node_status_ = SUCCESS;
}

ActionReturnHome::ActionReturnHome( BTNode* _parent, AI* _id )
	: ActionChase( _parent, _id )
{
    home_dist_square_ = -1.0f;
}

void ActionReturnHome::init()
{
	ActionChase::init();
    float dist = -1.0f;
	get_property_float("home-dist", dist);

    if (dist <= 0)
    {
		errorf("home-dist must greater than zero");
		return;
    }
    home_dist_square_ = dist * dist;
    ai_->set_home_dist_square(home_dist_square_);

	ANI_TYPE global_ani_type = tree_->get_global_ani_type();
	if (global_ani_type != SHARE_ANI_NONE)
	{
		set_ani_type(global_ani_type);
	}

	ANI_TYPE ani_type;
	if (get_property_ai_type("ani-type", ani_type))
	{
		set_ani_type(ani_type);
	}
}

void ActionReturnHome::activate()
{
    BTLeafNode::activate();
    set_time(get_default_time());
	init_path();
	select_path_type();
    ai_->l_set_obj_god_mode(ai_->get_id(), true);
    ai_->set_returning_home_flag(true);
}

NodeStatus ActionReturnHome::step()
{
	uint32_t cur_time = ai_->get_frame_end_time();
	if (time_tracker_.is_ready(cur_time))
	{
		node_status_ = SUCCESS;
	}
    else
    {
        chase_with_path();
    }

    if (node_status_ == SUCCESS || node_status_ == FAILURE )
    {
        ai_->find_chase_aoi_enemy(); 
        ai_->l_set_obj_god_mode(ai_->get_id(), false);
        ai_->set_returning_home_flag(false);
    }
    return node_status_;
}

bool ActionReturnHome::can_use()
{
    return ai_->need_return_home();
}

VECTOR3 ActionReturnHome::get_seek_pos() const
{
    return ai_->get_spawn_point();
}

ActionChargeSkill::ActionChargeSkill(BTNode* _parent, AI* _id)
	: ActionSkill(_parent, _id)
{
    is_config_error_ = false;
    do_charge_skill_ = false;
    charge_skill_id_ = 0;
    charge_time_ = 0;
    charge_time_range_.clear();
}

void ActionChargeSkill::init()
{
    ActionSkill::init();

	get_property_int_list("charge-time-range", charge_time_range_);
    if (charge_time_range_.size() < 2)
    {
        is_config_error_ = true;
        errorf("charge-time-range value num is less than 2");
    }
    else 
    {
        if (charge_time_range_[0] > charge_time_range_[1])
        {
            is_config_error_ = true;
            errorf("charge-time-range max value must be larger than min value");
        }
    }
}

void ActionChargeSkill::activate()
{
    if (is_config_error_)
    {
        return;
    }

    charge_skill_id_ = 0;
    charge_time_ = 0;
    bool is_succ = ai_->l_get_charge_skill_id(skill_id_, charge_time_range_, charge_time_, charge_skill_id_);
    if (!is_succ)
    {
        return;
    }

	update_cd();
	ai_->set_speed_len(0);
	ai_->clear_smoother();
	in_ai_delay_ = false; 

    uint32_t activate_time = ai_->get_frame_begin_time();
    charge_time_ = charge_time_ / 1000;    //convert ms to s
	charge_time_tracker_.init(Tracker::TYPE_PERIOD, charge_time_);
	charge_time_tracker_.activate(activate_time);
	max_time_tracker_.activate(activate_time);
    do_charge_skill_ = false;
    ai_->l_play_charge_skill_idle_anim(ai_->get_pos());  
}

NodeStatus ActionChargeSkill::step()
{
    if (is_config_error_ || charge_skill_id_ <= 0)
    {
        node_status_ = FAILURE;
        return node_status_;
    }

	if (!do_charge_skill_)
    {
        if (charge_time_tracker_.is_ready(ai_->get_frame_end_time()))
        {
            set_angle();
            use_skill(charge_skill_id_);
            do_charge_skill_ = true;
        }
        return node_status_;
    }

    return ActionSkill::step();
}

ActionChaseSkill::ActionChaseSkill(BTNode* _parent, AI* _id)
	: ActionSkill(_parent, _id)
{
    turn_speed_ = 0;
	clip_time_ = 0.5f;
	chase_dist_ = 0;
    chase_speed_ = 0;
    path_counter_ = 0;
	raycast_rate_ = 0;
    move_persist_skill_ = 0;
    is_first_skill_end_ = false;
    raycast_rate_ = 0;
    last_target_id_ = 0;
    min_dist_to_target_ = 0.5f;
}

void ActionChaseSkill::init()
{
    ActionSkill::init();

	get_property_int("move-persist-skill", move_persist_skill_);

    if (move_persist_skill_ > 0)
    {
        get_property_float("chase-speed", chase_speed_);
        if (chase_speed_ <= 0)
        {
            errorf("chase-speed must be greater than 0");
            chase_speed_ = 1.0f;
        }

        get_property_float("min-dist-to-target", min_dist_to_target_);
        if (min_dist_to_target_ < 0)
        {
            errorf("min-dist-to-target must be greater than or equal to 0");
            min_dist_to_target_ = 0.5f;
        }

        clip_time_ = 0.5f;
    }
    else
    {
        get_property_float("turn-speed", turn_speed_);
        if (turn_speed_ <= 0)
        {
            errorf("turn-speed must be greater than 0");
            turn_speed_ = 90.0f;
        }
        clip_time_ = 0.3f;
    }

	time_tracker_.init(Tracker::TYPE_PERIOD, 0);
    turn_time_tracker_.init(Tracker::TYPE_PERIOD, clip_time_);
}

void ActionChaseSkill::activate()
{
	if (init_action_)
	{
        update_cd();
        ai_->set_speed_len(1);
        ai_->clear_smoother();
        in_ai_delay_ = false; 
        is_first_skill_end_ = false;
        last_target_id_ = 0;
        path_.clear();

		ai_->speed_vec_to_tgt(NULL, 0);
        use_skill(skill_id_);
        max_time_tracker_.activate(ai_->get_frame_begin_time());
	}

    if (is_first_skill_end_)
    {
        return;
    }

    if (move_persist_skill_ > 0)
    {
        init_chase_path();
        if (path_.empty())
        {
            ai_->set_speed_len(0);
        }
    }
    else
    {
        turn_to_target();
    }
}

NodeStatus ActionChaseSkill::step()
{
    if (is_first_skill_end_)
    {
        return ActionSkill::step();
    }

    if (move_persist_skill_ > 0)
    {
        if (path_.size() >= 2)
        {
            chase_with_path();
        }
        else
        {
            node_status_ = SUCCESS;
        }
    }
    else
    {
        if (last_target_id_ > 0)
        {
            if (turn_time_tracker_.is_ready(ai_->get_frame_end_time()))
            {
                node_status_ = SUCCESS;
            }
        }
        else
        {
            node_status_ = SUCCESS;
        }
    }

    reactivate_on_success();

    return ActionSkill::step();
}

void ActionChaseSkill::on_message(MSG_TYPE _msg)
{
    if (_msg != AI_SKILL_END)
    {
        return;
    }

    is_first_skill_end_ = true;
    ActionSkill::on_message(_msg);
}

bool ActionChaseSkill::is_target_on_left(const VECTOR3& _target_pos, const VECTOR3& _cur_pos, const VECTOR3& _speed_vec)
{
	VECTOR3 target_vec = _target_pos - _cur_pos;
	VECTOR3 prod;
	vc3_cross(prod, target_vec, _speed_vec);
	return prod.y < 0;
}

void ActionChaseSkill::turn_to_target()
{
    Obj* target = ai_->tgt_get(this);
    if (target == NULL)
    {
        last_target_id_ = 0;
        return;
    }

    last_target_id_ = ai_->tgt_get_id(this);
    turn_time_tracker_.activate(ai_->get_frame_begin_time());

    //if not direct path to target, find direction to bypass
    VECTOR3 cur_pos = ai_->get_pos();
    VECTOR3 target_pos = ai_->tgt_get_pos(NULL);
    if (!ai_->raycast(cur_pos, target_pos, raycast_hitpos_, raycast_rate_))
    {
        Navmesh::Path temp_path;
        if (ai_->find_path(cur_pos, target_pos, &temp_path, 0x0) && temp_path.size() >= 2)
        {
            target_pos = temp_path[1];
            target_pos.y = 0;
        }
    }

    VECTOR3 speed_vec = ai_->get_angle_vec();  
    float turn_angle = degrees(dot_angle(target_pos - cur_pos, speed_vec));
    float max_turn_angle = turn_speed_ * clip_time_;
    turn_angle = (turn_angle < max_turn_angle) ? turn_angle : max_turn_angle; 

    int sign = is_target_on_left(target_pos, cur_pos, speed_vec) ? -1 : 1;
    turn_angle = turn_angle * sign;

    //calculate target angle
    float x = speed_vec.x * COS(turn_angle) - speed_vec.z * SIN(turn_angle); 
    float z = speed_vec.x * SIN(turn_angle) + speed_vec.z * COS(turn_angle); 
	speed_vec.x = x;
    speed_vec.z = z;
    ai_->set_speed_vec(speed_vec);

    float cur_angle = norm_angle(180 - ai_->owner_->get_angle_y());
    float target_angle = norm_angle(cur_angle + turn_angle);
    ai_->l_broadcast_turn_to_skill_angle(target_angle);
}

float ActionChaseSkill::get_speed() const
{
    float speed = chase_speed_ * ai_->get_owner_speed(); 
    if (speed <= 0)
    {
        ActionChaseSkill* no_const_ref = const_cast<ActionChaseSkill*>(this);
        no_const_ref->errorf("speed must be greater than 0");
        speed = 0.1f;
    }
    return speed;
}

void ActionChaseSkill::init_chase_path()
{
    path_.clear();
	path_timer_.clear();
	path_counter_ = 0;

    if (!ai_->is_owner_in_state(STATE_MOVE_PERSIST))
    {
        return;
    }

    Obj* target = ai_->tgt_get(this);
    if (target == NULL)
    {
        return;
    }

    VECTOR3 cur_pos = ai_->get_pos();
    VECTOR3 target_pos = ai_->tgt_get_pos(NULL);
    if (length2d(target_pos - cur_pos) <= min_dist_to_target_)
    {
        return;
    }

    path_.push_back(cur_pos);
    path_.push_back(target_pos);
    float speed = get_speed();
    chase_dist_ = speed * clip_time_;

	if (ai_->raycast(cur_pos, target_pos, raycast_hitpos_, raycast_rate_))
	{
		calculate_path_and_time(PATH_DIRECT);
	}
	else
	{
        Navmesh::Path temp_path;
		bool result = ai_->find_path(cur_pos, target_pos, &temp_path, 0x0);
		if (result)
		{
			path_ = temp_path;
            calculate_path_and_time(PATH_DETOUR);
		}
        else
        {
            if (raycast_rate_ > 0.1)
            {
                path_[1] = raycast_hitpos_;
                calculate_path_and_time(PATH_DIRECT);
            }
            else
            {
                path_.clear();
            }
        }
	}

    if (path_.size() >= 2)
    {
        ai_->l_broadcast_chase_skill_move(0, path_, speed);
    }
    else
    {
        path_.clear();
    }
}

void ActionChaseSkill::calculate_path_and_time(AI_TYPE _path_type)
{
	path_counter_ = 0;
	path_timer_.clear();

    if (path_.size() < 2)
    {
        path_.clear();
        return;
    }

    if (_path_type == PATH_DIRECT)
    {
        clip_direct_path();
	}
    else
	{
        clip_detour_path();
	}

	double time = static_cast<double>(ai_->get_frame_begin_time());
	double use_time = 0;
	VECTOR3 last_pos = path_[0];
    float speed = get_speed();

	for (uint32_t i = 0; i < path_.size(); ++i)
	{
		use_time = length2d(path_[i] - last_pos) / speed * THOUSAND;  //need convert to ms
		last_pos = path_[i];
		time += use_time;
		path_timer_.push_back(static_cast<uint32_t>(time));
	}

	ai_->set_speed_vec(path_[1] - path_[0]);
}

void ActionChaseSkill::clip_detour_path()
{
	float length = 0, old_length = 0, seg_length = 0;

	for (uint32_t i = 1; i < path_.size(); ++i)
	{
		old_length = length;
		seg_length = length2d(path_[i] - path_[i-1]);
		length += seg_length;

		if (length > chase_dist_)
		{
			Navmesh::Path new_path;
			for (uint32_t j = 0; j < i; ++j)
			{
				new_path.push_back(path_[j]);
			}

            VECTOR3 end_point;
            vc3_intrp(end_point, path_[i-1], path_[i], (chase_dist_ - old_length) / seg_length);
            new_path.push_back(end_point);

			path_ = new_path;
			return;
		}
	}
}

void ActionChaseSkill::clip_direct_path()
{
    VECTOR3 target_vec = path_[1] - path_[0];
    target_vec.y = 0;
    if (length2d(target_vec) > chase_dist_)
    {
        vc3_norm(target_vec, target_vec, chase_dist_);
        path_[1] = path_[0] + target_vec;
    }
}

void ActionChaseSkill::chase_with_path()
{
    if (path_timer_.size() != path_.size())
    {
		errorf("size of path_timer_ is not equal to size of path_");
        node_status_ = SUCCESS;
        return;
    }

    int32_t max_idx = static_cast<int32_t>(path_.size() - 1);
    if (path_counter_ >= max_idx)
    {
        node_status_ = SUCCESS;
        return;
    }

    int32_t next_idx = path_counter_ + 1;
    uint32_t start_time = path_timer_[path_counter_];
    uint32_t end_time = path_timer_[next_idx];
    VECTOR3 end_pos = path_[next_idx];
	uint32_t cur_time = ai_->get_frame_end_time();
    VECTOR3 cur_pos;

    if (cur_time >= end_time)
    {
        cur_pos = end_pos;
        path_counter_ += 1;

        if (path_counter_ >= max_idx)
        {
            node_status_ = SUCCESS;
        }
        else
        {
            VECTOR3 start_pos = path_[path_counter_];
            end_pos = path_[path_counter_ + 1];
            ai_->set_speed_vec(end_pos - start_pos);
        }
    }
    else
    {
        float rate = static_cast<float>(cur_time - start_time) / (end_time - start_time);
        const VECTOR3& start_pos = path_[path_counter_];
        vc3_intrp(cur_pos, start_pos, end_pos, rate);
    }

    ai_->set_pos(cur_pos);

    if (ai_->verbose_print_)
    {
        TRACE(2)("ai_debug cur_pos %f %f %f\n", cur_pos.x, cur_pos.y, cur_pos.z);
    }
}

int ActionChaseSkill::sync_path_to() const
{
	if (node_status_ != RUNNING)
	{
        return 0;
    }
    
    if (move_persist_skill_ > 0 && path_.size() >= 2)
    {
		Navmesh::Path cur_path;
        cur_path.push_back(ai_->get_pos());
        int32_t next_idx = path_counter_ + 1; 
        if (next_idx < (int32_t)path_.size())
        {
            cur_path.insert(cur_path.end(), path_.begin() + next_idx, path_.end());
        }

		if (cur_path.size() >= 2)
		{	
            float speed = get_speed();
            bool is_succ = ai_->l_broadcast_chase_skill_move(1, cur_path, speed);
            return is_succ ? 1 : 0;
		}
	}
	return 0;
}

}
