#include "assert.h"
#include "cpcqueue.h"
#include "misc.h"

PcQueue::PcQueue(int size)
{
#ifdef __linux
	pcqueue_init( &queue, size );
#else
	queue = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
#endif
}

PcQueue::~PcQueue()
{
#ifdef __linux
	pcqueue_destroy( &queue );
#else
	CloseHandle( queue );
#endif
}
	
int PcQueue::get_userdata( void** data, long  time )
{
#ifdef __linux
	return pcqueue_get( &queue, data );
#else
	return GetQueuedCompletionStatus( queue, (unsigned long *)data, &key, &ov, time );
#endif
}

int PcQueue::post_userdata( void* data, long time )
{
#ifdef __linux
	return pcqueue_post( &queue, data );
#else
    PostQueuedCompletionStatus( queue, (ULONG)data, TRUE, NULL );
	return -1;
#endif
}

int PcQueue::get_count()
{
#ifdef __linux
    return pcqueue_count( &queue );
#else
    return 0;
#endif
}

