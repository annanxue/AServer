#ifndef __APP_BASE__
#define __APP_BASE__

#include <signal.h>
#include "lua_utils.h"
#include "logic_thread.h"

const size_t APP_CFG_NAME_MAX = 128;

#define APP_GET_TABLE(table,num) \
    if( Lua::get_table( app_base_->L, -1, table ) == num ){ 

#define APP_END_TABLE() \
        lua_pop( app_base_->L, 1 ); \
    }

#define APP_GET_STRING(name,value) Lua::get_table_string( app_base_->L, -1, name, value, APP_CFG_NAME_MAX )
#define APP_GET_NUMBER(name,value) Lua::get_table_number( app_base_->L, -1, name, value )
#define APP_GET_NUMBER_SPLIT(name,value,num) Lua::get_table_split_number( app_base_->L, -1, name, value ,num)

#define APP_WHILE_TABLE() lua_pushnil(app_base_->L); while( lua_next( app_base_->L, -2) ) {
#define APP_END_WHILE() lua_pop(app_base_->L, 1); }

#define APP_TABLE_LEN ((int)lua_objlen( app_base_->L, -1 ))

class AppBase;
class AppClassInterface
{
public:
    virtual bool app_class_init(){ return true; }
    virtual bool app_class_destroy() { return true; }
   
    friend class AppBase;
protected:
    AppBase* app_base_;
private:
    AppClassInterface* next_;
    AppClassInterface* prev_;
};

class AppThread
{
public:
    LogicThread* thread_;
    AppThread* next_;
    AppThread* prev_;
};

class AppBase
{
public:
    bool init( int32_t argc, char* argv[] );
    bool register_class( AppClassInterface* );
    bool register_thread( LogicThread* );
    bool register_signal( int32_t _sig, sighandler_t _handler );
    bool start();
    lua_State* L;

    inline bool is_daemon() { return is_daemon_; }

public:
    AppBase(); 
    virtual ~AppBase();

protected:
    virtual bool main_loop() = 0;

private:
    AppClassInterface class_header_;
    AppThread thread_header_;
    char cfg_file_name_[APP_CFG_NAME_MAX];
    bool is_daemon_;

public:
    static volatile bool active_;
    static volatile int int_time_;
};

#endif

