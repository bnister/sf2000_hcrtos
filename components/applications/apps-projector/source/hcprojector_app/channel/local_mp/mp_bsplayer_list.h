/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-11-28 20:28:44
 */
#ifndef _MP_BSPLAYER_LIST_H
#define _MP_BSPLAYER_LIST_H

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#define MAX_MPLIST_ITEM 12

//add a text in list 
lv_obj_t* create_list_sub_text_obj2(lv_obj_t *parent,int w, int h);




int musiclist_enter(int index);
void musiclist_win_event_cb(lv_event_t * e );


int create_musiclist_win(lv_obj_t* p,lv_obj_t* sub_btn);

void * get_bs_musiclist_t(void);
void * get_bs_osdlist_t(void);

void* app_get_bsplayer_glist(void);
int clear_all_bsplayer_mem(void);


#endif