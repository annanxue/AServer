#ifndef __SCENE_OBJ_UTILS_H__
#define __SCENE_OBJ_UTILS_H__

#include <stdint.h>
extern "C"
{
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

#include "lunar.h"

int c_get_obj_arroud_with_degree( lua_State* _state ); 
int c_get_obj_arround_pos_with_degree( lua_State* _state );
int c_get_obj_around_with_offset_degree( lua_State* _state ); 
int c_get_obj_arroud_with_rect( lua_State* _state ); 
int c_get_obj_around_with_offset_rect( lua_State* _state ); 
int c_get_obj_arroud_by_pos( lua_State* _state );
int c_set_state_msg_retry_max( lua_State* _state );
int c_set_world_door_flags( lua_State* _state );	// 设置门标志接口
int c_get_world_door_flags( lua_State *_state );
int c_findpath( lua_State* _state );
int c_raycast( lua_State* _state );
int c_get_obj_in_aoi( lua_State *_state );
int c_band( lua_State *_state );
int c_bor( lua_State *_state );
int c_set_bit( lua_State* _state );
int c_clear_bit( lua_State* _state );
int c_test_bit( lua_State* _state );

int c_open_packet_stats( lua_State* _state );
int c_print_packet_stats( lua_State* _state );
int c_set_trace_search( lua_State* _state );

int c_reload_ai( lua_State* _state );

static inline int register_scene_obj_utils( lua_State* _state )
{
    lua_register( _state, "c_get_obj_arroud_with_degree", c_get_obj_arroud_with_degree ); 
    lua_register( _state, "c_get_obj_arround_pos_with_degree", c_get_obj_arround_pos_with_degree ); 
    lua_register( _state, "c_get_obj_around_with_offset_degree", c_get_obj_around_with_offset_degree ); 
    lua_register( _state, "c_get_obj_arroud_with_rect", c_get_obj_arroud_with_rect ); 
    lua_register( _state, "c_get_obj_around_with_offset_rect", c_get_obj_around_with_offset_rect ); 
    lua_register( _state, "c_get_obj_arroud_by_pos", c_get_obj_arroud_by_pos ); 
    lua_register( _state, "c_set_state_msg_retry_max", c_set_state_msg_retry_max ); 
	lua_register( _state, "c_set_world_door_flags", c_set_world_door_flags );
	lua_register( _state, "c_get_world_door_flags", c_get_world_door_flags );
	lua_register( _state, "c_findpath", c_findpath );
	lua_register( _state, "c_raycast", c_raycast );
	lua_register( _state, "c_get_obj_in_aoi", c_get_obj_in_aoi );
	lua_register( _state, "c_band", c_band );
	lua_register( _state, "c_bor", c_bor );
    lua_register( _state, "c_set_bit", c_set_bit );
    lua_register( _state, "c_clear_bit", c_clear_bit );
    lua_register( _state, "c_test_bit", c_test_bit );
	lua_register( _state, "c_open_packet_stats", c_open_packet_stats );
	lua_register( _state, "c_print_packet_stats", c_print_packet_stats );
    lua_register( _state, "c_reload_ai", c_reload_ai );
	lua_register( _state, "c_set_trace_search", c_set_trace_search );
    return 0;
}

#endif /* __SCENE_OBJ_UTILS_H__ */

