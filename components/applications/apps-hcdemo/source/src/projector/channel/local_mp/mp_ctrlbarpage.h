#ifndef __MP_CTRLBARPAGEPAGE_H_
#define __MP_CTRLBARPAGEPAGE_H_

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"

#ifdef __cplusplus
extern "C" {
#endif



extern lv_group_t * main_group ;
extern lv_group_t * sub_group ;
extern lv_group_t * fs_group ;
extern lv_group_t * play_bar_group;
extern lv_indev_t * indev_keypad;


void crtl_bar_enter(lv_obj_t * parent_btn);
void ctrl_bar_keyinput_event_cb(lv_event_t *event);
void show_errorwin_event_cb(lv_event_t *event);
void format_time(uint32_t time, char *time_fmt);
void sec_timer_cb(lv_timer_t * t);
void show_play_bar(bool show);
void bar_show_timer_cb(lv_timer_t * t);
void unsupport_win_timer_cb(lv_timer_t * t);
void play_bar_open(void);
void play_bar_close(void);
void ctrlbar_reflesh_speed(void);
static int vkey_transfer_btn(uint32_t vkey);
//void medai_message_ctrl_process(void);
int preview_mp_play(int fslist_index);
int preview_mp_close(void);
#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif



