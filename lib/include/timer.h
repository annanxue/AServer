#ifndef __TIMER_MNG_H__
#define __TIMER_MNG_H__

#include "mylist.h"
#include "mymap32.h"
#include "lunar.h"
#include <stdint.h>

#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)
#define NOOF_TVECS 5

typedef int32_t ( *timer_func )( uint64_t p1, uint64_t p2 );

struct timer_list{
	struct list_head list;
	uint32_t expires;
	uint32_t id;
	double data;
	double data2;
	int32_t cycle;
    timer_func handler;
	int32_t index;
};
	
struct timer_vec {
	int32_t index;
	struct list_head vec[TVN_SIZE];
};

struct timer_vec_root {
	int32_t index;
	struct list_head vec[TVR_SIZE];
};

class TimerMng
{
public:
    TimerMng(int32_t _max = 1024, int32_t _framerate = 10);
    ~TimerMng();
    int32_t add_timer( uint32_t _expires, uint32_t _id, double _data, double _data2, int32_t _cycle, timer_func _handler = 0 );
    void run_timer_list(void);

	int32_t del_timer( int32_t _index );
    int64_t get_remain( int32_t _index );
    int get_frame_rate() { return frame_rate_; }
    int conv_ms_2_frame(int _ms) { return frame_rate_ * _ms / 1000; }
    int get_ms_one_frame() { return 1000/frame_rate_; }
	uint32_t get_ms();
    int mod_timer( int _index, unsigned long _expires );
    
private:
    int mod_timer( struct timer_list* _timer, unsigned long _expires );
    int32_t handle_timer( struct timer_list * _timer );
    inline int32_t timer_pending (const struct timer_list * _timer);
    inline void init_timer( struct timer_list * _timer);
    void internal_add_timer( struct timer_list *_timer);
    int32_t del_timer( struct timer_list * _timer);
    void cascade_timers(struct timer_vec *_tv);
    void do_del_timer();
    
private:
    struct timer_vec tv5_;
    struct timer_vec tv4_;
    struct timer_vec tv3_;
    struct timer_vec tv2_;
    struct timer_vec_root tv1_;
    uint32_t timer_jiffies_;    
    
    struct timer_vec * tvecs_[5];
    
    list_head free_list_;
    list_head del_list_;

    
    timer_list* mem_;
    int32_t max_;

	//MyMap< struct timer_list* > map_timer;
	MyMap32 map_timer_;
	int	index_;
    int frame_rate_;

public:
	static const char className[];
	static Lunar<TimerMng>::RegType methods[];
    static const bool gc_delete_;

	int32_t c_add_timer( lua_State* _L );
	int32_t c_add_timer_second( lua_State* _L );
	int32_t c_del_timer( lua_State* _L );
	int32_t c_mod_timer( lua_State* _L );
    int32_t c_get_frame_rate( lua_State* _L );
    int32_t c_get_frame( lua_State* _L );
	int32_t c_get_ms( lua_State* _L );
	int32_t c_get_remain( lua_State* _L );
};

extern TimerMng* g_timermng;

#endif

