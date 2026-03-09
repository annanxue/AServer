#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include "ffsys.h"

//---------------------------------------------------------------------
// ff_sleep
//---------------------------------------------------------------------

long ff_sleep(long time)
{
#ifdef __linux 	// usleep( time * 1000 );
	usleep((time << 10) - (time << 4) - (time << 3));
#elif defined(_WIN32)
	Sleep(time);
#endif
	return 0;
}



//---------------------------------------------------------------------
// ff_clock
//---------------------------------------------------------------------
int64_t ff_clock(void)
{
#ifdef __linux
	static struct timezone tz={ 0,0 };
	struct timeval time;
	gettimeofday(&time,&tz);
	return (time.tv_sec*1000+time.tv_usec/1000);
#elif defined(_WIN32)
	return clock();
#endif
}

//---------------------------------------------------------------------
// ff_timeofday
//---------------------------------------------------------------------
long ff_timeofday(long *sec, long *usec)
{
#ifdef __linux
	static struct timezone tz={ 0,0 };
	struct timeval time;
	gettimeofday(&time,&tz);
	if (sec) *sec = time.tv_sec;
	if (usec) *usec = time.tv_usec;
#elif defined(_WIN32)
	static long mode = 0, addsec = 0;
#ifdef _MSC_VER
	static __int64 freq = 1;
	__int64 qpc;
#else
	static long long freq = 1;
	long long qpc;
#endif
	if (mode == 0)
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		freq = (freq == 0)? 1 : freq;
		QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
		addsec = (long)time(NULL);
		addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
		mode = 1;
	}
	QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
	if (sec) *sec = (long)(qpc / freq) + addsec;
	if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
#endif

	return 0;
}

#ifdef FF_MULTI_THREAD

//---------------------------------------------------------------------
// ff_thread_create
//---------------------------------------------------------------------
long ff_thread_create(pthread_t* tid, const ff_thread_start tproc, const void *tattr, void *targs)
{
#ifdef __linux
	pthread_attr_t threadattr;
	pthread_attr_init( &threadattr );
//	pthread_attr_setstacksize(&threadattr, (size_t)(64 * 1024 ) );	//for auto player
	long ret = pthread_create(tid, &threadattr, (void*(*)(void*)) tproc, targs);
	pthread_attr_destroy( &threadattr );
	if (ret) return -1;
#elif defined(_WIN32)
	long ret = (int)_beginthread((void(*)(void*))tproc, 0, targs);
	if (tid) *tid = (pthread_t)ret;
	if (ret < 0) return -1;
#endif
	return 0;
}

//---------------------------------------------------------------------
// ff_thread_exit
//---------------------------------------------------------------------
long ff_thread_exit(long retval)
{
#ifdef __linux
	pthread_exit(NULL);
#elif defined(_WIN32)
	_endthread();
#endif
	return 0;
}

//---------------------------------------------------------------------
// ff_thread_join
//---------------------------------------------------------------------
long ff_thread_join(long threadid)
{
	long status;
#ifdef __linux
	status = pthread_join((pthread_t)threadid, (void**)&status);
#elif defined(_WIN32)
	status = WaitForSingleObject((HANDLE)threadid, INFINITE);
#endif
	return status;
}

//---------------------------------------------------------------------
// ff_thread_detach
//---------------------------------------------------------------------
long ff_thread_detach(long threadid)
{
	long status;
#ifdef __linux
	status = pthread_detach((pthread_t)threadid);
#elif defined(_WIN32)
	CloseHandle((HANDLE)threadid);
	status = 0;
#endif
	return status;
}

//---------------------------------------------------------------------
// ff_thread_kill
//---------------------------------------------------------------------
long ff_thread_kill(long threadid)
{
#ifdef __linux
	pthread_cancel((pthread_t)threadid);
#elif defined(_WIN32)
	TerminateThread((HANDLE)threadid, 0);
#endif
	return 0;
}

// multi thread
#endif

char* ff_strcpy( char* _dest, size_t _dest_size, const char* _src )
{
#ifdef __linux
    return strcpy( _dest, _src );
#else
    strcpy_s( _dest, _dest_size, _src );
    return _dest;
#endif
}

char* ff_strncpy( char* _dest, size_t _dest_size, const char* _src, size_t _count )
{
#ifdef __linux
    return strncpy( _dest, _src, _count );
#else
    strncpy_s( _dest, _dest_size, _src, _count );
    return _dest;
#endif
}

int32_t ff_sprintf( char* _dest, size_t _dest_size, const char* _format, ... )
{
    va_list args;
    va_start( args, _format );
#ifdef __linux
    return vsprintf( _dest, _format, args );
#else
    return vsprintf_s( _dest, _dest_size, _format, args );
#endif
}

