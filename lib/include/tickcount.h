#ifndef __TICKCOUNT_H__
#define	__TICKCOUNT_H__

#include "autolock.h"
#include "log.h"

class TickCount
{
public:
	TickCount();
	~TickCount()	{}

	static TickCount*	get_instance();
	u_long				get_tickcount( void ) const;
	u_long				get_offset( const int64_t& _start ) const;
private:
	int64_t				base_time_;	//起始时间
};


inline TickCount::TickCount()
{
	base_time_ = ff_clock();
}


inline TickCount* TickCount::get_instance()
{
	static TickCount tc;
	return &tc;
}


inline u_long TickCount::get_tickcount( void ) const
{
	return (u_long)(ff_clock() - base_time_);
}


inline u_long TickCount::get_offset( const int64_t& _start ) const
{
	return (u_long)(ff_clock() - _start);
}


#endif
