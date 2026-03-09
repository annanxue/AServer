#ifndef __FF_SYS_H__
#define __FF_SYS_H__

#define FF_MULTI_THREAD

#if defined(__linux)
#include <sys/time.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#elif defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <windows.h>
#include <time.h>
#include <stddef.h>
#include <process.h>

#ifdef _MSC_VER
#ifndef _MT
#undef FF_MULTI_THREAD
#endif
#endif

#else
#error Unknow platform, only support linux and win32
#endif

/*
#ifdef __cplusplus
extern "C" {
#endif
*/

#define FF_THREAD_FUNC

#ifndef __linux
typedef __int64 int64_t;
#endif

// datetime interface
long ff_sleep(long time); // ms
long ff_sleep2(unsigned long time);
int64_t ff_clock(void);
// since 1970
long ff_timeofday(long *sec, long *usec);


typedef void (FF_THREAD_FUNC *ff_thread_start)(void*);

// thread operation
long  ff_thread_create(pthread_t* tid, const ff_thread_start tproc, const void *tattr, void *targs);
long  ff_thread_exit(long retval);
long  ff_thread_join(long threadid);
long  ff_thread_detach(long threadid);
long  ff_thread_kill(long threadid);

char* ff_strcpy( char* _dest, size_t _dest_size, const char* _src );
char* ff_strncpy( char* _dest, size_t _dest_size, const char* _src, size_t _count );
int32_t ff_sprintf( char* _dest, size_t _dest_size, const char* _format, ... );
// char* strlcpy( char* _dest, const char* _src, size_t _n  );

// mutex
typedef void* ff_mutex;

#ifndef FF_MAX_MUTEX
#define FF_MAX_MUTEX 0x10000
#endif


#ifdef __linux
#define FF_MUTEX_TYPE				pthread_mutex_t
#define FF_MUTEX_INIT(p)			{\
										pthread_mutexattr_t ma; \
										pthread_mutexattr_init( &ma ); \
										pthread_mutexattr_settype( &ma, PTHREAD_MUTEX_RECURSIVE ); \
										pthread_mutex_init(p, &ma); \
										pthread_mutexattr_destroy( &ma ); \
									}
#define FF_MUTEX_LOCK(p)			pthread_mutex_lock(p)
#define FF_MUTEX_UNLOCK(p)			pthread_mutex_unlock(p)
#define FF_MUTEX_TRYLOCK(p)			pthread_mutex_trylock(p)
#define FF_MUTEX_DESTROY(p)			pthread_mutex_destroy(p)

#define FF_COND_TYPE				pthread_cond_t
#define FF_COND_INIT(p)				pthread_cond_init(p, 0 )
#define FF_COND_WAIT(p, m)			pthread_cond_wait(p, m)
#define FF_COND_TIMEDWAIT(p, m, t)	pthread_cond_timedwait( p, m, t )
#define FF_COND_SIGNAL(p)			pthread_cond_signal(p)
#define FF_COND_DESTROY(p)			pthread_cond_destroy(p)

#define FF_THREAD_SELF(tid)			{ *tid = pthread_self(); }

#else

#define FF_MUTEX_TYPE				CRITICAL_SECTION
#define FF_MUTEX_INIT(p)			InitializeCriticalSection(p)
#define FF_MUTEX_LOCK(p)			EnterCriticalSection(p)
#define FF_MUTEX_UNLOCK(p)			LeaveCriticalSection(p)
#define FF_MUTEX_TRYLOCK(p)			((TryEnterCriticalSection(p))? 0 : -1)
#define FF_MUTEX_DESTROY(p)			DeleteCriticalSection(p)

#define FF_COND_TYPE				HANDLE
#define FF_COND_INIT(p)				{ *(p) = CreateEvent( NULL, FALSE, FALSE, NULL ); }
#define FF_COND_WAIT(p, m)			WaitForSingleObject( *(p), INFINITE )
#define FF_COND_TIMED_WAIT(p, m, t)	WaitForSingleObject( *(p), t )
#define FF_COND_SIGNAL(p)			SetEvent( *(p) )
#define FF_COND_DESTROY(p)			CLOSE_HANDLE( *(p) )

#define FF_THREAD_SELF(tid)			{ *tid = GetCurrentThread(); }
#endif

#ifdef __linux
#define ff_snprintf					snprintf
#define ff_vsnprintf				vsnprintf
#else
#define ff_snprintf					_snprintf
#define ff_vsnprintf				_vsnprintf
#endif

/*
#ifdef __cplusplus
}
#endif
*/


#endif

#ifdef __linux
#include <linux/version.h>
#include <sys/prctl.h>
#if LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,9)
#define SET_THREAD_NAME(name){ prctl( PR_SET_NAME, name ); }
#else
#define SET_THREAD_NAME(name){}
#endif
#else
#define SET_THREAD_NAME(name){}
#endif



