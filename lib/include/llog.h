#ifndef _LLOG_H_
#define _LLOG_H_

#include <stdint.h>
#include "lunar.h"

int32_t c_log( lua_State* _L );
int32_t c_trace( lua_State* _L );
int32_t c_err( lua_State* _L );
int32_t c_prof( lua_State* _L );
int32_t c_sells( lua_State* _L );
int32_t c_gm( lua_State* _L );
int32_t c_login( lua_State* _L );

int32_t c_bt( lua_State* _L );
int32_t c_traceback( lua_State* _L );
int32_t c_cpu_tick( lua_State* _L );
int32_t c_cpu_ms( lua_State* _L );

int32_t c_item_c( lua_State* _L );
int32_t c_item_t( lua_State* _L );
int32_t c_item_u( lua_State* _L );
int32_t c_item_b( lua_State* _L );

int32_t c_safe( lua_State* _L );
int32_t c_trigger( lua_State* _L );

int32_t c_instance( lua_State* _L );
int32_t c_money( lua_State* _L );
int32_t c_fight( lua_State* _L );
int32_t c_fightd( lua_State* _L );

int32_t c_syslog( lua_State* _L );
int32_t c_sdklog( lua_State* _L );
int32_t c_backup( lua_State* _L );
int32_t c_online( lua_State* _L );
int32_t c_tgalog( lua_State* _L );

int32_t c_set_log_level( lua_State* _L );

int c_and( lua_State* _L );
int c_or( lua_State* _L );
int c_xor( lua_State* _L );
int c_not( lua_State* _L );
int c_shl( lua_State* _L );
int c_shr( lua_State* _L );

int c_str_table_key( lua_State* _L );
int c_str_table_value( lua_State* _L );

int c_post_disconnect_msg( lua_State* _L ); 

int c_get_serial_num( lua_State* _L );
int c_des_encrypt( lua_State* _L );
int c_des_decrypt( lua_State* _L );
int c_rand_str( lua_State* _L );
int c_get_file_modify_time( lua_State* _L );
int c_assert( lua_State* _L );

int c_prof_start( lua_State* _L );
int c_prof_stop( lua_State* _L );

int c_md5( lua_State* _L );

int c_get_memory( lua_State* _L );

int c_get_miniserver_time_span( lua_State* _L );
int c_get_unity_time_span( lua_State* _L );

int c_reset_log_file_handle(lua_State* _L);
int c_migrate_log_file(lua_State* _L);
#endif // _LLOG_H_



