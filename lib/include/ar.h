#ifndef __AR_H__
#define __AR_H__

#include "buffer.h"
#include "log.h"

using namespace std;

struct _DOUBLE  { char doubleBits[sizeof(double)]; };
struct _float   { char floatBits[sizeof(float)]; };

#define CHECK_AR( _car ) \
{ \
	if( (_car).check_exception() ) \
	{ \
		ERR(2)( "[AR_ERR](CHECK_AR) %s:%d", __FILE__, __LINE__ ); \
	} \
}

#define safe_read_string( _ar, _buf, _buff_size ) \
{ \
	(_ar).read_string( (_buf), (_buff_size) ); \
	if((_ar).check_exception() ) \
	{ \
		ERR(2)( "[AR_ERR](AR_READ_STRING) %s:%d", __FILE__, __LINE__ ); \
	} \
}

class Ar
{
public:
	Ar( const void* buf = NULL, u_int buf_size = 0 );	
	~Ar();
public:
	bool is_loading() const;
	bool is_storing() const;
	
	void read( void * buf, int buf_size);
	void write(const void *buf, int buf_size);
	
	char* read_string(char* buf, int buf_size);
	void write_string(const char *buf, int buf_size);
	void write_string(const char *buf );
	
	void flush(void);
	void reel_in( unsigned int offset );
	int peek_int();
    unsigned short peek_short();
	int check_buf(int size);
	char* get_buffer(int* buf_size);
	u_long get_offset(void);
	void write_int_at( int num, u_long _offset );
	void write_byte_at( unsigned char num, u_long _offset );
	void recreate_buffer();
	
	void clear_exception();
	bool check_exception();
	void set_exception();

    void write_int_degree( int degree);
    int read_int_degree();

    void write_compress_pos_angle(float x, float y, float angle);
    void read_compress_pos_angle(float& x, float& y, float& angle);

public:
	Ar& operator<<(char by);
	Ar& operator<<(unsigned char uc);
	Ar& operator<<(short w);
	Ar& operator<<(unsigned short us);
	Ar& operator<<(long l);
	Ar& operator<<(u_long dw);
	Ar& operator<<(int i);
	Ar& operator<<(unsigned u);
	Ar& operator<<(float f);
	Ar& operator<<(double d);

	Ar& operator>>(char& by);
	Ar& operator>>(unsigned char& uc);
	Ar& operator>>(short& w);
	Ar& operator>>(unsigned short& us);
	Ar& operator>>(u_long& dw);
	Ar& operator>>(long& l);
	Ar& operator>>(int& i);
	Ar& operator>>(unsigned& u);
	Ar& operator>>(float& f);
	Ar& operator>>(double& d);
	
public:
	enum { store =0, load =1 };

	Buffer* buffer_;
	int errno_;

public:
	char* buf_cur_;
	char* buf_max_;

protected:
	char mode_;
	int buf_size_;
	char* buf_start_;

};

inline bool Ar::is_loading() const
{
	return ( mode_ == Ar::load );
}

inline bool Ar::is_storing() const
{
	return ( mode_ == Ar::store );
}

inline Ar& Ar::operator<<(int i)
{
	return Ar::operator<<((long)i);
}

inline Ar& Ar::operator<<(unsigned u)
{
	return Ar::operator<<((long)u);
}

inline Ar& Ar::operator<<(unsigned short us)
{
	return Ar::operator<<((short)us);
}

inline Ar& Ar::operator<<(short w)
{
	if( check_buf( sizeof( short ) ) )
		return *this;
	*(short*)buf_cur_ = HTONS(w); 
	buf_cur_ += sizeof(short); 
	
	return *this; 
}

inline Ar& Ar::operator<<(u_long dw)
{
	return Ar::operator<<((long)dw); 
}

inline Ar& Ar::operator<<(long l)
{
	if( check_buf( sizeof(long) ) )
		return *this;
	*(long*)buf_cur_ = HTONL(l); 
	buf_cur_ += sizeof(long); 

	return *this; 
}

inline Ar& Ar::operator<<(unsigned char uc)
{
	return Ar::operator<<((char)uc); 
}

inline Ar& Ar::operator<<(char ch)
{
	if( check_buf( sizeof(char) ) )
		return *this;
	*buf_cur_ = ch; 
	buf_cur_ += sizeof(char); 
	
	return *this; 
}

inline Ar& Ar::operator<<(float f)
{
	if( check_buf( sizeof(float) ) )
		return *this;
	*(_float *)buf_cur_ = *(_float*)&f; 
	buf_cur_ += sizeof(float); 
	
	return *this; 
}

inline Ar& Ar::operator<<(double d)
{
	if( check_buf( sizeof(double) ) )
		return *this;
	*(_DOUBLE *)buf_cur_ = *(_DOUBLE*)&d; 
	buf_cur_ += sizeof(double); 
	
	return *this; 
}

inline Ar& Ar::operator>>(int& i)
{
	return Ar::operator>>((long&)i);
}

inline Ar& Ar::operator>>(unsigned& u)
{
	return Ar::operator>>((long&)u);
}

inline Ar& Ar::operator>>(u_long& dw)
{
	return Ar::operator>>((long&)dw);
}

inline Ar& Ar::operator>>(unsigned short& us)
{
	return Ar::operator>>((short&)us);
}

inline Ar& Ar::operator>>(short& w)
{
	if( buf_cur_ + sizeof(short) > buf_max_ )
	{
		w = 0;
		buf_cur_ = buf_max_;
		set_exception();
	}
	else
	{
		w = NTOHS(*(short*)buf_cur_); 
		buf_cur_ += sizeof(short); 
	}

	return *this; 
}

inline Ar& Ar::operator>>(unsigned char& uc)
{
	return  Ar::operator>>((char&)uc);
}

inline Ar& Ar::operator>>(char& ch)
{
	if( buf_cur_ + sizeof(char) > buf_max_ )
	{
		ch = 0;
		buf_cur_ = buf_max_;
		set_exception();
	}
	else
	{
		ch = *buf_cur_; 
		buf_cur_ += sizeof(char); 
	}

	return *this; 
}

inline Ar& Ar::operator>>(long& l)
{
	if( buf_cur_ + sizeof(long) > buf_max_ )
	{
		l = 0;
		buf_cur_ = buf_max_;
		set_exception();
	}
	else
	{
		l = NTOHL(*(long*)buf_cur_); 
		buf_cur_ += sizeof(long); 
	}

	return *this; 
}



inline Ar& Ar::operator>>(float& f)
{
	if( buf_cur_ + sizeof(float) > buf_max_ )
	{
		f = 0.0f;
		buf_cur_ = buf_max_;
		set_exception();
	}
	else
	{
		*(_float*)&f = *(_float*)buf_cur_; 
		buf_cur_ += sizeof(float); 
	}

	return *this; 
}

inline Ar& Ar::operator>>(double& d)
{
	if( buf_cur_ + sizeof(double) > buf_max_ )
	{
		d = 0.0f;
		buf_cur_ = buf_max_;
		set_exception();
	}
	else
	{
		*(_DOUBLE*)&d = *(_DOUBLE*)buf_cur_; 
		buf_cur_ += sizeof(double); 
	}

	return *this; 
}

inline u_long Ar::get_offset(void)
{
	if( !is_storing() )
	{
		LOG(2)("[AR_ERR](get offset) error occur when get offset: not in store mode.");
		set_exception();
		return 0;
	}
	
	return ( buf_cur_ - buf_start_ );
}

inline void Ar::clear_exception()
{
	errno_ = 0;
}

inline bool Ar::check_exception()
{
	return (errno_!=0);
}

inline void Ar::set_exception()
{
	errno_ = 1;
}


#include "3dmath.h"
#include "3dtypes.h"

inline Ar& operator<<(Ar & ar, VECTOR3 v)
{	
	ar.write( &v, sizeof(VECTOR3) );	
	return ar;	
}

inline Ar& operator>>(Ar & ar, VECTOR3& v)
{	
	ar.read( &v, sizeof(VECTOR3) );		
	return ar;	
}

inline Ar& operator<<(Ar & ar, unsigned long long i)
{	
	ar.write( &i, sizeof(unsigned long long) );	
	return ar;	
}

inline Ar& operator>>(Ar & ar, unsigned long long& i)
{	
	ar.read( &i, sizeof(unsigned long long) );	
	return ar;	
}


inline Ar& operator<<(Ar & ar, long long i)
{	
	ar.write( &i, sizeof(long long) );	
	return ar;	
}

inline Ar& operator>>(Ar & ar, long long& i)
{	
	ar.read( &i, sizeof(long long) );	
	return ar;	
}


inline Ar& operator<<(Ar & ar, Rect rect)
{	
	ar.write( &rect, sizeof(Rect) );	
	return ar;	
}

inline Ar& operator>>(Ar & ar, Rect & rect)
{	
	ar.read( &rect, sizeof(Rect) );	
	return ar;	
}

int c_read_var_array_short( Ar & ar, int* array );

int32_t c_write_var_array_short( Ar &ar, int* array, unsigned int n );


inline int c_write_var( Ar &ar, int x ){
    int32_t abs = (x & 0x80000000) ? (~x + 1) : x;
    if ( !(abs & 0xFFFFFFE0) ){ ar<<char(x<<2); }
    else if( !(abs & 0xFFFFE000) ) { ar<<short(x<<2 | 0x1); }
    else{ ar<<(char)0x2; ar<<x; }
    return 0;
}

inline int c_write_uvar( Ar &ar, unsigned int x){
    if ( !(x & 0xFFFFFFC0) ){ ar<<char(x << 2); } 
    else if( !(x & 0xFFFFC000) ) { ar<<(short)((x << 2) | 0x1 ); }
    else{ ar<<(char)0x2; ar<<x; } 
    return 0;
}

inline int c_read_var( Ar &ar, int & data ){
    char x;
    ar >> x;
    if( !(x & 0x3) ){ data = x>>2; } 
    else if( !(x & 0x2) ) { char c; ar >> c; data = (c << 6) | ((unsigned char)(x)>>2); }
    else { ar >> data; }
    return 0;
}

inline int c_read_uvar( Ar &ar, unsigned int & data){
    unsigned char x;
    ar >> x;
    if( !(x & 0x3) ){ data = x>>2; }
    else if( !(x & 0x2) ) { unsigned char c; ar >> c; data = (c << 6) | (x>>2); }
    else { ar >> data; }
    return 0;
}
#endif

