#ifndef __SPIRIT_H__
#define __SPIRIT_H__

#include "ctrl.h"
#include "state.h"
#include "nav_points.h"


#define     CORR_NONE           0x00
#define     CORR_CACHE          0x01        /*!<    Cache引发的碰撞包   */
#define     CORR_CONTROL_FLY    0x02        /*!<    飞行控制引发碰撞    */
#define     CORR_CONTROL_GROUND 0x03        /*!<    地面控制引发碰撞    */
#define     CORR_RIDE           0x04        /*!<    上下飞行器引发碰撞  */
#define     CORR_REPLACE        0x05        /*!<    场景跳转引发碰撞    */
#define     CORR_STATE          0x06        /*!<    状态机转引发碰撞    */
#define     CORR_SERVER_QUERY   0x07        /*!<    服务器查询转引发碰撞*/
#define     CORR_MAX            8

#define COLLISION_ENDURATION		SEC( 1 )
#define DEFAULT_SPEED				2.0f
#define	DEFAULT_ACC					0.0015f
#define	DEFAULT_LOOK_ANGLE			1.0f	
#define DEFAULT_TURN_ANGLE			1.0f
#define	MAX_SPEED_RATE				360
#define MAX_MSG                     10

extern int	g_frame;

class Spirit : public Ctrl
{
public:
	Spirit();
	virtual ~Spirit();

public:
	virtual void		process_parallel();
	virtual void		process_serial() {}
	virtual void 		process_region() {};

public:
	float		        get_speed() { return speed_; }
	void		        set_speed( float _speed );				
	bool				is_die() { return state.is_state(STATE_DEAD); }
    VECTOR3             getposchange() { return getpos()-pos_old_; }       
    bool                mk_pos();
    int                 get_camp() { return camp_; }
    void                set_camp( int _camp ) { camp_ = _camp; }
    bool                is_mk_pos_succ() { return mk_pos_succ_; }


    void                clear_nav_points();
    void                get_nav_points( float& _x, float& _z );
    void                add_nav_points( NavPoints* _point );
    void                pop_nav_points();
    bool                is_nav_points_empty();

public:
	VECTOR3				v_delta_;								//!< 服务器一帧内由状态机引起的位移的变化量，即速度
    VECTOR3             pos_old_;                               // 服务器端前一次mk_pos函数产生的位置
    State               state;
    bool                mk_pos_succ_;

private:
	float				speed_;
    int                 camp_;
    NavPoints           nav_points_;
    bool                stealth_;

public:
    int                 send_state_msg( int msg, int p0, int p1, int p2, int p3 );
    int                 post_state_msg( int msg, int p0, int p1, int p2, int p3 );
    bool                is_stealth() const { return stealth_; }

public:
    int                 c_is_state( lua_State *_state );
    int                 c_get_states( lua_State *_state );
    int                 c_get_states_in_post( lua_State *_state );
    int                 c_is_state_in_post( lua_State *_state );
    int                 c_send_state_msg( lua_State *_state );
    int                 c_post_state_msg( lua_State *_state );
    int                 c_send_state_fmt_msg( lua_State *_state );
    int                 c_post_state_fmt_msg( lua_State *_state );
    int                 c_del_state( lua_State *_state );
    int                 c_del_state_safe( lua_State *_state );
    int                 c_del_all_state( lua_State *_state );
    int                 c_get_param( lua_State *_state );
    int                 c_set_param( lua_State *_state );
    int                 c_can_be_state( lua_State *_state );
    int                 c_can_be_state_late( lua_State *_state );
	int                 c_set_speed( lua_State* _state );
	int                 c_get_speed( lua_State* _state );
    int                 c_serialize_state( lua_State* _state );
    int                 c_set_camp( lua_State* _state );
    int                 c_get_camp( lua_State* _state );
    int                 c_is_in_skill_unbreakable( lua_State* _state );
    int                 c_store_state_param( lua_State* _state );
    int                 c_store_state_param_from( lua_State* _state );
    int                 c_set_nav_points( lua_State* _state );
    int                 c_clear_nav_points( lua_State* _state );
    int                 c_add_nav_points( lua_State* _state );
    int                 c_pop_nav_points( lua_State* _state );
    int                 c_is_nav_points_empty( lua_State* _state );
    int                 c_is_stealth( lua_State *_state );
    int                 c_set_stealth( lua_State *_state );

public:
	static const char className[];
	static Lunar<Spirit>::RegType methods[];
	static const bool gc_delete_;

};

//! 只有在这里定义过的函数在lua脚本中才可用
#define LUNAR_SPIRIT_METHODS						\
	method( Spirit, c_set_speed ),					\
    method( Spirit, c_get_speed ),                  \
	method( Spirit, c_send_state_msg),				\
	method( Spirit, c_post_state_msg),				\
	method( Spirit, c_send_state_fmt_msg),				\
	method( Spirit, c_post_state_fmt_msg),				\
	method( Spirit, c_serialize_state),				\
	method( Spirit, c_is_state),				\
	method( Spirit, c_get_states),				\
	method( Spirit, c_get_states_in_post),				\
	method( Spirit, c_is_state_in_post),				\
	method( Spirit, c_del_state),				\
	method( Spirit, c_del_state_safe),				\
	method( Spirit, c_del_all_state),				\
	method( Spirit, c_get_param),				\
	method( Spirit, c_can_be_state),				\
	method( Spirit, c_can_be_state_late),				\
	method( Spirit, c_set_param),                    \
	method( Spirit, c_set_camp),                    \
	method( Spirit, c_get_camp),                  \
	method( Spirit, c_is_in_skill_unbreakable),         \
	method( Spirit, c_set_nav_points ),			\
	method( Spirit, c_clear_nav_points ),			\
	method( Spirit, c_add_nav_points ),			\
	method( Spirit, c_pop_nav_points ),			\
	method( Spirit, c_is_nav_points_empty ),			\
	method( Spirit, c_store_state_param_from ),			\
	method( Spirit, c_store_state_param),               \
	method( Spirit, c_is_stealth),               \
	method( Spirit, c_set_stealth)



#endif

