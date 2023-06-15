#ifndef _MP_PREVIEW_H
#define _MP_PREVIEW_H



#define PREVIEW_W  640
#define PREVIEW_H  360
#define PREVIEW_X  640
#define PREVIEW_Y  360
#define DIS_SOURCE_W  1920
#define DIS_SOURCE_H  1080
#define DIS_SOURCE_X  0
#define DIS_SOURCE_Y  0


extern bool is_previwe_open;
extern lv_timer_t * preview_timer_handle;   /*timer handle */
extern lv_obj_t * ui_win_zoom;
extern lv_obj_t * ui_win_name;
extern lv_obj_t * ui_file_info;

int preview_init();
int preview_reset(void);
int preview_deinit(void);
void preview_timer_cb(lv_timer_t * t);
int preview_key_ctrl(uint32_t key);
void preview_player_message_ctrl(void * arg1,void *arg2);


#endif
