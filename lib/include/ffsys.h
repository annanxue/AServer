#ifndef __FF_SYS_H__
#define __FF_SYS_H__

#define FF_MULTI_THREAD

#include <stddef.h>
#include <stdint.h>

#if defined(__linux) || defined(__linux__)
#define ASERVER_POSIX 1
#define ASERVER_LINUX 1
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define ASERVER_POSIX 1
#define ASERVER_BSD 1
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#elif defined(_WIN32)
#define ASERVER_WINDOWS 1
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <time.h>
#include <process.h>
#include <io.h>
#include <direct.h>
#include <string.h>
#include <signal.h>
#ifndef SIGKILL
#define SIGKILL 9
#endif
#ifndef SIGUSR1
#define SIGUSR1 10
#endif
#ifndef SIGUSR2
#define SIGUSR2 12
#endif
#define localtime_r(t, tm) localtime_s((tm), (t))
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define chdir _chdir
#define getcwd _getcwd
#define WIFEXITED(status) ((status) >= 0)
#define WEXITSTATUS(status) (status)
/* 既有代码以 pthread_t 作为线程句柄类型；Windows 下映射为无符号整数句柄 */
typedef uintptr_t pthread_t;
#define bzero(p, n) memset((p), 0, (n))
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
/* DEBUG 分支把 stdin 设为非阻塞，Windows 下简化为空操作 */
#define ioctl(fd, cmd, arg) ((void)(fd), (void)(cmd), (void)(arg), 0)
#else
#error Unsupported platform: only Windows, Linux and BSD/macOS are supported
#endif

#define FF_THREAD_FUNC

typedef void (FF_THREAD_FUNC *ff_thread_start)(void*);

long ff_sleep(long time); /* ms */
long ff_sleep2(unsigned long time);
int64_t ff_clock(void);
long ff_timeofday(long *sec, long *usec);

long ff_thread_create(pthread_t* tid, const ff_thread_start tproc, const void *tattr, void *targs);
long ff_thread_exit(long retval);
long ff_thread_join(pthread_t threadid);
long ff_thread_detach(pthread_t threadid);
long ff_thread_kill(pthread_t threadid);

char* ff_strcpy(char* dest, size_t dest_size, const char* src);
char* ff_strncpy(char* dest, size_t dest_size, const char* src, size_t count);
int32_t ff_sprintf(char* dest, size_t dest_size, const char* format, ...);

#ifdef ASERVER_WINDOWS
size_t strlcpy(char* dst, const char* src, size_t size);
#endif

typedef void* ff_mutex;

#ifndef FF_MAX_MUTEX
#define FF_MAX_MUTEX 0x10000
#endif

#ifdef ASERVER_POSIX
#define FF_MUTEX_TYPE pthread_mutex_t
#define FF_MUTEX_INIT(p) { \
    pthread_mutexattr_t ma; \
    pthread_mutexattr_init(&ma); \
    pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE); \
    pthread_mutex_init(p, &ma); \
    pthread_mutexattr_destroy(&ma); \
}
#define FF_MUTEX_LOCK(p) pthread_mutex_lock(p)
#define FF_MUTEX_UNLOCK(p) pthread_mutex_unlock(p)
#define FF_MUTEX_TRYLOCK(p) pthread_mutex_trylock(p)
#define FF_MUTEX_DESTROY(p) pthread_mutex_destroy(p)

#define FF_COND_TYPE pthread_cond_t
#define FF_COND_INIT(p) pthread_cond_init(p, 0)
#define FF_COND_WAIT(p, m) pthread_cond_wait(p, m)
#define FF_COND_TIMEDWAIT(p, m, t) pthread_cond_timedwait(p, m, t)
#define FF_COND_TIMED_WAIT(p, m, t) FF_COND_TIMEDWAIT(p, m, t)
#define FF_COND_SIGNAL(p) pthread_cond_signal(p)
#define FF_COND_BROADCAST(p) pthread_cond_broadcast(p)
#define FF_COND_DESTROY(p) pthread_cond_destroy(p)

#define FF_THREAD_SELF(tid) { *(tid) = pthread_self(); }
#else
#define FF_MUTEX_TYPE CRITICAL_SECTION
#define FF_MUTEX_INIT(p) InitializeCriticalSection(p)
#define FF_MUTEX_LOCK(p) EnterCriticalSection(p)
#define FF_MUTEX_UNLOCK(p) LeaveCriticalSection(p)
#define FF_MUTEX_TRYLOCK(p) (TryEnterCriticalSection(p) ? 0 : -1)
#define FF_MUTEX_DESTROY(p) DeleteCriticalSection(p)

#define FF_COND_TYPE CONDITION_VARIABLE
#define FF_COND_INIT(p) InitializeConditionVariable(p)
#define FF_COND_WAIT(p, m) SleepConditionVariableCS((p), (m), INFINITE)
#define FF_COND_TIMED_WAIT(p, m, t) SleepConditionVariableCS((p), (m), (DWORD)(t))
#define FF_COND_TIMEDWAIT(p, m, t) FF_COND_TIMED_WAIT(p, m, t)
#define FF_COND_SIGNAL(p) WakeConditionVariable(p)
#define FF_COND_BROADCAST(p) WakeAllConditionVariable(p)
#define FF_COND_DESTROY(p) ((void)0)

#define FF_THREAD_SELF(tid) { *(tid) = GetCurrentThreadId(); }
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#define ff_snprintf _snprintf
#define ff_vsnprintf _vsnprintf
#else
#define ff_snprintf snprintf
#define ff_vsnprintf vsnprintf
#endif

#ifdef ASERVER_LINUX
#include <sys/prctl.h>
#define SET_THREAD_NAME(name) { prctl(PR_SET_NAME, (name)); }
#else
#define SET_THREAD_NAME(name) {}
#endif

#endif /* __FF_SYS_H__ */
