#include "bf_client_module.h"
#include "game_lua_module.h"
#include "game_svr_module.h"
#include "db_client_module.h"
#include "msgtype.h"
#include "misc.h"
#include "lmisc.h"
#include "msg.h"
#include "lar.h"

const char BFClient::className[] = "BFClient";
const bool BFClient::gc_delete_ = false;

Lunar<BFClient>::RegType BFClient::methods[] =
{
    method( BFClient, c_get_bf_ip ),
    method( BFClient, c_get_bf_port ),
    method( BFClient, c_get_is_connect ),
    method( BFClient, c_get_dpid_by_bfid ),
    method( BFClient, c_get_all_bf_dpid ),
    method( BFClient, c_get_center_dpid ),
    method( BFClient, c_get_all_dpid ),
    method( BFClient, c_choose_bf ),
    method( BFClient, c_get_random_bf ),
    method( BFClient, c_show_bfclient ),
    method( BFClient, c_get_bf_num ),
    method(BFClient, c_get_bf_by_serverid),
    {NULL, NULL}
};

void ConnectBFThread::init( BFClient* _bf_client, int _sleep, int _wait, int _crypt ) 
{ 
    thread_name_ = "ConnectBFThread";
    sleep_ = _sleep; 
    crypt_ = _crypt;
    bf_client_ = _bf_client; 
    bf_client_->init_multi_client( bf_client_->get_bf_num()*2, _wait );
}

void ConnectBFThread::send_heart_beat( DPID _dpid )
{
    BEFORESEND( ar, BF_TYPE_HEART_BEAT );
    SEND( ar, bf_client_, _dpid );
}

void ConnectBFThread::send_register( int _bf_id, DPID _dpid )
{
    BEFORESEND( ar, BF_TYPE_REGISTER );
    ar << _bf_id;
    ar << g_gamesvr->get_server_id();
    SEND( ar, bf_client_, _dpid );
}

void ConnectBFThread::update_connect( BFInfo& _info, DPID _dpid )
{
    _info.connected_ = true;
    _info.dpid_ = _dpid;
    _info.heart_beat_time_ = time(0);
}

void ConnectBFThread::run()
{
    while( active_ ) 
    {    
        BFInfo* info = bf_client_->get_bf_info();
        for( int i = 0; i < bf_client_->get_bf_num(); i++ ) 
        {
            if( !info[i].connected_ ) 
            { 
                CommonSocket* csock = (CommonSocket*)bf_client_->connect_server_new( info[i].ip_, info[i].port_, crypt_ ); 
                if( csock )
                {
                    update_connect( info[i], csock->get_dpid() );
                    send_register( i, csock->get_dpid() );
                }
            }
            else 
            {
                if( time(0) - info[i].heart_beat_time_ > HEART_BEAT_TIMEOUT ) 
                {
                    g_bfclient->post_disconnect_msg( info[i].dpid_, 0, 3 ); 
                }
                else
                {
                    send_heart_beat( info[i].dpid_ );
                }
            }
        }
        int sleeped_time = 0;
        while(active_ && sleeped_time < sleep_){
            int sleep_time = (sleep_ - sleeped_time) > 10 ? 10 : (sleep_ - sleeped_time);
            ff_sleep( sleep_time );
            sleeped_time += sleep_time;
        }
    }
}

BFClient::BFClient()
    :bf_num_(0), bf_info_(NULL)
{
    init_func_map();
}

BFClient::~BFClient()
{
    SAFE_DELETE_ARRAY( bf_info_ );
}

bool BFClient::is_bf_ready()
{
    if( bf_num_ < 2 ) 
    {
        return false;
    }
    else if( !bf_info_[0].connected_ ) 
    {
        return false;
    }
    return true;
}

int BFClient::choose_bfid_min_load()
{
    if( !is_bf_ready() )
    {
        return -1;
    }
    int min = 1<<30;
    int idx = -1;
    for( int i = 1; i < bf_num_; i++ ) 
    {
        if( bf_info_[i].connected_ && min > bf_info_[i].load_ ) 
        {
            min = bf_info_[i].load_;
            idx = i;
        }
    }
    return idx;
}

DPID BFClient::choose_dpid_min_load()
{
    int idx = choose_bfid_min_load();
    if( idx > 0 ) 
    {
        return bf_info_[idx].dpid_;
    }
    return DPID_UNKNOWN;
}

int BFClient::choose_bfid_random()
{
    if( !is_bf_ready() )
    {
        return -1;
    }

    int start_bf_id = random() % bf_num_;
    for( int i = 0; i < bf_num_; i++ ) 
    {
        int bf_id = (i + start_bf_id) % bf_num_;
        if( bf_info_[bf_id].connected_ ) 
        {
            return bf_id;
        }
    }
    return -1;
}


int BFClient::choose_bfid_by_serverid()
{
	if (!is_bf_ready())
	{
		return -1;
	}

	int start_bf_id = g_gamesvr->get_server_id() % bf_num_;
	for (int i = 0; i < bf_num_; i++)
	{
		int bf_id = (i + start_bf_id) % bf_num_;
		if (bf_info_[bf_id].connected_)
		{
			return bf_id;
		}
	}

	return -1; 
}


void BFClient::start_connect( int _sleep_time, int _poll_wait_time, int _crypt )
{
    connect_thread_.init( this, _sleep_time, _poll_wait_time, _crypt );
    connect_thread_.start();
}

void BFClient::stop_connect()
{
    connect_thread_.stop();
}

void BFClient::init_func_map()
{
    memset( func_map_, 0, sizeof(func_map_));

    func_map_[PT_CONNECT] = &BFClient::on_connect; 
    func_map_[PT_DISCONNECT] = &BFClient::on_disconnect; 
    func_map_[BF_TYPE_HEART_BEAT_REPLY] = &BFClient::on_heart_beat_reply;
    func_map_[BF_TYPE_B2G_SAVE_PLAYER] = &BFClient::on_save_player;
}

void BFClient::lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid )
{
   if( lua_gettop( _L ) != 0 )
    {
        LOG(1)( "[HANDLE](msg) %s:%d lua_msg_handle, lua_gettop() is %d", __FILE__, __LINE__, lua_gettop(_L) );
        lua_pop( _L, lua_gettop(_L) );
    }
    g_luasvr->get_lua_ref( GameLuaSvr::BFCLIENT_MESSAGE_HANDLER );
    LAr lar( _msg, _size );
    Lunar<LAr>::push( _L, &lar, false );        // push the ar
    lua_pushlightuserdata( _L, (void*)_msg );   // push the addr of buffer
    lua_pushnumber( _L, _size );                // push the size of buffer
    lua_pushnumber( _L, _packet_type );         // push the message type
    lua_pushnumber( _L, _dpid );                // push the sid of client
    assert( !lua_isnil( _L, -6 ) );

    if( llua_call( _L, 5, 0, 0 ) )      llua_fail( _L, __FILE__, __LINE__ );
}

void BFClient::on_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
}

void BFClient::on_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    lua_msg_handle( g_luasvr->L(), _buf, _buf_size, PT_DISCONNECT, _dpid );
    for( int i = 0; i < bf_num_; i++ ) 
    {
        if( bf_info_[i].connected_ && bf_info_[i].dpid_ == _dpid )
        {
            bf_info_[i].connected_ = false;
            bf_info_[i].load_ = 0;
            bf_info_[i].dpid_ = 0;
            break;
        }
    }
    del_sock( _dpid );
}

void BFClient::msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid )
{
    void (BFClient::*pfn)( Ar&, DPID, const char*, u_long ) = NULL;
    TICK(A);
    if ( _packet_type >= 0 && _packet_type < PT_PACKET_MAX ) 
    {    
        pfn = func_map_[_packet_type];
        if( pfn ) 
        {
            Ar ar( _msg, _size );   
            ( this->*(pfn) )( ar, _dpid, _msg, _size );
        }
        else
        {
            lua_msg_handle( g_luasvr->L(), _msg, _size, _packet_type, _dpid );
        }
    }
    else
    {
        ERR(2)( "[BFClient](msg_handle)packet: %d out of range", _packet_type );
    }
    TICK(B);
    mark_tick( _packet_type + TICK_TYPE_PACKET, A, B );
}

void BFClient::on_heart_beat_reply( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    int bf_id = -1;
    _ar >> bf_id;
    if( bf_id >= bf_num_ || bf_id < 0 ) 
    {
        ERR(2)( "[BFCLIENT](on_heart_beat_reply) bf_id: %d out range.", bf_id );
        return;
    }
    bf_info_[bf_id].heart_beat_time_ = time(0);
}

void BFClient::on_save_player( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size )
{
    u_long migrate_time, player_id;
    char online;
    _ar >> migrate_time >> online >> player_id;

    lua_State *L = g_luasvr->L();

    // 检查是否可以将包转给db。
    g_luasvr->get_lua_ref( GameLuaSvr::CAN_BF_SAVE_PLAYER );
    lua_pushnumber( L, player_id );
    lua_pushnumber( L, migrate_time );
    if( llua_call( L, 2, 1, 0 ) )      llua_fail( L, __FILE__, __LINE__ );

    int can_save = lua_toboolean( L, -1 );

    lua_pop( L, 1 );    

    if (can_save)
    {
        const char *buf = _buf + sizeof(migrate_time);
        u_long buf_size = _buf_size - sizeof(migrate_time);

        // 将包转给db。
        BEFORESEND( ar, DB_TYPE_SAVE_PLAYER );
        ar.write( buf, buf_size );
        SEND( ar, g_dbclient, 0 );

        // 因为lua层的响应函数可能会调用do_join向数据库取数据，所以必须在存盘包转给bf
        // 后再调用lua层的msg_handler，避免取到老数据。
        lua_msg_handle( g_luasvr->L(), buf, buf_size, BF_TYPE_B2G_SAVE_PLAYER, _dpid );  
    }
}

int BFClient::c_get_bf_ip( lua_State* _L )
{
    lcheck_argc( _L, 1 );           
    int idx = (int)lua_tonumber( _L, -1 );
    if( idx < bf_num_ && idx >= 0 ) 
    {
        lua_pushstring( _L, bf_info_[idx].ip_ );
    }
    else
    {
        lua_pushnil( _L );
    }
    return 1;
}

int BFClient::c_get_bf_port( lua_State* _L )
{
    lcheck_argc( _L, 1 );           
    int idx = (int)lua_tonumber( _L, -1 );
    if( idx < bf_num_ && idx >= 0 ) 
    {
        lua_pushnumber( _L, bf_info_[idx].port_ );
    }
    else
    {
        lua_pushnil( _L );
    }
    return 1;
}

int BFClient::c_get_is_connect( lua_State* _L )
{
    lcheck_argc( _L, 1 );           
    int idx = (int)lua_tonumber( _L, -1 );
    if( idx < bf_num_ && idx >= 0 ) 
    {
        lua_pushboolean( _L, bf_info_[idx].connected_ );
    }
    else 
    {
        lua_pushboolean( _L, false );
    }
    return 1;
}

int BFClient::c_get_dpid_by_bfid( lua_State* _L )
{
    lcheck_argc( _L, 1 );           
    int idx = (int)lua_tonumber( _L, -1 );
    if( idx < bf_num_ && idx >= 0 && bf_info_[idx].connected_ ) 
    {
        lua_pushnumber( _L, bf_info_[idx].dpid_ );
    }
    else
    {
        lua_pushnumber( _L, DPID_UNKNOWN );
    }
    return 1;
}

int BFClient::c_get_all_bf_dpid( lua_State* _L )
{
    lua_newtable( _L );
    for( int i = 1; i < bf_num_; i++ )
    {
        if( bf_info_[i].connected_ ) 
        {
            lua_pushnumber( _L, i );
            lua_pushnumber( _L, bf_info_[i].dpid_ );
            lua_settable( _L, -3 );
        }
    }
    return 1;
}

int BFClient::c_get_center_dpid( lua_State* _L )
{
    if( bf_info_[0].connected_ )
    {
        lua_pushnumber( _L, bf_info_[0].dpid_ );
    }
    else
    {
        lua_pushnumber( _L, DPID_UNKNOWN );
    }
    return 1;
}

int BFClient::c_get_all_dpid( lua_State* _L )
{
    lua_newtable( _L );
    for( int i = 0; i < bf_num_; i++ )
    {
        if( bf_info_[i].connected_ ) 
        {
            lua_pushnumber( _L, i );
            lua_pushnumber( _L, bf_info_[i].dpid_ );
            lua_settable( _L, -3 );
        }
    }
    return 1;
}

int BFClient::c_choose_bf( lua_State* _L )
{
    int id = choose_bfid_min_load();
    if( id > 0 ) 
    {
        lua_pushboolean( _L, true );
        lua_pushnumber( _L, id );
        lua_pushnumber( _L, bf_info_[id].dpid_ );
    }
    else
    {
        lua_pushboolean( _L, false );
        lua_pushnumber( _L, -1 );
        lua_pushnumber( _L, DPID_UNKNOWN );
    }
    return 3;
}

int BFClient::c_get_random_bf( lua_State* _L )
{
    int id = choose_bfid_random();
    if( id >= 0 ) 
    {
        lua_pushboolean( _L, true );
        lua_pushnumber( _L, id );
        lua_pushnumber( _L, bf_info_[id].dpid_ );
    }
    else
    {
        lua_pushboolean( _L, false );
        lua_pushnumber( _L, -1 );
        lua_pushnumber( _L, DPID_UNKNOWN );
    }
    return 3;
}

int BFClient::c_get_bf_by_serverid(lua_State* _L)
{
	int id = choose_bfid_by_serverid();
	if (id >= 0)
	{
		lua_pushboolean(_L, true);
		lua_pushnumber(_L, id);
		lua_pushnumber(_L, bf_info_[id].dpid_);
	}
	else
	{
		lua_pushboolean(_L, false);
		lua_pushnumber(_L, -1);
		lua_pushnumber(_L, DPID_UNKNOWN);
	}
	return 3;
}

int BFClient::c_show_bfclient( lua_State* _L )
{
    for( int i = 0; i < bf_num_; i++ )
    {
        if( i == 0 ) 
        {
            LOG(2)( "CENTER (connected: %d, DPID: %u)", bf_info_[i].connected_, bf_info_[i].dpid_ );
        }
        else
        {
            LOG(2)( "BF %d (connected: %d, DPID: %u):", i, bf_info_[i].connected_, bf_info_[i].dpid_ );
        }
        LOG(2)( "   IP: %s, Port: %d", bf_info_[i].ip_, bf_info_[i].port_ );
        LOG(2)( "   LOAD: %d", bf_info_[i].load_ );
        LOG(2)( "   HEART BEAT TIME: %d", bf_info_[i].heart_beat_time_ );
    }
    return 0;
}

int BFClient::c_get_bf_num( lua_State* _L )
{
    lua_pushnumber( _L, get_bf_num() );
    return 1;
}

bool BFClientModule::app_class_init()
{
    int32_t crypt = 1;
    int32_t poll_wait_time = 100;
    int32_t sleep_time = 100;

    APP_GET_TABLE( "BFClient", 2 );
        APP_GET_NUMBER( "Crypt", crypt );
        APP_GET_NUMBER( "PollWaitTime", poll_wait_time );
        APP_GET_NUMBER( "SleepTime", sleep_time );
        APP_GET_TABLE( "BFList", 3 );
        g_bfclient->set_bf_num( APP_TABLE_LEN );
        BFInfo* bf_info = g_bfclient->get_bf_info();
        int idx = 0;
	    APP_WHILE_TABLE()
            APP_GET_STRING( "IP", bf_info[idx].ip_ );
	        APP_GET_NUMBER( "Port", bf_info[idx].port_ );
            bf_info[idx].connected_ = false;
            bf_info[idx].dpid_ = 0;
            bf_info[idx].load_ = 0;
            bf_info[idx].heart_beat_time_ = 0;
            ++idx;
        APP_END_WHILE();
        APP_END_TABLE();    //BFList
    APP_END_TABLE();        //BFClient

    if( g_bfclient->get_bf_num() <= 0 ) 
    {
        ERR(2)( "[BFClientModule](app_class_init)get_bf_num = %d", g_bfclient->get_bf_num() );
        return false;
    }

    g_bfclient->start_connect( sleep_time, poll_wait_time, crypt );

    LOG(2)( "===========BFClient Module Start===========");
    return true;
}

bool BFClientModule::app_class_destroy()
{
    g_bfclient->stop_connect();

    LOG(2)( "===========BFClient Module Destroy===========");
    return true;
}


