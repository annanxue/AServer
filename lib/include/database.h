#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <stdlib.h>

#include "mysql.h"
#include "errmsg.h"
#include "mystring.h"
#include "ffsys.h"
#include "log.h"
#include "lunar.h"
#include "lmisc.h"
#include "mymap32.h"

const uint32_t MAX_ESCAPE_STR = 2*1024*1024;
const uint32_t BLOB_BIND_BUFFER_LEN = 512*1024;
const uint32_t STRING_BIND_BUFFER_LEN = 64*1024;

class DataBase
{
public:
	DataBase() 
        : mysql_stmt_(NULL), mysql_bind_param_(NULL), mysql_bind_result_(NULL), 
        trans_(false), pres_(NULL), pfields_(NULL), nfields_(0), lengths_(NULL), escape_str_stmt_index_(0)
	{
		escape_str_ = (char*)malloc(sizeof(char) * MAX_ESCAPE_STR );
		if( !escape_str_ )
            ERR(2)( "[DB](database) %s:%d mem create error!", __FILE__, __LINE__ );
		hex_str_ = (char*)malloc(sizeof(char) * MAX_ESCAPE_STR );
		if( !hex_str_ )
			ERR(2)( "[DB](database) %s:%d mem create error!", __FILE__, __LINE__ );
        if( !mysql_init(&mysql_) )
        {
            ERR(2)( "[DB](database) mysql_init() failed" );
            //return -1;
        }
        stmt_map_.init( 128, 128, "stmt_map_:database.cpp" );
	}

	~DataBase() 
	{
        stmt_close();
        clear_stmt_map();
		if(trans_) {
			rollback();
		}
		mysql_close(&mysql_);
		//SAFE_DELETE( escape_str_ );
		//SAFE_DELETE( hex_str_ );
        free( escape_str_ );
        free( hex_str_ );
	}

	/*! 所有函数只有在发生错误的情况下才返回-1*/
	/*! 连接数据库*/
	int32_t connect(const char * _host, const char * _user, const char * _passwd, const char * _db_name, uint16_t _port);
	int32_t reconnect();
    int32_t handle_error( const char * _msg, const char * _sql );
    int32_t handle_stmt_error( const char * _msg, const char * _sql );

	/*! 取得上次插入记录的id*/
	int32_t get_insert_id() { return mysql_insert_id(&mysql_); }

	/*! 取得错误号*/
	int32_t  get_errno() { return mysql_errno(&mysql_); }

	/*! 取得错误内容*/
	const char* get_error() { return mysql_error(&mysql_); }

	/*! 转义mysql字符串*/
	MyString mysql_escape(const char * _sql, uint32_t _len) ;

	/*! 返回实际修改的记录数,
		处理INSERT, UPDATE, DELETE等修改操作*/
	int32_t modify(const char * _sql);
	
	/*! 因为4.0的版本没有专门的事务函数,所以现在是使用的mysql_query来实现的*/
	int32_t begin();
	int32_t commit();
	int32_t rollback();

    //多用于SELECT
	int32_t query( const char * _sql); 
    int32_t row_num();

	/*! 返回－1表示出错，1表示没有记录了，0表示正常 */
	int32_t fetch();
	void clear();

    //statement is only supported in lua
    int32_t stmt_prepare( const char * _sql );
    int32_t stmt_get_insert_id();
    int32_t stmt_close( );
    void clear_stmt_map();
    void add_escape_str_stmt_index(unsigned _num);
    int stmt_set_param_bind( 
        MYSQL_BIND& _bind, 
        enum_field_types _buffer_type, 
        const char* _data, 
        int _data_len, 
        int _is_unsigned);
    int stmt_set_result_bind(
        MYSQL_BIND& _bind, 
        enum_field_types _buffer_type, 
        int _buffer_len);
    int32_t stmt_fetch();

	int32_t get_int32( const char* _name, int32_t *_val );
	int32_t get_int64( const char* _name, int64_t *_val );
	int32_t get_uint64( const char* _name, uint64_t *_val );
	int32_t get_float( const char* _name, float* _val );
	int32_t get_double( const char* _name, double* _val );
	int32_t get_char( const char* _name, char* _val );
	int32_t get_str( const char* _name, char* _buf, uint32_t _len );
	int32_t get_blob( const char* _name, char* _buf, uint32_t& _len );

	int32_t get_int32( uint32_t _ncol, int32_t *_val );
	int32_t get_int64( uint32_t _ncol, int64_t *_val );
	int32_t get_uint64( uint32_t _ncol, uint64_t *_val );
	int32_t get_float( uint32_t _ncol, float* _val );
	int32_t get_double( uint32_t _ncol, double* _val );
	int32_t get_char( uint32_t _ncol, char* _val );
	int32_t get_str( uint32_t _ncol, char* _buf, uint32_t _len );	
	int32_t get_blob( uint32_t _ncol, char* _buf, uint32_t& _len );

    // notice: only dave use
    int get_field_count();
    MYSQL_FIELD* get_fields_name();
    MYSQL_ROW get_row_value();

public:
    int32_t c_delete( lua_State* _L ) { if(this) delete this; return 0; }

    int32_t c_is_db_exist( lua_State* _L );
    int32_t c_query_table_list( lua_State* _L );
    int32_t c_select_db( lua_State* _L );
    int32_t c_connect( lua_State* _L );
	int32_t c_get_insert_id( lua_State* _L );
	int32_t c_get_errno( lua_State* _L );
	int32_t c_get_error( lua_State* _L ); 
	int32_t c_mysql_escape( lua_State* _L ) ;
    int32_t c_modify( lua_State* _L );
	int32_t c_query( lua_State* _L ); 
    int32_t c_row_num( lua_State* _L );
	int32_t c_fetch( lua_State* _L );

	int32_t c_get_int32( lua_State* _L );
	int32_t c_get_int64( lua_State* _L );
	int32_t c_get_uint64( lua_State* _L );
	int32_t c_get_float( lua_State* _L );
	int32_t c_get_double( lua_State* _L );
	int32_t c_get_char( lua_State* _L );
	int32_t c_get_str( lua_State* _L );

	int32_t c_get_format( lua_State* _L );

    //statement is only supported in lua
    int32_t c_stmt_format_modify( lua_State* _L );
    int32_t c_stmt_get_insert_id( lua_State* _L );
    int32_t c_stmt_format_query( lua_State* _L );
    int32_t c_stmt_fetch( lua_State* _L );
    int32_t c_stmt_get( lua_State* _L );
    int32_t c_stmt_row_num( lua_State* _L );

    int32_t c_autocommit( lua_State* _L );
    int32_t c_begin( lua_State* _L );
    int32_t c_rollback( lua_State* _L );
    int32_t c_commit( lua_State* _L );

    static const char className[];
    static Lunar<DataBase>::RegType methods[];
    static const bool gc_delete_;

private:
	DataBase operator = ( const DataBase& );

	int32_t find_col( const char* _name );

private:
	MYSQL				mysql_;
    MYSQL_STMT          *mysql_stmt_;
    MYSQL_BIND          *mysql_bind_param_;
    MYSQL_BIND          *mysql_bind_result_;
	bool				trans_;

	MYSQL_RES*			pres_;
	MYSQL_FIELD*		pfields_;
	uint32_t            nfields_;
    unsigned long *     lengths_;
	MYSQL_ROW			row_;
	char*			    escape_str_;
	unsigned	        escape_str_stmt_index_;
	char*				hex_str_;

    MyMapStr            stmt_map_;
};

#define LUNAR_DATABASE_METHODS                  \
    LUNAR_DECLARE_METHOD( DataBase, c_connect ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_is_db_exist ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_query_table_list ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_select_db ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_insert_id ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_errno ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_error ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_mysql_escape ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_modify ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_query ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_row_num ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_fetch ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_int32 ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_int64 ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_uint64 ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_float ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_double ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_char ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_str ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_get_format ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_stmt_format_modify ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_stmt_get_insert_id ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_stmt_format_query ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_stmt_fetch ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_stmt_get ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_stmt_row_num ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_autocommit ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_begin ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_rollback ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_commit ),   \
    LUNAR_DECLARE_METHOD( DataBase, c_delete )           

#endif //__DATABASE_H__

