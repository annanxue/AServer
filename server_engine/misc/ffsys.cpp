#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include "ffsys.h"

long ff_sleep(long time)
{
#ifdef ASERVER_POSIX
    usleep((unsigned long)time * 1000);
#elif defined(ASERVER_WINDOWS)
    Sleep(time);
#endif
    return 0;
}

long ff_sleep2(unsigned long time)
{
    return ff_sleep((long)time);
}

int64_t ff_clock(void)
{
#ifdef ASERVER_POSIX
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#elif defined(ASERVER_WINDOWS)
    return (int64_t)GetTickCount64();
#endif
}

long ff_timeofday(long *sec, long *usec)
{
#ifdef ASERVER_POSIX
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (sec) *sec = (long)tv.tv_sec;
    if (usec) *usec = (long)tv.tv_usec;
#elif defined(ASERVER_WINDOWS)
    static long mode = 0;
    static long addsec = 0;
    static int64_t freq = 1;
    int64_t qpc = 0;
    if (mode == 0)
    {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        freq = li.QuadPart ? li.QuadPart : 1;
        QueryPerformanceCounter(&li);
        qpc = li.QuadPart;
        addsec = (long)time(NULL) - (long)((qpc / freq) & 0x7fffffff);
        mode = 1;
    }
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    qpc = li.QuadPart;
    if (sec) *sec = (long)(qpc / freq) + addsec;
    if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
#endif
    return 0;
}

#ifdef FF_MULTI_THREAD

long ff_thread_create(pthread_t* tid, const ff_thread_start tproc, const void *tattr, void *targs)
{
    (void)tattr;
#ifdef ASERVER_POSIX
    pthread_attr_t threadattr;
    pthread_attr_init(&threadattr);
    int ret = pthread_create(tid, &threadattr, (void*(*)(void*))tproc, targs);
    pthread_attr_destroy(&threadattr);
    return ret ? -1 : 0;
#elif defined(ASERVER_WINDOWS)
    uintptr_t ret = _beginthread((void (*)(void*))tproc, 0, targs);
    if (ret == (uintptr_t)-1) return -1;
    if (tid) *tid = ret;
    return 0;
#endif
}

long ff_thread_exit(long retval)
{
    (void)retval;
#ifdef ASERVER_POSIX
    pthread_exit(NULL);
#elif defined(ASERVER_WINDOWS)
    _endthread();
#endif
    return 0;
}

long ff_thread_join(pthread_t threadid)
{
#ifdef ASERVER_POSIX
    return (long)pthread_join(threadid, NULL);
#elif defined(ASERVER_WINDOWS)
    return (long)WaitForSingleObject((HANDLE)threadid, INFINITE);
#endif
}

long ff_thread_detach(pthread_t threadid)
{
#ifdef ASERVER_POSIX
    return (long)pthread_detach(threadid);
#elif defined(ASERVER_WINDOWS)
    CloseHandle((HANDLE)threadid);
    return 0;
#endif
}

long ff_thread_kill(pthread_t threadid)
{
#ifdef ASERVER_POSIX
    pthread_cancel(threadid);
#elif defined(ASERVER_WINDOWS)
    TerminateThread((HANDLE)threadid, 0);
#endif
    return 0;
}

#endif /* FF_MULTI_THREAD */

char* ff_strcpy(char* dest, size_t dest_size, const char* src)
{
    (void)dest_size;
    return strcpy(dest, src);
}

char* ff_strncpy(char* dest, size_t dest_size, const char* src, size_t count)
{
    (void)dest_size;
    return strncpy(dest, src, count);
}

#ifdef ASERVER_WINDOWS
size_t strlcpy(char* dst, const char* src, size_t size)
{
    size_t src_len = strlen(src);
    if (size > 0)
    {
        size_t copy_len = (src_len >= size) ? size - 1 : src_len;
        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }
    return src_len;
}
#endif

int32_t ff_sprintf(char* dest, size_t dest_size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
#ifdef _MSC_VER
    int ret = vsnprintf_s(dest, dest_size, _TRUNCATE, format, args);
#else
    int ret = vsnprintf(dest, dest_size, format, args);
#endif
    va_end(args);
    return ret;
}
