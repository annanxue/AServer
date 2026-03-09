#ifndef __ACTION_H__
#define __ACTION_H__

#include <cmath>
#include <vector>
#include <string>
#include "btnode.h"
#include "common.h"
#include "component_ai.h"
#include "utils.h"
#include "states.h"

namespace SGame
{

class ActionSkill : public BTLeafNode
{
public:
	ActionSkill(BTNode* _parent, AI* _id);
	void init();
	void activate();
	bool can_reset() const;
	NodeStatus step();
	vector<int>& get_chunk_idx();
	float get_default_time() const;
	float check_endure_time(float _time);
	uint32_t get_target_info() const;
	VECTOR3 get_skill_pos();
    void set_angle();
    void use_skill(int32_t _skill_id);

	virtual void on_message(MSG_TYPE _msg);
	virtual bool can_use();
	virtual SKILL_TGT_TYPE get_skill_type() const;

protected:
	int32_t skill_id_;
    int32_t last_skill_id_;        //for continue skill
    int32_t last_skill_frame_;     //for continue skill
	Tracker max_time_tracker_;
	bool in_ai_delay_;
	float skill_cd_;

private:
	float ai_delay_time_;
	float force_angle_;
	int32_t spawn_angle_;
	std::vector<float> turn_pos_;
	float max_time_;
	bool use_external_skillid_;	
	float deviate_angle_;
	int32_t no_cd_;
	int32_t no_turn_;
	vector<int> chunk_idx_;
	int32_t bs_;
};

inline vector<int>& ActionSkill::get_chunk_idx()
{
	return chunk_idx_;
}

inline float ActionSkill::get_default_time() const
{
	return ai_delay_time_;
}

inline bool ActionSkill::can_reset() const
{
	return false;
}


class ActionChase : public BTLeafNode
{
public:
	ActionChase(BTNode* _parent, AI* _id, AI_TYPE _action_type = ACTION_CHASE);
	int sync_path_to() const;
	void activate();
	NodeStatus step();
	void init();
	bool is_spec_rush() const;
	float get_default_time() const;

protected:
	float get_speed();
	void get_cur_path(Navmesh::Path* _path) const;
	ANI_TYPE get_ani_type() const;
	void select_path_type_combat_npc();
	void check_path() const;
	void clip_path( float _max_dist, bool _intrp_last_point );
	virtual void set_patrol_index(bool _is_end){}

	virtual bool path_need_target();
	virtual uint32_t tgt_get_id() const;
	virtual bool is_direct_path();
	virtual void clip_detour_path(){ clip_path(chase_dist_, true); }
	virtual bool send_path() const;
	virtual VECTOR3 get_seek_pos() const;
	virtual VECTOR3 get_target_pos2d() const;
	virtual void process_arg();
	virtual void get_msg_path(Navmesh::Path* _path) const;
	virtual void set_ani_type(ANI_TYPE _ani_type, bool _override = false);
	virtual void chase_with_path();
	virtual bool can_move();
	virtual bool calcu_path(AI_TYPE _path_type);
	virtual void broadcast_path();
	virtual bool clip_direct_path();
	virtual void init_path();
	virtual	void select_path_type();

protected:	
	bool is_stand_;
	bool in_near_walk_;
	bool override_ani_type_;
	bool chase_master_;
	float raycast_rate_;
	float min_rate_;
	float clip_time_;
	float chase_dist_;
	float dir_radian_;	
	AI_TYPE action_type_;
	ANI_TYPE ani_type_;
	TURN_TYPE turn_type_;
	VECTOR3 raycast_hitpos_;
	int32_t path_counter_;
	uint32_t already_stand_time_;
	Navmesh::Path path_;
	vector<uint32_t> path_timer_;
    bool check_valid_pos_;
};

inline bool ActionChase::is_spec_rush() const
{
	return ani_type_ == SHARE_ANI_RUSH_FRONT || ani_type_ == SHARE_ANI_RUSH_RIGHT;
}

inline float ActionChase::get_default_time() const
{
	return 5.f;
}

inline ANI_TYPE ActionChase::get_ani_type() const
{
	return ani_type_;
}

class ActionExplore : public ActionChase
{
public:
	ActionExplore(BTNode* _parent, AI* _id);
	NodeStatus step();
	void init();
	bool set_path(Navmesh::Path &_path, bool _loop, bool _not_find_path, int32_t _ani_type, const vector<PATROL_INFO> *_patrol_info, bool _broadcast = true);
	virtual bool can_use();

protected:
	void activate();
	bool send_path() const;
	bool path_need_target();
	bool is_direct_path();
	uint32_t tgt_get_id() const;
	void init_path();
	void get_patrol_path();
	void chase_with_path();	
	bool clip_direct_path();
	void set_move_list();
	void set_patrol_move_list();
	int sync_path_to() const;
	void do_stand();
	void l_get_patrol_path();   
	void get_msg_path(Navmesh::Path* _path) const;
	void load_path( lua_State* _state, Navmesh::Path& _path );
	void get_old_dir(bool _is_in_battle, VECTOR3& _old_dir, const Navmesh::Path& _new_path);
	bool add_new_path(float* _dist, Navmesh::Path* _new_path,  const Navmesh::Path& _temp_path);
	bool find_by_poly(bool _is_in_battle, const VECTOR3& _old_dir, std::vector<VECTOR3>* _points, \
		const Navmesh::Path& _new_path, Navmesh::Path& _temp_path);
	bool find_by_ray(bool _is_in_battle, const VECTOR3& _old_dir, const Navmesh::Path& _new_path, \
		Navmesh::Path* _temp_path, bool _force_ray);
    size_t find_nearest_to_start(Navmesh::Path& _path);
    void broadcast_path();

	virtual bool can_move();
	virtual bool calcu_path(AI_TYPE _path_type);
	virtual void set_patrol_index(bool _is_end);

private:
	std::string ani_name_;
	ACT_TYPE act_type_;
	float move_rate_;
	float move_time_;
	float stand_time_;
	float min_angle_;
	float max_angle_;
	float radius_;
	float total_move_time_;
	float back_rate_;
	int32_t move_counter_;
	int32_t sign_;
	std::vector<bool> move_list_flag_;
	std::vector<uint32_t> move_list_time_;
	std::vector<bool> move_timer_flag_;
	std::vector<uint32_t> move_timer_time_;
	int32_t force_explore_;
	bool loop_;
	int32_t patrol_ani_type_;
    int32_t patrol_path_id_; 
	int32_t patrol_path_index_;
	Navmesh::Path patrol_path_;
	bool has_patrol_path_;
	vector<PATROL_INFO> patrol_info_;
    int32_t start_with_nearest_;
    bool broadcast_;
};

inline bool ActionExplore::send_path() const
{
	return false;
}

inline uint32_t ActionExplore::tgt_get_id() const
{
	return 1;
}

inline bool ActionExplore::path_need_target()
{
	return ai_->is_in_battle();
}

inline bool ActionExplore::is_direct_path()
{
	return true;
}


class ActionEvasion : public ActionChase
{
public:
	ActionEvasion(BTNode* _parent, AI* _id);

protected:
	bool is_direct_path();
	bool path_need_target();

private:
	void calcu_escape_path();
	void clip_detour_path();
};

inline bool ActionEvasion::is_direct_path()
{
	calcu_escape_path();
	return false;
}

inline bool ActionEvasion::path_need_target()
{
	return false;
}

inline void ActionEvasion::clip_detour_path()
{
	check_path();
}


class ActionSwing : public ActionChase
{
public:
	ActionSwing(BTNode* _parent, AI* _id);
	void init();
	void activate();
	virtual VECTOR3 get_seek_pos() const;

protected:

private:
	std::string ani_name_;
	float angle_min_;
	float angle_max_;
	mutable float sign_;
};


class ActionFollowMaster : public ActionChase
{
public:
    ActionFollowMaster(BTNode* _parent, AI* _id);
    void init();
    void activate();

protected:
	NodeStatus step();
    void select_path_type();
};


class ActionAct : public BTLeafNode
{
public:
	ActionAct(BTNode* _parent, AI* _id);
	NodeStatus step();
	void activate();
	void init();
	void set_anim(const std::string& _anim, float _dir);
	bool can_use();
	float get_default_time() const;
	int sync_path_to() const;
	float get_dir_radian() const;

protected:
    bool init_anim_time(const std::string& _anim_name);

protected:
	std::string ani_name_;
	ACT_TYPE act_type_;
	TURN_TYPE turn_type_;
	float turn_rate_;
	mutable float dir_radian_;
	bool is_spawn_;
	std::vector<float> turn_pos_;
};

inline bool ActionAct::can_use()
{
	if(is_spawn_ && ai_->tgt_exist())
	{
		return false;
	}
	return BTLeafNode::can_use();
}

inline float ActionAct::get_default_time() const
{
	return atof(get_ai_info("AIDelay").c_str());
}


class ActionShuffle : public ActionAct
{
public:
	ActionShuffle(BTNode* _parent, AI* _id);
	void activate();
    void init();

private:
    float min_deviate_angle_;
};

class ActionHold : public ActionChase
{
public:
	ActionHold(BTNode* _parent, AI* _id);
	void activate();
	void init();

protected:
	VECTOR3 get_seek_pos() const;
	void broadcast_path();
	void set_ani_type(ANI_TYPE _ani_type, bool _override = false);
	virtual void select_path_type();
	void do_stand();
private:
	ANI_TYPE base_ani_type_;
	int32_t front_;
	int32_t left_;
	int32_t right_;
	int32_t back_;
};

class ActionVaryingChase : public ActionChase
{
public:
	ActionVaryingChase(BTNode* _parent, AI* _id);
	void activate();

protected:
	void init();

private:
	std::vector<ANI_TYPE> ani_types_;
	std::vector<float> times_;
	std::vector<float> rates_;
	std::vector<float> clip_times_;
	uint32_t size_;
	Tracker clip_time_tracker_;
};

class ActionSkillChase : public ActionChase
{
public:
	ActionSkillChase(BTNode* _parent, AI* _id);
	void init();
	bool is_direct_path();
	void activate();
	bool is_cooling_down();
	VECTOR3 get_seek_pos() const;
	
protected:
	float check_endure_time(float _time);

private:
	NodeStatus step();

private:
	float min_range_;
	float max_range_;
	int32_t skill_id_;
};

inline bool ActionSkillChase::is_cooling_down()
{
	return false;
}

class ActionSpawn : public ActionAct
{
public:
	ActionSpawn(BTNode* _parent, AI* _id);
	void set_flag(bool _reborn, bool _spawn_buff);
	bool can_reset() const;
	void reset();
	void terminate();
	void set_anim(const char* _anim, float _time);
	virtual void clear_cd(bool _force);	

protected:
	bool can_use();

private:
	bool reborn_;
	bool spawn_buff_;
};

inline void ActionSpawn::set_flag(bool _reborn, bool _spawn_buff)
{
	reborn_ = _reborn;
	spawn_buff_ = _spawn_buff;
}

inline bool ActionSpawn::can_reset() const
{
	return false;
}

inline void ActionSpawn::reset()
{
	node_status_ = SUCCESS;
	terminate();
}

inline void ActionSpawn::terminate()
{
	if(SUCCESS  == node_status_)
	{
		node_status_ = START;
		//ai_->use_buff(SPAWN_BUFF, 0);
		ai_->l_set_spawn_id(0);
	}
	ActionAct::terminate();
}

inline void ActionSpawn::set_anim(const char* _anim, float _time)//bugful
{
	if(_anim)
	{
		ani_name_ = _anim;
	}
	set_time(_time);
}


class ActionShout : public ActionAct
{
public:
	ActionShout(BTNode* _parent, AI* _id);
	float get_default_time() const;

protected:
	bool can_use();
	void init();
	void activate();
	float check_endure_time(float _time);

private:
	std::vector<int32_t> ai_ids_;
	std::vector<Obj*> targets_;
	float range_;
};


class ActionSfx : public ActionAct
{
public:
	ActionSfx(BTNode* _parent, AI* _id);
	bool can_step() const;

protected:
	void init();
	void activate();

private:
	int32_t sfx_id_;
	int32_t is_open_;
	float delay_time_;
    bool is_self_play_;
};

inline bool ActionSfx::can_step() const
{
	return false;
}

class ActionSetBoard : public ActionAct
{
public:
	ActionSetBoard(BTNode* _parent, AI* _id);
	bool can_step() const;
	
protected:
	void init();
	void activate();

private:
	std::string tgt_;
	int32_t range_;
	int32_t add_;
    int32_t independ_;
};

inline bool ActionSetBoard::can_step() const
{
	return false;
}

class ActionDefault : public ActionAct
{
public:
	ActionDefault(BTNode* _parent, AI* _id);
	bool can_step() const;

protected:
	void activate();
	void init();
};

inline bool ActionDefault::can_step() const
{
	return false;
}

class ActionDrop : public BTLeafNode
{
public:
	ActionDrop( BTNode* _parent, AI* _id );
	void init();
	bool can_step() const;
	float get_default_time() const;

protected:
	void activate();
    int32_t drop_to_enemy_;
    float drop_dist_;
    
};

inline bool ActionDrop::can_step() const
{
	return false;
}

inline float ActionDrop::get_default_time() const
{
	return 0.0f;
}

class ActionUpdateFightAttr: public BTLeafNode
{
public:
	ActionUpdateFightAttr(BTNode* _parent, AI* _id);
	void init();
    bool can_step() const;
	float get_default_time() const;

protected:
	void activate();
private:
    float check_range_;
    int exclude_same_camp_;
    int by_host_;
};

inline bool ActionUpdateFightAttr::can_step() const
{
    return false;
}

inline float ActionUpdateFightAttr::get_default_time() const
{
	return 0.0f;
}

class ActionReturnHome : public ActionChase
{
public:
	ActionReturnHome(BTNode* _parent, AI* _id);

private:
	void init();
	void activate();
	bool can_use();
	void clip_detour_path();
	bool clip_direct_path();
	bool path_need_target();
	VECTOR3 get_seek_pos() const;
	NodeStatus step();
	float get_default_time() const;

private:
    float home_dist_square_;
};

inline float ActionReturnHome::get_default_time() const
{
	return 3600.0f;
}

inline bool ActionReturnHome::clip_direct_path()
{
	return true;
}

inline void ActionReturnHome::clip_detour_path()
{
}

inline bool ActionReturnHome::path_need_target()
{
	return false;
}

class ActionChargeSkill : public ActionSkill
{
public:
    ActionChargeSkill(BTNode* _parent, AI* _id);

private:
	void init();
	void activate();
	NodeStatus step();

private:
    bool is_config_error_;
    bool do_charge_skill_;
    int32_t charge_skill_id_;
    float charge_time_;
	vector<int> charge_time_range_;
	Tracker charge_time_tracker_;
};

class ActionChaseSkill : public ActionSkill
{
public:
    ActionChaseSkill(BTNode* _parent, AI* _id);
	int sync_path_to() const;

private:
	void init();
	void activate();
	NodeStatus step();
	float get_speed() const;
    void init_chase_path();
    void turn_to_target();
    void clip_direct_path();
    void clip_detour_path();
    void chase_with_path();
    void calculate_path_and_time(AI_TYPE _path_type);
	void on_message(MSG_TYPE _msg);
    bool is_target_on_left(const VECTOR3& _target_pos, const VECTOR3& _pos, const VECTOR3& _speed_vec);

private:
    float turn_speed_;
	float clip_time_;
	float chase_dist_;
    float chase_speed_;
	Navmesh::Path path_;
	int32_t path_counter_;
	vector<uint32_t> path_timer_;
    int32_t move_persist_skill_;
    bool is_first_skill_end_;
	VECTOR3 raycast_hitpos_;
	float raycast_rate_;
    int32_t last_target_id_; 
	Tracker turn_time_tracker_;
    float min_dist_to_target_;
};

}
#endif  // __ACTION_H__
