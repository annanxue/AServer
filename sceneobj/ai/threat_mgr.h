#ifndef __THREATMGR_H
#define __THREATMGR_H

#include "common.h"
#include "ctrl.h"
#include <set>
#include "lunar.h"

namespace SGame
{

class AI;

static const float VIEW_THREAT = 100.0f;
static const float BASE_THREAT = 100.0f;

enum ThreatType
{
    THREAT_TYPE_BASE = 1, 
    THREAT_TYPE_HURT, 
    THREAT_TYPE_RECOVER,
    THREAT_TYPE_SKILL, 
};

class ThreatItem
{
public: 
    ThreatItem();
    float get_threat();
    void set( ThreatType _threat_type,  float _threat );
    void add( ThreatType _threat_type, float _add_threat );
    void reset();

private:
    float base_threat_;
    float hurt_threat_;
    float recover_threat_;
    float skill_threat_;
};


class ThreatMgr
{
public:
	ThreatMgr();
	void set_owner(AI* _ai);
	void set_combat_npc_target(Ctrl* _master, bool _in_flag);
	void remove_invalid_target(bool _remove_dead);
	void remove_targets(const std::vector<uint32_t>& _out_target_ids);
	void remove_threat_with_entity(uint32_t _target_id, bool _not_out = false);
	void on_obj_leave(uint32_t _target_id);
	void on_obj_enter(Ctrl* _obj);
	bool is_enemy(Obj* _target);
	bool is_fake_battle() const;
	bool can_threat( Obj* _target, float _threat);
	void add_threat(Obj* _target, float _threat, bool _default_aoi, bool _not_ot = false);
	void on_change();
	float calc_threat(int32_t _damage_type, int32_t _damage_value, int32_t _skill_id);
	void execute(bool _in_flag, bool _out_flag);
	void try_replace_target(Obj* _target, float _threat);
	std::string dump_info() const;
	bool in_threat(uint32_t _target_id);
	void clear_all_threats();
	void add_damage(uint32_t _attacker_id, int32_t _damage_type, int32_t _damage_value, int32_t _skill_id);
    bool find_nearest_tgt();
    bool obj_out_combat_npc_reach(Ctrl* _master, uint32_t _tgt_id);
    void select_ai_target( SELECT_AI_TARGET_TYPE _select_type );
    void set_threat( Ctrl* _target,  ThreatType _threat_type, float _threat);
    int  get_threat_list_to_lua( lua_State* _state );
    void add_target_threat( Ctrl* _target, ThreatType _threat_type, float _threat );
    void reset_threat( Ctrl* _target );

protected:
	void outsight_then_insight();

private:
	uint32_t next_change_target_time_;
	float melee_ot_;
	float range_ot_;
	float change_target_interval_;
	AI *ai_;
	std::vector<uint32_t> ranks_;  // pid list
	//std::map<uint32_t, float> threats_;  //threat value
    std::map<uint32_t, ThreatItem*> threats_;
    /////AOI内怪视野外的敌人////
	std::set<uint32_t> out_of_sight_;
public:
	bool ai_debug_;
};

}

#endif
