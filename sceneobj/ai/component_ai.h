#ifndef __COMPONENT_AI_H__
#define __COMPONENT_AI_H__

#include <vector>
#include <list>
#include <string>
#include <map>
#include <sstream>

#include "lunar.h"
#include "btree.h"
#include "threat_mgr.h"
#include "navmesh.h"
#include "world_mng.h"
#include "obj.h"
#include "ctrl.h"
#include "define_func.h"
#include "btnode.h"
#include "timer.h"

class Obj;

namespace SGame
{

class SectorCircleRange;
class CircleRange;
class ConditionSVO;

class AI
{
public:
	AI();
	~AI();
		
	void execute();

	bool query_melee_range();
	bool query_common_range(int32_t _range_id) const;
	bool query_motion_range(RANGE_TYPE _range_type, bool _is_in);
	bool query_range(RANGE_TYPE _range_type, bool _is_in,  const VECTOR3& _tar_pos) const;
	bool tgt_query_dist(float _range_dist, bool _is_in);
	bool obj_query_dist(float _range_dist, bool _is_in, Obj* _obj);

	bool is_spawning();
	bool is_in_battle() const;
	bool is_positive() const;
	bool is_fake_battle() const;
	bool is_melee() const;
	bool is_good() const;
	bool is_obj_virtual(Obj* _obj) const;
	bool is_virtual() const;
	bool is_script_move() const;
	bool is_enemy(Obj* _obj) const;
	bool is_friend(Obj* _obj) const;
	bool is_ignore_block() const;
	bool has_master() const;
    bool is_only_target(Obj* _obj) const;

	void set_pos(VECTOR3& _pos);
	void set_speed_vec(const VECTOR3& _vec , float _deviate_angle = 0.0f);
	void set_speed_len(float _length);
	void set_speed(float _walk_speed, float _run_speed, float _rush_speed);
	//void set_door_flag(unsigned short _door_flags); //no need
	void set_obj_id(uint32_t _guid);
	void set_spawn_flag(SPAWN_FLAG _flag);
	bool set_path(Navmesh::Path &_path, bool _loop, bool _not_find_path, int32_t _ani_type, const vector<PATROL_INFO> *_patrol_info, bool _broadcast = true );
	void set_extra_skills(void *_skills);	
	void set_cur_time();
	void set_steped(bool _steped);
	void set_mode(int _mode);
	void set_target(Obj* _obj);
	void set_extra_skill_id(int _skill_id);
	void set_section(int _val);
	void set_last(const std::vector<uint32_t>& _last) const;
	void set_angle(float _angle);
	void set_client_angle(float _angle);
	void reset_good();
	void reset(bool _force = false);
	void reset_off_angle();
	void reset_spawn_point();
	Obj* get_obj(uint32_t _obj_pid) const;
	float get_bounding_radius() const;
	float get_facing_radius() const;
	float get_obj_radius(Obj* _obj) const;
	float get_obj_die_time(Obj* _obj);
	bool get_reborn_flag() const;
	void set_reborn_flag(bool _flag);
	float get_melee_range() const;
	float get_melee_offset() const;
	VECTOR3 get_pos() const;
	Obj* get_last_one() const;
	World* get_world() const;
	float get_aoi_radius() const;
	int get_plane() const;
	std::string get_scene_plane_str() const;
	int32_t get_obj_camp(Obj *_obj) const;
	const list<Obj*>& get_neighbors();
	float get_chase_aoi_dist() const;
	VECTOR3 get_obj_pos(Obj* _obj) const;
	int32_t get_type_id(Obj* _obj) const;
	const VECTOR3& get_speed_vec() const;
	float get_range(int32_t _range_idx) const;
	uint32_t get_frame_begin_time() const;
	uint32_t get_frame_end_time() const;
	int32_t get_frame_counter() const;
	std::string get_ai_info(const std::string& _key) const;
	float get_speed(ANI_TYPE _ani_type);
	Ctrl* get_master() const;
	void get_combat_npc_move_ranges();
	float get_in_range() const;
	float get_out_range() const;
	float get_dis_of_master(Ctrl* _master);
	VECTOR3 get_follow_pos(Ctrl* _master, bool _gen_off_angle = true);
	void get_move_pos(u_long _world_id, int32_t _plane_id, VECTOR3& _master_pos, float _master_angle, VECTOR3& _hit_pos, bool _gen_off_angle = true);
	void gen_off_angle();
	float get_off_angle();
	float get_master_speed(Ctrl* _master);
	VECTOR3 get_master_pos(Ctrl* _master, bool _not_offset = false);
	void get_tgt_from_master();
	float get_remote_fix_rad() const;
	float get_rush_master_rate() const;
	int32_t get_ai_id() const;
	int32_t get_type_id() const;
	const std::string&  get_template_name() const;
	uint32_t get_obj_id() const;
	SPAWN_FLAG get_spawn_flag() const;
	const std::vector<uint32_t> & getExtra_skills() const;
	const VECTOR3& get_spawn_point() const;
	float get_spawn_angle() const;
	uint32_t get_id() const;
	bool get_steped() const;
	void get_float_list(const string& _val, std::vector<float>& _values);
	const std::vector<uint32_t> &get_last() const;
	VECTOR3 get_angle_vec() const;
	int get_section() const;
	int get_extra_skill_id() const;
    float get_owner_speed() const;
	
	void tgt_check_exist();
	void tgt_safe_set(Obj* _target);
	bool tgt_exist() const;
	VECTOR3 tgt_get_pos(const BTLeafNode* _node) const;
	float tgt_get_dir(const BTLeafNode* _node) const;
	int tgt_is_type(int _type) const;
	float tgt_get_angle(const BTLeafNode *_node);
	float get_angle_with_target();
	bool tgt_is_right_side();
	float tgt_get_dist(const BTLeafNode* _node);
	uint32_t tgt_get_id(const BTLeafNode* _node=NULL, bool _find_last = false) const;
	Obj* tgt_get(const BTLeafNode* _node) const;
	
	bool is_invalid_target(uint32_t _obj_id) const;
    bool is_owner_in_state(uint32_t _state_idx) const; 
	bool obj_out_chase_radius(uint32_t _obj_id);
	bool obj_in_sight_radius(Obj* _obj);
	float obj_dist_of_master(Ctrl* _master, Obj* _obj);
	float get_obj_dist(const VECTOR3 &_pos);

	bool can_leave_fight() const;
	bool can_stop() const;
	bool can_shout(Obj* _obj) const;
    bool can_do_continue_skill() const;

	void find_chase_aoi_enemy();
	bool find_path(const VECTOR3& _start, const VECTOR3& _end, Navmesh::Path* _path, unsigned short _door_flags = 0x0 );
	bool find_path_force(const VECTOR3& _start, const VECTOR3& _end, Navmesh::Path* _path);
	bool find_poly(const VECTOR3& _pos, float _min_radius, float _max_radius,\
		std::vector<VECTOR3>& _centers, float _min_rate = 0.3f);
	bool raycast(const VECTOR3& _start, const VECTOR3& _end, VECTOR3& _hit_pos_ptr, float& _rate, unsigned short _door_flags = 0x0 );

	bool can_broadcast_msg(ACT_TYPE _act_type = SHARE_ACT_NONE, TURN_TYPE _turn_type = SHARE_TURN_NONE);
	int broadcast_mst_navigation(int _add, uint32_t _obj_id, ANI_TYPE _ani_type, float _dir_angle, const Navmesh::Path &_path, TURN_TYPE _turn_type, float _rush_rate);
	int broadcast_mst_stand(int _add, uint32_t _obj_id, const string& _ani_name, float _x, float _z, ACT_TYPE _act_type, TURN_TYPE _turn_type, uint32_t _tar_id, float _dir_radian);
	int broadcast_mst_sfx(int32_t _sfx_id, int32_t _is_open, uint32_t _obj_id);
    int sync_mst_patrol_path(uint32_t _obj_id, const Navmesh::Path &_path ) const;
    int sync_mst_patrol_index(uint32_t _obj_id, uint32_t _index) const;

	void del_cond_buff();
	bool check_obj(Obj* _obj, int _tag, bool _alive) const;
	void shout(Obj* _obj);
	void self_combat_end() const;
	void speed_vec_to_tgt(const BTLeafNode* _node, float _deviate_angle = 0.0f);	
	void cast_skill(int32_t _skill_id, char _use, uint32_t _guid, char _skill_type, const VECTOR3& _pos, int32_t _use_bs) const;
	float dist_of_obj(Obj* _obj);
	float get_obj_pos_dist(Obj* _obj, const VECTOR3 &_pos);
	const VECTOR3& smooth(const VECTOR3& _recent);
	void clear_smoother();
	void _throw() const;
	bool assert_fail(bool _cond) const;
	bool no_break() const;
	int32_t get_rate_acc() const;
	bool in_drama_mode() const;
	std::string dump_info();
    void clear_last();
	bool valid_target(Obj* _obj) const;
	void try_enlarge_chase(Obj* _target, bool _default_aoi, float _dist = 0);
	void on_obj_enter(Ctrl* _obj, bool _is_master_pass = false);
    void set_home_dist_square(float _dist);
    void set_returning_home_flag(bool _is_returning);
    bool need_return_home() const;
    bool is_returning_home() const;
    float get_run_speed() const;
    VECTOR3 get_forward_angle_vec() const;
	
	int32_t l_get_ani_id(const string& _anim_name);
	void l_set_spawn_id(int32_t _spawn_id);
	bool l_get_anim_time( const string& _anim_name, float& _time );
	void l_activate_trig(const vector<float>& _chunk_idx) const;

	lua_State* ai_msg(const std::string& _action, AI_MSG _msg, int32_t _node_id) const;
	void action_msg(const std::string& _action, AI_MSG _msg, int32_t _node_id) const;
	bool svo_msg(const std::string& _target_str, AI_MSG _msg, int32_t _node_id, const std::string & _event, const std::string & _range, 
		const std::vector<int32_t> &_targets, const std::vector<float> &_ranges) const;

	bool is_obj_in_buff( uint32_t _obj_id, const vector<float>& _buff_ids, int32_t _is_buff_id ) const;
	int is_in_buff_group(uint32_t _obj_id, int32_t _group_id) const;
	bool is_super_armor()const;
	void l_set_skill(int32_t _skill_id, float _skill_cd);
	int l_skill_can_use(int32_t _skill_id);
    int32_t l_get_trigger_cur_state(uint32_t _obj_id);
	void l_stop();

    bool l_get_ai_board_value( const string& _key, int32_t& _value, int _independ );
    bool l_set_ai_board_value( const string& _key, int32_t _value, int _independ );

	//返回位面属性
	bool l_get_plane_property(const string& _key , int& _value);
	bool l_get_plane_property(const string& _key , float& _value);
	bool l_get_plane_property(const string& _key , string& _value);

	//返回配置表属性
	bool l_get_table_field(int& _value , const string& _table , const string& _one , const string& _two="" , const string& _three="");
	bool l_get_table_field(float& _value, const string& _table , const string& _one , const string& _two="" , const string& _three="");
	bool l_get_table_field(string& _value, const string& _table , const string& _one , const string& _two="" , const string& _three="");

	//返回该位面指定类型的一只怪，找不到返回0
	int l_get_plane_monster(int _type_id );

    void l_do_monster_drop( bool _need_find_player, uint32_t _obj_id, uint32_t _tgt_id, float _drop_dist );

    void l_update_mst_fight_attr_fit_pl( uint32_t _monster_id, const vector<uint32_t>& _player_li );
    void l_update_mst_fight_attr_fit_host( uint32_t _monster_id);
    void l_set_obj_god_mode( uint32_t _obj_id, bool _is_god );
    bool l_has_monster_finished_spawn(uint32_t _obj_id, bool& _is_finished);
    bool l_get_rage_value(float& _rage_value);
    bool l_get_continue_skill_id(int32_t _skill_id, int32_t& _continue_skill_id);
    bool l_get_charge_skill_id(int32_t _skill_id, const vector<int32_t>& _charge_time_range, float& _charge_time, int32_t& _charge_skill_id);
    void l_play_charge_skill_idle_anim(const VECTOR3 _pos);
    void l_broadcast_turn_to_skill_angle(float _target_angle);
    bool l_broadcast_chase_skill_move(int32_t _is_serialize, const Navmesh::Path &_path, float _chase_speed);
    void l_try_replace_skill_id(int32_t& _skill_id);

protected:
	void set_chase_radius(float _radius);

private:
	void init();
	void init_range(const vector<float>& _range_vals);
	void init_motion_range(const vector<float>& _range_vals);
	bool is_dead_or_error();
	bool is_dead() const;
	std::string get_motion_str() const;

public:
	int	c_init(lua_State* _state);
	int	c_set_running(lua_State* _state);
	int c_is_running(lua_State* _state);
    int c_set_suspend(lua_State* _state);
	int c_set_rare_print(lua_State* _state);
	int c_set_verbose_print(lua_State* _state);
	int c_set_spawn_point(lua_State* _state);
	int c_set_angle_range(lua_State* _state);
	int c_sync_path_to(lua_State* _state);
	int c_on_obj_leave(lua_State* _state);
	int c_set_spawn_anim(lua_State* _state);
	int c_on_obj_enter(lua_State* _state);
	int c_is_spawning(lua_State* _state);	
	int c_tgt_get_id(lua_State* _state);
	int c_set_patrol_path(lua_State* _state);
	int c_set_follow_pos(lua_State* _state);
	int c_set_path(lua_State* _state);
	int c_set_cond_buff(lua_State* _state);
	int c_get_op_trigger(lua_State* _state);
	int c_on_message(lua_State* _state);
	int c_add_spawn_node(lua_State* _state);
	int c_get_prop_int( lua_State* _state);
	int c_get_prop_float( lua_State* _state);
	int c_get_prop_float_list( lua_State* _state);
	int c_assert_fail( lua_State* _state);
	int c_get_tgt_id( lua_State* _state);
    int c_leave_master( lua_State* _state);
	int c_set_extra_skill_id( lua_State* _state);
	int c_add_cond_buff( lua_State* _state);
	int c_set_last( lua_State* _state);
	int c_get_last( lua_State* _state);
	int c_get_threat_list( lua_State* _state);
    int c_add_threat( lua_State* _state );
    int c_reset_threat( lua_State* _state );
    int c_set_threat( lua_State* _state );
    int c_set_only_target(lua_State* _state);
    int c_set_forward_angle( lua_State* _state );
    int c_get_cur_node_name( lua_State* _state );
    

public:
	static const char className[];
	static Lunar<AI>::RegType methods[];
	static const bool gc_delete_;		
	int rare_print_;
	int verbose_print_;
	static int32_t proto_counter_;
	bool leave_fight_;	//可以脱战
	bool not_ot_;
	Obj* owner_;

protected:
	uint32_t cur_time_;
	uint32_t resume_time_;
	uint32_t buff_del_time_;
	int32_t frame_counter_;

private:
	Obj* target_;
    bool is_rm_combat_npc_;
    float remote_fix_rad_;
    float rush_master_rate_;
    uint32_t master_id_;
	uint32_t last_target_id_;
	uint32_t target_id_;
    float bt_in_range_;
    float bt_out_range_;
    float nbt_in_range_;
    float nbt_out_range_;
    float off_angle_;

	uint32_t obj_id_;
	VECTOR3 speed_vec_;
	mutable bool is_good_;
	BehaviorTree btree_;
	ThreatMgr threat_mgr_;
	Navmesh::Path path_;
	vector<PATROL_INFO> patrol_info_;
	int32_t motion_range_idx_;
	ConditionSVO* motion_node_;
	int32_t ai_id_;
	std::string template_name_;
	Smoother smoother_;
	bool target_visible_;
	float chase_aoi_dist_;
	VECTOR3 spawn_point_;
	float spawn_angle_;
	float angle_max_;
	float angle_min_;
	int cannot_turn_;
	ACT_TYPE last_act_type_;
	TURN_TYPE last_turn_type_;
	bool steped_;
	bool is_virtual_;
	bool nobreak_;
	int32_t rate_acc_;
	SPAWN_FLAG spawn_flag_;
	uint8_t mode_;
	bool script_move_;
	bool reborn_flag_;
	bool force_in_battle_;
	bool is_moving_;
	bool write_pos_;
	float walk_speed_;
	float run_speed_;
	float rush_speed_;
	float melee_offset_;
	float out_range_;
	float in_range_;
	bool  ignore_block_;
	int extra_skill_id_;	//用外部设置的skill来代替
	int section_;		//时间分段
	float buff_min_time_;
	float buff_mst_dist_;
	int32_t buff_mst_num_;
	int32_t buff_del_id_;
    float home_dist_square_;  //与出生点相差dist的距离就返回，此值为dist的平方
    bool is_returning_home_;  //是否正在返回出生点

	list<Obj*> neighbors_;
	list<Obj*> crowds_;
	vector<SectorCircleRange> motion_sectors_;	//进入战斗距离
	vector<CircleRange> motion_circles_;		//追击距离
	vector<CircleRange> near_walks_;
	vector<CircleRange> ranges_;
	mutable std::vector<uint32_t> last_;
	std::map<TURN_TYPE, TURN_TYPE> turn_map_;
	std::vector<uint32_t> extra_skills_;
	bool is_running_;
	bool is_positive_;
    OBJID only_id_;
    float forward_angle_; // 设置的向前的角度 action中使用
};



//inline function definitions

inline bool AI::get_reborn_flag() const
{
	return reborn_flag_;
}

inline void AI::set_reborn_flag( bool _flag )
{
	reborn_flag_ = _flag;
}

inline VECTOR3 AI::get_pos() const
{
	return owner_->getpos();
}

inline World* AI::get_world() const
{
	return static_cast<Ctrl*>(owner_)->get_world();
}

inline float AI::get_aoi_radius() const
{
	return get_world()->get_scene()->get_min_dynamic_land_size()*2.5;
}

inline int AI::get_plane() const
{
	return static_cast<Ctrl*>(owner_)->plane_;
}

inline bool AI::tgt_exist() const
{
	return target_ != NULL;
}

inline float AI::get_chase_aoi_dist() const
{
	return chase_aoi_dist_;
}

inline const VECTOR3& AI::get_speed_vec() const
{
	return speed_vec_;
}

inline float AI::tgt_get_angle(const BTLeafNode *_node)
{
	return degrees(dot_angle(tgt_get_pos(_node) - get_pos(), speed_vec_));
}

inline float AI::get_angle_with_target()
{
    float angle = get_degree( tgt_get_pos( NULL ), get_pos() );
	return norm_angle(angle);
}

inline bool AI::tgt_is_right_side()
{
	VECTOR3 target_vec_ = tgt_get_pos(NULL) - get_pos();
	VECTOR3 prod;
	vc3_cross(prod, target_vec_, speed_vec_);
	return prod.y < 0;
}

inline void AI::speed_vec_to_tgt( const BTLeafNode *_node, float _deviate_angle /*= 0.0f*/ )
{
	speed_vec_ = tgt_get_pos(_node) - get_pos();
	set_speed_vec(speed_vec_ , _deviate_angle);
}

inline bool AI::is_in_battle() const
{
	return target_ != NULL;
}

inline bool AI::is_positive() const
{
	return is_positive_;
}

inline uint32_t AI::get_frame_begin_time() const
{
	return cur_time_;
}

inline uint32_t AI::get_frame_end_time() const
{
	return cur_time_ + g_timermng->get_ms_one_frame();
}

inline int32_t AI::get_frame_counter() const
{
	return frame_counter_;
}

inline int32_t AI::get_ai_id() const
{
	return ai_id_;
}

inline const std::string& AI::get_template_name() const
{
	return template_name_;
}

inline float AI::tgt_get_dist( const BTLeafNode* _node )
{
	return	length2d(get_pos() - tgt_get_pos(_node));
}

inline float AI::dist_of_obj( Obj *_obj )
{
	return length2d(get_pos() - get_obj_pos(_obj));
}

inline void AI::set_obj_id( uint32_t _guid )
{
	obj_id_ = _guid;
}

inline uint32_t AI::get_obj_id() const
{
	return obj_id_;
}

inline SPAWN_FLAG AI::get_spawn_flag() const
{
	return spawn_flag_;
}

inline const std::vector<uint32_t> & AI::getExtra_skills() const
{
	return extra_skills_;
}

inline const VECTOR3& AI::get_spawn_point() const
{
	return spawn_point_;
}

inline float AI::get_spawn_angle() const
{
	return spawn_angle_;
}

inline const VECTOR3& AI::smooth( const VECTOR3& _recent )
{
	return smoother_.update(_recent);
}

inline void AI::clear_smoother()
{
	smoother_.clear();
}

inline void AI::_throw() const
{
	is_good_ = false;
}

inline bool AI::is_good() const
{
	return is_good_;
}

inline void AI::reset_good()
{
	is_good_ = true;
}

inline void AI::reset( bool _force/*=false*/ )
{
	btree_.reset(_force);
}

inline void AI::set_steped( bool _steped )
{
	steped_ = _steped;
}

inline void AI::set_mode( int _mode )
{
	mode_ = _mode;
}

inline bool AI::get_steped() const
{
	return steped_;
}

inline bool AI::can_stop() const
{
	return true;
}

inline int32_t AI::get_rate_acc() const
{
	return rate_acc_;
}

inline bool AI::in_drama_mode() const
{
	return mode_ == SHARE_AI_DRAMA_SEEK_MODE || mode_ == SHARE_AI_DRAMA_COMBAT_MODE;
}

inline bool AI::is_script_move() const
{
	return script_move_;
}

inline void AI::set_section( int _val )
{
	section_ = _val;
}

inline int AI::get_section() const
{
	return section_ ;
}

inline int AI::get_extra_skill_id() const
{
	return extra_skill_id_;
}

inline bool AI::is_ignore_block() const
{
	return ignore_block_;
}

inline void AI::set_home_dist_square(float _dist)
{
    home_dist_square_ = _dist;
}

inline void AI::set_returning_home_flag(bool _is_returning)
{
    is_returning_home_ = _is_returning;
}

inline bool AI::need_return_home() const
{
    if (home_dist_square_ > 0)
    {
        return vc3_len(get_spawn_point() - get_pos()) > home_dist_square_;
    }
    return false;
}

inline bool AI::is_returning_home() const
{
    return is_returning_home_;
}

inline float AI::get_run_speed() const
{
    return run_speed_;
}

#define LUNAR_AI_METHODS			\
	method( AI, c_init ),\
	method( AI, c_set_rare_print),\
	method( AI, c_set_verbose_print),\
	method( AI, c_set_spawn_point),\
	method( AI, c_set_angle_range),\
	method( AI, c_sync_path_to),\
	method( AI, c_on_obj_leave),\
	method( AI, c_set_spawn_anim),\
	method( AI, c_on_message),\
	method( AI, c_add_spawn_node),\
	method( AI, c_on_obj_enter),\
	method( AI, c_is_spawning), \
	method( AI, c_tgt_get_id),\
	method( AI, c_set_patrol_path),\
	method( AI, c_set_path),\
	method( AI, c_set_cond_buff),\
	method( AI, c_get_op_trigger),\
	method( AI, c_set_follow_pos),\
	method( AI, c_set_running),\
	method( AI, c_is_running),\
    method( AI, c_set_suspend ),\
    method( AI, c_leave_master),\
	method( AI, c_get_prop_int),\
	method( AI, c_get_prop_float),\
	method( AI, c_get_tgt_id),\
	method( AI, c_assert_fail),\
	method( AI, c_set_extra_skill_id),\
	method( AI, c_add_cond_buff),\
	method( AI, c_set_last),\
	method( AI, c_get_last),\
	method( AI, c_add_threat),\
	method( AI, c_reset_threat),\
	method( AI, c_set_threat),\
	method( AI, c_get_threat_list),\
	method( AI, c_set_only_target),\
	method( AI, c_get_cur_node_name),\
	method( AI, c_set_forward_angle)

}

#endif  // __COMPONENT_AI_H__
