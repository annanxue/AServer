#ifndef __BEHAVIORTREE_H
#define __BEHAVIORTREE_H

#include <map>
#include <string>
#include <vector>
#include "common.h"
#include "btnode.h"

namespace SGame
{

class BTRootNode;
class BTNode;
class Blackboard;
class BTLeafNode;
class AI;

class BTLeafFactory
{
public:
	typedef BTLeafNode* (*LeafCreator) (BTNode* _parent, AI* _owner);

	void unregister_all_creator();
	void register_creator(const char* _name, LeafCreator _creator);
	void unregister_creator(const char* _name);
	bool is_creator_registered(const char* _name);
	LeafCreator get_creator(const char* _name);
	static void destroy();
	static BTLeafFactory* get_instance();
	
private:
	BTLeafFactory();
	~BTLeafFactory();
	void register_leaf_nodes();

public:
	map<string, LeafCreator > entries_;
};

inline BTLeafFactory::LeafCreator BTLeafFactory::get_creator(const char* _name)
{
	if (entries_.find(_name) != entries_.end())
	{
		return entries_[_name];
	}
	return entries_["default"];
}

inline bool BTLeafFactory::is_creator_registered(const char* _name)
{
	if(entries_[_name] != 0) return true;
	return false;
}

inline void BTLeafFactory::unregister_all_creator()
{
	entries_.clear();
}

inline void BTLeafFactory::register_creator(const char* _name, LeafCreator _creator)
{
	entries_[_name] = _creator;
}

inline void BTLeafFactory::unregister_creator(const char* _name)
{
	entries_[_name] = 0;
}


enum BTInfo
{
	BT_UNDEF,
	BT_RUNTIME,
	BT_DEBUG_BOT
};

enum BTState
{
	BT_ST_IDLE,
	BT_ST_EXECUTING,
};

enum BTPlayState
{
	BT_PST_PLAYING,
	BT_PST_PAUSED,
	BT_PST_STOPPED,
	BT_PST_STEPPED,
};

enum BTStopProtocol
{
	BT_STOP_PROT_NON,
	BT_STOP_PROT_LEAFNODES,
	BT_STOP_PROT_ACTIONS,
	BT_STOP_PROT_CONDITIONS,
};

class BehaviorTree
{
public:
	BehaviorTree();
	~BehaviorTree();

	bool set_patrol_path(Navmesh::Path& _path, bool _loop, bool _not_find_path, int32_t _ani_type, const vector<PATROL_INFO> *_patrol_info, bool _broadcast);
	void set_id(uint32_t _id);
	void set_initialized(bool _init);
	void set_current_node(BTNode* _cur_node);
	void set_stop_protocol(BTStopProtocol _stop_protocol);
	void set_spawn_anim(const char* _anim, float _time);
	void set_msg_node(BTNode* _node);
	void set_global_ani_type(ANI_TYPE _ani_type);
	void reset(bool _force);

	uint32_t get_id();
	BTInfo get_bt_class_info();
	AI* get_owner();
	BTStopProtocol get_stop_protocol();
	BTNode* get_current_node();
	const vector<BTNode*>& get_condition() const;
	BTNode* get_root() const;
	ANI_TYPE get_global_ani_type() const;

	void register_node(int32_t _id, BTNode* _node);
	void register_condition(BTNode* _node);
	void register_action(BTNode* _node);

	void add_spawn_patrol_node();
	void add_spawn_node(const char* _anim, float _time, bool _reborn, bool _spawn_buff);
	
	void pause();
	void stop();
	void play();
	void step();
	NodeStatus execute();
	void clear_cd(bool _force = false);
	std::string dump_info();
	void dump_main_selector();
	void init_tree();
	void on_message(MSG_TYPE _msg);
	int sync_path_to() const;
	bool can_break() const;
	vector<int>& get_chunk_idx();
	void enable_branch(int32_t _node_id, bool _enabled);
	bool load(const std::string& _file, AI* _hnd);
	bool load(const char* _file, AI* _hnd);

	BTNode* get_node(int32_t id) { return node_map_[id];}

private:
	bool is_executable();

private:
	BTRootNode* root_;
	AI* owner_;
	BTInfo info_;
	BTState	bt_state_;
	BTPlayState play_state_;
	BTStopProtocol stop_protocol_;
	BTNode* current_node_;
	uint32_t id_;
	bool started_;
	bool initialized_;
	const BTNode* last_action_parent_;
	BTNode *msg_node_;
	ANI_TYPE ani_type_;
	bool has_spawn_anim_;

	vector<BTNode*> conditions_;
	vector<BTNode*> actions_;
	map<int32_t, BTNode*> node_map_;
};

inline void BehaviorTree::set_global_ani_type(ANI_TYPE _ani_type)
{
	ani_type_ = _ani_type;
}

inline ANI_TYPE BehaviorTree::get_global_ani_type() const
{
	return ani_type_;
}

inline void BehaviorTree::set_msg_node(BTNode* _node)
{
	msg_node_ = _node;
}

inline BTNode* BehaviorTree::get_root() const
{
	return root_;
}

inline const vector<BTNode*>& BehaviorTree::get_condition() const
{
	return conditions_;
}

inline void BehaviorTree::register_action(BTNode* _node)
{
	actions_.push_back(_node);
}

inline void BehaviorTree::register_condition(BTNode* _node)
{
	conditions_.push_back(_node);
}

inline void BehaviorTree::register_node( int32_t _id, BTNode* _node )
{
	node_map_[_id] = _node;
}

inline void BehaviorTree::set_stop_protocol(BTStopProtocol _stop_protocol)
{
	stop_protocol_ = _stop_protocol;
}

inline BTNode* BehaviorTree::get_current_node()
{
	return current_node_;
}

inline void BehaviorTree::set_current_node(BTNode* _cur_node)
{
	current_node_ = _cur_node;
}

inline void BehaviorTree::set_initialized(bool _init)
{
	initialized_ = _init;
}

inline void BehaviorTree::set_id(uint32_t _id)
{
	id_ = _id;
}

inline BTStopProtocol BehaviorTree::get_stop_protocol()
{
	return stop_protocol_;
}

inline uint32_t BehaviorTree::get_id()
{
	return id_;
}

inline BTInfo BehaviorTree::get_bt_class_info()
{
	return info_;
}

inline AI* BehaviorTree::get_owner()
{
	return owner_;
}

typedef map<string, int32_t> IntBlackboard;
typedef map<string, string>	StringBlackboard;
typedef map<string, float> FloatBlackboard;

class Blackboard
{
public:
	Blackboard();
	~Blackboard();
	void clean();
	void set_value_int(string _key, int32_t _value);
	void set_value_str(string _key, string _value);
	void set_value_float(string _key, float _value);
	int32_t get_value_int(string _key);
	std::string get_value_str(string _key);
	float get_value_float(string _key);
	
protected:
	IntBlackboard int_blackboard_;
	StringBlackboard str_blackboard_;
	FloatBlackboard float_blackboard_;
};


typedef map<uint32_t, Blackboard*> BlackboardCollection;

class BlackboardManager
{
public:
	static BlackboardManager* get_instance();
	static void destroy();
	Blackboard* get_blackboard(uint32_t _blackboard_id);

private:
	BlackboardManager();
	~BlackboardManager();

private:
	BlackboardCollection blackboards_;
};

}

#endif
