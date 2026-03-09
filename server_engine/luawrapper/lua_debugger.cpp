#ifndef __linux
#pragma warning(disable : 4996)
#endif

#include <assert.h>
#include "lua_debugger.h"
#include "ffsys.h"
#include "lua_server.h"
#include "lmisc.h"

LUADebugDriver ** LUADebugDriver::all_debug = NULL;
int32_t LUADebugDriver::obj_count = 0;
int32_t LUADebugDriver::obj_alloc = 0;
const char LUADebugDriver::tmptablename[] = "__MY_SCRIPTDEBUGGER_TMP_TABLE";
const char LUADebugDriver::tmpvarname[] = "__MY_SCRIPTDEBUGGER_TMP_VAR";
int LUADebugDriver::instruction_num = 0;

static int MAX_LUA_INSTRUCTION_NUM = MAX_LUA_INSTRUCTION_DEFAULT_NUM;

// if exp is a legal lua var name
// exp maybe has some trailing ' ' and/or '\t'
// return the length of the var
// else return 0
static int32_t get_lua_var_len( const char * exp )
{
	int32_t i;
	if( isdigit( exp[0] ) )
	{
		return 0;
	}
	for( i=0; exp[i]!=0; i++ ) 
	{
		if( !isalnum( exp[i] ) && exp[i] != '_' ) 
		{
			int32_t j;
			for( j=i; exp[j]!=0; j++ ) 
			{
				if( exp[j] != ' ' && exp[j] != '\t' )
				{
					return 0;
				}
			}
			return i;
		}
	}

	return i;
}

LUADebugDriver * LUADebugDriver::search_debugger( const lua_State * state )
{
	int32_t i;
	for( i=0; i<obj_count; i++ ) 
	{
		if( all_debug[i]->luastate_ == state )
		{
			return all_debug[i];
		}
	}

	return NULL;
}

void LUADebugDriver::all_hook( lua_State *L, lua_Debug *ar )
{
	if( ar->event == LUA_HOOKLINE )
	{
		line_hook( L, ar );
	}
	else if( ar->event == LUA_HOOKCALL || ar->event == LUA_HOOKRET )
	{
		func_hook( L, ar );
	}
}

void LUADebugDriver::check_max_instructions( lua_State* L, lua_Debug* ar )
{
    ++instruction_num;

    if( instruction_num >= MAX_LUA_INSTRUCTION_NUM )
    {
        char err_str[128];
        ff_sprintf( err_str, 128, "lua instruction num: %d over flow", instruction_num );

        reset_instruction_num();

        lua_pushstring( L, err_str );
        lua_error( L );
    }
}

void LUADebugDriver::line_hook( lua_State *L, lua_Debug *ar )
{
	LUADebugDriver * db = search_debugger( L );
    if( NULL == db )
    {
        return;
    }
	//assert( db != NULL );
	if( !db->is_enable_ )
	{
		return;
	}

    check_max_instructions( L, ar );

	if( !lua_getstack( L, 0, ar ))
	{
		return;
	}
	if( lua_getinfo( L, "lnS", ar ) ) 
	{
		if( db->step_ )
		{
			db->on_event( -1, ar );
		}
		else 
		{
			// search breakpoints
			int32_t bpnum = db->search_break_point( ar );
			if( bpnum < 0 )
			{
				return;
			}
			db->on_event( bpnum, ar );
		}
	}
	else 
	{
		assert( 0 );
	}
}

void LUADebugDriver::func_hook( lua_State *L, lua_Debug *ar )
{
	LUADebugDriver * db = search_debugger( L );
    if( NULL == db ) 
    {
        return;
    }

	//assert( db != NULL);
	if( !db->is_enable_ )
	{
		return;
	}
	if( !lua_getstack( L, 0, ar ) )
	{
		return;
	}
	if( lua_getinfo( L, "lnS", ar ) ) 
	{
		// search breakpoints
		bool is_return = false;
		int32_t bpnum = -1;
		if( ar->event == LUA_HOOKCALL ) 
		{
			bpnum = db->search_break_point( ar );
		}
		else 
		{
			// event when function returns
			assert( ar->event == LUA_HOOKRET );
			
			is_return = true;
		}

		db->on_func_event( bpnum, ar, is_return );
	}
	else 
	{
		assert( 0 );
	}
}

void LUADebugDriver::prof_hook( lua_State *L, lua_Debug *ar )
{
}


int32_t LUADebugDriver::error_hook( lua_State *L )
{
	LUADebugDriver * db = search_debugger( L );
	assert( db != NULL );
	
	db->on_error();
	return 0;
}

void LUADebugDriver::on_error()
{
	if( save_error_msg_ ) 
	{
		assert( save_error_msg_ != &error_hook );
		lua_pop( luastate_, (*save_error_msg_)(luastate_) );
	}
}

LUADebugDriver::LUADebugDriver( lua_State * state )
{
	step_ = false;
	max_bp_idx_ = 0;
	luastate_ = state;
	save_error_msg_ = NULL;
	call_depth_ = -1;
	memset( search_path_, 0, sizeof( search_path_ ) );

	int32_t i;
	for( i=0; i<LUA_MAX_BRKS; i++ )
	{
		bp_[i].reset();
	}
		
	if( obj_count >= obj_alloc ) 
	{ 
        LUADebugDriver** tmp = (LUADebugDriver**)malloc( (obj_alloc + 1)*sizeof(LUADebugDriver * ) );
        if( all_debug ) {
            memcpy( tmp, all_debug, obj_alloc * sizeof(LUADebugDriver*) );
            delete []all_debug;
        }
        all_debug = tmp;
        ++obj_alloc;
	}
	all_debug[obj_count] = this;
	++obj_count;

	last_error[0] = 0;

	enable_debug( true );
}

LUADebugDriver::~LUADebugDriver()
{
	lua_atpanic( luastate_, save_error_msg_ );
	--obj_count;
	if( obj_count == 0 ) 
	{
		free( all_debug );
		all_debug = NULL;
		obj_alloc = 0;
	}
}

void LUADebugDriver::enable_line_hook( bool is_enable )
{
	int32_t mask = lua_gethookmask( luastate_ );
	if( is_enable ) 
	{
		int32_t ret = lua_sethook( luastate_, all_hook, mask | LUA_MASKLINE, 0 );
		assert( ret != 0 );
	}
	else 
	{
		int32_t ret = lua_sethook( luastate_, all_hook, mask & ~LUA_MASKLINE, 0 );
		assert( ret != 0 );
	}
}


void LUADebugDriver::enable_func_hook( bool is_enable )
{
	int32_t mask = lua_gethookmask( luastate_ );
	if( is_enable ) 
	{
		int32_t ret = lua_sethook( luastate_, all_hook, mask | LUA_MASKCALL, 0 );
		assert( ret != 0 );
		step_ = false; // ??
	}
	else 
	{
		int32_t ret = lua_sethook( luastate_, all_hook, mask & ~LUA_MASKCALL, 0 );
		assert( ret != 0 );
	}
}


void LUADebugDriver::enable_error_hook( bool is_enable )
{
	if( is_enable ) 
	{
		lua_CFunction old = lua_atpanic( luastate_, error_hook );
		if( old && old != error_hook )
		{
			save_error_msg_ = old;
		}
	}
	else
	{
		lua_atpanic( luastate_, save_error_msg_ );
	}
}

void LUADebugDriver::enable_prof_hook( bool is_enable )
{
	if( is_enable ) 
	{
		int32_t ret = lua_sethook( luastate_, prof_hook, LUA_MASKCALL | LUA_MASKRET, 0 );
		assert( ret != 0 );
		step_ = false; // ??
	}
	else 
	{
		// lua_sethook( luastate_, NULL, LUA_MASKCALL, 0 );
		int32_t ret = lua_sethook( luastate_, 0, LUA_MASKCALL|LUA_MASKRET, 0 );
		assert( ret != 0 );
	}
}

void LUADebugDriver::set_max_instruction_num( int max_ins ) 
{
    MAX_LUA_INSTRUCTION_NUM = max_ins;
}

int LUADebugDriver::get_max_instruction_num() 
{
    return MAX_LUA_INSTRUCTION_NUM;
}

void LUADebugDriver::single_step( bool is_step )
{
	if( is_step )
	{
		enable_line_hook( true );
	}
	step_ = is_step;
}

void LUADebugDriver::step_in()
{
	if( !is_single_step() )
	{
		single_step( true );
	}	
	call_depth_ = -1;
}


int32_t LUADebugDriver::add_break_point( const char * filename, int32_t line, BreakPoint::BPType type )
{
	assert( line >= 0 );

	int32_t i;
	for( i=0; i<LUA_MAX_BRKS; i++ ) 
	{
		if( bp_[i].get_type() == BreakPoint::None ) 
		{
			bp_[i].reset();
			bp_[i].set_name( filename );
			bp_[i].linenum_ = line;
			bp_[i].type_ = type;

			return i;
		}
	}

	set_error_msg( "too many breakpoints" );
	return -1;
}


int32_t LUADebugDriver::add_break_point( const char * funcname, BreakPoint::BPType type, const char* namewhat )
{
	int32_t i;
	for( i=0; i<LUA_MAX_BRKS; i++ ) 
	{
		if( bp_[i].get_type() == BreakPoint::None ) 
		{
			bp_[i].reset();
			bp_[i].set_name( funcname );
			bp_[i].linenum_ = -1;
			bp_[i].type_ = type;
			if( namewhat )
			{
				strlcpy( bp_[i].name_what_, namewhat, 7 );
			}
			else
			{
				bp_[i].name_what_[0] = 0;
			}

			return i;
		}
	}

	set_error_msg( "too many breakpoints" );
	return -1;
}


bool LUADebugDriver::path_match( const char * name, const char * file_path ) const
{
	int32_t name_len = strlen( name );
	if( name_len > 2 )
	{
		bool ret = 0;
#ifdef __linux
		size_t addr_len;
		char fn[4096];
		addr_len = strlcpy( fn, search_path_, 4096 );
        if( name[0] == '.' )
        {
		    strlcpy( fn+addr_len, name+2, 4096-addr_len );
        }
        else
        {
		    strlcpy( fn+addr_len, name, 4096-addr_len );
        }
		ret = ( strcmp( fn, file_path ) == 0 );
#else
		ret = (strcmp(name, file_path) == 0 );
#endif
		return ret;
	}
	return false;
}


int32_t LUADebugDriver::search_break_point( lua_Debug *ar ) const
{
	int32_t i;

	if( ar->source[0] == '@' ) 
	{
		// search by filename and line number
		for( i=0; i<LUA_MAX_BRKS; i++ ) 
		{
			if( bp_[i].type_ != BreakPoint::None
				&& bp_[i].linenum_ >= 0
				&& ar->currentline == bp_[i].linenum_+1
				&& path_match( ar->source+1, bp_[i].name_ ) )
			{
				return i;
			}
		}
	}

	if( ar->name &&
		( ar->event == LUA_HOOKCALL || ar->event == LUA_HOOKRET ) &&
		get_lua_var_len( ar->name ) ) 
	{
		
		// function name is valid, so check by function name
		for( i=0; i<LUA_MAX_BRKS; i++ ) 
		{
			if( bp_[i].linenum_ == -1
				&& !strcmp( ar->name, bp_[i].name_ )
				&& !strcmp( ar->namewhat, bp_[i].name_what_ ) )
			{
				return i;
			}
		}
	}

	return -1;
}


void LUADebugDriver::setup_temp_script_begin( std::stringstream & str, lua_Debug *ar ) const
{
	lua_State * L = this->luastate_;

	str << "do";

	if( ar ) 
	{
		// put all local var into a temp table
		lua_pushstring( L, tmptablename );
		lua_newtable( L );
		int32_t table = lua_gettop( L );

		int32_t i;
		const char * name;
		for( i=1; (name = lua_getlocal( L, ar, i )) != NULL; i++ ) 
		{
			// lua will create some special var in loop
			// these var's name is illegal, so we must detect and skip them
			if( get_lua_var_len( name ) ) 
			{
				str << "\nlocal " << name << " = " << tmptablename << "[\"" << name << "\"]";
				lua_pushstring( L, name );
				lua_insert( L, -2 );
				lua_settable( L, table );
			}
			else 
			{
				lua_pop( L, 1 );
			}
		}

		lua_settop( L, table );
		lua_settable( L, LUA_GLOBALSINDEX );
	}

}

void LUADebugDriver::setup_temp_script_end( std::stringstream & str, lua_Debug *ar ) const
{
	lua_State * L = this->luastate_;

	if( ar ) 
	{
		// put all local var into a temp table
		int32_t i;
		const char * name;
		for( i=1; (name = lua_getlocal( L, ar, i )) != NULL; i++ ) 
		{
			if( get_lua_var_len( name ) )
			{
				str << "\n" << tmptablename << "[\"" << name << "\"] = " << name;
			}
			lua_pop( L, 1 );
		}
	}
	str << "\nend";
}

void LUADebugDriver::temp_table_cleanup( lua_Debug *ar ) const
{
	lua_State * L = this->luastate_;

	lua_pushstring( L, tmptablename );
	lua_pushvalue( L, -1 );
	lua_gettable( L, LUA_GLOBALSINDEX );
	int32_t table = lua_gettop( L );

	if( ar ) 
	{
		// copy the local var from temporary table back
		int32_t i;
		const char * name;
		for( i=1; (name = lua_getlocal( L, ar, i )) != NULL; i++ ) 
		{
			if( get_lua_var_len( name ) ) 
			{
				lua_pushstring( L, name );
				lua_gettable( L, table );
				const char * name2 = lua_setlocal( L, ar, i );
				assert( name2!=NULL && !strcmp( name, name2 ) );
			}
			lua_pop( L, 1 );
		}
	}
	
	lua_settop( L, table-1 );
	lua_pushnil( L );
	lua_settable( L, LUA_GLOBALSINDEX );
}

void LUADebugDriver::record( const char* format, ... )
{
    return;
}

// -- LUADebugDriver结束

// -- SourceMananger开始

class FileSource 
{  
	int32_t line_count;
		
	GAutoPtr<std::string, 4> lines_;
public:
	std::string name_;
	int32_t access_count;

	FileSource()
	{ 
		line_count = -1;
	}
	bool load_file( const char * filename );
	void upload() 
	{ 
		line_count = -1; 
		access_count = 0; 
		lines_.number_ = 0; 
	}
	bool is_loaded() const { return line_count != -1; }
		
	const char * get_line( int32_t l ) 
	{
		if( l>=0 && l<line_count ) 
		{
			++access_count;
			return (const char *)lines_[l].c_str();
		}
		return NULL;
	}

};

bool FileSource::load_file( const char * filename )
{
	FILE * fp = fopen( filename, "r" );
	if( !fp )
	{
		return false;
	}

	char buf[512];
    std::string line("");
    bool read_complete = false;
	line_count = 0;

	while( fgets( buf, sizeof(buf), fp ) != NULL) 
	{
		int32_t i;
		for( i=0; buf[i]!=0; i++ ) 
		{
			if( buf[i]=='\r' || buf[i]=='\n' ) 
			{
				buf[i] = 0;
                line = line + buf;
                read_complete = true;
                break;
			}
            read_complete = false;
		}

        if( read_complete )
        {
	        if( line_count >= lines_.size_ )
		    {
			    lines_.increase();
		    }

		    lines_[line_count] = line;
		    lines_.number_ = ++line_count;
            line = "";
        }
        else
        {
            line = line + buf;
        }
	}
	name_ = filename;
	fclose( fp );
	return true;
}

class SourceFileManager 
{
private:
	enum { max_num_ = 4 }; // a workaround for VC

	FileSource files_[max_num_];

public:
	const char * load_file_line( const char * filename, int32_t linenum );
	void reload_file();
};

void SourceFileManager::reload_file()
{
	for( int32_t i=0; i<max_num_; i++ )
	{	
		if( files_[i].name_ != "" )
		{
			files_[i].load_file( files_[i].name_.c_str() );
		}
	}
}

const char * SourceFileManager::load_file_line( const char * filename, int32_t linenum )
{
	int32_t i;
	int32_t ie = -1;
	int32_t min = 999999;
	int32_t im = 0;
	for( i=0; i<max_num_; i++ ) 
	{
		if( files_[i].is_loaded() )
		{
			if( files_[i].name_ == filename )
			{
				return files_[i].get_line( linenum );
			}
			else 
			{
				if( files_[i].access_count < min )
				{
					im = i;
					min = files_[i].access_count;
				}
			}
		}
		else 
		{
			if( ie < 0 )
			{
				ie = i;
			}
		}
	}
	if( ie >= 0 ) 
	{
		if( !files_[ie].load_file( filename ) )
		{
			return NULL;
		}
		return files_[ie].get_line( linenum );
	}
	
	files_[im].upload();
	if( !files_[im].load_file( filename ) )
	{
			return NULL;
	}
	return files_[im].get_line( linenum );
}

// -- SourceManager结束

// -- GScriptDebug 开始

GScriptDebug::GScriptDebug( lua_State * state ):
	LUADebugDriver(state), lua_svr_(NULL)
{
	memset( last_cmd_, 0, sizeof( last_cmd_ ) );
	input_func_ = NULL;
	src_manager_ = new SourceFileManager;
}

GScriptDebug::~GScriptDebug()
{
	delete src_manager_;
	src_manager_ = NULL;
}

void GScriptDebug::set_prompt()
{
	fprintf( stdout, "(ldb) " );
	fflush( stdout );
}

// search for a var in lua stack
// return local var's index num, 0 when not found
static int32_t search_local_var( lua_State *L, lua_Debug * ar, const char * varname, int32_t varlen )
{
	int32_t i;
	const char * name;
	for( i=1; (name = lua_getlocal(L, ar, i)) != NULL; i++ ) 
	{
		if( (int32_t)strlen( name ) == varlen && !strncmp( name, varname, varlen ) )
		{
			return i;
		}
		// not match, pop out the var's value
		lua_pop( L, 1 );
	}
	return 0;
}

static bool search_global_var( lua_State *L, const char * varname, int32_t varlen )
{
	// char * dv = new char [varlen+1];
	char dv[512];
	int32_t minlen = 511 > varlen ? varlen : 511;
	ff_strncpy( dv, sizeof( dv ), varname, minlen );
	dv[minlen]=0;
	lua_getglobal( L, dv );

	if( lua_type( L, -1 ) == LUA_TNIL ) 
	{
		lua_pop( L, 1 );
		return false;
	}

	return true;
}

static int32_t getcalldepth( lua_State *L )
{
	int32_t i = 0;
	lua_Debug ar;
	for( ; lua_getstack( L, i+1, &ar )!=0; i++ );
	return i;
}

void GScriptDebug::print_string_var( int32_t si, int32_t depth )
{
	lua_State * L = this->luastate_;

	print( "\"" );

	// value string may contains NUL and/or '"'
	const char * val = lua_tostring( L, si );
	int32_t vallen = lua_strlen( L, si );
	int32_t i;
	const char spchar[] = "\"\t\n\r";
	for( i=0; i<vallen; ) 
	{
		if( val[i] == 0 ) 
		{
			print( "\\000" );
			++i;
		}
		else if( val[i] == '"' ) 
		{
			print( "\\\"" );
			++i;
		}
		else if( val[i] == '\\' ) 
		{
			print( "\\\\" );
			++i;
		}
		else if( val[i] == '\t' ) 
		{
			print( "\\t" );
			++i;
		}
		else if( val[i] == '\n' ) 
		{
			print( "\\n" );
			++i;
		}
		else if( val[i] == '\r') 
		{
			print( "\\r" );
			++i;
		}
		else 
		{
			int32_t splen = strcspn( val+i, spchar );
			assert( splen > 0 );

			print( "%.*s", splen, val+i );
			i += splen;
		}
	}
	print( "\"" );
}

void GScriptDebug::print_table_var( int32_t si, int32_t depth )
{
	lua_State * L = this->luastate_;

	int32_t pos_si = si>0 ? si : (si-1);
	print( "{" );
	int32_t top = lua_gettop( L );
	lua_pushnil( L );
	bool empty = true;
	while( lua_next( L, pos_si ) !=0 ) 
	{
		if( empty )
		{
			print( "\n" );
			empty = false;
		}

		int32_t i;
		for( i=0; i<depth; i++ )
		{
			print( "\t" );
		}

		print( "[" );
		print_var( -2 );
		print( "] = " );
		if( depth > 5 )
		{
			print( "{...}" );
		}
		else
		{
			print_var( -1, depth+1 );
		}
		lua_pop(L, 1);
		print(",\n");
	}
	if (empty)
	{
		print(" }");
	}
	else 
	{
		int32_t i;
		for (i=0; i<depth-1; i++)
		{
			print("\t");
		}
		print("}");
	}
	lua_settop(L, top);
}

void GScriptDebug::print_var( int32_t si, int32_t depth )
{
	lua_State * L = this->luastate_;

	switch( lua_type(L, si) ) 
	{
	case LUA_TNIL:
		print( "(nil)" );
		break;

	case LUA_TNUMBER:
		print( "%f", lua_tonumber( L, si ) );
		break;

	case LUA_TBOOLEAN:
		print( "%s", (lua_toboolean( L, si )? "true":"false") );
		break;

	case LUA_TFUNCTION: 
		{
			lua_CFunction func = lua_tocfunction( L, si );
			if( func != NULL )
			{
				print( "(C function)0x%p", func );
			}
			else
			{
				print( "(function)" );
			}
		}
		break;

	case LUA_TUSERDATA:
		print( "(user data)0x%p", lua_touserdata( L, si ) );
		break;

	case LUA_TSTRING: 
		print_string_var( si, depth );
		break;
		
	case LUA_TTABLE: 
		print_table_var( si, depth );
		break;

	default:
		print("(%s)", lua_typename(L, si));
	}
}


void GScriptDebug::print_expression( const char * exp, lua_Debug *ar )
{
	lua_State * L = this->luastate_;

	int32_t explen = get_lua_var_len( exp );
	if( explen ) 
	{
		// it is just a simple var

		if( ar!=NULL && search_local_var( L, ar, exp, explen ) ) 
		{
			// found local var match that
			print( "local %.*s = ", explen, exp );
			print_var( -1 );
			lua_pop( L, 1 );
			print( "\n" );
		}
		else if( search_global_var( L, exp, explen ) ) 
		{
			print( "global %.*s = ", explen, exp );
			print_var( -1 );
			lua_pop( L, 1 );
			print( "\n" );
		}
		else 
		{
			print( "variable %.*s not found\n", explen, exp );
		}
	}
	else 
	{
		// it is a expression

		std::stringstream str;
		setup_temp_script_begin( str, ar );

		str << "\n" << tmpvarname << " = (" << exp << " )";
		setup_temp_script_end( str, ar );
		
		if( luaL_loadbuffer( L, str.str().c_str(), str.str().length(), "Debugger Temp Script" )
			|| llua_call( L, 0, 0, 0 ) )
		{
            print( "%s\n", lua_tostring( L, -1 ) );
            lua_pop( L, 1 );

			print( "%.*s\n", str.str().length(), str.str().c_str() );
			print( "failed to evaluate expression: %s\n", exp );
			return;
		}

		temp_table_cleanup( ar );

		lua_pushstring( L, tmpvarname );
		lua_gettable( L, LUA_GLOBALSINDEX );
		print_var( -1 );
		print( "\n" );
		lua_pop( L, 1 );
	}
}

void GScriptDebug::reload()
{
	src_manager_->reload_file();
	for( int32_t i = 0; i < LUA_MAX_BRKS; ++i )
	{
		bp_[i].reset();
	}
}

void GScriptDebug::debug_dump_stack( int32_t depth, int32_t verbose )
{
	lua_State * state = this->luastate_;
	lua_Debug ldb;
	int32_t i;
	for( i=depth; lua_getstack(state, i, &ldb)==1; i++ ) 
	{
		lua_getinfo( state, "Slnu", &ldb );
		const char * name = ldb.name;
		if( !name )
		{
			name = "";
		}
		const char * filename = ldb.source;

		print( "#%d: %s:'%s', '%s' line %d\n",
					i+1-depth, ldb.what, name, filename, ldb.currentline );

		//convert file name		
		size_t addr_len;
		char fn[4096];
		addr_len = strlcpy( fn, search_path_, 4096 );
		strlcpy( fn+addr_len, filename+3, 4096-addr_len ); // @./

		if( verbose ) 
		{
			if( ldb.source[0]=='@' && ldb.currentline!=-1 ) 
			{
				// print the source line
				const char * line = src_manager_->load_file_line( fn, ldb.currentline-1 );
				if( line ) 
				{
					print( "%s\n", line );
				}
				else 
				{
					print( "[no source available]\n" );
				}
			}
			else 
			{
				print( "[no source available]\n" );
			}
		}
	}
}

void GScriptDebug::set_input_func( INPUT_FUNC _input_func )
{
	input_func_ = _input_func;
}

int32_t GScriptDebug::get_input( char* _buff, int32_t _size )
{
	if( input_func_ )
	{
		return (*input_func_)( _buff, _size );
	}
	return -1;
}

bool GScriptDebug::handle_command_ldb( const char * cmd, lua_Debug * ar )
{
	// parse command
	char cmd_new[4096];
	copy_with_back_space( cmd_new, cmd );
	const char ws[] = " \t\r\n";
	const char * p = cmd_new;
	int32_t len;

	if( *p == 0 ) 
	{
		if( last_cmd_ ) 
		{
			p = last_cmd_;
			set_prompt();
			print( "%s\n", p );
		}
		else 
		{
			// empty command
			return true;
		}
	}

	p += strspn( p, ws ); // skip leading whitespace
	len = strcspn( p, ws ); // command len

	char command[32];
	int32_t min_len = len < 31 ? len : 31;
	ff_strncpy( command, sizeof( command ), p, min_len );
	command[min_len] = 0;
	for( int32_t i=0; entries_ldb[i].cmd!=NULL; i++ )
	{
		if( !strcmp( entries_ldb[i].cmd, command ) )
		{
			CommandHandlerLdb pfn = entries_ldb[i].handler;
			const char * op = p + len + strspn( p+len, ws );
			int32_t oplen = strcspn( op, ws );
			if( op == p + len )
			{
				oplen = 0;
			}
			return (this->*(pfn))( p, op, oplen, ar );
		}
	}

	// handle all other command.
	handle_command( cmd, ar );

	return true;
}

void GScriptDebug::on_event( int32_t bpnum, lua_Debug *ar )
{
	lua_State *L = luastate_;

	int32_t depth = getcalldepth( L );

	assert( call_depth_ >= -1 );

	if( bpnum<0 && call_depth_!=-1 && depth > call_depth_ ) 
	{
		// if not hit by a breakpoint
		// we are step into a deeper function, and last command should be 'n'
		// so nothing to do
		return;
	}

	call_depth_ = depth; // save how deep we are

	if( bpnum >= 0 ) 
	{
		print("Breakpoint #%d hit!\n", bpnum);
	}
	set_prompt();

	if( ar && ar->source[0]=='@' && ar->currentline!=-1 ) 
	{		
		//convert file name	
		char* line = NULL;
#ifdef __linux
		size_t addr_len;
		char fn[4096];
		addr_len = strlcpy( fn, search_path_, 4096 );
        if( ar->source[1] == '.' )
        {
		    strlcpy( fn+addr_len, ar->source+3, 4096-addr_len ); // @./
        }
        else
        {
		    strlcpy( fn+addr_len, ar->source+1, 4096-addr_len ); // @./
        }
		line = (char* )src_manager_->load_file_line( fn, ar->currentline-1 );
#else
		line = (char* )src_manager_->load_file_line( ar->source+1, ar->currentline-1 );
#endif

		if( !line ) 
		{
			print( "[[cannot load source %s:%d]\n", ar->source+1, ar->currentline );
		}
		else 
		{
			print( "[%s:%d]\n%s\n", ar->source+1, ar->currentline, line );
		}
		set_prompt();
	}

	for( ; ; set_prompt() ) 
	{
		// get input
		char buffer[256];
		if( get_input( buffer, 256 ) <= 0 )
		{
			continue;
		}

		// backspace \13
		
		if( !handle_command_ldb( buffer, ar ) )
		{
			break;
		}

	}
}

void GScriptDebug::on_exception( lua_Debug * ar )
{
	set_prompt();
	
	if( ar && ar->source[0]=='@' && ar->currentline!=-1 ) 
	{		
		//convert file name		
		size_t addr_len;
		char fn[4096];
		addr_len = strlcpy( fn, search_path_, 4096 );
		strlcpy( fn+addr_len, ar->source+3, 4096-addr_len ); // @./

		const char * line = src_manager_->load_file_line( fn, ar->currentline-1 );

		if( !line ) 
		{
			print( "[[cannot load source %s:%d]\n", ar->source+1, ar->currentline );
		}
		else 
		{
			print( "[%s:%d]\n%s\n", ar->source+1, ar->currentline, line );
		}
		set_prompt();
	}

	for( ; ; set_prompt() ) 
	{
		// get input
		char buffer[256];
		if( get_input( buffer, 256 ) <= 0 )
		{
			continue;
		}
		
		if( !handle_command_ldb( buffer, ar ) )
		{
			break;
		}

	}
}

void GScriptDebug::err_func()
{
	if( !is_enable_ )
	{
		return;
	}

	lua_State* L = luastate_;
	lua_Debug ar;

	if( !lua_getstack( L, 1, &ar ))
	{
		return;
	}
	if( lua_getinfo( L, "lnS", &ar ) ) 
	{
		print( "Exception: %s\n", lua_tostring( L, -1 ) );
		on_exception( &ar );
	}
	return;
}

int32_t GScriptDebug::add_break_point( const char * filename, int32_t linenum, BreakPoint::BPType type )
{
	const char * line = src_manager_->load_file_line( filename, linenum );
	if( !line ) 
	{
		print( "cannot load source file %s, line %d\n", filename, linenum+1 );
		return -1;
	}
	return LUADebugDriver::add_break_point( filename, linenum, type );
}

void GScriptDebug::handle_func_break_point_command( const char * op, int32_t oplen )
{
	lua_State* L = this->luastate_;
	int32_t top = lua_gettop( L );

	// get function from table, such as TABLE.function
	char* t = const_cast<char*>( op );
	char* tv = (char*)memchr( t, '.', oplen );
	int32_t index = LUA_GLOBALSINDEX;

	while( tv )
	{
		lua_pushlstring( L, t, tv-t );
		lua_gettable( L, index );
		if( lua_isnil( L, -1 ) ) 
		{
			lua_settop( L, top );
			print( "Table '%.*s' not found!\n", tv-t, t );
			return;
		}
		index = lua_gettop( L );
		t = ++tv;
		tv = (char*)memchr( t, '.', oplen-(tv-t) );
	}
	//--

	int32_t fl = oplen-(t-op);
	lua_pushlstring( L, t, fl );
	lua_gettable( L, index );
	
	if( lua_isnil( L, -1 ) ) 
	{
		print( "Function '%.*s' not found!\n", fl, t );
	}
	else if( lua_iscfunction( L, -1 ) ) 
	{
		print("Cannot break on C Function '%.*s'.\n", fl, t);
	}
	else if( lua_isfunction(L, -1) ) 
	{
		// char * tmp = new char[fl+1];
		char tmp[512];
		int32_t minlen = 511 > fl ? fl : 511;
		ff_strncpy( tmp, sizeof( tmp ), t, minlen );
		tmp[minlen] = 0;
		int32_t bpnum = -1;
		if( t == op )
		{
			bpnum = LUADebugDriver::add_break_point( tmp, BreakPoint::User, "global" );
		}
		else
		{
			bpnum = LUADebugDriver::add_break_point( tmp, BreakPoint::User, "field" );
		}

		if( bpnum < 0 ) 
		{
			print( "set breakpoint failed!\n" );
			return;
		}

		
		print( "Breakpoint #%d is set at function '%.*s'\n",
				bpnum, oplen, op );

		enable_func_hook( true );
		lua_settop( L, top );
		return;
	}
	else 
	{
		print( "'%.*s' is not a function!\n", fl, t );
	}
	lua_settop( L, top );
}

void GScriptDebug::handle_line_break_point_command( const char * dv, const char * op, int32_t oplen )
{
	int32_t linenum, sc;
	if( sscanf( dv+1, "%d%n", &linenum, &sc )==0 || sc!=oplen-(dv-op)-1 || linenum<=0 ) 
	{
		print( "Invalid line number" );
		return;
	}

	char fn[4096];
	
	if( search_path_ && (*op!='/' && *op!='\\') ) 
	{
		size_t addlen;
		// fn = new char[dv-op+addlen+1];
		addlen = strlcpy( fn, search_path_, 4096 ); // there is already a \ in search_path_
		int32_t minlen = 4095-addlen > dv-op ? dv-op : 4095-addlen;
		strncat( fn + addlen, op, minlen );
	}
	else 
	{
		// fn = new char[dv-op+1];
		int32_t minlen = 4095 > dv-op ? dv-op : 4095;
		ff_strncpy( fn, sizeof( fn ), op, minlen );
		fn[minlen]=0;
	}
	
	const char * line;
	line = src_manager_->load_file_line( fn, linenum-1 );   
	if( !line ) 
	{
		print( "Cannot load source %s, line %d!\n", fn, linenum );
		return;
	}

	int32_t bpnum = add_break_point( fn, linenum-1 );


	if( bpnum < 0 ) 
	{
		print( "Add breakpoint failed: %s\n", get_last_error_msg() );
		return;
	}

	print( "Breakpoint #%d is set at %.*s, line %d:\n%s\n", bpnum, dv-op, op, linenum, line );

	enable_line_hook( true );
}

void GScriptDebug::handle_break_point_command( const char * op, int32_t oplen )
{
	// search for ':'
	const char * dv = (const char *)memchr( op, ':', oplen );
	if( !dv ) 
	{
		handle_func_break_point_command( op, oplen );
	}
	else
	{
		handle_line_break_point_command( dv, op, oplen );
	}
}


void GScriptDebug::on_func_event( int32_t bpnum, lua_Debug *ar, bool is_return )
{
	if( !is_return ) 
	{
		if( bpnum >= 0 ) 
		{
			const BreakPoint * bp = get_break_point( bpnum );
			assert( bp != NULL );
			
			if( bp->get_type() == BreakPoint::User ) 
			{
				// user want to break when this function is called
				// now the function is about to be called, but not called yet.
				// we let the debugger stop the script at the next executed line
				print("Breakpoint #%d hit!\n", bpnum);
	
				step_in();

				return;
			}

			// this breakpoint is used by debugger itself
			// debugger may use this event to profile some functions calls.
		}

		return;
	}
	
	// returning from some function
}


void GScriptDebug::handle_command(const char * cmd, lua_Debug *ar)
{
	int32_t depth = getcalldepth( luastate_ );
	
	assert( call_depth_ >= -1 );

	call_depth_ = depth; // save how deep we are

	char cmd_new[4096];
	copy_with_back_space( cmd_new, cmd );
	const char ws[] = " \t\r\n";
	const char * p = cmd_new;
	int32_t len;

	p += strspn( p, ws ); // skip leading whitespace
	len = strcspn( p, ws ); // command len


	char command[32];
	int32_t min_len = len < 31 ? len : 31;
	ff_strncpy( command, sizeof( command ), p, min_len );
	command[min_len] = 0;
	for( int32_t i=0; entries[i].cmd!=NULL; i++ )
	{
		if( !strcmp( entries[i].cmd, command ) )
		{
			CommandHandler pfn = entries[i].handler;
			const char * op = p + len + strspn( p+len, ws );
			int32_t oplen = strcspn( op, ws );
			if( op == p + len )
			{
				oplen = 0;
			}
			(this->*(pfn))( op, oplen, ar );
			return;
		}
	}

}

void GScriptDebug::print( const char * format, ... ) const
{
	va_list argList;
	va_start( argList, format );
	vfprintf( stdout, format, argList );
	va_end( argList );
	fflush( stdout );
}

void GScriptDebug::copy_with_back_space( char * des, const char * src ) const
{
	char * pdes = des;
	const char * psrc = src;
	while( *psrc )
	{
		if( *psrc == 8 )
		{
			if( pdes > des )
			{
				pdes--;
			}
		}
		else
		{
			*(pdes++) = *psrc;
		}
		psrc++;
	}
	*pdes = 0;
}

void GScriptDebug::on_error()
{
	LUADebugDriver::on_error();

	lua_State *L = luastate_;

	print( "script error: %s\n", lua_tostring(L, -1) );
	debug_dump_stack( 1, 1 );
}

void GScriptDebug::set_search_path( const char * path )
{
	if( !path || *path==0 )
	{
		return;
	}

	int32_t len = strlen( path );
#ifdef __linux
#define SLASH '/'
#else
#define SLASH '\\'
#endif
	if( path[len-1] == SLASH )
	{
		strlcpy( search_path_, path, 4096 );
	}
	else
	{
		strlcpy( search_path_, path, 4096 );
		if( len < 4095 )
		{
			search_path_[len] = SLASH; 
			search_path_[len+1] = 0;
		}
	}
}

void GScriptDebug::save_last_command( const char * cmd )
{
	strlcpy( last_cmd_, cmd, 4096 );
}

GScriptDebug::Entry GScriptDebug::entries[] =
{
	{ "b", &GScriptDebug::handle_cmd_breakpoint },
	{ "delete", &GScriptDebug::handle_cmd_delete },
	{ "list", &GScriptDebug::handle_cmd_list },
	{ "p", &GScriptDebug::handle_cmd_print },
	{ "stat", &GScriptDebug::handle_cmd_stat },
	{ "path", &GScriptDebug::handle_cmd_path },
	{ "help", &GScriptDebug::handle_cmd_help },
	{ "?", &GScriptDebug::handle_cmd_help },
	{ "enable", &GScriptDebug::handle_cmd_enable },
	// { "line", &GScriptDebug::handle_cmd_line },
	// { "func", &GScriptDebug::handle_cmd_func },
	// { "error", &GScriptDebug::handle_cmd_error },
	{ "disable", &GScriptDebug::handle_cmd_disable },
	{ "do", &GScriptDebug::handle_cmd_do },
	{ "dofile", &GScriptDebug::handle_cmd_dofile },
	{ "clear", &GScriptDebug::handle_cmd_clear },
	{ "debug", &GScriptDebug::handle_cmd_debug },
	{ "trace", &GScriptDebug::handle_cmd_trace },
	{ "reload", &GScriptDebug::handle_cmd_reload },
	{ "instruction", &GScriptDebug::handle_cmd_instruction },
	{ NULL, NULL }
};

GScriptDebug::EntryLdb GScriptDebug::entries_ldb[] =
{
	{ "c", &GScriptDebug::handle_cmd_continue },
	{ "n", &GScriptDebug::handle_cmd_next },
	{ "s", &GScriptDebug::handle_cmd_step },
	{ "bt", &GScriptDebug::handle_cmd_backtrace },
	// { "setlocal", &GScriptDebug::handle_cmd_setlocal },
	// { "setglobal", &GScriptDebug::handle_cmd_setglobal },
	{ NULL, NULL }
};


// 以下是普通命令

void GScriptDebug::handle_cmd_breakpoint( const char * op, int32_t oplen, lua_Debug * ar )
{
	if( oplen == 0 ) 
	{
		print( "Incorrect use of 'b'\n" );
		print( "b <filename>:<linenum>\n" );
		print( "b <funcname>\n" );
		return;
	}	   

	handle_break_point_command( op, oplen );
}

void GScriptDebug::handle_cmd_delete( const char * op, int32_t oplen, lua_Debug * ar )
{
	if( oplen==0 ) 
	{
		print( "Incorrect use of 'delete'\n" );
		print( "delete <breakpointnum>\n" );
		return;
	}

	// op should be a breakpoint number
	int32_t bpnum, sc;
	if( sscanf( op, "%d%n", &bpnum, &sc )==0 ||
			sc!=oplen || bpnum<0 || bpnum>LUA_MAX_BRKS )
	{
		print( "Invalid breakpoint number.\n" );
		return;
	}

	if( get_break_point( bpnum )->get_type() == BreakPoint::None ) 
	{
		print( "Breakpoint #%d not found\n", bpnum );
		return;
	}

	remove_break_point( bpnum );
	print( "Breakpoint #%d removed\n", bpnum );
}

void GScriptDebug::handle_cmd_list( const char * op, int32_t oplen, lua_Debug * ar )
{
	// list all breakpoints
	int32_t i;
	for( i=0; i<LUA_MAX_BRKS; i++ ) 
	{
		const BreakPoint * bp = get_break_point( i );
		if( bp->get_type() != BreakPoint::None ) 
		{
			if( bp->get_line() == -1 )
			{
				print( "#%d: func breakpoint( name: %s, what: %s )\n", i, bp->get_name(), bp->get_name_what() );
			}
			else
			{
				print( "#%d: line breakpoint( file: %s, line: %d )\n", i, bp->get_name(), bp->get_line()+1 );
			}
		}
	}
}

void GScriptDebug::handle_cmd_print( const char * op, int32_t oplen, lua_Debug * ar )
{
	if( oplen == 0 ) 
	{
		print( "Incorrect use of 'p'\n" );
		print( "p <varname>\n" );
		return;
	}

    if( strncmp( op, "INSTRUCTION", 11 ) == 0 )
    {
        print( "instruction num: %d\n", instruction_num );
    }
    else
    {
        print_expression( op, ar );
    }
}

void GScriptDebug::handle_cmd_stat( const char * op, int32_t oplen, lua_Debug * ar )
{
	if( is_enable_ )
	{
		print( "Debugger is enable\n" );
	}
	else
	{
		print( "Debugger is disable\n" );
	}

	if( is_single_step() ) 
	{
		print( "Under step by step mode\n" );
	}
}

void GScriptDebug::handle_cmd_path( const char * op, int32_t oplen, lua_Debug * ar )
{
	// set file search path
	if( oplen == 0 ) 
	{
		print( "Current debug path: %s\n", search_path_ );
		return;
	}

	set_search_path( op );
}

void GScriptDebug::handle_cmd_help( const char * op, int32_t oplen, lua_Debug * ar )
{
	print( "List of commands:\n" );
	print( "\n" );
	print( "b <filename>:<linenum>           -- Add breakpoint\n" );
	print( "b <funcname>                     -- Add breakpoint(function)\n" );
	print( "clear                            -- Clear all breakpoints\n" );
	print( "debug                            -- Enter debugger\n" );
	print( "delete <breakpointnum>           -- Delete a breakpoint\n" );
	print( "disable                          -- Disable debugger\n" );
	print( "do <script>                      -- Execute a script string (you can use local var)\n" );
	// print( "dofile <filename>                -- Execute a file\n" );
	print( "enable                           -- Enable debugger\n" );
	// print( "error                            -- Enable error hook\n" );
	// print( "func                             -- Enable function call hook\n" );
	// print( "line                             -- Enable line hook\n" );
	print( "list                             -- List all breakpoints\n" );
	print( "p <varname>                      -- Print a variable or any expression that can be evaluated to a value\n" );
	print( "path                             -- Display current debug path\n" );
	print( "reload                           -- Reload the script\n" );
	print( "\n" );
	print( "The following commands are ONLY available when script is stopped:\n" );
	print( "\n" );
	print( "bt                               -- Print call stack\n" );
	print( "c                                -- Continue to execute\n" );
	print( "n                                -- Step over\n" );
	print( "s                                -- Step into\n" );
	// print( "setglobal <localvar> <globalvar> -- Set a local var's value equal to a global var\n" );
	// print( "setlocal <globalvar> <localvar>  -- Set a global var's value equal to a local var\n" );
}

void GScriptDebug::handle_cmd_enable( const char * op, int32_t oplen, lua_Debug * ar )
{
	enable_debug( true );
	print( "Debugger is enabled\n" );
}

void GScriptDebug::handle_cmd_line( const char * op, int32_t oplen, lua_Debug * ar )
{
	enable_line_hook( true );
	print( "Line hook is enabled\n" );
}

void GScriptDebug::handle_cmd_func( const char * op, int32_t oplen, lua_Debug * ar )
{
	enable_func_hook( true );
	print( "Function call hook is enabled\n" );
}

void GScriptDebug::handle_cmd_error( const char * op, int32_t oplen, lua_Debug * ar )
{
	enable_func_hook( true );
	print( "Error hook is enabled\n" );
}

void GScriptDebug::handle_cmd_disable( const char * op, int32_t oplen, lua_Debug * ar )
{
	enable_debug( false );
	print( "Debugger is disabled\n");
}

void GScriptDebug::handle_cmd_do( const char * op, int32_t oplen, lua_Debug * ar )
{
	lua_State* L = luastate_;
	// call lua_do
	if( *op == 0 ) 
	{
		print( "Invalid use of 'do'\n" );
		print( "do <script>\n" );
		return;
	}

	std::stringstream str;
	setup_temp_script_begin( str, ar );

	str << " do " << op << " end";

	setup_temp_script_end( str, ar );

	int32_t err = luaL_loadbuffer( L, str.str().c_str(), str.str().length(), "Debugger Temp Script" );

	switch( err ) 
	{
	case 0:
        {
            // no error
            int last_max_ins_num = get_max_instruction_num();
            set_max_instruction_num( 0x7fffffff );
            int32_t err = llua_call( L, 0, 0, 0 );
            set_max_instruction_num( last_max_ins_num );
            if( err ) {
                print( "%s\n", lua_tostring( L, -1 ) );
                lua_pop( L, 1 );
            }
        }
		break;

	case LUA_ERRRUN:
        lua_pop( L, 1 );
		print( "Error while running\n" );
		break;

	case LUA_ERRSYNTAX:
        lua_pop( L, 1 );
		print( "Error in syntax\n" );
		break;

	case LUA_ERRMEM:
        lua_pop( L, 1 );
		print( "Error in memory\n" );
		break;

	default:
        lua_pop( L, 1 );
		print( "some other error\n" );
		break;
	}
	temp_table_cleanup( ar );
}

void GScriptDebug::handle_cmd_dofile( const char * op, int32_t oplen, lua_Debug * ar )
{
	lua_State * L = luastate_;
	// do file
	if( oplen == 0 ) 
	{
		print( "Incorrect use of 'dofile'\n" );
		print( "dofile <filename>\n" );
		return;
	}

    int last_max_ins_num = get_max_instruction_num();
    set_max_instruction_num( 0x7fffffff );
	bool succ = lua_svr_->do_file( op );
    set_max_instruction_num( last_max_ins_num );

	if( succ )
	{
		print( "Dofile '%s' done!\n", op );
	}
	else 
	{		// 1
        print( "%s\n", lua_tostring( L, 1 ) );
        lua_pop( L, 1 );

		print( "Error occur!\n" );
	} 
}

void GScriptDebug::handle_cmd_clear( const char * op, int32_t oplen, lua_Debug * ar )
{
	for( int32_t i = 0; i < LUA_MAX_BRKS; ++i )
	{
		bp_[i].reset();
	}
	print( "All breakpoints cleared\n" );
}

void GScriptDebug::handle_cmd_debug( const char * op, int32_t oplen, lua_Debug * ar )
{
	step_in();
}

void GScriptDebug::handle_cmd_trace( const char * op, int32_t oplen, lua_Debug * ar )
{
}

void GScriptDebug::handle_cmd_reload( const char * op, int32_t oplen, lua_Debug * ar )
{
	if( lua_svr_ ) 
    {
		lua_svr_->reload();
	}
	else {
		print( "Err! LuaServer not set!!" );
	}
}

void GScriptDebug::handle_cmd_instruction( const char * op, int32_t oplen, lua_Debug * ar )
{
    char* pend;
    long max_ins = strtol( op, &pend, 10 );
    if( *pend != '\0' )       
    {
        print( "Invalid input. instruction num: %s", op );
        return;
    }

    MAX_LUA_INSTRUCTION_NUM = (int)max_ins;
    print( "max lua instruction num set to %d", MAX_LUA_INSTRUCTION_NUM );
}

// 以下是断点处使用命令

bool GScriptDebug::handle_cmd_continue( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar )
{
	// continue and stop single step mode
	print( "Continue...\n" );
	single_step(false);
	call_depth_ = -1;
	return false;
}

bool GScriptDebug::handle_cmd_next( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar )
{
	// next
	if( !is_single_step() )
	{
		single_step( true );
	}
	if( cmd != last_cmd_ )
	{
		save_last_command( cmd );
	}
	return false;
}

bool GScriptDebug::handle_cmd_step( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar )
{
	// step into function call
	step_in();
			
	if( cmd != last_cmd_ )
	{
		save_last_command( cmd );
	}
	return false;
}

bool GScriptDebug::handle_cmd_backtrace( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar )
{
	// dump call stack
	debug_dump_stack( 0, 0 );
	return true;
}

bool GScriptDebug::handle_cmd_setlocal( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar )
{
	lua_State* L = luastate_;
	const char* ws = " \t\r\n";

	// set a local var's value from a global var
	const char * op2 = op + oplen + strspn( op+oplen, ws );
	int32_t op2len = strcspn( op2, ws );

	if( oplen==0 || op2 == op+oplen || op2len==0 ) 
	{
		print( "Incorrect use of 'setlocal'\n" );
		print( "setlocal <localvar> <globalvar>\n" );
		return true;
	}

	int32_t varidx;
	if( !(varidx=search_local_var( L, ar, op, oplen )) ) 
	{
		print("Local var '%.*s' not found!\n", oplen, op);
		return true;
	}
	
	lua_pushlstring( L, op2, op2len );
	lua_gettable( L, LUA_GLOBALSINDEX );

	if( !lua_setlocal( L, ar, varidx ) ) 
	{
		print( "Set local var '%.*s' failed!\n", oplen, op );
		return true;
	}

	return true;
}

bool GScriptDebug::handle_cmd_setglobal( const char * cmd, const char * op, int32_t oplen, lua_Debug * ar )
{
	lua_State* L = luastate_;
	const char* ws = " \t\r\n";

	// set a local var's value from a global var
	const char * op2 = op + oplen + strspn( op+oplen, ws );
	int32_t op2len = strcspn( op2, ws );

	if( oplen==0 || op2 == op+oplen || op2len==0 ) 
	{
		print( "Incorrect use of 'setlocal'\n" );
		print( "setglobal <globalvar> <localvar>\n" );
		return true;
	}

	lua_pushlstring( L, op2, op2len );
	lua_gettable( L, LUA_GLOBALSINDEX );  // the key: global var name

	int32_t varidx;
	if( !(varidx=search_local_var( L, ar, op2, op2len )) ) 
	{
		print( "Local var '%.*s' not found!\n", op2len, op2 );
		return true;
	}

	lua_rawset( L, -3 );

	return true;
}

// -- GScriptDebug结束


