#ifndef __MYSTRING_H__
#define __MYSTRING_H__

#include "misc.h"

#define MAX_STRING_LEN 16384
//#define MAX_STRING_LEN 8192

class MyStringBuffer
{
public:	
	MyStringBuffer();
	~MyStringBuffer();
	MyStringBuffer( int _size );
	char* buf_;
	int max_size_;
private:
	char buffer_[ MAX_STRING_LEN ];
};

class MyString
{
public:
	MyString();
	MyString( const char* _str );
	MyString( const char* _str, int len );
	MyString( const MyString& _str );
	~MyString();
	inline void check_buf( int _size )
	{
		if( _size > buf_->max_size_ )
		{
			int new_size = MAX_STRING_LEN * ( _size / MAX_STRING_LEN + 1 );
			MyStringBuffer* new_buf = new MyStringBuffer( new_size );
			memcpy( new_buf->buf_, buf_->buf_, len_ + 1 );
			delete buf_;
			buf_ = new_buf;
		}
	}
	inline char* c_str()
	{
		return buf_->buf_;
	}
	inline void set_str( const char* _str )
	{
		len_ = strlen( _str );
		check_buf( len_ + 1 );
		memcpy( buf_->buf_, _str, len_ );
        buf_->buf_[len_] = 0;
	}
	inline void cat_str( const char* _str, int _len )
	{
		check_buf( len_ + _len + 1 );
		char* np = buf_->buf_ + len_;
		memcpy( np, _str, _len );
        np[_len] = 0;
		len_ += _len;
	}
	inline int length()
	{
		return len_;
	}
	inline void clear()
	{
		buf_->buf_[0] = 0;
		len_ = 0;
	}
private:
	MyStringBuffer* buf_;
	int len_;
};

#endif
