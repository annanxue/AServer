#include "thread.h"

namespace AServer {

Thread::Thread()
    : running_(false)
    , detached_(false) {
    memset(thread_name_, 0, sizeof(thread_name_));
}

Thread::~Thread() {
    if (running_ && !detached_) {
#ifdef _WIN32
        CloseHandle(handle_);
#else
        pthread_detach(handle_);
#endif
    }
}

void Thread::start() {
    if (running_) return;
#ifdef _WIN32
    handle_ = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thread_wrapper, this, 0, NULL);
    if (handle_) {
        running_ = true;
    }
#else
    pthread_create(&handle_, NULL, thread_wrapper, this);
    running_ = true;
#endif
}

void Thread::join() {
    if (!running_ || detached_) return;
#ifdef _WIN32
    WaitForSingleObject(handle_, INFINITE);
    CloseHandle(handle_);
#else
    pthread_join(handle_, NULL);
#endif
    running_ = false;
}

void Thread::detach() {
    if (!running_ || detached_) return;
#ifdef _WIN32
    CloseHandle(handle_);
#else
    pthread_detach(handle_);
#endif
    detached_ = true;
}

void Thread::stop() {
    running_ = false;
}

void Thread::set_thread_name(const char* name) {
    strncpy(thread_name_, name, sizeof(thread_name_) - 1);
#ifdef _WIN32
    // SetThreadDescription is only available on Windows 10+
#else
    // pthread_setname_np on Linux
#endif
}

void* Thread::thread_wrapper(void* arg) {
    Thread* thread = static_cast<Thread*>(arg);
    thread->run();
    return nullptr;
}

Mutex::Mutex() {
#ifdef _WIN32
    InitializeCriticalSection(&cs_);
#else
    pthread_mutex_init(&mutex_, NULL);
#endif
}

Mutex::~Mutex() {
#ifdef _WIN32
    DeleteCriticalSection(&cs_);
#else
    pthread_mutex_destroy(&mutex_);
#endif
}

void Mutex::lock() {
#ifdef _WIN32
    EnterCriticalSection(&cs_);
#else
    pthread_mutex_lock(&mutex_);
#endif
}

void Mutex::unlock() {
#ifdef _WIN32
    LeaveCriticalSection(&cs_);
#else
    pthread_mutex_unlock(&mutex_);
#endif
}

bool Mutex::try_lock() {
#ifdef _WIN32
    return TryEnterCriticalSection(&cs_) != 0;
#else
    return pthread_mutex_trylock(&mutex_) == 0;
#endif
}

ReadWriteLock::ReadWriteLock() {
#ifdef _WIN32
    InitializeSRWLock(&srwlock_);
#else
    pthread_rwlock_init(&rwlock_, NULL);
#endif
}

ReadWriteLock::~ReadWriteLock() {
#ifdef _WIN32
    // SRW lock doesn't need explicit destruction
#else
    pthread_rwlock_destroy(&rwlock_);
#endif
}

void ReadWriteLock::read_lock() {
#ifdef _WIN32
    AcquireSRWLockShared(&srwlock_);
#else
    pthread_rwlock_rdlock(&rwlock_);
#endif
}

void ReadWriteLock::read_unlock() {
#ifdef _WIN32
    ReleaseSRWLockShared(&srwlock_);
#else
    pthread_rwlock_unlock(&rwlock_);
#endif
}

void ReadWriteLock::write_lock() {
#ifdef _WIN32
    AcquireSRWLockExclusive(&srwlock_);
#else
    pthread_rwlock_wrlock(&rwlock_);
#endif
}

void ReadWriteLock::write_unlock() {
#ifdef _WIN32
    ReleaseSRWLockExclusive(&srwlock_);
#else
    pthread_rwlock_unlock(&rwlock_);
#endif
}

ThreadLocal::ThreadLocal() {
#ifdef _WIN32
    key_ = TlsAlloc();
#else
    pthread_key_create(&key_, NULL);
#endif
}

ThreadLocal::~ThreadLocal() {
#ifdef _WIN32
    TlsFree(key_);
#else
    pthread_key_delete(key_);
#endif
}

void* ThreadLocal::get() const {
#ifdef _WIN32
    return TlsGetValue(key_);
#else
    return pthread_getspecific(key_);
#endif
}

void ThreadLocal::set(void* value) {
#ifdef _WIN32
    TlsSetValue(key_, value);
#else
    pthread_setspecific(key_, value);
#endif
}

}
