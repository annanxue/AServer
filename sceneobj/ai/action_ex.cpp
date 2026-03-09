#include "action_ex.h"

namespace SGame
{

ActionEmote::ActionEmote( BTNode* _parent, AI* _id ) 
	:ActionAct(_parent, _id)
{

}

ActionActivateTrig::ActionActivateTrig( BTNode* _parent, AI* _id ) 
	:BTLeafNode(_parent, _id)
{
}

void ActionActivateTrig::init()
{
    BTLeafNode::init();
	get_property_float_list("chunk-idx", chunk_idx_);
}

void ActionActivateTrig::activate()
{
    BTLeafNode::activate();
	ai_->l_activate_trig(chunk_idx_);
	node_status_ = SUCCESS;
}

ActionMoveTo::ActionMoveTo( BTNode* _parent, AI* _id ) 
	:ActionChase(_parent, _id)
{
	vc3_init(seek_pos_, 0, 0, 0);
	front_dis_ = 0;
	find_path_ = 1;
    towards_ = 0;
}

void ActionMoveTo::init()
{
	ActionChase::init();
	get_property_int("find-path", find_path_);

	get_property_float("front-dis", front_dis_);
    get_property_int( "towards", towards_ );
	if (front_dis_ > 0 || target_last())
	{
		return;
	}

	std::vector<float> pos;
	bool ret = get_property_float_list("pos", pos);

	if(ai_->assert_fail(ret && pos.size() == 2))
	{
		errorf("ActionMoveTo no pos");
		return;
	}

	seek_pos_.x = pos[0];
	seek_pos_.z = pos[1];
}

VECTOR3 ActionMoveTo::get_seek_pos() const
{
	if(target_last())
	{
		return ai_->tgt_get_pos(this);
	}
	else if(front_dis_ > 0)
	{
        if( towards_ > 0 ) 
        {
            return ai_->get_pos() + ai_->get_forward_angle_vec() * front_dis_;         
        }
        else
        {
		    return ai_->get_pos() + ai_->get_angle_vec() * front_dis_;
        }
	}
	else
	{
		return seek_pos_;
	}
}

void ActionMoveTo::select_path_type()
{
	if(find_path_)
	{
		ActionChase::select_path_type();
	}
	else
	{
		calcu_path(PATH_DIRECT);
	}
}

NodeStatus ActionMoveTo::step()
{
	chase_with_path();
	uint32_t cur_time = ai_->get_frame_end_time();
	if(rule_tracker_.is_ready(cur_time))
	{
		if (check_endure_cond())
		{
			node_status_ = SUCCESS;
			return node_status_;
		}
	}
	if(time_tracker_.is_ready(cur_time))
	{
		node_status_ = SUCCESS;
		return node_status_;
	}
	return node_status_;
}



ActionFollowMonster::ActionFollowMonster( BTNode* _parent, AI* _id ) 
	:ActionChase(_parent, _id)
{
	vc3_init(seek_pos_, 0, 0, 0);
}

void ActionFollowMonster::init()
{
	ActionChase::init();
	bool ret = get_property_int_list("monster", monster_type_id_);

	if(ai_->assert_fail(ret))
	{
		errorf("ActionFollowMonster  no type id ");
		return;
	}
}

void ActionFollowMonster::activate()
{
	Obj * target = NULL;
	for (size_t i = 0; i < monster_type_id_.size() ; ++i)
	{
		int mst_type_id = monster_type_id_[i];
		int ctrl_id = ai_->l_get_plane_monster( mst_type_id );
		target = ai_->get_obj( ctrl_id );
		if( ctrl_id >0 && is_valid_obj(target))
		{
			break;
		}
	}

	if(target)
	{
		seek_pos_ = target->getpos();
	}else{
		seek_pos_ = ai_->get_pos();
	}

	ActionChase::activate();
}

ActionSetSection::ActionSetSection( BTNode* _parent , AI* _id ) 
	:ActionAct(_parent , _id)
{
}

void ActionSetSection::init()
{
	ActionAct::init();
	bool ret = get_property_float_list("range", section_range_);

	if ( ai_->assert_fail(ret) || section_range_.size() < 2 ||  section_range_[0] > section_range_[1] )
	{
		errorf("ActionSetSection section error ");
		return;
	}
}

void ActionSetSection::activate()
{
	ActionAct::activate();

	if( section_range_[0] == section_range_[1])
	{
		ai_->set_section( section_range_[0] );
		return ;
	}

	while (true)
	{
		int section = rand_int(section_range_[0] , section_range_[1]);
		if( section != ai_->get_section() )
		{
			ai_->set_section( section );
			return;
		}
	}
}

}
