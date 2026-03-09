#ifndef __CONDITIONS_H__
#define __CONDITIONS_H__

#include <vector>
#include <string>
#include "btnode.h"
#include "common.h"
#include "spirit.h"

namespace SGame
{

class ConditionFailure: public BTLeafNode
{
public:
	ConditionFailure(BTNode* _parent, AI* _id);
	void init();
	void activate();
};


class ConditionSuccessful: public BTLeafNode
{
public:
	ConditionSuccessful(BTNode* _parent, AI* _id);
	void init();
	void activate();
};


class ConditionSVO: public BTLeafNode
{
public:
	ConditionSVO(BTNode* _parent, AI* _id);
	void init();
	void activate();

	void set_ranges(const vector<float>& _ranges, AI_TYPE _range_type);
	const vector<float>& get_ranges() const;
	void set_range_pair(int32_t _idx_start, int32_t _idx_end);
	void set_chase_radius(float _radius);
	void reset_chase_radius();

	AI_TYPE get_range_type() const;
	float get_chase_radius();
	AI_TYPE get_event() const;
	const string& get_range_str() const;
	virtual bool can_use();

protected:
	bool svo_msg_default(AI_MSG _msg);
	bool eval_section();
	bool eval_time();
	bool eval_obj();
	bool eval_obj_camp(AI_TYPE _ai_type);
	bool eval_obj_area();
	void eval_obj_area_imp(std::vector<uint32_t> &list, int32_t _link_type, int32_t _obj_type);
	void eval_obj_camp_imp(std::vector<uint32_t> &list, int32_t _link_type, int32_t _obj_type, AI_TYPE _ai_type);

	bool eval_target();
	bool eval_master();
	bool eval_state();
	bool eval_board();
    bool eval_target_side();
    bool eval_rage_value();

	bool check_num(float _num, float _range);
	bool check_range_dists(float _dist);
	bool check_range_num_dist(float _dist);
	bool check_type_id(Ctrl *obj);
	bool check_not_self(Ctrl *obj);
	bool check_camp(Ctrl * _obj, AI_TYPE _ai_type);
	bool check_camps(Ctrl * _obj);
    bool check_spawned(Ctrl* _obj, int32_t _obj_type);

private:
	void set_id(int32_t _id);

private:
	std::string target_str_;
    string target_substr_;
	std::vector<int32_t> target_ids_;

	std::string event_str_;
	AI_TYPE event_;

	std::string range_str_;  //  range_str
	vector<float> ranges_;  //  range_val
	AI_TYPE range_type_;  //  range_type
	int32_t range_pair_[2];  //  range_id

	float chase_radius_;
	float time_;
    int32_t state_;  //state for trigger
	VECTOR3 pos_;
	float radius_;
	std::vector<int32_t> camps_;
	AI_TYPE camp_;
	bool is_default_;
    int32_t independ_;
    AI_TYPE side_type_;   //left, right
    int32_t finished_spawn_;  //for monster check
    int32_t is_buff_id_;
};

inline void ConditionSVO::reset_chase_radius()
{
	chase_radius_ = get_ranges()[RANGE_CHASE];
}

inline float ConditionSVO::get_chase_radius()
{
	return chase_radius_;
}

inline void ConditionSVO::set_chase_radius(float _radius)
{
	chase_radius_ = _radius;
}

inline void ConditionSVO::set_range_pair(int32_t _idx_start, int32_t _idx_end)
{
	range_pair_[0] = _idx_start;
	range_pair_[1] = _idx_end;
}

inline const vector<float>& ConditionSVO::get_ranges() const
{
	return ranges_;
}

inline AI_TYPE ConditionSVO::get_range_type() const
{
	return range_type_;
}

inline void ConditionSVO::set_ranges(const vector<float>& _ranges, AI_TYPE _range_type)
{
	ranges_ = _ranges;
	range_type_ = _range_type;
}

inline AI_TYPE ConditionSVO::get_event() const
{
	return event_;
}

inline const string& ConditionSVO::get_range_str() const
{
	return range_str_;
}

}

#endif  // __CONDITIONS_H__
