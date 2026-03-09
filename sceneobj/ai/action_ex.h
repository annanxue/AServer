#ifndef __ACTIONEX_H__
#define __ACTIONEX_H__

#include "action.h"

namespace SGame
{

class ActionEmote : public ActionAct
{
public:
	ActionEmote(BTNode* _parent, AI* _id);
	bool can_reset() const;
	
};

inline bool ActionEmote::can_reset() const
{
	return false;
}


class ActionActivateTrig : public BTLeafNode
{
public:
	ActionActivateTrig(BTNode* _parent, AI* _id);
	void init();
	bool can_step() const;
	void activate();
    float get_default_time() const;

private:
	vector<float> chunk_idx_;
};

inline float ActionActivateTrig::get_default_time() const
{
	return atof(get_ai_info("AIDelay").c_str());
}

inline bool ActionActivateTrig::can_step() const
{
	return false;
}

class ActionMoveTo : public ActionChase
{
public:
	ActionMoveTo(BTNode* _parent, AI* _id);

private:
	void init();
	virtual bool clip_direct_path();
	virtual void clip_detour_path();
	bool path_need_target();
	VECTOR3 get_seek_pos() const;
	NodeStatus step();
	void select_path_type();
	float get_default_time() const;

private:
	VECTOR3 seek_pos_;
	float front_dis_;
	int32_t find_path_;
    int towards_;
};

inline float ActionMoveTo::get_default_time() const
{
	return 3600.0f;
}

inline bool ActionMoveTo::clip_direct_path()
{
	return true;
}

inline void ActionMoveTo::clip_detour_path()
{
}

inline bool ActionMoveTo::path_need_target()
{
	return false;
}


//找指定的怪，然后追过去
class ActionFollowMonster : public ActionChase
{
public:
	ActionFollowMonster(BTNode* _parent, AI* _id);
	virtual void activate();

protected:
	void init();
	bool clip_direct_path();
	bool path_need_target();
	VECTOR3 get_seek_pos() const;
	NodeStatus step();
	void select_path_type();

private:
	VECTOR3 seek_pos_;
	std::vector<int> monster_type_id_;
};

inline bool ActionFollowMonster::clip_direct_path()
{
	return true;
}

inline bool ActionFollowMonster::path_need_target()
{
	return false;
}

inline VECTOR3 ActionFollowMonster::get_seek_pos() const
{
	return seek_pos_;
}

inline NodeStatus ActionFollowMonster::step()
{
	chase_with_path();
	return node_status_;
}

inline void ActionFollowMonster::select_path_type()
{
	calcu_path(PATH_DIRECT);
}

class ActionSetSection : public ActionAct
{
public :
	ActionSetSection(BTNode* _parent , AI* _id);
	void init();
	void activate();

private:
	std::vector<float> section_range_;
};

}

#endif  // __ACTIONEX_H__
