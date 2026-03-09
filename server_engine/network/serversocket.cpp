#ifdef __linux

#include "log.h"
#include "serversocket.h"
#include "clientsocket.h"
#include "netmng.h"
//#include "msghdr.h"

namespace FF_Network {

int ServerSocket::process(int events, NetMng * parent)
{
	if ( events & poll_data::EVENT_IN )
	{
		int fd;
		EndPoint ep;

		while ( ( fd = this->Accept( ep ) ) > 0 )
		{
			if ( ( parent->get_user_num() + 1 ) > parent->get_max_fd() )
			{
				ff_close(fd);
				ERR(2)("[NET](server_socket) ServerSocket::process() max file-descriptor!");
				continue;
			}

			if ( parent->get_select_thread()->fd_add( fd, poll_data::EVENT_IN | poll_data::EVENT_OUT ) < 0)
			{
				ff_close(fd);
				ERR(2)("[NET](server_socket) ServerSocket::process() fd_add() failed! code : %d", ErrNo() );
				continue;
			}

			/*
			if ( dos.is_dos(ep.GetIP()))
			{
				ff_close(fd);
				continue;
			}*/
		
			ClientSocket * csock = new ClientSocket( fd, need_crypt() );

			if ( !csock )
			{
				ff_close(fd);
				ERR(2)("[NET](server_socket) ServerSocket::process() can not alloc memory of ClientSocket.");
				continue;
			}

			if ( parent->add_sock( (CommonSocket*)csock ) < 0 )
			{
				ERR(2)("[NET](server_socket) ServerSocket::process() add_sock failed.");
				SAFE_DELETE(csock);
				continue;
			}

//LOG(2)("cur user num is: %d", parent->get_user_num());

            if( need_crypt() ) {
                csock->send_seed();
            }
			csock->set_peer(ep);

			LOG(2)("[NET](server_socket) client connect, fd: %d, dpid: 0x%08X, ip: %s", fd, csock->get_dpid(), csock->get_endpoint()->GetString());

			parent->post_connect_msg(csock);
		}
		
		/*		
		if( fd <= 0 ){
			ERR(2)("[NET](server_socket) ServerSocket::process(), server accept error!!!!!!!!, fd: %d, ip: %s", fd, ep.GetString());
		}*/
	}
	
	if ( events & poll_data::EVENT_ERR )
	{
		ERR(2)("[NET](server_socket) ServerSocket::process() get EVENT_ERR fd : %d, code : %d", Handle(), ErrNo());
		return -1;
	}
	return 0;
}

}

#endif //__linux
