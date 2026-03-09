#include "resource.h"
#include "lunar.h"
#include "log.h"

using namespace Lua;

Resource::Resource()
{
	scene_map_.init( 32, 32, "scene_map_:resource.cpp" );
}

Resource::~Resource()
{
    Scene* scene = NULL;
    Node* node = scene_map_.first();
    while( node ) {
        scene = (Scene*)(node->val);
		SAFE_DELETE( scene );
        node = scene_map_.next( node );
    }
}


int Resource::open_resource( const char* _file_name )
{
	return 0;
}

Scene* Resource::get_scene( int _scene_id ) 
{
	Scene* scene = NULL;
    scene_map_.find( _scene_id, (intptr_t&)scene );
	return scene;
}

bool Resource::load_navmesh( Scene* _scene, const char* _name )
{
    char nav_path[MAX_PATH] = "";
    snprintf( nav_path, MAX_PATH, "%s.navmesh", _name );
    if( !_scene->init_navmesh( nav_path ) )
    {
        ERR(2)( "[Resource](load_scene)load navmesh %s failed!", nav_path );
        return false;
    }
    return true;
}

bool Resource::load_scene( lua_State *_L, int _scene_id )
{
    int width = 0, height = 0;
    char name[MAX_PATH]; 
    int min_dynamic_land_size = MIN_DYNAMIC_LAND_SIZE;

    get_table_number(_L, -1, "width", width);
    get_table_number(_L, -1, "height", height);
    get_table_string(_L, -1, "name", name, sizeof(name));
    get_table_number(_L, -1, "min_dynamic_land_size", min_dynamic_land_size);

    char cwd[MAX_PATH];
    if( !getcwd( cwd, MAX_PATH ) ) {
        ERR(2)( "[Resource](load_scene)getcwd failed!" );
        return false;
    }

    if ( chdir( name ) < 0 )
    {
        ERR(2)( "[Resource](load_scene)chdir failed!" );
        return false;
    }

    Scene* scene = new Scene( _scene_id, 0, name );
    scene->init_scene_base( width, height, min_dynamic_land_size );

    // load navmesh
    if( !load_navmesh( scene, name ) )
    {
        return false;
    }

    // load ...

    // add to world
    scene_map_.set( _scene_id, (intptr_t)scene );
    scene->add_world();

    chdir( cwd );

    return true;
}

bool Resource::load_all_scenes( const char* _path )
{
    char cwd[MAX_PATH];

    if( !getcwd( cwd, MAX_PATH ) ) {
        ERR(2)( "[Resource](load_all_scenes)getcwd failed!" );
        return false;
    }

    if ( chdir( _path ) < 0 )
    {
        ERR(2)( "[Resource](load_all_scenes)chdir failed!" );
        return false;
    }

    lua_State* L = lua_open();  

    if( luaL_dofile( L, "world_config.lua" ) )
    {
        ERR(2)( "[Resource](load_all_scenes) %s:%d lua_call failed\n\t%s", __FILE__, __LINE__, lua_tostring( L, -1 ) );
		lua_pop( L, 1 );
        return false;
    }

    //if( Lua::GetTable(L, LUA_GLOBALSINDEX, "WorldMng") > 0 ) 
    if( lua_type( L, -1 ) != LUA_TTABLE )
    {
        ERR(2)("[Resource]Lua::GetTable, WorldMng error");
        return false;
    }
    WHILE_TABLE(L)
        int scene_id = (int)lua_tointeger(L, -2);  
        if( !load_scene(L, scene_id) )
        {
            ERR(2)("[Resource]load_scene %d error", scene_id);
            return false;
        }
    END_WHILE(L)
    lua_pop(L, 1); 

    chdir( cwd );

    lua_close( L );

    return true;
}

bool ResourceModule::app_class_init(){
    char res_path[APP_CFG_NAME_MAX];
    APP_GET_STRING( "Resource", res_path );
    if( !g_resmng->load_all_scenes( res_path ) )
    {
        LOG(2)( "===========Resource Module Init Failed===========");
        return false;
    }
    LOG(2)( "===========Resource Module Init ===========");
    return true;
}

bool ResourceModule::app_class_destroy(){
    LOG(2)( "===========Resource Module Destroy===========");
    return true;
}



