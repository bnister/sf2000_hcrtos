#ifndef _MP_THUMBNAIL_H
#define _MP_THUMBNAIL_H

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

int thumpnail_data_handle();
int thumbnail_pthread_stop(void);
int thumbnail_pthread_start();




#endif  