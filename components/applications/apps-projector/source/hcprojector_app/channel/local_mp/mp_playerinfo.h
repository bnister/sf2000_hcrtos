/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-11-28 17:15:19
 */
#ifndef _MP_PLAYERINFO_H
#define _MP_PLAYERINFO_H

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

extern lv_obj_t *subwin_mpinfo;

lv_obj_t* create_list_sub_text_obj(lv_obj_t *parent,int w, int h,char * str1,int str_id);
lv_obj_t* create_list_sub_btn_obj(lv_obj_t *parent);
lv_obj_t* create_list_sub_btn_obj3(lv_obj_t *parent, int str1, char * str2);
lv_obj_t* create_list_obj2(lv_obj_t *parent, int w, int h);

int create_mpinfo_win(lv_obj_t *p,lv_obj_t* sub_btn);
void mpinfo_win_event_cb(lv_event_t *event);


#endif