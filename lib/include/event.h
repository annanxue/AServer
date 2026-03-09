#ifndef __EVENT_H__
#define __EVENT_H__

#ifdef __linux

#include "ffsys.h"
#include <string.h>

#define MAX_EVENTS			256

class Event
{
public:
	Event(int size = 1);
	~Event();

	void				set_event(int idx = 0);
	void				wait_event(int idx = 0);
	void				wait_all_event();
private:
	int					size_;
	char				events_[MAX_EVENTS];
	pthread_mutex_t		mutex_;
	pthread_cond_t		cond_;
};

#endif //__linux

#endif //__EVENT_H__
