#ifndef __CPCQUEUE_H__
#define __CPCQUEUE_H__

#ifdef __linux
#include "pcqueue.h"	
#elif defined(_WIN32)
#include "Windows.h"
#endif


class PcQueue
{
public:
	PcQueue(int size);
	~PcQueue();

	/*
	* get_userdata() return an item from the queue
	* if the quueue is empty, would block for time miliseconds at most
	*/	
	int get_userdata( void**, long time = INFINITE );

	/*
	* post_userdata() put an item from to queue
	* if the queue is full, would block for time miliseconds at most
	*/	
	int post_userdata( void*, long time = INFINITE );
	
	int get_count();
private:
#ifdef __linux
	pcqueue_t queue;
#elif defined(_WIN32)
	unsigned long key;
	LPOVERLAPPED ov;
	HANDLE queue;
#endif
};

#endif
