#ifndef _LUAAR_H_
#define _LUAAR_H_

#include "ar.h"
#include "lunar.h"

/*
* wrapper of Ar,　to use it in lua
*/
class LAr
{

public:
	Ar* ar_;
	
	//constructor
	LAr();
	LAr( Ar* _ar );
	LAr( const void* _buff, int _len );
	LAr( lua_State* _L );
	
	//destructor
	~LAr();
	
	//add a message header into ar
	//send the message from network
	int before_send( lua_State* _L );
	int send( lua_State* _L );
	
	int flush_before_send( lua_State* _L );
	int flush_before_send_one_byte( lua_State* _L );
    int flush_send_one_packet( lua_State* _L );
    int send_one_ar( lua_State* _L );

	//write data to ar
	int write_byte( lua_State* L ); //无符号数
	int write_ubyte( lua_State* L );//无符号数
    int write_char( lua_State* L ); //有符号数
	int write_short( lua_State* L );
	int write_ushort( lua_State* L );
	int write_int( lua_State* L );
	int write_uint( lua_State* L );
	int write_long( lua_State* L );
	int write_ulong( lua_State* L );
	int write_float( lua_State* L );
	int write_double( lua_State* L );
	int write_string( lua_State* L );
	int write_boolean( lua_State* L );
	int write_buffer( lua_State* L );
    int write_format( lua_State* L );
    int write_var_array_short( lua_State* L );      //255大小以内的数组
    int write_int_degree( lua_State* L );     
    int write_compress_pos_angle( lua_State* L );     
    int write_lar( lua_State* L );     
	
	//read data from ar
	int read_byte( lua_State* _L );	//无符号数
	int read_ubyte( lua_State* _L ); //无符号数 
    int read_char( lua_State* _L );  //有符号数
	int read_short( lua_State* _L );
	int read_ushort( lua_State* _L );
	int read_int( lua_State* _L );
	int read_uint( lua_State* _L );
	int read_long( lua_State* _L );
	int read_ulong( lua_State* _L );
	int read_float( lua_State* _L );
	int read_double( lua_State* _L );
	int read_string( lua_State* _L );	//NOTICE: not thread safe, use static storage
	int read_boolean( lua_State* _L );
	int read_buffer( lua_State* L );
    int read_format( lua_State* _L );
    int read_var_array_short( lua_State* L );       //255大小以内的数组
    int read_int_degree( lua_State* L );      
    int read_compress_pos_angle( lua_State* L );      
	int read_lar( lua_State* _L );      //NOTICE: reuse the buffer in parent lar, so the lifecycle is the same; nil may be returned 

	int is_loading( lua_State* _L );
	int is_storing( lua_State* _L );

	int get_offset( lua_State* _L );
	int write_int_at( lua_State* _L );
    int write_byte_at( lua_State* _L );

    int reuse( lua_State* _L );
    int get_buffer( lua_State* _L );

	int c_delete( lua_State* _L );

	int bpack( lua_State* _L );
	int upack( lua_State* _L );

    int flush( lua_State* _L );

	int check_ar( lua_State* _L );

	//get the pointer of instance in CPP
	int get_this( lua_State* _L );
	int init( const char* _buff = 0, int _len = 0 );
	int reuse( const char* _buff = 0, int _len = 0 );
	void destroy();

	int print( lua_State* _L );

	static const char className[];
	static Lunar<LAr>::RegType methods[];
	static const bool gc_delete_;

    char    flush_flag_;
private:
	bool delete_ar_;
};

#define LUNAR_LAR_METHODS														\
	method(LAr, before_send),													\
	method(LAr, flush_before_send),											    \
	method(LAr, flush_before_send_one_byte),								    \
	method(LAr, flush_send_one_packet),											\
	method(LAr, send),															\
	method(LAr, send_one_ar ),													\
	method(LAr, write_byte),													\
	method(LAr, write_ubyte),													\
    method(LAr, write_char),                                                    \
	method(LAr, write_short),													\
	method(LAr, write_ushort),													\
	method(LAr, write_int),														\
	method(LAr, write_uint),													\
	method(LAr, write_long),													\
	method(LAr, write_ulong),													\
	method(LAr, write_float),													\
	method(LAr, write_double),													\
	method(LAr, write_string),													\
	method(LAr, write_boolean),													\
	method(LAr, write_format),													\
	method(LAr, write_var_array_short),										    \
	method(LAr, write_int_degree),										        \
	method(LAr, write_compress_pos_angle),							            \
																				\
	method(LAr, read_byte),														\
	method(LAr, read_ubyte),													\
    method(LAr, read_char),                                                     \
	method(LAr, read_short),													\
	method(LAr, read_ushort),													\
	method(LAr, read_int),														\
	method(LAr, read_uint),														\
	method(LAr, read_long),														\
	method(LAr, read_ulong),													\
	method(LAr, read_float),													\
	method(LAr, read_double),													\
	method(LAr, read_string),													\
	method(LAr, read_boolean),													\
	method(LAr, read_buffer),													\
    method(LAr, read_format),                                                   \
    method(LAr, read_var_array_short),                                          \
    method(LAr, read_int_degree),                                               \
    method(LAr, read_compress_pos_angle),                                       \
																				\
	method(LAr, bpack),															\
	method(LAr, flush),															\
	method(LAr, upack),															\
	method(LAr, get_this),														\
	method(LAr, is_loading),													\
	method(LAr, is_storing),													\
	method(LAr, get_offset),													\
	method(LAr, write_int_at),													\
	method(LAr, write_byte_at),													\
	method(LAr, print),															\
	method(LAr, c_delete),														\
	method(LAr, check_ar),														\
	method(LAr, reuse),														\
	method(LAr, get_buffer),														\
	method(LAr, write_buffer)

#endif
