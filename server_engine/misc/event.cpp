#ifdef __linux

#include "event.h"

Event::Event(int size):size_(size)
{
	if( size > MAX_EVENTS )
	{
		size = 0;
	}

	memset( events_, 0, MAX_EVENTS );
	pthread_cond_init( &cond_, 0 );
	pthread_mutex_init( &mutex_, 0 );
}


Event::~Event()
{
	pthread_cond_destroy( &cond_ );
	pthread_mutex_destroy( &mutex_ );
}

void Event::set_event(int idx)
{
	if( idx < 0 || idx >= size_ )
	{
		return;
	}

	pthread_mutex_lock( &mutex_ );
	events_[idx] = 1;
	pthread_cond_broadcast( &cond_ );	
	pthread_mutex_unlock( &mutex_ );
}


void Event::wait_event(int idx)
{
	if( idx < 0 || idx >= size_ )
	{
		return;
	}

	pthread_mutex_lock( &mutex_ );
	while( events_[idx] == 0 )
	{
		pthread_cond_wait( &cond_, &mutex_ );
	}
	events_[idx] = 0;
	pthread_mutex_unlock( &mutex_ );
}


void Event::wait_all_event()
{
	for(int i = 0; i < size_; ++i)
	{
		wait_event(i);
	}
}


#endif //__linux
