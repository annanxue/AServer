#ifndef __PLAYER_MNG_H__
#define __PLAYER_MNG_H__

#include "buffer.h"
#include "mymap.hpp"
#include "player.h"
#include "lunar.h"
#include "world.h"
#include "vector.h"
#include "monster.h"
#include "mymap32.h"
#include "autolock.h"
#include "singleton.h"

#define FALL_NONE       1
#define FALL_RAIN       2
#define FALL_SNOW       3
#define FALL_WIND       4

#define MAX_WEATHER     4

#define BC_THREAD_NUM 4

#define BC_MSG_QUEUE_MAX_LEN 16384
#define BC_AR_TYPE_CORR 1
#define BC_AR_TYPE_STATE 2
#define BC_AR_TYPE_LOGIC_RANGE 3
#define BC_AR_TYPE_ADD_BUFF 4

class PlayerMng
{	
public:
	PlayerMng();
	~PlayerMng();
public:
	int					add_player( DPID _dpid, Player* _player );
	void				remove_player( DPID _dpid );
	int					get_count() { return map_dpid2player_.count(); }
	void				notify();

    void                add_setpos( Ctrl* _ctrl, VECTOR3& _pos );
	void 				add_block_global( const char* _buf, int _buf_size );
	void				add_transmit( Ctrl* _ctrl, const char* _buf, int _buf_size ); 

	int					c_setpos( lua_State* _L );
	int					c_broadcast_non_snapshot( lua_State* _L );
	int					c_broadcast_global_non_snapshot( lua_State* _L );
	int					c_broadcast_range( lua_State* _L );
	int					c_broadcast_global( lua_State* _L );

	int					c_broadcast_one_ar( lua_State* _L );
	int					c_broadcast_netmng_one_ar( lua_State* _L );
	int					c_broadcast_global_one_ar( lua_State* _L );
    int                 c_broadcast_world_player_one_ar( lua_State* _L );
    int                 c_broadcast_by_pos_one_ar( lua_State* _L );
    void                trace_pos_to_all_players( Ctrl* _ctrl );
    void                trace_state( Ctrl* _ctrl );

public:
	static const char className[];
	static Lunar<PlayerMng>::RegType methods[];
	static const bool gc_delete_;

    int c_add_player( lua_State* _L );
    int c_remove_player( lua_State* _L );
    int c_get_count( lua_State* _L );
    int c_set_trace_pos( lua_State* _L );
    int c_set_trace_id( lua_State* _L );
public:

	MyMap32	    map_playerid2player_;
	MyMap32     map_dpid2player_;

	char        weather_[MAX_LAND];

	Mutex	    ar_lock_;
    ulong       ar_size_[0xFFFF];
    ulong       ar_count_[0xFFFF];
    PcQueue*    bc_msg_queue_[BC_THREAD_NUM];
    int         bc_index_;
    bool        trace_pos_;
    OBJID       trace_id_;

};

#define g_playermng ( Singleton<PlayerMng>::instance_ptr() )

#define LUNAR_PLAYERMNG_METHODS													\
	method( PlayerMng, c_add_player ),											\
	method( PlayerMng, c_remove_player ),										\
	method( PlayerMng, c_get_count ),											\
	method( PlayerMng, c_set_trace_pos ),										\
	method( PlayerMng, c_set_trace_id ),	 									\
	method( PlayerMng, c_setpos ),											\
	method( PlayerMng, c_broadcast_non_snapshot ),							    \
	method( PlayerMng, c_broadcast_global_non_snapshot ),							    \
	method( PlayerMng, c_broadcast_range ),										\
	method( PlayerMng, c_broadcast_global ),                                    \
	method( PlayerMng, c_broadcast_one_ar ),                                    \
	method( PlayerMng, c_broadcast_netmng_one_ar ),                             \
	method( PlayerMng, c_broadcast_global_one_ar ),                             \
    method( PlayerMng, c_broadcast_world_player_one_ar ),                        \
    method( PlayerMng, c_broadcast_by_pos_one_ar )

#endif // __PLAYER_MNG_H__

