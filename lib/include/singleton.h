#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <memory>
#include "autolock.h"

//_instance_id here is to make several instance possible
template<typename DataType, int _instance_id = 0>
class Singleton 
{
public: 
    inline static DataType* instance_ptr( void ) {
        if (NULL == data_holder_.get()) {
            AutoLock lock(&data_mutex_);
            if (NULL == data_holder_.get()) {
                data_holder_.reset(new DataType);
            }
        }
        return data_holder_.get();
    }

    // 全局对象引用
    static DataType& instance( void ) {
        return *instance_ptr();
    }

//    // 手动销毁实例。
//    static void release( void ) {
//        data_holder_.reset(NULL);
//    }

    // 判断实例是否已经存在
    static bool exists( void ) {
        return NULL != data_holder_.get();
    }
private:
    Singleton(){};
    ~Singleton(){};

    static std::auto_ptr<DataType> data_holder_;
    static Mutex data_mutex_;
};

template<typename DataType, int _instance_id>
std::auto_ptr<DataType> Singleton<DataType, _instance_id>::data_holder_;

template<typename DataType, int _instance_id>
Mutex Singleton<DataType, _instance_id>::data_mutex_;

//DataType with init
//  we got a new InitDataSingleton here to 
//  bypass a mistake in mulit-thread: set the data before init
//  this would make the data in used before inited
//  
template<typename DataType, int _instance_id = 0>
class InitDataSingleton 
{
public: 
    inline static DataType* instance_ptr( void ) {
        if (NULL == data_holder_.get()) {
            AutoLock lock(&data_mutex_);
            if (NULL == data_holder_.get()) {
                DataType* data = new DataType;
                data->init();
                data_holder_.reset(data);
            }
        }
        return data_holder_.get();
    }

    // 全局对象引用
    static DataType& instance( void ) {
        return *instance_ptr();
    }

//    // 手动销毁实例。
//    static void release( void ) {
//        data_holder_.reset(NULL);
//    }

    // 判断实例是否已经存在
    static bool exists( void ) {
        return NULL != data_holder_.get();
    }
private:
    InitDataSingleton(){};
    ~InitDataSingleton(){};

    static std::auto_ptr<DataType> data_holder_;
    static Mutex data_mutex_;
};

template<typename DataType, int _instance_id>
std::auto_ptr<DataType> InitDataSingleton<DataType, _instance_id>::data_holder_;

template<typename DataType, int _instance_id>
Mutex InitDataSingleton<DataType, _instance_id>::data_mutex_;


#endif // __SINGLETON_H__
