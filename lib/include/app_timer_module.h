#ifndef __APP_TIMER_MODULE_H__
#define __APP_TIMER_MODULE_H__

#include "app_base.h"

class TimerModule:public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#endif

