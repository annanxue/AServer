#ifndef __BUFFER_H__
#define	__BUFFER_H__

#ifdef __linux
#include <netinet/in.h>
#elif defined(_WIN32)
#include <winsock.h>
#endif
#include "autolock.h"
//#include "buddy.h"
#include "lj_alloc.h"
#include "mylist.h"

typedef unsigned long			ff_header_t;

#define HEADER_SIZE				( sizeof(ff_header_t) )
#define MAX_BUFFER			    2048	

typedef unsigned long			DPID;

#define DPID_UNKNOWN			(DPID)0xffffffff
#define DPID_SERVER_PLAYER		(DPID)0
#define DPID_ALL_PLAYER			(DPID)0xfffffffe

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif

#ifndef LITTLE_ENDIAN
#define HTONL(x)				htonl(x)
#define HTONS(x)				htons(x)
#define NTOHL(x)				ntohl(x)
#define NTOHS(x)				ntohs(x)
#else
#define HTONL(x)				(x)
#define HTONS(x)				(x)
#define NTOHL(x)				(x)
#define NTOHS(x)				(x)
#endif


/**********************************************************************************

 Buffer

 *********************************************************************************/

class LJEnv
{
    public:
        LJEnv() { lj_ud_ = lj_alloc_create(); }
        ~LJEnv() { lj_alloc_destroy( lj_ud_ ); }
    public:
        void* lj_alloc( size_t _size ) { return lj_alloc_f( lj_ud_, NULL, 0, _size ); }
        void* lj_free( void* _fptr ) { return lj_alloc_f( lj_ud_, _fptr, 0, 0 ); }
        void* get_lj_ud() { return lj_ud_; }
    private:
        void* lj_ud_;
};

class Buffer
{
public:
	DPID		dpid_;				/*!< which socket the buffer receive from, when DPSock  worker thread waked 
									up by Clientsock, it will set value to this member */
	size_t		cb_;					/*!< the packet number in the buffer, added in CClientSock::Fetch func*/
	//char		buffer_[MAX_BUFFER];
	char*		buf_start_;		/*!<缓冲区起始位置*/
	char*		buf_max_;		/*!<缓冲区结束位置*/
	char*		head_;			/*!<缓冲区可读数据的起始位置*/
	char*		tail_;			/*!<缓冲区可写数据的起始位置*/
	int			crypted_;		/*!<缓冲区已加密的标志*/

    list_head   link_;

    static      Mutex mtx_;
    static      int count_;
    static      int max_num_;

public:
	Buffer( size_t buf_size );
	virtual ~Buffer();

	int		get_size( void ) const;		/*!< 得到缓冲区的容量 */
	char*	get_writable_buffer( int* buf_size ) const;
	int		get_writable_buffer_size( void ) const;
	char*	get_readable_buffer( int* buf_size ) const;
	int		get_readable_buffer_size( void ) const;

	ff_header_t get_packet_size( char* ptr ) const
	{
		return HTONL( *(ff_header_t*)ptr );
	}

	void set_header( ff_header_t data_size )
	{
		*(ff_header_t*)tail_ = HTONL( (ff_header_t)(data_size + HEADER_SIZE) );
		tail_ += HEADER_SIZE;
	}
public:
    //static Buddy buddy_;
    static LJEnv& get_lj_ev() 
    { 
        static LJEnv g_lj_ev;
        return g_lj_ev;
    }  
};


inline int Buffer::get_size( void ) const
{	
	return (int)( buf_max_ - buf_start_ );	
}

inline char* Buffer::get_writable_buffer( int* buf_size ) const
{	
	*buf_size	= (int)( buf_max_ - tail_ );	
	return tail_;	
}

inline int Buffer::get_writable_buffer_size( void ) const
{	
	return (int)( buf_max_ - tail_ );	
}

inline char* Buffer::get_readable_buffer( int* buf_size ) const
{	
	*buf_size	= (int)( tail_ - head_ );	
	return head_;	
}

inline int Buffer::get_readable_buffer_size( void ) const
{	
	return (int)( tail_ - head_ );	
}


/**********************************************************************************

 BufferQueue

 *********************************************************************************/
class BufferQueue
{
public:
	Mutex mtx_;

	BufferQueue();
	~BufferQueue();
	
	void		add_tail( Buffer* buf );
	void		clear( bool del );

public:
    list_head   link_;
};




inline void BufferQueue::add_tail( Buffer* buf )
{	
	AutoLock lock(&mtx_);
    list_add_tail( &(buf->link_), &link_ );
}




/**********************************************************************************

 BufferFactory

 *********************************************************************************/
class BufferFactory
{
public:
	Buffer* create_buffer( size_t buf_size = MAX_BUFFER );
	BufferQueue* create_bufque();
	static BufferFactory& get_instance();

private:
	BufferFactory();
};

#endif	






