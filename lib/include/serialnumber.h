#ifndef __SERIAL_NUMBER_H__
#define __SERIAL_NUMBER_H__

#include <time.h>
#include "singleton.h"

class SerialNumber
{
public:
	SerialNumber(){init();};
	~SerialNumber(){};

public:
	void init(){ next_ = time(0); }

    //rand
	unsigned long get(){
        next_ = next_ * 1103515245 + 12345;
        return next_;
    }

private:
	unsigned long next_;
	
};

#define g_serial_num ( Singleton<SerialNumber>::instance_ptr() )

#endif

