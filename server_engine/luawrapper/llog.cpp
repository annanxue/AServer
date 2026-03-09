#include <time.h>
#include <sys/stat.h>
#include "stdio.h"
#include "llog.h"
#include "ffsys.h"
#include "log.h"
#include "lmisc.h"
#include "netmng.h"
#include "serialnumber.h"
#include "des.h"
#include "gperftools/profiler.h"
#include "md5.h"

extern uint32_t g_frame;
const char* CYPHER_KEY = "hahaha";

#include "lua_debugger.h"

#define uchar(c)        ((unsigned char)(c))
#define LLOG_INTFRMLEN       "ll"
#define LLOG_INTFRM_T        long long

/* valid flags in a format specification */
#define LLOG_FLAGS   "-+ #0"
/*
** maximum size of each format specification (such as '%-099.99d')
** (+10 accounts for %99.99x plus margin of error)
*/
#define LLOG_MAX_FORMAT  (sizeof(LLOG_FLAGS) + sizeof(LLOG_INTFRMLEN) + 10)


static void addintlen (char *form) {
  size_t l = strlen(form);
  char spec = form[l - 1];
  ff_strcpy(form + l - 1, 3, LLOG_INTFRMLEN);
  form[l + sizeof(LLOG_INTFRMLEN) - 2] = spec;
  form[l + sizeof(LLOG_INTFRMLEN) - 1] = '\0';
} 

static const char *scanformat (lua_State *L, const char *strfrmt, char *form) {
    const char *p = strfrmt;  
    while (*p != '\0' && strchr(LLOG_FLAGS, *p) != NULL) p++;  /* skip flags */
    if ((size_t)(p - strfrmt) >= sizeof(LLOG_FLAGS))
        luaL_error(L, "invalid format (repeated flags)");
    if (isdigit(uchar(*p))) p++;  /* skip width */
    if (isdigit(uchar(*p))) p++;  /* (2 digits at most) */
    if (*p == '.') {
        p++;
        if (isdigit(uchar(*p))) p++;  /* skip precision */
        if (isdigit(uchar(*p))) p++;  /* (2 digits at most) */
    }       
    if (isdigit(uchar(*p)))
        luaL_error(L, "invalid format (width or precision too long)");
    *(form++) = '%'; 
    ff_strncpy(form, p - strfrmt + 2, strfrmt, p - strfrmt + 1);
    form += p - strfrmt + 1;
    *form = '\0';
    return p;
}

bool check_str_len( lua_State* _L )
{
    size_t len = 0;
    luaL_checklstring(_L, 1, &len);
    if( len < MAX_BUFF_LEN )
    {
        return true;
    }
    return false;
}

int parse_str( lua_State* _L, char * _buff, size_t _size )
{
    int32_t arg = 1; 
    size_t sfl;
    const char *strfrmt = luaL_checklstring(_L, arg, &sfl);
    const char *strfrmt_end = strfrmt+sfl;
    size_t nbuff = 0; 
    while (strfrmt < strfrmt_end) {
        if (*strfrmt != '%'){
            if ( nbuff < _size - 1 ) {
                _buff[ nbuff++ ] = *strfrmt++;
            }
            else {
                _buff[ _size-1 ] = '\0';
                return 0;
            }
        }
        else if (*++strfrmt == '%'){
            if ( nbuff < _size - 1 ) {
                _buff[ nbuff++ ] = *strfrmt++;
            }
            else {
                _buff[ _size-1 ] = '\0';
                return 0;
            }
        }
        else { /* format item */
            char form[LLOG_MAX_FORMAT];  /* to store the format (`%...') */
            int32_t n = 0;
            arg++; 
            strfrmt = scanformat(_L, strfrmt, form);
            switch (*strfrmt++) {
                case 'c': {
                              n = ff_snprintf(_buff+nbuff, _size-nbuff, form, (int)luaL_checknumber(_L, arg));
                              break;
                          }
                case 'd':  case 'i': {
                                         addintlen(form);
                                         n = ff_snprintf(_buff+nbuff, _size-nbuff, form, (LLOG_INTFRM_T)luaL_checknumber(_L, arg));
                                         break;
                                     }
                case 'o':  case 'u':  case 'x':  case 'X': {
                                                               addintlen(form);
                                                               n = ff_snprintf(_buff+nbuff, _size-nbuff, form, (unsigned LLOG_INTFRM_T)luaL_checknumber(_L, arg));
                                                               break;
                                                           } 
                case 'e':  case 'E': case 'f':
                case 'g': case 'G': {
                                        n = ff_snprintf(_buff+nbuff, _size-nbuff, form, (double)luaL_checknumber(_L, arg));
                                        break;
                                    }
                case 's': {
                              size_t l;
                              const char *s = luaL_checklstring(_L, arg, &l);
                              n = ff_snprintf(_buff+nbuff, _size-nbuff, form, s);
                              break;
                          }
                case 'p': {
                            void *data = lua_touserdata( _L, arg );
                            int32_t len = ff_snprintf( _buff + nbuff, _size - nbuff, "userdata:" );
                            n = len + ff_snprintf( _buff + nbuff + len, _size - nbuff - len, form, data );
                            break;
                          }
                default: {  /* also treat cases `pnLlh' */
                             ff_snprintf(_buff, _size, "PARSE STRING ERROR" );
                             return 0;
                         }
            }
            if ( n == -1 ){
                ff_snprintf(_buff, _size, "PARSE STRING ERROR" );
                return 0;
            }
            nbuff += n;
            if( nbuff >= _size )
            {
                _buff[ _size-1 ] = '\0';
                return 0;
            }
        }
    }
    _buff[ nbuff ] = '\0';
    return 0;
}

#define DEFINE_LOG_PROC(proc_name,type_name)              \
int proc_name( lua_State* _L )                          \
{                                                       \
	assert( _L );                                       \
    lua_Debug ldb;                                      \
    lua_getstack( _L, 1, &ldb);                         \
    lua_getinfo(_L, "Sl", &ldb);                        \
    if( check_str_len( _L ) )                           \
    {                                                   \
        char buff[ MAX_BUFF_LEN ];                      \
        parse_str( _L, buff, MAX_BUFF_LEN );            \
        type_name("[%s:%d][%d]%s", ldb.source, ldb.currentline, g_frame, buff);     \
    }                                                   \
    else                                                \
    {                                                   \
        ERR(2)("[%s:%d] log too long !", ldb.source, ldb.currentline);                       \
    }                                                   \
    return 0;                                           \
} 

DEFINE_LOG_PROC( c_log, L_LOG(2) )
DEFINE_LOG_PROC( c_trace, L_TRACE(2) )
DEFINE_LOG_PROC( c_err, L_ERR(2) )
DEFINE_LOG_PROC( c_prof, L_PROF(2) )
DEFINE_LOG_PROC( c_item_c, L_ITEM_C(2) )
DEFINE_LOG_PROC( c_item_t, L_ITEM_T(2) )
DEFINE_LOG_PROC( c_item_u, L_ITEM_U(2) )
DEFINE_LOG_PROC( c_item_b, L_ITEM_B(2) )
DEFINE_LOG_PROC( c_safe, L_SAFE(2) )
DEFINE_LOG_PROC( c_sells, L_SELLS(2) )
DEFINE_LOG_PROC( c_gm, L_GM(2) )
DEFINE_LOG_PROC( c_login, L_LOGIN(2) )
DEFINE_LOG_PROC( c_trigger, L_TRIGGER(2) )
DEFINE_LOG_PROC( c_instance, L_INSTANCE(2) )
DEFINE_LOG_PROC( c_money, L_MONEY(2) )
DEFINE_LOG_PROC( c_fight, L_FIGHT(2) )
DEFINE_LOG_PROC( c_fightd, L_FIGHT(4) )
//DEFINE_LOG_PROC( c_syslog, L_SYSLOG(2) )

int32_t c_set_log_level( lua_State* _L ) 
{
    lcheck_argc( _L, 2 ); 
    int log_type = lua_tointeger( _L, 1 ); 
    int log_level = lua_tointeger( _L, 2 ); 
    g_log->set_level( log_type, log_level );
    return 0;
}

int32_t c_bt( lua_State* _L )
{	
 	lua_Debug ldb;
	ERR(2)("[LUAWRAPPER](lua_stack) begin .......... ");
    for(int32_t i = 0; lua_getstack( _L, i, &ldb)==1; i++)
    {
        lua_getinfo(_L, "Slnu", &ldb);
        const char * name = ldb.name;
        if (!name)
            name = "";
        const char * filename = ldb.source;

		ERR(2)("[LUAWRAPPER](bt) #%d: %s:'%s', '%s' line %d", i, ldb.what, name, filename, ldb.currentline );
	}
	ERR(2)("[LUAWRAPPER](lua_stack) end .......... ");
	return 0;
}

int32_t c_traceback( lua_State* _L )
{
    bool notTrace = false;
    int32_t argc = lua_gettop( _L );

    if( argc > 0 && !lua_isnone( _L, 1 ) )
    {
        notTrace = !!lua_toboolean( _L, 1 );
    }

	lua_getfield( _L, LUA_GLOBALSINDEX, "debug" );
	if( !lua_istable(_L, -1) )
	{
		lua_pop(_L, 1);
		return 1;
	}
	lua_getfield( _L, -1, "traceback" );
	if( !lua_isfunction( _L, -1 ) )
	{
		lua_pop( _L, 2 );
		return 1;
	}
	lua_call( _L, 0, 1 );  /* call debug.traceback */

    if( notTrace )
    {
	    ERR(2)( "[LUAWRAPPER](llog) lua_traceback: %s", lua_tostring(_L, -1) );
    }
    else
    {
#ifndef __EDITOR
	    TRACE(2)( "[LUAWRAPPER](llog) lua_traceback: %s", lua_tostring(_L, -1) );
#endif
    }
	return 1;
}

int32_t c_cpu_tick( lua_State* _L )
{
	lcheck_argc( _L, 0 );	
	TICK( cpu_tick );
	lua_pushnumber( _L, cpu_tick );
	return 1;
}

int32_t c_cpu_ms( lua_State* _L )
{
	lcheck_argc( _L, 0 );	
	TICK( cpu_tick );
	lua_pushnumber( _L, get_msec( cpu_tick ) );
	return 1;
}

int c_str_table_key( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    luaL_checktype( _L, 1, LUA_TTABLE );
    luaL_Buffer b;
    luaL_buffinit( _L, &b );
    int tp = lua_type( _L, 2 );
    if( tp == LUA_TNUMBER )
    {
        char buff[512];
        sprintf( buff, "[%.14g]", lua_tonumber( _L, 2 ) );
        luaL_addlstring(&b, buff, strlen(buff));  
    }
    else if( tp == LUA_TSTRING )
    {
        size_t l;
        const char *s = luaL_checklstring( _L, 2, &l );
        luaL_addlstring(&b, "[\"", 2);
        for( size_t i=0; i<l; ++i )
        {
            switch(s[i])
            {
                case 92:
                    luaL_addchar(&b, 92);
                    luaL_addchar(&b, 92);
                    break;
                case 34:
                    luaL_addchar(&b, 92);
                    luaL_addchar(&b, 34);
                    break;
                case 13:
                    luaL_addchar(&b, 92);
                    luaL_addchar(&b, 114);
                    break;
                case 10:
                    luaL_addchar(&b, 92);
                    luaL_addchar(&b, 110);
                    break;
                default:
                    luaL_addchar(&b, s[i]);
                    break;
            }
        }
        luaL_addlstring(&b, "\"]", 2);
    }
    luaL_pushresult( &b );
    lua_pushvalue( _L, -1 );
    lua_insert( _L, 1 );
    lua_settable( _L, 2 );
    lua_pop( _L, 1 );
    return 1;
}

int c_str_table_value( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    luaL_checktype( _L, 1, LUA_TTABLE );
    luaL_Buffer b;
    luaL_buffinit( _L, &b );
    int tp = lua_type( _L, 2 );
    if( tp == LUA_TNUMBER )
    {
        char buff[512];
        sprintf( buff, "%.14g", lua_tonumber( _L, 2 ) );
        luaL_addlstring(&b, buff, strlen(buff));  
    }
    else if( tp == LUA_TSTRING )
    {
        size_t l;
        const char *s = luaL_checklstring( _L, 2, &l );
        luaL_addchar(&b, '"');
        for( size_t i=0; i<l; ++i )
        {
            switch(s[i])
            {
                case 92:
                    luaL_addchar(&b, 92);
                    luaL_addchar(&b, 92);
                    break;
                case 34:
                    luaL_addchar(&b, 92);
                    luaL_addchar(&b, 34);
                    break;
                case 13:
                    luaL_addchar(&b, 92);
                    luaL_addchar(&b, 114);
                    break;
                case 10:
                    luaL_addchar(&b, 92);
                    luaL_addchar(&b, 110);
                    break;
                default:
                    luaL_addchar(&b, s[i]);
                    break;
            }
        }
        luaL_addchar(&b, '"');
    }
    else if( tp == LUA_TBOOLEAN )
    {
        if( lua_toboolean( _L, 2 ) )
        {
            luaL_addlstring(&b, "true", 4);  
        }
        else
        {
            luaL_addlstring(&b, "false", 5);  
        }
    }
    luaL_pushresult( &b );
    lua_pushvalue( _L, -1 );
    lua_insert( _L, 1 );
    lua_settable( _L, 2 );
    lua_pop( _L, 1 );
    return 1;
}

int c_post_disconnect_msg( lua_State* _L ) 
{
    lcheck_argc( _L, 2 ); 
    FF_Network::NetMng* mng = *((FF_Network::NetMng**)lua_topointer( _L, 1 ));
    DPID dpid = (DPID)lua_tonumber( _L, 2 ); 
    mng->post_disconnect_msg( dpid );
    return 0;
}

int c_and( lua_State* _L ) 
{
    lcheck_argc( _L, 2 ); 
    uint64_t op1 = (uint64_t)lua_tonumber( _L, 1 ); 
    uint64_t op2 = (uint64_t)lua_tonumber( _L, 2 ); 
    uint64_t ret = op1 & op2; 
    lua_pushnumber( _L, ret );
    return 1;
}

int c_or( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    uint64_t op1 = (uint64_t)lua_tonumber( _L, 1 ); 
    uint64_t op2 = (uint64_t)lua_tonumber( _L, 2 ); 
    uint64_t ret = op1 | op2;
    lua_pushnumber( _L, ret );
    return 1;
}

int c_xor( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    uint64_t op1 = (uint64_t)lua_tonumber( _L, 1 ); 
    uint64_t op2 = (uint64_t)lua_tonumber( _L, 2 ); 
    uint64_t ret = op1 ^ op2;
    lua_pushnumber( _L, ret );
    return 1;
}

int c_not( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    uint64_t op1 = (uint64_t)lua_tonumber( _L, 1 ); 
    uint64_t ret = ~op1;
    lua_pushnumber( _L, ret );
    return 1;
}

int c_shl( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    uint64_t op1 = (uint64_t)lua_tonumber( _L, 1 ); 
    uint64_t op2 = (uint64_t)lua_tonumber( _L, 2 ); 
    uint64_t ret = op1 << op2;
    lua_pushnumber( _L, ret );
    return 1;
}

int c_shr( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    uint64_t op1 = (uint64_t)lua_tonumber( _L, 1 ); 
    uint64_t op2 = (uint64_t)lua_tonumber( _L, 2 ); 
    uint64_t ret = op1 >> op2;
    lua_pushnumber( _L, ret );
    return 1;
}

int c_get_serial_num( lua_State* _L )
{
    lcheck_argc( _L, 0 ); 
    lua_pushnumber( _L, g_serial_num->get() );
    return 1;
}

int c_des_encrypt( lua_State* _L )
{
    lcheck_argc( _L, 1 ); 
    const char *plain_text = lua_tostring( _L, 1 ); 
    char ret[128];
    memset( ret, 0, sizeof(ret));
    DES_Encrypt2Char(plain_text, CYPHER_KEY, ret);
    lua_pushstring( _L, ret );
    return 1; 
}

int c_des_decrypt( lua_State* _L )
{
    lcheck_argc( _L, 1 ); 
    const char *cipher_text = lua_tostring( _L, 1 ); 
    char ret[128];
    memset( ret, 0, sizeof(ret));
    DES_DecryptFromChar(cipher_text, strlen(cipher_text), CYPHER_KEY, ret);
    lua_pushstring( _L, ret );
    return 1; 
}

int c_assert( lua_State* _L )
{
    lcheck_argc( _L, 2 ); 
    bool result = lua_toboolean( _L, 1 );
    const char* err_str = lua_tostring( _L, 2 );

    if ( ! result )
    {
        ERR(2)("assert failed:%s", err_str);
        assert(false);
    }
    return 0;
}

/**
 * return random input chars(0-9, a-z)
 */
int c_rand_str( lua_State* _L )
{
    lcheck_argc( _L, 1 ); 
    u_long char_num = (u_long)lua_tonumber( _L, 1 ); 

    if (char_num > 31) char_num = 31; 

    char rand_str[32] = {0};
    if (char_num > 0)
    {
        for( u_long i = 0; i < char_num; i++ )
        {
            char c = g_serial_num->get() % 36; 
            rand_str[i] = (c < 10) ? ( '0' + c ) : ( 'a' + c - 10 );
        }
        rand_str[char_num] = '\0';
    }

    lua_pushstring( _L, rand_str );
    return 1;
}

int c_get_file_modify_time( lua_State* _L ) 
{
    lcheck_argc( _L, 1 ); 
    const char *filename = lua_tostring( _L, 1 ); 

    struct stat file_stat;
    if (0 == stat(filename, &file_stat))
    {
        lua_pushnumber( _L, file_stat.st_mtime );
        return 1;
    }
    return 0; 
}

int c_prof_start( lua_State* _L )
{
    lcheck_argc( _L, 1 ); 
    const char* file_name = lua_tostring( _L, 1 );
    ProfilerStart( file_name );
    TRACE(2)( "[PROF]prof c start. file name: %s", file_name );
    return 0;
}

int c_prof_stop( lua_State* _L )
{
    lcheck_argc( _L, 0 ); 
    ProfilerStop();
    TRACE(2)( "[PROF]prof c stop." );
    return 0;
}

int c_md5( lua_State* _L )
{
    lcheck_argc( _L, 1 ); 
    int len = 0;
    const char* str = lua_tolstring( _L, 1, (size_t*)&len );
    unsigned char md5[16];
    char md5str[33] = {0};
    MD5( ( unsigned char* )str, len, md5 );
    snprintf( md5str, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7], md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15] );
    lua_pushstring( _L, md5str );
    return 1;
}

int c_get_memory( lua_State* _L )
{
    int memory = get_mm();
    lua_pushinteger( _L, memory );
    return 1;
}

int c_syslog( lua_State* _L )                          
{                                                       
	assert( _L );                                       
    lua_Debug ldb;                                      
    lua_getstack( _L, 1, &ldb);                         
    lua_getinfo(_L, "Sl", &ldb);                        
    if( check_str_len( _L ) )                           
    {                                                   
        char buff[ MAX_BUFF_LEN ];                      
        parse_str( _L, buff, MAX_BUFF_LEN );            
        L_SYSLOG(2)("%s", buff);                       
    }                                                   
    else                                                
    {                                                   
        ERR(2)("[%s:%d] log too long !", ldb.source, ldb.currentline);                       
    }                                                   
    return 0;                                           
} 

int c_sdklog( lua_State* _L )                          
{                                                       
	assert( _L );                                       
    lua_Debug ldb;                                      
    lua_getstack( _L, 1, &ldb);                         
    lua_getinfo(_L, "Sl", &ldb);                        
    if( check_str_len( _L ) )                           
    {                                                   
        char buff[ MAX_BUFF_LEN ];                      
        parse_str( _L, buff, MAX_BUFF_LEN );            
        L_SDKLOG(2)("%s", buff);                       
    }                                                   
    else                                                
    {                                                   
        ERR(2)("[%s:%d] log too long !", ldb.source, ldb.currentline);                       \
    }                                                   
    return 0;                                           
}

int32_t c_tgalog( lua_State* _L )
{
	assert( _L );                                       
    lua_Debug ldb;                                      
    lua_getstack( _L, 1, &ldb);                         
    lua_getinfo(_L, "Sl", &ldb);                        
    if( check_str_len( _L ) )                           
    {                                                   
        char buff[ MAX_BUFF_LEN ];                      
        parse_str( _L, buff, MAX_BUFF_LEN );            
        L_TGALOG(2)("%s", buff);                       
    }                                                   
    else                                                
    {                                                   
        ERR(2)("[%s:%d] log too long !", ldb.source, ldb.currentline);                       \
    }                                                   
    return 0;  	
}

int c_backup( lua_State* _L )                          
{                                                       
	assert( _L );                                       
    lua_Debug ldb;                                      
    lua_getstack( _L, 1, &ldb);                         
    lua_getinfo(_L, "Sl", &ldb);                        
    if( check_str_len( _L ) )                           
    {                                                   
        char buff[ MAX_BUFF_LEN ];                      
        parse_str( _L, buff, MAX_BUFF_LEN );            
        L_BACKUP(2)("%s", buff);                       
    }                                                   
    else                                                
    {                                                   
        ERR(2)("[%s:%d] log too long !", ldb.source, ldb.currentline);                       \
    }                                                   
    return 0;                                           
}

int c_get_miniserver_time_span( lua_State* _L )
{
    lcheck_argc( _L, 0 ); 

    struct timezone tz;
	struct timeval tv;
	gettimeofday(&tv,&tz);
  
    unsigned int span = tv.tv_sec;
    lua_pushinteger( _L, span );

    return 1;
}

int c_get_unity_time_span( lua_State* _L )
{
    lcheck_argc( _L, 0 ); 

    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
  
    unsigned int span = tv.tv_sec;
    lua_pushinteger( _L, span );

    return 1;
}

int c_online( lua_State* _L )                          
{                                                       
	assert( _L );                                       
    lua_Debug ldb;                                      
    lua_getstack( _L, 1, &ldb);                         
    lua_getinfo(_L, "Sl", &ldb);                        
    if( check_str_len( _L ) )                           
    {                                                   
        char buff[ MAX_BUFF_LEN ];                      
        parse_str( _L, buff, MAX_BUFF_LEN );            
        L_ONLINE(2)("%s", buff);                       
    }                                                   
    else                                                
    {                                                   
        ERR(2)("[%s:%d] log too long !", ldb.source, ldb.currentline);                       
    }                                                   
    return 0;                                           
} 

int c_reset_log_file_handle(lua_State* _L) 
{
	lcheck_argc(_L, 0);

	g_log->reset_log_file_handle(); 
	return 0;
}

int c_migrate_log_file(lua_State* _L) 
{
        lcheck_argc(_L, 0);
        
        g_log->migrate_log_file(); 
        return 0;
}

