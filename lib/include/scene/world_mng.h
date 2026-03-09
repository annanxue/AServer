#ifndef __WORLD_MNG_H__
#define __WORLD_MNG_H__

#include "mymap.hpp"
#include "world.h"
#include "scene.h"

#include "lunar.h"
#include "lar.h"
#include "lmisc.h"
#include "mymap32.h"
#include "singleton.h"

#define	MAX_REMOVEWORLD	256

class Ctrl;
class Spirit;
class Player;

class WorldMng
{
public:
	WorldMng();
	~WorldMng();

	void				add_world( World* _world );
	void				remove_world( World* _world );
	World*				get_world( u_long _world_id );
	void				lock() { mutex_.Lock(); }
	void				unlock() { mutex_.Unlock(); }
	void				process();
	void				add_ctrl( Ctrl* _ctrl );
	void				remove_ctrl( Ctrl* _ctrl );
	Ctrl*				get_ctrl( u_long _ctrlid );
	Spirit*				get_spirit( u_long _ctrlid );
	Player*				get_player( u_long _ctrlid );
	int					get_obj_num(){ return ctrl_map_.count(); }
	
private:
	u_long				ctrl_index_;
	World*				remove_world_ary_[MAX_REMOVEWORLD];
	int					remove_world_cnt_;
	Mutex				mutex_;
    MyMap32             world_map_;
    MyMap32             ctrl_map_;

	void				remove();
public:
	static const char className[];
	static Lunar<WorldMng>::RegType methods[];
	static const bool gc_delete_;
public:
    int                 c_add_world( lua_State *_L);
    int                 c_remove_world( lua_State *_L);
    int                 c_in_world( lua_State *_L );
    int                 c_is_valid_sceneid( lua_State* _L );
    int                 c_get_world_player_count( lua_State *_L );
};

#define g_worldmng ( Singleton<WorldMng>::instance_ptr() )

#define LUNAR_WORLDMNG_METHODS	\
	method( WorldMng, c_add_world),    \
	method( WorldMng, c_remove_world),    \
    method( WorldMng, c_in_world ), \
    method( WorldMng, c_get_world_player_count ),\
    method( WorldMng, c_is_valid_sceneid )

#endif
