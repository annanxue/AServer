#include "log.h"
#include "misc.h"
#include "commonsocket.h"

#define SEED_HEADER_SIZE	( HEADER_SIZE + sizeof(int) )

namespace FF_Network {

extern int g_network_frm;

int CommonSocket::serial_no_ = 0;
int CommonSocket::send_buf_num = 0;
int CommonSocket::recv_buf_num = 0;
Mutex CommonSocket::send_lock;
Mutex CommonSocket::recv_lock;

CommonSocket::CommonSocket(int sock, bool crypt):Socket(sock),need_crypt_(crypt)
{
	dpid_ = ((serial_no_++) << 16) | (Handle() & 0x0000FFFF);
	init();
}

CommonSocket::CommonSocket(bool crypt):Socket(AF_INET, SOCK_STREAM, 0),need_crypt_(crypt)
{
	dpid_ = ((serial_no_++) << 16) | (Handle() & 0x0000FFFF);
	init();
}

CommonSocket::~CommonSocket()
{
	SAFE_DELETE(recv_buf_);
}

void CommonSocket::init()
{
    is_ok_ = true;
	recv_buf_ = BufferFactory::get_instance().create_buffer();
	timeout_prev_ = NULL;
	timeout_next_ = NULL;
	timeout_ = g_network_frm;
    real_ip_ = 0;
}

DPID CommonSocket::get_dpid() const
{
	return dpid_;
}

const EndPoint* CommonSocket::get_endpoint() const
{
	return &peer_addr_;
}

/*!< 这两个实际的发送与接收函数都尽最大可能发送与接收数据,所以无需循环调用.
	返回实际发送与接收的字节数,断开连接返回-1.
*/
int CommonSocket::act_send(const char * buf, int length)
{
	int len = -1;
	int total_send = 0;

	do {
		len = Send( &buf[ total_send ], length - total_send, 0 );
		if ( len > 0 ) total_send += len;
	} while( len > 0 && total_send < length );

	if ((!len) || ((len == -1) && (ErrNo() != DWOULDBLOCK)))
	{
        TRACE(2)("[NET]act_send() error, len: %d, errno: %d, dpid: 0x%08X, return -1", len, ErrNo(), dpid_ );
		return -1;
	}

	return total_send;
}

int CommonSocket::act_recv(char * buf, int length)
{
	int len = -1;
	int total_recv = 0;

	do {
        if( length-total_recv == 0 )
            ERR( 2 )( "[NET] recv get length 0!!!!!" );
		len = Recv( &buf[ total_recv ], length - total_recv, 0 );	
		if ( len > 0 ) total_recv += len;
	} while( len > 0 && total_recv < length );

	if ((!len) || ((len == -1) && (ErrNo() != DWOULDBLOCK)))
	{
        TRACE(2)("[NET]act_recv() error, len: %d, errno: %d, dpid: 0x%08X, return: -1", len, ErrNo(), dpid_ );
		return -1;
	}

	return total_recv;
}

#if 0
/*!< 发送缓冲队列中的所有Buffer数据.
	成功发送全部数据返回0,发送部分数据返回1,断开连接返回-1.
*/
int CommonSocket::do_send()
{
#ifdef __linux

    #if 0 
	if ( need_crypt_ )
	{
		if( flow_rate_send_.get_over_flow() )
		{
			ERR(2)("[NET](common_socket) CommonSocket::do_send() over flow");
			return -1;
		}		
	}
    #endif
#endif
	while ( send_bufque_.get_count() )
	{
		Buffer * send_buf = send_bufque_.get_head();
		if (!send_buf)
		{
			ERR(2)("[NET](common_socket) CommonSocket::do_send() send buf Null!");
			return -1;
		}

		int len;
		char * buf = NULL;

		buf = send_buf->get_readable_buffer( &len );


		if ( need_crypt_ && (!send_buf->crypted_) )
		{
#ifdef __linux
			ctx_encode( ctx_send_, buf, len );
#else
			ctx_encode_send( buf, len );
#endif
			send_buf->crypted_ = TRUE;
		}

		assert(buf);
		int ret = act_send(buf, len);

		if ( ret < 0 )
		{
			//ERR(2)("[NET](common_socket) CommonSocket::do_send() ret < 0!");
			return -1;
		}
#ifdef __linux
		if ( need_crypt_ ) 
			flow_rate_send_.add(-ret);
#endif

//		LOG(2)( "CommonSocket::do_send, this = 0x%08X", this );
//		OutPutAscii( buf, ret );


		if ( ret != len )
		{
			send_buf->head_ += ret;
			return 1;
		}
		else
		{
			send_bufque_.remove_head();
			SAFE_DELETE(send_buf);
		}
	}
	return 0;
}
#endif

/*!< 发送缓冲队列中的所有Buffer数据.
	成功发送全部数据返回0,发送部分数据返回1,断开连接返回-1.
*/
int CommonSocket::do_send()
{
    AutoLock lock( &send_mutex_ );
    while ( !list_empty( &(send_bufque_.link_) ) ) {
        list_head* pos = send_bufque_.link_.next;
		Buffer * send_buf = list_entry( pos, Buffer, link_ );

		int len = 0;
		char * buf = NULL;
		buf = send_buf->get_readable_buffer( &len );

		if ( need_crypt_ && (!send_buf->crypted_) ) {
			ctx_encode( ctx_send_, buf, len );
			send_buf->crypted_ = TRUE;
		}

        /*! 对ret返回值进行判断和容错, 增加一个安全控制变量count，最多重试count次*/
		int ret = act_send(buf, len);

        if( ret >= 0 ) {
            if( ret >= len ) {
                list_del( pos );
                SAFE_DELETE( send_buf );
                send_down();
            } else {
                send_buf->head_ += ret;
                if ( ret ) {
                    LOG(2)("[NET]do_send() not all, len: %d, ret: %d, errno: %d, dpid: 0x%08X", len, ret, ErrNo(), dpid_ );
                } else {
                    PROF(2)("[NET]do_send() error, len: %d, ret: %d, errno: %d, dpid: 0x%08X", len, ret, ErrNo(), dpid_ );
                }
                return 1;
            }  
        } else {
            return -1;
        }
	}
	return 0;
}



/*!< 接收数据,生成Buffer,并使用传出参数buf返回该Buffer,没有生成Buffer,buf置NULL;
	成功返回0,接收被阻塞返回1,断开连接返回-1;
*/
int CommonSocket::do_recv(Buffer ** buf)
{
	assert(buf);
	assert(recv_buf_);
	int len;
	char * buf_recved = recv_buf_->get_writable_buffer( &len );

    /*! 对ret返回值进行判断，容错，并增加一个安全控制变量count，最多重试count次*/
	int ret = act_recv(buf_recved, len);

	//OutPutAscii( buf_recved, ret );
	
	if ( ret < 0)
	{
		(*buf) = NULL;
		return -1;
	}

	if ( 0 == ret )
	{
        LOG(2)("[NET]do_recv() error, ret: %d, len: %d, errno: %d, dpid: 0x%08X, count: %d", ret, len, ErrNo(), dpid_ );
		return 1;
	}

	if ( need_crypt_ )
	{
#ifdef __linux
		if ( flow_rate_recv_.is_over_flow_with_frame(ret) )
		{
			ERR(2)("[NET](common_socket) CommonSocket::do_recv() over flow");
			return -1;			
		}
		ctx_encode( ctx_recv_, buf_recved, ret );
#else
		ctx_encode_recv( buf_recved, ret );
#endif
		
	}

	recv_buf_->tail_ += ret;
	
	int remnant;
	char * ptr	= recv_buf_->get_readable_buffer( &remnant );

	Buffer* old_buf	= NULL;
	size_t packet_size;

    static int index = 0;
    index++;
    DPID dpid = get_dpid();
	while(true)
	{
		if( remnant < (int)HEADER_SIZE )
		{
			/*!< if cb > 0 , it means part of the head info append the buffer
			(but is it impossible? because the AddData of buffer queue will alloc a new buffer for this case)
			otherwise it means error message received, the func will return NULL,the caller will close the connection
			*/
			if( recv_buf_->cb_ > 0 )
			{

				old_buf = recv_buf_;
				old_buf->tail_	-= remnant;	/*!< remove remnant from old buffer*/
				recv_buf_ = BufferFactory::get_instance().create_buffer();
				if (!recv_buf_)
				{
					SAFE_DELETE(old_buf);
					(*buf) = NULL;
					return -1;
				}
				memcpy( recv_buf_->tail_, ptr, remnant );
				recv_buf_->tail_ += remnant;
			}
            
			(*buf) = old_buf;
			return 0;
		}
		else
		{
			packet_size	= recv_buf_->get_packet_size( ptr );   /*!<ptr[1] store the size of the packet.*/
			
			/*!< 这里取出的长度必须是大于HEADER_SIZE的,否则就是非法的包,
			     更严重的是: 当packet_size=0时, 会使本while语句陷入死循环
			     sodme@netease
			 */

			if( packet_size < HEADER_SIZE ) 
			{
				ERR(2)("[NET](common_socket) CommonSocket::do_recv(), packet_size < HEAD_SIZE, %d, %d, %d", packet_size, remnant, index );
				return -1;
			}

			if( (unsigned int)remnant < packet_size )  /*!<it means the packet stored in several buffer*/
            {
                old_buf = recv_buf_;
                if( old_buf->cb_ > 0 ) {
                    recv_buf_ = BufferFactory::get_instance().create_buffer( packet_size );
                    if( recv_buf_ == NULL ) {
				        ERR(2)("[NET](common_socket) CommonSocket::do_recv(), BUFFER_ERROR, packet_size = 0x%08X, dpid = 0x%08X, 1", packet_size, dpid );
                        SAFE_DELETE( old_buf );
                        (*buf) = NULL;
                        return -1;
                    }
                    memcpy( recv_buf_->tail_, ptr, remnant );
                    recv_buf_->tail_ += remnant;
                    (*buf) = old_buf;
                } else {
                    if( old_buf->buf_max_ - old_buf->buf_start_ < (int)packet_size ) {
                        recv_buf_ = BufferFactory::get_instance().create_buffer( packet_size );
                        if( recv_buf_ == NULL ) {
							ERR(2)("[NET](common_socket) CommonSocket::do_recv(), BUFFER_ERROR, packet_size = 0x%08X, dpid = 0x%08X, 2", packet_size, dpid );
                            SAFE_DELETE( old_buf );
                            (*buf) = NULL;
                            return -1;
                        }
                        memcpy( recv_buf_->tail_, ptr, remnant );
                        recv_buf_->tail_ += remnant;
                        SAFE_DELETE( old_buf );
                    } 
                    (*buf) = NULL;
                }
                return 0;
            }
			else	/*!< it means the buffer might store several packets.*/
			{
				recv_buf_->cb_++;			/*!<maybe the cb is the number of packet in the buffer.*/
				remnant -= ( packet_size );	/*!<the buffer is not just catain a packet*/
				ptr += ( packet_size );		/*!<continue to fetch other packets in current buffer*/
			}
		}
	}
}

int CommonSocket::do_connect(const EndPoint & ep)
{
	if ( Connect(ep) < 0 )
	{
		ERR(2)("[NET](common_socket) CommonSocket::do_connect() can not init connection! code : %d", ErrNo() );
		return -1;
	}

	/*!< 在非加密连接时也同样调用recv_seed()函数,以验证连接的合法性.*/
    if( need_crypt_ ) {
        if ( recv_seed() < 0 )
        {
            ERR(2)("[NET](common_socket) CommonSocket::do_connect() can not recv seed! code : %d", ErrNo() );
            return -1;
        }
    }

	return 0;
}

int CommonSocket::do_disconnect()
{
	return Close();
}

int CommonSocket::do_listen(EndPoint & ep, int backlog)
{
	if ( Bind(ep) < 0)
	{
		ERR(2)("[NET](common_socket) CommonSocket::do_listen() can not bind address on %s.", ep.GetString() );
		return -1;
	}

	if ( Listen(backlog) < 0)
	{
		ERR(2)("[NET](common_socket) CommonSocket::do_listen() can not listen on port %d.", ep.GetPort() );
		return -1;
	}

	return 0;
}

int CommonSocket::send_msg(Buffer * buffer)
{
    AutoLock lock( &send_mutex_ );
#ifdef __linux
	/*!< add flow control */
	if ( need_crypt_ )
		flow_rate_send_.is_over_flow( buffer->get_readable_buffer_size() );
    
    if( is_ok() == false ) {
        SAFE_DELETE( buffer );
        return 0;
    }

#endif
	send_bufque_.add_tail(buffer);
	return 0;
}

void CommonSocket::set_peer(EndPoint peer)
{
	peer_addr_ = peer;
}

void CommonSocket::set_seed(int seed)
{
	if ( need_crypt_ )
	{
#ifdef __linux
		ctx_init( seed, ctx_send_ );
		ctx_init( seed, ctx_recv_ );
#else
		ctx_init( seed );
#endif

	}
	seed_ = seed;
}

/*!< 建立连接还包括了如下两个函数,使用一个种子包发送和接收种子.
	同时,使用该种子包的包头SEED_HEADER_SIZE初步检验该连接的合法性.
*/
int CommonSocket::send_seed()
{
	int seed = rand();
	set_seed(seed);
	Buffer * buf = BufferFactory::get_instance().create_buffer();
	if (!buf)
	{
		ERR(2)("[NET](common_socket) CommonSocket::send_seed() can not alloc memory of seed buf.");
		return -1;
	}
	char * start = buf->buf_start_;
	/*!< 构造一个种子包并直接放到发送队列中.*/
	*(ff_header_t*)start = HTONL(SEED_HEADER_SIZE);
	*(int*)(start + HEADER_SIZE) = HTONL(seed);
	buf->tail_ += SEED_HEADER_SIZE;
	buf->crypted_ = TRUE;	/*!< 该包跳过加密*/
	send_bufque_.add_tail(buf);
    send_up();
	return 0;
}

int CommonSocket::recv_seed()
{
	char seed[SEED_HEADER_SIZE] = { 0, };
	u_long idx = 0, time = 0;
	/* 设置与Connect时间相同的接收超时, 如果无法接收到种子,则当作连接失败*/
	while ( (idx < sizeof(seed)) && (time++ < CONNECT_TIMEOUT*20) )
	{
		int ret =  Recv( seed + idx, sizeof(seed) - idx, 0 );

		if ((!ret) || ((ret == -1) && (ErrNo() != DWOULDBLOCK)))
		{
			return -1; /*!< 连接已断开 */
		}

		if ( ret > 0 )
			idx += ret;
		ff_sleep(50);
	}
	/*!< 验证种子包合法性,也通过该种子包头长度初步检验了该连接的合法性*/
	if ( ( SEED_HEADER_SIZE != idx ) || ( SEED_HEADER_SIZE != HTONS( *(ff_header_t*)seed ) ) )
	{
		return -1;
	}

	set_seed( HTONL( *(int*)(seed + HEADER_SIZE) ) );
	return 0;
}


void CommonSocket::set_bad()
{
    is_ok_ = false;
    send_bufque_.clear( true );
	SAFE_DELETE(recv_buf_);
}


} //namespace FF_Network

