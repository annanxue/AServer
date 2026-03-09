#include <mysqld_error.h>
#include "database.h"

const uint32_t MAX_DATABASE_STR_LEN = 2048;

int32_t DataBase::connect(const char * _host, const char * _user, const char * _passwd, const char * _db_name, uint16_t _port)
{
	if( mysql_options(&mysql_, MYSQL_SET_CHARSET_NAME, "latin1" ) )
	{
		ERR(2)("[DB](database) can't set the default charset to latin1, %s %s, %s", _host, _user, _db_name );
		return -1;
	}

	if( mysql_options(&mysql_, MYSQL_OPT_RECONNECT, "1" ) )
	{
		ERR(2)("[DB](database) can't set auto reconnect, %s %s, %s", _host, _user, _db_name );
		return -1;
    }
	
	if (!mysql_real_connect(&mysql_, _host, _user, _passwd, _db_name, _port, NULL, 0))
	{
		ERR(2)("[DB](database) can not connect, error: %s", mysql_error(&mysql_));
		return -1;
	}

	if ( mysql_set_character_set(&mysql_, "latin1")) 
	{
	    ERR(2)("[DB](database) New client character set failed, current set: %s\n", mysql_character_set_name(&mysql_));
	}
	
    const char* client_version = mysql_get_client_info();
    const char* server_version = mysql_get_server_info( &mysql_ );
	LOG(2)( "[DB](database) DataBase::connect, mysql client verion: %s, server version: %s", client_version, server_version );
	
	return 0;
}

int32_t DataBase::reconnect()
{
    LOG(2)( "[DataBase](reconnect) try to reconnect to mysql" );
    int ret = 0;
    while ( (ret = mysql_ping( &mysql_ )) != 0 )
    {
        LOG(2)( "[DataBase](reconnect) reconnect to mysql failed: %d", ret );
        ff_sleep( 1000 );
    }
    LOG(2)( "[DataBase](reconnect) reconnect to mysql succ: %d", ret );
    return 0;
}

int32_t DataBase::handle_error( const char * _msg, const char* _sql )
{
    unsigned int err_no = mysql_errno( &mysql_ );
    ERR(2)( "[DataBase](handle_error) %s, errno: %u, sql: %s", _msg, err_no, _sql );
    if ( err_no == CR_SERVER_LOST || err_no == CR_SERVER_GONE_ERROR )
    {
        reconnect();
        return 0;
    }
    return -1;
}

int32_t DataBase::handle_stmt_error( const char * _msg, const char * _sql )
{
    unsigned int err_no = mysql_stmt_errno( mysql_stmt_ );
    ERR(2)( "[DataBase](handle_stmt_error) %s, mysql_stmt_: 0x%x, errno: %u, sql: %s", _msg, mysql_stmt_, err_no, _sql );
    if ( err_no == CR_SERVER_LOST || err_no == CR_SERVER_GONE_ERROR )
    {
        reconnect();
        return 0;
    }
    return -1;
}

MyString DataBase::mysql_escape(const char * _sql_param, uint32_t _len)
{
	/*! 转义以后的最大可能长度是原来长度的两倍,这里多分配一个字节确保结尾*/
	if( _len > MAX_ESCAPE_STR / 2 )
	{
		ERR(2)( "[DB](database) %s:%d sql string exceed length, cutted!", __FILE__, __LINE__ );
		_len = MAX_ESCAPE_STR / 2;
	}
	uint32_t str_len = strlen(_sql_param);
	if( _len > str_len)
		_len = str_len;
	mysql_real_escape_string(&mysql_, escape_str_, _sql_param, _len);
	return escape_str_;
}

int32_t DataBase::modify(const char * _sql)
{
    clear();
	LOG(2)( "MODIFY: %s",_sql );
	if ( mysql_query( &mysql_, _sql ) )
    {
        if ( handle_error( "modify failed", _sql ) )
        {
            return -1;
        }
        else
        {
            return modify( _sql );
        }
	}
	int32_t num = mysql_affected_rows(&mysql_);
	return num;
}

int32_t DataBase::begin()
{
	/*! 假如已经开始了事务,不再BEGIN*/
	if (!trans_) {
		/*! 只有在BEGIN成功的情况下,才设置事务开始*/
		if(!mysql_query(&mysql_, "BEGIN")) 
			trans_ = true;
		else 
			return -1;
    }
	return 0;
}

int32_t DataBase::commit()
{
	if (trans_) {
		if(!mysql_query(&mysql_, "COMMIT"))
			trans_ = false;
		else
			return -1;
    }
	return 0;
}

int32_t DataBase::rollback()
{
	if (trans_) {
		if (!mysql_query(&mysql_, "ROLLBACK"))
			trans_ = false;
		else
			return -1;
    }
	return 0;
}


int32_t DataBase::query( const char * _sql) 
{
	clear();
    LOG(2)("QUERY:%s", _sql);
	if (mysql_query(&mysql_, _sql)) 
	{
        if ( handle_error( "query failed", _sql ) )
        {
            return -1;
        }
        else
        {
            return query( _sql );
        }
	}

	if( (pres_ = mysql_store_result(&mysql_) ) == NULL )
	{
		LOG(2)( "[DB](database) QUERY, store: %s", _sql );
		return -2;
	}

	pfields_ = mysql_fetch_fields( pres_ );
	nfields_ = mysql_num_fields( pres_ );
    lengths_ = mysql_fetch_lengths( pres_ );

	return 0;
}

int32_t DataBase::row_num()
{
	return mysql_num_rows( pres_ );
}

int32_t DataBase::fetch()
{
	row_ = mysql_fetch_row( pres_ );
	if( !row_ )
	{
		if( mysql_errno(&mysql_) )
		{
			ERR(2)( "[DB](database) %s:%d mysql error", __FILE__, __LINE__ );
			return -1;
		}
		else
			return 1;
	}

	return 0;
}

void DataBase::clear()
{
	if( pres_ )
	{
		mysql_free_result( pres_ );
    }

	pres_ = NULL;
	pfields_ = NULL;
	nfields_ =0;
    lengths_ = NULL;
	row_ = NULL;

}

int32_t DataBase::find_col( const char* _name )
{
	if( pfields_ )
	{
		for( uint32_t i = 0; i < nfields_; ++i )
		{
			if( strcmp( _name, pfields_[i].name ) == 0 )
				return i;
		}
	}
	LOG(2)( "[DB](database) find_col, no name = %s", _name );
	return -1;
}

// only dave use
int32_t DataBase::get_field_count()
{
    return nfields_;
}

// only dave use
MYSQL_FIELD* DataBase::get_fields_name()
{
    return pfields_;
}

// only dave use
MYSQL_ROW DataBase::get_row_value()
{
    return row_;
}

int32_t DataBase::get_int32( uint32_t _ncol, int32_t *_val )
{
	if( _ncol >= nfields_ )
		return -1;

	if( row_[ _ncol ] )
	{
		char* pend;
		int32_t a = strtol( row_[_ncol], &pend, 10 );
		if( *pend != '\0' )
			return -1;

		*_val = (int32_t)a;
		return 0;
	}
	return -1;
}

int32_t DataBase::get_int64( uint32_t _ncol, int64_t *_val )
{
	if( _ncol >= nfields_ )
		return -1;

	if( row_[ _ncol ] )
	{
		char* pend;
		int64_t a = strtol( row_[_ncol], &pend, 10 );
		if( *pend != '\0' )
			return -1;

		*_val = (int64_t)a;
		return 0;
	}
	return -1;
}

int32_t DataBase::get_uint64( uint32_t _ncol, uint64_t *_val )
{
	if( _ncol >= nfields_ )
		return -1;

	if( row_[ _ncol ] )
	{
		char* pend;
		uint64_t a = strtoul( row_[_ncol], &pend, 10 );
		if( *pend != '\0' )
			return -1;

		*_val = (uint64_t)a;
		return 0;
	}
	return -1;
}

int32_t DataBase::get_float( uint32_t _ncol, float* _val )
{
	if( _ncol >= nfields_ )
		return -1;

	if( row_[ _ncol ] )
	{
		char* pend;
		float a = strtof( row_[_ncol], &pend );
		if( *pend != '\0' )
			return -1;

		*_val = a;
		return 0;
	}
	return -1;
}

int32_t DataBase::get_double( uint32_t _ncol, double* _val )
{
	if( _ncol >= nfields_ )
		return -1;

	if( row_[ _ncol ] )
	{
		char* pend;
		double a = strtod( row_[_ncol], &pend );
		if( *pend != '\0' )
			return -1;

		*_val = a;
		return 0;
	}
	return -1;
}

int32_t DataBase::get_char( uint32_t _ncol, char* _val )
{
	if( _ncol >= nfields_ )
		return -1;

	if( row_[ _ncol ] )
	{
		*_val = row_[_ncol][0];
		return 0;
	}

	return -1;
}

int32_t DataBase::get_str( uint32_t _ncol, char* _buf, uint32_t _len )
{
    // if( !row_ ) ERR(2)( 0 )( "[DB](get_str) empty row" );

	if( _ncol >= nfields_ )
		return -1;

	if( row_[ _ncol ] )
	{
		uint32_t l = strlen( row_[_ncol] );
		if( l >= _len )
			return -1;

		strlcpy( _buf, row_[_ncol], l+1 );/*! include the teminal NULL  */
		return 0;
	}

	return -1;
}

int32_t DataBase::get_blob( uint32_t _ncol, char* _buf, uint32_t& _len )
{
	if( _ncol >= nfields_ )
		return -1;

	if( row_[ _ncol ] && lengths_ )
	{
        _len = lengths_[ _ncol ];
		memcpy( _buf, row_[_ncol], _len );
		return 0;
	}

	return -1;
}

int32_t DataBase::get_int32( const char* _name, int32_t *_val )
{
	int32_t ncol = find_col( _name );
	if( ncol > -1 )
	{
		if ( get_int32( (uint32_t)ncol, _val ) == 0 )
			return 0;
	}

	ERR(2)("[DB](database) DataBase::get_int, error, name: %s", _name );
	return -1;
}

int32_t DataBase::get_int64( const char* _name, int64_t *_val )
{
	int32_t ncol = find_col( _name );
	if( ncol > -1 )
	{
		if ( get_int64( (uint32_t)ncol, _val ) == 0 )
			return 0;
	}
	ERR(2)("[DB](database) DataBase::get_long, error, name: %s", _name );
	return -1;
}

int32_t DataBase::get_uint64( const char* _name, uint64_t *_val )
{
	int32_t ncol = find_col( _name );
	if( ncol > -1 )
	{
		if ( get_uint64( (int32_t)ncol, _val ) == 0 )
			return 0;
	}
	ERR(2)("[DB](database) DataBase::get_long, error, name: %s", _name );
	return -1;
}


int32_t DataBase::get_float( const char* _name, float* _val )
{
	int32_t ncol = find_col( _name );
	if( ncol > -1 )
	{
		if ( get_float( (int32_t)ncol, _val ) == 0 )
			return 0;
	}
	ERR(2)("[DB](database) DataBase::get_float, error, name: %s", _name );
	return -1;
}

int32_t DataBase::get_double( const char* _name, double* _val )
{
	int32_t ncol = find_col( _name );
	if( ncol > -1 )
	{
		if ( get_double( (int32_t)ncol, _val ) == 0 )
			return 0;
	}
	ERR(2)("[DB](database) DataBase::get_double, error, name: %s", _name );
	return -1;
}

int32_t DataBase::get_char( const char* _name, char* _val )
{
	int32_t ncol = find_col( _name );
	if( ncol > -1 )
	{
		if ( get_char( (uint32_t)ncol, _val ) == 0 )
			return 0;
	}
	ERR(2)("[DB](database) DataBase::get_char, error, name: %s", _name );
	return -1;
}

int32_t DataBase::get_str( const char* _name, char* _buf, uint32_t _len )
{
	int32_t ncol = find_col( _name );
	if( ncol > -1 )
	{
		if ( get_str( (int32_t)ncol, _buf, _len ) == 0 )
			return 0;
	}
	ERR(2)( "[DB](database) get_str, not find col; name = %s ", _name );
	return -1;	
}

int32_t DataBase::get_blob( const char* _name, char* _buf, uint32_t& _len )
{
	int32_t ncol = find_col( _name );
	if( ncol > -1 )
	{
		if ( get_blob( (int32_t)ncol, _buf, _len ) == 0 )
			return 0;
	}
	ERR(2)( "[DB](database) get_blob, not find col; name = %s ", _name );
	return -1;	
}


/******************************************************************************
for lua
******************************************************************************/

int32_t DataBase::c_connect( lua_State* _L )
{
    lcheck_argc( _L, 4 );
    const char* host = lua_tolstring( _L, 1, NULL );
    const char* user= lua_tolstring( _L, 2, NULL );
    const char* passwd = lua_tolstring( _L, 3, NULL );
    uint16_t port = (uint16_t)lua_tonumber( _L, 4 );

    int32_t ret = connect( host, user, passwd, NULL, port);
    lua_pushnumber( _L, ret );
    return 1;
}

int32_t DataBase::c_is_db_exist( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    const char* db_name = lua_tolstring( _L, 1, NULL );
    clear();
    bool result = false;
    if( ( pres_ = mysql_list_dbs( &mysql_, db_name ) ) != NULL ){
        result = mysql_num_rows( pres_ ) == 0 ? false : true;
    }
    lua_pushboolean( _L, result );
    return 1;
}

int32_t DataBase::c_select_db( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    const char* db_name = lua_tolstring( _L, 1, NULL );
    int32_t result = mysql_select_db( &mysql_, db_name );
    if( result == 0 )
        LOG(2)( "[DB](database) DataBase::select database to:%s", db_name );
    lua_pushnumber( _L, result );
    return 1;
}

int32_t DataBase::c_query_table_list( lua_State* _L )
{
    clear();
    bool result = false;
    if( ( pres_ = mysql_list_tables( &mysql_, "") ) != NULL )
        result = true;

	pfields_ = mysql_fetch_fields( pres_ );
	nfields_ = mysql_num_fields( pres_ );
    lengths_ = mysql_fetch_lengths( pres_ );

    lua_pushboolean( _L, result );
    return 1;
}

int32_t DataBase::c_mysql_escape( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    const char* sql = lua_tolstring( _L, 1, NULL );
    uint32_t len = lua_tonumber( _L, 2 );

    MyString ret = mysql_escape( sql, len );
    lua_pushstring( _L, ret.c_str() );
    return 1;
}

int32_t DataBase::c_modify( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    const char* sql = lua_tolstring( _L, 1, NULL );
    
    int32_t ret = modify( sql );
    lua_pushnumber( _L, ret );
    return 1;
}
int32_t DataBase::c_query( lua_State* _L ) 
{
    lcheck_argc( _L, 1 );
    const char* sql = lua_tolstring( _L, 1, NULL );
    
    int32_t ret = query( sql );
    lua_pushnumber( _L, ret );
    return 1;
}

int32_t DataBase::c_row_num( lua_State* _L )
{
    lcheck_argc( _L, 0 );
    int32_t ret = row_num();
    lua_pushnumber( _L, ret );
    return 1;
}

int32_t DataBase::c_fetch( lua_State* _L )
{
    lcheck_argc( _L, 0 );
    int32_t ret = fetch();
    lua_pushnumber( _L, ret );
    return 1;
}

int32_t DataBase::c_get_insert_id( lua_State* _L ) 
{ 
    lcheck_argc( _L, 0 );
    int32_t ret = get_insert_id();
    lua_pushnumber( _L, ret );
    return 1;
}
int32_t DataBase::c_get_errno( lua_State* _L ) 
{ 
    lcheck_argc( _L, 0 );
    int32_t ret = get_errno(); 
    lua_pushnumber( _L, ret );
    return 1;
}
int32_t DataBase::c_get_error( lua_State* _L ) 
{
    lcheck_argc( _L, 0 );
    const char* ret = get_error(); 
    lua_pushstring( _L, ret );
    return 1;
}

int32_t DataBase::c_get_int32( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    int32_t result = 0;
    int32_t ret = 0;
    if( lua_isnumber( _L, 1 ) ){
        uint32_t col = lua_tonumber( _L, 1 );
        result = get_int32( col, &ret );
    }
    else{
        const char* name = lua_tolstring( _L, 1, NULL );
        result = get_int32( name, &ret );
    }
    lua_pushnumber( _L, result );
    lua_pushnumber( _L, ret );
    return 2;
}

int32_t DataBase::c_get_int64( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    int32_t result = 0;
    int64_t ret = 0;
    if( lua_isnumber( _L, 1 ) ){
        uint32_t col = lua_tonumber( _L, 1 );
        result = get_int64( col, &ret );
    }
    else{
        const char* name = lua_tolstring( _L, 1, NULL );
        result = get_int64( name, &ret );
    }
    lua_pushnumber( _L, result );
    lua_pushnumber( _L, ret );
    return 2;
}

int32_t DataBase::c_get_uint64( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    int32_t result = 0;
    uint64_t ret = 0;
    if( lua_isnumber( _L, 1 ) ){
        uint32_t col = lua_tonumber( _L, 1 );
        result = get_uint64( col, &ret );
    }
    else{
        const char* name = lua_tolstring( _L, 1, NULL );
        result = get_uint64( name, &ret );
    }
    lua_pushnumber( _L, result );
    lua_pushnumber( _L, ret );
    return 2;
}

int32_t DataBase::c_get_float( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    int32_t result = 0;
    float ret = 0;
    if( lua_isnumber( _L, 1 ) ){
        uint32_t col = lua_tonumber( _L, 1 );
        result = get_float( col, &ret );
    }
    else{
        const char* name = lua_tolstring( _L, 1, NULL );
        result = get_float( name, &ret );
    }
    lua_pushnumber( _L, result );
    lua_pushnumber( _L, ret );
    return 2;
}

int32_t DataBase::c_get_double( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    int32_t result = 0;
    double ret = 0;
    if( lua_isnumber( _L, 1 ) ){
        uint32_t col = lua_tonumber( _L, 1 );
        result = get_double( col, &ret );
    }
    else{
        const char* name = lua_tolstring( _L, 1, NULL );
        result = get_double( name, &ret );
    }
    lua_pushnumber( _L, result );
    lua_pushnumber( _L, ret );
    return 2;
}

int32_t DataBase::c_get_char( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    int32_t result = 0;
    char ret;
    if( lua_isnumber( _L, 1 ) ){
        uint32_t col = lua_tonumber( _L, 1 );
        result = get_char( col, &ret );
    }
    else{
        const char* name = lua_tolstring( _L, 1, NULL );
        result = get_char( name, &ret );
    }
    lua_pushnumber( _L, result );
    lua_pushlstring( _L, &ret, 1 );
    return 2;
}

int32_t DataBase::c_get_str( lua_State* _L )
{
    lcheck_argc( _L, 2 );
    int32_t result = 0;
    char ret[MAX_DATABASE_STR_LEN] = { 0, };
    if( lua_isnumber( _L, 1 ) ){
        uint32_t col = lua_tonumber( _L, 1 );
        uint32_t len = lua_tonumber( _L, 2 );
        if( len > MAX_DATABASE_STR_LEN ){
            ERR(2)( "[DataBase] (c_get_str) string too long:%d, max:%d ", len, MAX_DATABASE_STR_LEN );
        }
        result = get_str( col, ret, len );
    }
    else{
        const char* name = lua_tolstring( _L, 1, NULL );
        uint32_t len = lua_tonumber( _L, 2 );
        if( len > MAX_DATABASE_STR_LEN ){
            ERR(2)( "[DataBase] (c_get_str) string too long:%d, max:%d ", len, MAX_DATABASE_STR_LEN );
        }
        result = get_str( name, ret, len );
    }
    lua_pushnumber( _L, result );
    lua_pushstring( _L, ret );
    return 2;
}

int32_t DataBase::c_get_format( lua_State* _L )
{
    int32_t narg = lua_gettop( _L );
    if( narg < 2 ) {
        ERR(2)( "[DataBase] (c_get_format), narg < 2" );
        return 0;
    }

    size_t len;
    const char *fmt = luaL_checklstring( _L, 1, &len );
    if( !fmt ) {
        ERR(2)( "[DataBase] (c_get_format), invalid format" );
        return 0;
    }

    if( len != size_t( narg-1 ) ) {
        ERR(2)( "[DataBase] (c_get_format), format: %s not match nargs: %d", fmt, narg );
        return 0;
    }

    /*
     * a: buffer, blob
     * b: char, int8
     * i: int, int32
     * l: long, int64
     * L: qword, uint64
     * f: float
     * d: double
     * s: string
     */

    for( size_t n = 0; n < len; n++ ) {
        const char* name = lua_tolstring( _L, n+2, NULL );
        switch( fmt[n] ) {
            case 'b': {
                char b = 0;
                if( get_char( name, &b ) ){
                    ERR(2)( "[DataBase] (c_get_format), sql err, type:c, name:%s", name );
                    return 0;
                }
                lua_pushlstring( _L, &b, 1 );
                break;
            }
            case 'i': {
                int32_t i = 0;
                if( get_int32( name, &i ) ){
                    ERR(2)( "[DataBase] (c_get_format), sql err, type:i, name:%s", name );
                    return 0;
                }
                lua_pushnumber( _L, i );
                break;
            }
            case 'l': {
                int64_t l = 0;
                if( get_int64( name, &l ) ){
                    ERR(2)( "[DataBase] (c_get_format), sql err, type:l, name:%s", name );
                    return 0;
                }
                lua_pushnumber( _L, l );
                break;
            }
            case 'L': {
                uint64_t L = 0;
                if( get_uint64( name, &L ) ){
                    ERR(2)( "[DataBase] (c_get_format), sql err, type:q, name:%s", name );
                    return 0;
                }
                lua_pushnumber( _L, L );
                break;
            }
            case 'f': {
                float f = 0;
                if( get_float( name, &f ) ){
                    ERR(2)( "[DataBase] (c_get_format), sql err, type:f, name:%s", name );
                    return 0;
                }
                lua_pushnumber( _L, f );
                break;
            }
            case 'd': {
                double d = 0;
                if( get_double( name, &d ) ){
                    ERR(2)( "[DataBase] (c_get_format), sql err, type:o, name:%s", name );
                    return 0;
                }
                lua_pushnumber( _L, d );
                break;
            }
            case 's': {
                char s[MAX_DATABASE_STR_LEN] = {0};
                if( get_str( name, s, MAX_DATABASE_STR_LEN ) ){
                    ERR(2)( "[DataBase] (c_get_format), sql err, type:t, name:%s", name );
                    return 0;
                }
                lua_pushstring( _L, s );
                break;
            }
            case 'a': {
                char s[MAX_DATABASE_STR_LEN] = {0};
                uint32_t len = 0;
                if( get_blob( name, escape_str_, len ) ){
                    ERR(2)( "[DataBase] (c_get_format), sql err, type:a, name:%s", name );
                    return 0;
                }
                lua_pushlstring( _L, s, len );
                break;
            }
            default: {
                ERR(2)( "[DataBase] c_get_format, invalid format: %c", fmt[n] );
                return 0;
            }
        }
    }
    return len;
}

int32_t DataBase::stmt_prepare( const char * _sql )
{
    clear();
    stmt_close();

    intptr_t iptr = 0;
    if ( stmt_map_.find( _sql, iptr ) )
    {
        MYSQL_STMT* mysql_stmt = ( MYSQL_STMT* )iptr;
        unsigned int err_no = mysql_stmt_errno( mysql_stmt );
        if ( err_no )
        {
            LOG(2)( "[DataBase](stmt_prepare) invalid mysql_stmt: 0x%x, err_no: %u, clear stmt_map_", mysql_stmt, err_no );
            clear_stmt_map();
        }
        else
        {
            mysql_stmt_ = mysql_stmt;
            if ( mysql_stmt_free_result( mysql_stmt_ ) )
            {
                ERR(2)( "[DataBase](stmt_prepare) mysql_stmt_free_result failed:[0x%x]", mysql_stmt_ );
            }
            return 0;
        }
    }

    mysql_stmt_ = mysql_stmt_init( &mysql_ );
    if ( !mysql_stmt_ )
    {
        ERR(2)( "[DataBase](stmt_prepare) mysql_stmt_init failed, out of memory" );
        return -1;
    }

    if ( mysql_stmt_prepare( mysql_stmt_, _sql, strlen( _sql ) ) )
    {
        handle_stmt_error( "mysql_stmt_prepare failed", _sql );
        return -1;
    }
	LOG(2)( "[DB](database) stmt_prepare:[0x%x, %s]", mysql_stmt_, _sql );

    if( !stmt_map_.set( _sql, (intptr_t)mysql_stmt_ ) )
    {
        ERR(2)( "[DataBase](stmt_prepare) cache mysql_stmt_ failed" );
        mysql_stmt_close( mysql_stmt_ ); 
        return -1;
    }
    return 0;
}

int32_t DataBase::stmt_get_insert_id()
{
    return mysql_stmt_insert_id( mysql_stmt_ );
}

void DataBase::add_escape_str_stmt_index(unsigned _num)
{
    //32位对齐
    escape_str_stmt_index_ += ((_num + 3)/4) * 4;
}

int DataBase::stmt_set_param_bind(
        MYSQL_BIND& _bind, 
        enum_field_types _buffer_type, const char* _data, int _data_len, int _is_unsigned)
{
    //wrong reuse the bind structure, make it crash
    assert (_bind.buffer_type == 0); 
    if (escape_str_stmt_index_ + _data_len > MAX_ESCAPE_STR){
        ERR(2)("[DataBase]stmt_set_param_bind, buffer reach MAX_ESCAPE_STR");
        return -1;
    }

    /*
     * b: char, int8
     * i: int, int32
     * l: long, int64
     * L: qword, uint64
     * f: float
     * d: double
     * s: string
     */
    _bind.buffer_type = _buffer_type;
    if ( _bind.buffer_type == MYSQL_TYPE_TINY
            || _bind.buffer_type == MYSQL_TYPE_LONG
            || _bind.buffer_type == MYSQL_TYPE_LONGLONG
            || _bind.buffer_type == MYSQL_TYPE_FLOAT
            || _bind.buffer_type == MYSQL_TYPE_DOUBLE) {

        _bind.buffer = escape_str_ + escape_str_stmt_index_;
        _bind.is_unsigned = _is_unsigned;
        memcpy(_bind.buffer, _data, _data_len);
        add_escape_str_stmt_index(_data_len);

    }else if( _bind.buffer_type == MYSQL_TYPE_STRING
            || _bind.buffer_type == MYSQL_TYPE_BLOB){

        _bind.buffer = escape_str_ + escape_str_stmt_index_;
        memcpy(_bind.buffer, _data, _data_len);
        add_escape_str_stmt_index(_data_len);
        _bind.buffer_length = _data_len;
        _bind.length = & _bind.buffer_length;

    }else{
        ERR(2)("[DataBase]stmt_set_param_bind, unknow buffer_type");
        //not support other
        assert(0);
    }
    return 0;

}

int DataBase::stmt_set_result_bind(
        MYSQL_BIND& _bind, enum_field_types _buffer_type, int _buffer_len)
{
    //wrong reuse the bind structure, make it crash
    assert (_bind.buffer_type == 0); 
    if (escape_str_stmt_index_ + _buffer_len + sizeof(*_bind.length) > MAX_ESCAPE_STR){
        ERR(2)("[DataBase]stmt_set_param_bind, buffer reach MAX_ESCAPE_STR");
        return -1;
    }

    /*
     * b: char, int8
     * i: int, int32
     * l: long, int64
     * L: qword, uint64
     * f: float
     * d: double
     * s: string
     */
    _bind.buffer_type = _buffer_type;
    if ( _bind.buffer_type == MYSQL_TYPE_TINY
            || _bind.buffer_type == MYSQL_TYPE_LONG
            || _bind.buffer_type == MYSQL_TYPE_LONGLONG
            || _bind.buffer_type == MYSQL_TYPE_FLOAT
            || _bind.buffer_type == MYSQL_TYPE_DOUBLE) {

        _bind.buffer = escape_str_ + escape_str_stmt_index_;
        memset(_bind.buffer, 0, _buffer_len);
        add_escape_str_stmt_index(_buffer_len);

    }else if( _bind.buffer_type == MYSQL_TYPE_STRING
            || _bind.buffer_type == MYSQL_TYPE_BLOB){

        _bind.buffer_length = _buffer_len;

        _bind.length = (long unsigned int*)(escape_str_ + escape_str_stmt_index_);
        memset(_bind.length, 0, sizeof(*_bind.length) );
        add_escape_str_stmt_index( sizeof(*_bind.length) );

        _bind.buffer = escape_str_ + escape_str_stmt_index_;
        memset(_bind.buffer, 0, _buffer_len);
        add_escape_str_stmt_index(_buffer_len);

    }else{
        ERR(2)("[DataBase]stmt_set_param_bind, unknow buffer_type");
        //not support other
        assert(0);
    }
    return 0;

}

int32_t DataBase::stmt_fetch()
{
    if( !mysql_stmt_ )
    {
        ERR(2)("[DataBase](stmt_fetch) mysql_fetch failed: empty mysql_stmt_");
        return -1;
    }
    if ( mysql_stmt_fetch( mysql_stmt_ ) )
    {
        handle_stmt_error( "mysql_stmt_fetch failed", "" );
        return -1;
    }
    return 0;
}

int32_t DataBase::stmt_close()
{
    if ( mysql_bind_param_ )
    {
        delete[] mysql_bind_param_;
        mysql_bind_param_ = NULL;
    }
    if ( mysql_bind_result_ )
    {
        delete[] mysql_bind_result_;
        mysql_bind_result_ = NULL;
    }
	escape_str_stmt_index_ = 0;
    return 0;
}

void DataBase::clear_stmt_map()
{
    MYSQL_STMT* mysql_stmt = NULL;
    StrNode* node = stmt_map_.first();
    while( node )
    {
        mysql_stmt = (MYSQL_STMT*)(node->val);
        LOG(2)( "[DataBase](clear_stmt_map) mysql_stmt_close:[0x%x]", mysql_stmt );
        mysql_stmt_close( mysql_stmt ); 
        node = stmt_map_.next( node );
    }
    stmt_map_.clean();
}

int32_t DataBase::c_stmt_format_modify( lua_State* _L )
{
    //check input
    int32_t narg = lua_gettop( _L );
    if( narg < 1 ) {
        ERR(2)( "[DataBase] (c_stmt_format_modify), narg < 1" );
        lua_pushnumber( _L, -1 );
        return 1;
    }

    const char* sql = lua_tolstring( _L, 1, NULL );
    if (stmt_prepare( sql ) != 0){
        ERR(2)( "[DataBase] (c_stmt_format_modify), stmt_prepare error" );
        lua_pushnumber( _L, -1 );
        return 1;
    }

    size_t len = 0;
    if ( narg > 1 ){
        const char *fmt = luaL_checklstring( _L, 2, &len );
        if( !fmt ) {
            ERR(2)( "[DataBase] (c_stmt_format_modify), invalid format" );
            lua_pushnumber( _L, -1 );
            return 1;
        }

        if( len != size_t( narg-2 ) ) {
            ERR(2)( "[DataBase] (c_stmt_format_modify), format: %s not match nargs: %d", fmt, narg );
            lua_pushnumber( _L, -1 );
            return 1;
        }
        if( len !=  mysql_stmt_param_count(mysql_stmt_)) {
            ERR(2)( "[DataBase] (c_stmt_format_modify), format: %s not match stmt len: %d", fmt, mysql_stmt_param_count(mysql_stmt_) );
            lua_pushnumber( _L, -1 );
            return 1;
        }

        if (len > 0 )
        {
            assert(mysql_bind_param_ == NULL);
            mysql_bind_param_ = new MYSQL_BIND[len];
            memset(mysql_bind_param_, 0, sizeof(MYSQL_BIND) * len);
        }

        /*
         * b: char, int8
         * i: int, int32
         * l: long, int64
         * L: qword, uint64
         * f: float
         * d: double
         * s: string
         */

        int bind_result = 0;
        for( size_t n = 0; n < len; n++ ) {
            if (bind_result != 0){
                ERR(2)( "[DataBase] (c_stmt_format_modify), bind failed for index:%d", n-1 );
                lua_pushnumber( _L, -1 );
                return 1;
            }

            switch( fmt[n] ) {
                case 'b':{ 
                    const char* b = lua_tolstring( _L, n + 3, NULL );
                    bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_TINY, b, 1, 0);
                    break;
                }
                case 'i':{
                    int32_t i = lua_tointeger( _L, n + 3);
                    bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_LONG, (char*)&i, sizeof(i), 0);
                    break;
                }
                case 'l':{
                    int64_t l = lua_tonumber( _L, n + 3);
                    bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_LONGLONG, (char*)&l, sizeof(l), 0);
                    break;
                }
                case 'L':{
                    uint64_t L = lua_tonumber( _L, n + 3);
                    bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_LONGLONG, (char*)&L, sizeof(L), 1);
                    break;
                }
                case 'f':{
                    float f = lua_tonumber( _L, n + 3);
                    bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_FLOAT, (char*)&f, sizeof(f), 0);
                    break;
                }
                case 'd':{
                    double d = lua_tonumber( _L, n + 3);
                    bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_DOUBLE, (char*)&d, sizeof(d), 0);
                    break;
                }
                case 'a':{
                    size_t bufsize = 0;
                    const char* buf = lua_tolstring( _L, n + 3, &bufsize );
                    bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_BLOB, buf, bufsize, 0);
                    break;
                }
                case 's':{
                    size_t str_len;
                    const char* s= lua_tolstring( _L, n + 3, &str_len );
                    bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_STRING, (char*)s, str_len, 0);
                    break;
                }
                default:{
                    ERR(2)( "[DataBase] c_stmt_format_modify, invalid format: %c", fmt[n] );
                    lua_pushnumber( _L, -1 );
                    return 1;
                }
            }
        }
        

        if ( mysql_stmt_bind_param( mysql_stmt_, mysql_bind_param_ ) )
        {
            handle_stmt_error( "mysql_stmt_bind_param failed", sql );
            lua_pushnumber( _L, -1 );
            return 1;
        }
    }

    if ( mysql_stmt_execute( mysql_stmt_ ) )
    {
        if ( handle_stmt_error( "mysql_stmt_execute failed", sql ) )
        {
            lua_pushnumber( _L, -1 );
            return 1;
        }
        else
        {
            return c_stmt_format_modify( _L );
        }
    }
    int32_t affected_rows = mysql_stmt_affected_rows( mysql_stmt_ );
    lua_pushnumber( _L, affected_rows );
    return 1;
}

int32_t DataBase::c_stmt_get_insert_id( lua_State* _L )
{
    lcheck_argc( _L, 0 );
    lua_pushnumber( _L, stmt_get_insert_id() );
    return 1;
}

int32_t DataBase::c_stmt_format_query( lua_State* _L )
{
    //check input
    int32_t narg = lua_gettop( _L );
    if( narg < 3 ) {
        ERR(2)( "[DataBase] (c_stmt_format_query), narg < 3" );
        lua_settop( _L, 0);
        lua_pushnumber( _L, -1 );
        return 1;
    }

    const char* sql = lua_tolstring( _L, 1, NULL );
    if (stmt_prepare( sql ) != 0){
        ERR(2)( "[DataBase] (c_stmt_format_query), stmt_prepare error" );
        lua_pushnumber( _L, -1 );
        return 1;
    }

    size_t input_len = 0;
    const char *fmt = luaL_checklstring( _L, 2, &input_len );
    if( !fmt ) {
        ERR(2)( "[DataBase] (c_stmt_format_query), invalid format" );
        lua_pushnumber( _L, -1 );
        return 1;
    }

    if( input_len !=  mysql_stmt_param_count(mysql_stmt_)) {
        ERR(2)( "[DataBase] (c_stmt_format_query), format: %s not match stmt len: %d", fmt, mysql_stmt_param_count(mysql_stmt_) );
        lua_pushnumber( _L, -1 );
        return 1;
    }

    if (input_len > 0 )
    {
        assert(mysql_bind_param_ == NULL);
        mysql_bind_param_ = new MYSQL_BIND[input_len];
        memset(mysql_bind_param_, 0, sizeof(MYSQL_BIND) * input_len);
    }

    /*
     * a: buffer, blob
     * b: char, int8
     * i: int, int32
     * l: long, int64
     * L: qword, uint64
     * f: float
     * d: double
     * s: string
     */

    int bind_result = 0;
    for( size_t n = 0; n < input_len; n++ ) {
        if (bind_result != 0){
            ERR(2)( "[DataBase] (c_stmt_format_query), bind failed for index:%d", n-1 );
            lua_pushnumber( _L, -1 );
            return 1;
        }

        switch( fmt[n] ) {
            case 'b':{ 
                const char* b = lua_tolstring( _L, n + 3, NULL );
                LOG(2)("[DataBase] (c_stmt_format_query) input %d:%s", n, b);
                bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_TINY, b, 1, 0);
                break;
            }
            case 'i':{
                int32_t i = lua_tointeger( _L, n + 3);
                LOG(2)("[DataBase] (c_stmt_format_query) input %d:%d", n, i);
                bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_LONG, (char*)&i, sizeof(i), 0);
                break;
            }
            case 'l':{
                int64_t l = lua_tonumber( _L, n + 3);
                LOG(2)("[DataBase] (c_stmt_format_query) input %d:%lld", n, l);
                bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_LONGLONG, (char*)&l, sizeof(l), 0);
                break;
            }
            case 'L':{
                uint64_t L = lua_tonumber( _L, n + 3);
                LOG(2)("[DataBase] (c_stmt_format_query) input %d:%llu", n, L);
                bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_LONGLONG, (char*)&L, sizeof(L), 1);
                break;
            }
            case 'f':{
                float f = lua_tonumber( _L, n + 3);
                LOG(2)("[DataBase] (c_stmt_format_query) input %d:%f", n, f);
                bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_FLOAT, (char*)&f, sizeof(f), 0);
                break;
            }
            case 'd':{
                double d = lua_tonumber( _L, n + 3);
                LOG(2)("[DataBase] (c_stmt_format_query) input %d:%g", n, d);
                bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_DOUBLE, (char*)&d, sizeof(d), 0);
                break;
            }
            case 'a':{
                size_t str_len;
                const char* s= lua_tolstring( _L, n + 3, &str_len );
                LOG(2)("[DataBase] (c_stmt_format_query) input %d:%s", n, s);
                bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_BLOB, (char*)s, str_len, 0);
                break;
            }
            case 's':{
                size_t str_len;
                const char* s= lua_tolstring( _L, n + 3, &str_len );
                LOG(2)("[DataBase] (c_stmt_format_query) input %d:%s", n, s);
                bind_result = stmt_set_param_bind(mysql_bind_param_[n], MYSQL_TYPE_STRING, (char*)s, str_len, 0);
                break;
            }
            default:{
                ERR(2)( "[DataBase] c_stmt_format_query, invalid format: %c", fmt[n] );
                lua_pushnumber( _L, -1 );
                return 1;
            }
        }
    }
        
    if ( mysql_stmt_bind_param( mysql_stmt_, mysql_bind_param_ ) )
    {
        handle_stmt_error( "mysql_stmt_bind_param failed", sql );
        lua_pushnumber( _L, -1 );
        return 1;
    }

    if ( mysql_stmt_execute( mysql_stmt_ ) )
    {
        if ( handle_stmt_error( "mysql_stmt_execute failed", sql ) )
        {
            lua_pushnumber( _L, -1 );
            return 1;
        }
        else
        {
            return c_stmt_format_query( _L );
        }
    }

    if ( mysql_stmt_store_result( mysql_stmt_ ) )
    {
        handle_stmt_error( "mysql_stmt_store_result failed", sql );
        lua_pushnumber( _L, -1 );
        return 1;
    }

    //---------------------------------------
    //output below
    //---------------------------------------
    size_t output_start_index = 2 + input_len + 1;
    size_t output_len = 0;
    const char *out_fmt = luaL_checklstring( _L, output_start_index, &output_len );
    if( !out_fmt ) {
        ERR(2)( "[DataBase] (c_stmt_format_query), invalid format" );
        lua_pushnumber( _L, -1 );
        return 1;
    }

	if( ( pres_ = mysql_stmt_result_metadata( mysql_stmt_ ) ) == NULL )
	{
        handle_stmt_error( "mysql_stmt_result_metadata failed", sql );
        lua_pushnumber( _L, -1 );
        return 1;
	}
	pfields_ = mysql_fetch_fields( pres_ );
	nfields_ = mysql_num_fields( pres_ );
    if( output_len !=  nfields_)
    {
        ERR(2)( "[DataBase] (c_stmt_format_query), format: %s not match stmt len: %d", out_fmt, nfields_ );
        lua_pushnumber( _L, -1 );
        return 1;
    }

    if (output_len > 0 )
    {
        assert(mysql_bind_result_ == NULL);
        mysql_bind_result_ = new MYSQL_BIND[output_len];
        memset(mysql_bind_result_, 0, sizeof(MYSQL_BIND) * output_len);
    }

    /*
     * b: char, int8
     * i: int, int32
     * l: long, int64
     * L: qword, uint64
     * f: float
     * d: double
     * s: string
     */

    bind_result = 0;
    for( size_t n = 0; n < output_len; n++ ) {
        if (bind_result != 0){
            ERR(2)( "[DataBase] (c_stmt_format_query), bind output failed for index:%d", n );
            lua_pushnumber( _L, -1 );
            return 1;
        }

        switch( out_fmt[n] ) {
            case 'b':{ 
                bind_result = stmt_set_result_bind(mysql_bind_result_[n], MYSQL_TYPE_TINY, 1);
                break;
            }
            case 'i':{
                bind_result = stmt_set_result_bind(mysql_bind_result_[n], MYSQL_TYPE_LONG, sizeof(long));
                break;
            }
            case 'l':{
                bind_result = stmt_set_result_bind(mysql_bind_result_[n], MYSQL_TYPE_LONGLONG, sizeof(long long));
                break;
            }
            case 'L':{
                bind_result = stmt_set_result_bind(mysql_bind_result_[n], MYSQL_TYPE_LONGLONG, sizeof(long long));
                break;
            }
            case 'f':{
                bind_result = stmt_set_result_bind(mysql_bind_result_[n], MYSQL_TYPE_FLOAT, sizeof(float));
                break;
            }
            case 'd':{
                bind_result = stmt_set_result_bind(mysql_bind_result_[n], MYSQL_TYPE_DOUBLE, sizeof(double));
                break;
            }
            case 'a':{
                bind_result = stmt_set_result_bind(mysql_bind_result_[n], MYSQL_TYPE_BLOB, BLOB_BIND_BUFFER_LEN);
                break;
            }
            case 's':{
                bind_result = stmt_set_result_bind(mysql_bind_result_[n], MYSQL_TYPE_STRING, STRING_BIND_BUFFER_LEN);
                break;
            }
            default:{
                ERR(2)( "[DataBase] c_stmt_format_query, invalid format: %c", out_fmt[n] );
                lua_pushnumber( _L, -1 );
                return 1;
            }
        }
    }
        
    if ( mysql_stmt_bind_result( mysql_stmt_, mysql_bind_result_ ) )
    {
        handle_stmt_error( "mysql_stmt_bind_result failed", sql );
        lua_pushnumber( _L, -1 );
        return 1;
    }
 
    lua_pushnumber( _L, 0 );
    return 1;
}

int32_t DataBase::c_stmt_fetch( lua_State* _L )
{
    lcheck_argc( _L, 0 );
    lua_pushnumber( _L, stmt_fetch() );
    return 1;
}

int32_t DataBase::c_stmt_get( lua_State* _L )
{
    int32_t narg = lua_gettop( _L );
    if( narg < 1 ) {
        ERR(2)( "[DataBase] (c_get_format), narg < 1" );
        return 0;
    }
    if (mysql_bind_result_ == NULL){
        ERR(2)( "[DataBase] (c_get_format), bind result is null" );
        return 0;
    }

    for( int32_t n = 1; n <= narg; n++ ) {
        const char* name = lua_tolstring( _L, n, NULL );
        int32_t field_index = find_col(name);
        if (field_index < 0 ) {
            ERR(2)( "[DataBase] (c_stmt_get), sql err, type:t, name:%s", name );
            return 0;
        }
        MYSQL_BIND& bind = mysql_bind_result_[field_index] ; 
        /*
         * a: buffer, blob
         * b: char, int8
         * i: int, int32
         * l: long, int64
         * L: qword, uint64
         * f: float
         * d: double
         * s: string
         */
        if( bind.buffer_type == MYSQL_TYPE_TINY ) {
            lua_pushlstring( _L, (const char*)bind.buffer, 1 );

        }else if( bind.buffer_type == MYSQL_TYPE_LONG ){
            if ( bind.is_unsigned ) {
                int32_t i = *((int32_t*)bind.buffer);
                lua_pushnumber( _L, i );
            }else{
                uint32_t i = *((uint32_t*)bind.buffer);
                lua_pushnumber( _L, i );
            }

        } else if( bind.buffer_type == MYSQL_TYPE_LONGLONG ){
            if (bind.is_unsigned) {
                int64_t l = *((int64_t*)bind.buffer);
                lua_pushnumber( _L, l );
            }else{
                uint64_t l = *((uint64_t*)bind.buffer);
                lua_pushnumber( _L, l );
            }

        } else if( bind.buffer_type == MYSQL_TYPE_FLOAT ){
            float f = *((float*)bind.buffer);
            lua_pushnumber( _L, f );

        } else if( bind.buffer_type == MYSQL_TYPE_DOUBLE ) {
            double f = *((double*)bind.buffer);
            lua_pushnumber( _L, f );

        }else if( bind.buffer_type == MYSQL_TYPE_STRING ){
            lua_pushlstring( _L, (char*)bind.buffer, *bind.length );

        }else if( bind.buffer_type == MYSQL_TYPE_BLOB ){
            lua_pushlstring( _L, (char*)bind.buffer, *bind.length );

        }else {
            ERR(2)( "[DataBase] c_stmt_get, invalid type: %d", bind.buffer_type );
            return 0;
        }
    }
    return narg; 
}

int32_t DataBase::c_stmt_row_num( lua_State* _L )
{
    lcheck_argc( _L, 0 );
    if ( mysql_stmt_ == NULL )
        return 0;
    lua_pushnumber( _L, mysql_stmt_num_rows(mysql_stmt_) );
    return 1;
}

int32_t DataBase::c_autocommit( lua_State* _L )
{
    lcheck_argc( _L, 1 );
    bool mode = lua_toboolean( _L, 1 );
    int32_t ret = mysql_autocommit( &mysql_, mode );
    lua_pushnumber( _L, ret );
    return 1;
}

int32_t DataBase::c_begin( lua_State* _L )
{
    lcheck_argc( _L, 0 );
    lua_pushnumber( _L, begin() );
    return 1;
}

int32_t DataBase::c_rollback( lua_State* _L )
{
    lcheck_argc( _L, 0 );
    lua_pushnumber( _L, rollback() );
    return 1;
}

int32_t DataBase::c_commit( lua_State* _L )
{
    lcheck_argc( _L, 0 );
    lua_pushnumber( _L, commit() );
    return 1;
}

const char DataBase::className[] = "DataBase";
const bool DataBase::gc_delete_ = false;
Lunar<DataBase>::RegType DataBase::methods[] =
{
    LUNAR_DATABASE_METHODS,
    { NULL, NULL }
};
