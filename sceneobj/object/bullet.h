#ifndef __BULLET_H__
#define __BULLET_H__

#include "spirit.h"
#include "chipmunk.h"

#define MAX_HIT 10
class BulletSpace
{
public:
    BulletSpace();
    ~BulletSpace();
public:
    bool ray_test( VECTOR& start, VECTOR& end, Ctrl* target );
    bool ray_test( VECTOR& start, VECTOR& end, float radius, Ctrl* target );
private:
    cpShape* cp_circle_;
private:
    Mutex mutex_;
};

class Bullet : public Spirit
{
public:
    enum PathType
    {
        SEGMENT_LINE = 1,
        PARABOLA = 2,
        FULL_PARABOLA = 3,
        TRACE_TARGET = 4
    };

	enum MoveStatus
	{
		FRONT = 0,
		STAY  = 1,
		BACK = 2
	};

    enum TraceType
    {
        MUST_HIT = 1,
        PROBABLY_HIT = 2,
        DYNAMIC_TRACE = 3,
    };

public:
    Bullet();
public:
	virtual void		process_parallel();
	virtual void		process_serial();
    virtual void        safe_delete();
    void                set_shoot_range(float _range) { shoot_range_ = _range; }
private:
    void                search_hit_player( Ctrl* _owner );
    void                search_hit_monster( Ctrl* _owner );
    void                search_hit_trigger( Ctrl* _owner );
    void                search_hit();
    void                start_emit( float _x, float _z, float _angle, float _range, float _speed );
    bool                need_search_hit_when_move();
    bool                need_process_trace_move();
private:
    float               shoot_range_;
    VECTOR              old_pos_;
    VECTOR              target_pos_;
    int                 hit_index_;
    OBJID               hit_id_[MAX_HIT];
    bool                is_emit_;
    VECTOR              start_pos_;
    float               alive_max_;
    int                 frame_count_;
    OBJID               owner_id_;
    bool                do_delete_;
    int                 search_type_;
    int                 hit_continue_;
    unsigned char       path_type_;
    OBJID               last_hit_id_;
	int					is_back_;
	int					back_to_cur_pos_;
	int					out_stealth_finish_stay_;
	int32_t				back_stay_time_;
	MoveStatus			move_status_;
	uint32_t			back_time_;
    OBJID               target_id_;

private:
    TraceType           trace_type_;
    float               trace_time_;
    float               trace_angle_;
    float               trace_radius_;
    float               trace_turn_speed_;

public:
    static BulletSpace& get_space();
public:
	static const char className[];
	static Lunar<Bullet>::RegType methods[];
	static const bool gc_delete_;
public:
    int                 c_emit_to_pos( lua_State* _state );
    int                 c_set_owner_id( lua_State* _state );
    int                 c_get_owner_id( lua_State* _state );
    int                 c_set_target_id( lua_State* _state );
    int                 c_get_target_id( lua_State* _state );
    int                 c_set_target_pos( lua_State* _state );
    int                 c_get_target_pos( lua_State* _state );
    int                 c_get_is_emit( lua_State* _state );
    int                 c_set_search_type( lua_State* _state );
    int                 c_get_search_type( lua_State* _state );
    int                 c_set_shoot_range( lua_State* _state );
    int                 c_set_hit_continue( lua_State* _state );
    int                 c_set_path_type( lua_State* _state );
    int                 c_get_path_type( lua_State* _state );
    int                 c_set_last_hit( lua_State* _state );
	int                 c_set_back_time( lua_State* _state );
    int                 c_get_move_status( lua_State* _state );
    int                 c_set_trace_type( lua_State* _state );
    int                 c_set_trace_time( lua_State* _state );
    int                 c_set_trace_params( lua_State* _state );
};

#define LUNAR_BULLET_METHODS					        \
	method( Bullet, c_emit_to_pos ),                    \
	method( Bullet, c_set_owner_id ),                   \
	method( Bullet, c_get_owner_id ),                   \
	method( Bullet, c_set_target_id ),                   \
	method( Bullet, c_get_target_id ),                   \
	method( Bullet, c_get_is_emit ),                    \
	method( Bullet, c_set_target_pos ),                 \
	method( Bullet, c_set_search_type ),                 \
	method( Bullet, c_get_search_type ),                 \
	method( Bullet, c_set_shoot_range ),                 \
	method( Bullet, c_set_hit_continue ),                 \
	method( Bullet, c_set_path_type ),                  \
    method( Bullet, c_get_path_type ),                 \
	method( Bullet, c_set_last_hit ),                  \
	method( Bullet, c_set_back_time ),                  \
	method( Bullet, c_get_target_pos ),                 \
    method( Bullet, c_get_move_status),                 \
    method( Bullet, c_set_trace_type ),                  \
    method( Bullet, c_set_trace_time ),                  \
    method( Bullet, c_set_trace_params )
#endif


