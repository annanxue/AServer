#include <unistd.h>
#include <stdio.h>

#include "app_base.h"
#include "app_log_module.h"
#include "app_timer_module.h"
#include "timer.h"
#include "log.h"


int g_frame = 0;

static void sigterm_handler( int32_t _sig ){
    printf("recv sig:%d\n", _sig);
    AppBase::active_ = false;
    LOG(2)( "[AppBase]recv sig: %d, active: %d", _sig, AppBase::active_ );
}

int32_t timeout_handler( uint64_t _data1, uint64_t _data2 ){
    AppBase::active_ = false;
    LOG(2)( "[AppBase]timeout and exit" );
    return 0;
}

class TestApp: public AppBase{
public:
    bool main_loop(){
        g_timermng->add_timer( 9UL, 0x00010001U, 0,0, 0, timeout_handler );
        while(active_){
            LOG(2)("gframe:%d/9, For test only. Ctrl + C to exit.", g_frame);
            g_frame++;
            g_timermng->run_timer_list();
            sleep(1);
        }
        return 0;
    }
};

int main( int argc, char** argv )
{
    LogModule log_module;
    TimerModule timer_module;
    TestApp test_app;
    if (test_app.init(argc, argv)){
        test_app.register_class( &log_module );
        test_app.register_class( &timer_module );
        test_app.register_signal(SIGTERM, sigterm_handler);
        test_app.register_signal(SIGINT, sigterm_handler);
        test_app.start();
    }
    return 0;
}


