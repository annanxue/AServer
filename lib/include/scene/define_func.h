#ifndef __DEFINE_FUNC_H__
#define __DEFINE_FUNC_H__

#include "world.h"

/*! just for level 0 */
#define FOR_LINKMAP( _world, _plane, _pos, _range, _type )                                              \
{                                                                                                       \
    Scene* scene = _world->get_scene();                                                                 \
	assert( scene );																				    \
    int max_width  = scene->get_width() * _world->get_land_width(0); 		                            \
    int max_height = scene->get_height() * _world->get_land_width(0);                                  	\
    int patch_size = _world->get_patch_size( 0 );	    	                                            \
    int min_x = (int)( ( _pos.x - _range ) / patch_size );                                              \
    int max_x = (int)( ( _pos.x + _range ) / patch_size );                                              \
    int min_z = (int)( ( _pos.z - _range ) / patch_size );                                              \
    int max_z = (int)( ( _pos.z + _range ) / patch_size );                                              \
    min_x = min_x < 0 ? 0 : min_x;                                                                      \
    min_z = min_z < 0 ? 0 : min_z;                                                                      \
    max_x = max_x >= max_width  ? max_width  -1 : max_x;                                                \
    max_z = max_z >= max_height ? max_height -1 : max_z;                                                \
    int i, j;                                                                                           \
    Ctrl** obj_link_map = _world->get_obj_link( _type, 0 );                                             \
    for( i = min_x; i <= max_x; ++i )                                                                   \
    {                                                                                                   \
        for( j = min_z; j <= max_z; ++j)                                                                \
        {                                                                                               \
            Ctrl* obj = obj_link_map[ j * max_width + i ];                                              \
            while( obj ) {                                                                              \
                if( ( _plane == 0 || obj->plane_ == 0 || obj->plane_ == _plane ) && !obj->is_ignore_search() ) {

#define END_LINKMAP                                                                                     \
                }                                                                                       \
                obj = ( Ctrl* )obj->get_next_node();                                                             \
            }                                                                                           \
        }                                                                                               \
    }                                                                                                   \
}   

/*! just for level 0 */
#define FOR_LINKMAP_VIEW( _world, _plane, _pos, _type, _range_mode )                                    \
{                                                                                                       \
    Scene* scene = _world->get_scene();                                                                 \
	assert( scene );																				    \
    /*int max_width  = scene->get_width() * _world->get_land_width(0); 		                            \
    int max_height = scene->get_height() * _world->get_land_width(0); */                                 	\
    int patch_size = _world->get_patch_size( 0 );	    	                                            \
    int _cur_x_ = (int)( _pos.x / patch_size );                                                           \
    int _cur_z_ = (int)( _pos.z / patch_size );                                                           \
    int _k_ = 0;                                                                                          \
    for( _k_=0; _k_<25; ++_k_ ) {                                                                             \
        if( scene->view_range_mode_[_range_mode][_k_] ) {                                                 \
            int _x_ = _cur_x_ + scene->range[_k_].x;                                                          \
            int _z_ = _cur_z_ + scene->range[_k_].z;                                                          \
            if( _x_ >= 0 && _x_ < scene->limit.x && _z_ >= 0 && _z_ < scene->limit.z ) {                        \
                int _idx_ = _x_ + _z_ * scene->limit.x;                                                       \
                Ctrl** _map_ = world->get_obj_link( _type, 0 );                                           \
                Ctrl* obj = _map_[ _idx_ ];                                                                 \
                while( obj ) {                                                                          \
                    if( ( _plane == 0 || obj->plane_ == 0 || obj->plane_ == _plane) && !obj->is_ignore_search() ) {

#define END_LINKMAP_VIEW                                                                                \
                    }                                                                                   \
                    obj = ( Ctrl* )obj->get_next_node();                                                             \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
    }                                                                                                   \
}

#define FOR_VISIBILITYRANGE( ctrl, range_mode )                                                         \
{                                                                                                       \
    World* world = ctrl->get_world();                                                                   \
    if( world ) {                                                                                       \
        Scene* scene = world->get_scene();                                                              \
        int k = 0;                                                                                      \
        for( k=0; k<25; ++k ) {                                                                         \
            if( scene->view_range_mode_[range_mode][k] ) {                                              \
                int x = ctrl->x_ + scene->range[k].x;                                                   \
                int z = ctrl->z_ + scene->range[k].z;                                                   \
                if( x >= 0 && x < scene->limit.x && z >= 0 && z < scene->limit.z ) {                    \
                    int idx = x + z * scene->limit.x;                                                   \
                    Ctrl** map = world->get_obj_link( Obj::LinkPlayer, 0 );                             \
                    Obj* obj = map[ idx ];                                                              \
                    while( obj ) {                                                                      \
                        if( obj->is_ctrl() && ( ctrl->plane_ == 0 || (( Ctrl* )obj)->plane_ == 0 || (( Ctrl* )obj )->plane_ == ctrl->plane_) && !obj->is_ignore_search() ) {

#define PLAYER_PTR                                                                                      \
                        ((Player*)obj)

#define	NEXT_VISIBILITYRANGE( ctrl )                                                                    \
                        }                                                                               \
                        obj = obj->get_next_node();                                                     \
                    }                                                                                   \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
    }                                                                                                   \
}

#define	GETBLOCK( ar, buf, size )																		\
	int size;																							\
	char* buf	= ar.get_buffer( &size );

#define SET_SNAPSHOT_TYPE( ar, type )                                                                   \
{                                                                                                       \
    ar << (unsigned char)type;                                                                      \
}

#define BROADCAST( ar, _ctrl, _invalid_ctrl )															\
	GETBLOCK( ar, buf, size );																			\
	FOR_VISIBILITYRANGE( _ctrl, VIEW_RANGE_FULL )														\
		assert( PLAYER_PTR );																			\
		if( PLAYER_PTR != _invalid_ctrl )																\
			PLAYER_PTR->add_block( buf, size );															\
	NEXT_VISIBILITYRANGE( _ctrl )



#define	CHECK_RANGE( _line, _player, _pos )															    \
	if( vc3_len( _pos -_player->getpos() ) > VALID_RANGE )												\
	{ \
        VECTOR3 shitpos = _player->getpos(); \
        TRACE(2)( "CHECK_RANGE %d,  %4.4f %4.4f -> %4.4f, %4.4f, %d", _line, shitpos.x, shitpos.z, _pos.x, _pos.z, _player->get_playerid() );\
	}

#define AR_TRACE_BEGIN(type)                                                                            \
    unsigned short ar_type = type;                                                                      \
    snapshot_.last_send_packet_type_ = type;                                                            \
    int ar_len1 = 0;                                                                                    \
    snapshot_.ar_.get_buffer( &ar_len1 );

#define AR_TRACE_END()                                                                                  \
    int ar_len2 = 0;                                                                                    \
    snapshot_.ar_.get_buffer( &ar_len2 );                                                               \
    snapshot_.last_send_packet_length_ = ar_len2-ar_len1;                                               \
    Ctrl::get_netmng()->mark_snapshot_stats( ar_type, ar_len2-ar_len1 );                                \


extern unsigned long long g_tick_move;
extern unsigned long long g_tick_total_move;
extern unsigned long long g_tick_process;
extern unsigned long long g_tick_proc;
extern unsigned long long g_tick_ai;
extern unsigned long long g_tick_serial;



#endif //__DEFINE_FUNC_H__


