#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include "btnode.h"
#include "btree.h"
#include "common.h"
#include "component_ai.h"
#include "condition.h"
#include "states.h"

namespace SGame
{

MyMapStr BTNode::ai_type_map_;

AIRegType BTNode::reg_type_[] =
{
    { "player", COND_TARGET }, 
    { "target", COND_TARGET }, 
    { "master", COND_MASTER }, 
    { "in", COND_IN }, 
    { "out", COND_OUT }, 
    { "monster", COND_MONSTER }, 
    { "user", COND_USER }, 
    { "diein", COND_DIE_IN }, 
    { "targetangle", COND_TARGET_ANGLE }, 
    { "LOOP", SHARE_ACT_LOOP }, 
    { "STAND", SHARE_ACT_STAND }, 
    { "ONCESTAND", SHARE_ACT_ONCE_STAND }, 
    { "ONCESTILL", SHARE_ACT_ONCE_STILL }, 
    { "SPAWN", SHARE_ACT_SPAWN }, 
    { "STILL", SHARE_TURN_STILL }, 
    { "TOCHARONCE", SHARE_TURN_TO_CHAR_ONCE }, 
    { "TOCHARALWAYS", SHARE_TURN_TO_CHAR_ALWAYS }, 
    { "TOPATH", SHARE_TURN_TO_PATH }, 
    { "TOCHARONCEPATH", SHARE_TURN_TO_CHAR_ONCE_PATH }, 
    { "TODIRPATH", SHARE_TURN_TO_DIR_PATH }, 
    { "TURNDIRECTLY", SHARE_TURN_DIRECTLY }, 
    { "WALK", SHARE_ANI_WALK }, 
    { "WALK02", SHARE_ANI_WALK_02 }, 
    { "WALK03", SHARE_ANI_WALK_03 }, 
    { "WALK04", SHARE_ANI_WALK_04 }, 
    { "RUN", SHARE_ANI_RUN }, 
    { "RUN02", SHARE_ANI_RUN_02 }, 
    { "RUN03", SHARE_ANI_RUN_03 }, 
    { "RUN04", SHARE_ANI_RUN_MASTER }, 
    { "RUNBATTLE", SHARE_ANI_RUN_BATTLE },
    { "RUSH", SHARE_ANI_RUSH }, 
    { "RUSH02", SHARE_ANI_RUSH_02 }, 
    { "RUSH03", SHARE_ANI_RUSH_03 }, 
    { "RUSH04", SHARE_ANI_RUSH_MASTER }, 
    { "RUSHFRONT", SHARE_ANI_RUSH_FRONT }, 
    { "RUSHRIGHT", SHARE_ANI_RUSH_RIGHT }, 
    { "<", COND_LESS }, 
    { ">", COND_GREAT }, 
    { "range", COND_RANGE }, 
    { "skill", COND_SKILL }, 
    { "melee", COND_MELEE }, 
    { "=", COND_EQUAL }, 
    { "seek", TARGET_SEEK }, 
    { "second", TARGET_SECOND }, 
    { "front", TARGET_FRONT }, 
    { "state", COND_STATE }, 
    { "STATESTAND", STATE_STAND }, 
    { "STATEMOVETO", STATE_MOVE_TO }, 
    { "STATEDEAD", STATE_DEAD }, 
    { "STATERUSH", STATE_RUSH }, 
    { "STATESKILLING", STATE_SKILLING }, 
    { "STATEHURT", STATE_HURT }, 
    { "STATEMOVEGROUND", STATE_MOVE_GROUND }, 
    { "STATENAVIGATION", STATE_NAVIGATION }, 
    { "STATEHURTBACK", STATE_HURT_BACK }, 
    { "STATEDAZED", STATE_DAZED }, 
    { "STATECAUGHT", STATE_CAUGHT }, 
    { "STATEHURTFLY", STATE_HURT_FLY }, 
    { "STATEMOVEPERSIST", STATE_MOVE_PERSIST }, 
    { "STATEHURTBACKFLY", STATE_HURT_BACK_FLY }, 
    { "section", COND_SECTION }, 
    { "time", COND_TIME }, 
    { "board", COND_BOARD }, 
    { "self", COND_SELF }, 
    { "buff", COND_BUFF }, 
    { "far", COND_FAR},
    { "near", COND_NEAR},
    { "last", TARGET_LAST }, 
    { "posdist", COND_POS_DIST }, 
    { "areaobj", COND_AREA_OBJ }, 
    { "enemy", COND_ENEMY }, 
    { "friend", COND_FRIEND },
    { "targetside", COND_TARGET_SIDE }, 
    { "left", COND_LEFT },
    { "right", COND_RIGHT },
    { "ragevalue", COND_RAGE_VALUE },
};

PropCache::~PropCache()
{
	clear();
}

void PropCache::clear()
{
	PropertyTable::iterator itr = ai_prop_cache_.begin();
	for(;itr != ai_prop_cache_.end(); itr++)
	{
		delete (*itr).second;
	}
	ai_prop_cache_.clear();
}

PropMap* PropCache::get_ai_prop(const std::string& _ai_key)
{
	PropertyTable::iterator itr = ai_prop_cache_.find(_ai_key);
	if(itr != ai_prop_cache_.end())
	{
		return (*itr).second;
	}
	PropMap* ret = new PropMap;
	ai_prop_cache_[_ai_key] = ret;
	return ret;
}

#if defined(AI_VERBOSE_PRINT) || defined(AI_RARE_PRINT)
static const char* NODE_STATUS_TO_STRING[] =
	{"START", "FAILURE", "SUCCESS", "RUNNING"};

#endif

BTNode::BTNode(BTNode* parent)
{
	use_checked_ = false;
	init_action_ = true;
	parent_ = parent;
	id_ = -1;
	tree_ = 0;
	node_status_ = START;
	enabled_ = true;

	move_msg_ = false;
	rate = 0;
	rate_acc = 0;
	rate_min = 0;
	rate_max = 10000;

	cd_ = 0;
	skill_used_count_ = 0;
	last_used_time_ = 0;
	clear_cd_ = 1;

    prop_map_ = NULL;

	init_ai_type_map();
}


BTNode::~BTNode()
{
	ChildNodes::const_iterator i;
	for(i = children_.begin(); i != children_.end(); ++i)
	{
		BTNode* node = (*i);
		delete node;
		node = 0;
	}
}

void BTNode::init_ai_type_map()
{
	if (ai_type_map_.isempty())
	{
        ai_type_map_.init( 96, 96, "BTNode" );

        int count = sizeof(reg_type_)/sizeof(AIRegType);

		for (int i = 0; i < count; ++i)
		{
            ai_type_map_.set( reg_type_[i].key_, reg_type_[i].value_ );
		}
	}
}

void BTNode::execute()
{

}

void BTNode::set_property_once(const char* _key, const char* _value, PropMap* _prop_map)
{
	if(ai_->assert_fail(id_ > -1))
	{
		errorf("no id when set prop");
		return;
	}

    char cat_key[128];
	ff_snprintf(cat_key, 128, "%d+%s", id_, _key);

    (*_prop_map)[cat_key] = _value;
}

const string BTNode::get_property(const char* _key, bool _need_underline/*= false*/)
{
	if(ai_->assert_fail(id_ > -1))
	{
		errorf("no id when get prop");
		return "";
	}

    if( prop_map_ == NULL )
    {
        prop_map_ = g_prop_cache->get_ai_prop( ai_->get_template_name() );
    }

    char cat_key[128];
	ff_snprintf(cat_key, 128, "%d+%s", id_, _key);

	std::string value = "";

	PropMap::iterator itr = prop_map_->find(cat_key);

	if(itr != prop_map_->end())
	{
		value = itr->second;
		str_rep(value);
	}

	if(value.size() && value[0] == '-')
	{
		if(!atoi(value.c_str()))
		{
			str_rep(value, "-");
			value = get_ai_info(value.c_str());
			str_rep(value);
			str_rep(value, "-");
		}
	}
	else
	{
		str_rep(value, "-");
	}

    if (!_need_underline)
    {
        str_rep(value, "_");
    }

	return value;
}

void BTNode::push_back(BTNode* _node)
{
	children_.push_back(_node);
}

void BTNode::log_node_result(int32_t _status)
{
	if(ai_->verbose_print_)
	{
		if ( -1 == _status )
		{
			_status = node_status_;
		}
		TRACE(2)("ai_debug %s -> %s : %s", name_.c_str(), NODE_STATUS_TO_STRING[_status],
			node_name_.c_str());
	}
}

void BTNode::init_node()
{
	get_property_string("Name", name_);
	ChildNodes::const_iterator itChild;
	for(itChild = children_.begin(); itChild != children_.end(); ++itChild)
	{
		BTNode* child = (*itChild);
		if(ai_->assert_fail(child))
		{
			errorf("no child");
			return;
		}
		child->init_node();

		child->init();
	}
}


void BTNode::set_enabled_mode(bool _is_enable)
{
	enabled_ = _is_enable;

	ChildNodes::const_iterator itChild;
	for(itChild = children_.begin(); itChild != children_.end(); ++itChild)
	{
		BTNode* child = (*itChild);
		if(ai_->assert_fail(child))
		{
			errorf("no child");
			return;
		}
		child->set_enabled_mode(_is_enable);
	}
}


void BTNode::handle_disabled_child()
{
	if(parent_)
	{
		parent_->handle_disabled_child();
	}
}

void BTNode::reset_node()
{
	ChildNodes::const_iterator itChild;
	for(itChild = children_.begin(); itChild != children_.end(); ++itChild)
	{
		BTNode* child = (*itChild);
		if(ai_->assert_fail(child))
		{
			errorf("no child");
			return;
		}
		child->reset_node();

		child->reset();
	}
}

BTNode*	BTNode::search_node(int32_t _id)
{
	ChildNodes::const_iterator itChild;
	for(itChild = children_.begin(); itChild != children_.end(); ++itChild)
	{
		BTNode* child = (*itChild);
		if(ai_->assert_fail(child))
		{
			errorf("no child");
			return NULL;
		}
		if(child->get_id() == _id)
		{
			return child;
		}

		BTNode* n = child->search_node(_id);
		if(n) return n;
	}

	return 0;
}

void BTNode::notify_result()
{
	if((node_status_ == FAILURE) || (node_status_ == SUCCESS))
	{
		NodeStatus status = node_status_;
		node_status_ = START;
		parent_->update(status);
	}
	else
	{
		do_execute(true);
	}
}

void BTNode::do_execute( bool _is_exec /*= false*/ )
{
	if(!enabled_)
	{
		if(parent_)
		{
			parent_->handle_disabled_child();
		}
		return;
	}

	tree_->set_current_node(this);

	if(children_.size() != 0)
	{
		log_node_result(START);
	}

	if( _is_exec )
	{
		execute();
	}
}

void BTNode::set_id(int32_t _id)
{
	id_ = _id;
	//char str_id[10];
	tree_->register_node(_id, this);
}

void BTNode::set_behavior_tree(BehaviorTree *bt)
{
	tree_ = bt;
	ai_ = tree_->get_owner();
}


BTRootNode::BTRootNode()
	: BTNode(0)
{
}

BTRootNode::BTRootNode(BTNode* _parent)
	: BTNode(_parent)
{
}

void BTRootNode::update(NodeStatus _result)
{
	tree_->set_current_node(this);

	log_node_result();
}

void BTRootNode::execute()
{
	if(ai_->assert_fail(children_.size() <= 1))
	{
		errorf("root has too many children!");
		return;
	}

	it_child_ = children_.begin();
	(*it_child_)->do_execute();
}

void BTRootNode::init()
{
	BTNode::init();
	ANI_TYPE ani_type = 0;
	if(get_property_ai_type("ani-type", ani_type))
	{
		tree_->set_global_ani_type(ani_type);
	}
}

BTLeafNode::BTLeafNode(BTNode* _parent, AI* _owner)
	: BTNode(_parent)
{
	leaf_type_ = BTLEAF_UNDEF;
	target_type_ = TARGET_FIRST;
	can_reset_ = 1;
}

BTLeafNode::~BTLeafNode()
{
}

void BTLeafNode::execute()
{
	activate_on_start();

	if(RUNNING == node_status_)
	{
		BTLeafNode::step();
		node_status_ = step();
	}

	if(is_over())
	{
		log_node_result();
		parent_->update(node_status_);
		terminate();
	}
}

void BTLeafNode::log_node_result(int32_t _status)
{
	if(ai_->verbose_print_)
	{
		if(-1 == _status)
		{
			_status = node_status_;
		}
        VECTOR3 pos = ai_->get_pos();
		TRACE(2)("ai_debug %s -> %s : %s, pos:%f %f %f\n", name_.c_str(), NODE_STATUS_TO_STRING[_status],
			node_name_.c_str(),
            pos.x, pos.y, pos.z);
	}
}

void BTLeafNode::activate()
{
	if(BTLEAF_ACTION == leaf_type_)
	{
		rule_tracker_.activate(ai_->get_frame_begin_time());
		time_tracker_.activate(ai_->get_frame_begin_time());
		update_cd();
	}
}

void BTNode::update_cd()
{
	// when cd < 0, cd is skill count.
	if(cd_ < 0)
	{
		skill_used_count_++;
	}
	// when cd < 0, cd is time in sec.
	else if ( cd_ > 0)
	{
		last_used_time_ = ai_->get_frame_end_time();
	}
}

bool BTNode::is_cooling_down()
{
	if(cd_ < 0 && -cd_ <= skill_used_count_)
	{
		return true;
	}
	else if ( cd_ > 0 && static_cast<uint32_t>(cd_) > ai_->get_frame_end_time() - last_used_time_)
	{
		return true;
	}

	return false;
}

bool BTLeafNode::can_use()
{
	if (is_cooling_down())
		return false;
	return true;
}

void BTNode::activate_on_start()
{
	if (START == node_status_)
	{
		if(ai_->get_steped() && can_step())
		{
			return;
		}

		if(init_action_ && !use_checked_ && !can_use())
		{
			node_status_ = FAILURE;
		}
		else
		{
			node_status_ = RUNNING;

			if(ai_->rare_print_)
			{
				if (init_action_ && node_name_ != "svo")
				{
					TRACE(2)("ai_debug %s -> %s : %s time %d frame %d\n", name_.c_str(),
						NODE_STATUS_TO_STRING[node_status_], node_name_.c_str(), ai_->get_frame_end_time(), ai_->get_frame_counter());
				}
			}

			move_msg_ = false;
			activate();
			if(move_msg_ && node_status_ != RUNNING)
			{
				//  errorf("send msg but no action step");
			}
			else if (!move_msg_ && node_status_ == RUNNING)
			{
				//  errorf("no send msg but action step");
			}
		}
		log_node_result();
	}
}

std::string BTNode::get_ai_info(const char* _key) const
{
	return ai_->get_ai_info(_key);
}

bool BTNode::get_property_float(const char* _key, float& _value)
{
	std::string val;
	if(get_property_string(_key, val))
	{
		_value = atof(val.c_str());
		return true;
	}
	else
	{
		return false;
	}
}

bool BTNode::get_property_int(const char* _key, int32_t& _value)
{
	std::string val;
	if(get_property_string(_key, val))
	{
		_value = static_cast<int32_t>(atof(val.c_str()));
		return true;
	}
	else
	{
		return false;
	}
}

bool BTNode::get_property_string(const char* _key, string& _value, bool _need_underline/*=false*/)
{
	std::string val = get_property(_key, _need_underline);
	if(val.size())
	{
		_value = val;
		return true;
	}
	else
	{
		return false;
	}
}

bool BTNode::is_leaf_node() const
{
	return false;
}

bool BTNode::get_property_float_list(const char* _key, std::vector<float>& _values)
{
	std::vector<std::string> string_list;
	_values.clear();
	if(get_property_string_list(_key, string_list))
	{
		for(uint32_t i = 0; i < string_list.size(); ++i)
		{
			_values.push_back(atof(string_list[i].c_str()));
		}
		return true;
	}
	else
	{
		return false;
	}
}

void BTNode::insert_node(BTNode* _node, int32_t _index)
{
	children_.insert(children_.begin() + _index, _node);
}

AI_TYPE BTNode::get_ai_type(const std::string& _key)
{
    intptr_t type;

	if( ai_type_map_.find(_key.c_str(), (intptr_t&)type) )
    {
        return (AI_TYPE)type;
    }
    else
    {
        return ACTION_THINK;
    }
}

void BTNode::errorf(const char* _desc)
{
	ERR(2)("[BTNode](errorf)%s, ai_id=%d, template=%s, node_name=%s, node_id=%d", _desc,
		ai_->get_ai_id(), ai_->get_template_name().c_str(), get_property("Name").c_str(), id_);
}

void BTNode::broadcast_msg(void* _msg, bool _move_msg, ACT_TYPE _act_type, TURN_TYPE _turn_type)
{
	if(_move_msg)
	{
		move_msg_ = _move_msg;
	}
	//ai_->can_broadcastMsg(msg, act_type, turn_type);
}

bool BTNode::get_property_string_list(const char* _key, std::vector<std::string>& _values)
{
	std::string val;
	_values.clear();
	if(get_property_string(_key, val))
	{
		if(val.find("(") != string::npos || val.find("[") != string::npos || val.find("{") != string::npos)
		{
			str_split(str_trim_bracket(&val), _values);
		}
		else
		{
			_values.push_back(val);
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool BTNode::get_property_ai_type(const char* _key, unsigned char& _ai_type)
{
	std::string value;
	if(get_property_string(_key, value))
	{
		_ai_type = get_ai_type(value);
		return true;
	}
	return false;
}

bool BTNode::get_property_ai_type_list(const char* _key, std::vector<unsigned char>& _ai_types)
{
	std::vector<std::string> string_list;
	_ai_types.clear();
	if(get_property_string_list(_key, string_list))
	{
		for(uint32_t i = 0; i < string_list.size(); ++i)
		{
			_ai_types.push_back(get_ai_type(string_list[i]));
		}
		return true;
	}
	else
	{
		return false;
	}
}

int32_t BTNode::get_rate() const
{
	int rate_ = rate + rate_acc * ai_->get_rate_acc();
	rate_ = max(rate_, rate_min);
	rate_ = min(rate_, rate_max);
	return rate_;
}

void BTNode::enter_child(int32_t _index)
{
	it_child_ = children_.begin() + _index;
	tree_->set_current_node((*it_child_));
}

std::string BTNode::dump_info()
{
	std::ostringstream oss;
	oss << "act:" << (node_name_.size() ? node_name_ : "root") << "," <<\
		(get_property("Name").size() ? get_property("Name") : "root") << "," <<id_;
	return oss.str();
}

void BTNode::clear_cd( bool _force )
{
	if(clear_cd_ != 0)
	{
		skill_used_count_ = 0;
		last_used_time_ = 0;
	}
}

bool BTNode::insert_skill(int _skill_id, float _cd_time, int _rate)
{
	char buff[32] = "skill";
	BTLeafNode *new_node = BTLeafFactory::get_instance()->get_creator(buff)(this, ai_);
	new_node->set_behavior_tree(tree_);
	new_node->set_leaf_type(BTLEAF_ACTION);

	int32_t Nodeid = 1000 + children_.size();
	sprintf(buff, "%d", Nodeid);
	new_node->set_id(Nodeid);
	sprintf(buff, "skill%d", _skill_id);

    PropMap* prop_map = g_prop_cache->get_ai_prop( ai_->get_template_name() );

	new_node->set_property_once("Name", buff, prop_map);
	new_node->set_property_once("Operation", "skill", prop_map);

	sprintf(buff, "%d", _skill_id);
	new_node->set_property_once("skill-id", buff, prop_map);
	sprintf(buff, "%f", _cd_time);
	new_node->set_property_once("cd", buff, prop_map);
	sprintf(buff, "%d", _rate);
	new_node->set_property_once("rate", buff, prop_map);

	new_node->init();
	push_back(new_node);
	return true;
}

float BTNode::get_property_rand_float( const char* _key, const float _default_val, float _step/*=0*/ )
{
	float ret = _default_val;
	std::vector<float> vals;
	get_property_float_list(_key, vals);
	if(vals.size() == 1)
	{
		ret = vals[0];
	}
	else if (vals.size() == 2)
	{
		if(_step > 0 )
		{
			ret = rand_int(vals[0]/_step, vals[1]/_step)*_step;
		}
		else
		{
			ret = rand_float(vals[0], vals[1]);
		}
	}
	return ret;
}

bool BTNode::get_property_int_list( const char* _key , std::vector<int>& _values)
{
	std::vector<std::string> string_list;
	_values.clear();
	if(get_property_string_list(_key, string_list))
	{
		for(uint32_t i = 0; i < string_list.size(); ++i)
		{
			_values.push_back(atoi(string_list[i].c_str()));
		}
		return true;
	}
	else
	{
		return false;
	}

}

void BTNode::init()
{
	int value;
	if(get_property_int("mask", value) && 1 == value)
	{
		set_enabled_mode(false);
	}
}

void BTNode::reset()
{
	terminate();
}

bool BTNode::can_reset() const
{
	return true;
}

vector<int>& BTNode::get_chunk_idx()
{
	return l;
}

void BTNode::update( NodeStatus _result )
{

}

bool BTNode::can_use()
{
	return enabled_;
}

void BTNode::on_message( MSG_TYPE _msg )
{

}

bool BTNode::can_step() const
{
	return true;
}

int BTNode::sync_path_to() const
{
	return 0;
}

void BTNode::reactivate_on_success()
{
	if (SUCCESS == node_status_)
	{
		init_action_ = false;
		node_status_ = START;
	}
}

void BTNode::activate()
{

}

void BTNode::init_cd()
{
	float cd  = get_property_rand_float("cd", 0);
	get_property_int("clear-cd", clear_cd_);

	if(cd > 0)
	{
		cd_ = cd * THOUSAND;
	}
	else
	{
		cd_ = cd;
	}
}

void BTNode::terminate()
{
	init_action_ = true;
	node_status_ = START;
}

void BTLeafNode::init()
{
	BTNode::init();
	float time = get_property_rand_float("time", get_default_time());
	time = check_endure_time(time);
	time_tracker_.init(Tracker::TYPE_PERIOD, time);
	rule_tracker_.init(Tracker::TYPE_NOW);

	process_arg();
	process_endure();

	get_property_ai_type("target", (unsigned char&)target_type_);

	get_property_int("reset", can_reset_);
	init_cd();
}

bool BTLeafNode::is_leaf_node() const
{
	return true;
}

BTLeafType BTLeafNode::get_leaf_type() const
{
	return leaf_type_;
}

void BTLeafNode::process_endure()
{
	std::vector<float> node_ids;
	std::vector<string> node_strs;

	//LOG(2)("ProcessEndure_");
	if (get_property_string_list("suc-endure", node_strs))
	{
		for (uint32_t i =0; i< node_strs.size(); i++)
		{
			//LOG(2)("node str: %s", node_strs[i].c_str());
			for (uint32_t j = 0;j < tree_->get_condition().size(); j++)
			{
				std::string name;
				tree_->get_condition()[j]->get_property_string("Name", name);
				//LOG(2)("cond name: %s, %s", name.c_str(), node_strs[i].c_str());
				if(name == node_strs[i])
				{
					//LOG(2)("push node id: %d", j);
					node_ids.push_back(j);
					break;
				}
			}
		}
	}

	if (node_ids.size() > 0 || get_property_float_list("suc-endure", node_ids))
	{
		for(uint32_t i = 0; i< node_ids.size(); ++i)
		{
			int32_t node_id = node_ids[i];
			if(ai_->assert_fail(node_id < static_cast<int32_t>(tree_->get_condition().size())))
			{
				errorf("error suc condition index");
				return;
			}
			if(node_id < (int)tree_->get_condition().size())//vv
			{
				endure_cond_node_.push_back(tree_->get_condition()[node_id]);
				endure_cond_flag_.push_back(true);
			}
		}
	}

	if (get_property_string_list("fail-endure", node_strs))
	{
		for (uint32_t i =0; i< node_strs.size(); i++)
		{
			//LOG(2)("node str: %s", node_strs[i].c_str());
			for (uint32_t j = 0;j < tree_->get_condition().size(); j++)
			{
				std::string name;
				tree_->get_condition()[j]->get_property_string("Name", name);
				//LOG(2)("cond name: %s, %s", name.c_str(), node_strs[i].c_str());
				if(name == node_strs[i])
				{
					//LOG(2)("push node id: %d", j);
					node_ids.push_back(j);
					break;
				}
			}
		}
	}

	if (node_ids.size() > 0 && get_property_float_list("fail-endure", node_ids))
	{
		for(uint32_t i = 0; i< node_ids.size(); ++i)
		{
			int32_t node_id = node_ids[i];
			if(ai_->assert_fail(node_id < static_cast<int32_t>(tree_->get_condition().size())))
			{
				errorf("error fail condition index");
				return;
			}
			if(node_id < (int)tree_->get_condition().size())//vv
			{
				endure_cond_node_.push_back(tree_->get_condition()[node_id]);
				endure_cond_flag_.push_back(false);
			}
		}
	}
}

bool BTLeafNode::check_endure_cond() const
{
	for (uint32_t i = 0; i < endure_cond_node_.size(); ++i)
	{
		if(static_cast<ConditionSVO *>(endure_cond_node_[i])->can_use() == endure_cond_flag_[i])
		{
			return true;
		}
	}
	return false;
}

NodeStatus BTLeafNode::step()
{
	if(ai_->get_steped())
	{
		//std::string desc("redundancy action step");
		//  tree_->get_last_step_node()->errorf(desc.c_str());
		//  errorf(desc.c_str());
	}
	else
	{
		ai_->set_steped(true);
	}
	return START;
}

float BTLeafNode::check_endure_time(float _time)
{
// 	if(ai_->assert_fail(time > 0))
// 	{
// 		errorf("no default time");
// 	}
	return _time;
}

void BTLeafNode::set_id(int32_t _id)
{
	BTNode::set_id(_id);
	if(BTLEAF_ACTION == leaf_type_)
	{
		tree_->register_action(this);
	}
}

bool BTLeafNode::target_last() const
{
	return target_type_ == TARGET_LAST;
}

float BTLeafNode::get_default_time() const
{
	return -1.f;
}

void BTLeafNode::process_arg()
{

}

bool BTLeafNode::target_master() const
{
	return target_type_ == COND_MASTER;
}

void BTLeafNode::action_msg_default( AI_MSG _msg )
{
	ai_->action_msg(get_name(), _msg, get_id());
}

}
