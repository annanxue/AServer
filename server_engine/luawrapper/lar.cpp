#include "3dmath.h"
#include "log.h"
#include "llog.h"
#include "netmng.h"
#include "lmisc.h"
#include "lar.h"


#define	OP_STRING	's'		/* string */
#define	OP_FLOAT	'f'		/* float */
#define	OP_DOUBLE	'd'		/* double */
#define OP_BOOL		'o'		/* bool */
#define	OP_BYTE		'b'		/* byte */
#define	OP_UBYTE	'B'		/* unsigned byte */
#define	OP_SHORT	'h'		/* short */
#define	OP_USHORT	'H'		/* unsigned short */
#define	OP_INT		'i'		/* int */
#define	OP_UINT		'I'		/* unsigned int */
#define	OP_LONG		'l'		/* long */
#define	OP_ULONG	'L'		/* unsigned long */


/******************************************************************************
for lua
******************************************************************************/

const char LAr::className[] = "LAr";
const bool LAr::gc_delete_ = true;

Lunar<LAr>::RegType LAr::methods[] =
{
	LUNAR_LAR_METHODS,
	{NULL, NULL}
};


LAr::LAr()
{
	ar_ = new Ar;
	if( !ar_ )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d LAR_ERR, create ar_ failed! ", __FILE__, __LINE__ );
	}
	delete_ar_ = true;
    flush_flag_ = 0;
}


LAr::LAr( Ar* _ar )
{
	ar_ = _ar;
	if( !ar_ )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d LAR_ERR, create ar_ failed! ", __FILE__, __LINE__ );
	}
	delete_ar_ = false;
    flush_flag_ = 0;
}

LAr::LAr( const void* _buff, int _len )
{
	ar_ = new Ar( _buff, _len );if( !ar_ )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d LAR_ERR, create ar_ failed! ", __FILE__, __LINE__ );
	}
	delete_ar_ = true;
    flush_flag_ = 0;
}

int LAr::init( const char* _buff, int _len )
{
	if( ar_ )			return 0;
	if( _buff == NULL )
	{
		ar_ = new Ar;
	}
	else
	{
		ar_ = new Ar( _buff, _len );
	}
	if( !ar_ )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d LAR_ERR, create ar_ failed! ", __FILE__, __LINE__ );
		return -1;
	}
	delete_ar_ = true;
    flush_flag_ = 0;
	return 0;
}

int LAr::reuse( const char* _buff, int _len )
{
	SAFE_DELETE( ar_ );
	return init( _buff, _len );
}

void LAr::destroy()
{
	if( delete_ar_ )
	{
		SAFE_DELETE( ar_ );
		delete_ar_ = false;
	}
}

LAr::~LAr()
{
	destroy();
}

/*****************************************************************************
* write functions
******************************************************************************/

bool w_check_argc( lua_State* _L, int c, int line  ) 
{
    assert( _L );
    int argc = lua_gettop( _L );
    if( argc == c ) {
        return true;
    }

    ERR(2)( "[LUAWRAPPER](lar) write ARGC lar.cpp line = %d, argc: %d, c: %d", line, argc, c );    
    c_bt( _L );
    return false;
}

int LAr::flush( lua_State* _L )
{
    if ( ar_->is_storing()) {
        ar_->flush();
    }else{
        ERR(2)( "[LUAWRAPPER](lar) load flag ar can't be flushed ");    
    }
    return 0;
}

int LAr::write_byte( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        unsigned char chr = (unsigned char)lua_tonumber( _L, 1);
        (*ar_) << chr;
    } else {
        (*ar_) << (signed char)0;    
    }
	return 0;
}

int LAr::write_ubyte( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        unsigned char chr = (unsigned char)lua_tonumber( _L, 1);
        (*ar_) << chr;
    } else {
        (*ar_) << (unsigned char)0;    
    }
	return 0;
}


int LAr::write_char( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        char chr = (char)lua_tonumber( _L, 1);
        (*ar_) << chr;
    } else {
        (*ar_) << (char)0;    
    }
    return 0;
}

int LAr::write_short( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        short w = (short)lua_tonumber( _L, 1 );
        (*ar_) << w;
    } else {
        (*ar_) << (short)0;
    }
	return 0;
}

int LAr::write_ushort( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        unsigned short w = (unsigned short)lua_tonumber( _L, 1 );
        (*ar_) << w;
    } else {
        (*ar_) << (unsigned short)0;
    }
	return 0;
}

int LAr::write_int( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        int i = (int)lua_tonumber( _L, 1);
        (*ar_) << i;
    } else {
        (*ar_) << (int)0;
    }
	return 0;
}

int LAr::write_uint( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        unsigned int i = (unsigned int)lua_tonumber( _L, 1);
        (*ar_) << i;
    } else {
        (*ar_) << (unsigned int)0;
    }
	return 0;
}

int LAr::write_long( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        long i = (long)lua_tonumber( _L, 1);
        (*ar_) << i;
    } else {
        (*ar_) << (long)0;
    }
	return 0;
}

int LAr::write_ulong( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        u_long i = (u_long)lua_tonumber( _L, 1);
        (*ar_) << i;
    } else {
        (*ar_) << (u_long)0;
    }
	return 0;
}

int LAr::write_float( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        float f = (float)lua_tonumber( _L, 1 );
        (*ar_) << f;
    } else {
        (*ar_) << (float)0;
    }
	return 0;
}

int LAr::write_double( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        double db = (double)lua_tonumber( _L, 1 );
        (*ar_) << db;
    } else {
        (*ar_) << (double)0;
    }
	return 0;
}

int LAr::write_string( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        size_t str_len = 0;
        const char* str = lua_tolstring( _L, 1, &str_len );
        if( str ) {
            ar_->write_string(str, str_len);
        } else {
            ar_->write_string( "" );
        }
    } else {
        ar_->write_string( "" );
    }

	return 0;
}

int LAr::write_boolean( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        char b = (char)lua_toboolean( _L, 1 );
        (*ar_) << b;
    } else {
        (*ar_) << (char)0;
    }
	return 0;
}

int LAr::write_buffer( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        int len = 0;
        const char* buf = lua_tolstring( _L, 1, (size_t*)&len );
        if( buf == NULL ) {
            len = 0;
        }
        (*ar_) << len;
        if( buf ) {
            ar_->write( buf, len );
        }
    } else {
        (*ar_) << (int)0;
    }
	return 0;
}

int LAr::write_var_array_short( lua_State* _L )
{
    if( w_check_argc( _L, 2, __LINE__ ) ) {
        unsigned int n = (unsigned int)lua_tonumber( _L, 2);
        assert( n <= 255 );
        if( n & 0xFFFFFF00 ) {
            ERR(2)("[LUAWRAPPER] LAr::write_var_array_short() error, array length > 255");
            c_traceback( _L );
            (*ar_) << (unsigned char)0;
            return 0;
        }
        
        (*ar_) << ( unsigned char )n;
        unsigned int num4 = (unsigned int)(n/4.0f)*4;
        for( unsigned int i=1; i<num4+1; i=i+4 ){
            u_long offset = ar_->get_offset();
            char mask = 0;
            (*ar_) << mask;
            lua_rawgeti( _L, 1, i );
            int32_t data = (int32_t)lua_tonumber( _L, -1 );
            lua_pop( _L, 1 );
            int32_t abs = (data & 0x80000000) ? (~data + 1) : data;
            if( !abs ){}
            else if ( !(abs & 0xFFFFFF80) ){ mask |= 0x40; (*ar_)<<(char)data; }
            else if( !(abs & 0xFFFF8000) ) { mask |= 0x80; (*ar_)<<(short)data; }
            else{ mask |= 0xc0; (*ar_)<<data; }

            lua_rawgeti( _L, 1, i+1 );
            data = (int32_t)lua_tonumber( _L, -1 );
            lua_pop( _L, 1 );
            abs = (data & 0x80000000) ? (~data + 1) : data;
            if( !abs ){}
            else if ( !(abs & 0xFFFFFF80) ){ mask |= 0x10; (*ar_)<<(char)data; }
            else if( !(abs & 0xFFFF8000) ) { mask |= 0x20; (*ar_)<<(short)data; }
            else{ mask |= 0x30; (*ar_)<<data; }

            lua_rawgeti( _L, 1, i+2 );
            data = (int32_t)lua_tonumber( _L, -1 );
            lua_pop( _L, 1 );
            abs = (data & 0x80000000) ? (~data + 1) : data;
            if( !abs ){}
            else if ( !(abs & 0xFFFFFF80) ){ mask |= 0x04; (*ar_)<<(char)data; }
            else if( !(abs & 0xFFFF8000) ) { mask |= 0x08; (*ar_)<<(short)data; }
            else{ mask |= 0x0c; (*ar_)<<data; }

            lua_rawgeti( _L, 1, i+3 );
            data = (int32_t)lua_tonumber( _L, -1 );
            lua_pop( _L, 1 );
            abs = (data & 0x80000000) ? (~data + 1) : data;
            if( !abs ){}
            else if ( !(abs & 0xFFFFFF80) ){ mask |= 0x01; (*ar_)<<(char)data; }
            else if( !(abs & 0xFFFF8000) ) { mask |= 0x02; (*ar_)<<(short)data; }
            else{ mask |= 0x03; (*ar_)<<data; }

            int bufsize;
            char* buf = ar_->get_buffer( &bufsize );
            *(char*)( buf + offset ) = mask;
        }
        if( num4 < n ){
            u_long offset = ar_->get_offset();
            char mask = 0;
            (*ar_) << mask;
            for( unsigned int i=num4+1; i<n+1; i++ ){
                lua_rawgeti( _L, 1, i );
                int32_t data = (int32_t)lua_tonumber( _L, -1 );
                lua_pop( _L, 1 );
                int32_t abs = (data & 0x80000000) ? (~data + 1) : data;
                int o = 8-(i-num4)*2;
                if( !abs ){}
                else if ( !(abs & 0xFFFFFF80) ){ mask |= (0x01<<o); (*ar_)<<(char)data; }
                else if( !(abs & 0xFFFF8000) ) { mask |= (0x02<<o); (*ar_)<<(short)data; }
                else{ mask |= (0x03<<o); (*ar_)<<data; }
            }
            int bufsize;
            char* buf = ar_->get_buffer( &bufsize );
            *(char*)( buf + offset ) = mask;
        }
    }
    else{
        (*ar_) << (unsigned char)0;
    }
    return 0;
}

int LAr::write_int_degree( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        int degree = lua_tointeger( _L, 1 );
        ar_->write_int_degree( degree );
    } else {
        ar_->write_int_degree( 0 );
    }
	return 0;
}

int LAr::write_compress_pos_angle( lua_State* _L )
{
    if( w_check_argc( _L, 3, __LINE__ ) ) {
        float x = lua_tonumber( _L, 1 );
        float y = lua_tonumber( _L, 2 );
        float angle = lua_tonumber( _L, 3 );
        ar_->write_compress_pos_angle( x, y, angle );
    } else {
        ar_->write_compress_pos_angle( 0, 0, 0 );
    }
	return 0;
}

int LAr::write_lar( lua_State* _L )
{
    if( w_check_argc( _L, 1, __LINE__ ) ) {
        LAr* lar = Lunar<LAr>::check( _L, 1 );
        int bufsize = 0;
        const char* buf = lar->ar_->get_buffer( &bufsize );
        (*ar_) << bufsize;
        if ( bufsize > 0 )
            ar_->write( buf, bufsize );
    } else{
        (*ar_) << (int)0;
    }
	return 0;
}

int LAr::write_format( lua_State* _L )
{
    int narg = lua_gettop( _L );
	if( narg < 2 ){
        ERR(2)("[LUAWRAPPER] LAr::write_format() error, narg < 2 !!!");
        c_traceback( _L );
        return 0;
    }
		
	const char* format_str = lua_tostring( _L, 1 );
    if ( format_str==NULL ) {
        ERR(2)("[LUAWRAPPER] LAr::write_format() error, format_str is NULL !!!");
        c_traceback( _L );
        return 0;
    }

    int len = strlen( format_str );
    if( len != narg-1 ) {
        ERR(2)("[LUAWRAPPER] LAr::write_format() error, len[%d] != narg-1[%d]", len, narg-1);
        c_traceback( _L );
        return 0;
    }

    //unsigned int offset = ar_->get_offset();
    for( int i=0; i<len; i++ ) {
        switch( format_str[i] ) {
            case 'i': (*ar_) << (int)lua_tonumber( _L, i+2 ); break;
            case 'b': (*ar_) << (unsigned char)lua_tonumber( _L, i+2 ); break;
            case 'u': (*ar_) << (u_long)lua_tonumber( _L, i+2 ); break;
            case 'w': (*ar_) << (unsigned short)lua_tonumber( _L, i+2 ); break;
            case 'f': (*ar_) << (float)lua_tonumber( _L, i+2 ); break;
            case 'c': (*ar_) << (char)lua_tonumber( _L, i+2 ); break;
            case 'y': (*ar_) << (char)lua_toboolean( _L, i+2 ); break;
            case 'l': (*ar_) << (double)lua_tonumber( _L, i+2 ); break;
            case 'x':
            {
                int32_t data = (int32_t)lua_tonumber( _L, i + 2 ); 
                int32_t abs = (data & 0x80000000) ? (~data + 1) : data;
                if ( !(abs & 0xFFFFFFE0) ){ (*ar_)<<char(data<<2); }
                else if( !(abs & 0xFFFFE000) ) { (*ar_)<<short(data<<2 | 0x1); }
                else{ (*ar_)<<(char)0x2; (*ar_)<<data; }
                break;
            }
            case 'X':
            {
                uint32_t data = (uint32_t)lua_tonumber( _L, i + 2 ); 
                if ( !(data & 0xFFFFFFC0) ){ (*ar_)<<char(data << 2); }
                else if( !(data & 0xFFFFC000) ) { (*ar_)<<(short)((data << 2) | 0x1 ); }
                else{ (*ar_)<<(char)0x2; (*ar_)<<data; }
                break;
            }
            case 'a':
            {
                size_t bufsize = 0;
                const char* buf = lua_tolstring(_L, i+2, &bufsize );
                if ( buf ){
                    (*ar_) << (unsigned short)bufsize;
                    ar_->write( buf, bufsize );
                }else{
                    ar_->write_string( "" );
                }
                break;
            }
            case 's': 
            {
                size_t bufsize = 0;
                const char* buf = lua_tolstring(_L, i+2, &bufsize );
                if ( buf ){
                    ar_->write_string( buf, bufsize );
                }else{
                    ar_->write_string( "" );
                }
                break;
            }
            default: ERR(2)("[LUAWRAPPER] LAr::write_format() error, param[%d]=%c !!!", i+1, format_str[i]); return 0;
        }
    }

	return 0;
}

/*****************************************************************************
* read functions
******************************************************************************/
bool r_check_argc( lua_State* _L, int c, int line  ) 
{
    assert( _L );
    int argc = lua_gettop( _L );
    if( argc == c ) {
        return true;
    }

    ERR(2)( "[LUAWRAPPER](lar) read ARGC lar.cpp line = %d, argc: %d, c: %d", line, argc, c );
    c_bt( _L );
    return false;
}

//原有的接口对于128以上的数据会被转换为负数
//由于默认的编程习惯都是认为read_byte应该是以0~255为表示范围的，所以对此接口修改为无符号类型
int LAr::read_byte( lua_State* _L )  
{
    lcheck_argc( _L, 0 );
	assert( ar_ );
	unsigned char chr;
	(*ar_) >> chr;
	
	lua_pushnumber( _L, (lua_Number)chr );
	return 1;
}

int LAr::read_char( lua_State* _L )
{
    lcheck_argc( _L, 0 );
	assert( ar_  );
	char chr;
	(*ar_) >> chr;

	lua_pushnumber( _L, (lua_Number)chr );
	return 1;
}


int LAr::read_ubyte( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
	unsigned char chr;
	(*ar_) >> chr;
	
	lua_pushnumber( _L, (lua_Number)chr );
	return 1;
}


int LAr::read_short( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
	short w;
	(*ar_) >> w;

	lua_pushnumber( _L, (lua_Number)w );
	return 1;
}

int LAr::read_ushort( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
	unsigned short w;
	(*ar_) >> w;

	lua_pushnumber( _L, (lua_Number)w );
	return 1;
}

int LAr::read_int( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
	int i;
	(*ar_) >> i;

	lua_pushnumber( _L, (lua_Number)i );
	return 1;
}

int LAr::read_uint( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
	unsigned int i;
	(*ar_) >> i;

	lua_pushnumber( _L, (lua_Number)i );
	return 1;
}

int LAr::read_long( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
	long i;
	(*ar_) >> i;

	lua_pushnumber( _L, (lua_Number)i );
	return 1;
}

int LAr::read_ulong( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
	u_long i;
	(*ar_) >> i;

	lua_pushnumber( _L, (lua_Number)i );
	return 1;
}


int LAr::read_float( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
	float f;
	(*ar_) >> f;

	lua_pushnumber( _L, (lua_Number)f );
	return 1;
}

int LAr::read_double( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
	double db;
	(*ar_) >> db;

	lua_pushnumber( _L, db );
	return 1;
}

int LAr::read_string( lua_State* _L )
{
	assert( ar_ );
    
    u_short len=0;
    (*ar_) >> len;
    ar_->buf_cur_ = ar_->buf_cur_ - sizeof(u_short);

    if ( len < 8192 ) {
        /*! 使用栈内存*/
        char str[8192];
        memset( str, 0, 8192 );
        ar_->read_string((char*)str, 8192);
        if( ar_->check_exception() )
        {
            ERR(2)( "[LUAWRAPPER](lar) %s:%d, read_string() [8192] AR_ERR", __FILE__, __LINE__ );
            str[0] = '\0';
            
            /*! 当发生协议错误时，打印出堆栈信息*/
            c_bt( _L );
        }

        CHECK_AR( (*ar_) );
        lua_pushlstring( _L, str, len );
    } else {
        /*! 使用堆内存 */
        char* str = new char[len+1];
        memset( str, 0, len );
        ar_->read_string((char*)str, len+1);
        if( ar_->check_exception() )
        {
            ERR(2)( "[LUAWRAPPER](lar) %s:%d, read_string() [>8192] AR_ERR", __FILE__, __LINE__ );
            str[0] = '\0';

            /*! 当发生协议错误时，打印出堆栈信息*/
            c_bt( _L );
        }

        CHECK_AR( (*ar_) );
        lua_pushlstring( _L, str, len );
        /*
        ERR(2)( "[LUAWRAPPER](lar) %s:%d, read_string() [>8192] AR_ERR, new len: %d", __FILE__, __LINE__, len );

        c_bt( _L );
        */

        delete[] str;
    }

    return 1;
}

int LAr::read_boolean( lua_State* _L )
{	
	lcheck_argc( _L, 0 );
	assert( ar_ );
	char b;
	(*ar_) >> b;

	lua_pushboolean( _L, b );

	return 1;
}

int LAr::read_format( lua_State* _L )
{
    int narg = lua_gettop( _L );
	if( narg != 1 ){
        ERR(2)("[LUAWRAPPER] LAr::read_format() error, narg != ! !!!");
        c_traceback( _L );
        return 0;
    }

	const char* format_str = lua_tostring( _L, 1 );
    if ( format_str==NULL ) {
        ERR(2)("[LUAWRAPPER] LAr::read_format() error, format_str is NULL !!!");
        c_traceback( _L );
        return 0;
    }

    int len = strlen( format_str );
    //const char* str;
    for( int i = 0; i < len; i++ ) {
        switch( format_str[i] ) {
            case 'i':
            {
                int i ;
                ( *ar_ ) >> i;
                lua_pushnumber( _L, i );
            }
            break;
            case 'b':
            {
                unsigned char b;
                ( *ar_ ) >> b;
                lua_pushnumber( _L, b );
            }
            break;
            case 'u':
            {
                u_long u;
                ( *ar_ ) >> u;
                lua_pushnumber( _L, u );
            }
            break;
            case 'w':
            {
                unsigned short w;
                ( *ar_ ) >> w;
                lua_pushnumber( _L, w );
            }
            break;
            case 'f':
            {
                float f;
                ( *ar_ ) >> f;
                lua_pushnumber( _L, f );
            }
            break;
            case 'c':
            {
                char c;
                ( *ar_ ) >> c;
                lua_pushnumber( _L, c );
            }
            break;
            case 'y':
            {
                char y;
                ( *ar_ ) >> y;
                lua_pushboolean( _L, y );
            }
            break;
            case 'l':
            {
                double l;
                ( *ar_ ) >> l;
                lua_pushnumber( _L, l );
            }
            break;
            case 'x':
            {
                char x;
                ( *ar_ ) >> x;
                if( !(x & 0x3) ){ lua_pushnumber( _L, x>>2 ); }
                else if( !(x & 0x2) ) { char c; ( *ar_ ) >> c; lua_pushnumber( _L,  (c << 6) | ((unsigned char)(x)>>2) ); }
                else { int i; ( *ar_ ) >> i; lua_pushnumber( _L, i ); }
            }
            break;
            case 'X':
            {
                unsigned char x;
                ( *ar_ ) >> x;
                if( !(x & 0x3) ){ lua_pushnumber( _L, x>>2 ); }
                else if( !(x & 0x2) ) { unsigned char c; ( *ar_ ) >> c; lua_pushnumber( _L, (c << 6) | (x>>2) ); }
                else { unsigned int i; ( *ar_ ) >> i; lua_pushnumber( _L, i ); } 
            }
            break;
            case 'a':
            case 's':
            {
                u_short len = 0;
                ( *ar_ ) >> len;
                ar_->buf_cur_ -= sizeof( u_short );

                if( len < 8192 ) {
                    char str[8192];
                    memset( str, 0, 8192 );
                    ar_->read_string((char*)str, 8192);
                    if( ar_->check_exception() )
                    {
                        ERR(2)( "[LUAWRAPPER](lar) %s:%d, read_format() [8192] AR_ERR", __FILE__, __LINE__ );
                        str[0] = '\0';

                        /*! 当发生协议错误时，打印出堆栈信息*/
                        c_bt( _L );
                    }

                    CHECK_AR( ( *ar_ ) );
                    lua_pushlstring( _L, str, len );
                }
                else {
                    /*! 使用堆内存 */
                    char* str = new char[len+1];
                    memset( str, 0, len );
                    ar_->read_string((char*)str, len+1);
                    if( ar_->check_exception() )
                    {
                        ERR(2)( "[LUAWRAPPER](lar) %s:%d, read_format() [>8192] AR_ERR", __FILE__, __LINE__ );
                        str[0] = '\0';

                        /*! 当发生协议错误时，打印出堆栈信息*/
                        c_bt( _L );
                    }

                    CHECK_AR( ( *ar_ ) );
                    lua_pushlstring( _L, str, len );
                    ERR(2)( "[LUAWRAPPER](lar) %s:%d, read_string() [>8192] AR_ERR, new len: %d", __FILE__, __LINE__, len );
                    delete[] str;
                }
            }
            break;
            default: ERR(2)("[LUAWRAPPER] LAr::read_format() error, param[%d]=%c !!!", i+1, format_str[i]);  c_traceback( _L ); return 0;
        }
    }
    return len;
}

int LAr::read_var_array_short( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	assert( ar_ );
	unsigned char n;
	(*ar_) >> n;
    unsigned int num4 = (unsigned int)(n/4.0f)*4;
    for( unsigned int i=1; i<num4+1; ){
        char mask;
        (*ar_) >> mask;
        char m1 = (mask >> 6) & 0x3;
        char m2 = (mask >> 4) & 0x3;
        char m3 = (mask >> 2) & 0x3;
        char m4 = mask & 0x3;

        if( !m1 ){ lua_pushnumber( _L, 0 ); }
        else if( !(m1&0x2) ){ char data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        else if( !(m1&0x1) ){ short data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        else{ int32_t data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        lua_rawseti( _L, 1, i++ );
       
        if( !m2 ){ lua_pushnumber( _L, 0 ); }
        else if( !(m2&0x2) ){ char data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        else if( !(m2&0x1) ){ short data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        else{ int32_t data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        lua_rawseti( _L, 1, i++ );

        if( !m3 ){ lua_pushnumber( _L, 0 ); }
        else if( !(m3&0x2) ){ char data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        else if( !(m3&0x1) ){ short data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        else{ int32_t data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        lua_rawseti( _L, 1, i++ );

        if( !m4 ){ lua_pushnumber( _L, 0 ); }
        else if( !(m4&0x2) ){ char data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        else if( !(m4&0x1) ){ short data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        else{ int32_t data; (*ar_)>>data; lua_pushnumber( _L, data ); }
        lua_rawseti( _L, 1, i++ );
    }
    if( num4 < n ){
        char mask;
        (*ar_) >> mask;
        for( int i=num4+1; i<n+1; i++ ){
            int o = 8-(i-num4)*2;
            char m = (mask >> o) & 0x3;
            if( !m ){ lua_pushnumber( _L, 0 ); }
            else if( !(m&0x2) ){ char data; (*ar_)>>data; lua_pushnumber( _L, data ); }
            else if( !(m&0x1) ){ short data; (*ar_)>>data; lua_pushnumber( _L, data ); }
            else{ int32_t data; (*ar_)>>data; lua_pushnumber( _L, data ); }
            lua_rawseti( _L, 1, i );
        }
    }
    return 1;
}

int LAr::read_int_degree( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
    int degree = ar_->read_int_degree();
	lua_pushnumber( _L, degree );
    return 1;
}

int LAr::read_compress_pos_angle( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	assert( ar_ );
    float x, y, angle;
    ar_->read_compress_pos_angle( x, y, angle );
	lua_pushnumber( _L, x );
	lua_pushnumber( _L, y );
	lua_pushnumber( _L, angle );
    return 3;
}

int LAr::check_ar( lua_State* _L )
{
	lcheck_argc( _L, 0 )
	assert( ar_ );
    bool tmp_val = ar_->check_exception();
    if ( tmp_val ) {
        c_bt( _L );
    }
	lua_pushboolean( _L, tmp_val );

	return 1;
}

int LAr::read_lar( lua_State* _L )
{
    int len = 0;
    (*ar_) >> len;

    if ( len <= 0 ) {
        // return nil 
        return 0;
    }

	if( ar_->buf_cur_ + len > ar_->buf_max_ )
	{
		ar_->set_exception();
        ERR(2)( "[LUAWRAPPER](lar) %s:%d, read_lar() AR_ERR", __FILE__, __LINE__ );
        /*! 当发生协议错误时，打印出堆栈信息*/
        c_bt( _L );
        return 0;
	}
    char* buffer = ar_->buf_cur_;
    ar_->buf_cur_ += len;

    LAr* lar = new LAr( buffer , len );
    Lunar<LAr>::push( _L, lar, true );        // push the ar
    return 1;
}



/*****************************************************************************
* misc functions
******************************************************************************/
int LAr::print( lua_State* _L )
{
	int size = 0;
	char* buff = ar_->get_buffer( &size );

	for( int i = 0; i < size; ++i )
	{
		fprintf( stdout, "%c", (char)(buff[i]) );
	}
	fprintf( stdout, "\n" );
	
	for( int i = 0; i < size; ++i )
	{
		fprintf( stdout, "%x", (char)(buff[i]) );
	}
	fprintf( stdout, "\n" );
	return 0;
}

int LAr::get_this( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushlightuserdata( _L, this );
	return 1;
}

/*! 注意: flush_before_send必须与send_one_ar配套使用!!!
 * 其与before_send和send()的不同点在于使用全局 lar
 * */
int LAr::flush_before_send( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	assert( ar_ );
	unsigned short pack_type = (unsigned short)lua_tonumber( _L, -1 );
	if( pack_type == 0 )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d invalid pack_type", __FILE__, __LINE__ );
	}
    flush_flag_ = 1;
    ar_->flush();
	(*ar_) << pack_type;
	return 0;
}

int LAr::flush_before_send_one_byte( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	assert( ar_ );
	unsigned char pack_type = (unsigned char)lua_tonumber( _L, -1 );
	if( pack_type == 0 )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d invalid pack_type", __FILE__, __LINE__ );
	}
    flush_flag_ = 1;
    ar_->flush();
	(*ar_) << pack_type;
	return 0;
}

int LAr::send_one_ar( lua_State* _L )
{
	lcheck_argc( _L, 2 );

    FF_Network::NetMng* nm = Lunar<FF_Network::NetMng>::nocheck( _L, 1 );
	if( !nm )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d lua_topointer is null !!!!!!!!!!", __FILE__, __LINE__ );
		return 0;
	}
    if( flush_flag_ != 1 ) {
		ERR(2)( "[LUAWRAPPER](lar) %s:%d flush flag not set, this is very dangerous!!!! pls concat: sodme !!!! ", __FILE__, __LINE__ );
		return 0;
    }

	DPID idto = (DPID)lua_tonumber( _L, 2 );

	int buff_size = 0;
	char* lp_buf = ar_->get_buffer( &buff_size );
	nm->send_msg( lp_buf, buff_size, idto );

	//OutPutAscii( lp_buf, buff_size );
	//destroy();
	return 0;
}

int LAr::flush_send_one_packet( lua_State* _L )
{
	lcheck_argc( _L, 3 );

    flush_flag_ = 1;
    ar_->flush();

    FF_Network::NetMng* nm = Lunar<FF_Network::NetMng>::nocheck( _L, 1 );
	if( !nm )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d lua_topointer is null !!!!!!!!!!", __FILE__, __LINE__ );
		return 0;
	}
    if( flush_flag_ != 1 ) {
		ERR(2)( "[LUAWRAPPER](lar) %s:%d flush flag not set, this is very dangerous!!!! pls concat: sodme !!!! ", __FILE__, __LINE__ );
		return 0;
    }

	DPID idto = (DPID)lua_tonumber( _L, 2 );

    size_t buff_size = 0;
    const char* lp_buf = lua_tolstring( _L, 3, &buff_size );
    if ( buff_size == 0 )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d buffer_size error:%d !!!!!!!!!!", __FILE__, __LINE__, buff_size );
		return 0;
	}

	nm->send_msg( lp_buf, (int)buff_size, idto );

	//OutPutAscii( lp_buf, buff_size );
	//destroy();
	return 0;
}

/*! 注意：before_send必须与send配套使用
 *  其与flush_before_send的区别在于使用单一新new的lar
 *  这会导致性能低下
 * */
int LAr::before_send( lua_State* _L )
{
	lcheck_argc( _L, 1 );
	assert( ar_ );
	unsigned short pack_type = (unsigned short)lua_tonumber( _L, -1 );
	if( pack_type == 0 )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d invalid pack_type", __FILE__, __LINE__ );
	}
    flush_flag_ = 0;
	(*ar_) << pack_type;
	return 0;
}

int LAr::send( lua_State* _L )
{
	lcheck_argc( _L, 2 );

    FF_Network::NetMng* nm = Lunar<FF_Network::NetMng>::nocheck( _L, 1 );
	if( !nm )
	{
		ERR(2)( "[LUAWRAPPER](lar) %s:%d lua_topointer is null", __FILE__, __LINE__ );
		return 0;
	}

	DPID idto = (DPID)lua_tonumber( _L, 2 );

	int buff_size = 0;
	char* lp_buf = ar_->get_buffer( &buff_size );
	nm->send_msg( lp_buf, buff_size, idto );

	//OutPutAscii( lp_buf, buff_size );
	destroy();
	return 0;
}

int LAr::is_loading( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	lua_pushboolean( _L, ar_->is_loading() );
	return 1;
}

int LAr::is_storing( lua_State* _L )
{	
	lcheck_argc( _L, 0 );
	lua_pushboolean( _L, ar_->is_storing() );
	return 1;
}

int LAr::get_offset( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	u_long offset = ar_->get_offset();
	lua_pushnumber( _L, offset );
	return 1;
}

int LAr::write_int_at( lua_State* _L )
{
	lcheck_argc( _L, 2 );
	int num = (int)lua_tonumber( _L, 1 );
	u_long offset = (u_long)lua_tonumber( _L, 2 );
	
	int bufsize;
	char* buf = ar_->get_buffer( &bufsize );
	*(int*)( buf + offset )	= num;
	return 0;
}


int LAr::write_byte_at( lua_State* _L )
{
	lcheck_argc( _L, 2 );
	unsigned char num = (unsigned char)lua_tonumber( _L, 1 );
	u_long offset = (u_long)lua_tonumber( _L, 2 );
	
	int bufsize;
	char* buf = ar_->get_buffer( &bufsize );
	*(unsigned char*)( buf + offset )	= num;
	return 0;
}

int LAr::c_delete( lua_State* _L )
{
	lcheck_argc( _L, 0 );
	destroy();
	return 0;
}

int LAr::reuse( lua_State* _L )
{
	lcheck_argc( _L, 1 );
    size_t bufsize = 0;
    const char* buf = lua_tolstring( _L, 1, &bufsize);
    if ( buf ){
        reuse( buf, bufsize );
    }else{
        reuse( "", 0 );
    }
    return 0;
}

int LAr::get_buffer( lua_State* _L )
{
	lcheck_argc( _L, 0 );
    int bufsize;
    char* buf = ar_->get_buffer( &bufsize );
    lua_pushlstring( _L, buf, bufsize );
    return 1;
}

int LAr::bpack( lua_State* L )
{
    int i,j;
    const char *s=luaL_checkstring(L,1);
    if( !s ) {
        ERR(0)( "[AR] bpack error, s == NULL" );
        return 0;
    }

    for (i=0, j=2; s[i]; i++, j++)
    {
        int c=s[i];
        switch (c)
        {
            case 'A':
                {
                    size_t l;
                    const char *a=luaL_checklstring(L,j,&l);
                    unsigned short len = (unsigned short)l;
                    ar_->write( &len, sizeof(len ) );
                    ar_->write( a, l );
                    break;
                }
            case 'd':
                {
                    double a=(double)luaL_checknumber(L,j);
                    ar_->write( &a, sizeof(a) );
                    break;
                }
            case 'f':
                {
                    float a=(float)luaL_checknumber(L,j);
                    ar_->write( &a, sizeof(a) );
                    break;
                }
            case 'c':
                {
                    char a=(char)luaL_checknumber(L,j);
                    ar_->write( &a, sizeof(a) );
                    break;
                }
            case 'C':
                {
                    unsigned char a=(unsigned char)luaL_checknumber(L,j);
                    ar_->write( &a, sizeof(a) );
                    break;
                }
            case 's':
                {
                    short a=(short)luaL_checknumber(L,j);
                    ar_->write( &a, sizeof(a) );
                    break;
                }
            case 'S':
                {
                    unsigned short a=(unsigned short)luaL_checknumber(L,j);
                    ar_->write( &a, sizeof(a) );
                    break;
                }
            case 'i':
                {
                    int a=(int)luaL_checknumber(L,j);
                    ar_->write( &a, sizeof(a) );
                    break;
                }
            case 'I':
                {
                    unsigned int a=(unsigned int)luaL_checknumber(L,j);
                    ar_->write( &a, sizeof(a) );
                    break;
                }
            case 'l':
                {
                    long a=(long)luaL_checknumber(L,j);
                    ar_->write( &a, sizeof(a) );
                    break;
                }
            case 'L':
                {
                    unsigned long a=(unsigned long)luaL_checknumber(L,j);
                    ar_->write( &a, sizeof(a) );
                    break;
                }
            case '_':
                {
                    j--;
                    break;
                }
            default:
                ERR(0)( "[AR] write format = %s", s);
                break;
        }
    }
    return 0;
}


int LAr::upack( lua_State* L )
{
    const char *f=luaL_checkstring(L,1);
    char* cur = ar_->buf_cur_;
    char* end = ar_->buf_max_;
    int n = 0;

    if( !f ) {
        ERR(0)( "[AR] uppack error, f == NULL " );
        return 0;
    }

    int i = 0;
    for (n=0; f[n]; n++)
    {
        int c=f[n];
        switch (c)
        {
            case 'A':
                {
                    unsigned short l;
                    if( cur + sizeof(l) <= end ) {
                        memcpy( &l, cur, sizeof(l) );
                        cur += sizeof(l);
                        if( cur + l < end ) {
                            lua_pushlstring(L,cur,l);
                            i++;
                            cur += l;
                        }
                    }
                    break;
                }
            case 'd':
                {
                    double a = 0;
                    if( cur + sizeof(a) <= end ) {
                        memcpy( &a, cur, sizeof(a) );
                        cur += sizeof(a);
                    }
                    lua_pushnumber(L,a);
                    i++;
                    break;
                }
            case 'f':
                {
                    float a = 0.0f;
                    if( cur + sizeof(a) <= end ) {
                        memcpy( &a, cur, sizeof(a) );
                        cur += sizeof(a);
                    }
                    lua_pushnumber(L,a);
                    i++;
                    break;
                }
            case 'c':
                {
                    char a = 0;
                    if( cur + sizeof(a) <= end ) {
                        memcpy( &a, cur, sizeof(a) );
                        cur += sizeof(a);
                    }
                    lua_pushnumber(L,a);
                    i++;
                    break;
                }
            case 'C':
                {
                    unsigned char a = 0;
                    if( cur + sizeof(a) <= end ) {
                        memcpy( &a, cur, sizeof(a) );
                        cur += sizeof(a);
                    }
                    lua_pushnumber(L,a);
                    i++;
                    break;
                }
            case 's':
                {
                    short a = 0;
                    if( cur + sizeof(a) <= end ) {
                        memcpy( &a, cur, sizeof(a) );
                        cur += sizeof(a);
                    }
                    lua_pushnumber(L,a);
                    i++;
                    break;
                }
            case 'S':
                {
                    unsigned short a = 0;
                    if( cur + sizeof(a) <= end ) {
                        memcpy( &a, cur, sizeof(a) );
                        cur += sizeof(a);
                    }
                    lua_pushnumber(L,a);
                    i++;
                    break;
                }
            case 'i':
                {
                    int a = 0;
                    if( cur + sizeof(a) <= end ) {
                        memcpy( &a, cur, sizeof(a) );
                        cur += sizeof(a);
                    }
                    lua_pushnumber(L,a);
                    i++;
                    break;
                }
            case 'I':
                {
                    unsigned int a = 0;
                    if( cur + sizeof(a) <= end ) {
                        memcpy( &a, cur, sizeof(a) );
                        cur += sizeof(a);
                    }
                    lua_pushnumber(L,a);
                    i++;
                    break;
                }
            case 'l':
                {
                    long a = 0;
                    if( cur + sizeof(a) <= end ) {
                        memcpy( &a, cur, sizeof(a) );
                        cur += sizeof(a);
                    }
                    lua_pushnumber(L,a);
                    i++;
                    break;
                }
            case 'L':
                {
                    unsigned long a = 0;
                    if( cur + sizeof(a) <= end ) {
                        memcpy( &a, cur, sizeof(a) );
                        cur += sizeof(a);
                    }
                    lua_pushnumber(L,a);
                    i++;
                    break;
                }   
            case '_':
                break;
            default:
                ERR(0)( "[AR] read format = %s", f);
                break;
        }
    }

    ar_->buf_cur_ = cur; 
    return i;
}

int LAr::read_buffer( lua_State* _L )
{
    int len = 0;
    (*ar_) >> len;

    if ( len <= 0 ) {
        // return nil 
        return 0;
    }

    if( ar_->buf_cur_ + len > ar_->buf_max_ )
    {
        ar_->set_exception();
        ERR(2)( "[LUAWRAPPER](lar) %s:%d, read_buffer() AR_ERR", __FILE__, __LINE__ );
        /*! 当发生协议错误时，打印出堆栈信息*/
        c_bt( _L );
        return 0;
    }

    if( len < 8192 )
    {
        char str[8192];
        ar_->read( str, len );
        lua_pushlstring( _L, str, len );
    }
    else
    {
        char* str = new char[len];
        ar_->read( str, len );
        lua_pushlstring( _L, str, len );
        delete[] str;
    }

    return 1;
}



