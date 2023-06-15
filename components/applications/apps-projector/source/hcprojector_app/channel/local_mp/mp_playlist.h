/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-11-28 20:05:46
 */
#ifndef _MP_PLAYLIST_H
#define _MP_PLAYLIST_H

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif



lv_obj_t* create_playlist_win(lv_obj_t * p,lv_obj_t *sub_btn);

int playlist_init();
int playlist_deinit();
void * app_get_playlist_t();


#endif  