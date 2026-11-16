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
	DWORD bytes = 0;
	LPOVERLAPPED completion = NULL;
	BOOL ok = GetQueuedCompletionStatus( queue, &bytes, &key, &completion, (DWORD)time );
	if ( ok && completion )
	{
		*data = (void*)completion;
		return 0;
	}
	return -1;
#endif
}

int PcQueue::post_userdata( void* data, long time )
{
#ifdef __linux
	return pcqueue_post( &queue, data );
#else
	return PostQueuedCompletionStatus( queue, 0, 0, (LPOVERLAPPED)data ) ? 0 : -1;
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

