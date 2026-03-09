/*
* pcqueue is short name for bounded productor/consumer queue.
* There is a fixed size array internal the pcqueue, storing the
* data pointers. When a productor thread pushes the data item into
* the queue, if the queue is full, it will be blocked until the 
* queue has at least one empty item in the array. When a consumer 
* thread gets an item of data from the queue, vice versa, if the
* queue is empty, it will be blocked until the queue has at least
* one item.
* pcqueue is similar to IOCP under win32 platform, in some aspect.
*/
#ifndef __PCQUEUE_H__
#define __PCQUEUE_H__

#ifdef __linux

#include <pthread.h>

#define INFINITE (int)-1
#define TIMEOUT (int)-1

#ifdef __cplusplus
extern "C" {
#endif 

/*
* define the data entry of pcqueue
*/
typedef struct
{
	int _size;					// data array size
	int _first;					// first item position in array
	int _end;					// last item position in array
	int _count;					// total item counts in array
	pthread_mutex_t _mutex;
	pthread_cond_t _cond_full;
	pthread_cond_t _cond_empty;
	void** _userdata;			// pointer to array
} pcqueue_t;

/*
* init the pcqueue_t entry
* malloc memory for pcqueue_t and init the memory block
*/
int pcqueue_init( pcqueue_t* , int size );

/*
* puts an item of data into the queue
* would block if the queue is full
*/
int pcqueue_post( pcqueue_t*, void* );

/*
* gets an item of data from the queue
* would block if the queue is empty
*/
int pcqueue_get( pcqueue_t*, void** );

/*
* return the size of the queue
*/
int pcqueue_size( pcqueue_t* );

/*
* return the count of items in the queue
*/
int pcqueue_count( pcqueue_t* );

/*
* release the resource of the pcqueue
*/
int pcqueue_destroy( pcqueue_t* );

#ifdef __cplusplus
}
#endif 

#endif	// __linux

#endif
