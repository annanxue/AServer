#ifndef __WORLD_H__
#define __WORLD_H__

#include <stack>
#include <map>
#include "thread.h"
#include "scene.h"
#include "ctrl.h"
#include "define.h"
#include "event.h"
#include "cpcqueue.h"
#include "mylist.h"

#define MAX_LAND 100

class Ctrl;
class World;
struct ReplaceObj;

struct NodeObj
{
    list_head   link;
    int         idx;
};

class WorkThread : public Thread
{
public:
	WorkThread(){ thread_name_ = "WorldThread"; }
	~WorkThread() { }
	void				run();
};

struct WorldProcessTask
{
	World*		world_;
	int			task_id_;
	int			start_;
	int			end_;
};


class World
{
public:
	World( u_long _id, Scene* _scene );
	~World();

	u_long				get_id() const { return world_id_; }
	Scene*				get_scene() const { return scene_; }
	int					get_player_count() const { return player_count_; }
	int					is_delete() const { return delete_; }
	Ctrl**				get_obj_link( int _link_type, int _link_level );
	int					get_land_width( int _link_level ) const { return land_width_[_link_level]; }
	int					get_patch_size( int _link_level ) const { return patch_size_[_link_level]; }
	int					add_obj( Ctrl* _obj );
	int					delete_obj( Ctrl* _obj );
    int                 get_visible_range( int level ) { return visibility_range_[level];}
	
	int					process();

	int					in_world( const VECTOR3& pos ) const { return scene_->in_scene(pos.x, pos.z); }
	int					in_world( float x, float z ) const { return scene_->in_scene(x, z); }

private:
	u_long				world_id_;	/*! 世界id*/
	Scene*				scene_;		/*! 对应场景*/
	int					player_count_;
	int					delete_;
	/*! todo这里暂时采用静态数组,不过主场景world和工会场景world的数组
		长度肯定是应该不相同的,否则工会场景world会太消耗内存 */
	Ctrl**				add_ary_;
	int					max_addobjs_;
	int					add_cnt_;
	Ctrl**				obj_ary_;
	int					max_dynamicobjs_;
	int					obj_cnt_;
	Mutex				modify_mutex_;
	Ctrl**				modify_ary_;
	int					max_modifylinkobjs_;
	int					modify_cnt_;
	Mutex				replace_mutex_;
	ReplaceObj*			replace_ary_;
	int					max_replaceobjs_;
	int					replace_cnt_;
	Ctrl**				delete_ary_;
	int					max_deleteobjs_;
	int					delete_cnt_;
	static int			world_count_;
	static int			process_num_;	/* 处理线程数量*/
	static Event*		process_event_;
	static PcQueue*		process_task_;
	static WorkThread*	work_thread_list_;
	static WorldProcessTask* work_task_list_;

	friend class Ctrl;
	friend class Player;
    friend class WorkThread;


	//std::stack<int>		obj_idx_stack_;
    list_head           idx_list_;
    NodeObj             idx_node_[10240];

	Ctrl**				obj_link_[DYNAMIC_LINKTYPE][DYNAMIC_LINKLEVEL];
    int                 obj_link_count_[DYNAMIC_LINKTYPE][DYNAMIC_LINKLEVEL];
	int					visibility_range_[DYNAMIC_LINKLEVEL];	
	int					land_width_[DYNAMIC_LINKLEVEL];
	int					patch_size_[DYNAMIC_LINKLEVEL];
	int					unused_timer_;

	int					init_world();
	void				destroy_world();
	void				_add();
	void				_process();
	void				_modifylink();
	void				_replace();
	void				_delete();

	static void			create_work_thread( int _process_num );
	static void			close_work_thread();

	int					add_to_view( Ctrl* _obj, bool _is_refresh = false );
	int					modify_view( Ctrl* _obj );
	int					modify_view_level_0( Ctrl* _obj );
    int                 remove_view( Ctrl* _obj );

	int					add_obj_ary( Ctrl* _obj );
	int					remove_obj_ary( Ctrl* _obj );
	int					add_obj_link( Ctrl* _obj );
	int					remove_obj_link( Ctrl* _obj );
	int					remove_obj_link2( Ctrl* _obj );

    bool                is_valid_obj_link_count( int _type, int _level, int _link_pos );

public:
    bool                try_add_obj_link( Ctrl* _obj );
    bool                is_view( int _level1, int _idx1, int _level2, int _idx2 );


public:
    MyMap32*    vland_[MAX_LAND];     // map array, map for playerid --> player_ptr, one land one map, include land 0

	// plane的门标志map管理
private:
	MyMap32 door_flags_map_;
public:
	void set_door_flags(unsigned short _flag = 0xffff, unsigned int _plane_id = 0)
	{
		unsigned short door_flags = _flag;

		// rightest bit can't be 0
		door_flags = door_flags | 0x01;
		door_flags_map_.set(_plane_id, (int)door_flags);
	}

	unsigned short get_door_flags(unsigned int _plane_id = 0)
	{
		intptr_t door_flags = 0;
		if (door_flags_map_.find(_plane_id, (intptr_t &)door_flags))
		{
			return (unsigned short)door_flags;
		}

		return 0xffff;
	}
};


#endif
