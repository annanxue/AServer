#ifndef ASERVER_SINGLETON_H
#define ASERVER_SINGLETON_H

#include "define.h"
#include "thread.h"
#include <memory>
#include <mutex>

namespace AServer {

template<typename T>
class Singleton {
public:
    static T* instance_ptr() {
        if (!instance_) {
            std::lock_guard<Mutex> lock(mutex_);
            if (!instance_) {
                instance_ = new T();
            }
        }
        return instance_;
    }

    static T& instance() {
        return *instance_ptr();
    }

    static void destroy() {
        std::lock_guard<Mutex> lock(mutex_);
        if (instance_) {
            delete instance_;
            instance_ = nullptr;
        }
    }

protected:
    Singleton() = default;
    ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

private:
    static T* instance_;
    static Mutex mutex_;
};

template<typename T>
T* Singleton<T>::instance_ = nullptr;

template<typename T>
Mutex Singleton<T>::mutex_;

}

#endif
