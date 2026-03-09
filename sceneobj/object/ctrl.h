#ifndef __CTRL_H__
#define __CTRL_H__

#include "obj.h"
#include "mymap.hpp"
#include "mymap32.h"
#include "lua_server.h"
#include "lar.h"
#include "lmisc.h"
#include "netmng.h"
#include "navmesh.h"

using namespace FF_Network;

typedef u_long			OBJID;
#define NULL_ID			(u_long)0xffffffff
#define TYPE_MOVER 	0
#define TYPE_ITEM 	1
#define TYPE_AI		2
#define NULL_POS (VECTOR3(0.0f,0.0f,0.0f))
#define MAX_LUA_MSG                 4096

#define FIX_LUA_MSG         //开启关闭lua_msg_q的实现

enum WorldLogic
{
    NOT_IN_WORLD = 0,
    TRY_ADD = 1,
    IN_WORLD = 2,
    TRY_REPLACE = 3,
};

typedef struct{
    unsigned long objid;
    int state;
    int msg;
    int p0;
    int p1;
    int p2;
    int p3;
    int p4;
    int frame;
    list_head link;
} LUA_MSG;

class Ctrl;
struct ReplaceObj
{
	Ctrl*		obj_;
	u_long		world_id_;
    int         plane_;
	VECTOR3		pos_;	
};

class Ctrl : public Obj
{
public:
	Ctrl();
	virtual ~Ctrl();

public:
    static void         set_lua_server( LuaServer* _L ) { g_lua_server = _L; }
    static LuaServer*   get_lua_server() { return g_lua_server; }
    static void         set_lua_regid( int _id ) { g_lua_regid = _id; }
    static int          get_lua_regid() { return g_lua_regid; }
    static void         set_netmng( NetMng* mng ) { g_net_mng = mng; }
    static NetMng*      get_netmng() { return g_net_mng; }
public:
    static LuaServer*   g_lua_server;
    static int          g_lua_regid;
    static NetMng*      g_net_mng;
public:
    virtual void        serialize_unchange();
    virtual void        serialize_sometimes_change();
    virtual void        serialize_always_change( bool _active );          // 主角非主角的差别只能放在肯定会改变的部分

	virtual int			setpos( VECTOR3& _pos );

    virtual void        before_add( World* _world ) {}
    virtual void        after_add( World* _world ) { set_world_logic( IN_WORLD ); }
    virtual void        before_replace( World* _old, World* _new ) { set_world_logic( IN_WORLD ); }
    virtual void        after_replace( World* _old, World* _new ) { replace_notify_ = true; }
    virtual void        before_delete( World* _world ) {}
    virtual void        after_delete( World* _world ) { set_world_logic( NOT_IN_WORLD ); }

	virtual int			view_add( Ctrl* _obj );
	virtual int			view_remove( Ctrl* _obj );

	virtual void		process_parallel() {}
	virtual void		process_serial() {}

    virtual void        safe_delete();
    void                set_world_logic( WorldLogic _logic ) { world_logic_ = _logic; }

public:
    void                serialize_lua( int _ref, bool _active );
	void		        serialize( Ar& _ar, bool _active = false );       // 只用来向客户端传数据

	void				set_world( World* _world );
	World*				get_world() const { return world_; }
	void				set_id( OBJID _id ) { ctrl_id_ = _id; }
	OBJID				get_id() const { return ctrl_id_; }
    void                set_link_pos( int _link_pos );
	int					get_link_pos() const { return link_pos_; }
    int                 get_new_link_pos() const { return new_link_pos_; }

	void				set_center_pos( int _level, int x, int z );

	int					get_center_pos_x( int _level ) const { return center_pos_[_level][0]; }
	int					get_center_pos_z( int _level ) const { return center_pos_[_level][1]; }
	void				set_obj_ary_idx( u_long _idx ) { obj_ary_idx_ = _idx; }
	u_long				get_obj_ary_idx() const { return obj_ary_idx_; }
	int					replace( u_long _scene_id, VECTOR3& _pos, int _plane = 0 );
	bool                raycast_by_range(float _range, VECTOR3 *_hit_pos, float *_hit_rate);

    bool                get_valid_pos( const VECTOR3* _pos, float _radius, VECTOR3* _result_pos );

	void		        remove_from_view();

    Ctrl*               get_iaobj();
    void                set_iaobj(Ctrl* obj) { if( iaobj_ != obj ) sometimes_change_ = true; iaobj_ = obj; }
    VECTOR3             get_iapos();
    float               get_iaangle();
    void                set_iapos( VECTOR3& pos );
    void                set_iaangle( float angle );

    void                set_pos_real( VECTOR3 _pos );

	bool				is_range_obj( Ctrl* _obj, float _range );
	bool				is_y_angle_obj( Ctrl* _obj, float _degree );
    bool                is_server_only() { return is_server_only_; }
	
	virtual bool		is_attackable_trigger() const { return false; }
    u_long              get_worldid() const { return world_id_; }
    int                 get_plane() const { return plane_; }
	bool				is_ignore_block() { return is_ignore_block_; };
	void				set_ignore_block(bool ig) { is_ignore_block_ = ig;};
    bool                get_ignore_block() { return is_ignore_block_; };
	void				set_door_flags(unsigned short door_flags = 0) { door_flags_ = door_flags; };
	bool                check_plane_for_view( const Ctrl* _obj ) const { return (plane_ == 0) || (_obj->plane_ == 0) || (plane_ == _obj->plane_); }
    bool                get_replace_notify() const { return replace_notify_; }
    void                set_replace_notify( bool _notify ) { replace_notify_ = _notify; }
    int                 plane_;

private:
    WorldLogic          world_logic_;
    bool                is_server_only_;

public:
	int c_delete( lua_State* _L );
	int c_client_delete( lua_State* _L );
	int c_get_id( lua_State* _L );
	int c_set_id( lua_State* _L );
	int c_replace( lua_State* _L );
	int c_get_worldid( lua_State* _L );
	int c_get_sceneid( lua_State* _L );
	int c_take_in_world( lua_State* _L );
	int	c_face_to( lua_State* _L );
	int	c_face_pos( lua_State* _L );
	int c_is_near_pc( lua_State* _L );
    int c_get_front_pos( lua_State* _L );
    int c_get_iaobjid( lua_State* _L);

    int c_get_pc_num( lua_State* _L );
    int c_run_always( lua_State *_L );
    int c_get_plane( lua_State *_L );
    int c_set_plane( lua_State *_L );
    int c_replace_plane( lua_State *_L );
    int c_is_y_angle_obj( lua_State* _L );
	int c_set_ignore_block( lua_State* _state );
	int c_is_ignore_block( lua_State* _state );
	int c_set_server_only( lua_State* _state );

	int c_findpath(lua_State *_L);
	bool findpath( const VECTOR3* _start_pos, const VECTOR3* _end_pos, Navmesh::Path* _path, unsigned short _door_flags = 0 );
	int c_raycast(lua_State *_L);
	bool raycast( const VECTOR3* _start_pos, const VECTOR3* _end_pos, VECTOR3* _hit_pos, float* _hit_rate, unsigned short _door_flags = 0 );
	int c_raycast_by_range(lua_State *_L);
	int c_set_door_flag(lua_State *_L);
	int c_find_pos_to_wall(lua_State *_L);
    int c_set_self_door_flag( lua_State* _state );
    int c_get_self_door_flag( lua_State* _state );
    int c_is_target_in_range( lua_State* _state );
    int c_get_valid_pos( lua_State* _state );

	static const char className[];
	static Lunar<Ctrl>::RegType methods[];
	static const bool gc_delete_;

    void to_lua( int state, int msg, int p0 = 0, int p1 = 0, int p2= 0, int p3 = 0, int p4 = 0, int frame = 0 );
    static  void init_lua_msg();
    static  LUA_MSG lua_msg[MAX_LUA_MSG];
    static  list_head lua_msg_q;
#ifdef FIX_LUA_MSG    
    static  list_head lua_msg_free_q;
#endif
    static  Mutex lua_msg_lock;

public:
    int                 pc_num_;
    Ar                  ar_;            //序列化用到的缓存
private:
	World*				world_;								/*! 所在世界*/
    unsigned long       world_id_;
	OBJID				ctrl_id_;							/*! 对象ID*/
	int					link_pos_;							/*! 在linkmap中链接的位置*/
    int                 new_link_pos_;                      /*! setpos后新linkpos*/
	int					center_pos_[DYNAMIC_LINKLEVEL][2];	/*! 当前在linkmap中每层上的视野中心*/
	u_long				obj_ary_idx_;						/*! 在世界对象队列中的下标序号*/
	bool				is_ignore_block_;					/*! 无视阻挡 */
	unsigned short		door_flags_;					
    bool                replace_notify_;                    /*! 跳转场景是否通知客户端 */

    Ctrl*               iaobj_;
    VECTOR3             iapos_;
    float               iaangel_;
};

//! 只有在这里定义过的函数在lua脚本中才可用
#define LUNAR_CTRL_METHODS				\
	method( Ctrl, c_delete ),			\
	method( Ctrl, c_client_delete ),    \
	method( Ctrl, c_replace ),			\
	method( Ctrl, c_get_id ),			\
	method( Ctrl, c_set_id ),			\
	method( Ctrl, c_get_worldid ),		\
	method( Ctrl, c_get_sceneid ),		\
	method( Ctrl, c_take_in_world ),	\
	method( Ctrl, c_face_to ),			\
	method( Ctrl, c_face_pos ),			\
	method( Ctrl, c_is_near_pc ),		\
	method( Ctrl, c_get_iaobjid ),      \
	method( Ctrl, c_get_front_pos ),    \
    method( Ctrl, c_get_pc_num ),       \
    method( Ctrl, c_run_always ),       \
    method( Ctrl, c_get_plane ),        \
    method( Ctrl, c_set_plane ),        \
    method( Ctrl, c_replace_plane ),	\
    method( Ctrl, c_is_y_angle_obj ),	\
	method( Ctrl, c_findpath ),			\
	method( Ctrl, c_raycast ),			\
	method( Ctrl, c_raycast_by_range ), \
	method( Ctrl, c_find_pos_to_wall ), \
	method( Ctrl, c_set_door_flag ),	\
	method( Ctrl, c_set_ignore_block ), \
	method( Ctrl, c_is_ignore_block ), \
	method( Ctrl, c_set_server_only ), \
    method( Ctrl, c_set_self_door_flag ),\
    method( Ctrl, c_get_self_door_flag ), \
    method( Ctrl, c_is_target_in_range ), \
    method( Ctrl, c_get_valid_pos )

#endif
