
#ifndef __LUA_DEBUGGER_H__
#define __LUA_DEBUGGER_H__

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <sstream>
#include <string.h>

#include <stdint.h>

#include <stdlib.h>
#include <assert.h>
#include "ffsys.h"

#define LUA_MAX_BRKS 64

const int MAX_LUA_INSTRUCTION_DEFAULT_NUM = 500000;

class LUADebugDriver;

class BreakPoint 
{

public:

	BreakPoint() 
	{ 
		reset(); 
	}
	~BreakPoint() { }

	void reset() 
	{
		linenum_ = 0;
		hit_count_ =0;
		type_ = None;
		memset( name_, 0, sizeof( name_ ) );
		memset( name_what_, 0, 7 );
	}
	int32_t hit() { return ++hit_count_; }

	enum BPType { None = 0 , User, Debugger };

	void set_name( const char * s ) { strlcpy( name_, s, 512 ); }
	const char * get_name() const { return name_; }
	const char * get_name_what() const { return name_what_; }
	int32_t get_line() const { return linenum_; }
	BPType get_type() const { return type_; }

	friend class LUADebugDriver;

private:

	BPType	type_;
	char	name_[512]; // function name or filename
	int32_t	 linenum_;
	int32_t	 hit_count_;
	char	name_what_[7];

};

class LUADebugDriver 
{

public:

	LUADebugDriver( lua_State * );
	virtual ~LUADebugDriver();

	// add breakpoint by filename and line number
	virtual int32_t add_break_point( const char * filename, int32_t linenum, BreakPoint::BPType type = BreakPoint::User );
	// add breakpoint by function name
	virtual int32_t add_break_point( const char * funcname, BreakPoint::BPType type = BreakPoint::User, const char* namewhat = NULL );
	int32_t add_break_point( const char * funcname );
	const BreakPoint * get_break_point( int32_t bpnum ) const 
	{
		assert( bpnum>=0 && bpnum<LUA_MAX_BRKS );
		return &bp_[bpnum];
	}
	int32_t hit_break_point( int32_t bpnum ) 
	{
		assert( bpnum>=0 && bpnum<LUA_MAX_BRKS );
		assert( bp_[bpnum].get_type() != BreakPoint::None );
		return bp_[bpnum].hit();
	}
	void remove_break_point( int32_t bpnum ) 
	{
		assert( bpnum>=0 && bpnum<LUA_MAX_BRKS );
		bp_[bpnum].reset();
	}

	bool path_match( const char * name, const char * file_path ) const;

	bool is_single_step() const { return step_; }
	void single_step( bool is_step );
	void step_in();
	
	void enable_line_hook( bool is_enable );
	void enable_func_hook( bool is_enable );
	void enable_debug( bool is_enable ) 
	{ 
		enable_line_hook( is_enable ); 
		enable_func_hook( is_enable ); 
		is_enable_ = is_enable;
	}
	void enable_error_hook( bool is_enable );
	void enable_prof_hook( bool is_enable );
    void set_max_instruction_num( int max_ins );
    int  get_max_instruction_num();

	const char * get_last_error_msg() const { return last_error; }

    void record( const char* format, ... );
	
	static const char tmptablename[];
	static const char tmpvarname[];

    static void reset_instruction_num() { instruction_num = 0; }

protected:

	void setup_temp_script_begin( std::stringstream & str, lua_Debug *ar ) const;
	void setup_temp_script_end( std::stringstream & str, lua_Debug *ar ) const;
	void temp_table_cleanup( lua_Debug *ar ) const;

	// event handle for every line
	virtual void on_event( int32_t bpnum, lua_Debug *ar ) = 0;
	// envent handle for every function call/return
	virtual void on_func_event( int32_t bpnum, lua_Debug *ar, bool is_return ) = 0;
	// event handle for every _ERRORMESSAGE
	virtual void on_error();

	void set_error_msg( const char * s ) { ff_strcpy( last_error, sizeof( last_error ), s ); }

	lua_State * luastate_;
	lua_CFunction save_error_msg_;
	int32_t	 call_depth_;
	BreakPoint bp_[LUA_MAX_BRKS];
	char search_path_[4096];
	bool is_enable_;

protected:
    static int instruction_num;

private:
	static void line_hook( lua_State *L, lua_Debug *ar );
	static void func_hook( lua_State *L, lua_Debug *ar );
	static void all_hook( lua_State *L, lua_Debug *ar );
	static void prof_hook( lua_State *L, lua_Debug *ar );
	static int32_t error_hook( lua_State *L );
    static void check_max_instructions( lua_State* L, lua_Debug* ar );

	int32_t search_break_point( lua_Debug *ar ) const;

	bool step_;   // single step or not
	char last_error[128];
	int32_t max_bp_idx_;

	static LUADebugDriver **all_debug;
	static int32_t obj_count, obj_alloc;
	static LUADebugDriver * search_debugger( const lua_State * state );

};

typedef int32_t(*INPUT_FUNC)( char* _buff, int32_t _size );

class SourceFileManager;

class LuaServer;

class GScriptDebug: public LUADebugDriver 
{

public:

	GScriptDebug( lua_State * state );
	virtual ~GScriptDebug();

	void on_event( int32_t, lua_Debug * ar );

	void on_exception( lua_Debug * ar );

	void set_input_func( INPUT_FUNC _input_func );
	int32_t get_input( char* _buff, int32_t _size );

	void handle_command( const char * cmd, lua_Debug *ar = NULL );
	void handle_break_point_command( const char * op, int32_t oplen );

	void reload();
	void set_search_path( const char * path );
	void set_prompt();
	int32_t add_break_point( const char * filename, int32_t linenum, BreakPoint::BPType type = BreakPoint::User );
	int32_t add_break_point( const char * funcname, BreakPoint::BPType type = BreakPoint::User, const char* namewhat = NULL )
	{
		return LUADebugDriver::add_break_point( funcname, type, namewhat );
	}

	void set_luasvr( LuaServer* _lua_svr ) { lua_svr_ = _lua_svr; }
	void err_func();

protected:
	void on_func_event( int32_t bpnum, lua_Debug *ar, bool is_return);
	void on_error();

private:

	void print_string_var( int32_t si, int32_t depth );
	void print_table_var( int32_t si, int32_t depth );
	void print_var( int32_t si = -1, int32_t depth = 1 );
	void print_expression( const char * exp, lua_Debug *ar );

	void debug_dump_stack( int32_t depth = 0, int32_t verbose = 0 );
	void save_last_command( const char *cmd );
	void print( const char *, ... ) const;
	void copy_with_back_space( char * des, const char * src ) const; // find \8 and do what back space do 

	bool handle_command_ldb( const char * cmd, lua_Debug * ar );
	void handle_func_break_point_command( const char * op, int32_t oplen );
	void handle_line_break_point_command( const char * dv, const char* op, int32_t oplen );

	// 命令处理函数开始
	void handle_cmd_breakpoint( const char * op, int32_t oplen, lua_Debug * ar ); // b
	void handle_cmd_delete( const char * op, int32_t oplen, lua_Debug * ar ); // delete
	void handle_cmd_list( const char * op, int32_t oplen, lua_Debug * ar ); // list
	void handle_cmd_print( const char * op, int32_t oplen, lua_Debug * ar ); // p
	void handle_cmd_stat( const char * op, int32_t oplen, lua_Debug * ar ); // stat
	void handle_cmd_path( const char * op, int32_t oplen, lua_Debug * ar ); // path
	void handle_cmd_help( const char * op, int32_t oplen, lua_Debug * ar ); // help ?
	void handle_cmd_enable( const char * op, int32_t oplen, lua_Debug * ar ); // enable
	void handle_cmd_line( const char * op, int32_t oplen, lua_Debug * ar ); // line
	void handle_cmd_func( const char * op, int32_t oplen, lua_Debug * ar ); // func
	void handle_cmd_error( const char * op, int32_t oplen, lua_Debug * ar ); // error
	void handle_cmd_disable( const char * op, int32_t oplen, lua_Debug * ar ); // disable
	void handle_cmd_do( const char * op, int32_t oplen, lua_Debug * ar ); // do
	void handle_cmd_dofile( const char * op, int32_t oplen, lua_Debug * ar ); // dofile
	void handle_cmd_clear( const char * op, int32_t oplen, lua_Debug * ar ); // clear
	void handle_cmd_debug( const char * op, int32_t oplen, lua_Debug * ar ); // debug
	void handle_cmd_trace( const char * op, int32_t oplen, lua_Debug * ar ); // trace
	void handle_cmd_reload( const char * op, int32_t oplen, lua_Debug * ar ); // reload
	void handle_cmd_instruction( const char * op, int32_t oplen, lua_Debug * ar ); // reload
	// 以下为断点处才能使用的函数
	bool handle_cmd_continue( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar ); // c
	bool handle_cmd_next( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar ); // n
	bool handle_cmd_step( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar ); // s
	bool handle_cmd_backtrace( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar ); // bt
	bool handle_cmd_setlocal( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar ); // setlocal
	bool handle_cmd_setglobal( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar ); // setglobal
	// 命令处理函数结束


	typedef void (GScriptDebug::*CommandHandler)( const char * , int32_t, lua_Debug * );
	typedef bool (GScriptDebug::*CommandHandlerLdb)( const char *, const char * , int32_t, lua_Debug * );

	//std::map<std::string, CommandHandler> entries;
	struct Entry { const char * cmd; CommandHandler handler; };
	static Entry entries[];
	struct EntryLdb { const char * cmd; CommandHandlerLdb handler; };
	static EntryLdb entries_ldb[];

	SourceFileManager * src_manager_;
	char last_cmd_[4096];
	INPUT_FUNC input_func_;

private:
	LuaServer* lua_svr_;
};

template <typename T, int32_t init_size = 4>
class GAutoPtr 
{
protected:
	bool ref_;
	T* ptr_;
public:
	int32_t size_;
	int32_t number_;
	GAutoPtr()   
	{
		ref_ = false;
		ptr_ = 0;
		size_ = 0;
		number_ = 0;
	}
	~GAutoPtr() 
	{ 
		if( !ref_ && ptr_ )
		{
			delete[] ptr_; 
		}
	}
	operator T*() { return ptr_; }
	T& operator[]( int32_t n ) 
	{ 
		assert( n>=0 && n<size_ ); 
		return ptr_[n]; 
	}
	void increase();
};

template <typename T, int32_t init_size>
void GAutoPtr<T, init_size>::increase()
{
	if( ptr_ == 0 ) 
	{
		ptr_ = new T[init_size];
		size_ = init_size;
	}
	else 
	{
		T *temp;
		int32_t newsize = size_ + size_ / 2;
		int32_t i;
		temp = new T[newsize];
		for( i=0; i < number_; i++ )
		{
			temp[i] = ptr_[i];
		}
		delete[] ptr_;
		ptr_ = temp;
		size_ = newsize;
	}
}

#endif

