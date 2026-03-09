#include "player_mng.h"
#include "log.h"
#include "define_func.h"
#include "lmisc.h"
#include "scene.h"
#include "world.h"
#include "world_mng.h"
#include "lar.h"
#include "tickcount.h"
#include "resource.h"
#include "msgtype.h"
#include "msg.h"

PlayerMng::PlayerMng()
    :bc_index_(0), trace_pos_(false), trace_id_(NULL_ID)
{
	map_playerid2player_.init( 1024, 4096, "map_playerid2player_:player_mng.cpp" );
	map_dpid2player_.init( 1024, 4096, "map_dpid2player_:player_mng.cpp" );

	for ( int i = 0; i < MAX_LAND; i++ )
	{
		int weather = rand() % MAX_WEATHER + 1;
		weather_[i] = weather;
	}

    memset( ar_size_, 0, sizeof( ar_size_ ));
    memset( ar_count_, 0, sizeof( ar_count_ ));
}


PlayerMng::~PlayerMng()
{
}

int PlayerMng::add_player( DPID _dpid, Player* _player )
{
    int step = 0;
    Player* tmp = NULL;
    map_dpid2player_.lock();

    if( map_dpid2player_.find( _dpid, (intptr_t&)tmp ) == false ) {
        step++;
        map_dpid2player_.set( _dpid, (intptr_t)_player );
    }

    map_dpid2player_.unlock();

    if( step != 1 ) {
        ERR(2)( "[PLAYER_MNG](add) step = %d, playerid = %d", step, _player->get_playerid() );
        return -1;
    }

    return 0;
}


void PlayerMng::remove_player( DPID _dpid )
{
    Player* player = NULL;
    map_dpid2player_.lock();

    if( map_dpid2player_.find( _dpid, (intptr_t&)player ) ) {
        map_dpid2player_.del( _dpid );
    }

    map_dpid2player_.unlock();
}

void PlayerMng::notify()
{
    map_dpid2player_.lock();
    Node* node = map_dpid2player_.first();
    while( node ) {
        Player* player = (Player*)(node->val); 
        if( is_valid_obj( player ) ) {
            player->notify(); 
        }
        node = map_dpid2player_.next( node );
    }
	map_dpid2player_.unlock();
}

void PlayerMng::add_setpos( Ctrl* _ctrl, VECTOR3& _pos )
{
    if( !_ctrl )        return;

    Ar ar;
    SET_SNAPSHOT_TYPE( ar, ST_SETPOS );
    ar << _ctrl->get_id();
    ar << _pos.x;
    ar << _pos.z;
    BROADCAST( ar, _ctrl, NULL );
}

void PlayerMng::add_block_global( const char* _buf, int _buf_size )
{
    Node* node = map_dpid2player_.first();
    while( node ) {
        Player* player = (Player*)(node->val); 
        if( is_valid_obj( player ) ) {
			player->add_block( _buf, _buf_size ); 
        }
        node = map_dpid2player_.next( node );
    }
}

/*******************************************************************************
for lua
*******************************************************************************/

const char PlayerMng::className[] = "PlayerMng";
const bool PlayerMng::gc_delete_ = false;
	
Lunar<PlayerMng>::RegType PlayerMng::methods[] =
{
	LUNAR_PLAYERMNG_METHODS,
	{NULL, NULL}
};


int PlayerMng::c_get_count( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, get_count() );
	return 1;	
}

int PlayerMng::c_set_trace_pos( lua_State* _L )
{
	lcheck_argc( _L, 1 );
    trace_pos_ = lua_toboolean( _L, 1 );
    return 0;
}

int PlayerMng::c_set_trace_id( lua_State* _L )
{
	lcheck_argc( _L, 1 );
    trace_id_ = lua_tointeger( _L, 1 );
    return 0;
}

int PlayerMng::c_broadcast_global_one_ar( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	LAr* lar = Lunar<LAr>::check( _L, 1 );

	if ( lar == NULL || lar->ar_ == NULL )
		return 0;

    if( lar->flush_flag_ != 1 ) {
		ERR(2)( "[LUAWRAPPER](lar) %s:%d flush flag not set, this is very dangerous!!!! pls concat: sodme !!!! ", __FILE__, __LINE__ );
		return 0;
    }

	int size;
	char* buf	= lar->ar_->get_buffer( &size );

	add_block_global( buf, size );

	//lar->destroy();

	return 0;
}

int PlayerMng::c_broadcast_world_player_one_ar( lua_State* _L )
{
    lcheck_argc( _L, 3 );

    LAr* lar = Lunar<LAr>::check(_L, 1);
    OBJID objid = (OBJID)lua_tonumber(_L, 2);
    int sceneid = (int)lua_tonumber(_L, 3);

    if( lar == NULL || lar->ar_ == NULL )
        return 0;

    if( lar->flush_flag_ != 1 ) {
		ERR(2)( "[LUAWRAPPER](lar) %s:%d flush flag not set, this is very dangerous!!!! pls concat: sodme !!!! ", __FILE__, __LINE__ );
		return 0;
    }

    int size;
    char* buf = lar->ar_->get_buffer( &size );

    World *pworld = NULL;
    if( objid != 0 ){
        Ctrl* pctrl = g_worldmng->get_ctrl(objid);
        if(pctrl) {
            pworld = pctrl->get_world();
        }
    }else{
        Scene* scene = g_resmng->get_scene( sceneid );
        if(scene) pworld = scene->get_world(0);
    }

    if ( pworld ) {
        for( int landid = 0; landid < MAX_LAND; landid++ ) {
            if( pworld->vland_[landid] ){
                Node* node = pworld->vland_[landid]->first();
                while(node){
                    Player* player = (Player*)(node->val);
                    if( is_valid_obj( player) ){
                        player->add_block( buf, size );
                    }
                    node = pworld->vland_[landid]->next(node);
                }
            }
        }
    } else {
        ERR(2)("[PLAYERMNG] c_broadcast_world_player_one_ar(), world is NULL!");
    }

	//lar->destroy();

    return 0;
}

int PlayerMng::c_broadcast_by_pos_one_ar( lua_State* _L )
{
	lcheck_argc( _L, 7 );

	LAr* lar = Lunar<LAr>::check( _L, 1 );

    if ( lar == NULL || lar->ar_ == NULL )
		return 0;

    if( lar->flush_flag_ != 1 ) {
		ERR(2)( "[LUAWRAPPER](lar) %s:%d flush flag not set, this is very dangerous!!!! pls concat: sodme !!!! ", __FILE__, __LINE__ );
		return 0;
    }

    int world_id = (int)lua_tonumber( _L, 2 );
	World* world = g_worldmng->get_world( world_id );

	if( world == NULL )
        return 0;

    int plane_id = lua_tointeger( _L, 3 );

	VECTOR3 pos;
    pos.x = (int)lua_tonumber( _L, 4 );
    pos.y = (int)lua_tonumber( _L, 5 );
    pos.z = (int)lua_tonumber( _L, 6 );

    int range = (int)lua_tonumber( _L, 7 );

	int size;
	char* buf = lar->ar_->get_buffer( &size );

	FOR_LINKMAP( world, plane_id, pos, range, Obj::LinkPlayer )
		assert( PLAYER_PTR );
		PLAYER_PTR->add_block( buf, size );	
	END_LINKMAP	

	//lar->destroy();

	return 0;
}

int PlayerMng::c_setpos( lua_State* _L )
{
	lcheck_argc( _L, 3 );
	Ctrl* ctrl = Lunar<Ctrl>::nocheck( _L, 1 );
    float x = lua_tonumber( _L, 2 );
    float z = lua_tonumber( _L, 3 );

    if ( ctrl )
    {
        VECTOR3 pos( x, 0, z );
        ctrl->setpos(pos); 
        add_setpos( ctrl, pos );
    }
    return 0;
}

int PlayerMng::c_broadcast_non_snapshot( lua_State* _L )
{
	lcheck_argc( _L, 3 );
	LAr* lar = Lunar<LAr>::check( _L, 1 );
	Ctrl* ctrl = Lunar<Ctrl>::nocheck( _L, 2 );
	Ctrl* invalid_ctrl = Lunar<Ctrl>::nocheck( _L, 3 );

	if ( lar == NULL || lar->ar_ == NULL || ctrl == NULL || lar->flush_flag_ != 1 )
		return 0;

    NetMng* nm = Ctrl::get_netmng();

	int size;
	char* buf	= lar->ar_->get_buffer( &size );

	FOR_VISIBILITYRANGE( ctrl, VIEW_RANGE_FULL )
		assert( PLAYER_PTR );
		if( PLAYER_PTR != invalid_ctrl )
            nm->send_msg( buf, size, PLAYER_PTR->get_dpid() );
	NEXT_VISIBILITYRANGE( ctrl )

	return 0;
}

int PlayerMng::c_broadcast_global_non_snapshot( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	LAr* lar = Lunar<LAr>::check( _L, 1 );

	if ( lar == NULL || lar->ar_ == NULL || lar->flush_flag_ != 1 )
		return 0;

    NetMng* nm = Ctrl::get_netmng();

	int size;
	char* buf	= lar->ar_->get_buffer( &size );

    Node* node = map_dpid2player_.first();
    while( node ) {
        Player* player = (Player*)(node->val); 
        if( is_valid_obj( player ) ) {
            nm->send_msg( buf, size, player->get_dpid() );
        }
        node = map_dpid2player_.next( node );
    }

    return 0;
}

int PlayerMng::c_broadcast_range( lua_State* _L )
{
	lcheck_argc( _L, 4 );
	LAr* lar = Lunar<LAr>::check( _L, 1 );
	Ctrl* ctrl = Lunar<Ctrl>::nocheck( _L, 2 );
	Ctrl* invalid_ctrl = Lunar<Ctrl>::nocheck( _L, 3 );
    int view_range_mode = (int)lua_tonumber( _L, 4 ); 

	if ( lar == NULL || lar->ar_ == NULL || ctrl == NULL )
		return 0;

    if( view_range_mode < 1 || view_range_mode >= VIEW_RANGE_MAX )
    {
        ERR(2)( "c_broadcast_range view_range_mode: %d error.", view_range_mode );
        return 0;
    }

	int size;
	char* buf	= lar->ar_->get_buffer( &size );

	FOR_VISIBILITYRANGE( ctrl, view_range_mode )
		assert( PLAYER_PTR );
		if( PLAYER_PTR != invalid_ctrl )
			PLAYER_PTR->add_block( buf, size );	
	NEXT_VISIBILITYRANGE( ctrl )

	lar->destroy();

	return 0;
}

int PlayerMng::c_broadcast_global( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	LAr* lar = Lunar<LAr>::check( _L, 1 );

	if ( lar == NULL || lar->ar_ == NULL )
		return 0;

	int size;
	char* buf	= lar->ar_->get_buffer( &size );

	add_block_global( buf, size );

	lar->destroy();

	return 0;
}

int PlayerMng::c_broadcast_one_ar( lua_State* _L )
{
    lcheck_argc( _L, 3 );
    LAr* lar = Lunar<LAr>::check( _L, 1 );
    Ctrl* ctrl = Lunar<Ctrl>::nocheck( _L, 2 );
    Ctrl* invalid_ctrl = Lunar<Ctrl>::nocheck( _L, 3 );

    if ( lar == NULL || lar->ar_ == NULL || ctrl == NULL )
        return 0;

    if( lar->flush_flag_ != 1 ) {
        return 0;
    }

    int size;
    char* buf   = lar->ar_->get_buffer( &size );

    FOR_VISIBILITYRANGE( ctrl, VIEW_RANGE_FULL )
        assert( PLAYER_PTR );
    if( PLAYER_PTR != invalid_ctrl && PLAYER_PTR != ctrl )
        PLAYER_PTR->add_block( buf, size );
    NEXT_VISIBILITYRANGE( ctrl )

    if( ctrl->is_player() && ctrl != invalid_ctrl )
    {
        ((Player*)ctrl)->add_block( buf, size );
    }
    return 0;
}

int PlayerMng::c_broadcast_netmng_one_ar( lua_State* _L )
{
    lcheck_argc( _L, 4 );
    FF_Network::NetMng* nm = Lunar<FF_Network::NetMng>::nocheck( _L, 1 );
    LAr* lar = Lunar<LAr>::check( _L, 2 );
    Ctrl* ctrl = Lunar<Ctrl>::nocheck( _L, 3 );
    Ctrl* invalid_ctrl = Lunar<Ctrl>::nocheck( _L, 4 );

    if ( nm == NULL || lar == NULL || lar->ar_ == NULL || ctrl == NULL )
        return 0;

    if( lar->flush_flag_ != 1 ) {
        return 0;
    }

    int size;
    char* buf   = lar->ar_->get_buffer( &size );

    FOR_VISIBILITYRANGE( ctrl, VIEW_RANGE_FULL )
        assert( PLAYER_PTR );
    if( PLAYER_PTR != invalid_ctrl && PLAYER_PTR != ctrl )
        nm->send_msg( buf, size, PLAYER_PTR->get_dpid() );
    NEXT_VISIBILITYRANGE( ctrl )

    if( ctrl->is_player() && ctrl != invalid_ctrl )
    {
        nm->send_msg( buf, size, ((Player*)ctrl)->get_dpid() );
    }
    return 0;
}

int PlayerMng::c_add_player( lua_State* _L ) 
{
    lcheck_argc( _L, 2 ); 
    DPID dpid = (DPID)lua_tonumber( _L, 1 ); 
    Player* player = Lunar<Player>::check( _L, 2 ); 
    int ret = ( add_player( dpid, player ) < 0 ) ? false : true;
    lua_pushboolean( _L, ret );
    return 1;
}

int PlayerMng::c_remove_player( lua_State* _L ) 
{
    lcheck_argc( _L, 1 ); 
    DPID dpid = (DPID)lua_tonumber( _L, 1 ); 
    remove_player( dpid );
    return 0;
}

void PlayerMng::trace_pos_to_all_players( Ctrl* _ctrl )
{
    if( trace_pos_ )
    {
        VECTOR3 pos = _ctrl->getpos();
        Ar ar;
        SET_SNAPSHOT_TYPE( ar, ST_TRACE_POS );
        ar << _ctrl->get_id();
        ar << pos.x;
        ar << pos.y;
        ar << pos.z;
        ar << _ctrl->get_angle_y();
        BROADCAST( ar, _ctrl, NULL ); 
    }
}

void PlayerMng::trace_state( Ctrl* _ctrl )
{	
    if( _ctrl->get_id() == trace_id_ )
    {
        Spirit* spirit = (Spirit*)_ctrl;
        Ar ar;
        SET_SNAPSHOT_TYPE( ar, ST_TRACE_STATE );
        ar << _ctrl->get_id();
        ar << spirit->state.run_set[0];
        BROADCAST( ar, _ctrl, NULL ); 
    }
}

