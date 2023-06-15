#include "app_config.h"

#include <stdio.h>

#include "hdmi_rx.h"
#include "screen.h"
#include "setup.h"
#include "factory_setting.h"
#include "com_api.h"
#include "hcstring_id.h"
#include "osd_com.h"
#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define NO_SIG_RECT_W 25
    #define NO_SIG_RECT_H  20

#else
    #define NO_SIG_RECT_W 15
    #define NO_SIG_RECT_H 12
#endif

#define min(a, b) ((a)<(b) ? (a) : (b))
#define max(a, b) ((a)<(b) ? (b) : (a))
#define HDMI_SWITCH_HDMI_STATUS_PLUGOUT		0
#define HDMI_SWITCH_HDMI_STATUS_PLUGIN		1

lv_obj_t* hdmi_scr;
lv_group_t *hdmi_g;
lv_obj_t* no_signal_rect;

char* no_sig_str = "NO SIGNAL\0无信号\0PAS DE SIGNAL";
static char* hdmi_str = "HDMI\0高清";

static lv_timer_t* timer_start = NULL;
static lv_timer_t *timer_err_input = NULL;
static lv_coord_t h=0;
static lv_coord_t w=0;
static lv_timer_t *timer = NULL;
static bool up_dir = true;
static bool left_dir = true;

extern lv_font_t* select_font_normal[3];

static void hdmi_screen_plugoutin(void);
static void hdmi_screen_start(void);
static void timer_err_input_handle(lv_timer_t *e);

static void timer_err_input_handle(lv_timer_t *e){
    lv_obj_t *msgbox = (lv_obj_t*)e->user_data;
    lv_msgbox_close(msgbox);
    if(lv_obj_has_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN)){
        lv_obj_clear_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN);
    }        
    timer_err_input = NULL;
}

static void timer_handle(lv_timer_t *e){
    if(hdmirx_get_plug_status() == HDMI_SWITCH_HDMI_STATUS_ERR_INPUT){
        if(lv_obj_has_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN)){
            if(timer_err_input == NULL){
                lv_obj_t* msgbox = lv_msgbox_create(lv_layer_top(), NULL, api_rsc_string_get(STR_HDMI_FORMAT_USPT), NULL, false);
                lv_obj_set_style_text_font(msgbox, osd_font_get(FONT_MID), 0);

                timer_err_input = lv_timer_create(timer_err_input_handle, 3000, msgbox);
                lv_timer_set_repeat_count(timer_err_input, 1);
                lv_timer_reset(timer_err_input);
                
            }
        return;
        }
    }
    if(hdmirx_get_plug_status() == HDMI_SWITCH_HDMI_STATUS_PLUGIN){
        if(!lv_obj_has_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN)){
             lv_obj_add_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN);
        }        
        return;
    }

    if(lv_obj_has_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN)){
        lv_obj_clear_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN);
    }

    lv_coord_t p_h = lv_obj_get_x(no_signal_rect);
    lv_coord_t p_v = lv_obj_get_y(no_signal_rect);
    if(up_dir){
        p_v = max(0, p_v - 10);
        if(p_v == 0){
            up_dir = false;
        }
    }else{
        p_v = min(h/6*5, p_v+10);
        if(p_v == h/6*5){
            up_dir = true;
        }
    }

    if(left_dir){
        p_h = max(0, p_h - 20);
        if(p_h == 0){
            left_dir = false;
        }
    }else{
        p_h = min(w/5*4, p_h + 20);
        if(p_h == w/5*4){
            left_dir = true;
        }
    }
    lv_obj_set_pos(no_signal_rect, p_h, p_v);
}

static void timer_handle1(lv_timer_t* timer_){
    timer_start = NULL;
    lv_obj_clean(hdmi_scr);
    hdmi_screen_plugoutin();
    if ( hdmirx_get_plug_status() == HDMI_SWITCH_HDMI_STATUS_PLUGIN){
        lv_obj_add_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN);        
    }
    
}

static void timer_handle2(lv_timer_t *time_){
    enter_standby();
}

static void hdmi_event_handle(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SCREEN_LOAD_START){
      key_set_group(hdmi_g);
        hdmi_screen_start();
        hdmi_rx_enter();
        timer_start = lv_timer_create(timer_handle1, 3000, 0);
        lv_timer_set_repeat_count(timer_start, 1);
        lv_timer_reset(timer_start);
    }else if(code == LV_EVENT_SCREEN_UNLOAD_START){
        if(timer){
            lv_timer_del(timer);
            timer = NULL;
        }
        if(timer_start){
            lv_timer_del(timer_start);
            timer_start = NULL;
        }
		// switch hrx quickly, pause instead of rx_leave
        hdmirx_pause();//hdmi_rx_leave();
        if(timer_err_input){
            lv_timer_ready(timer_err_input);
        }
        lv_obj_clean(hdmi_scr);
        if(lv_obj_has_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN)){
            lv_obj_clear_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN);
        }
    }else if(code == LV_EVENT_KEY){
        uint8_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
        }
    }
}

static void hdmi_screen_start(void){
    lv_obj_t *obj = lv_obj_create(hdmi_scr);

    lv_obj_set_style_bg_color(obj, lv_palette_lighten(LV_PALETTE_BLUE, 1), 0);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(8));
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t* label = lv_label_create(obj);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(label, api_rsc_string_get(STR_HDMI_TITLE));
    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID),0);
}



static void hdmi_screen_plugoutin(void){
    no_signal_rect = lv_obj_create(hdmi_scr);
    
    lv_obj_set_size(no_signal_rect, LV_PCT(NO_SIG_RECT_W), LV_PCT(NO_SIG_RECT_H));
    lv_obj_set_style_bg_color(no_signal_rect, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_radius(no_signal_rect, 5, 0);
    timer = lv_timer_create(timer_handle, 1000, NULL);
    lv_timer_set_repeat_count(timer, -1);
    lv_timer_ready(timer);



    lv_obj_t *label = lv_label_create(no_signal_rect);
    lv_obj_center(label);
    lv_label_set_text(label, api_rsc_string_get(STR_NO_SIGNAL));

    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);
}


void hdmi_screen_init(void)
{
    hdmi_scr = lv_obj_create(NULL);
    hdmi_g = lv_group_create();
    lv_group_t *g = lv_group_get_default();
    lv_group_set_default(hdmi_g);
    lv_group_add_obj(hdmi_g, hdmi_scr);
    lv_obj_set_style_bg_opa(hdmi_scr, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(hdmi_scr, hdmi_event_handle, LV_EVENT_ALL, 0);

    h = lv_disp_get_ver_res(lv_disp_get_default());
    w = lv_disp_get_hor_res(lv_disp_get_default());

    lv_group_set_default(g);
   
}

