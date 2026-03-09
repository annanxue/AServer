#include <assert.h>
#include <math.h>
#include "scene.h"
#include "world_mng.h"
#include "log.h"
#include "lunar.h"
#include "lua_server.h"

Scene::Scene( u_long _id, int _multi_world, const char* _scene_name )
{
	assert( _scene_name );

	scene_id_			= _id;
	multi_world_		= _multi_world;
	world_index_		= 0;
	width_			    = 0;
	height_			    = 0;
	navmesh_ = NULL;

	strlcpy( scene_name_, _scene_name, MAX_PATH );

	if( multi_world_ )
		world_map_.init( 8, 8, "world_map_:scene.cpp" );
	else
		world_map_.init( 1, 8, "world_map_:scene.cpp" );

    min_dynamic_land_size_ = MIN_DYNAMIC_LAND_SIZE;
}


Scene::~Scene()
{
	for( int level = 0; level < STATIC_LINKLEVEL; ++level )
	{
		SAFE_DELETE_ARRAY(static_link_[level]);
	}

	SAFE_DELETE(navmesh_);
}


int Scene::init_scene( lua_State* _L )
{
    return 1;
}

bool Scene::load_lmk( char* _file )
{
    return true;
}

void Scene::init_scene_base( int _width, int _height, int _min_dynamic_land_size )
{
    width_ = _width;
    height_ = _height;

    for( int i = 2; i < 10; i++ )
    {
        int land_size = 1 << i;
        if( land_size >= _min_dynamic_land_size )
        {
            min_dynamic_land_size_ = land_size;
            break;
        }
    }
    
    if( min_dynamic_land_size_ > LANDSCAPE_SIZE )
    {
        min_dynamic_land_size_ = LANDSCAPE_SIZE;
    }

	int link_count_land = LANDSCAPE_SIZE / MIN_STATIC_LAND_SIZE;

	for( int level = 0; level < STATIC_LINKLEVEL; ++level )
	{
		int link_count_all = link_count_land * link_count_land * width_ * height_;

		static_link_[level] = new Obj*[link_count_all];
		memset( static_link_[level], 0, sizeof(Obj*) * link_count_all );

		land_width_[level] = link_count_land;
		patch_size_[level] = LANDSCAPE_SIZE / link_count_land;

		link_count_land /= 2;
	}

    /* --+ x
    |   11  12  13  14  15
    +   10  2   3   4   16
    z   9   1   0   5   17
        24  8   7   6   18
        23  22  21  20  19
    */

    range[0].x =    0;
    range[0].z =    0;

    range[1].x =    -1;
    range[1].z =    0;

    range[2].x =    -1;
    range[2].z =    -1;

    range[3].x =    0;
    range[3].z =    -1;

    range[4].x =    1;
    range[4].z =    -1;

    range[5].x =    1;
    range[5].z =    0;

    range[6].x =    1;
    range[6].z =    1;

    range[7].x =    0;
    range[7].z =    1;

    range[8].x =    -1;
    range[8].z =    1;

    range[9].x =    -2;
    range[9].z =    0;

    range[10].x =    -2;
    range[10].z =    -1;

    range[11].x =    -2;
    range[11].z =    -2;

    range[12].x =    -1;
    range[12].z =    -2;

    range[13].x =    0;
    range[13].z =    -2;

    range[14].x =    1;
    range[14].z =    -2;

    range[15].x =    2;
    range[15].z =    -2;

    range[16].x =    2;
    range[16].z =    -1;

    range[17].x =    2;
    range[17].z =    0;

    range[18].x =    2;
    range[18].z =    1;

    range[19].x =    2;
    range[19].z =    2;
    
    range[20].x =    1;
    range[20].z =    2;
    
    range[21].x =    0;
    range[21].z =    2;

    range[22].x =    -1;
    range[22].z =    2;

    range[23].x =    -2;
    range[23].z =    2;

    range[24].x =    -2;
    range[24].z =    1;

    limit.x = width_ * LANDSCAPE_SIZE / min_dynamic_land_size_; 
    limit.z = height_ * LANDSCAPE_SIZE / min_dynamic_land_size_; 

    memset( view_range_mode_, 0, sizeof(view_range_mode_) );

    //init view_range_mode_
    int i = 0;
    view_range_mode_[VIEW_RANGE_NEAR][0] = 1;

    for( i = 1; i < 9; i++ )
    {
        view_range_mode_[VIEW_RANGE_MID][i] = 1;
    }
    
    for( i = 9; i < 25; i++ )
    {
        view_range_mode_[VIEW_RANGE_FAR][i] = 1;
    }

    for( i = 0; i < 25; i++)
    {
        view_range_mode_[VIEW_RANGE_FULL][i] = 1;
    }

    for( i = 0; i < 9; i++ )
    {
        view_range_mode_[VIEW_RANGE_NEAR+VIEW_RANGE_MID][i] = view_range_mode_[VIEW_RANGE_NEAR][i] + view_range_mode_[VIEW_RANGE_MID][i];
    }

    for( i = 0; i < 25; i++ )
    {
        view_range_mode_[VIEW_RANGE_NEAR+VIEW_RANGE_FAR][i] = view_range_mode_[VIEW_RANGE_NEAR][i] + view_range_mode_[VIEW_RANGE_FAR][i];
    }

    for( i = 0; i < 25; i++ )
    {
        view_range_mode_[VIEW_RANGE_MID+VIEW_RANGE_FAR][i] = view_range_mode_[VIEW_RANGE_MID][i] + view_range_mode_[VIEW_RANGE_FAR][i];
    }
}

bool Scene::load_rsp( lua_State* _L, char* _file )
{
    return true;
}

bool Scene::load_rgn( lua_State* _L, char* _file )
{
	return true;
}

int Scene::load( lua_State* L )
{
	if( '\0' == scene_name_[0] )
	{
		ERR(2)("[SCENE](load) Scene::load() invalid scene name!id:%d", get_id());
		return -1;
	}

	if( init_scene( L ) < 0 )
	{
		ERR(2)("[SCENE](load) Scene::load() failed to init scene : %s", scene_name_);
		return -1;
	}

	return 0;
}

bool Scene::load_object_dyo()
{
	return true;  
}

int Scene::add_obj_link( Obj* _obj )
{
	assert(_obj);

	VECTOR3 pos = _obj->getpos();
	int link_level = _obj->get_linklevel();
	Obj** obj_link_map = get_obj_link(link_level);
	assert(obj_link_map);

	/*!\todo*/
	int max_width = get_land_width(link_level) * width_ ;
	int max_height = get_land_width(link_level) * height_;
	int cur_x = (int)( pos.x / get_patch_size(link_level) );
	int cur_z = (int)( pos.z / get_patch_size(link_level) );

	if( cur_x < 0 || cur_x >= max_width || cur_z < 0 || cur_z >= max_height )
	{
		ERR(2)("[SCENE](load) Scene::add_obj_link() error obj pos(%d,%d),  max_width = %d, max_height = %d ", cur_x, cur_z, max_width, max_height );
		return -1;
	}

	int idx = cur_z * max_width + cur_x;

	Obj* head_obj = obj_link_map[idx];

	if( head_obj )
	{
		head_obj->ins_next_node( _obj );
	}
	else
	{
		obj_link_map[idx] = _obj;
	}

	return 0;
}


World* Scene::add_world()
{
    u_long world_id = (world_index_ << 16) + scene_id_;	// 高16位表示world_id, 低16位:scene_id
    world_index_++;
	World* world = new World( world_id, this );
	LOG(2)( "Scene::add_world scene = %d, idx = %d, ptr = %-08X", scene_id_, world_id, (u_long)world );

	if( !world ) {
		ERR(2)("[SCENE](load) Scene::add_world() can not alloc world! scene:%d", scene_id_);
		return NULL;
	}

	world_map_.set( world_id, (intptr_t)world );
	g_worldmng->add_world( world );
	return world;
}


int Scene::remove_world( World* _world )
{
	assert(_world);
	g_worldmng->remove_world( _world );
	world_map_.del( _world->get_id() );
	return 0;
}


World* Scene::get_world( u_long world_id )
{
    World* pworld = NULL;
    if(world_id == 0) world_id = scene_id_;
    if(world_map_.find(world_id, (intptr_t&)pworld)) {
        return pworld;
    }     
    return NULL;
}

bool Scene::in_scene( float _x, float _z )
{
    if( _x >= 0 && _x < get_width() * LANDSCAPE_SIZE ) {
        if( _z >= 0 && _z < get_height() * LANDSCAPE_SIZE )
            return true;
    }
    return false;
}

bool Scene::init_navmesh( const char* _nav_path )
{
	navmesh_ = new Navmesh(_nav_path);
    return navmesh_->is_good();
}

bool Scene::findpath( const VECTOR3* _start_pos, const VECTOR3* _end_pos, Navmesh::Path* _path, const unsigned short _door_flag ) const
{
	if(navmesh_)
	{
		return navmesh_->findpath(_start_pos, _end_pos, _path, _door_flag);
	}
	return false;
}

bool Scene::raycast( const VECTOR3* _start_pos, const VECTOR3* _end_pos, VECTOR3* _hit_pos, float* _hit_rate, const unsigned short _door_flag ) const
{
	if(navmesh_)
	{
		return navmesh_->raycast(_start_pos, _end_pos, _hit_pos, _hit_rate, _door_flag);
	}
	return false;
}

bool Scene::find_pos_to_wall(const VECTOR3* _pos, float _radius, VECTOR3* _hit_pos, const unsigned short _door_flag ) const
{
	if(navmesh_)
    {
        return navmesh_->find_pos_to_wall(_pos, _radius, _hit_pos, _door_flag);
    }
    return false;
}

bool Scene::get_valid_pos( const VECTOR3* _pos, float _radius, VECTOR3* _end_pos, unsigned short _door_flag ) const
{
    if( navmesh_ )
    {
        return navmesh_->get_valid_pos( _pos, _radius, _end_pos, _door_flag );
    }
    return false;
}

