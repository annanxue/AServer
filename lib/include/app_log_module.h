#ifndef __APP_LOG_MODULE_H__
#define __APP_LOG_MODULE_H__

#include "app_base.h"

class LogModule : public AppClassInterface
{
public:   
    bool app_class_init();    
    bool app_class_destroy(); 
};

#endif

