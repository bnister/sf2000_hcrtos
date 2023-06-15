#ifndef SETUP_HELPER_H_
#define SETUP_HELPER_H_

#include "lvgl/lvgl.h"

typedef void (*aspect_ratio_update_func)(lv_obj_t* obj);

typedef struct aspect_ratio_update_
{
    lv_obj_t* obj;
    aspect_ratio_update_func func;
} aspect_ratio_update_node;



extern void aspect_ratio_update_init();

extern void aspect_ratio_update_add_node(lv_obj_t *obj, aspect_ratio_update_func func);

extern void aspect_ratio_update_run();

#endif