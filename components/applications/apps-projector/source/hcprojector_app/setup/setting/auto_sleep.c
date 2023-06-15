#include "screen.h"
#include "setup.h"


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "osd_com.h"

int auto_sleep_vec[] = {STR_OFF, LINE_BREAK_STR, STR_AUTO_SLEEP_1H, LINE_BREAK_STR, STR_AUTO_SLEEP_2H, LINE_BREAK_STR, STR_AUTO_SLEEP_3H,
LINE_BREAK_STR, BLANK_SPACE_STR, LINE_BREAK_STR, BLANK_SPACE_STR, BTNS_VEC_END};

static lv_obj_t *sleep_obj;
static lv_obj_t *auto_sleep_prev_obj = NULL;
static lv_timer_t *auto_sleep_sure_timer;
static lv_timer_t *auto_sleep_timer = NULL;
static int countdown = 60;

static void auto_sleep_timer_handle(lv_timer_t *tiemr_);
static int set_auto_sleep(int mode);
static void auto_sleep_btnsmatrix_event(lv_event_t *e);
void auto_sleep_widget(lv_obj_t *btn);

extern void btnmatrix_event(lv_event_t* e, btn_matrix_func f);
extern lv_obj_t *new_widget_(lv_obj_t*, int title, int *,uint32_t index, int len, int w, int h);

void auto_sleep_widget(lv_obj_t *btn){
    lv_obj_t * obj = new_widget_(btn, STR_AUTO_SLEEP, auto_sleep_vec,projector_get_some_sys_param(P_AUTOSLEEP), 12,0,0);
    lv_obj_add_event_cb(lv_obj_get_child(obj, 1), auto_sleep_btnsmatrix_event, LV_EVENT_ALL, btn);
}

static void auto_sleep_btnsmatrix_event(lv_event_t *e){
    btnmatrix_event(e, set_auto_sleep);
}

static void auto_sleep_sure_timer_handle(lv_timer_t *timer_){
    
    if(countdown==0){
        enter_standby();
        return;
    }
    lv_obj_t *label = (lv_obj_t*)timer_->user_data;
    lv_label_set_text_fmt(label, "%d second after shutdown,\n press any key to cancel", countdown--);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0);
}

static void auto_sleep_sure_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t*)lv_event_get_user_data(e);

    if(code == LV_EVENT_KEY){
        lv_timer_del(auto_sleep_sure_timer);
        auto_sleep_sure_timer = NULL;
        auto_sleep_timer = NULL;
        lv_group_focus_obj(obj);
        lv_obj_del(sleep_obj);
        countdown = 60;
    }

}

static void auto_sleep_timer_handle(lv_timer_t *timer_){//关机前一分钟弹出倒数计时，从60到0，按任何键结束计时
    
    sleep_obj = lv_obj_create(setup_slave_root);
    lv_obj_set_style_text_color(sleep_obj, lv_color_white(),0);
    lv_obj_set_size(sleep_obj, LV_PCT(33), LV_PCT(33));
    lv_obj_center(sleep_obj);
    set_pad_and_border_and_outline(sleep_obj);
    lv_obj_set_style_pad_ver(sleep_obj, 0, 0);
    lv_obj_set_style_bg_color(sleep_obj, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_bg_opa(sleep_obj, LV_OPA_90, 0);
   
    lv_obj_t* label = lv_label_create(sleep_obj);
    lv_obj_center(label);
    auto_sleep_sure_timer = lv_timer_create(auto_sleep_sure_timer_handle, 1000, label);
    lv_timer_set_repeat_count(auto_sleep_sure_timer, 61);
    lv_timer_ready(auto_sleep_sure_timer);
   
    lv_group_add_obj(lv_group_get_default(), sleep_obj);
    auto_sleep_prev_obj = lv_group_get_focused(lv_group_get_default());
    lv_group_focus_obj(sleep_obj);
    lv_obj_add_event_cb(sleep_obj, auto_sleep_sure_event_handle, LV_EVENT_ALL, auto_sleep_prev_obj);
    

}

static int set_auto_sleep(int mode){
    if(mode == AUTO_SLEEP_OFF){
        if(auto_sleep_timer){
            lv_timer_del(auto_sleep_timer);
            auto_sleep_timer = NULL;
        }
        return 0;
    }
    if(!auto_sleep_timer){
        auto_sleep_timer = lv_timer_create(auto_sleep_timer_handle, 3600000, NULL);//3600000
        lv_timer_set_repeat_count(auto_sleep_timer, 1);
        lv_timer_reset(auto_sleep_timer);
    }
	projector_set_some_sys_param(P_AUTOSLEEP,mode);
    switch (mode)
    {
    case AUTO_SLEEP_TWO_HOURS:
        lv_timer_set_period(auto_sleep_timer, 7200000);
        break;
    case AUTO_SLEEP_ONE_HOUR:
        lv_timer_set_period(auto_sleep_timer, 3600000);
        break;
    case AUTO_SLEEP_THREE_HOURS:
        lv_timer_set_period(auto_sleep_timer, 10800000);
        break;
    default:
        break;
    }
    lv_timer_reset(auto_sleep_timer);
    return 1;
}