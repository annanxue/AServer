#ifdef __linux

#include <error.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include "pcqueue.h"
#include <stdio.h>

int pcqueue_init( pcqueue_t* queue, int size )
{
	assert( queue );
	queue->_size = size;
	queue->_userdata = (void**)malloc( sizeof(void*) * size );
	pthread_mutex_init( &(queue->_mutex), (void*)0 );
	pthread_cond_init( &(queue->_cond_full ), (void*)0 );
	pthread_cond_init( &(queue->_cond_empty ), (void*)0 );
	queue->_first = 0;
	queue->_end = 0;
	queue->_count = 0;
	return 0;
}


int pcqueue_post( pcqueue_t* queue, void* userdata )
{
	assert( queue != (void*)0 );
	pthread_mutex_lock( &(queue->_mutex) );
	while ( queue->_count == queue->_size )	//CAUTION: must use while here
	{
		pthread_cond_wait( &(queue->_cond_full), &(queue->_mutex) );
	}
	queue->_userdata[queue->_end] = userdata;
	queue->_end++;
	if( queue->_end >= queue->_size )
	{
		queue->_end -= queue->_size;
	}
	queue->_count++;
	assert( queue->_count <= queue->_size );
	if( queue->_count == 1 )
	{
		pthread_cond_broadcast( &(queue->_cond_empty) );
	}
	pthread_mutex_unlock( &(queue->_mutex) );
	return 0;
}

int pcqueue_get( pcqueue_t* queue, void** userdata_ptr)
{
	assert( queue != (void*)0 );
	pthread_mutex_lock( &(queue->_mutex) );
	while ( queue->_count == 0 )	//CAUTION: must use while here
	{
		pthread_cond_wait( &(queue->_cond_empty), &(queue->_mutex) );
	}
	if ( userdata_ptr )
		*userdata_ptr = queue->_userdata[queue->_first];
	queue->_first++;
	if( queue->_first >= queue->_size )
	{
		queue->_first -= queue->_size;
	}
	queue->_count--;
	assert( queue->_count >= 0 );
	if( queue->_count == queue->_size -1 )
	{
		pthread_cond_broadcast( &(queue->_cond_full) );
	}
	pthread_mutex_unlock( &(queue->_mutex) );
	return 0;
}

int pcqueue_size( pcqueue_t* queue )
{
	assert( queue != (void*)0 );
	int ret = 0;
	pthread_mutex_lock( &(queue->_mutex) );
	ret = queue->_size;
	pthread_mutex_unlock( &(queue->_mutex) );
	return ret;
}

int pcqueue_count( pcqueue_t* queue )
{
	assert( queue != (void*)0 );
	int ret = 0;
	pthread_mutex_lock( &(queue->_mutex) );
	ret = queue->_count;
	pthread_mutex_unlock( &(queue->_mutex) );
	return ret;
}

int pcqueue_destroy( pcqueue_t* queue )
{
	assert( queue != (void*)0 );
	pthread_mutex_lock( &(queue->_mutex) );
	free( queue->_userdata );
	pthread_mutex_unlock( &(queue->_mutex) );
	pthread_cond_destroy( &(queue->_cond_empty) );
	pthread_cond_destroy( &(queue->_cond_full) );
	pthread_mutex_destroy( &(queue->_mutex) );
//	free( queue );
	return 0;
}

#endif

