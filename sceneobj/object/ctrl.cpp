#include <assert.h>
#include "ctrl.h"
#include "world.h"
#include "player.h"
#include "player_mng.h"
#include "world_mng.h"
#include "resource.h"
#include "log.h"
#include "llog.h"

LuaServer* Ctrl::g_lua_server = NULL;
int Ctrl::g_lua_regid = -1;
NetMng* Ctrl::g_net_mng = NULL;

list_head Ctrl::lua_msg_q;
#ifdef FIX_LUA_MSG
list_head Ctrl::lua_msg_free_q;
#endif
Mutex Ctrl::lua_msg_lock;
LUA_MSG Ctrl::lua_msg[4096];

Ctrl::Ctrl()
{
	world_			= NULL;
    world_id_       = 0;
	obj_ary_idx_	= 0xFFFFFFFF;
	link_pos_		= -1;
    new_link_pos_   = -1;
	for(int i = 0; i < DYNAMIC_LINKLEVEL; ++i)
	{
		center_pos_[i][0] = -1;
		center_pos_[i][1] = -1;
	}
	settype( OT_CTRL );
    pc_num_ = 0;

	g_worldmng->add_ctrl( this );

    iaobj_ = NULL;
    plane_ = 0;
	is_ignore_block_ = false;
	door_flags_ = 0;
    replace_notify_ = true;

    set_world_logic( NOT_IN_WORLD );
    is_server_only_ = false;
}


Ctrl::~Ctrl()
{
	g_worldmng->remove_ctrl( this );

    if( get_prev_node() || get_next_node() )
    {
	    del_node();
        ERR(2)( "[CHECKBUG]Ctrl id: %d index: %d is not deleted by World::_delete()!!!!!!!!", get_id(), get_index() );
    }
}

void Ctrl::serialize( Ar& _ar, bool _active )
{
#ifdef DEBUG
    TICK( A );
#endif
    if( _active || data_ver_ != g_frame ) {
        // 序列化不变的部分，一次性
        if( first_time_ ) {
            ar_.flush();
            serialize_unchange();
            first_time_ = false;
            sometimes_change_offset_ = ar_.get_offset();
        }

        // 序列化有时会改变的部分
        if( sometimes_change_ ) {
            ar_.reel_in( sometimes_change_offset_ );
            serialize_sometimes_change();
            sometimes_change_ = false;
            always_change_offset_ = ar_.get_offset();
        }

        // 序列化必然会改变的部分
        ar_.reel_in( always_change_offset_ );
        serialize_always_change( _active );

        // 如果是主角自己序列化则不更新数据版本
        if( !_active )
            data_ver_ = g_frame;
    }

    int len = 0;
    char* buf = ar_.get_buffer( &len );
    _ar.write( buf, len );

    // 玩家角色需要区别是否是主角
    if( is_player() ) {
        _ar << ( char )_active;

        // 主角序列化特殊处理
        // 主角用传进来的ar而不是自己的ar是为了避免让主角序列化包把自己的ar撑大浪费内存
        if( _active )
            ( ( Player* )this )->serialize_active( &_ar );
    }

#ifdef DEBUG
    TICK( B );

    int st = 0;
    if( is_player() )
        st = S_PLAYER;
    else if( is_pet() )
        st = S_PET;
    else if( is_monster() )
        st = S_MONSTER;
    else if( is_common() )
        st = S_COMMON;
    else if( is_item() )
        st = S_ITEM;
    SerializeStat& ss = serialize_stats_[st];
    ss.count_++;
    ss.time_ += B-A;
#endif
}

void Ctrl::serialize_lua( int _ref, bool _active )
{
}

void Ctrl::serialize_unchange()
{
    Obj::serialize_unchange_ar( ar_ );

    ar_ << ctrl_id_;
}

void Ctrl::serialize_sometimes_change()
{
    Obj::serialize_sometimes_change_ar( ar_ );

    if( is_valid_obj( iaobj_ ) )
        ar_ << iaobj_->get_id();
    else
        ar_ << NULL_ID;
}

void Ctrl::serialize_always_change( bool _active )
{
    Obj::serialize_always_change_ar( ar_, _active );
}

int Ctrl::replace( u_long _worldid, VECTOR3& _pos, int _plane )
{
    World *world_replace = g_worldmng->get_world(_worldid);

	if( !world_replace )
	{
		ERR(2)("[OBJ](ctrl) Ctrl::replace() can not get world in world:%d! obj:%d", _worldid, get_id());
		return FALSE;
	}

	if( !world_replace->in_world(_pos) )
	{
		ERR(2)("[OBJ](ctrl) Ctrl::replace() pos(%f,%f,%f) wanna replace is not in world:%d! obj:%d", _pos.x, _pos.y, _pos.z, world_replace->get_id(), get_id());
		return FALSE;
	}

    World *world = get_world();
    if( !world ) {
		ERR(2)("[OBJ](ctrl) Ctrl::replace() ctrl is not in world! obj:%d", get_id());
		return FALSE;
    }

	ReplaceObj* replace_obj = NULL;
	AutoLock Lock( &world->replace_mutex_ );
	/*! 已跳转则不再添加*/
	for( int i = 0; i < world->replace_cnt_; ++i )
	{
		if( world->replace_ary_[i].obj_ == this )
		{
			replace_obj = &world->replace_ary_[i];
			break;
		}
	}

	if( replace_obj == NULL )
	{
		if( world->replace_cnt_ >= world->max_replaceobjs_ )
		{
			ERR(2)("[OBJ](ctrl) Ctrl::replace() replace count overflowed. Max Replace obj=%d.", world->max_replaceobjs_ );
			return FALSE;
		}

		replace_obj = &world->replace_ary_[world->replace_cnt_++];
		replace_obj->obj_ = this;
        
        if( get_iaobj() )
        {
            set_iaobj( NULL );
        }
	}

	replace_obj->world_id_ = world_replace->get_id();
    replace_obj->plane_ = _plane;
	replace_obj->pos_ = _pos;

    set_world_logic( TRY_REPLACE );

	return TRUE;
}

bool Ctrl::raycast_by_range(float _range, VECTOR3 *_hit_pos, float *_hit_rate)
{
    VECTOR3 start_pos = getpos();
    VECTOR3 end_pos = start_pos + GET_DIR(get_angle_y()) * _range;
    //unsigned short door_flags = world_->get_door_flags(plane_);
    return raycast(&start_pos, &end_pos, _hit_pos, _hit_rate);  //world_->get_scene()->raycast(&start_pos, &end_pos, _hit_pos, _hit_rate, door_flags);
}

void Ctrl::set_world( World* _world ) 
{ 
    world_ = _world; 
    if (_world) world_id_ = _world->get_id(); 
}
	

int Ctrl::setpos( VECTOR3& _pos )
{
	VECTOR3 pos = getpos();
	if( pos == _pos ) return TRUE;
	World* world = get_world();

    if( !world ) 
    {
        /*我们可以在对象没有world的时候设置位置，比如玩家还没加入场景的时候，这个是正常情况*/
	    /*! Obj层设置实际位置,ctrl层只负责位置变化的视野处理*/
	    Obj::setpos( _pos );
        return TRUE;
    }

    /*正在跳转场景的时候不可以设置位置，会引发逻辑错误*/
    if( world_logic_ == TRY_ADD )
    {
        TRACE(2)( "[OBJ](ctrl) Ctrl::setpos()obj: %d try to setpos when adding to world", ctrl_id_ );
        return FALSE;
    }

    if( world_logic_ == TRY_REPLACE )
    {
        TRACE(2)( "[OBJ](ctrl) Ctrl::setpos()obj: %d try to setpos when replacing between worlds", ctrl_id_ );
        return FALSE;
    }

    if( !world->in_world( _pos ) ) 
    {
        TRACE(2)("[OBJ](ctrl) Ctrl::setpos() pos(%f,%f,%f) not in world:0x%08X, obj:%d", _pos.x, _pos.y, _pos.z, world->get_id(), get_id());
        return FALSE;
    }

    int link_level = get_linklevel();
    int max_width = world->get_land_width( link_level ) * world->get_scene()->get_width();
    int max_height = world->get_land_width( link_level ) * world->get_scene()->get_height();
    int cur_idx = (int)( _pos.z / world->get_patch_size(link_level) ) * max_width + (int)( _pos.x / world->get_patch_size(link_level) );
    int link_idx = get_link_pos();

    if( link_idx < 0 || link_idx >= max_width * max_height ) 
    {
        ERR(2)("[OBJ](ctrl) Ctrl::setpos() error idx : %d", link_idx);
        return FALSE;
    }

    /*! 逻辑格发生变化*/
    if( cur_idx != link_idx ) 
    {
        /*! 已设置位置则不再添加到队列*/
        if( new_link_pos_ == -1 ) 
        {
            AutoLock Lock( &world->modify_mutex_ );
            if( world->modify_cnt_ >= world->max_modifylinkobjs_ ) 
            {
                ERR(2)("[OBJ](ctrl) Ctrl::setpos() modify count overflowed.");
                return FALSE;
            }
            world->modify_ary_[world->modify_cnt_++] = this;
        }
        new_link_pos_ = cur_idx;
    } 
    else /*! 逻辑格没有发生变化*/ 
    {
        /*! 如果已经设置过位置,则从队列中清除掉*/
        if( new_link_pos_ != -1 ) 
        {
            AutoLock Lock( &world->modify_mutex_ );
            for( int i = 0; i < world->modify_cnt_; ++i ) {
                if( world->modify_ary_[i] == this ) {
                    memmove( (void*)&world->modify_ary_[i], (void*)&world->modify_ary_[i+1], sizeof(Obj*) * ( world->modify_cnt_-i-1 ) );
                    world->modify_cnt_--;
                    break;
                }
            }
            new_link_pos_ = -1;
        }
    }
	/*! Obj层设置实际位置,ctrl层只负责位置变化的视野处理*/
	return Obj::setpos( _pos );
}

void Ctrl::set_pos_real( VECTOR3 _pos )
{
    Obj::setpos( _pos );
    return;
}

/*! 这里只会通知别的玩家把自己移除掉,而不会通知自己把别的玩家移除掉
	这和添加时互相通知是不一样的. */
void Ctrl::remove_from_view()
{
    World* world = get_world();
    if( world )
    {
        Scene* scene = world->get_scene();
        int k = 0;
        for( k=0; k<25; ++k ) {
            int x = x_ + scene->range[k].x;
            int z = z_ + scene->range[k].z;
            if( x >= 0 && x < scene->limit.x && z >= 0 && z < scene->limit.z ) {
                int idx = x + z * scene->limit.x;
                Ctrl** map = world->get_obj_link( Obj::LinkPlayer, 0 );
                Obj* obj = map[ idx ];
                while( obj ) { 
                    if( obj != this && check_plane_for_view( (Ctrl*)obj) ) {
                        ((Player*)obj)->view_remove( this );
                    }
                    obj = obj->get_next_node();
                }
                map = world->get_obj_link( Obj::LinkDynamic, 0 );
                obj = map[ idx ];
                while( obj ) { 
                    if( check_plane_for_view( (Ctrl*)obj) ) {
                        ((Ctrl*)obj)->view_remove( this );
                    }
                    obj = obj->get_next_node();
                }
            }
        }
    }
}


int Ctrl::view_add( Ctrl* _obj )
{
	assert(_obj);
    if( _obj->is_player() || _obj->is_robot() ){
        pc_num_++;

        //LOG(1)( "ADD %d + %d , num = %d", ctrl_id_, _obj->ctrl_id_, pc_num_ );
        return 1;
    }
	return 0;
}


int Ctrl::view_remove( Ctrl* _obj )
{
	assert(_obj);
    if( _obj->is_player() || _obj->is_robot() ) {
        pc_num_--;        

        //LOG(1)( "REM %d - %d , num = %d ", ctrl_id_, _obj->ctrl_id_, pc_num_ );
        return 1;
    }
	return 0;
}

void Ctrl::safe_delete()
{
	World* world = get_world();
	if( world )
	{	
		world->delete_obj( this );
	}
    else
    {
        ERR(2)( "[OBJ](ctrl) %s:%d delete a ctrl:%d not in world", __FILE__, __LINE__, get_id() );
        delete this;
    }
}

Ctrl* Ctrl::get_iaobj()
{
    return iaobj_;
}

VECTOR3 Ctrl::get_iapos()
{
    if(get_iaobj()) {
        return iapos_;
    } else {
        return getpos();
    }
}

float Ctrl::get_iaangle()
{
    if(get_iaobj()) {
        return iaangel_;
    } else {
        return get_angle_y();
    }
}

void Ctrl::set_iapos( VECTOR3& _iapos )
{
    if(get_iaobj()) {
        iapos_ = _iapos;
    } else {
        setpos(_iapos); 
    }
}

void Ctrl::set_iaangle( float _iaangle )
{
    if(get_iaobj()) {
        iaangel_ = _iaangle;
        set_angle_y(_iaangle - get_iaobj()->get_angle_y());
    } else {
        set_angle_y(_iaangle);
    }
}

void Ctrl::set_link_pos( int _link_pos ) 
{ 
    link_pos_ = _link_pos; 
    new_link_pos_ = -1; 
}

void Ctrl::set_center_pos( int _level, int x, int z ) 
{ 
    center_pos_[_level][0] = x; 
    center_pos_[_level][1] = z; 
    if( _level == 0 ) {
        x_ = x; z_ = z; 
    }
}

bool Ctrl::is_range_obj( Ctrl* _obj, float _range )
{
    if( world_ != _obj->get_world() ) 
    {
        return false;
    }
        
    if( plane_ != 0 && _obj->plane_ != 0 && plane_ != _obj->plane_ ) 
    {
        return false;
    }

	VECTOR3 dist = getpos() - _obj->getpos();
	//float len = get_radius() + _obj->get_radius() + _range ;
	float len = _obj->get_radius() + _range ;
    //ERR(2)(" ------------ range, x: %f, y: %f, z: %f, len: %f ---------------", dist.x, dist.y, dist.z, len );
    if( fabsf( dist.x ) <= len && fabsf( dist.y ) <= len && fabsf( dist.z ) <= len ) 
    {
	    return  ( dist.x * dist.x + dist.z * dist.z + dist.y * dist.y < ( len * len  ) );
    }
    
    return false;
}

/**
 * is in [y_angle - _degree/2,  y_angle + _degree/2]
 */
bool Ctrl::is_y_angle_obj( Ctrl* _obj, float _degree )
{
    if( world_ != _obj->get_world() )
    {
        return false;
    }

    if( plane_ != 0 && _obj->plane_ != 0 && plane_ != _obj->plane_ ) 
    {
        return false;
    }

    float dst_y_angle = d3d_to_degree( vc3_get_angle( _obj->getpos() - getpos() ) ) ;
    float diff_y_angle = dst_y_angle - get_angle_y() ; 
    float trans_diff_y_angle = diff_y_angle + _degree/2.0 ; 
   
    while(trans_diff_y_angle > 360) trans_diff_y_angle -= 360;
    while(trans_diff_y_angle < 0) trans_diff_y_angle += 360;
    return trans_diff_y_angle <= _degree;
}

/******************************************************************************
for lua
******************************************************************************/

const char Ctrl::className[] = "Ctrl";
const bool Ctrl::gc_delete_ = false;

Lunar<Ctrl>::RegType Ctrl::methods[] =
{	
	LUNAR_OBJ_METHODS,
	LUNAR_CTRL_METHODS,
	{NULL, NULL}
};



int Ctrl::c_delete( lua_State* _L )
{
    safe_delete();
	return 0;
}

int Ctrl::c_client_delete( lua_State* _L )
{
    delete this;
	return 0;
}

int Ctrl::c_get_id( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushnumber( _L, (int)get_id() );
	return 1;	
}

int Ctrl::c_set_id( lua_State* _state )
{
	lcheck_argc( _state, 1 );
	int ctrl_id = lua_tointeger( _state, 1 );
	set_id(ctrl_id);
	return 0;
}

int Ctrl::c_replace( lua_State* _L )
{
	lcheck_argc( _L, 3 );
	u_long scene_id = (u_long)lua_tointeger( _L, -3 );
	float x = lua_tonumber( _L, -2 );
	float z = lua_tonumber( _L, -1 );

	VECTOR3 pos;
	pos.x = x;
	pos.z = z;
	pos.y = 0;
	replace( scene_id, pos );
	TRACE(2)( "[OBJ](ctrl) c_replace, id =%d, x = %f, z = %f, scene = %d", ctrl_id_, x, z, scene_id );
	return 0;
}

int Ctrl::c_get_worldid( lua_State* _L )
{
	lcheck_argc( _L, 0 );
    lua_pushnumber( _L, world_id_ );
    return 1;
}


int Ctrl::c_get_sceneid( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	//lua_pushnumber( _L, world_id_ & 0x0000FFFF );
    lua_pushnumber( _L, world_id_ );
	return 1;
}

int Ctrl::c_take_in_world( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    unsigned long world_id = (int)lua_tonumber( _L, 1 );
    int plane = ( int )lua_tonumber( _L, 2 );
    World* world = g_worldmng->get_world(world_id);
    if( !world ) {
        ERR(2)( "[OBJ](ctrl) c_take_in_world error, no world, world_id = %d", world_id );
        c_bt( _L );
        lua_pushboolean( _L, false );
        return 1;
    }

    if( is_player() ) view_add( this );

    set_world( world );

    // 设置位面
    plane_ = plane;

    this->before_add( world );

    if( world->add_obj_link( this ) == 0 ) {
        if( world->add_obj_ary( this ) == 0 ) {
            if( world->add_to_view( this ) < 0 ) {
                lua_pushboolean( _L, false );	
            }else {
                this->after_add( world );

                lua_pushboolean( _L, true );	
            }
            return 1;
        }
    }
    
    lua_pushboolean( _L, false );	
    return 1;
}

int Ctrl::c_face_to( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	OBJID targetid = (OBJID)lua_tonumber( _L, 1 );
	Ctrl* target = g_worldmng->get_ctrl( targetid );
	int rt = 0;
	if( is_valid_obj( target ) )
	{
		set_angle_y( d3d_to_degree( vc3_get_angle( target->getpos() - getpos() ) ) );
		rt = 1;
	}
	lua_pushnumber( _L, rt );
	return 1;
}

int Ctrl::c_face_pos( lua_State* _L )
{
	lcheck_argc( _L, 2 );
    float x = lua_tonumber( _L, 1 );
    float z = lua_tonumber( _L, 2 );
    VECTOR3 target_pos( x, 0, z );
    set_angle_y( d3d_to_degree( vc3_get_angle( target_pos - getpos() ) ) );
    return 0;
}

int Ctrl::c_get_front_pos( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    float range = (float)lua_tonumber( _L, 1 );
    VECTOR3 pos = getpos();
    pos = pos + GET_DIR( get_angle_y() ) * range;
    lua_pushnumber( _L, pos.x );
    lua_pushnumber( _L, pos.y );
    lua_pushnumber( _L, pos.z );
    return 3;
}

int Ctrl::c_is_near_pc( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	OBJID targetid = (OBJID)lua_tonumber( _L, 1 );

    bool ret = false;
    Player* target = g_worldmng->get_player( targetid ); 
    if( target ) {
        if( target->get_world() == get_world() ) {
            VECTOR3 A = target->getpos();
            VECTOR3 B = getpos();
            VECTOR3 D = A - B;
            if( fabsf( D.x ) < 80.0f && fabsf( D.z ) < 80.0f ) ret = true;
        }
    }

	lua_pushboolean( _L, ret );
	return 1;
}

int Ctrl::c_get_iaobjid( lua_State* _L)
{
	lcheck_argc( _L, 0 );
    u_long id = NULL_ID;
    Ctrl* ride = get_iaobj();
    if(is_valid_obj(ride))  id = ride->get_id();
    lua_pushnumber(_L, id);
    return 1;
}

int Ctrl::c_get_pc_num( lua_State* _L )
{
    lcheck_argc( _L, 0 );
    lua_pushnumber( _L, pc_num_ );
    return 1;
}

int Ctrl::c_run_always( lua_State *_L )
{
	lcheck_argc( _L, 0 );
    pc_num_ += 1;
    return 0;
}

int Ctrl::c_get_plane( lua_State *_L )
{
    lcheck_argc( _L, 0 );
    lua_pushnumber(_L, plane_ );
    return 1;
}

int Ctrl::c_set_plane( lua_State *_L )
{
    lcheck_argc( _L, 1 );
    plane_ = (int)lua_tonumber( _L, 1 );
    return 0;
}

int Ctrl::c_replace_plane( lua_State *_L )
{
    lcheck_argc( _L, 6 );
    u_long world_id = ( u_long )lua_tonumber( _L, -6 );
    unsigned int plane = ( unsigned int )lua_tonumber( _L, -5 );
    VECTOR3 pos;
    pos.x = ( float )lua_tonumber( _L, -4 );
    pos.y = ( float )lua_tonumber( _L, -3 );
    pos.z = ( float )lua_tonumber( _L, -2 );
    bool notify = ( bool )lua_toboolean( _L, -1 );
    
    set_replace_notify( notify );

    replace( world_id, pos, plane );

    TRACE(2)( "[OBJ](ctrl) c_replace_plane, id = %d, x = %f, y = %f, z = %f, world = %d, plane = %d", ctrl_id_, pos.x, pos.y, pos.z, world_id, plane );

    return 0;
}

int Ctrl::c_is_y_angle_obj( lua_State* _L )
{
    lcheck_argc( _L, 2 );

	OBJID obj_id = (OBJID)lua_tonumber( _L, 1 );
    float degree = lua_tonumber( _L, 2 );

	Ctrl* obj = g_worldmng->get_ctrl( obj_id );

	if( is_valid_obj( obj ) )
    {
        lua_pushboolean( _L, is_y_angle_obj( obj, degree ) );
    }
    else
    {
        lua_pushboolean( _L, false );
    }

    return 1;
}

int Ctrl::c_set_ignore_block( lua_State* _state )
{
	lcheck_argc(_state, 1);
	is_ignore_block_ = (lua_tointeger( _state, 1 ) == 1);
	return 0;
}

int Ctrl::c_is_ignore_block( lua_State* _state )
{
	lcheck_argc(_state, 0);
    lua_pushboolean( _state, is_ignore_block_ );
	return 1;
}

int Ctrl::c_set_server_only( lua_State* _state )
{
	lcheck_argc(_state, 1);
    is_server_only_ = lua_toboolean( _state, 1 );
    return 0;
}

void Ctrl::to_lua( int state, int msg, int p0, int p1, int p2, int p3, int p4, int frame )
{
    AutoLock lock(&lua_msg_lock);
#ifdef FIX_LUA_MSG
    LUA_MSG *node = NULL;
    if(!list_empty(&Ctrl::lua_msg_free_q)) {
        list_head *pos = lua_msg_free_q.prev;
        node = list_entry(pos, LUA_MSG, link);
        list_del(&node->link);
    } else {
        node = new LUA_MSG;
    }
#else
    list_head *pos = lua_msg_q.prev; 
    LUA_MSG *node = list_entry(pos, LUA_MSG, link);
    
    if(node->objid == 0) {
        list_del(&node->link);
    } else {
        node = new LUA_MSG;
    }
#endif
    
    node->objid = get_id();
    node->state = state;
    node->msg = msg;
    node->p0 = p0;
    node->p1 = p1;
    node->p2 = p2;
    node->p3 = p3;
    node->p4 = p4;
    node->frame = g_frame + frame;

#ifdef FIX_LUA_MSG
    list_add_tail(&node->link, &lua_msg_q); //fix bug dave 2009.5.20
#else
    list_add(&node->link, &lua_msg_q);    
#endif
}

void Ctrl::init_lua_msg()
{
    memset(lua_msg, 0, sizeof(lua_msg)); 
    INIT_LIST_HEAD(&lua_msg_q);
    int i;

#ifdef FIX_LUA_MSG
    INIT_LIST_HEAD(&lua_msg_free_q);
    for(i = 0; i < MAX_LUA_MSG; ++i) {
        list_add_tail(&lua_msg[i].link, &lua_msg_free_q);  
    }
#else
    for(i = 0; i < MAX_LUA_MSG; ++i) {
        list_add_tail(&lua_msg[i].link, &lua_msg_q);  
    }
#endif
}

int Ctrl::c_findpath( lua_State *_L )
{
	float sx = luaL_checknumber(_L,1);
	float sz = luaL_checknumber(_L,2);
	float ex = luaL_checknumber(_L,3);
	float ez = luaL_checknumber(_L,4);

	VECTOR3 start_pos(sx, 0, sz);
	VECTOR3 end_pos(ex, 0, ez);
	Navmesh::Path path;

	bool ret = findpath(&start_pos, &end_pos, &path);

	lua_pushboolean(_L, ret);
	lua_newtable(_L);
	for(unsigned int i = 0; i < path.size(); ++i)
	{
		lua_newtable(_L);
		lua_pushnumber(_L, path[i].x);
		lua_rawseti(_L, -2, 1);
		lua_pushnumber(_L, path[i].z);
		lua_rawseti(_L, -2, 2);
		lua_rawseti(_L, -2, i+1);
	}
	lua_pushinteger(_L, path.size());
	return 3;
}

int Ctrl::c_raycast(lua_State *_L)
{
	float sx = luaL_checknumber(_L,1);
	float sz = luaL_checknumber(_L,2);
	float ex = luaL_checknumber(_L,3);
	float ez = luaL_checknumber(_L,4);

	VECTOR3 start_pos(sx, 0, sz);
	VECTOR3 end_pos(ex, 0, ez);
	VECTOR3 hit_pos;
	float hit_rate;
	
	bool ret = raycast(&start_pos, &end_pos, &hit_pos, &hit_rate);

	lua_pushboolean(_L, ret);
	lua_pushnumber(_L, hit_pos.x);
	lua_pushnumber(_L, hit_pos.z);
	lua_pushnumber(_L, hit_rate);
	return 4;
}

int Ctrl::c_raycast_by_range( lua_State* _L )
{
	lcheck_argc( _L, 1 );

	float range = luaL_checknumber( _L, 1 );

	VECTOR3 hit_pos;
	float hit_rate;
    bool ret = raycast_by_range(range, &hit_pos, &hit_rate);

	lua_pushboolean(_L, ret);
	lua_pushnumber(_L, hit_pos.x);
	lua_pushnumber(_L, hit_pos.z);
	lua_pushnumber(_L, hit_rate);
	return 4;
}

int  Ctrl::c_set_door_flag(lua_State *_L)
{
	unsigned short door = (unsigned short)luaL_checknumber(_L,1);
	unsigned int flag = (unsigned int)luaL_checknumber(_L,2);
	unsigned short door_flags = world_->get_door_flags(plane_);

	if (flag > 0)
	{
		door_flags |= (1<<door);
	}
	else
	{
		door_flags &= (~(1<<door));
	}
	world_->set_door_flags(door_flags, plane_);
	return 0;
}

int Ctrl::c_set_self_door_flag( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    unsigned short door_flag = (unsigned short)lua_tonumber( _state, 1 );
    door_flags_ = door_flag;
    return 0;
}

int Ctrl::c_get_self_door_flag( lua_State* _state )
{
	lcheck_argc( _state, 0 );
	lua_pushnumber( _state, door_flags_ );
	return 1;	
}

int Ctrl::c_find_pos_to_wall( lua_State* _L )
{
    lcheck_argc( _L, 3 );

    float x = lua_tonumber( _L, 1 );
    float z = lua_tonumber( _L, 2 );
    float radius = lua_tonumber( _L, 3 );

	VECTOR3 pos(x, 0, z);
	VECTOR3 hit_pos;
	unsigned short door_flags = (door_flags_ != 0) ? door_flags_ : world_->get_door_flags(plane_);
	bool ret = world_->get_scene()->find_pos_to_wall( &pos, radius, &hit_pos, door_flags );

    lua_pushboolean( _L, ret );
    lua_pushnumber( _L, hit_pos.x );
    lua_pushnumber( _L, hit_pos.z );
    return 3;
}

bool Ctrl::raycast( const VECTOR3* _start_pos, const VECTOR3* _end_pos, VECTOR3* _hit_pos, float* _hit_rate, unsigned short _door_flags )
{
	if (is_ignore_block_)
	{
		_hit_pos->x = _end_pos->x;
		_hit_pos->z = _end_pos->z;
		*_hit_rate = 1.0f;
		return true;
	}
	else
	{
        // 优先顺序 _door_flags > door_flags_ > world->door_flag_
		unsigned short door_flags = (_door_flags != 0) ? _door_flags : ((door_flags_ != 0) ? door_flags_ : world_->get_door_flags(plane_));
		return world_->get_scene()->raycast(_start_pos, _end_pos, _hit_pos, _hit_rate, door_flags);
	}
}

bool Ctrl::findpath( const VECTOR3* _start_pos, const VECTOR3* _end_pos, Navmesh::Path* _path, unsigned short _door_flags )
{
	unsigned short door_flags = ( _door_flags != 0) ? _door_flags :(door_flags_ != 0) ? door_flags_ : world_->get_door_flags(plane_);
	return world_->get_scene()->findpath(_start_pos, _end_pos, _path, door_flags );
}

int Ctrl::c_is_target_in_range( lua_State* _state ) 
{
    lcheck_argc( _state, 2 );
	OBJID targetid = (OBJID)lua_tonumber( _state, 1 );
	Ctrl* target = g_worldmng->get_ctrl( targetid );
    float range = (float)lua_tonumber( _state, 2 );
    bool ret = false;
    if( target ) 
    {
        ret = is_range_obj_ground( target, range );
    }
    lua_pushboolean( _state, ret );
    return 1;
}

bool Ctrl::get_valid_pos( const VECTOR3* _pos, float _radius, VECTOR3* _result_pos ) 
{
	unsigned short door_flags = (door_flags_ != 0) ? door_flags_ : world_->get_door_flags(plane_);
    return world_->get_scene()->get_valid_pos( _pos, _radius, _result_pos, door_flags );
}

int Ctrl::c_get_valid_pos( lua_State* _state ) 
{
    lcheck_argc( _state, 3 );
    float x = (float)lua_tonumber( _state, 1 ) ;
    float z = (float)lua_tonumber( _state, 2 ) ;
    float radius = (float)lua_tonumber( _state, 3 );
    VECTOR3 pos( x, 0, z );
    VECTOR3 result(0,0,0);
    bool r = get_valid_pos( &pos, radius, &result );
    lua_pushboolean( _state, r );
    lua_pushnumber( _state, result.x );
    lua_pushnumber( _state, result.z );
    return 3;
}
