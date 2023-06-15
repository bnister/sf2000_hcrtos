#include "app_config.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif
#include "screen.h"
#include "factory_setting.h"
#include "setup.h"
#include "mul_lang_text.h"
#include "osd_com.h"
#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define OSD_TIME_WIDGET_W 50
#else
    #define OSD_TIME_WIDGET_W 33
#endif

extern char* osd_timer_k;
extern char* osd_timer_v[14];
extern int btnmatrix_choose_id;
extern lv_timer_t *timer_setting;

int osd_timer_vec[] = {STR_OFF, LINE_BREAK_STR, STR_OSD_TIME_5S, LINE_BREAK_STR, STR_OSD_TIEM_10S, LINE_BREAK_STR,STR_OSD_TIME_15S, LINE_BREAK_STR,
STR_OSD_TIME_20S,LINE_BREAK_STR, STR_OSD_TIME_25S, LINE_BREAK_STR, STR_OSD_TIME_30S, BTNS_VEC_END};

void osd_time_widget(lv_obj_t* btn);
static void osd_time_btnmatrix_event(lv_event_t *e);
static int set_osd_time(int time); 

extern void btnmatrix_event(lv_event_t* e, btn_matrix_func func);
extern void timer_setting_handler(lv_timer_t* timer_setting1);

void osd_time_widget(lv_obj_t* btn){
    lv_obj_t* osd_time = create_new_widget(OSD_TIME_WIDGET_W, 56);
    create_widget_head(osd_time,STR_OSD_TIMER, 14);

    lv_obj_t *matrix_btn = create_widget_btnmatrix(osd_time, 100, 74, osd_timer_vec, 14);
    // char* text = lv_label_get_text(lv_obj_get_child(btn, 1));
    // char chs[10];
    // memset(chs, 0, 10);
    // strncpy(chs, text+8, strlen(text)-9);
    // const char * map;
    // for(int i=0; i<len; i++){
    //     map = get_some_language_str(btn_map[i], projector_get_some_sys_param(P_OSD_LANGUAGE));
    //     if (strcmp(chs, map) == 0){
    //         lv_btnmatrix_set_selected_btn(matrix_btn, i/2);
    //         lv_btnmatrix_set_btn_ctrl(matrix_btn, i/2, LV_BTNMATRIX_CTRL_CHECKED);
    //         btnmatrix_choose_id = i/2;
    //     }
    // }
    btnmatrix_choose_id = projector_get_some_sys_param(P_OSD_TIME);
    printf("btns choose id: %d \n", btnmatrix_choose_id);
    lv_btnmatrix_set_selected_btn(matrix_btn, btnmatrix_choose_id);
    lv_btnmatrix_set_btn_ctrl(matrix_btn, btnmatrix_choose_id, LV_BTNMATRIX_CTRL_CHECKED);
    btn->user_data = (void*)projector_get_some_sys_param(P_OSD_TIME);
    //

    create_widget_foot(osd_time, 13, btn);


    lv_obj_add_event_cb(matrix_btn, osd_time_btnmatrix_event, LV_EVENT_ALL, btn);
}

static void osd_time_btnmatrix_event(lv_event_t *e){
    btnmatrix_event(e, set_osd_time);
}
static int set_osd_time(int time){
    if(time >= OSD_TIME_OFF && time <= OSD_TIME_30S){
        projector_set_some_sys_param(P_OSD_TIME, time);
    }
    if(time==0){
        if(timer_setting){
            lv_timer_t *timer_temp = timer_setting;
            timer_setting = NULL;
            lv_timer_del(timer_temp);
        }
        return -1;
    }
    if(!timer_setting){
        timer_setting = lv_timer_create(timer_setting_handler, time*5000, 0);
        lv_timer_set_repeat_count(timer_setting, 1);
        lv_timer_reset(timer_setting);
    }else{
        lv_timer_set_period(timer_setting, time*5000);
        printf("time: %d", time);
    }
    return 0;
}