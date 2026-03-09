#ifndef __BTNODE_H__
#define __BTNODE_H__

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include "common.h"
#include "utils.h"
#include "mymap32.h"

namespace SGame
{

class AI;
class BTNode;
class BehaviorTree;

enum NodeStatus
{
	START = 0,
	FAILURE = 1,
	SUCCESS = 2,
	RUNNING = 3
};

typedef vector<BTNode*> ChildNodes;

struct AIRegType
{
    const char* key_;
    int value_;   
};

typedef std::map<std::string, std::string> PropMap;

class BTNode
{
public:
	explicit BTNode(BTNode* parent);
	virtual ~BTNode();

	void set_name(const char* _node_name);
	void set_use_checked(bool _flag);
	void set_enabled_mode(bool _is_enable);
	float get_property_rand_float(const char* _key, const float _default_val, float _step = 0);
	void set_property_once(const char* _key, const char* _value, PropMap* _prop_map);
	void set_behavior_tree(BehaviorTree* bt);
	void reset_node();

	int32_t get_rate() const;
	std::string get_ai_info(const char* _key) const;
	const string get_property(const char* _key, bool _need_underline = false);
	bool get_property_float(const char* _key, float& _value);
	bool get_property_int(const char* _key, int32_t& _value);
	bool get_property_int_list(const char* _key , std::vector<int>& _values);
	bool get_property_float_list(const char* _key, std::vector<float>& _values);
	bool get_property_string_list(const char* _key, std::vector<std::string>& _values);
	bool get_property_string(const char* _key, string& _value, bool _need_underline = false);
	const BTNode *get_parent() const;
	int32_t get_num_children();
	BTNode* get_child(int32_t _index);
	BehaviorTree* get_behavior_tree();
	int32_t get_id();
	const string& get_name() const;
	NodeStatus get_node_status();
    const string& get_raw_name() const;
	
	void enter_child(int32_t _index);
	void init_node();
	std::string dump_info();
	void update_cd();
	bool is_cooling_down();
	BTNode*	search_node(int32_t _id);
	void do_execute( bool _is_exec = false );
	void notify_result();
	void errorf(const char* _desc);
	void broadcast_msg(void* _msg, bool _move_msg = true, ACT_TYPE _act_type = SHARE_ACT_NONE, \
		TURN_TYPE _turn_type = SHARE_TURN_NONE);

	virtual void init();
	virtual void execute();
	virtual bool is_leaf_node() const;
	virtual void log_node_result(int32_t _status = -1);
	virtual void clear_cd(bool _force);
	virtual void handle_disabled_child();
	virtual void reset();
	virtual bool can_reset() const;
	virtual void push_back(BTNode* _node);
	virtual void insert_node(BTNode* _node, int32_t _index);
	virtual void update(NodeStatus _result);
	virtual void set_id(int32_t _id);
	virtual bool can_use();
	virtual void on_message(MSG_TYPE _msg);
	virtual bool can_step() const;
	virtual int sync_path_to() const;
	virtual vector<int>& get_chunk_idx();

protected:
	void init_cd();
	void init_ai_type_map();
	void activate_on_start();
	bool insert_skill(int _skill_id, float _cd_time, int _rate);
	void reactivate_on_failure();
	bool is_over();
	void reactivate_on_over();
	AI_TYPE get_ai_type(const std::string& _key);
	bool get_property_ai_type(const char* _key, unsigned char& _ai_type);
	bool get_property_ai_type_list(const char* _key, std::vector<unsigned char>& _ai_types);
	
	virtual void reactivate_on_success();
	virtual void activate();
	virtual void terminate();

public:
	int32_t rate;
	int32_t rate_acc;
	int32_t rate_min;
	int32_t rate_max;
	int32_t cd_;
	int32_t skill_used_count_;
	uint32_t last_used_time_;
	int32_t clear_cd_;

protected:
    static AIRegType reg_type_[];
	static MyMapStr ai_type_map_;
	BTNode* parent_;
	ChildNodes children_;
	ChildNodes::const_iterator it_child_;
	BehaviorTree* tree_;
	AI* ai_;
	int32_t id_;
	string node_name_;
    string name_;
	NodeStatus node_status_;
	bool use_checked_;
	bool init_action_;  // node action cycle step
	bool move_msg_;
	bool enabled_;
    PropMap* prop_map_;
	vector<int> l;
};

inline void BTNode::reactivate_on_over()
{
	if(is_over())
	{
		node_status_ = START;
	}
}

inline bool BTNode::is_over()
{
	return FAILURE == node_status_ || SUCCESS == node_status_;
}

inline void BTNode::reactivate_on_failure()
{
	if (FAILURE == node_status_)
	{
		node_status_ = START;
	}
}

inline BTNode* BTNode::get_child(int32_t _index)
{
	return children_[_index];
}

inline int32_t BTNode::get_num_children()
{
	return children_.size();
}

inline const BTNode* BTNode::get_parent() const
{
	return parent_;
}

inline void BTNode::set_use_checked(bool _flag)
{
	use_checked_ = _flag;
}

inline void BTNode::set_name(const char* _node_name)
{
	node_name_ = _node_name;
}

inline NodeStatus BTNode::get_node_status()
{
	return node_status_;
}

inline BehaviorTree* BTNode::get_behavior_tree()
{
	return tree_;
}

inline int32_t BTNode::get_id()
{
	return id_;
}

inline 	const string& BTNode::get_name() const
{
	return node_name_;
}

inline const string& BTNode::get_raw_name() const 
{
    return name_;
}


class BTRootNode : public BTNode
{
public:
	BTRootNode();
	explicit BTRootNode(BTNode* _parent);

	void execute();
	void init();
	void update(NodeStatus _result);
};

enum BTLeafType
{
	BTLEAF_UNDEF = 0,
	BTLEAF_CONDITION = 1,
	BTLEAF_ACTION = 2
};

class BTLeafNode : public BTNode
{
public:
	BTLeafNode(BTNode* _parent, AI* _owner);
	~BTLeafNode();

	void init();
	void execute();
	void set_time(float _arg);
	void set_id(int32_t _id);
	void set_leaf_type(BTLeafType _leaf_type);
	float get_time() const;
	BTLeafType get_leaf_type() const;
	bool can_reset() const;
	bool target_last() const;
	bool target_master() const;
	bool is_leaf_node() const;
	void log_node_result(int32_t _status = -1);
	
	virtual bool can_use();
	virtual float get_default_time() const;
	virtual NodeStatus step();
	void action_msg_default(AI_MSG _msg);

protected:
	void activate();
	virtual void process_arg();
	virtual void process_endure();
	virtual float check_endure_time(float _time);
	virtual bool check_endure_cond() const;

protected:
	Tracker rule_tracker_;
	Tracker time_tracker_;
	BTLeafType leaf_type_;
	AI_TYPE target_type_;
	vector<bool> endure_cond_flag_;
	vector<BTNode *> endure_cond_node_;
	int32_t can_reset_;
};

inline bool BTLeafNode::can_reset() const
{
	return can_reset_ == 1;
}

inline void BTLeafNode::set_time(float _arg)
{
	time_tracker_.set_period(_arg);
}

inline float BTLeafNode::get_time() const
{
	return time_tracker_.get_period();
}

inline void BTLeafNode::set_leaf_type(BTLeafType _leaf_type)
{
	leaf_type_ = _leaf_type;
}


typedef std::map<std::string, PropMap*> PropertyTable;

class PropCache
{
public:
	virtual ~PropCache();
	PropMap* get_ai_prop(const std::string& _ai_key);
	void clear();

private:
	PropertyTable ai_prop_cache_;
};

#define g_prop_cache Singleton<PropCache>::instance_ptr() 

}

#endif  // __BTNODE_H__
