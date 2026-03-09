#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "mymap32.h"
#include "ctrl.h"
#include "scene.h"
#include "app_base.h"
#include "singleton.h"

class Resource
{
public:
	Resource();
	~Resource();
public:
	int					open_resource( const char* _file_name );
	Scene*				get_scene( int _scene_id );
private:
    bool                load_navmesh( Scene* _scene, const char* _path );
public:
    bool                load_scene( lua_State* _L, int _scene_id );
    bool                load_all_scenes( const char* _path );
private:
	MyMap32             scene_map_;
};

class ResourceModule : public AppClassInterface
{
    public:
        bool app_class_init();    
        bool app_class_destroy(); 
};

#define g_resmng ( Singleton<Resource>::instance_ptr() )

#endif
