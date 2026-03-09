#ifndef __SCENE_H__
#define __SCENE_H__

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "mymap.hpp"
#include "define.h"
#include "obj.h"
#include "mymap32.h"
#include "navmesh.h"

#define VIEW_RANGE_MAX  8
#define VIEW_RANGE_FULL 0x07
#define VIEW_RANGE_FAR  0x04
#define VIEW_RANGE_MID  0x02
#define VIEW_RANGE_NEAR 0x01

#define DEFAULT_WORLD	0xFFFFFFFF

class World;

struct Range{
    int x;
    int z;
};

class Scene
{
public:
	Scene( u_long _id, int _multi_world, const char* _scene_name );
	virtual ~Scene();

	int					load( lua_State* L );
	u_long				get_id() const { return scene_id_; }
	const char*			get_scene_name() const { return scene_name_; }
	World*				add_world();
	int					remove_world( World* _world );
    World*              get_world( u_long world_id );

	int					get_world_count() { return world_map_.count(); }
	int					is_multi_world() const { return multi_world_; }
	int					get_width() const { return width_; }
	int					get_height() const { return height_; }
	Obj**				get_obj_link( int _link_level ) const { return static_link_[_link_level]; }
	int					get_land_width( int _link_level ) const { return land_width_[_link_level]; }
	int					get_patch_size( int _link_level ) const { return patch_size_[_link_level]; }

	bool				findpath(const VECTOR3* _start_pos, const VECTOR3* _end_pos, Navmesh::Path* _path, const unsigned short _door_flag = 0xffff ) const;
	bool				raycast(const VECTOR3* _start_pos, const VECTOR3* _end_pos, VECTOR3* _hit_pos, float* _hit_rate, const unsigned short _door_flag = 0xffff ) const;
    bool                find_pos_to_wall(const VECTOR3* _pos, float _radius, VECTOR3* _hit_pos, const unsigned short _door_flag = 0xffff ) const;

    bool                get_valid_pos( const VECTOR3* _pos, float _radius, VECTOR3* _result_pos, unsigned short _door_flag ) const;

    bool                in_scene( float _x, float _z );

    int                 get_min_dynamic_land_size() { return min_dynamic_land_size_; }
    void                init_scene_base( int _width, int _height, int _min_dynamic_land_size = MIN_DYNAMIC_LAND_SIZE );
	bool   				init_navmesh(const char* _nav_path);
	bool				load_rsp( lua_State* _L, char* _file );
	bool 				load_rgn( lua_State* _L, char* _file );
	bool				load_object_dyo();
    bool                load_lmk( char* _file );
public:
    int                 min_dynamic_land_size_;
    struct Range        range[25];
    struct Range        limit; 
    int                 max_width;
    unsigned char       view_range_mode_[VIEW_RANGE_MAX][25];
	Navmesh*			navmesh_;

private:
	u_long				world_index_;
	u_long				scene_id_;			/*! 场景的id*/
	int					multi_world_;
	char				scene_name_[MAX_PATH];
	int					width_;
	int					height_;

	Obj**				static_link_[STATIC_LINKLEVEL];
	int					land_width_[STATIC_LINKLEVEL];
	int					patch_size_[STATIC_LINKLEVEL];
    MyMap32             world_map_;/*! 由该场景生成的world列表*/

	int					init_scene( lua_State* L );
	int					add_obj_link( Obj* _obj );
};

#endif //__WORLD_MNG_H__
