#include <assert.h>
#include "autolock.h"


Mutex::Mutex()
{
	FF_MUTEX_INIT( &mutex_ );
}

Mutex::~Mutex()
{
	FF_MUTEX_DESTROY( &mutex_ );
}

int Mutex::Lock()
{
	FF_MUTEX_LOCK( &mutex_ );
	return 0;
}

int Mutex::Unlock()
{
	FF_MUTEX_UNLOCK( &mutex_ );
	return 0;
}

int Mutex::Trylock()
{
	return FF_MUTEX_TRYLOCK( &mutex_ );
}

AutoLock::AutoLock( Mutex* m )
{
	mutex = m;
	if( m )
	{
		m->Lock();
	}
}

AutoLock::~AutoLock()
{
	if( mutex )
	{
		mutex->Unlock();
	}
}

AutoUnlock::AutoUnlock( Mutex* m )
{
	mutex_ = m;
}

AutoUnlock::~AutoUnlock()
{
	if( mutex_ )
	{
		mutex_->Unlock();
	}
}

