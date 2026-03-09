#ifndef __AUTOLOCK_H__
#define __AUTOLOCK_H__

#include "ffsys.h"

class Mutex
{
public:
	Mutex();
	~Mutex();

	int Lock();
	int Unlock();
	int Trylock();

private:
	FF_MUTEX_TYPE mutex_;
};

class AutoLock
{
public:
	AutoLock(Mutex* mutex);
	~AutoLock();

private:
	Mutex* mutex;
	
};

class AutoUnlock
{
public:
	AutoUnlock( Mutex* m ); 
	~AutoUnlock();
private:
	Mutex* mutex_;
};

#endif
