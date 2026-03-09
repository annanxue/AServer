#include <assert.h>
#include "world.h"
#include "log.h"
#include "world_mng.h"
#include "player_mng.h"
#include "define_func.h"

/* 所有的world共用处理线程*/
int					World::world_count_			= 0;
int					World::process_num_			= 0;
Event*				World::process_event_		= NULL;
PcQueue*			World::process_task_		= NULL;
WorkThread*			World::work_thread_list_	= NULL;
WorldProcessTask*	World::work_task_list_		= NULL;


void WorkThread::run()
{
	WorldProcessTask* work_task = NULL;

	unsigned long long time_run;
	while(1)
	{
		World::process_task_->get_userdata( (void**)&work_task );
		//LOG(2)( "World Work thread, %x, objs( %d %d )", pthread_self(), work_task->start_, work_task->end_ );
		time_run = 0;
		TICK_L( time_run )

		if( !work_task )
		{
			return;
		}

		if( !work_task->world_ )
		{
			continue;
		}

		for(int i = work_task->start_; i < work_task->end_; ++i )
		{
			Ctrl* obj = work_task->world_->obj_ary_[i];
			if( obj )
			{

				if( obj->get_world() != work_task->world_ ){
					ERR(2)( "[SENCE](run) WorkThread::run, obj's world = 0x%08X, this = 0x%08X", obj->get_world(), work_task->world_ );
					ERR(2)( "[SENCE](run) WorkThread::run, ctrl = %d, addr = 0x%08X", obj->get_id(), obj );
				}

				assert( obj->get_world() == work_task->world_  );
				
				if( obj && !obj->is_delete() )
				{
					obj->process_parallel();
				}
			}
		}

		TICK_H( time_run )
		//LOG(2)( "id = 0x%08X, time = %4llu, num = %d", self, SECM(time_run), work_task->end_ - work_task->start_ );
		World::process_event_->set_event(work_task->task_id_);
	}
}


World::World( u_long _id, Scene* _scene )
{
	assert(_scene);

	if( world_count_++ == 0 )
	{
		create_work_thread( 4 );
	}
	
	world_id_		= _id;
	scene_			= _scene; /*! 场景是静态数据，直接保存指针*/
	player_count_	= 0;
	delete_			= FALSE;
	add_cnt_		= 0;
	obj_cnt_		= 0;
	modify_cnt_		= 0;
	replace_cnt_	= 0;
	delete_cnt_		= 0;

	init_world();
	door_flags_map_.init( 1, 8, "door_flags_map_:world.cpp" );

    INIT_LIST_HEAD( &idx_list_ );
    unsigned int i;
    for(i = 0; i < sizeof(idx_node_)/sizeof(NodeObj); ++i){
        idx_node_[i].idx = -1;         
        list_add(&idx_node_[i].link, &idx_list_);
    }
}


World::~World()
{
	Ctrl* obj = NULL;
    LOG(1)("WORLD DESTROY 0x%08x", get_id());

	/* world的析构只会发生在玩家为0之后的一段时间,此时add队列应该
	是为空的,所以此处用断言,而为安全起见,后面还是进行了清除 */
	//assert(add_cnt_==0); 
	if( add_cnt_ != 0 )
	{
		ERR(2)( "[SENCE](run) %s:%d, add_cnt is not zero", __FILE__, __LINE__ );
	}

	for( int i = 0; i < add_cnt_; ++i )
	{
		obj = add_ary_[i];
		if( !obj || obj->is_player() )
		{
			ERR(2)( "[SENCE](run) %s:%d %s: invalid world add objs", __FILE__, __LINE__, __FUNCTION__ );
		}
		SAFE_DELETE( obj );
	}

    AutoLock lock(&Ctrl::lua_msg_lock);
	for( int i = 0; i < obj_cnt_; ++i )
    {
        obj = obj_ary_[i];
        if (obj) {
            obj->set_world(NULL);
            obj->set_delete();
            LOG(1)("ADD WORLD GONE TO LUA %d", obj->get_id());
        }
    }

	destroy_world();

	if( --world_count_ == 0 )
	{
		close_work_thread();
	}

    list_head* pos;
    list_for_each_safe(pos, &idx_list_) {
        NodeObj* node = list_entry( pos, NodeObj, link );
        int index = node - idx_node_;
        if(index < 0 || (unsigned int)index >= sizeof(idx_node_)/sizeof(NodeObj) ) {
            list_del(pos);
            delete node;
        }
    }

    for(int i = 0; i < MAX_LAND; ++i) {
        if(vland_[i]) {
            SAFE_DELETE(vland_[i]);
        } 
    }

}

/* 建立处理线程*/
void World::create_work_thread( int _process_num )
{
	process_num_ = _process_num;
	process_event_ = new Event(process_num_);
	process_task_ = new PcQueue(process_num_);
	work_task_list_ = new WorldProcessTask[process_num_];
	work_thread_list_ = new WorkThread[process_num_];

	for( int i = 0; i < process_num_; ++i )
	{
		work_thread_list_[i].start();
	}
}

/* 关闭处理线程*/
void World::close_work_thread()
{
	for( int i = 0; i < process_num_; ++i )
	{
		process_task_->post_userdata( NULL );
	}

	SAFE_DELETE_ARRAY(work_thread_list_);
	SAFE_DELETE_ARRAY(work_task_list_);
	SAFE_DELETE(process_task_);
	SAFE_DELETE(process_event_);
}


int World::process()
{
    TICK(A);
	_add();
    TICK(B);
	_process();
    TICK(C);
    _modifylink();
    TICK(D);
	_replace();
    TICK(E);
	_delete();
    TICK(F);

    mark_tick(TICK_ADD, A, B); //C
    mark_tick(TICK_PRO, B, C); //D
    mark_tick(TICK_MOD, C, D); //E
    mark_tick(TICK_REP, D, E); //F
    mark_tick(TICK_DEL, E, F); //10

	return 0;
}


void World::_add()
{
	Ctrl* obj = NULL;

	for( int i = 0; i < add_cnt_; ++i )
	{
		obj = add_ary_[i];
		if( is_invalid_obj( obj ) )
			continue;
			
		assert( obj->get_world() == this );

        obj->before_add( this );

		/* 添加到linkmap*/
		if( add_obj_link( obj ) < 0 )
		{
			ERR(2)( "[SENCE](world) World::_add() fail to add obj link! id:%d", obj->get_id());
			obj->set_world( NULL );
			continue;
		}
		/* 添加到world队列*/
		if( add_obj_ary( obj ) < 0 )
		{
			ERR(2)( "[SENCE](world) World::_add() fail to add obj array! id:%d", obj->get_id());
			remove_obj_link( obj );
			obj->set_world( NULL );
			continue;
		}

		if( add_to_view( obj ) < 0 )
        {
			ERR(2)( "[SENCE](world) World::_add() fail to add obj view! id:%d", obj->get_id());
            remove_obj_ary( obj );
			remove_obj_link( obj );
			obj->set_world( NULL );
            continue;
        }

        obj->after_add( this );
	}

	add_cnt_ = 0;
}


void World::_process()
{
	int cb = obj_cnt_;

	if( cb )
	{
		/* 将所有obj对象,均分给各个处理线程*/
		int obj_count = (obj_cnt_ + process_num_ -  1) / process_num_;
		int task = 0;

        TICK(A)
		while(cb > 0)
		{
			int len = cb>obj_count?obj_count:cb;
			/* 填充任务结构并发送到任务队列中*/
			WorldProcessTask* work_task = &work_task_list_[task];
			work_task->world_	= this;
			work_task->task_id_	= task;
			work_task->start_	= obj_cnt_ - cb;
			work_task->end_		= work_task->start_ + len;

			cb -= len;			

			process_task_->post_userdata( (void*)(work_task) );
			task++;
		}
		assert(task<=process_num_);
		/* 等待各个并行处理线程处理完毕*/
		for( int i = 0; i < task; ++i)
		{
			process_event_->wait_event(i);
		}
        TICK(B)
        mark_tick( TICK_PARALLEL, A, B );
		
        TICK(A1)
		for( int i = 0; i < obj_cnt_; ++i )
		{
			Ctrl* obj = obj_ary_[i];
			if( obj && !obj->is_delete() )
			{
				obj->process_serial();
			}
		}
        TICK(B1)
        mark_tick( TICK_SERIAL, A1, B1 );

	}
}

// set obj.x_ obj.z_
void World::_modifylink()
{
	Ctrl* obj = NULL;

	for( int i = 0; i < modify_cnt_; ++i ) {
		obj = modify_ary_[i];
		if( is_invalid_obj( obj ) ) continue;
		if( obj->get_world() != this ) continue;

		int link_idx = obj->get_link_pos();
        int cur_idx = obj->get_new_link_pos();

		if( cur_idx != -1 && cur_idx != link_idx ) {
            // Get link map
            int link_level = obj->get_linklevel();
			int link_type = obj->get_linktype();
			Ctrl** obj_link_map = get_obj_link( link_type, link_level );
			assert( obj_link_map );
			assert( !obj->is_spirit() || link_level == 0  );

            if( is_valid_obj_link_count( link_type, link_level, cur_idx ) == false )
            {
                ERR(2)( "[SCENE](world) World::_modifylink() obj_link_count is err, link_type: %d, link_level: %d, link_pos: %d, obj_link_count: %d, world_id: %d, id:%u", link_type, link_level, cur_idx, obj_link_count_[link_type][link_level], world_id_, obj->get_id() );
                continue;
            }
            
            // Remove from old link map
            /* 前节点为空,却又不在链表头部,则遍历整个linkmap的所有头节点,并删除之*/
			if( obj->get_prev_node() == NULL && obj_link_map[link_idx] != obj ) {
				ERR(2)("[SENCE](world) World::_modifylink() obj prev node null, but not the header of linkmap. id:%d", obj->get_id());
				remove_obj_link2( obj );
			}

			if( obj_link_map[link_idx] == obj ) {
				obj_link_map[link_idx] = (Ctrl*)obj->get_next_node();
			}
			obj->del_node();

            // Insert into new link map
			if( obj_link_map[cur_idx] ) {
				obj_link_map[cur_idx]->ins_next_node( obj );
			} else {
				obj_link_map[cur_idx] = obj;
			}

			obj->set_link_pos( cur_idx );

            // Process view modify
			modify_view( obj );
		}
	}
	modify_cnt_ = 0;
}

void World::_replace()
{
	Ctrl* obj = NULL;

	for( int i = 0; i < replace_cnt_; ++i )
	{
		obj				= replace_ary_[i].obj_;
		u_long world_id	= replace_ary_[i].world_id_;
        int plane       = replace_ary_[i].plane_;
		VECTOR3& pos	= replace_ary_[i].pos_;

		if( is_invalid_obj( obj ) ) continue;

		if( obj->get_world() != this ) {
            TRACE(2)( "[SENCE](world) _replace, obj's world != this" );
            continue;
        }

        u_long old_world_id = get_id();
        int old_plane = obj->plane_;

        // 同世界，同位面跳转
		if( old_world_id == world_id && plane == old_plane ) {
            World* world = obj->get_world();
            if( !world ) {
                ERR( 2 )( "[SENCE](world) World::_replace() obj not in world! obj: %d", obj->get_id() );
                continue;
            }    

            if( !world->in_world( pos ) ) {
                ERR( 2 )( "[SENCE](world) World::_replace() obj %d replace pos not in world: %d! %f, %f, %f", obj->get_id(), world->get_id(), pos.x, pos.y, pos.z );
                continue;
            }    

            obj->before_replace( this, world );

            CHECK_NULL_POS( pos );
			obj->setpos( pos );
            g_playermng->add_setpos( obj, pos );   

            obj->after_replace( this, world );
        }
        // 同世界，不同位面跳转
        else if( old_world_id == world_id && plane != old_plane ) {
            World* world = obj->get_world();
            if( !world ) {
                ERR( 2 )( "[SENCE](world) World::_replace() obj not in world! obj: %d", obj->get_id() );
                continue;
            }    

            if( !world->in_world( pos ) ) {
                ERR( 2 )( "[SENCE](world) World::_replace() obj %d replace pos not in world: %d! %f, %f, %f", obj->get_id(), world->get_id(), pos.x, pos.y, pos.z );
                continue;
            }    

            obj->before_replace( this, world );

            old_world_id = world->get_id();

            // 从当前世界移除
            remove_view( obj );
            remove_obj_ary( obj );
            remove_obj_link( obj );
            obj->set_world( NULL );

            // 设置新位面
            obj->plane_ = plane;

            // 重新添加到世界
            CHECK_NULL_POS( pos );
            obj->setpos( pos );
            if( obj->is_player() )
                ( ( Player* )obj )->add_setpos( pos );
            world->add_obj( obj );

            obj->after_replace( this, world );
        }
        // 不同世界跳转
        else {
            World* world = g_worldmng->get_world( world_id );

            if( !world ) {
	        	ERR(2)("[SENCE](world) World::_replace() no world to replace to! world_id:%d, obj:%d", world_id, obj->get_id());
                continue;
            }

            if( !world->in_world( pos ) ) {
		        ERR(2)("[SENCE](world) World::_replace() obj %d replace pos not in world:%d! %f,%f,%f", obj->get_id(), world_id, pos.x, pos.y, pos.z);
                continue;
            }

            obj->before_replace( this, world );

            obj->remove_from_view();
            remove_obj_ary( obj );
            remove_obj_link( obj );
            obj->set_world( NULL );

            if( obj->is_player() && obj->get_replace_notify() ) {
                Scene* scene = world->get_scene();
                if( !scene ) {
                    ERR(2)("[SENCE](world) World::_replace() can not get scene of world:%d!obj:%d", world_id, obj->get_id());
                    continue;
                }
                ((Player*)obj)->add_replace( scene->get_id(), pos );
                // push scene_id in lua
                //((Player*)obj)->to_lua( STATE_NONE, MSG_REPLACE, scene->get_id() );    
            }

            // 设置新位面
            obj->plane_ = plane;

            CHECK_NULL_POS( pos );
            obj->setpos( pos );
            world->add_obj( obj );

            obj->after_replace( this, world );
        }
	}

	replace_cnt_ = 0;
}


void World::_delete()
{
	Ctrl* obj = NULL;

	for( int i = 0; i < delete_cnt_; ++i )
	{
		obj = delete_ary_[i];
		if( NULL == obj )
			continue;
		assert( obj->get_world() == this );

        obj->before_delete( this );

		for( int j = 0; j < replace_cnt_; ++j )
		{
			if( replace_ary_[j].obj_ == obj )
			{
				replace_ary_[j].obj_ = NULL;
			}
		}

		for( int j = 0; j < modify_cnt_; ++j )		
		{
			if( modify_ary_[j] == obj )
			{
				modify_ary_[j] = NULL;
			}
		}

        obj->remove_from_view();
		remove_obj_ary( obj );
		remove_obj_link( obj );
		obj->set_world( NULL );

        obj->after_delete( this );

		SAFE_DELETE( obj );
	}

	delete_cnt_ = 0;
}


int World::add_obj( Ctrl* _obj )
{
	assert(_obj);

	if( !in_world( _obj->getpos() ) || _obj->get_world() )
	{
		ERR(2)("[SENCE](world) World::add_obj() can not add obj:%d,pos:(%f,%f,%f),world:0x%08X",
			_obj->get_id(), _obj->getpos().x, _obj->getpos().y, _obj->getpos().z, _obj->get_world());
		return -1;
	}

	if( add_cnt_ >= max_addobjs_ )
	{
		ERR(2)("[SENCE](world) World::add_obj() add count overflowed.");
		return -1;
	}

	_obj->set_world( this );
	add_ary_[add_cnt_++] = _obj;

    _obj->set_world_logic( TRY_ADD );

	return 0;
}


int World::delete_obj( Ctrl* _obj )
{
	assert(_obj);

	if( _obj->get_world() != this || _obj->is_delete() )
	{
		TRACE(2)("[SENCE](world) World::delete_obj() can not delete obj:%d,is_delete:%d", _obj->get_id(), _obj->is_delete());
		return -1;
	}
	/* 如果是正在添加队列中的,则直接删除*/
	for( int i = 0; i < add_cnt_; ++i )
	{
		if( add_ary_[i] == _obj )
		{
			_obj->set_world( NULL );
			add_ary_[i]	= NULL;
			SAFE_DELETE( _obj );
			return 0;
		}
	}

	if( delete_cnt_ >= max_deleteobjs_ )
	{
		ERR(2)("[SENCE](world) World::delete_obj() delete count overflowed.");
		return -1;
	}

	_obj->set_delete();
	delete_ary_[delete_cnt_++] = _obj;

	return 0;
}


int World::add_obj_ary( Ctrl* _obj )
{
    /*! list version */
    int idx = -1;
    if( !list_empty( &idx_list_ ) ) {
        list_head* pos = idx_list_.next;
        NodeObj* node = list_entry( pos, NodeObj, link );
        if( node->idx >= 0 ) {
            list_del( pos );
            idx = node->idx;
            node->idx = -1;
            
            int index = node - idx_node_;
            if( index >= 0 && (unsigned int)index < sizeof(idx_node_)/sizeof(NodeObj) ) {
                list_add_tail( pos, &idx_list_ );
            } else {
                delete node; 
            }

        } else {
		    idx = obj_cnt_++;
        }
    } else {
        idx = obj_cnt_++;
    }
    
    if( idx < max_dynamicobjs_ ) {
        if( obj_ary_[idx] == NULL ) {
            obj_ary_[idx] = _obj;
            _obj->set_obj_ary_idx( idx );
            if( _obj->is_player() ) {
                player_count_++;
            }
            return 0;
        }
    }
    return -1;
}


int World::remove_obj_ary( Ctrl* _obj )
{
    /*! list version  */
    int idx = _obj->get_obj_ary_idx();
    if( idx < max_dynamicobjs_ ) {
        if( _obj == obj_ary_[idx] ) {
            _obj->set_obj_ary_idx( 0xFFFFFFFF );
            obj_ary_[idx] = NULL;

            if(!list_empty(&idx_list_)) {
                list_head* pos = idx_list_.prev;
                NodeObj* node = list_entry(pos, NodeObj, link);
                if(node->idx < 0) {
                    list_del(pos); 
                } else {
                    node = new NodeObj; 
                }

                node->idx = idx; 
                list_add( &node->link, &idx_list_ );

                return 0;
            }
        }
    }
    return -1;
}


int World::add_obj_link( Ctrl* _obj )
{
	assert(_obj);
	assert(!_obj->is_spirit() || _obj->get_linklevel() == 0);

	VECTOR3 pos = _obj->getpos();
	int link_level = _obj->get_linklevel();
	int link_type = _obj->get_linktype();  
	Ctrl** obj_link_map = get_obj_link( link_type, link_level );
	assert(obj_link_map);
	int max_width = get_land_width( link_level ) * scene_->get_width();
	int max_height = get_land_width( link_level ) * scene_->get_height();
	int cur_x = (int)( pos.x / get_patch_size(link_level) );
	int cur_z = (int)( pos.z / get_patch_size(link_level) );

	if( cur_x < 0 || cur_x >= max_width || cur_z < 0 || cur_z >= max_height ) {
		ERR(2)("[SENCE](world) World::add_obj_link() error obj pos(%d,%d), block(%d,%d), obj:%d, world_id:%d, type:0x%08x, index:%d", int( pos.x ), int( pos.z ), cur_x, cur_z, _obj->get_id(), get_id(), _obj->gettype(), _obj->get_index() );
		return -1;
	}

	int idx = cur_z * max_width + cur_x;
	Ctrl* head_obj = obj_link_map[idx];
	if( head_obj ) {
		head_obj->ins_next_node( _obj );
	} else {
		obj_link_map[idx] = _obj;
	}
	_obj->set_link_pos( idx );

	return 0;
}


int World::remove_obj_link( Ctrl* _obj )
{
	assert(_obj);
	assert(!_obj->is_spirit() || _obj->get_linklevel() == 0);

	int link_level = _obj->get_linklevel();
	int link_type = _obj->get_linktype();  
	Ctrl** obj_link_map = get_obj_link( link_type, link_level );
	assert(obj_link_map);
	int idx = _obj->get_link_pos();
	
	int max_width = get_land_width( link_level ) * scene_->get_width();
	int max_height = get_land_width( link_level ) * scene_->get_height();

	if( idx < 0 || idx >= max_width * max_height )
	{
		ERR(2)("[SENCE](world) World::remove_obj_link() error idx : %d, obj:%d, world_id:%d, type:0x%08x, index:%d", idx, _obj->get_id(), get_id(), _obj->gettype(), _obj->get_index());
		if( remove_obj_link2( _obj ) < 0 )
		{
			_obj->del_node();
		}
		return -1;
	}

	if( _obj->get_prev_node() == NULL && obj_link_map[idx] != _obj )
	{
		return remove_obj_link2( _obj );
	}

	if( obj_link_map[idx] == _obj )
	{
		obj_link_map[idx] = (Ctrl*)_obj->get_next_node();
	}

	_obj->del_node();
    
	return 0;
}


int World::remove_obj_link2( Ctrl* _obj )
{
	assert(_obj);
	assert(!_obj->is_spirit() || _obj->get_linklevel() == 0);

	int link_level = _obj->get_linklevel();
	int link_type = _obj->get_linktype();  
	Ctrl** obj_link_map = get_obj_link( link_type, link_level );
	assert(obj_link_map);
	int max_width = get_land_width( link_level ) * scene_->get_width();
	int max_height = get_land_width( link_level ) * scene_->get_height();
	int max_pos = max_width * max_height;

	for( int idx = 0; idx < max_pos ; ++idx ) {
		if( obj_link_map[idx] == _obj )
		{
			obj_link_map[idx] = (Ctrl*)_obj->get_next_node();
			_obj->del_node();

			return 0;
		}
	}

    if(_obj->is_player()) {
        for(int idx = 0; idx < MAX_LAND; ++idx) {
            if(vland_[idx]) vland_[idx]->del(((Player*)_obj)->get_playerid());
        }
    }

	return -1;
}

bool World::try_add_obj_link( Ctrl* _obj )
{
	assert(_obj);
	assert(!_obj->is_spirit() || _obj->get_linklevel() == 0);

	VECTOR3 pos = _obj->getpos();
    if( pos.x < 0 || pos.z < 0 ) return false;

	int link_level = _obj->get_linklevel();
	int max_width = get_land_width( link_level ) * scene_->get_width();
	int max_height = get_land_width( link_level ) * scene_->get_height();
	int cur_x = (int)( pos.x / get_patch_size(link_level) );
	int cur_z = (int)( pos.z / get_patch_size(link_level) );

	if( cur_x < 0 || cur_x >= max_width || cur_z < 0 || cur_z >= max_height ) {
		return false;
	}
    return true;
}

int World::init_world()
{
	assert(scene_);
	/* 多世界场景所生成的世界建立小容量的处理队列,单一世界则使用大容量队列*/
	if( scene_->is_multi_world() )
	{
		max_addobjs_		= MAX_ADDOBJS_S;
		max_dynamicobjs_	= MAX_DYNAMICOBJS_S;
		max_modifylinkobjs_	= MAX_MODIFYLINKOBJS_S;
		max_replaceobjs_	= MAX_REPLACEOBJS_S;
		max_deleteobjs_		= MAX_DELETEOBJS_S;
	}
	else
	{
		max_addobjs_		= MAX_ADDOBJS_L;
		max_dynamicobjs_	= MAX_DYNAMICOBJS_L;
		max_modifylinkobjs_	= MAX_MODIFYLINKOBJS_L;
		max_replaceobjs_	= MAX_REPLACEOBJS_L;
		max_deleteobjs_		= MAX_DELETEOBJS_L;
	}

	add_ary_				= new Ctrl*[max_addobjs_];
	obj_ary_				= new Ctrl*[max_dynamicobjs_];
	modify_ary_				= new Ctrl*[max_modifylinkobjs_];
	replace_ary_			= new ReplaceObj[max_replaceobjs_];
	delete_ary_				= new Ctrl*[max_deleteobjs_];

	memset( add_ary_, 0, sizeof( Ctrl* ) * max_addobjs_ );
	memset( obj_ary_, 0, sizeof( Ctrl* ) * max_dynamicobjs_ );
	memset( modify_ary_, 0, sizeof( Ctrl* ) * max_modifylinkobjs_ );
	memset( replace_ary_, 0, sizeof( ReplaceObj) * max_replaceobjs_ );
	memset( delete_ary_, 0, sizeof( Ctrl* ) * max_deleteobjs_ );

    memset(obj_link_, 0, sizeof(obj_link_));
    int link_count_land = LANDSCAPE_SIZE / scene_->get_min_dynamic_land_size();
	for( int level = 0; level < DYNAMIC_LINKLEVEL; ++level ) {
		int link_count_all = link_count_land * link_count_land * scene_->get_width() * scene_->get_height();
        for(int type = Obj::LinkStatic + 1; type < DYNAMIC_LINKTYPE; ++type) {
            obj_link_[type][level] = new Ctrl*[link_count_all];
            memset( obj_link_[type][level], 0, sizeof(Ctrl*) * link_count_all );
            obj_link_count_[type][level] = link_count_all;
        }
		land_width_[level] = link_count_land;
		patch_size_[level] = LANDSCAPE_SIZE / link_count_land;
		int range = (int)( 2 / pow( 2.0f, level ) );
		visibility_range_[level] = range?range:1;
		link_count_land /= 2;
	}

    memset(vland_, 0, sizeof(vland_));
    
	return 0;
}




void World::destroy_world()
{
	for( int level = 0; level < DYNAMIC_LINKLEVEL; ++level )
	{
		for(int type = 0; type < DYNAMIC_LINKTYPE; ++type)
		{
			SAFE_DELETE_ARRAY(obj_link_[type][level]);
		}
	}

	SAFE_DELETE_ARRAY(add_ary_);
	SAFE_DELETE_ARRAY(obj_ary_);
	SAFE_DELETE_ARRAY(modify_ary_);
	SAFE_DELETE_ARRAY(replace_ary_);
	SAFE_DELETE_ARRAY(delete_ary_);
}

Ctrl** World::get_obj_link( int _link_type, int _link_level ) 
{
    if(_link_type != Obj::LinkStatic) {
        return obj_link_[_link_type][_link_level];
    } else {
        return NULL;
    }
}

int World::add_to_view( Ctrl* _obj, bool _is_refresh )
{
    assert(_obj);
    VECTOR3 pos = _obj->getpos();
    for( int level = 0; level < DYNAMIC_LINKLEVEL; ++level ) {
        int visibility = visibility_range_[level];
        int max_width = get_land_width( level ) * scene_->get_width();
        int max_height = get_land_width( level ) * scene_->get_height();
        int patch_size = get_patch_size(level);
        int cur_x = (int)( pos.x / patch_size );
        int cur_z = (int)( pos.z / patch_size );

        if( cur_x < 0 || cur_x >= max_width || cur_z < 0 || cur_z >= max_height ) {
            ERR(2)("[SENCE](world) World::add_to_view() error obj pos(%d,%d), obj:%d", cur_x, cur_z, _obj->get_id());
            return -1;
        }

        _obj->set_center_pos( level, cur_x, cur_z );

        for( int type = Obj::LinkStatic + 1; type < DYNAMIC_LINKTYPE; ++type ) {
            Ctrl** obj_link_map = get_obj_link( type, level );
            assert(obj_link_map);
            for( int x = cur_x - visibility; x <= cur_x + visibility; ++x ) {
                if( x < 0 || x >= max_width ) continue;
                for( int z = cur_z - visibility; z <= cur_z + visibility; ++z ) {
                    if( z < 0 || z >= max_height ) continue;
                    Ctrl* obj = obj_link_map[ z * max_width + x ];
                    while( obj ) {
                        // if( !obj->is_delete() && obj != _obj && obj->plane_ == _obj->plane_ ) {
                        if( !obj->is_delete() && obj != _obj && _obj->check_plane_for_view( obj ) ) {
                            _obj->view_add( obj );
                            if( !_is_refresh )
                            {
                                obj->view_add( _obj );
                            }
                        }
                        obj = (Ctrl*)obj->get_next_node();
                    }
                }
            }
        }
    }
    return 0;
}

static int view[3][3] = {
    {0, 0, 0,},  // static
    {0, 1, 1,},  // player
    {0, 1, 1,},  // dynamic
};

static int belevel[3][1] = {
    {1,},
    {1,},
    {1,}
};

int World::modify_view_level_0( Ctrl* _obj )
{
    assert(_obj);

    int type = _obj->get_linktype();
    int land_width = get_land_width(0);
    int w = land_width * scene_->get_width();    //width
    int h = land_width * scene_->get_height();   //height

    int old_x = _obj->get_center_pos_x(0);
    int old_z = _obj->get_center_pos_z(0);
    int patch_size = get_patch_size(0);
    int new_x = (int)( _obj->getpos().x / patch_size );
    int new_z = (int)( _obj->getpos().z / patch_size );

    _obj->set_center_pos( 0, new_x, new_z );

    if(old_x == new_x && old_z == new_z) return 0;

    // check link type 
    for(int linktype = Obj::LinkStatic + 1; linktype < DYNAMIC_LINKTYPE; ++linktype) 
    {
        if(!view[type][linktype]) continue;
        // check link level
        int t_old_x = old_x;
        int t_old_z = old_z;
        int t_new_x = new_x;
        int t_new_z = new_z;
        int t_w = w;
        int t_h = h;
        for(int linklevel = 0; linklevel < DYNAMIC_LINKLEVEL; ++linklevel) 
        {
            if(!belevel[linktype][linklevel]) 
            {
                // level up
                t_old_x >>= 1;
                t_old_z >>= 1;
                t_new_x >>= 1;
                t_new_z >>= 1;
                t_w >>= 1;
                t_h >>= 1;
                continue;
            }
            // check range
            if(t_old_x != t_new_x || t_old_z != t_new_z) 
            {
                int r = visibility_range_[linklevel];
                Ctrl** obj_link_map = get_obj_link(linktype, linklevel);

                _obj->set_center_pos( linklevel, t_new_x, t_new_z );

                // chech view remove
                for(int x = t_old_x - r; x <= t_old_x + r; ++x) 
                {
                    if(x < 0 || x >= t_w) continue;
                    for(int z = t_old_z - r; z <= t_old_z + r; ++z) 
                    {
                        if(z < 0 || z >= t_h) continue;
                        if(abs(x - t_new_x) > r || abs(z - t_new_z) > r) 
                        {
                            Ctrl *obj = obj_link_map[z * t_w + x];
                            while(obj) 
                            {
                                // if(!obj->is_delete() && obj != _obj && obj->plane_ == _obj->plane_ ) {
                                if(!obj->is_delete() && obj != _obj && _obj->check_plane_for_view( obj ) ) {
                                        _obj->view_remove(obj);
                                        obj->view_remove(_obj);
                                    } 
                                    obj = (Ctrl*)obj->get_next_node();
                            }
                        }
                    }
                }

                // check view add
                for(int x = t_new_x - r; x <= t_new_x + r; ++x) 
                {
                    if(x < 0 || x >= t_w) continue;
                    for(int z = t_new_z - r; z <= t_new_z + r; ++z) 
                    {
                        if(z < 0 || z >= t_h) continue;
                        if(abs(x - t_old_x) > r || abs(z - t_old_z) > r) 
                        {
                            Ctrl *obj = obj_link_map[z * t_w + x];
                            while(obj) 
                            {
                                // if(!obj->is_delete() && obj != _obj && obj->plane_ == _obj->plane_ ) {
                                if(!obj->is_delete() && obj != _obj && _obj->check_plane_for_view( obj ) ) {
                                        _obj->view_add(obj);
                                        obj->view_add(_obj);
                                    } 
                                    obj = (Ctrl*)obj->get_next_node();
                            }
                        }
                    }
                }

                // level up
                t_old_x >>= 1;
                t_old_z >>= 1;
                t_new_x >>= 1;
                t_new_z >>= 1;
                t_w >>= 1;
                t_h >>= 1;
            }
        }
    }
    return 0;
}

int World::modify_view( Ctrl* _obj )
{
    assert(_obj);

    int level = _obj->get_linklevel();
    if( level == 0 ) return modify_view_level_0( _obj );

    int type = _obj->get_linktype();
    int land_width = get_land_width(level);
    int w = land_width * scene_->get_width();    //width
    int h = land_width * scene_->get_height();   //height

    int old_x = _obj->get_center_pos_x(level);
    int old_z = _obj->get_center_pos_z(level);
    int patch_size = get_patch_size(level);
    int new_x = (int)( _obj->getpos().x / patch_size );
    int new_z = (int)( _obj->getpos().z / patch_size );

    //if(_obj->is_player()) {
        //LOG(1)("PLAYER POS %d, %d", new_x, new_z);
    //}
	_obj->set_center_pos( level, new_x, new_z );

    if(old_x != new_x || old_z != new_z) {
        for(int linktype = Obj::LinkStatic + 1; linktype < DYNAMIC_LINKTYPE; ++linktype) {
            if(view[type][linktype]) {
                /*! handle down level */
                int r = visibility_range_[level];
                for(int x = old_x - r; x <= old_x + r; ++x) {
                    if(x < 0 || x >= w) continue;
                    for(int z = old_z - r; z <= old_z + r; ++z) {
                        if(z < 0 || z >= h) continue;
                        if(abs(x - new_x) > r || abs(z - new_z) > r) {
                            int left = x;
                            int top = z;
                            int count = 1;
                            int mul = w; 
                            for(int lv = level; lv >= 0; --lv) {
                                if(lv != level) {
                                    left  <<= 1;
                                    top   <<= 1;
                                    count <<= 1;
                                    mul   <<= 1;
                                }
                                if(!belevel[linktype][lv]) continue;
                                Ctrl** obj_link_map = get_obj_link(linktype, lv);
                                for(int sx = left; sx < (left + count); ++sx) {
                                    for(int sz = top; sz < (top + count); ++sz) {
                                        //LOG(1)("LINK_DEL %d, %d, %d, %d", linktype, lv, sx, sz);
                                        Ctrl *obj = obj_link_map[sz * mul + sx];
                                        while(obj) {
                                            // if(!obj->is_delete() && obj != _obj && obj->plane_ == _obj->plane_ ) {
                                            if(!obj->is_delete() && obj != _obj && _obj->check_plane_for_view( obj ) ) {
                                                //if(obj->is_player() || _obj->is_player()) {
                                                    //LOG(1)("DEL_PLAYER %d", linktype, lv);
                                                //}

                                                _obj->view_remove(obj);
                                                obj->view_remove(_obj);
                                                /*
                                                TEST( 0 )( "VIEW_REMOVE modify_view, A id: %d old: %d new: %d, B id: %d old %d new %d, frame %d",
                                                        _obj->get_id(), _obj->get_link_pos(), _obj->get_new_link_pos(),
                                                        obj->get_id(), obj->get_link_pos(), obj->get_new_link_pos(),
                                                        g_frame );
                                                */
                                            } 
                                            obj = (Ctrl*)obj->get_next_node();
                                        }
                                    }
                                }
                            }   // for(int lv = level; lv >= 0; --lv)
                        }
                    }
                } // for(int x = old_x - r; x < old_x + r; ++x)

                for(int x = new_x - r; x <= new_x + r; ++x) {
                    if(x < 0 || x >= w) continue;
                    for(int z = new_z - r; z <= new_z + r; ++z) {
                        if(z < 0 || z >= h) continue;
                        if(abs(x - old_x) > r || abs(z - old_z) > r) {
                            int left = x;
                            int top = z;
                            int count = 1;
                            int mul = w; 
                            for(int lv = level; lv >= 0; --lv) {
                                if(lv != level) {
                                    left  <<= 1;
                                    top   <<= 1;
                                    count <<= 1;
                                    mul   <<= 1;
                                }
                                if(!belevel[linktype][lv]) continue;
                                Ctrl** obj_link_map = get_obj_link(linktype, lv);
                                for(int sx = left; sx < (left + count); ++sx) {
                                    for(int sz = top; sz < (top + count); ++sz) {
                                        //LOG(1)("LINK_ADD %d, %d, %d, %d", linktype, lv, sx, sz);
                                        Ctrl *obj = obj_link_map[sz * mul + sx];
                                        while(obj) {
                                            // if(!obj->is_delete() && obj != _obj && obj->plane_ == _obj->plane_ ) {
                                            if(!obj->is_delete() && obj != _obj && _obj->check_plane_for_view( obj ) ) {
                                                //if(obj->is_player() || _obj->is_player()) {
                                                    //LOG(1)("ADD_PLAYER %d", linktype, lv);
                                                //}
                                                _obj->view_add(obj);
                                                obj->view_add(_obj);
                                                /*
                                                TEST( 0 )( "VIEW_ADD modify_view, A id: %d old: %d new: %d, B id: %d old %d new %d, frame %d",
                                                        _obj->get_id(), _obj->get_link_pos(), _obj->get_new_link_pos(),
                                                        obj->get_id(), obj->get_link_pos(), obj->get_new_link_pos(),
                                                        g_frame );
                                                */
                                            } 
                                            obj = (Ctrl*)obj->get_next_node();
                                        }
                                    }
                                }
                            }   // for(int lv = level; lv >= 0; --lv)
                        }
                    }
                } // for(int x = new_x - r; x < new_x + r; ++x)

                /*! handle up level */
                
                int t_old_x = old_x;
                int t_old_z = old_z;
                int t_new_x = new_x;
                int t_new_z = new_z;
                int t_w = w;
                int t_h = h;
                for(int linklevel = level + 1; linklevel < DYNAMIC_LINKLEVEL; ++linklevel) {
                    if(!belevel[linktype][linklevel]) break;
                    Ctrl** obj_link_map = get_obj_link(linktype, linklevel);
                    t_old_x >>= 1;
                    t_old_z >>= 1;
                    t_new_x >>= 1;
                    t_new_z >>= 1;
                    t_w >>= 1;
                    t_h >>= 1;
                    if(t_old_x != t_new_x || t_old_z != t_new_z) {
                        _obj->set_center_pos( linklevel, t_new_x, t_new_z );
                        int r = visibility_range_[linklevel];

                        for(int x = t_old_x - r; x <= t_old_x + r; ++x) {
                            if(x < 0 || x >= t_w) continue;
                            for(int z = t_old_z - r; z <= t_old_z + r; ++z) {
                                if(z < 0 || z >= t_h) continue;
                                if(abs(x - t_new_x) > r || abs(z - t_new_z) > r) {
                                    //LOG(1)("2 LINK_DEL %d, %d, %d, %d", linktype, linklevel, x, z);
                                    Ctrl *obj = obj_link_map[z * t_w + x];
                                    while(obj) {
                                        // if(!obj->is_delete() && obj != _obj && obj->plane_ == _obj->plane_ ) {
                                        if(!obj->is_delete() && obj != _obj && _obj->check_plane_for_view( obj ) ) {
                                            //if(obj->is_player() || _obj->is_player()) {
                                                //LOG(1)("UP DEL_PLAYER %d, %d, %d, %d", linktype, linklevel, x, z);
                                            //}
                                            _obj->view_remove(obj);
                                            obj->view_remove(_obj);
                                            /*
                                            TEST( 0 )( "VIEW_REMOVE modify_view, A id: %d old: %d new: %d, B id: %d old %d new %d, frame %d",
                                                    _obj->get_id(), _obj->get_link_pos(), _obj->get_new_link_pos(),
                                                    obj->get_id(), obj->get_link_pos(), obj->get_new_link_pos(),
                                                    g_frame );
                                            */
                                        } 
                                        obj = (Ctrl*)obj->get_next_node();
                                    }
                                }
                            }
                        }

                        for(int x = t_new_x - r; x <= t_new_x + r; ++x) {
                            if(x < 0 || x >= t_w) continue;
                            for(int z = t_new_z - r; z <= t_new_z + r; ++z) {
                                if(z < 0 || z >= t_h) continue;
                                if(abs(x - t_old_x) > r || abs(z - t_old_z) > r) {
                                    //LOG(1)("2 LINK_ADD %d, %d, %d, %d", linktype, linklevel, x, z);
                                    Ctrl *obj = obj_link_map[z * t_w + x];
                                    while(obj) {
                                        // if(!obj->is_delete() && obj != _obj && obj->plane_ == _obj->plane_ ) {
                                        if(!obj->is_delete() && obj != _obj && _obj->check_plane_for_view( obj ) ) {
                                            //if(obj->is_player() || _obj->is_player()) {
                                                //LOG(1)("UP ADD_PLAYER %d, %d, %d, %d", linktype, linklevel, x, z);
                                            //}
                                            _obj->view_add(obj);
                                            obj->view_add(_obj);
                                            /*
                                            TEST( 0 )( "VIEW_ADD modify_view, A id: %d old: %d new: %d, B id: %d old %d new %d, frame %d",
                                                    _obj->get_id(), _obj->get_link_pos(), _obj->get_new_link_pos(),
                                                    obj->get_id(), obj->get_link_pos(), obj->get_new_link_pos(),
                                                    g_frame );
                                            */
                                        } 
                                        obj = (Ctrl*)obj->get_next_node();
                                    }
                                }
                            }
                        }

                    }
                }
            }
        } // for(int linktype = Obj::LinkStatic + 1; linktype < DYNAMIC_LINKLEVEL; ++linktype) 
    }
    return 0;
}

int World::remove_view( Ctrl* _obj )
{
    assert(_obj);
    VECTOR3 pos = _obj->getpos();
    if( _obj->is_player() ) {
        for( int level = 0; level < DYNAMIC_LINKLEVEL; ++level ) {
            int visibility = visibility_range_[level];
            int max_width = get_land_width( level ) * scene_->get_width();
            int max_height = get_land_width( level ) * scene_->get_height();
            int patch_size = get_patch_size(level);
            int cur_x = (int)( pos.x / patch_size );
            int cur_z = (int)( pos.z / patch_size );

            if( cur_x < 0 || cur_x >= max_width || cur_z < 0 || cur_z >= max_height ) {
                ERR(2)("[SENCE](world) World::remove_view() error obj pos(%d,%d), obj:%d", cur_x, cur_z, _obj->get_id());
                return -1;
            }

            for( int type = Obj::LinkStatic + 1; type < DYNAMIC_LINKTYPE; ++type ) {
                Ctrl** obj_link_map = get_obj_link( type, level );
                assert(obj_link_map);
                for( int x = cur_x - visibility; x <= cur_x + visibility; ++x ) {
                    if( x < 0 || x >= max_width ) continue;
                    for( int z = cur_z - visibility; z <= cur_z + visibility; ++z ) {
                        if( z < 0 || z >= max_height ) continue;
                        Ctrl* obj = obj_link_map[ z * max_width + x ];
                        while( obj ) {
                            // if( !obj->is_delete() && obj != _obj && obj->plane_ == _obj->plane_ ) {
                            if( !obj->is_delete() && obj != _obj && _obj->check_plane_for_view( obj ) ) {
                                _obj->view_remove( obj );
                                obj->view_remove( _obj );
                            }
                            obj = (Ctrl*)obj->get_next_node();
                        }
                    }
                }
            }
        }
   } else {
        int level = _obj->get_linklevel();
        int patch_size = get_patch_size(level);
        int cur_x = (int)(pos.x / patch_size );
        int cur_z = (int)(pos.z / patch_size );
        
        int scene_width = scene_->get_width();
        int scene_height = scene_->get_height();
        int max_width = get_land_width( level ) * scene_width;
        int max_height = get_land_width( level ) * scene_height;
        if( cur_x < 0 || cur_x >= max_width || cur_z < 0 || cur_z >= max_height ) {
            ERR(2)("[SENCE](world) World::remove_view() error obj pos(%d,%d), obj:%d", cur_x, cur_z, _obj->get_id());
            return -1;
        }   
        
        int range = visibility_range_[level];
        int tile = (1 << level);
        int minx = (cur_x - range) * tile;
        int minz = (cur_z - range) * tile;
        int num = (range * 2 + 1) * tile;
        
        int max_width_0 = get_land_width( 0 ) * scene_width;
        int max_height_0 = get_land_width( 0 ) * scene_height;
        
        Ctrl** obj_link_map = get_obj_link( Obj::LinkPlayer, 0 );
        for(int x = minx; x < minx + num; ++x) {
            if( x < 0 || x >= max_width_0 ) continue;
            for(int z = minz; z < minz + num; ++z) {
                if( z < 0 || z >= max_height_0 ) continue;
                Ctrl* obj = obj_link_map[ z * max_width_0 + x ];
                while( obj ) {
                    //if( !obj->is_delete() && obj->plane_ == _obj->plane_ ) {
                    if( !obj->is_delete() && _obj->check_plane_for_view( obj ) ) {
                        _obj->view_remove( obj );
                        obj->view_remove( _obj );
                    }
                    obj = (Ctrl*)obj->get_next_node();
                }
            }
        }
    }
    return 0;
}

bool World::is_valid_obj_link_count( int _type, int _level, int _link_pos )
{
    return (obj_link_count_[_type][_level] > _link_pos );
}

/*have bug in else, no use function
 * */
bool World::is_view( int _level1, int _idx1, int _level2, int _idx2 )
{
    int scene_width = scene_->get_width();
    int width1 = get_land_width( _level1 )*scene_width;
    int width2 = get_land_width( _level2 )*scene_width;

    int lv1_x1 = _idx1%width1;
    int lv1_z1 = _idx1/width1;

    int lv2_x2 = _idx2%width2;
    int lv2_z2 = _idx2/width2;

    //TEST( 0 )( "IS_VIEW, x1: %d, z1: %d, lv1: %d, x2: %d, z2: %d, lv2: %d, frame: %d", lv1_x1, lv1_z1, _level1, lv2_x2, lv2_z2, _level2, g_frame );

    if( _level1 == _level2 ) {
        int r = visibility_range_[ _level1 ];
        return abs( lv1_x1-lv2_x2 ) <= r && abs( lv1_z1-lv2_z2 ) <= r;
    }
    else {
        // 如果在不同层次，则其中一层可见，即可见
        int r = 0;
        if( _level1 > _level2 ) {
            int noff1 = _level1-_level2; 
            lv2_x2 = lv2_x2 >> noff1;
            lv2_z2 = lv2_z2 >> noff1;
            r = visibility_range_[ _level1];
        }
        else if( _level1 < _level2 ) {
            int noff1 = _level2-_level1; 
            lv1_x1 = lv1_x1 >> noff1;
            lv1_z1 = lv1_z1 >> noff1;
            r = visibility_range_[_level2];
        }
        return abs( lv1_x1-lv2_x2 ) <= r && abs( lv1_z1-lv2_z2 ) <= r;
        //return ( abs( lv1_x1-lv1_x2 ) <= lv1_r && abs( lv1_z1-lv1_z2 ) <= lv1_r ) || ( abs( lv2_x1-lv2_x2 ) <= lv2_r && abs( lv2_z1-lv2_z2 ) <= lv2_r );
    }
}

