#include <stdlib.h>
#include "mystring.h"
#include "log.h"

MyStringBuffer::MyStringBuffer()
{
	buf_ = buffer_;
	max_size_ = MAX_STRING_LEN;
}

MyStringBuffer::~MyStringBuffer()
{
	if( buf_ != buffer_ )
	{
        delete []buf_;
	}
}

MyStringBuffer::MyStringBuffer( int _size )
{
	if( _size <= MAX_STRING_LEN )
	{
		buf_ = buffer_;
		max_size_ = MAX_STRING_LEN;
	}
	else
	{
        buf_ = new char[ _size ];
		if( !buf_ )
		{
			LOG(2)("[MYSTRING](construct) MyStringBuffer::MyStringBuffer() fail to construct MyStringBuffer object, perhaps is because of memory overflow.");
			exit(0);
		}
		max_size_ = _size;
	}
}

MyString::MyString()
{
	buf_ = new MyStringBuffer;
	buf_->buf_[ 0 ] = 0;
	len_ = 0;
}

MyString::MyString( const char* _str )
{
	buf_ = new MyStringBuffer;
	len_ = strlen( _str );
	check_buf( len_ + 1 );
	memcpy( buf_->buf_, _str, len_ + 1 );
}

MyString::MyString( const char* _str, int len )
{
	buf_ = new MyStringBuffer;
	len_ = len;
	check_buf( len_ + 1 );
	memcpy( buf_->buf_, _str, len_ );
    buf_->buf_[len_] = 0;
}

MyString::MyString( const MyString& _str )
{
	buf_ = new MyStringBuffer;
	len_ = _str.len_;
	check_buf( len_ + 1 );
	memcpy( buf_->buf_, _str.buf_->buf_, len_ + 1 );
}

MyString::~MyString()
{
	delete buf_;
}
