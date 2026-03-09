#include "lua_server.h"
#include "lmisc.h"
#include "llog.h"
#include "log.h"

const int32_t DIR_PATH_MAX = 1024;
const int32_t LUA_GC_MIN_STEP = 10;

static LuaServer* g_luasvr = NULL;

typedef void (*lua_large_table_warn_func)( lua_State *L, char* fmt, ... );
extern lua_large_table_warn_func g_lua_large_table_warn;

#ifdef DEBUG
GScriptDebug* g_ldb = NULL;
#endif

int32_t c_debug( lua_State* _L )
{
#ifdef DEBUG
    g_ldb->step_in();
#endif
    return 0;
}

/*-----------------------------------------------------------------------
 *                  lua 内存管理模块
 *----------------------------------------------------------------------*/

static int err_func( lua_State * _L )
{
  lua_getfield(_L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(_L, -1)) {
    lua_pop(_L, 1);
    return 1;
  }
  lua_getfield(_L, -1, "traceback");
  if (!lua_isfunction(_L, -1)) {
    lua_pop(_L, 2);
    return 1;
  }
  lua_pushvalue(_L, 1);  /* pass ERR(2)or message */
  lua_pushinteger(_L, 2);  /* skip this function and traceback */
  lua_call(_L, 2, 1);  /* call debug.traceback */
  return 1;
}

void lua_large_table_warn( lua_State* _L, char* _fmt, ... )
{
    char log_buf[2048] = { 0 };
    va_list arg_list;
    va_start(arg_list, _fmt);
    int i = vsnprintf(log_buf, sizeof(log_buf), _fmt, arg_list);
    va_end(arg_list);

    if (i <= 0) {
        return;
    }

    lua_Debug ldb;
    for(int32_t i = 0; lua_getstack( _L, i, &ldb)==1 && i < 10; i++)
    {
        lua_getinfo(_L, "Slnu", &ldb);
        const char * name = ldb.name;
        if (!name)
            name = "";
        const char * filename = ldb.source;

        char stack_buf[256] = { 0 };
        snprintf(stack_buf, sizeof(stack_buf), "\n[LUAWRAPPER](bt) #%d: %s:'%s', '%s' line %d", i, ldb.what, name, filename, ldb.currentline );
        if (strlen(log_buf) + strlen(stack_buf) >= sizeof(log_buf))
        {
            break;
        }

        strncat( log_buf, stack_buf, sizeof(log_buf));
    }
    PROF(2)( "%s", log_buf );
}


/*-----------------------------------------------------------------------
 *                  LuaServer
 *----------------------------------------------------------------------*/

#define my_lua_fail( _L, _file, _line )                                             \
{                                                                                   \
    ERR(2)( "[LUAWRAPPER](lmisc) %s:%d lua_call failed\n\t%s", _file, _line, lua_tostring( _L, -1 ) ); \
} 

#ifdef DEBUG

#ifdef __linux 
static int32_t ldb_input_func(char* _buff, int32_t _size)
{
	for(;;)
	{
        ff_sleep( 50 );
		if( fgets(_buff, _size, stdin ) != 0 )
		{
			return strlen( _buff );
		}
	}
}
#else
// windows版本暂且和linux版本一样
static int32_t ldb_input_func(char* _buff, int32_t _size)
{
	for(;;)
	{
		if( fgets(_buff, _size, stdin) != 0 )
		{
			return strlen( _buff );
		}
	}
}
#endif

#endif

static int32_t my_lua_panic( lua_State* _L ) 
{
    lua_Debug ldb;
    for( int32_t i = 0; lua_getstack( _L, i, &ldb ) == 1; ++i )
    {
        lua_getinfo( _L, ">Slnu", &ldb );
        const char * name = ldb.name ? ldb.name : "";
        const char * filename = ldb.source ? ldb.source : "";
        ERR(2)( "[LUAWRAPPER](LuaServer)my_lua_panic: %s '%s' @ file '%s', line %d\n", ldb.what, name, filename, ldb.currentline );
    }
    return 0;
}

static int32_t c_reload_file( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    const char* filename = lua_tostring( _L, 1 );
    int ret = g_luasvr->do_file( filename );
    if( !ret )
    {
        ERR(2)( "[LUAWRAPPER](c_reload_file) reload failed, file: %s", filename );
    }
    lua_pushboolean( _L, ret );
    lua_insert( _L, 2 );
    return lua_gettop( _L ) - 1;
}

LuaServer::LuaServer()
    :L_(NULL)
{
    for( int32_t i = 0; i < LUASERVER_REF_MAX; ++i )
    {
        refs_[i] = LUA_NOREF;
    }
    memset( luadir_, 0, sizeof( luadir_ ) );
    memset( main_file_, 0, sizeof( main_file_ ) );
    if (g_luasvr == NULL)
    {
        g_luasvr = this;
    }
    else
    {
        ERR(2)( "[LUAWRAPPER](LuaServer) more than one LuaServer instance detected" );
    }
}

LuaServer::~LuaServer()
{
#ifdef DEBUG
	delete debugger_;
#endif
    if( L_ )    
    {
        unregister_lua_refs();
        lua_gc( L_, LUA_GCCOLLECT, 0 );
        lua_close( L_ );                    
        L_ = NULL;
    }
    if (g_luasvr == this)
    {
        g_luasvr = NULL;
    }
}

int LuaServer::debug_call( lua_State * _L, int32_t _nargs, int32_t _nresults, int32_t _errfunc )
{
#ifdef DEBUG
    g_ldb->reset_instruction_num();
#endif
    int32_t base = lua_gettop( _L ) - _nargs;
    lua_pushcfunction( _L, &err_func );
    lua_insert( _L, base );
    int32_t ret = lua_pcall( _L, _nargs, _nresults, base );
    lua_remove( _L, base );
    return ret;
}

bool LuaServer::start_server( const char* _luadir, const char* _main_file )
{
    L_ = lua_open();

    luaL_openlibs( L_ );
    lua_atpanic( L_, my_lua_panic );

#ifdef DEBUG
    lua_pushboolean( L_, true );
#else
    lua_pushboolean( L_, false );
#endif
    lua_setglobal( L_, "g_debug" );

    register_class();
    register_global(); 

#ifdef DEBUG
    debugger_ = new GScriptDebug( L_ );
    debugger_->set_search_path( _luadir );
    debugger_->set_input_func( ldb_input_func );
	debugger_->set_luasvr( this );

    g_ldb = debugger_;
    g_ldb->set_max_instruction_num( 0x7fffffff );
#endif

    strlcpy( luadir_, _luadir, sizeof(luadir_) );
    lua_pushstring( L_, _luadir );
    lua_setglobal( L_, "g_luadir" );

    if( !do_file( _main_file ) ) return false;

    strlcpy( main_file_, _main_file, sizeof(main_file_) );
    lua_pushstring( L_, main_file_ );
    lua_setglobal( L_, "g_main_file" );

    lua_gc( L_, LUA_GCRESTART, 0 );

    register_lua_refs();

#ifdef DEBUG
    g_ldb->set_max_instruction_num( MAX_LUA_INSTRUCTION_DEFAULT_NUM );
#endif

    g_lua_large_table_warn = lua_large_table_warn;

    return true;
}

void LuaServer::check_gc( int32_t _frame_left_time )
{
    //fulltime gc occupy too much cpu usage which can be shared,
    //so it will only be turned on only for release version
#ifdef DEBUG
    lock();
    if( lua_gc( L_, LUA_GCSTEP, LUA_GC_MIN_STEP ) == 1 )
    {
        lua_gc( L_, LUA_GCRESTART, 0 );
    }
    unlock();
#else
    int32_t gc_start_time = msec();
    int32_t gc_check_time = gc_start_time;
    // gc at least one time per frame
    do
    {
        if( lua_gc( L_, LUA_GCSTEP, LUA_GC_MIN_STEP ) == 1 )
        {
            lua_gc( L_, LUA_GCRESTART, 0 );
            gc_check_time = msec();
            //extern int g_frame;
            //PROF(0)( "[lua_server](check_gc) LUA_GCRESTART at frame:%d, current frame gc time:%d, can use gc time:%d", 
            //        g_frame, gc_check_time - gc_start_time, _frame_left_time );
            break;
        }
        gc_check_time = msec();
    } while( gc_check_time - gc_start_time < _frame_left_time );
#endif

}

bool LuaServer::do_file( const char* _file )
{
    char cwd[DIR_PATH_MAX];
#ifdef __linux
    if( !getcwd( cwd, DIR_PATH_MAX ) ) {
#else
    if( !_getcwd( cwd, DIR_PATH_MAX ) ) {
#endif
        ERR(2)( "[LUAWRAPPER](LuaServer)do_file getcwd failed!" );
        return false;
    }

    /* change to lua directory */
#ifdef __linux
    if ( chdir( luadir_ ) < 0 )
#else
	if ( _chdir( luadir_ ) < 0 )
#endif
    {
        ERR(2)( "[LUAWRAPPER](LuaServer)do_file %s:%d chdir to Lua folder %s failed", __FILE__, __LINE__, luadir_ );
        return false;
    }

	bool ret = luaL_dofile( L_, _file ) == 0;
    if( ret )
        LOG(2)( "[LUAWRAPPER](LuaServer)do_file load lua main file %s successed", _file );
    else {
        my_lua_fail( L_, __FILE__, __LINE__ );
		lua_pop( L_, 1 );
    }

#ifdef __linux
    chdir( cwd );
#else
	_chdir( cwd );
#endif

	return ret;
}

void LuaServer::register_global()
{
    lua_register( L_, "c_debug", c_debug );

    lua_register( L_, "c_bt", c_bt );
    lua_register( L_, "c_traceback", c_traceback );

    lua_register( L_, "c_log", c_log );
    lua_register( L_, "c_trace", c_trace );
    lua_register( L_, "c_err", c_err );
    lua_register( L_, "c_prof", c_prof );
    lua_register( L_, "c_sells", c_sells );
    lua_register( L_, "c_gm", c_gm );
    lua_register( L_, "c_login", c_login );

    lua_register( L_, "c_item_c", c_item_c );
    lua_register( L_, "c_item_t", c_item_t );
    lua_register( L_, "c_item_u", c_item_u );
    lua_register( L_, "c_item_b", c_item_b );
    lua_register( L_, "c_syslog", c_syslog);
    lua_register( L_, "c_online", c_online);
    lua_register( L_, "c_sdklog", c_sdklog);
    lua_register( L_, "c_tgalog", c_tgalog);
    lua_register( L_, "c_backup", c_backup);

    lua_register( L_, "c_safe", c_safe );
    lua_register( L_, "c_trigger", c_trigger );
    lua_register( L_, "c_instance", c_instance );
    lua_register( L_, "c_money", c_money );
    lua_register( L_, "c_fight", c_fight );
    lua_register( L_, "c_fightd", c_fightd );

    lua_register( L_, "c_set_log_level", c_set_log_level );

    lua_register( L_, "c_cpu_tick", c_cpu_tick );
    lua_register( L_, "c_cpu_ms", c_cpu_ms );
    lua_register( L_, "c_str_table_key", c_str_table_key );
    lua_register( L_, "c_str_table_value", c_str_table_value );

    lua_register( L_, "c_post_disconnect_msg", c_post_disconnect_msg );

    lua_register( L_, "c_and", c_and );
    lua_register( L_, "c_or", c_or );
    lua_register( L_, "c_xor", c_xor );
    lua_register( L_, "c_not", c_not );
    lua_register( L_, "c_shl", c_shl );
    lua_register( L_, "c_shr", c_shr );

    lua_register( L_, "c_get_serial_num", c_get_serial_num );
    lua_register( L_, "c_des_encrypt", c_des_encrypt );
    lua_register( L_, "c_des_decrypt", c_des_decrypt );
    lua_register( L_, "c_rand_str", c_rand_str );
    lua_register( L_, "c_get_file_modify_time", c_get_file_modify_time );
    lua_register( L_, "c_assert", c_assert );

    lua_register( L_, "c_prof_start", c_prof_start );
    lua_register( L_, "c_prof_stop", c_prof_stop );
    lua_register( L_, "c_md5", c_md5 );
    lua_register( L_, "c_get_memory", c_get_memory );

    lua_register( L_, "c_get_miniserver_time_span", c_get_miniserver_time_span );
    lua_register( L_, "c_get_unity_time_span", c_get_unity_time_span );

	lua_register(L_, "c_reset_log_file_handle", c_reset_log_file_handle);
	lua_register(L_, "c_migrate_log_file", c_migrate_log_file);
    lua_register( L_, "c_reload_file", c_reload_file );
}

bool LuaServer::get_table_field_ref( const char* _table, const char* _field, int32_t _ref, int32_t _index )
{
    if( _ref >= LUASERVER_REF_MAX )
    {
        ERR(2)( "[LUAWRAPPER](LuaServer)get_table_field_ref %s:%d _ref %d reach LUASERVER_REF_MAX(%d)", __FILE__, __LINE__, _ref, LUASERVER_REF_MAX );
        return false;
    }
    lua_getglobal( L_, _table );
    if( lua_isnil( L_, -1 ) )
    {
        ERR(2)( "[LUAWRAPPER](LuaServer)get_table_field_ref %s:%d %s is nil, field: %s", __FILE__, __LINE__, _table,  _field );
        return false;
    }
    lua_getfield( L_, -1, _field );
    if( lua_isnil( L_, -1 ) )
    {
        ERR(2)( "[LUAWRAPPER](LuaServer)get_table_field_ref %s:%d table: %s, field: %s is nil", __FILE__, __LINE__, _table, _field );
        return false;
    }
    refs_[_ref] = luaL_ref( L_, _index );
    lua_pop( L_, 1 );
    return true;   
}

int LuaServer::get_lua_ref( int _ref )
{   
    if( refs_[_ref] == LUA_NOREF )
    {   
#ifndef __EDITOR
        TRACE(2)( "[LUAWRAPPER](lua_server) %s:%d, cannot get lua ref %d", __FILE__, __LINE__, _ref );
#endif
        return 0;
    }   
    lua_rawgeti( L_, LUA_REGISTRYINDEX, refs_[_ref] );
    if( lua_isnil( L_, -1 ) ) 
    {   
        ERR(2)( "[LUAWRAPPER](LuaServer)get_lua_ref %s:%d fail", __FILE__, __LINE__ );
        return 0;
    }   
    return 0;
}

void LuaServer::unregister_lua_refs()
{
    if( !L_ ) return;

    for( int32_t i = 0; i < LUASERVER_REF_MAX; ++i )
    {
        if( refs_[i] != LUA_NOREF )
        {
            luaL_unref( L_, LUA_REGISTRYINDEX, refs_[i] );
            refs_[i] = LUA_NOREF;
        }
    }
}

bool LuaServer::reload()
{
	bool ret = false;

#ifdef DEBUG
    int last_max_ins_num = MAX_LUA_INSTRUCTION_DEFAULT_NUM;

    if( g_ldb )
    {   
        g_ldb->reload();
        last_max_ins_num = g_ldb->get_max_instruction_num();
        g_ldb->set_max_instruction_num( 0x7fffffff );
    }   
#endif

    //! 读取脚本逻辑文件
    if( do_file( main_file_ ) ) 
	{   
		/* get lua references */
		unregister_lua_refs();
		register_lua_refs();
		ret = true;

		LOG(2)( "[LUAWRAPPER](LuaServer) %s:%d reload %s successed", __FILE__, __LINE__, main_file_ );
	}   
    else
	{   
		ERR(2)( "[LUAWRAPPER](LuaServer) %s:%d luaL_dofile failed", __FILE__, __LINE__ );
	}   
    
#ifdef DEBUG
    if( g_ldb )
    {   
        g_ldb->set_max_instruction_num( last_max_ins_num );
    }
#endif
 
    return ret;
}

int LuaServer::reload( const char* _lua_file )
{
    LOG(2)( "[LUAWRAPPER](lua_server) start reload lua server at %s", luadir_ );

    /* change to lua directory */
    if ( chdir( luadir_ ) < 0 )
    {
        ERR(2)( "[LUAWRAPPER](lua_server) %s:%d chdir to Lua folder %s failed", __FILE__, __LINE__, luadir_ );
        return -1;
    }

#ifdef DEBUG
    int last_max_ins_num = MAX_LUA_INSTRUCTION_DEFAULT_NUM;

    if( g_ldb )
    {   
        g_ldb->reload();
        last_max_ins_num = g_ldb->get_max_instruction_num();
        g_ldb->set_max_instruction_num( 0x7fffffff );
    } 
#endif

    if( luaL_dofile( L_, _lua_file ) ) {
        ERR(2)( "[LUAWRAPPER](lua_server) %s:%d luaL_dofile failed", __FILE__, __LINE__ );
        const char* err_msg = lua_tostring( L_, -1 );
        ERR(2)( "[LUAWRAPPER](lua_server) %s", err_msg );
        return -1;
    } else {
        LOG(2)( "[LUAWRAPPER](lua_server) %s:%d reload %s successed", __FILE__, __LINE__, _lua_file );
    }

#ifdef DEBUG
    if( g_ldb )
    {   
        g_ldb->set_max_instruction_num( last_max_ins_num );
    }
#endif
 
    /* update lua references */
    unregister_lua_refs();
    register_lua_refs();

    LOG(2)( "[LUAWRAPPER](lua_server) reload completed, file: %s", _lua_file );
    return 0;
}

int LuaServer::lua_mem_size()
{
    if( !L_ ) return -1;
    int ret = lua_gc( L_, LUA_GCCOUNT, 0 );
    return ret;
}

