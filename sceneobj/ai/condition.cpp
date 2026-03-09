#include "condition.h"
#include <string>
#include <list>
#include <vector>
#include "common.h"
#include "btree.h"
#include "component_ai.h"
#include "timer.h"

namespace SGame
{

ConditionFailure::ConditionFailure( BTNode* _parent, AI* _id ) 
	: BTLeafNode(_parent, _id)
{

}

void ConditionSuccessful::init()
{
	BTNode::init();
}

void ConditionSuccessful::activate()
{
	node_status_ = SUCCESS;
}

ConditionSuccessful::ConditionSuccessful( BTNode* _parent, AI* _id ) 
	: BTLeafNode(_parent, _id)
{

}

void ConditionFailure::init()
{
	BTNode::init();
}

void ConditionFailure::activate()
{
	node_status_ = FAILURE;
}


ConditionSVO::ConditionSVO( BTNode* _parent, AI* _id ) 
	: BTLeafNode(_parent, _id), time_(0.f), state_(-1), radius_(0.f), camp_(ACTION_THINK), side_type_(ACTION_THINK), 
      finished_spawn_(-1), is_buff_id_(0)
{
	vc3_init(pos_, 0, 0, 0);
	is_default_ = false;
    independ_ = 0;
}

void ConditionSVO::init()
{
	BTNode::init();
	init_cd();

	get_property_string("target", target_str_);
    target_type_ = get_ai_type( target_str_ );

	if(target_type_== ACTION_THINK || target_type_ == TARGET_LAST || target_type_ == COND_SELF )
	{
        std::string tgt_sub = target_str_.substr(0,5); 
        if (tgt_sub == "board")
        {
            target_type_ = get_ai_type( tgt_sub );
            target_substr_ = target_str_.substr(5);
        }
        else
        {
		    std::vector<float> temp;
		    bool ret = get_property_float_list("target", temp);
		    if(ai_->assert_fail(ret))
		    {
			    errorf("no svo condition target ids");
			    return;
		    }

		    for(uint32_t i = 0; i < temp.size(); ++i)
		    {
			    target_ids_.push_back(static_cast<int32_t>(temp[i]));
		    }
			is_default_ = temp.size() == 1 && temp[0] == 0;
        }
	}
	get_property_string("event", event_str_);
	event_ = get_ai_type( event_str_ );

	if(ai_->assert_fail(get_property_string("range", range_str_)))
	{
		errorf("error svo condition");
		return;
	}

	if (is_default_)
	{
		svo_msg_default(MSG_INIT);
	}

	get_property_float("time", time_);
    get_property_int("state", state_);

	get_property_float("rad", radius_);

	get_property_int_list("camps", camps_);
    get_property_int("finished-spawn", finished_spawn_);
    get_property_int("isbuffid", is_buff_id_);

	std::string camp_str;
	if(get_property_string("camp", camp_str))
	{
		camp_ = get_ai_type( camp_str );
	}

	std::vector<float> pos;
	if (get_property_float_list("pos", pos) && pos.size() == 2)
	{
		vc3_init(pos_, pos[0], 0, pos[1]);
	}

    get_property_int("independ", independ_);

    std::string side_str;
	if(get_property_string("side", side_str))
    {
        side_type_ = get_ai_type(side_str);
    }
}

void ConditionSVO::set_id(int32_t _id)
{
	BTLeafNode::set_id(_id);
	tree_->register_condition(this);
}

bool ConditionSVO::eval_section()
{
	if( ranges_.empty() )
		return false;

	if( COND_EQUAL == event_  && ranges_[0] == ai_->get_section() )
	{
		return true;
	}

	return false;
}

bool ConditionSVO::eval_time()
{
	string period_str = ai_->get_ai_info("period");
	if( period_str.empty())
	{
		LOG(2)("ConditionSVO::CheckRuleTime 失败,不能进行周期换阶段,请在角色AI表中填上周期时间");
		return false;
	}

	int period = atoi( period_str.c_str() );
	if( period <= 0 )
		return false;

	if( ranges_.size() < 2 )
	{
		return false;
	}

	if( COND_IN != event_ )
		return false;

	if( ranges_[0] >  ranges_[1] )
	{
		LOG(2)("ConditionSVO::eval_cond_time 失败");
		return false;
	}

	period =  period* THOUSAND; // s to ms
	//int current = ai_->get_frameEnd_time() % period;
	uint32_t current = (ai_->get_frame_counter() * 1000 / g_timermng->get_frame_rate()) % period;	// time from ai start

	if(  ranges_[0] * THOUSAND <= current  && current  < ranges_[1] * THOUSAND)
	{
		return true;
	}

	return false;
}

void ConditionSVO::activate()
{
	update_cd();
	node_status_ = SUCCESS;
}

bool ConditionSVO::can_use()
{
	if(!BTLeafNode::can_use())
    {
        return false;
    }

	if (is_default_)
	{
		return svo_msg_default(MSG_ACTIVATE);
	}

	bool flag = false;

	switch(target_type_)
	{
	case COND_TARGET:
		flag = eval_target();
		break;

	case ACTION_THINK:
	case COND_MONSTER:
	case COND_USER:
		flag = eval_obj();
		break;

	case COND_ENEMY:
	case COND_FRIEND:
		flag = eval_obj_camp(target_type_);
		break;

	case COND_SECTION:
		flag = eval_section();
		break;

	case COND_TIME:
		flag = eval_time();
		break;

	case COND_TARGET_ANGLE:
		flag = check_num( norm_angle( ai_->get_angle_with_target() - ai_->get_spawn_angle() ), ranges_[0]);
		break;

    case COND_TARGET_SIDE:
        flag = eval_target_side();
        break;

	case COND_BOARD:
		flag = eval_board();
		break;

	case COND_STATE:
		flag = eval_state();
		break;

	case COND_MASTER:
		flag = eval_master();
		break;

	case COND_POS_DIST:
		flag = check_num(ai_->get_obj_dist(pos_), ranges_[0]);
		break;

	case COND_AREA_OBJ:
		flag = eval_obj_area();
		break;

    case COND_RAGE_VALUE:
        flag = eval_rage_value(); 
        break;

	default:
		break;
	}
	return flag;
}

bool ConditionSVO::check_range_dists(float _dist)
{
	if(ranges_.size() == 1)
	{
		return _dist <= ranges_[0];
	}
	else if (ranges_.size() == 2)
	{
		return ranges_[0] <= _dist  && _dist <= ranges_[1];
	}
	return false;
}

bool ConditionSVO::eval_obj()
{
	if (COND_DIE_IN == event_)
	{
		std::vector<uint32_t> list;
		FOR_LINKMAP( ai_->get_world(), ai_->get_plane(), ai_->get_pos(), ai_->get_aoi_radius(), Obj::LinkDynamic )
		{
			if(ai_->check_obj(obj, OT_MONSTER, false) &&
				check_range_dists(ai_->dist_of_obj(obj)) &&
				check_type_id(obj) &&
				ai_->get_obj_die_time(obj) >= time_)
			{
				list.push_back(obj->get_id());
			}
		}
		END_LINKMAP

		if(!list.empty())
		{
			ai_->set_last(list);
		}

		if (ranges_.size() <= 2)
		{
			return list.size() >= 1;
		}
		else
		{
			return list.size() >= ranges_[2];
		}
	}
	else
	{
		std::vector<uint32_t> list;

		int32_t obj_type = 0;
		if (target_type_ == COND_MONSTER )
		{
			obj_type = OT_MONSTER;
		}
		else if(target_type_ == COND_USER)
		{
			obj_type = OT_PLAYER;
		}
		else if(target_type_ == ACTION_THINK)
		{
			if(!target_ids_.empty() && target_ids_[0] < 10000)
			{
				obj_type = OT_TRIGGER;
			}
			else
			{
				obj_type = OT_MONSTER;
			}
		}

		int obj_link = obj_type == OT_PLAYER ? Obj::LinkPlayer : Obj::LinkDynamic;

		FOR_LINKMAP( ai_->get_world(), ai_->get_plane(), ai_->get_pos(), ai_->get_aoi_radius(), obj_link)
		{
			if(ai_->check_obj(obj, obj_type, true) &&
                check_spawned(obj, obj_type) &&
				check_range_num_dist(ai_->dist_of_obj(obj)) &&
				check_type_id(obj))
			{
				if (obj_type == OT_TRIGGER && state_ != -1)
				{
					int32_t cur_state = ai_->l_get_trigger_cur_state(obj->get_id());
					if (cur_state == state_)
					{
						list.push_back(obj->get_id());
					}
				}
				else
				{
					list.push_back(obj->get_id());
				}
			}
		}
		END_LINKMAP

		if(!list.empty())
		{
			ai_->set_last(list);
		}
		return check_num(list.size(), ranges_[0]);
	}
}

bool ConditionSVO::check_type_id( Ctrl *obj )
{
	return check_not_self(obj) && 
		(target_ids_.empty() || find(target_ids_.begin(), target_ids_.end(), ai_->get_type_id(obj)) != target_ids_.end());
}

bool ConditionSVO::check_num( float _num, float _range )
{
	bool flag = false;
	if(COND_LESS == event_)
	{
		flag = _num < _range;
	}
	else if(COND_GREAT == event_)
	{
		flag = _num > _range;
	}
	else if(COND_EQUAL == event_)
	{
		flag = (int)_num == (int)_range;
	}
	return flag;
}

bool ConditionSVO::eval_master()
{
    bool flag = false;

    Ctrl* master = ai_->get_master();
    if (master != NULL)
    {
        if(event_str_ == "buffgroup")
        {
            flag = ai_->is_in_buff_group(master->get_id(), ranges_[0]);
        }
        else
        {
            float dis = ai_->get_dis_of_master(master);
            float range = 0.f;
            int range_type = get_ai_type(range_str_);

            // get range val
            if (COND_IN == range_type)
            {
                range = ai_->get_in_range();
            }
            else if (COND_OUT == range_type)
            {
                range = ai_->get_out_range();
            }

            // check if condition met
            if (COND_IN == event_)
            {
                flag = (range > dis);
            }
            else if (COND_OUT == event_)
            {
                flag = (range < dis);
            }
        }
    }
    return flag;
}

bool ConditionSVO::eval_state()
{
    bool flag = false;
    if(COND_EQUAL == event_)
    {
        if("superarmor" == range_str_)
        {
            flag = ai_->is_super_armor();
        }
    }
    return flag;
}

bool ConditionSVO::eval_board()
{
    int32_t value = 0; 
    bool is_succ = ai_->l_get_ai_board_value( target_substr_, value, independ_ );
    if (is_succ)
    {
        return check_num(value, ranges_[0]);
    }
    return false;
}

bool ConditionSVO::eval_target()
{
	bool flag = false;
    Obj* tgt_obj = ai_->tgt_get(NULL);
    Spirit* target = (tgt_obj != NULL) ? static_cast<Spirit*>(tgt_obj) : NULL;

	if (COND_IN	== event_)
	{
		int32_t start = range_pair_[0];
		int32_t end = range_pair_[1];
		bool start_flag = true;
		bool end_flag = true;

		if (-1 != start)
		{
			start_flag = !ai_->query_common_range(start);
		}

		if (-1 != end)
		{
			end_flag = ai_->query_common_range(end);
		}
		if (range_type_ == RANGE_LIST)
		{
			end_flag = ai_->is_in_battle();
		}
		else if (range_type_ == RANGE_MELEE)
		{
			end_flag = ai_->query_melee_range();
		}
		flag = start_flag && end_flag;
	}
	else if (COND_STATE	== event_)
	{
		int32_t range_state = get_ai_type(range_str_);
		flag = (target != NULL) && (target->state.is_state(range_state) > 0);
	}
    else if (COND_BUFF == event_)
	{
		flag = (target != NULL) && (ai_->is_obj_in_buff(target->get_id(), ranges_, is_buff_id_));
	}

	return flag;
}

bool ConditionSVO::eval_obj_area()
{
	std::vector<uint32_t> list;
	
	eval_obj_area_imp(list, Obj::LinkPlayer, OT_PLAYER);
	eval_obj_area_imp(list, Obj::LinkDynamic, OT_MONSTER);

	if(!list.empty())
	{
		ai_->set_last(list);
	}
	return check_num(list.size(), ranges_[0]);
}

void ConditionSVO::eval_obj_area_imp( std::vector<uint32_t> &list, int32_t _link_type, int32_t _obj_type )
{
	FOR_LINKMAP( ai_->get_world(), ai_->get_plane(), pos_, radius_, _link_type )
	{
		if(ai_->check_obj(obj, _obj_type, true) &&
			check_camps(obj) &&
			ai_->get_obj_pos_dist(obj, pos_) < radius_)
		{
			list.push_back(obj->get_id());
		}
	}
	END_LINKMAP
}

bool ConditionSVO::svo_msg_default(AI_MSG _msg)
{
	return ai_->svo_msg(target_str_, _msg, get_id(), event_str_, range_str_, target_ids_, ranges_);
}

bool ConditionSVO::eval_obj_camp( AI_TYPE _ai_type )
{
	std::vector<uint32_t> list;

	eval_obj_camp_imp(list, Obj::LinkPlayer, OT_PLAYER, _ai_type);
	eval_obj_camp_imp(list, Obj::LinkDynamic, OT_MONSTER, _ai_type);

	if(!list.empty())
	{
		ai_->set_last(list);
	}
	return check_num(list.size(), ranges_[0]);
}

void ConditionSVO::eval_obj_camp_imp( std::vector<uint32_t> &list, int32_t _link_type, int32_t _obj_type, AI_TYPE _ai_type )//hq
{
	FOR_LINKMAP( ai_->get_world(), ai_->get_plane(), ai_->get_pos(), ai_->get_aoi_radius(), _link_type )
	{
		if(check_not_self(obj) &&
			ai_->check_obj(obj, _obj_type, true) &&
			check_camp(obj, _ai_type) &&
            check_spawned(obj, _obj_type) &&
			check_range_num_dist(ai_->dist_of_obj(obj)))
		{
			list.push_back(obj->get_id());
		}
	}
	END_LINKMAP
}

bool ConditionSVO::eval_target_side()
{ 
    VECTOR3 forward_vec = ai_->get_angle_vec();
    VECTOR3 tgt_vec = ai_->tgt_get_pos(this) - ai_->get_pos();
    forward_vec.y = 0;
    tgt_vec.y = 0;
 
	VECTOR3 product_vec;
	vc3_cross(product_vec, tgt_vec, forward_vec);
    float diff_angle = degrees(dot_angle(forward_vec, tgt_vec));
    AI_TYPE side_type = product_vec.y < 0 ? COND_LEFT : COND_RIGHT;

    if (side_type != side_type_)
    {
        return false;
    }
    return check_num( diff_angle, ranges_[0] );   
}

bool ConditionSVO::check_range_num_dist( float _dist )
{
	if(ranges_.size() == 1)
	{
		return true;
	}
	else if (ranges_.size() == 2)
	{
		return _dist <= ranges_[1];
	}
	return false;
}

bool ConditionSVO::check_camp( Ctrl * _obj, AI_TYPE _ai_type )
{
	if (_ai_type == COND_ENEMY)
	{
		return ai_->is_enemy(_obj);
	}
	if(_ai_type == COND_FRIEND)
	{
		return ai_->is_friend(_obj);
	}
	return false;
}

bool ConditionSVO::check_not_self( Ctrl *_obj )
{
	return _obj->get_id() != ai_->get_id();
}

bool ConditionSVO::check_camps( Ctrl * _obj )
{
	return check_camp(_obj, camp_) || find(camps_.begin(), camps_.end(), ai_->get_obj_camp(_obj)) != camps_.end();
}

bool ConditionSVO::check_spawned(Ctrl* _obj, int32_t _obj_type)
{
    if (_obj_type != OT_MONSTER || finished_spawn_ == -1)
    {
        return true;
    }

    bool is_finished = false;
    bool is_succ = ai_->l_has_monster_finished_spawn(_obj->get_id(), is_finished);
    if (is_succ)
    {
        int32_t finished_spawn = is_finished ? 1 : 0;
        return finished_spawn == finished_spawn_;
    }
    return false;
}

bool ConditionSVO::eval_rage_value()
{
    if (ranges_.empty())
    {
        return false;
    }

    float rage_value = 0; 
    bool is_succ = ai_->l_get_rage_value(rage_value);
    if (is_succ)
    {
        return check_num(rage_value, ranges_[0]);
    }
    return false;
}

}
