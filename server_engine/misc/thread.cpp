#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include "misc.h"
#include "thread.h"

void thread_start(void *p)
{
	Thread *thr = (Thread*)p;
	assert( thr );
    SET_THREAD_NAME( thr->thread_name_ );
	thr->active_ = TRUE;
	thr->run();
}

Thread::Thread()
{
	active_ = FALSE;
}

Thread::~Thread()
{
	if( active_ )
		stop();
}

void Thread::join()
{
	ff_thread_join( thread_id_ );
}

int Thread::start()
{
	if( ff_thread_create(&(this->thread_id_), thread_start, NULL, this) < 0 )
	{
		return -1;
	}

	return 0;
}

int Thread::stop()
{
	if( active_ )
	{
		active_ = FALSE;
		this->join();
	}
	return 0;
}
