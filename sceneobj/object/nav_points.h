#ifndef __NAV_POINTS_H__
#define __NAV_POINTS_H__

#include "mylist.h"

struct NavPoints{
    list_head   link_;
    float x_; 
    float z_;
};

#endif /* __NAV_POINTS_H__ */

