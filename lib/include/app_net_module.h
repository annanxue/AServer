#ifndef __APP_NET_MODULE_H__
#define __APP_NET_MODULE_H__

#include "app_base.h"

class NetModule : public AppClassInterface
{
public:   
    bool app_class_init();    
    bool app_class_destroy(); 
};

#endif

