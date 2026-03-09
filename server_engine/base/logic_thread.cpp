#include "logic_thread.h"
#include "app_base.h"
#include "misc.h"
#include "log.h"
#include "gperftools/malloc_extension.h"
#include "netmng.h"

extern int g_frame;

const int TC_STAT_LOG_LEN = 4096;
volatile bool LogicThread::reload_lua_ = false;
volatile bool LogicThread::pre_stop_ = false;
volatile bool LogicThread::pre_stop_tried_ = false;

extern int is_lib_time_out();

void LogicThread::frame_begin()
{
    g_frame++;
    FF_Network::g_network_frm++;

    if( g_frame % frame_rate_ == 0 ) 
    {
        LOG_PROCESS_TIMER();

        if( is_lib_time_out() )
        {
            pre_stop_ = true;
            LOG(2)( "[LogicThread](frame_begin) lib timeout" );
        }
    }
}

void LogicThread::do_log_frame( NetMng* _net_mng, lua_State* _L )
{
    if( g_frame % ( frame_rate_ * 60 ) == 0 )
    {
        char tc_stat[TC_STAT_LOG_LEN];
        MallocExtension::instance()->GetStats( tc_stat, TC_STAT_LOG_LEN );
        PROF(2)( "%s", tc_stat );

        _net_mng->print_packet_stats();

        size_t tc_memory = 0; 
        MallocExtension::instance()->GetNumericProperty( "generic.current_allocated_bytes", &tc_memory );

        int virtual_memory = get_mm();
        int lua_memory = lua_gc( _L, LUA_GCCOUNT, 0 );

        PROF(0)( "[MEMORY]lua memory: %dM, tcmalloc use: %dM, buf count: %d, max buf count: %d, virtual memory: %dM", 
                lua_memory>>10, tc_memory>>20, Buffer::count_, Buffer::max_num_, virtual_memory>>10 );
    }
}

void LogicThread::frame_end()
{
    log_frame();

    int32_t frame_end_time = frame_start_time_ + uint32_t(g_frame*(1000.0f/frame_rate_)); 
    frame_wait_time_ = frame_end_time - msec();
    run_gc_step( frame_wait_time_ - 5 );
    frame_wait_time_ = frame_end_time - msec();

    if( frame_wait_time_ > 0 ) {
        log_tick( 0 );
        ff_sleep( frame_wait_time_ );
    }
    else if ( frame_wait_time_ < 0 ){
        log_tick( 1 );
        TRACE(2)( "[CPU] WAIT TIME %d, %zd", frame_wait_time_, g_frame );
    }
    else{ // frame_wait_time_ == 0
        log_tick( 0 );
    }
}

void LogicThread::run()
{
    frame_start_time_ = msec();
    frame_wait_time_ = 0;   

    while( active_ ) {
        frame_begin();
#ifdef DEBUG
        lock();
#endif
        TICK( BEGIN );
        reload_lua();
        check_stop();
        recv_msg();
        process();
        do_logic_state_lua();
        run_timer(); 
        send_msg();
        TICK( END );
        mark_tick( TICK_TOTAL, BEGIN, END );
#ifdef DEBUG
        unlock();
#endif
        frame_end();
    }
}




