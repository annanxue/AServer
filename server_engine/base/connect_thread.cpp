#include "connect_thread.h"
#include "clientsocket.h"
#include "log.h"

void ConnectThread::init( NetMng* _netmng, char* _ip, uint16_t _port, bool _crypt, int _time_wait, int _sleep_time )
{
    netmng_ = _netmng;
    conn_info_.connected_ = false;
    ff_strncpy( conn_info_.ip_, sizeof( conn_info_.ip_ ), _ip, sizeof( conn_info_.ip_ ) );
    conn_info_.port_ = _port;
    crypt_ = _crypt;
    time_wait_ = _time_wait;
    sleep_time_ = _sleep_time;
    is_conn_info_reset_ = false;
}

void ConnectThread::run()
{
    if( !netmng_ ) return;

    while( active_ ) 
    {
        if( !conn_info_.connected_ ) 
        {
            if( netmng_->connect_server( conn_info_.ip_, conn_info_.port_, crypt_, time_wait_ ) == 0 )
            {
                conn_info_.connected_ = true;
            }
            else
            {
                TRACE(2)( "[NET](connect_thread), try connect %s:%d failed", conn_info_.ip_, conn_info_.port_ );
            }
        }

        int sleeped_time = 0;
        while(active_ && sleeped_time < sleep_time_)
        {
            int sleep_a_time = (sleep_time_ - sleeped_time) > 10 ? 10 : (sleep_time_ - sleeped_time);
            ff_sleep( sleep_a_time );
            sleeped_time += sleep_a_time;
        }

        if( is_conn_info_reset_ )
        {
            if( conn_info_.connected_ )
            {
                CommonSocket* svr_sock = netmng_->get_svr_sock();
                if( svr_sock )
                {
                    netmng_->post_disconnect_msg( svr_sock->get_dpid() );
                }
            }
            is_conn_info_reset_ = false;
        }
    }
}

void ConnectThread::on_conn_disconnect()
{
    conn_info_.connected_ = false;
}

void ConnectThread::reset_connect_info( const char* _ip, uint16_t _port )
{
    memset( conn_info_.ip_, 0, sizeof( conn_info_.ip_ ) );
    ff_strncpy( conn_info_.ip_, sizeof( conn_info_.ip_ ), _ip, sizeof( conn_info_.ip_ ) );
    conn_info_.port_ = _port;

    is_conn_info_reset_ = true;
}

MultiConnectThread::MultiConnectThread()
: netmng_( 0 ), max_conn_( 0 ), curr_conn_( 0 ), conn_infos_( 0 ), connected_conns_( 0 ), crypt_(false), sleep_time_(1000)
{
    thread_name_ = "MulTiConnectThread";
}

MultiConnectThread::~MultiConnectThread()
{
    clean();
}

void MultiConnectThread::init( NetMng* _netmng, uint32_t _max_conn, bool _crypt, int _sleep_time )
{
    if( !_netmng ) return;

    clean();

    netmng_     = _netmng;
    max_conn_   = _max_conn;
    curr_conn_  = 0;
    conn_infos_ = new ConnInfo[_max_conn];
    connected_conns_ = new MyMap32();
    connected_conns_->init( _max_conn, _max_conn, "MultiConnectThread" );
    INIT_LIST_HEAD( &pending_unconnected_conns_ );
    INIT_LIST_HEAD( &free_unconnected_conns_ );
    crypt_ = _crypt;
    sleep_time_ = _sleep_time;
}

void MultiConnectThread::add_conn( const char* _ip, uint16_t _port, bool _add_tail )
{
    if( curr_conn_ >= max_conn_ && list_empty( &free_unconnected_conns_ ) ) return;

    ConnInfo* info;
    if ( list_empty( &free_unconnected_conns_ ) )
    {
        info = &conn_infos_[curr_conn_++];
    }
    else
    {
        info = list_entry( free_unconnected_conns_.next, ConnInfo, link_ );
        list_del( &info->link_ );
    }

    info->connected_ = false;
    ff_strncpy( info->ip_, sizeof( info->ip_ ), _ip, sizeof( info->ip_ ) );
    info->port_ = _port;

    mutex_.Lock();
    if ( _add_tail )
    {
        list_add_tail( &info->link_, &pending_unconnected_conns_ );
    }
    else
    {
        list_add( &info->link_, &pending_unconnected_conns_ );
    }
    mutex_.Unlock();
}

void MultiConnectThread::run()
{
    if( !conn_infos_ || !max_conn_ ) return;

    while( active_ ) {
        mutex_.Lock();
        if( !list_empty( &pending_unconnected_conns_ ) && netmng_->get_user_num() < netmng_->get_max_fd() ) {
            ConnInfo* info = list_entry( pending_unconnected_conns_.next, ConnInfo, link_ );
            if( info ) {
                list_del( &info->link_ );
                ClientSocket* sock = netmng_->connect_server_new( info->ip_, info->port_, crypt_ );
                if( sock ) {
                    info->connected_ = true;
                    connected_conns_->set( sock->get_dpid(), ( intptr_t )info );
                }
                else {
                    list_add_tail( &info->link_, &pending_unconnected_conns_ );
                    TRACE(2)( "[NET](connect_thread), try connect %s:%d failed", info->ip_, info->port_ );
                }
            }
        }
        mutex_.Unlock();
        ff_sleep( sleep_time_ );
    }
}

void MultiConnectThread::on_conn_disconnect( DPID _dpid, bool _reconnect )
{
    intptr_t iptr;
    if( !connected_conns_->find( _dpid, iptr ) ) return;

    ConnInfo* info = ( ConnInfo* )iptr;
    info->connected_ = false;

    mutex_.Lock();
    connected_conns_->del( _dpid );
    if ( _reconnect )
    {
        list_add_tail( &info->link_, &pending_unconnected_conns_ );
    }
    else
    {
        list_add_tail( &info->link_, &free_unconnected_conns_ );
    }
    mutex_.Unlock();
}

void MultiConnectThread::clean()
{
    SAFE_DELETE_ARRAY( conn_infos_ );
    SAFE_DELETE( connected_conns_ );
}
