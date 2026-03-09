#ifndef __TRIGGER_H__
#define __TRIGGER_H__

#include <vector>

#include "spirit.h"
#include "chipmunk.h"

const int RECTANGLE_CORNER_NUM = 4;

class Trigger;
class AoiTrigger 
{
public:
	AoiTrigger( OBJID _owner_id, int _shape_type, float _rad, float _lw_per );
	virtual ~AoiTrigger();

public:
	bool ray_test( Ctrl* _target );
	bool ray_test( OBJID _obj_id );

	void set_check_dynamic(bool _check) { is_check_dynamic_ = _check; }
	bool get_check_dynamic() { return is_check_dynamic_; }

	void set_radius(float _rad) { radius_ = _rad; };
private:
    void create_aoi_shape();
    void create_circle_shape_aoi();
    void create_rectangle_shape_aoi();
    bool is_in_aoi( Ctrl* _target );
    bool is_in_circle_aoi( Ctrl* _target );
private:
    int shape_type_;
	float radius_;
	float lw_percenet_;
	OBJID    owner_id_;
	bool	 is_check_dynamic_;
    VECTOR3 corners_[RECTANGLE_CORNER_NUM];
    cpShape* aoi_shape_;
};

class Trigger : public Spirit
{
public:
	Trigger();
	virtual ~Trigger();

	virtual void		process_parallel();
	virtual void		process_serial();
	virtual int			view_add( Ctrl* _obj );
	virtual int			view_remove( Ctrl* _obj );

	void		add_aoi( int _shape_type, float _raduis, float _lw_percent );
	void		search_aoi();

	void		player_enter( OBJID _player_id );
	void		player_move( OBJID _player_id, VECTOR3 _old_pos, VECTOR3 _cur_pos );
	void		player_leave( OBJID _player_id );
	void		self_move( VECTOR3 _old_pos, VECTOR3 _cur_pos );

	void		set_active(bool _active);
	bool		get_active(){ return is_active_; };

	virtual bool is_attackable_trigger() const { return is_attackable_; }
	void		set_attackable(bool _is_attackable) { is_attackable_ = _is_attackable; }

private:
	bool		check_in_aoi(OBJID _obj_id, bool is_remove = false);
	//OBJID		get_neighbor();
    void        clear_aoi_map();
private:
	MyMap32				obj_in_aoi_map_;
	std::vector<OBJID>	obj_enter_ary_;
	std::vector<OBJID>	obj_leave_ary_;

	bool		is_active_;
	int			cur_frame_count;
	bool		is_attackable_;	

	AoiTrigger* aoi_;

public:
	int c_do_set_angle_y( lua_State* _state );
	int c_replace_pos( lua_State* _state );
	int c_add_aoi( lua_State* _state );
	int c_set_active( lua_State* _state );
	int c_get_neighbors( lua_State* _state );
	int c_set_is_attackable( lua_State* _state );
	int c_set_check_dynamic( lua_State* _state );
    int c_rectangle_aoi_check( lua_State* _state );
 
public:
	static const char className[];
	static Lunar<Trigger>::RegType methods[];
	static const bool gc_delete_;
};


#define LUNAR_TRIGGER_METHODS				\
	method( Trigger, c_replace_pos ),		\
	method( Trigger, c_do_set_angle_y ),	\
	method( Trigger, c_add_aoi ),			\
	method( Trigger, c_set_active),			\
	method( Trigger, c_get_neighbors),		\
	method( Trigger, c_set_is_attackable),	\
	method( Trigger, c_set_check_dynamic ), \
	method( Trigger, c_rectangle_aoi_check )



#endif //__TRIGGER_H__
