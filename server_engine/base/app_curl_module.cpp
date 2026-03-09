#include "app_curl_module.h"
#include "curl/curl.h"
#include "log.h"
#include "lua_server.h"
#include "lmisc.h"
#include "openssl/err.h"
#include <openssl/opensslconf.h>
#include "md5.h"
#include <time.h>

#define OPENSSL_THREAD_ID  pthread_self() 

#define AUTH_CMD_QUERY_URL "http://172.30.10.171:1180/auth.php"

#define SERVER_TIME_DELTA_THRESHOLD 3600

#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_1_0_0

#define APP_ID "1000002"

static MyString g_app_key;

extern time_t get_lib_build_time();
extern int is_lib_time_out();
extern int TIME_OUT_SECONDS_THRESHOLD;

static void openssl_threadid_function( CRYPTO_THREADID* _id )
{
    CRYPTO_THREADID_set_numeric( _id, (unsigned long)OPENSSL_THREAD_ID );
}

#else

static unsigned long openssl_threadid_function(void)
{
    return ((unsigned long)OPENSSL_THREAD_ID);
}

#endif

static void openssl_locking_function( int _mode, int _n, const char* _file, int _line )
{
    if( _mode & CRYPTO_LOCK )
    {
        g_http_mng->openssl_lock( true, _n );
    }
    else
    {
        g_http_mng->openssl_lock( false, _n );
    }
}

static size_t http_write_cb( void *_ptr, size_t _size, size_t _nmemb, void* _data )
{
    size_t realsize = _size * _nmemb;
    HttpRequest* request = (HttpRequest*)_data;
    request->response_info_.cat_str( (char*)_ptr, realsize );
    return realsize;
}

static size_t query_auth_write_cb( void *_ptr, size_t _size, size_t _nmemb, void *_data )
{
    size_t realsize = _size * _nmemb; 
    MyString* response_str = (MyString*)_data;
    response_str->cat_str( (char*)_ptr, realsize );
    return realsize;
}

static size_t query_timeout_write_cb( void *_ptr, size_t _size, size_t _nmemb, void *_data )
{
    size_t realsize = _size * _nmemb; 
    MyString* response_str = (MyString*)_data;
    response_str->cat_str( (char*)_ptr, realsize );
    return realsize;
}


static size_t query_app_key_write_cb( void *_ptr, size_t _size, size_t _nmemb, void *_data )
{
    size_t realsize = _size * _nmemb; 
    MyString* response_str = (MyString*)_data;
    response_str->cat_str( (char*)_ptr, realsize );
    return realsize;
}

static void query_auth()
{
    CURL *curl;
    CURLcode res;
    MyString response_str;

    curl = curl_easy_init();
    assert( curl );
    curl_easy_setopt( curl, CURLOPT_URL, AUTH_CMD_QUERY_URL );
    curl_easy_setopt( curl, CURLOPT_POSTFIELDS, "cmd=time" );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, query_auth_write_cb );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)&response_str );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L ); 
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L ); 

    while( ( res = curl_easy_perform( curl ) ) != CURLE_OK )
    {
        ERR(2)( "[CurlModule](query_auth) curl_easy_perform failed: %s", curl_easy_strerror( res ) );
        response_str.clear();
        ff_sleep( 1000 );
    }
    curl_easy_cleanup( curl );

    int remote_time = atoi( response_str.c_str() );
    time_t current_time = time( NULL );
    int delta = current_time - remote_time;
    if ( abs( delta ) > SERVER_TIME_DELTA_THRESHOLD )
    {
        ERR(2)( "[CurlModule](query_auth) remote_server_time: %d, our_server_time: %d, delta: %ds out range", remote_time, current_time, delta );
        assert( false );
    }
    else
    {
        SAFE(2)( "[CurlModule](query_auth) remote_server_time: %d, our_server_time: %d, delta: %ds ok", remote_time, current_time, delta );
    }
}

static void query_lib_timeout()
{
    CURL *curl;
    CURLcode res;
    MyString response_str;

    curl = curl_easy_init();
    assert( curl );
    curl_easy_setopt( curl, CURLOPT_URL, AUTH_CMD_QUERY_URL );
    curl_easy_setopt( curl, CURLOPT_POSTFIELDS, "cmd=ymrgame" );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, query_timeout_write_cb );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)&response_str );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L ); 
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L ); 

    while( ( res = curl_easy_perform( curl ) ) != CURLE_OK )
    {
        ERR(2)( "[CurlModule](query_auth) curl_easy_perform failed: %s", curl_easy_strerror( res ) );
        response_str.clear();
        ff_sleep( 1000 );
    }
    curl_easy_cleanup( curl );

    int timeout = atoi( response_str.c_str() );

    TIME_OUT_SECONDS_THRESHOLD = timeout;
}

static void decode_app_key()
{
    char* str = g_app_key.c_str();
    int len = g_app_key.length();

    for( int i = 0; i < len; i++ )
    {
        if( i % 2 == 0 )
        {
            str[i] = str[i] - 1; 
        }
        else
        {
            str[i] = str[i] - 2; 
        }
    }
}

static const char* get_server_type( lua_State* _L )
{
    if( Lua::get_table( _L, -1, "RobotClient" ) == 2 )
    {
        lua_pop( _L, 1 );
        return "robot";
    }

    if( Lua::get_table( _L, -1, "DBClient" ) == 2 )
    {
        lua_pop( _L, 1 );
        return "game";
    }

    if( Lua::get_table( _L, -1, "BFSvr" ) == 2 )
    {
        lua_pop( _L, 1 );
        return "bf";
    }

    assert( lua_gettop( _L ) == 1 );

    return "other";
}

static void query_app_key( lua_State* _L )
{
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    assert( curl );
    curl_easy_setopt( curl, CURLOPT_URL, AUTH_CMD_QUERY_URL );

    char sign[512] = {0};
    time_t t = time(NULL);  /* get current time */
    snprintf( sign, 512, "%s|ymrgame|%d|6666", APP_ID, (int)t );
    unsigned char md5[16];
    MD5( ( unsigned char* )sign, strlen(sign), md5 );
    char md5str[33] = {0};
    snprintf( md5str, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7], md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15] );
  
    char cmd[512] = {0};
    sprintf( cmd, "cmd=%s&sign=%s&time=%d&type=%s&lib=%u&timeout=%u", APP_ID, md5str, (int)t, get_server_type( _L ), (unsigned int)get_lib_build_time(), TIME_OUT_SECONDS_THRESHOLD );

    curl_easy_setopt( curl, CURLOPT_POSTFIELDS, cmd );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, query_app_key_write_cb );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)&g_app_key );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L ); 
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L ); 

    while( ( res = curl_easy_perform( curl ) ) != CURLE_OK )
    {
        ERR(2)( "[CurlModule](query_auth) curl_easy_perform failed: %s", curl_easy_strerror( res ) );
        g_app_key.clear();
        ff_sleep( 1000 );
    }
    curl_easy_cleanup( curl );

    LOG(2)( "[CurlModule](query_app_key)query appkey success, appkey before decode: %s", g_app_key.c_str() );
}

HttpTask::HttpTask()
    :cache_request_(NULL)
{
    thread_name_ = "HttpTask";
    task_queue_ = new PcQueue( HTTP_TASK_NUM_MAX );
    cache_request_ = create_http_request();
}

HttpTask::~HttpTask()
{
    destroy_http_request( cache_request_ );
    SAFE_DELETE( task_queue_ );
}

HttpRequest* HttpTask::reset_http_request_opt( HttpRequest* _request, const char* _url, const char* _post_data )
{
    // set http version 1.0 : disable 100-continue to save ask-answer time
    //curl_easy_setopt( _request->curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0 );

    // more max connection means more cache, default 5 is enough for me
    //curl_easy_setopt( _request->curl_, CURLOPT_MAXCONNECTS, 10 );

    // request timeout set to 25 seconds
    curl_easy_setopt( _request->curl_, CURLOPT_TIMEOUT, 25 );

    // signal may not be thread-safe
    curl_easy_setopt( _request->curl_, CURLOPT_NOSIGNAL, 1L );

    // dns cache to 5 minites
    curl_easy_setopt( _request->curl_, CURLOPT_DNS_CACHE_TIMEOUT, 60*5 );

    // set header : disable 100-coutinue to reduce ask-answer time
    // set header : Keep-alive, much faster because it may keep the connection
    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append( chunk, "Expect:" );
    chunk = curl_slist_append( chunk, "Connection: Keep-alive" );
    curl_easy_setopt( _request->curl_, CURLOPT_HTTPHEADER, chunk );

    // set url
    _request->url_.set_str( _url );
    curl_easy_setopt( _request->curl_, CURLOPT_URL, _request->url_.c_str() ); 

    // set post data
    _request->post_data_.set_str( _post_data );
	if (!_post_data || strlen(_post_data) == 0) {
		// post dataÎª¿Õ , Ö´ÐÐhttpgetÇëÇóI
		curl_easy_setopt(_request->curl_, CURLOPT_HTTPGET, 1);
	}
	else  {
		curl_easy_setopt( _request->curl_, CURLOPT_POSTFIELDS, _request->post_data_.c_str() ); 
	}
    
	
    // clear last response
    _request->response_info_.clear();

    // set write data callback
    curl_easy_setopt( _request->curl_, CURLOPT_WRITEFUNCTION, http_write_cb );
    curl_easy_setopt( _request->curl_, CURLOPT_WRITEDATA, (void *)_request );

    // set verbose
    if( g_http_mng->is_debug() )
    {
        curl_easy_setopt( _request->curl_, CURLOPT_VERBOSE, 1L );
    }
    else
    {
        curl_easy_setopt( _request->curl_, CURLOPT_VERBOSE, 0L );
    }

    // https ignore ssl verify
    curl_easy_setopt( _request->curl_, CURLOPT_SSL_VERIFYPEER, 0L ); 
    curl_easy_setopt( _request->curl_, CURLOPT_SSL_VERIFYHOST, 0L ); 

    return _request;
}

HttpRequest* HttpTask::create_http_request()
{
    HttpRequest* request = new HttpRequest();
    request->curl_ = curl_easy_init();
    return request;
}

void HttpTask::destroy_http_request( HttpRequest* _request )
{
    if( _request )
    {
        curl_easy_cleanup( _request->curl_ );
        delete _request;
    }
}

void HttpTask::run()
{
    while( active_ )
    {
        HttpTaskInfo* task_info;
        task_queue_->get_userdata( (void**)&task_info );  

        if( !task_info )
        {
            // post NULL means thread should be destroyed
            LOG(2)( "[HttpTask](run)task stop..." );
            return;
        }
        
        TICK( A );
        HttpRequest* request = http_request( task_info->url_.c_str(), task_info->post_data_.c_str() );
        TICK( B );

        if( g_http_mng->is_debug() )
        {
            TRACE(2)( "[HttpTask](run)thread id: %lu, http_request task user_id: %d, url: %s, use_time: %dms", this->thread_id_, task_info->user_id_, request->url_.c_str(), get_msec(B-A) );
        }
        else
        {
            PROF(2)( "[HttpTask](run)thread id: %lu, http_request task user_id: %d, url: %s, use_time: %dms, task left: %d", this->thread_id_, task_info->user_id_, request->url_.c_str(), get_msec(B-A), task_queue_->get_count() );
        }

        g_http_mng->cache_http_response_from_request( task_info->user_id_, request );

        // don't forget detele task info 
        SAFE_DELETE( task_info );
    }
}

HttpRequest* HttpTask::http_request( const char* _url, const char* _post_data )
{
    HttpRequest* request = reset_http_request_opt( cache_request_, _url, _post_data );

    if( g_http_mng->is_debug() )
    {
        TRACE(2)( "[HttpTask](http_request)try curl_easy_perform. url: %s, post_data: %s", request->url_.c_str(), request->post_data_.c_str() ); 
    }

    request->curl_code_ = curl_easy_perform( request->curl_ );

    return request;
}

const char HttpMng::className[] = "HttpMng";
const bool HttpMng::gc_delete_ = false;

Lunar<HttpMng>::RegType HttpMng::methods[] =
{
    method( HttpMng, c_http_request ),
    method( HttpMng, c_get_task_num ),
    method( HttpMng, c_set_debug ),
    method( HttpMng, c_get_app_id ),
    method( HttpMng, c_get_app_key ),
	{NULL, NULL}
};

HttpMng::HttpMng()
    :http_task_(NULL), task_num_(0), task_index_(0), is_debug_(false)
{
    INIT_LIST_HEAD( &recv_list_ );
    INIT_LIST_HEAD( &proc_list_ );
}

HttpMng::~HttpMng()
{
}

bool HttpMng::start( int _task_num )
{
    if( _task_num <= 0 )
    {
        return false;
    }

    task_num_ = _task_num;
    task_index_ = 0;
    http_task_ = new HttpTask[task_num_];

    for( int i = 0; i < task_num_; i++ )
    {
        http_task_[i].start();
    }

    int openssl_lock_num = CRYPTO_num_locks();
    openssl_mutex_ = new Mutex[openssl_lock_num];

#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_1_0_0
    CRYPTO_THREADID_set_callback( openssl_threadid_function );
#else
    CRYPTO_set_id_callback( openssl_threadid_function );
#endif

    CRYPTO_set_locking_callback( openssl_locking_function );

    return true;
}

void HttpMng::stop()
{
    for( int i = 0; i < task_num_; i++ )
    {
        http_task_[i].post_task( NULL );
        http_task_[i].stop();
    }

    SAFE_DELETE_ARRAY( http_task_ );
    SAFE_DELETE_ARRAY( openssl_mutex_ );

    CRYPTO_set_id_callback( NULL );
    CRYPTO_set_locking_callback( NULL );
}

void HttpMng::cache_http_response_from_request( int _user_id, HttpRequest* _request )
{
    // delete by HttpMng::process()
    HttpResponse* response = new HttpResponse;

    response->user_id_ = _user_id;
    response->url_.set_str( _request->url_.c_str() );
    response->response_info_.set_str( _request->response_info_.c_str() );
    response->post_data_.set_str( _request->post_data_.c_str() );
    response->curl_code_ = _request->curl_code_;
    INIT_LIST_HEAD( &response->link_ );

    AutoLock lock( &mutex_ );
    list_add_tail( &response->link_, &recv_list_ );
}

void HttpMng::process( lua_State* _L )
{
    mutex_.Lock();
    list_splice_tail( &recv_list_, &proc_list_ );
    INIT_LIST_HEAD( &recv_list_ );
    mutex_.Unlock();

    while( !list_empty( &proc_list_) ) 
    {
        list_head* pos = proc_list_.next;
        list_del( pos );

        HttpResponse* response = list_entry( pos, HttpResponse, link_ );

        // user_id 0 means do not call lua function
        if( response->user_id_ != 0 || is_debug() )
        {
            lua_getglobal( _L, "http_mng" );        //http_mng
            lua_pushstring( _L, "on_response" );    //http_mng "on_request"
            lua_gettable( _L, -2 );                 //http_mng  on_request    

            lua_pushinteger( _L, response->user_id_ );
            lua_pushstring( _L, response->response_info_.c_str() );
            lua_pushstring( _L, response->url_.c_str() );
            lua_pushstring( _L, response->post_data_.c_str() );
            lua_pushinteger( _L, response->curl_code_ );

            if( llua_call( _L, 5, 0, 0 ) )           //http_mng
            {
                llua_fail( _L, __FILE__, __LINE__ );
            }

            lua_pop( _L, 1 );
        }

        SAFE_DELETE( response );
    }
}

HttpTask* HttpMng::get_next_idle_http_task()
{
    task_index_ %= task_num_;
    return &http_task_[task_index_++];
}

void HttpMng::openssl_lock( bool _is_lock, int _index )
{
    if( _is_lock )
    {
        openssl_mutex_[_index].Lock();
    }
    else
    {
        openssl_mutex_[_index].Unlock();
    }
}

bool HttpMng::http_request( int _user_id, const char* _url, const char* _post_data )
{
    if( !is_start() )
    {
        return false;
    }

    HttpTask* task = get_next_idle_http_task();

    if( task->is_task_full() )
    {
        ERR(2)( "[HttpMng](http_request)request user_id: %d failed, task queue full, max count: %d", _user_id, HTTP_TASK_NUM_MAX );
        return false;
    }

    // delete in HttpTask::run()
    HttpTaskInfo* task_info = new HttpTaskInfo();
    task_info->user_id_ = _user_id;
    task_info->url_.set_str( _url );
    task_info->post_data_.set_str( _post_data );

    task->post_task( task_info );
    
    if( is_debug() )
    {
        TRACE(2)( "[HttpMng](http_request)post new task %d, url: %s, post_data: %s, to task thread %d", 
                task_info->user_id_, task_info->url_.c_str(), task_info->post_data_.c_str(), task_index_ );
    }

    return true;
}

int HttpMng::c_http_request( lua_State* _L )
{
    lcheck_argc( _L, 3 );

    int user_id = lua_tointeger( _L, 1 );
    const char* url = lua_tostring( _L, 2 );
    const char* post_data = luaL_optstring( _L, 3, NULL );

    bool ret = http_request( user_id, url, post_data );
    lua_pushboolean( _L, ret );

    return 1;
}

int HttpMng::c_get_task_num( lua_State* _L )
{
    lua_pushinteger( _L, get_task_num() );
    return 1;
}

int HttpMng::c_set_debug( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    set_debug( lua_toboolean( _L, 1 ) );
    return 0;
}

int HttpMng::c_get_app_id( lua_State* _L )
{
    lua_pushstring( _L, APP_ID );
    return 1;
}

int HttpMng::c_get_app_key( lua_State* _L )
{
    lua_pushlstring( _L, g_app_key.c_str(), g_app_key.length() );
    return 1;
}

bool CurlModule::app_class_init()
{
    int openssl_thread_support = 0;
#if defined(OPENSSL_THREADS)
    openssl_thread_support = 1;
#endif
    int task_num = 1;
    APP_GET_NUMBER( "HttpTaskNum", task_num );

    curl_global_init( CURL_GLOBAL_ALL );
    query_auth();
    query_lib_timeout();
    query_app_key( app_base_->L );
    decode_app_key();

    if( is_lib_time_out() )
    {
        FLOG( "./app_trace.log", "fatal error: lib timeout. rebuild it and restart server");
        return false;
    }

    g_http_mng->start( task_num );
    LOG(2)( "===========Curl Module Start. HttpTask num: %d, openssl thread support: %d===========", task_num, openssl_thread_support );
    return true;
}

bool CurlModule::app_class_destroy()
{
    g_http_mng->stop();
    curl_global_cleanup();
    LOG(2)( "===========Curl Module Destroy===========");
    return true;
}

