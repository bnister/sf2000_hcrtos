#include "app_config.h"
#ifdef PROJECTOR_VMOTOR_SUPPORT
#include <hcuapi/input-event-codes.h>
#include "setup.h"
#include "screen.h"
#include "factory_setting.h"
#include "../../mul_lang_text.h"
#include "vmotor.h"
#include "hcstring_id.h"
#include "osd_com.h"

#ifdef LVGL_RESOLUTION_240P_SUPPORT

#define VMOTOR_HEIGHT_PCT 46

#else

#define VMOTOR_HEIGHT_PCT 35
#endif

lv_obj_t *focus_list;

extern lv_timer_t *timer_setting;
extern lv_obj_t *slave_scr_obj;
extern lv_obj_t *tab_btns;
extern uint16_t tabv_act_id;
extern SCREEN_TYPE_E cur_scr;

void focus_list_init(lv_obj_t *parent);

static int sel_id = 0;
static void event_handle(lv_event_t *e){

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    uint8_t vmotor_count=0;
    if(code == LV_EVENT_KEY){
        if(timer_setting)
            lv_timer_pause(timer_setting);
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_HOME){
            turn_to_main_scr();
            return;
            
        }else if(key == LV_KEY_ESC){
            turn_to_main_scr();
            return;
        }else if(key == LV_KEY_UP || key == LV_KEY_LEFT){
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            sel_id -= 1;
            if(sel_id<0){
                lv_group_focus_prev(lv_group_get_default());
                return;
            }
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
        }else if(key == LV_KEY_DOWN || key == LV_KEY_RIGHT){
             lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            sel_id += 1;
            if(sel_id>=lv_obj_get_child_cnt(target)){
                lv_group_focus_obj(tab_btns);
                return;
            }
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);           
        }
    }else if(code == LV_EVENT_FOCUSED){
         if(act_key_code == KEY_UP){
            sel_id = lv_obj_get_child_cnt(target)-1;
        }else if(act_key_code == KEY_DOWN){
            sel_id = 0;
        }
        if(sel_id>-1)
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
    }else if(code == LV_EVENT_DEFOCUSED){
        
        if(sel_id<lv_obj_get_child_cnt(target))
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
        sel_id = 0;
    }else if(code == LV_EVENT_PRESSED){
        vMotor_set_step_count(192);
        if(sel_id==0){
            vMotor_set_direction(BMOTOR_STEP_FORWARD);

        }else{
            vMotor_set_direction(BMOTOR_STEP_BACKWARD);
        }
    }
}

void focus_list_init(lv_obj_t *parent){

    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_set_style_pad_left(obj, 0, 0);
    lv_obj_set_size(obj, LV_PCT(85), LV_PCT(VMOTOR_HEIGHT_PCT));
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);

    lv_obj_t *label = lv_label_create(obj);
    lv_obj_set_style_pad_hor(label, 0, 0);
    lv_obj_set_size(label, lv_pct(23), LV_SIZE_CONTENT);
    // language_choose_add_label1(label, STR_FOCUS);
    // set_label_text1(label, projector_get_some_sys_param(P_OSD_LANGUAGE), NULL);
    set_label_text2(label, STR_FOCUS,FONT_NORMAL);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);

    focus_list = create_list_obj1(obj, 77, 100);

    create_list_sub_text_obj4(focus_list, 100, 50,   STR_FOCUS_FORWAD);
   // lv_obj_set_style_text_font(lv_obj_get_child(focus_list, 1), &lv_font_montserrat_40,0);

    label = lv_label_create(lv_obj_get_child(focus_list, 0));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, "<");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_40,0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    label = lv_label_create(lv_obj_get_child(focus_list, 0));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, ">");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_40,0);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);


    create_list_sub_text_obj4(focus_list, 100, 50,   STR_FOCUS_BACK);
   // lv_obj_set_style_text_font(lv_obj_get_child(focus_list, 1), &lv_font_montserrat_40,0);

    label = lv_label_create(lv_obj_get_child(focus_list, 1));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, "<");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_40,0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    label = lv_label_create(lv_obj_get_child(focus_list, 1));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, ">");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_40,0);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);

     lv_obj_add_event_cb(focus_list, event_handle, LV_EVENT_ALL, NULL);
}
#endif