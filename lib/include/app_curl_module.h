#ifndef __APP_CURL_MODULE_H__
#define __APP_CURL_MODULE_H__

#include "app_base.h"
#include "netmng.h"
#include "mystring.h"
#include "mymap32.h"
#include "curl/curl.h"
#include "lunar.h"

const int HTTP_TASK_NUM_MAX = 4096;

struct HttpRequest
{
    MyString    url_;
    MyString    response_info_;
    MyString    post_data_;
    CURL*       curl_;
    CURLcode    curl_code_; 
};

struct HttpResponse
{
    int         user_id_;
    MyString    url_;
    MyString    response_info_;
    MyString    post_data_;
    CURLcode    curl_code_; 
    list_head   link_;
};

struct HttpTaskInfo
{
    int         user_id_;
    MyString    url_;
    MyString    post_data_;
};

class HttpTask : public Thread
{
public:
    HttpTask();
    ~HttpTask();
public:
    HttpRequest*        http_request( const char* _url, const char* _post_data );
    void                run();
    void                post_task( HttpTaskInfo* _task_info ) { task_queue_->post_userdata( _task_info ); }
    bool                is_task_full() { return task_queue_->get_count() >= HTTP_TASK_NUM_MAX; }
private:
    HttpRequest*        create_http_request();
    void                destroy_http_request( HttpRequest* _request );
    HttpRequest*        reset_http_request_opt( HttpRequest* _request, const char* _url, const char* _post_data );
private:
    PcQueue*            task_queue_;
    HttpRequest*        cache_request_;
};

class HttpMng
{
public:
    HttpMng();
    ~HttpMng();
public:
    bool                start( int _task_num );
    void                stop();
    void                cache_http_response_from_request( int _user_id, HttpRequest* _request );
    bool                http_request( int _user_id, const char* _url, const char* _post_data );
    void                process( lua_State* _L );
    int                 get_task_num() { return task_num_; }
    void                set_debug( bool _is_debug ) { is_debug_ = _is_debug; }
    bool                is_debug() { return is_debug_; }
    void                openssl_lock( bool _is_lock, int _index ); 
private:
    bool                is_start() { return task_num_ > 0; }
    HttpTask*           get_next_idle_http_task();
private:
    HttpTask*           http_task_;
    int                 task_num_;
    Mutex               mutex_;
    list_head           recv_list_;
    list_head           proc_list_;
    int                 task_index_;
    bool                is_debug_;
    Mutex*              openssl_mutex_;
public:
	static const char   className[];
	static              Lunar<HttpMng>::RegType methods[];
	static const bool   gc_delete_;
public:
    int                 c_http_request( lua_State* _L );
    int                 c_get_task_num( lua_State* _L );
    int                 c_set_debug( lua_State* _L );
    int                 c_get_app_id( lua_State* _L );
    int                 c_get_app_key( lua_State* _L );
};

#define g_http_mng Singleton<HttpMng>::instance_ptr() 

class CurlModule : public AppClassInterface
{
public:   
    bool app_class_init();    
    bool app_class_destroy(); 
};

#endif

