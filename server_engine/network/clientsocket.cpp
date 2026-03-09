#include "log.h"
#include "clientsocket.h"
#include "ffpoll.h"
#include "netmng.h"

namespace FF_Network {

int ClientSocket::process(int events, NetMng * parent)
{
    if( is_ok() == false ) return -1;

    if ( events & poll_data::EVENT_IN )
    {
        if( !parent->is_stop_recv() )
        {
            Buffer * buf = NULL;
            if ( do_recv( &buf ) < 0 )
            {
                return -1;
            }

            if (buf)
            {
                buf->dpid_ = this->get_dpid();
                parent->add_recv_msg( buf );
            }

		    parent->refresh_timeout( this );
        }
    }
    if ( events & poll_data::EVENT_OUT )
    {
        if( !parent->is_stop_send() )
        {
            int ret = do_send();
            if ( ret < 0 )
            {
                return -2;
            }
            else if ( 0 == ret ) /*!< 所有数据都已发送,取消可写状态*/
            {
                if ( parent->get_select_thread()->fd_mod( Handle(), poll_data::EVENT_IN ) < 0 )
                    ERR(2)("[NET](client_socket) ClientSocket::process() fd_mod failed! fd : %d, code : %d", Handle(), ErrNo());
            }
        }
    }
	if ( events & poll_data::EVENT_ERR )
	{
		ERR(2)("[NET](client_socket) ClientSocket::process() get EVENT_ERR fd : %d, code : %d", Handle(), ErrNo());
		return -3;
	}
	return 0;
}

}
