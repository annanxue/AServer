#include <string.h>
#include "buffer.h"
#include "log.h"
#include "misc.h"
#include "mylist.h"
#include "commonsocket.h"

int Buffer::count_ = 0;
int Buffer::max_num_ = 0;
Mutex Buffer::mtx_;

/**********************************************************************************

 Buffer

 *********************************************************************************/

Buffer::Buffer( size_t buf_size )
{
#if 0
	if( buf_size > MAX_BUFFER ) /*!< use heap*/
	{  
        /*
		buf_start_ = (char*)malloc( sizeof(char) * buf_size );
		if( !buf_start_ ) {
			ERR(2)("[NET](buf) Buffer::Buffer() fail to construct Buffer object, perhaps is because of memory overflow.");
			exit(0);
		}
        */

        unsigned int i;
        for( i = 0; i < 7; ++i ) {
            if( buf_size <= (MAX_BUFFER) << i ) {
                buf_start_ = (char*)buddy_.get(i);
                break;
            }
        }

	}
	else	/*!< use stack, the length of the array is 4096*/
	{  
		buf_start_	= buffer_;
		buf_size	= MAX_BUFFER;
	}
#endif


    /*
    for(int i = 0; i <= 8; ++i) {
        if(buf_size <= (MAX_BUFFER) << i) {
            buf_start_ = (char*)buddy_.get(i);
            buf_size = (MAX_BUFFER) << i;
            break;
        }
    }
    */
    /*
    mtx_.Lock();
    buf_start_ = (char*)get_lj_ev().lj_alloc( buf_size );
    mtx_.Unlock();
    */

    buf_start_ = (char*)malloc( buf_size );

	head_ = tail_ = buf_start_;
	buf_max_ = buf_start_ + buf_size;
	cb_ = 0;
	dpid_ = DPID_UNKNOWN;

    INIT_LIST_HEAD( &link_ );
	crypted_ = FALSE;

    //mtx_.Lock();
    count_++;
    if( count_ > max_num_ ) max_num_ = count_;
    //mtx_.Unlock();
}



Buffer::~Buffer()
{
#if 0
	/*! free the heap, nothing need to do if use stack*/
	if( buf_start_ != buffer_ )
	{
        /*
		free(buf_start_);
        buf_start_ = NULL;
        */
        buddy_.put( buf_start_ );
	}
#endif
    //buddy_.put( buf_start_ );
    //mtx_.Lock();
    //get_lj_ev().lj_free( buf_start_ );
    free( buf_start_ );
    count_--;
    //mtx_.Unlock();
}





/**********************************************************************************

 BufferQueue

 *********************************************************************************/

BufferQueue::BufferQueue()
{
    INIT_LIST_HEAD( &link_ );
}

BufferQueue::~BufferQueue()
{
	clear( true );
}

void BufferQueue::clear( bool del )
{
    AutoLock lock(&mtx_);
    while( !list_empty( &link_ ) ) {
        list_head* pos = link_.next;
        list_del( pos );
        Buffer* buf = list_entry( pos, Buffer, link_ );
        SAFE_DELETE( buf );
        FF_Network::CommonSocket::send_down();
    }
}



/**********************************************************************************

 BufferFactory

 *********************************************************************************/
BufferFactory::BufferFactory()
{

}


BufferFactory& BufferFactory::get_instance()
{
	static BufferFactory g_factory;
	return g_factory;
}


Buffer* BufferFactory::create_buffer( size_t buf_size )
{
	if( buf_size > 262144 )
	{
		ERR(2)("[NET](buf) BufferFactory::create_buffer() BUFFER_ERROR fail to create buffer, the size is %lu", buf_size);
		return NULL;
	}
	Buffer* buffer = new Buffer( buf_size );
	return buffer;
}


BufferQueue* BufferFactory::create_bufque()
{
	BufferQueue* bufque = new BufferQueue();
	return bufque;
}

