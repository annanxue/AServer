#include <cstdio>
#include <ctime>
#include <string>
#include "btree.h"
#include "load_btree.h"
#include "action.h"
#include "condition.h"
#include "common.h"
#include "action_ex.h"

namespace SGame
{

template<typename T>

BTLeafNode* create_leaf_node(BTNode* _parent, AI* _id)
{
	return new T(_parent, _id);
}


BTLeafFactory::BTLeafFactory()
{
	register_leaf_nodes();
}

BTLeafFactory::~BTLeafFactory()
{

}

BTLeafFactory* BTLeafFactory::get_instance()
{
	static BTLeafFactory instance;
	return &instance;
}

void BTLeafFactory::register_leaf_nodes()
{
	register_creator("skill", &create_leaf_node<ActionSkill>);
	register_creator("chargeskill", &create_leaf_node<ActionChargeSkill>);
	register_creator("chaseskill", &create_leaf_node<ActionChaseSkill>);
	register_creator("chase", &create_leaf_node<ActionChase>);
	register_creator("hold", &create_leaf_node<ActionHold>);
	register_creator("varyingchase", &create_leaf_node<ActionVaryingChase>);
	register_creator("skillchase", &create_leaf_node<ActionSkillChase>);
	register_creator("spawn", &create_leaf_node<ActionSpawn>);
	register_creator("shout", &create_leaf_node<ActionShout>);

	register_creator("explore", &create_leaf_node<ActionExplore>);
	register_creator("evasion", &create_leaf_node<ActionEvasion>);
	register_creator("swing", &create_leaf_node<ActionSwing>);
    register_creator("followmaster", &create_leaf_node<ActionFollowMaster>);

	register_creator("idle", &create_leaf_node<ActionAct>);
	register_creator("act", &create_leaf_node<ActionAct>);
	register_creator("shuffle", &create_leaf_node<ActionShuffle>);
	register_creator("sfx", &create_leaf_node<ActionSfx>);

	register_creator("success", &create_leaf_node<ConditionSuccessful>);
	register_creator("fail", &create_leaf_node<ConditionFailure>);

	register_creator("svo", &create_leaf_node<ConditionSVO>);

	register_creator("emote", &create_leaf_node<ActionEmote>);
	register_creator("trigger", &create_leaf_node<ActionActivateTrig>);
	register_creator("moveto", &create_leaf_node<ActionMoveTo>);
	register_creator("setsection", &create_leaf_node<ActionSetSection>);
	register_creator("setboard", &create_leaf_node<ActionSetBoard>);
	register_creator("default", &create_leaf_node<ActionDefault>);

	register_creator("followmonster", &create_leaf_node<ActionFollowMonster>);
	register_creator("drop", &create_leaf_node<ActionDrop>);
	register_creator("updatefightattr", &create_leaf_node<ActionUpdateFightAttr>);
	register_creator("returnhome", &create_leaf_node<ActionReturnHome>);
}

BehaviorTree::BehaviorTree()
{
	root_ = new BTRootNode();
	current_node_ = root_;
	info_ = BT_UNDEF;
	owner_ = 0;
	bt_state_ = BT_ST_IDLE;
	id_ = 0;
	started_ = false;
	initialized_ = 0;
	play_state_ = BT_PST_PAUSED;
	stop_protocol_ = BT_STOP_PROT_NON;
	last_action_parent_ = NULL;
	ani_type_ = SHARE_ANI_NONE;
	msg_node_ = NULL;
	has_spawn_anim_ = false;
}

BehaviorTree::~BehaviorTree()
{
	id_ = 0;
	current_node_ = 0;

	delete root_;
	root_ = 0;
}

bool BehaviorTree::load(const char* _file, AI* _ai)
{
	if(!_ai || _ai->assert_fail(_file))
	{
		return false;
	}

	owner_ = _ai;
	if(root_ == 0) 
    {
        root_ = new BTRootNode();
    }
	if(!load_bt_tree(_file, this, root_, _ai))
	{
		return false;
	}

	if(owner_->verbose_print_)
	{
		TRACE(2)("ai_debug ***** Behavior tree loaded: %s ***** \n", _file);
	}

	started_ = false;
	play_state_ = BT_PST_PAUSED;
	current_node_ = root_;

	bt_state_ = BT_ST_IDLE;

	return true;
}

bool BehaviorTree::load(const std::string& _file, AI* _ai)
{
	if(!_ai)
	{
		return false;
	}
	bool load_ok = true;
	owner_ = _ai;

	std::string file_name(AI_ROOT);
	char temp[MAX_FILE_NAME];
	ff_snprintf(temp, MAX_FILE_NAME, "%s.xml", _file.c_str());
	file_name += temp;

	if(!load_bt_tree(file_name, this, root_, _ai))
	{
		return false;
	}

	if(root_)
	{
		info_ = BT_RUNTIME;
	}
	else
	{
		info_ = BT_UNDEF;
	}

	bt_state_ = BT_ST_IDLE;
	started_ = false;
	play_state_ = BT_PST_PLAYING;
	current_node_ = root_;

	if(info_ == BT_UNDEF)
	{
		load_ok = false;
	}
	else if (info_ == BT_DEBUG_BOT)
	{
		play_state_ = BT_PST_PAUSED;
	}

	if(owner_->verbose_print_)
	{
		TRACE(2)("ai_debug ***** Behavior tree loaded result: %d ***** \n", load_ok);
	}
	return load_ok;
}

void BehaviorTree::init_tree()
{
	if( root_ )
	{
		root_->init();
		root_->init_node();
		initialized_ = true;
	}
}

bool BehaviorTree::is_executable()
{
    if( !root_ )
    {
        return false;
    }

    if( !current_node_ )
    {
        return false;
    }

    if( root_->get_num_children() <=0 )
    {
        return false;
    }

	bool is_ready = (bt_state_ == BT_ST_IDLE);
	bool is_playable = (play_state_ == BT_PST_PLAYING || play_state_ == BT_PST_STEPPED);

	return is_ready && is_playable;
}

NodeStatus BehaviorTree::execute()
{
	NodeStatus result = FAILURE;

	if(!initialized_)
	{
		init_tree();
	}

	if(is_executable())
	{
		if( !started_ )
		{
			started_ = true;
		}

		bt_state_ = BT_ST_EXECUTING;

		BTNode *cur_node = NULL;
		do 
		{
			cur_node = current_node_;
			cur_node->do_execute(true);
		} 
        while (cur_node != current_node_ && current_node_ != root_);

		result = current_node_->get_node_status();

		bt_state_ = BT_ST_IDLE;

		if(play_state_ == BT_PST_STEPPED)
        {
			play_state_ = BT_PST_PAUSED;
        }
	}

	return result;
}

void BehaviorTree::enable_branch(int32_t _node_id, bool _enabled)
{
	BTNode *node = root_->search_node(_node_id);

	if(node)
	{
		node->set_enabled_mode(_enabled);
	}
}


void BehaviorTree::play()
{
	play_state_ = BT_PST_PLAYING;
}

void BehaviorTree::pause()
{
	play_state_ = BT_PST_PAUSED;
}

void BehaviorTree::stop()
{
	play_state_ = BT_PST_STOPPED;
	current_node_ = root_;
	root_->reset_node();
}

void BehaviorTree::step()
{
	play_state_ = BT_PST_STEPPED;
}

void BehaviorTree::on_message(MSG_TYPE _msg)
{
	current_node_->on_message(_msg);
}

void BehaviorTree::reset( bool _force )
{
    if (owner_->is_returning_home())
    {
        return;
    }

	if(!current_node_) 
    {
        return;
    }

	if(!_force)
    {
        const BTNode* node = current_node_;
        while (node != NULL && node != root_) 
        {
            if (!node->can_reset()) return;
            node = node->get_parent(); 
        }
    }
	current_node_->reset();
	current_node_ = root_;
}

void BehaviorTree::add_spawn_node(const char* _anim, float _time, bool _reborn, bool _spawn_buff)
{
	BTNode *main_seletor =  root_->get_child(0);

    const char* node_name = "spawn";

	BTLeafNode *new_node = BTLeafFactory::get_instance()->get_creator(node_name)(
		main_seletor, owner_);
	new_node->set_behavior_tree(this);
	new_node->set_name(node_name);
	new_node->set_leaf_type(BTLEAF_ACTION);

	new_node->set_id(SPAWN_ID);

    PropMap* prop_map = g_prop_cache->get_ai_prop( owner_->get_template_name() );

	new_node->set_property_once("Name", node_name, prop_map);
	new_node->set_property_once("Operation", node_name, prop_map);

	const int MAX_FLOAT = 8;
	char str_time[MAX_FLOAT];
	ff_snprintf(str_time, MAX_FLOAT, "%.1f", _time);
	new_node->set_property_once("time", str_time, prop_map);

	new_node->set_property_once("cd", "-1", prop_map);
	new_node->set_property_once("Type", "Action", prop_map);
	new_node->set_property_once("ani", _anim, prop_map);
	new_node->set_property_once("Node_id", "10003", prop_map);

	main_seletor->insert_node(new_node, 0);
	new_node->init();
	static_cast<ActionSpawn*>(new_node)->set_flag(_reborn, _spawn_buff);
	has_spawn_anim_ = true;
}

void BehaviorTree::add_spawn_patrol_node()
{
	BTSeqNode *main_seletor =  static_cast<BTSeqNode*>(root_->get_child(0));

    const char* node_name = "explore";

	BTLeafNode *new_node = BTLeafFactory::get_instance()->get_creator(node_name)(
		main_seletor, owner_);
	new_node->set_behavior_tree(this);
	new_node->set_name("patrol_explore");
	new_node->set_leaf_type(BTLEAF_ACTION);
	new_node->set_id(SPAWN_PATROL_ID);

    PropMap* prop_map = g_prop_cache->get_ai_prop( owner_->get_template_name() );

	new_node->set_property_once("Name", node_name, prop_map);
	new_node->set_property_once("Operation", node_name, prop_map);

	new_node->set_property_once("time", "2", prop_map);
	new_node->set_property_once("stand-time", "2", prop_map);
	new_node->set_property_once("rad", "10", prop_map);

	new_node->set_property_once("move-time", "2", prop_map);
	new_node->set_property_once("move-rate", "1", prop_map);
	new_node->set_property_once("dir", "(0,0)", prop_map);

	new_node->set_property_once("Type", "Action", prop_map);
	new_node->set_property_once("Node_id", "10004", prop_map);

	main_seletor->insert_node(new_node, main_seletor->get_num_children() -1);
	new_node->init();
}

bool BehaviorTree::set_patrol_path( Navmesh::Path& _path, bool _loop, bool _not_find_path, int32_t _ani_type, const vector<PATROL_INFO> *_patrol_info, bool _broadcast )
{
	BTSeqNode *main_seletor =  static_cast<BTSeqNode*>(root_->get_child(0));
	BTNode *patrol_node = main_seletor->get_child(main_seletor->get_num_children() -2);
	return static_cast<ActionExplore*>(patrol_node)->set_path(_path, _loop, _not_find_path, _ani_type, _patrol_info, _broadcast );
}

int BehaviorTree::sync_path_to() const
{
	if (current_node_)
	{
		return current_node_->sync_path_to();
	}
	return 0;
}

vector<int>& BehaviorTree::get_chunk_idx()
{
	return current_node_->get_chunk_idx();
}

bool BehaviorTree::can_break() const
{
	if (owner_->no_break())
	{
		if(current_node_ && (current_node_->get_name() == "skill" || current_node_->get_name() == "posskill"))
		{
			return false;
		}
	}
	return true;
}

std::string BehaviorTree::dump_info()
{
	if(current_node_)
	{
		return current_node_->dump_info();
	}
	else
	{
		return "noAct";
	}
}

void BehaviorTree::clear_cd( bool _force/*=false*/ )
{
	for(int i = 0; i < static_cast<int32_t>(actions_.size()); ++i)
	{
		actions_[i]->clear_cd(_force);
	}

	for(int i = 0; i < static_cast<int32_t>(conditions_.size()); ++i)
	{
		conditions_[i]->clear_cd(_force);
	}
}

void BehaviorTree::set_spawn_anim( const char* _anim, float _time )
{
	if(has_spawn_anim_)
	{
		static_cast<ActionSpawn*>(root_->get_child(0)->get_child(0))->set_anim(_anim, _time);
	}
}

void BehaviorTree::dump_main_selector()
{
	printf("DumpMainSelector\n");
	for(int32_t i = 0; i < root_->get_child(0)->get_num_children(); ++i)
	{
		printf("%s\n", root_->get_child(0)->get_child(i)->get_name().c_str());
	}
}



Blackboard::Blackboard()
{
}

Blackboard::~Blackboard()
{
	clean();
}

void Blackboard::clean()
{
	int_blackboard_.clear();
	str_blackboard_.clear();
	float_blackboard_.clear();
}

void Blackboard::set_value_int(string _key,	int32_t _value)
{
	int_blackboard_[_key] = _value;
}

int32_t Blackboard::get_value_int(string _key)
{
	IntBlackboard::iterator itBlack = int_blackboard_.find(_key);
	if(itBlack == int_blackboard_.end())
	{
		return 0;
	}
	return itBlack->second;
}

void Blackboard::set_value_str(string _key, string _value)
{
	str_blackboard_[_key] = _value;
}

string Blackboard::get_value_str(string _key)
{
	StringBlackboard::iterator itBlack = str_blackboard_.find(_key);
	if(itBlack == str_blackboard_.end())
	{
		return "";
	}
	return itBlack->second;
}

void Blackboard::set_value_float(string _key, float _value)
{
	float_blackboard_[_key] = _value;
}

float Blackboard::get_value_float(string _key)
{
	FloatBlackboard::iterator itBlack = float_blackboard_.find(_key);
	if(itBlack == float_blackboard_.end())
	{
		return 0.f;
	}
	return itBlack->second;
}

BlackboardManager::BlackboardManager()
{
}

BlackboardManager::~BlackboardManager()
{
	BlackboardCollection::iterator itBlack;
	for(itBlack = blackboards_.begin(); itBlack != blackboards_.end(); ++itBlack)
	{
		Blackboard* bboard = itBlack->second;
		delete bboard;
		bboard  = 0;
	}
	blackboards_.clear();
}

BlackboardManager* BlackboardManager::get_instance()
{
	static BlackboardManager instance;
	return &instance;
}

Blackboard* BlackboardManager::get_blackboard(uint32_t _blackboard_id)
{
	Blackboard* bboard = 0;
	BlackboardCollection::iterator itBlack = blackboards_.find(_blackboard_id);
	if(itBlack == blackboards_.end())
	{
		bboard = new Blackboard();
		blackboards_[_blackboard_id] = bboard;
	}
	else
	{
		bboard = itBlack->second;
	}

	return bboard;
}

}

