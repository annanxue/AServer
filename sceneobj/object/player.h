#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "autolock.h"
#include "spirit.h"
#include "lar.h"
#include "snapshot.h"
#include "monster.h"


#define MAX_WAITING_QUERY	512

#define MAX_CHANNEL         32
#define CHANNEL_POS         0
#define CHANNEL_FRIEND      1
#define MAX_WORLD_BITSET    ( 256 / 32 )
#define MAX_IN_REGION       8

class Player : public Spirit
{
public:
	Player();
	virtual ~Player();
public:
    void                serialize_active( Ar* _ar );

	virtual void		process_parallel();
	virtual void		process_serial();
	virtual void		process_region();

	void				set_dpid( DPID _dpid ) { dpid_ = _dpid; }
	DPID				get_dpid(void) const { return dpid_; }
	u_long				get_playerid() const { return player_id_; }

	virtual int			view_add( Ctrl* _obj );
	virtual int			view_remove( Ctrl* _obj );

    virtual void        before_replace( World* _old, World* _new );
    virtual void        after_replace( World* _old, World* _new );
    virtual void        after_add( World* world );

    void                check_ar();
	void				notify();
	void				add_block( const char* buf, int len );

	void				add_addobj( Ctrl* _obj );
	void				add_removeobj( Ctrl* _obj );
	void				add_replace( u_long _scene_id, VECTOR3& _pos );
    void                add_setpos( VECTOR3& _pos );
	
	void				add_spirit_pos( OBJID _objid, const VECTOR3 & _pos, float _angle );



public:
    static int          view_add_num_;
    static int          view_rm_num_;
    static int          view_repeat_num_;

protected:
	u_long				player_id_;
	DPID				dpid_;
    Snapshot            snapshot_;

    int                 dest_tele_plane_;
    
public:
	int	c_set_dpid( lua_State* _L );
	int	c_get_dpid( lua_State* _L );
	int c_get_id( lua_State* _L );
	int	c_get_player_id( lua_State* _L );
	int	c_set_player_id( lua_State* _L );
	int	c_add_block( lua_State* _L );
	int	c_add_block_one_ar( lua_State* _L );
	int	c_serialize( lua_State* _L );
	
	int c_replace_pos( lua_State* _L );

    int c_set_dest_tele_plane( lua_State* _L );
    int c_get_dest_tele_plane( lua_State* _L );

    int c_refresh_view( lua_State* _L );
	static const char className[];
	static Lunar<Player>::RegType methods[];
	static const bool gc_delete_;

};


//! 只有在这里定义过的函数在lua脚本中才可用
#define LUNAR_PLAYER_METHODS					\
	method( Player, c_set_dpid ),				\
	method( Player, c_get_dpid ),				\
	method( Player, c_get_id ),					\
	method( Player, c_get_player_id ),			\
	method( Player, c_set_player_id ),			\
	method( Player, c_add_block ),				\
	method( Player, c_add_block_one_ar ),		\
	method( Player, c_serialize ),				\
	method( Player, c_replace_pos ),			\
    method( Player, c_set_dest_tele_plane ),    \
    method( Player, c_get_dest_tele_plane ),    \
    method( Player, c_refresh_view )

#endif //__PLAYER_H__




