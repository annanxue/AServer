#include <assert.h>
#include "ar.h"
#include "misc.h"

Ar::Ar( const void* buf, u_int buf_size)
{
	if( buf )
	{
		mode_ = load;
		buf_start_ = (char*)buf;
		buf_size_ = buf_size;
	}
	else
	{
		mode_ = store;
		buffer_ = BufferFactory::get_instance().create_buffer();
		buf_start_ = buffer_->get_writable_buffer( &buf_size_ );
	}
	
	buf_max_ = buf_start_ + buf_size_;
	buf_cur_ = buf_start_;
	clear_exception();
}

Ar::~Ar()
{
	if( is_storing() )
	{
		SAFE_DELETE(buffer_);
	}
}


void Ar::read( void *buf, int buf_size )
{
	assert( is_loading() );
	assert( buf );
	assert( buf_size >=0 );

	if( buf_cur_ + buf_size > buf_max_ )
	{
		LOG(2)("[AR_ERR](read) error occur when read :m_lpBufCur + nBufSize[%lu] > m_lpBufMax.", buf_size);
		set_exception();
		buf_cur_ = buf_max_;
		return;
	}
	
	memcpy( buf, buf_cur_, buf_size );
	buf_cur_ += buf_size;
}

char* Ar::read_string( char* buf, int buf_size )
{
	assert( is_loading() );
	assert( buf );
	assert( buf_size>0 );
	
	u_short len;
	*this >> len;

    /*
	if( (len >= buf_size) || (buf_cur_+len > buf_max_) )
	{
		LOG(2)("[AR_ERR][read string] error occur when read string: nLen [%d], nBufSize [%d]", len, buf_size);
		buf_cur_ = buf_max_;
		set_exception();
		buf[buf_size - 1] = '\0';
		return buf;
	}*/

    if ( buf_cur_+len > buf_max_) 
	{
		LOG(2)("[AR_ERR][read string] buf_cur_+len > buf_max_, error occur when read string: nLen [%d], nBufSize [%d], buf_cur_: %d, buf_max_: %d", len, buf_size, buf_cur_, buf_max_ );
		buf_cur_ = buf_max_;
		set_exception();
		buf[buf_size - 1] = '\0';
		return buf;
	}

	if(len >= buf_size) {
		LOG(2)("[AR_ERR][read string] len>=buf_size, error occur when read string: nLen [%d], nBufSize [%d]", len, buf_size);
		buf_cur_ = buf_cur_ + len;
		set_exception();
		buf[buf_size - 1] = '\0';
		return buf;
    }

    /*
    if( len>8192 ){
        ERR(2)("[AR_ERR][read string] more 8192,  error occur when read string: nLen [%d], nBufSize [%d]", len, buf_size);
    }
    */
	
	memcpy( buf,  buf_cur_, len );
	buf[len] = '\0';
	buf_cur_ += len;
	
	return buf;	
}

void Ar::write( const void* buf, int buf_size )
{
	assert( is_storing() );
	assert( buf );
	assert( buf_size>=0 );
	
	if( check_buf( buf_size ) ) return;
	memcpy( buf_cur_, buf, buf_size );
	buf_cur_ += buf_size;	
}


void Ar::write_string(const char *buf, int buf_size)
{
	assert( buf );
	
	u_short len = strlen( buf );

	if ( len > buf_size )
		len = buf_size;

	*this << len;
	write( buf, sizeof(char) * len );
}


void Ar::write_string(const char *buf )
{
	assert( buf );
	
	u_short len = strlen( buf );

	*this << len;
	write( buf, sizeof(char) * len );
}


int Ar::check_buf( int size )
{
    assert( size>=0 );

    if( buf_cur_ + size < buf_max_ ) {
        return 0;
    }

    u_int offset = buf_cur_ - buf_start_;
    u_int extension = MAX_BUFFER * (size / MAX_BUFFER + 1 );
    u_int  tmp_size = (buf_max_ - buf_start_) + extension;
    Buffer* new_buf = BufferFactory::get_instance().create_buffer( tmp_size );
    if( new_buf == NULL ) {
		ERR(2)("[AR](ar) Ar::check_buf(), BUFFER_ERROR, packet_size = 0x%08X, dpid = 0x%08X", tmp_size, buffer_->dpid_ );
        return -1;
    }

    memcpy( new_buf->buf_start_, buffer_->buf_start_, offset );
    new_buf->dpid_ = buffer_->dpid_;

    delete buffer_;

    buffer_ = new_buf;
    buf_start_  = buffer_->buf_start_;
    buf_max_    = buffer_->buf_start_ + tmp_size;
    buf_cur_    = buffer_->buf_start_ + offset;

    buf_size_ = buf_max_ - buf_cur_;

    return 0;
}

char* Ar::get_buffer( int* buf_size )
{
	assert( is_storing() );
	assert( buf_size );
	
	*buf_size = buf_cur_ - buf_start_;
	return buf_start_;
}

void Ar::write_int_at( int _num, u_long _offset )
{	
	int bufsize;
	char* buf = get_buffer( &bufsize );
	*(int*)( buf + _offset )	= _num;
}

void Ar::write_byte_at( unsigned char _num, u_long _offset )
{
	int bufsize;
	char* buf = get_buffer( &bufsize );
	*(unsigned char*)( buf + _offset ) = _num;
}

void Ar::flush( void )
{
	assert( is_storing() );
	
	buf_cur_ = buf_start_;
}

void Ar::reel_in( unsigned int offset )
{
	assert ( is_storing() ) ;

	if( buf_start_ + offset > buf_cur_ ) 
	{
		LOG(2)("[AR_ERR](reelin) error occur when reelin: m_lpBufStart + uOffset[%lu] > m_lpBufCur.", offset);
		return;
	}

	buf_cur_	= buf_start_ + offset;
}

int Ar::peek_int()
{
	if( buf_max_ - buf_cur_ < 4 ) return -1;
	int value = *(int *)buf_cur_;
	
	return value;
}

unsigned short Ar::peek_short()
{
	if( buf_max_ - buf_cur_ < 2 ) return 0;
	unsigned short value = *(unsigned short *)buf_cur_;
	
	return value;
}

void Ar::recreate_buffer()
{
	buffer_ = BufferFactory::get_instance().create_buffer( MAX_BUFFER );
    buf_start_  = buffer_->buf_start_;
	buf_cur_    = buffer_->buf_start_;
	buf_max_    = buffer_->buf_start_ + MAX_BUFFER;

    buf_size_ = buf_max_ - buf_cur_;
}

void Ar::write_int_degree( int degree )
{
    if( degree < 0 )
    {
        degree = ( degree % 360 ) + 360;
    }
    degree = degree % 360;
    if( degree >= 0x80 )
    {
        unsigned char first_byte = (degree >> 8) | 0x80;
        (*this) << first_byte;
        unsigned char second_byte = degree & 0x000000ff;
        (*this) << second_byte;
    }
    else
    {
        (*this) << (unsigned char)degree;
    }
}

void Ar::write_compress_pos_angle(float x, float y, float angle)
{
    int ix = x * 10;
    int iy = y * 10;
    int iangle = angle * 10;
    unsigned char mask = 0;
    int offset = this->get_offset();
    (*this) << mask;

    if( ix >= 0x10000 ) { 
        mask |= 0xc0; 
        (*this) << ix;
    }else if( ix >= 0x100 ) {
        mask |= 0x80; 
        (*this) << (unsigned short)ix;
    }else {
        mask |= 0x40;
        (*this) << (unsigned char)ix;
    }

    if( iy >= 0x10000 ) { 
        mask |= 0x30; 
        (*this) << iy;
    }else if( iy >= 0x100 ) {
        mask |= 0x20; 
        (*this) << (unsigned short)iy;
    }else {
        mask |= 0x10;
        (*this) << (unsigned char)iy;
    }

    mask |= ( (iangle >> 8) & 0x0f );
    (*this) << (unsigned char)(iangle & 0x000000ff);

    this->write_byte_at( mask, offset );
}

void Ar::read_compress_pos_angle(float& x, float& y, float& angle)
{
    unsigned char mask;
    (*this) >> mask;    

    int ix = 0;
    int iy = 0;
    int iangle = 0;

    unsigned char tmask = mask & 0xc0;
    if( tmask == 0xc0 ) {
        (*this) >> ix;
    } else if( tmask == 0x80 ) {
        unsigned short tmp;
        (*this) >> tmp;
        ix = tmp;
    }
    else {
        unsigned char tmp;
        (*this) >> tmp;
        ix = tmp;
    }

    tmask = mask & 0x30;
    if( tmask == 0x30 ) {
        (*this) >> iy;
    } else if( tmask == 0x20 ) {
        unsigned short tmp;
        (*this) >> tmp;
        iy = tmp;
    }
    else {
        unsigned char tmp;
        (*this) >> tmp;
        iy = tmp;
    }

    tmask = mask & 0x0f;
    unsigned char bangle;
    (*this) >> bangle;

    iangle = bangle | (tmask << 8);

    x = ix * 0.1f;
    y = iy * 0.1f;
    angle = iangle * 0.1f;
}

int Ar::read_int_degree()
{
    unsigned char first_byte, second_byte;
    int degree = 0;
    (*this) >> first_byte;

    if(first_byte & 0x80)
    {
        degree |= (first_byte & 0x01) << 8;
        (*this) >> second_byte;
        degree |= second_byte;
    }
    else
    {
        degree = first_byte;
    }
    return degree;
}

int c_read_var_array_short( Ar & ar, int* array ) 
{ 
    unsigned char n;
    ar >> n;
    unsigned int num4 = (unsigned int)(n/4.0f)*4;
    
    for( unsigned int i=0; i<num4; ){ 
        char mask;  
        (ar) >> mask; 
        char m1 = (mask >> 6) & 0x3;    
        char m2 = (mask >> 4) & 0x3;    
        char m3 = (mask >> 2) & 0x3;    
        char m4 = mask & 0x3;   
        if( !m1 ){ (array)[i++] = 0; }    
        else if( !(m1&0x2) ){ char data; (ar)>>data; (array)[i++] = (int32_t)data; }    
        else if( !(m1&0x1) ){ short data; (ar)>>data; (array)[i++] = (int32_t)data; }   
        else{ int32_t data; (ar)>>data; (array)[i++] = (int32_t)data; } 
        if( !m2 ){ (array)[i++] = 0; }    
        else if( !(m2&0x2) ){ char data; (ar)>>data; (array)[i++] = (int32_t)data; }    
        else if( !(m2&0x1) ){ short data; (ar)>>data; (array)[i++] = (int32_t)data; }   
        else{ int32_t data; (ar)>>data; (array)[i++] = (int32_t)data; }     
        if( !m3 ){ (array)[i++] = 0; }    
        else if( !(m3&0x2) ){ char data; (ar)>>data; (array)[i++] = (int32_t)data; }    
        else if( !(m3&0x1) ){ short data; (ar)>>data; (array)[i++] = (int32_t)data; }   
        else{ int32_t data; (ar)>>data; (array)[i++] = (int32_t)data; } 
        if( !m4 ){ (array)[i++] = 0; }    
        else if( !(m4&0x2) ){ char data; (ar)>>data; (array)[i++] = (int32_t)data; }    
        else if( !(m4&0x1) ){ short data; (ar)>>data; (array)[i++] = (int32_t)data; }   
        else{ int32_t data; (ar)>>data; (array)[i++] = (int32_t)data; } 
    }   
    if( num4 < n ){ 
        char mask;  
        (ar) >> mask; 
        for( int i=num4; i<n; i++ ){    
            int o = 8-(i-num4)*2;   
            char m = (mask >> o) & 0x3; 
            if( !m ){ (array)[i++] = 0; } 
            else if( !(m&0x2) ){ char data; (ar)>>data; (array)[i++] = (int32_t)data; }     
            else if( !(m&0x1) ){ short data; (ar)>>data; (array)[i++] = (int32_t)data; }    
            else{ int32_t data; (ar)>>data; (array)[i++] = (int32_t)data; } 
        }   
    }
    return 0;
}

int32_t c_write_var_array_short( Ar &ar, int* array, unsigned int n ) 
{   
    assert( (n) <= 255 && (n)>=0 ); 
    (ar) << ( unsigned char )(n);   
    unsigned int num4 = (unsigned int)((n)/4.0f)*4; 
    for( unsigned int i=0; i<num4; ){   
        u_long offset = (ar).get_offset();  
        char mask = 0;  
        (ar) << mask;   
        int32_t data = (int32_t)(array)[i++];   
        int32_t abs = (data & 0x80000000) ? (~data + 1) : data; 
        if( !abs ){}    
        else if ( !(abs & 0xFFFFFF80) ){ mask |= 0x40; (ar)<<(char)data; }   
        else if( !(abs & 0xFFFF8000) ) { mask |= 0x80; (ar)<<(short)data; } 
        else{ mask |= 0xc0; (ar)<<data; }   
        data = (int32_t)(array)[i++];   
        abs = (data & 0x80000000) ? (~data + 1) : data; 
        if( !abs ){}    
        else if ( !(abs & 0xFFFFFF80) ){ mask |= 0x10; (ar)<<(char)data; }   
        else if( !(abs & 0xFFFF8000) ) { mask |= 0x20; (ar)<<(short)data; } 
        else{ mask |= 0x30; (ar)<<data; }   
        data = (int32_t)(array)[i++];   
        abs = (data & 0x80000000) ? (~data + 1) : data; 
        if( !abs ){}    
        else if ( !(abs & 0xFFFFFF80) ){ mask |= 0x04; (ar)<<(char)data; }   
        else if( !(abs & 0xFFFF8000) ) { mask |= 0x08; (ar)<<(short)data; } 
        else{ mask |= 0x0c; (ar)<<data; }   
        data = (int32_t)(array)[i++];   
        abs = (data & 0x80000000) ? (~data + 1) : data; 
        if( !abs ){}    
        else if ( !(abs & 0xFFFFFF80) ){ mask |= 0x01; (ar)<<(char)data; }   
        else if( !(abs & 0xFFFF8000) ) { mask |= 0x02; (ar)<<(short)data; } 
        else{ mask |= 0x03; (ar)<<data; }   
        int bufsize;    
        char* buf = (ar).get_buffer( &bufsize );    
        *(char*)( buf + offset ) = mask;    
    }   
    if( num4 < n ){ 
        u_long offset = (ar).get_offset();  
        char mask = 0;  
        (ar) << mask;   
        for( unsigned int i=num4+1; i<n+1; i++ ){   
            int32_t data = (int32_t)(array)[i]; 
            int32_t abs = (data & 0x80000000) ? (~data + 1) : data; 
            int o = 8-(i-num4)*2;   
            if( !abs ){}    
            else if ( !(abs & 0xFFFFFF80) ){ mask |= (0x01<<o); (ar)<<(char)data; } 
            else if( !(abs & 0xFFFF8000) ) { mask |= (0x02<<o); (ar)<<(short)data; }    
            else{ mask |= (0x03<<o); (ar)<<data; }  
        }   
        int bufsize;    
        char* buf = (ar).get_buffer( &bufsize );    
        *(char*)( buf + offset ) = mask;    
    }   
    return 0;
}
