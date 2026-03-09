#include "app_base.h"
#include "app_fx.h"
#include "log.h"
#include <stdlib.h>

volatile bool AppBase::active_ = true;
volatile int AppBase::int_time_ = 0;

//static void default_signal_handler( int32_t _sig ){
//    if( _sig == SIGUSR1 ) {
//        AppBase::active_ = false;
//        LOG(2)( "[AppBase](default_sigusr1_handler)recv signal: %d, active: %d", _sig, AppBase::active_ );
//    }
//}

AppBase::AppBase() : is_daemon_( true ) {
    class_header_.next_ = &class_header_;
    class_header_.prev_ = &class_header_;
    thread_header_.next_ = &thread_header_;
    thread_header_.prev_ = &thread_header_;
}

AppBase::~AppBase(){
}

bool AppBase::register_class( AppClassInterface* _class ){
    _class->prev_ = class_header_.prev_;
    _class->next_ = &class_header_;
    class_header_.prev_->next_ = _class;
    class_header_.prev_ = _class;
    _class->app_base_ = this;
    return true;
}

bool AppBase::register_thread( LogicThread* _thread ){
    AppThread* app_thread  = new AppThread;
    app_thread->thread_ = _thread;
    app_thread->prev_ = thread_header_.prev_;
    app_thread->next_ = &thread_header_;
    thread_header_.prev_->next_ = app_thread;
    thread_header_.prev_ = app_thread;
    return true; 
}

bool AppBase::register_signal( int32_t _sig, sighandler_t _handler ){
    signal( _sig, _handler );
    return true;
}

bool AppBase::start(){
    L = lua_open();
    if( Lua::do_file( L, cfg_file_name_ ) ){
        lua_close(L);
        FLOG( "./app_trace.log", "[AppBase]%s:%d load cfg_file fail!\n", __FILE__, __LINE__ );
        exit(0);
    }

    if( Lua::get_table( L, LUA_GLOBALSINDEX, "AppConfig" ) != 1 ) {
        lua_close( L );
        FLOG( "./app_trace.log", "[AppBase]%s:%d load cfg_file attr AppConfig err\n", __FILE__, __LINE__ );
        exit(0);
    }

    Lua::get_table_number( L, -1, "Daemon", is_daemon_ );
    if( is_daemon_ ) { 
        daemon_init();
        FLOG( "./app_trace.log", "[AppBase]%s:%d (init)app run in daemon mode!!!!\n", __FILE__, __LINE__ );
    }

    AppClassInterface* pclass;
    for( pclass = class_header_.next_; pclass != &class_header_; pclass = pclass->next_ ){
        if ( !pclass->app_class_init() ){
            lua_close( L );
            FLOG( "./app_trace.log", "[AppBase]%s:%d class error\n", __FILE__, __LINE__ );
            exit(0);
        }
    }

    init_tick();

    AppThread* papp_thread;
    for( papp_thread = thread_header_.next_; papp_thread != &thread_header_; papp_thread = papp_thread->next_ ){
        int frame_rate = 10;
        Lua::get_table_number( L, -1, "LogicFrameRate", frame_rate );
        papp_thread->thread_->set_frame_rate( frame_rate );
        if ( papp_thread->thread_->start() ){
            FLOG( "./app_trace.log", "[AppBase]%s:%d thread error\n", __FILE__, __LINE__ );
            exit(0);
        }
    }

    FLOG( "./app_trace.log", "[AppBase]%s:%d class success!\n", __FILE__, __LINE__ );
    lua_close(L);

    FLOG( "./app_trace.log", "-----------------[AppBase]Start Main Loop-----------------\n", __FILE__, __LINE__ );
    main_loop();
    FLOG( "./app_trace.log", "-----------------[AppBase]End Main Loop-----------------\n", __FILE__, __LINE__ );

    for( papp_thread = thread_header_.prev_; papp_thread != &thread_header_; ){
        AppThread* tmp = papp_thread;
        if ( papp_thread->thread_->stop() ){
            FLOG( "./app_trace.log", "[AppBase]%s:%d destroy thread error\n", __FILE__, __LINE__ );
            exit(0);
        }
        papp_thread = papp_thread->prev_;
        papp_thread->next_ = tmp->next_;
        tmp->prev_->next_ = papp_thread;
        delete tmp;
    }

    for( pclass = class_header_.prev_; pclass != &class_header_; pclass = pclass->prev_ ){
        if ( !pclass->app_class_destroy() ){
            FLOG( "./app_trace.log", "[AppBase]%s:%d destroy class error\n", __FILE__, __LINE__ );
            exit(0);
        }
    }

    class_header_.prev_ = class_header_.next_ = &class_header_;
    return true;
}

extern int is_lib_time_out();

bool AppBase::init( int argc, char* argv[] ){
    memset( cfg_file_name_, 0, sizeof(cfg_file_name_) );

    if( pre_main( argc, argv, cfg_file_name_ ) == -1 ) {
        return false;
    }

    if( strlen( cfg_file_name_ ) <=0 ){
        printf("Usage : \n    -c filename: start with config file.\n ");
        FLOG( "./app_trace.log", "[AppBase]%s:%d init cfg_file_name read fail!\n", __FILE__, __LINE__ );
        return false;
    }

    /*
    if( is_lib_time_out() )
    {
        FLOG( "./app_trace.log", "fatal error: lib timeout. rebuild it and restart server");
        return false;
    }
    */
    
    return true;
}

