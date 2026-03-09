#include "msgtype.h"
#include "snapshot.h"
#include "msg.h"

Snapshot::Snapshot()
{
    flush();
}


Snapshot::~Snapshot()
{
}


void Snapshot::flush()
{
    cb_ = 0;
    ar_ << (ff_header_t)0 << (packet_type_t)ST_SNAPSHOT << cb_ ;

    last_send_packet_type_ = 0;
    last_send_packet_length_ = 0;
    combine_packet_count_ = 0;
    combine_packet_size_ = 0;
}


void Snapshot::set_header( DPID _dpid )
{
    int len;
    char* buf = ar_.get_buffer( &len );

    static int offset = HEADER_SIZE + PACKET_TYPE_SIZE;

    *(ff_header_t*)buf = HTONL( (ff_header_t)len );
    *(u_short*)(buf + offset) = HTONS(cb_);

    ar_.buffer_->dpid_ = _dpid; 
    ar_.buffer_->tail_ += len;
}


void Snapshot::reset()
{
    ar_.recreate_buffer();
    flush();
}

// ST_CORR_POS ST_CORR_POS_HERO ST_STATE_MST_STAND ST_STATE_MST_NAVIGATE ST_SETPOS should write ctrl_id first
bool Snapshot::is_packet_can_combine( unsigned short _packet_type, const char* _new_buf )
{
    if( _packet_type == ST_CORR_POS 
        || _packet_type == ST_CORR_POS_HERO 
        || _packet_type == ST_CORR_POS_SKILL
        || _packet_type == ST_STATE_MST_STAND 
        || _packet_type == ST_STATE_MST_NAVIGATE 
        || _packet_type == ST_SETPOS )
    {
        if( _packet_type == last_send_packet_type_ )
        {
            char* old_buf = ar_.buf_cur_ - last_send_packet_length_;

            u_long old_ctrl_id = *((u_long*)(old_buf + sizeof(unsigned char)));
            u_long new_ctrl_id = *((u_long*)(_new_buf + sizeof(unsigned char)));

            if( old_ctrl_id == new_ctrl_id )
            {
                return true;
            }
        }
    }

    return false;
}

void Snapshot::log_combine( int _ctrl_id )
{
    ++combine_packet_count_;
    combine_packet_size_ += last_send_packet_length_;

    if( combine_packet_count_ > 3 ) 
    {
        PROF(2)( "[Snapshot](log_combine)ctrl_id: %d, combine_packet_count: %d, combine_packet_size: %d", _ctrl_id, combine_packet_count_, combine_packet_size_ );
    }
}

