/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-12-06 20:18:39
 */
#ifndef __MP_FSPAGE_H_
#define __MP_FSINPAGE_H_

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"
#include "file_mgr.h"
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_FILELIST_ITEMS 12


#ifdef LVGL_RESOLUTION_240P_SUPPORT
        #define DISBALE_PREVIEW
#endif
extern file_list_t *m_cur_file_list;

extern void ebook_free_buff(void);
void *mp_get_cur_player(void);
void *mp_get_cur_player_hdl(void);
void * mp_get_cur_fullpath(void);
void* app_get_file_list();
void fs_page_keyinput_event_cb(lv_event_t *event);
void get_fsnode_name(char * path);
void clear_fslist_obj(void);
void draw_media_fslist(lv_obj_t * parent,char * dir_path);
int media_fslist_enter(int index);
void media_fslist_open(void);
void media_fslist_close(void);
uint16_t fslist_ctl_proc(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t pos);
void osd_list_update_top(obj_list_ctrl_t *list_ctrl);
void clear_fslist_path(void *path);
int label_set_long_mode_with_state(int foucus_idx);
#if 1
void previwe_open_timer_cb(lv_timer_t * t);
int previwe_close(void);
void win_previwe_create(lv_obj_t* parent);
void win_previwe_clear(void);
#endif
int app_media_list_all_free();

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif



