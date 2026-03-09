#include "app_timer_module.h"
#include "log.h"
#include "timer.h"

bool TimerModule::app_class_init(){
    int32_t timer_size = 0;   
    int32_t frame_rate = 10;
    APP_GET_NUMBER("TimerSize", timer_size );
    APP_GET_NUMBER("LogicFrameRate", frame_rate);
    g_timermng = new TimerMng( timer_size, frame_rate );
    LOG(2)("===========Timer Module Start===========");
    LOG(2)("Timer Size:%d", timer_size );

    return true;         
}

bool TimerModule::app_class_destroy(){
    SAFE_DELETE(g_timermng);  
    LOG(2)("===========Timer Module Destroy===========");
    return true;
}

