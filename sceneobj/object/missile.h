#ifndef __MISSILE_H__
#define __MISSILE_H__

#include "bullet.h"
#include "bezier_curve.h"

#define MAX_TARGET 10
#define PREDICT_LEAD_FRAME (ceil(200 * 0.001 * g_timermng->get_frame_rate()))

enum {
    CURVE            = 0,
    TRACK            = 1,
    STAY             = 3,
};


class MissileStage
{
public:
    int         type_;
    OBJID       missile_ctrl_id_;
    float       speed_;
    float       max_frame_;
    VECTOR3     start_pos_;
    float       start_angle_y_;
    int         start_frame_;
    float       search_angle_;
    float       search_radius_;
    float       start_search_frame_;
    float       stop_search_frame_;
    bool        has_predicted_;

public:
    virtual ~MissileStage() {}

public:
    virtual void init( lua_State* _state, OBJID _owner_id );
    virtual void start();
    virtual bool move( VECTOR3& _pos, VECTOR3& _ori ) = 0;
    virtual bool move_by_frames( VECTOR3& _pos, VECTOR3& _ori, int _frame_count );
    virtual bool is_almost_finished();
    virtual bool is_finished();
    bool should_search_target();
};


class CurveMove : public MissileStage
{
public:
    int         curve_id_;
    bool        is_finish_continue_;

public:
    CurveMove() { type_ = CURVE; }

public:
    virtual void init( lua_State* _state, OBJID _missile_ctrl_id );
    virtual bool move( VECTOR3& _pos, VECTOR3& _ori );
    virtual bool move_by_frames( VECTOR3& _pos, VECTOR3& _ori, int _frame_count );
};


class TrackMove : public MissileStage
{
public:
    float turn_speed_;

public:
    TrackMove() { type_ = TRACK; }

public:
    virtual void init( lua_State* _state, OBJID _missile_ctrl_id );
    virtual bool move( VECTOR3& _pos, VECTOR3& _ori );
    virtual bool is_finished();
    virtual bool is_almost_finished();
};


class StayMove : public MissileStage
{
public:
    float turn_speed_;

public:
    StayMove() { type_ = STAY; }

public:
    virtual void init( lua_State* _state, OBJID _missile_ctrl_id );
    virtual bool move( VECTOR3& _pos, VECTOR3& _ori );
    virtual bool move_by_frames( VECTOR3& _pos, VECTOR3& _ori, int _frame_count );
};


class Missile : public Ctrl
{
public:
    Missile();
    ~Missile();

public:
	virtual void        process_parallel();
	virtual void        process_serial();
    virtual void        safe_delete();
	virtual int			setpos( VECTOR3& _pos );

public:
    OBJID               get_owner_id()    { return owner_id_; }
    OBJID               get_target_id()   { return target_id_; }
    VECTOR3             get_ori()         { return ori_; }
    bool                is_hit_continue() { return is_hit_continue_; }

private:
    void                next_stage();
    void                predict_stage_end();
    void                move();
    void                search_target();
    void                check_valid_target( Ctrl* _target, float _search_angle, float _search_radius );
    bool                check_valid_hit( Ctrl* _obj );
    void                search_hit();
    void                search_hit_player();
    void                search_hit_monster();
    void                search_hit_trigger();
    MissileStage*       get_current_stage();
    MissileStage*       get_next_stage();
    bool                init_stages( lua_State* _state );

public:
    static BulletSpace& get_space();

private:
    OBJID               owner_id_;
    OBJID               target_id_;
    int                 apply_type_;
    VECTOR3             old_pos_;
    bool                do_delete_;
    int                 target_index_;
    OBJID               target_id_array_[MAX_TARGET];
    int                 hit_index_;
    OBJID               hit_id_array_[MAX_HIT];
    MissileStage**      stages_;
    int                 stage_count_;
    int                 stage_index_;
    VECTOR3             ori_;
    bool                is_hit_continue_;

public:
	static const char className[];
	static Lunar<Missile>::RegType methods[];
	static const bool gc_delete_;

public:
    int                 c_init( lua_State* _state );
    int                 c_set_owner_id( lua_State* _state );
    int                 c_get_owner_id( lua_State* _state );
    int                 c_fire( lua_State* _state );
    int                 c_on_hit( lua_State* _state );
    int                 c_set_target_id( lua_State* _state );
    int                 c_get_current_stage_index( lua_State* _state );
};

#define LUNAR_MISSILE_METHODS					        \
	method( Missile, c_on_hit ),                        \
	method( Missile, c_set_target_id ),                 \
	method( Missile, c_set_owner_id ),                  \
	method( Missile, c_get_owner_id ),                  \
	method( Missile, c_get_current_stage_index ),       \
	method( Missile, c_init ),                          \
	method( Missile, c_fire )                           \

#endif

