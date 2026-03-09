#include "timer.h"
#include "log.h"
#include "llog.h"
#include "lua_server.h"
#include "lmisc.h"

extern uint32_t g_frame;
LuaServer* lua_timer = NULL;

const char TimerMng::className[] = "TimerMng";
const bool TimerMng::gc_delete_ = false;

Lunar<TimerMng>::RegType TimerMng::methods[] = {
	LUNAR_DECLARE_METHOD( TimerMng, c_add_timer ),
	LUNAR_DECLARE_METHOD( TimerMng, c_add_timer_second ),
	LUNAR_DECLARE_METHOD( TimerMng, c_del_timer ),
	LUNAR_DECLARE_METHOD( TimerMng, c_mod_timer ),
	LUNAR_DECLARE_METHOD( TimerMng, c_get_frame_rate ),
	LUNAR_DECLARE_METHOD( TimerMng, c_get_frame ),
	LUNAR_DECLARE_METHOD( TimerMng, c_get_ms ),
	LUNAR_DECLARE_METHOD( TimerMng, c_get_remain ),
	{ 0, 0 },
};

TimerMng::TimerMng( int32_t _max, int32_t _frame_rate )
{
    map_timer_.init( 1024*16, _max , "Timer");
    tvecs_[0] = (struct timer_vec*)&tv1_;
    tvecs_[1] = &tv2_;
    tvecs_[2] = &tv3_;    
    tvecs_[3] = &tv4_;
    tvecs_[4] = &tv5_;
        
	int32_t i;
	for (i = 0; i < TVN_SIZE; i++) {
		INIT_LIST_HEAD(tv5_.vec + i);
		INIT_LIST_HEAD(tv4_.vec + i);
		INIT_LIST_HEAD(tv3_.vec + i);
		INIT_LIST_HEAD(tv2_.vec + i);
	}
	
	for (i = 0; i < TVR_SIZE; i++) {
		INIT_LIST_HEAD(tv1_.vec + i);
	}

	timer_jiffies_ = 0;
	tv1_.index = 0;
	tv2_.index = 0;
	tv3_.index = 0;
	tv4_.index = 0;
	tv5_.index = 0;

	index_ = 0;

    max_ = _max;

    INIT_LIST_HEAD( &del_list_ );
    INIT_LIST_HEAD( &free_list_ );
    mem_ = new timer_list[max_];
    for(i = 0; i < max_; ++i) {
        list_add( &mem_[i].list, &free_list_ );    
    }
    frame_rate_ = _frame_rate;
}

TimerMng::~TimerMng()
{ 
    delete [] mem_;
}

int32_t TimerMng::timer_pending (const timer_list * _timer)
{
    return _timer->list.next != 0;
}

void TimerMng::init_timer(timer_list * _timer)
{
	_timer->list.next = _timer->list.prev = 0;
}

void TimerMng::internal_add_timer(timer_list *_timer)
{
	/*
	 * must be cli-ed when calling this
	 */
	uint64_t expires = _timer->expires;
	int64_t idx = expires - timer_jiffies_;
	struct list_head * vec;


	uint32_t i = 0;
    if (idx < 0 ){
		vec = tv1_.vec + tv1_.index;
    }else if (idx < TVR_SIZE) {
		i = expires & TVR_MASK;
		vec = tv1_.vec + i;
	} else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
		i = (expires >> TVR_BITS) & TVN_MASK;
		vec = tv2_.vec + i;
	} else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
		i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		vec =  tv3_.vec + i;
	} else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
		i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
		vec = tv4_.vec + i;
	} else if (idx <= 0xffffffffL) {
		i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		vec = tv5_.vec + i;
	} else {
		/* Can only get here on architectures with 64-bit jiffies */
        ERR(2)("%d:%d TIMER IDX OVERFLOW, idx=%zd, expires=%zd, g_frame=%zd, timer_jiffies=%zd", __FILE__, __LINE__, idx, expires, g_frame, timer_jiffies_);
		INIT_LIST_HEAD(&_timer->list);
		return;
	}
	/*
	 * Timers are FIFO!
	 */
    
	list_add(&_timer->list, vec->prev);
}


void TimerMng::do_del_timer()
{
    while(!list_empty(&del_list_)) {
        list_head* pos = del_list_.next;
        list_del_init(pos);
        timer_list* timer = list_entry(pos, timer_list, list);
        int32_t offset = timer - mem_;
        if(offset >= 0 && offset < max_){
            list_add_tail( &(timer->list), &free_list_ );
        } else {
            delete timer;
        }
    }
}


int32_t TimerMng::del_timer(timer_list * _timer)
{
	map_timer_.del( _timer->index );
    list_del_init(&_timer->list);
    list_add(&_timer->list, &del_list_);
    _timer->expires = 0;
    _timer->cycle = 0;
    return 1;
}

int TimerMng::mod_timer( timer_list* _timer, unsigned long _expires )
{
    _timer->expires = _expires + g_frame;
    list_del_init( &_timer->list );
    internal_add_timer( _timer );
    return 1;
}

int TimerMng::mod_timer( int _index, unsigned long _expires )
{
    timer_list* timer;
    if( map_timer_.find( _index, (intptr_t&)timer ) ) { 
        return mod_timer( timer, _expires );
    }   
    return -1; 
}

int32_t TimerMng::del_timer( int32_t _index )
{
	intptr_t timer;
	if( map_timer_.find( _index, timer ) ) {
		return del_timer( (timer_list*)timer );
	}
	return -1;
}


int64_t TimerMng::get_remain( int32_t _index )
{
    intptr_t timer;
    uint64_t remain = 0;
    if( map_timer_.find( _index, timer) ) {
        remain = ((timer_list*)timer)->expires - g_frame;
    }
    return remain;
}


void TimerMng::cascade_timers(struct timer_vec *_tv)
{
	/* cascade all the timers from tv up one level */
	struct list_head *head, *curr, *next;

	head = _tv->vec + _tv->index;
	curr = head->next;
	/*
	 * We are removing _all_ timers from the list, so we don't  have to
	 * detach them individually, just clear the list afterwards.
	 */
	while (curr != head) {
		timer_list *tmp;

		tmp = list_entry(curr, timer_list, list);
		next = curr->next;
		list_del_init(curr); // not needed
		internal_add_timer(tmp);
		curr = next;
	}
	INIT_LIST_HEAD(head);
	_tv->index = (_tv->index + 1) & TVN_MASK;
}

void TimerMng::run_timer_list(void)
{
    do_del_timer();
	while( g_frame >= timer_jiffies_ ) {
		struct list_head *head, *curr;
		if (!tv1_.index) {
			int32_t n = 1;
			do {
				cascade_timers(tvecs_[n]);
			} while (tvecs_[n]->index == 1 && ++n < NOOF_TVECS);
		}
repeat:
		head = tv1_.vec + tv1_.index;
		curr = head->next;
		if (curr != head) {
			timer_list *timer;
			timer = list_entry(curr, timer_list, list);
            list_del_init(curr);

#ifdef __linux
            TICK(A)
#endif
			int32_t rt = handle_timer( timer );
#ifdef __linux
            TICK(B)
            mark_tick( TICK_TYPE_TIMER+timer->id, A, B );
#endif

            if ( rt == 0 || timer->cycle <= 0 )
            {
                del_timer( timer->index ); 
            }
            else
            {
				timer->expires = timer_jiffies_ + timer->cycle ;
				internal_add_timer(timer );
            }

			goto repeat;
		}
		++timer_jiffies_; 
		tv1_.index = (tv1_.index + 1) & TVR_MASK;
	}
}


int32_t TimerMng::add_timer( uint32_t _expires, uint32_t _id, double _data, double _data2, int32_t _cycle, timer_func _handler )
{
    timer_list* timer = NULL;
    if(!list_empty(&free_list_) ){
        list_head* pos = free_list_.next;
        list_del_init(pos);
        timer = list_entry(pos, struct timer_list, list);
    } else {
        timer = new timer_list;
    }   

    if( timer )
    {
        timer->expires = _expires + g_frame ;
		timer->id = _id;
		timer->data = _data;
		timer->data2 = _data2;
		timer->cycle = _cycle;
		timer->index = index_++;
        timer->handler = _handler;
		internal_add_timer( timer );

		map_timer_.set( timer->index, (intptr_t)timer );
		return timer->index;
    }
    return -1;
}


int32_t TimerMng::handle_timer( struct timer_list * _timer )
{
	int32_t rt = 1;
	if( _timer->id <= 0x0000FFFF ) /*! handle by script*/
	{
        if (!lua_timer)
        {
#ifndef __EDITOR
            TRACE(2)("NEED INIT lua_timer");
#endif
            return 0; 
        }
		lua_State* _L = lua_timer->L();
		assert( _L != NULL );
        int32_t timer_ref = lua_timer->get_timer_ref();
        if (!timer_ref)
            return 0;

		lua_timer->get_lua_ref( timer_ref );
		lua_pushnumber( _L, _timer->id );
		lua_pushnumber( _L, _timer->data );
		lua_pushnumber( _L, _timer->data2 );
        lua_pushnumber( _L, _timer->index );

        if( llua_call( _L, 4, 1, 0 ) == 0) {
            rt = (int)lua_tonumber( _L, -1 );
            lua_pop( _L, 1 );         
        } else {         
            ERR(2)("[TIMER]handler_timer failed, timerid: %d, data1: %f, data2: %f, index: %d", _timer->id, _timer->data, _timer->data2, _timer->index );
            llua_fail( _L, __FILE__, __LINE__ ); 
        }
	}
	else /*! handle by engine */
	{
        if( _timer->handler ) {
#ifndef __EDITOR
            TRACE(2)( "[TIMER](handle timer) handle_timer, id = 0x%08X, data1 = %f, cycle = %zd", _timer->id, _timer->data, _timer->cycle );	
#endif
            return _timer->handler( _timer->data, _timer->data2 );
        }
	}
	
	return rt;
	
}

TimerMng* g_timermng;

int32_t TimerMng::c_add_timer( lua_State* _L )
{
	lcheck_argc( _L, 5 );

	uint32_t expires = ( uint32_t )lua_tonumber( _L, 1 );
	uint32_t id = ( uint32_t )lua_tonumber( _L, 2 );
	double data = lua_tonumber( _L, 3 );
	double data2 = lua_tonumber( _L, 4 );
	int32_t cycle = ( int32_t )lua_tonumber( _L, 5 );

	lua_pushnumber( _L, add_timer( expires, id, data, data2, cycle ) );
	return 1;
}

int32_t TimerMng::c_add_timer_second( lua_State* _L )
{
	lcheck_argc( _L, 5 );

	uint32_t expires = ( uint32_t )lua_tonumber( _L, 1 );
	uint32_t id = ( uint32_t )lua_tonumber( _L, 2 );
	double data = lua_tonumber( _L, 3 );
	double data2 = lua_tonumber( _L, 4 );
	int32_t cycle = ( int32_t )lua_tonumber( _L, 5 );

    expires *= frame_rate_;
    cycle *= frame_rate_;
	lua_pushnumber( _L, add_timer( expires, id, data, data2, cycle ) );
	
    return 1;
}

int32_t TimerMng::c_del_timer( lua_State* _L )
{
	lcheck_argc( _L, 1 );

    if( lua_isnil( _L, 1 ) )
    {
        c_bt( _L );
        return 0;
    }

	int32_t index = ( int32_t )lua_tonumber( _L, 1 );
	lua_pushnumber( _L, del_timer( index ) );
	return 1;
}

int TimerMng::c_mod_timer( lua_State* _L ) 
{
    lcheck_argc( _L, 2 ); 
    int index = (int)lua_tonumber( _L, 1 ); 
    unsigned long expires = (unsigned long)lua_tonumber( _L, 2 ); 
    int rt = mod_timer( index, expires );
    lua_pushnumber( _L, rt );
    return 1;
}

int32_t TimerMng::c_get_frame_rate( lua_State* _L )
{
    lua_pushnumber( _L, frame_rate_ );
    return 1;
}

int32_t TimerMng::c_get_frame( lua_State* _L )
{
    lua_pushnumber( _L, g_frame );
    return 1;
}

int32_t TimerMng::c_get_ms( lua_State* _L )
{
	lua_pushnumber( _L, g_frame * (1000.0f / frame_rate_));
	return 1;
}

int32_t TimerMng::c_get_remain( lua_State* _L )
{
	lcheck_argc( _L, 1 );

	int32_t index = ( int32_t )lua_tonumber( _L, 1 );
    lua_pushnumber( _L, get_remain(index) );
    return 1;
}

uint32_t TimerMng::get_ms()
{
	return g_frame * (1000.0f / frame_rate_);
}


