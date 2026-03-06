#ifndef ASERVER_THREAD_H
#define ASERVER_THREAD_H

#include "define.h"
#include <thread>
#include <atomic>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace AServer {

class Thread {
public:
    Thread();
    virtual ~Thread();

    virtual void run() = 0;
    void start();
    void join();
    void detach();
    void stop();
    bool is_running() const { return running_; }
    void set_thread_name(const char* name);

private:
    static void* thread_wrapper(void* arg);

#ifdef _WIN32
    HANDLE handle_;
#else
    pthread_t handle_;
#endif
    bool running_;
    bool detached_;
    char thread_name_[32];
};

class Mutex {
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();
    bool try_lock();

private:
#ifdef _WIN32
    CRITICAL_SECTION cs_;
#else
    pthread_mutex_t mutex_;
#endif
};

class AutoLock {
public:
    explicit AutoLock(Mutex& mutex) : mutex_(mutex) { mutex_.lock(); }
    ~AutoLock() { mutex_.unlock(); }

private:
    Mutex& mutex_;
    AutoLock(const AutoLock&) = delete;
    AutoLock& operator=(const AutoLock&) = delete;
};

class ReadWriteLock {
public:
    ReadWriteLock();
    ~ReadWriteLock();

    void read_lock();
    void read_unlock();
    void write_lock();
    void write_unlock();

private:
#ifdef _WIN32
    SRWLOCK srwlock_;
#else
    pthread_rwlock_t rwlock_;
#endif
};

class ThreadLocal {
public:
    ThreadLocal();
    ~ThreadLocal();

    void* get() const;
    void set(void* value);

private:
#ifdef _WIN32
    DWORD key_;
#else
    pthread_key_t key_;
#endif
};

class AtomicCounter {
public:
    AtomicCounter(int32_t initial = 0) : counter_(initial) {}

    int32_t increment() {
#ifdef _WIN32
        return InterlockedIncrement((LONG*)&counter_);
#else
        return __sync_add_and_fetch(&counter_, 1);
#endif
    }

    int32_t decrement() {
#ifdef _WIN32
        return InterlockedDecrement((LONG*)&counter_);
#else
        return __sync_sub_and_fetch(&counter_, 1);
#endif
    }

    int32_t get() const {
#ifdef _WIN32
        return InterlockedAdd((LONG*)&counter_, 0);
#else
        return counter_;
#endif
    }

    void set(int32_t value) {
#ifdef _WIN32
        InterlockedExchange((LONG*)&counter_, value);
#else
        __sync_synchronize();
        counter_ = value;
#endif
    }

private:
    volatile int32_t counter_;
};

}

#endif
