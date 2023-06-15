#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include "com_api.h"
#include "osd_com.h"

lv_style_t m_text_style_mid_normal;
lv_style_t m_text_style_mid_high;
lv_style_t m_text_style_mid_disable;
lv_style_t m_text_style_left_normal;
lv_style_t m_text_style_left_high;
lv_style_t m_text_style_left_disable;
lv_style_t m_text_style_right_normal;
lv_style_t m_text_style_right_high;
lv_style_t m_text_style_right_disable;

static lv_obj_t *m_obj_msg = NULL;
static lv_timer_t *msgbox_timer = NULL;
static msg_timeout_func m_msg_timeout_func = NULL;
static void *m_timeout_func_data = NULL;
static lv_obj_t *m_obj_label_msg = NULL;

// Message box function for message
static pthread_mutex_t m_mutex_msgbox = PTHREAD_MUTEX_INITIALIZER;

void osd_draw_background(void *parent, bool if_transp)
{
    if (if_transp) {//set the backgroud to transparency, may only valid 32bit colors??
        lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, LV_PART_MAIN);
        //lv_obj_set_style_bg_color(parent, LV_COLOR_CHROMA_KEY, LV_PART_MAIN);
    }
    else {
         lv_obj_set_style_bg_color(parent, COLOR_FRENCH_GREY, LV_PART_MAIN);
    }
    lv_obj_set_pos(parent, 0, 0);
    lv_obj_set_size(parent, OSD_MAX_WIDTH, OSD_MAX_HEIGHT);
    lv_obj_set_style_border_opa(parent, LV_OPA_TRANSP, LV_PART_MAIN);
}


static void msgbox_msg_clear()
{
    if (msgbox_timer){
        lv_timer_pause(msgbox_timer);
        lv_timer_del(msgbox_timer);
    }
    msgbox_timer = NULL;

    if (m_obj_msg)
        lv_obj_del(m_obj_msg);

    m_obj_msg = NULL;
}

static void msgbox_timer_cb(lv_timer_t * t)
{
    //msgbox_msg_clear();
    pthread_mutex_lock(&m_mutex_msgbox);
	msgbox_msg_clear();
	
    if (m_msg_timeout_func){
        m_msg_timeout_func(m_timeout_func_data);
    }
    pthread_mutex_unlock(&m_mutex_msgbox);
}

void win_msgbox_msg_open(const char *str_msg, uint32_t timeout, msg_timeout_func timeout_func, void *user_data)
{
    int w = OSD_MAX_WIDTH/3;
    int h = OSD_MAX_HEIGHT/3;

    win_msgbox_msg_close();

    m_obj_msg = lv_obj_create(lv_scr_act());

    lv_obj_set_style_bg_color(m_obj_msg, COLOR_DEEP_GREY, LV_PART_MAIN); //grey
    lv_obj_set_style_border_opa(m_obj_msg, LV_OPA_TRANSP,LV_PART_MAIN);
    lv_obj_set_style_radius(m_obj_msg, 10, 0);
    lv_obj_clear_flag(m_obj_msg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(m_obj_msg, w, h);
    lv_obj_center(m_obj_msg);

    lv_obj_t *txt_msg = lv_label_create(m_obj_msg);
    lv_obj_set_style_text_color(txt_msg, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(txt_msg, &lv_font_montserrat_22, 0);

    lv_obj_center(txt_msg);

    lv_label_set_text(txt_msg, str_msg);
    
    if (0 != timeout && INFINITE_VALUE != timeout){
        msgbox_timer = lv_timer_create(msgbox_timer_cb, timeout, NULL);
    }

    if (timeout_func){
        pthread_mutex_lock(&m_mutex_msgbox);
        m_msg_timeout_func = timeout_func;
        m_timeout_func_data = user_data;
        pthread_mutex_unlock(&m_mutex_msgbox);
    }
}

void win_msgbox_msg_close(void)
{
    pthread_mutex_lock(&m_mutex_msgbox);
	msgbox_msg_clear();
    m_msg_timeout_func = NULL;
    m_timeout_func_data = NULL;
    pthread_mutex_unlock(&m_mutex_msgbox);
}


void osd_obj_com_set(void)
{
    //common font setting
    lv_style_init(&m_text_style_mid_normal);
    lv_style_set_text_font(&m_text_style_mid_normal, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_mid_normal, COLOR_WHITE);
    lv_style_set_text_align(&m_text_style_mid_normal, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&m_text_style_mid_high);
    lv_style_set_text_font(&m_text_style_mid_high, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_mid_high, FONT_COLOR_HIGH);
    lv_style_set_text_align(&m_text_style_mid_high, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&m_text_style_mid_disable);
    lv_style_set_text_font(&m_text_style_mid_disable, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_mid_disable, FONT_COLOR_GRAY);
    lv_style_set_text_align(&m_text_style_mid_disable, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&m_text_style_left_normal);
    lv_style_set_text_font(&m_text_style_left_normal, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_left_normal, COLOR_WHITE);
    lv_style_set_text_align(&m_text_style_left_normal, LV_TEXT_ALIGN_LEFT);

    lv_style_init(&m_text_style_left_high);
    lv_style_set_text_font(&m_text_style_left_high, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_left_high, FONT_COLOR_HIGH);
    lv_style_set_text_align(&m_text_style_left_high, LV_TEXT_ALIGN_LEFT);

    lv_style_init(&m_text_style_left_disable);
    lv_style_set_text_font(&m_text_style_left_disable, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_left_disable, FONT_COLOR_GRAY);
    lv_style_set_text_align(&m_text_style_left_disable, LV_TEXT_ALIGN_LEFT);

    lv_style_init(&m_text_style_right_normal);
    lv_style_set_text_font(&m_text_style_right_normal, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_right_normal, COLOR_WHITE);
    lv_style_set_text_align(&m_text_style_right_normal, LV_TEXT_ALIGN_RIGHT);

    lv_style_init(&m_text_style_right_high);
    lv_style_set_text_font(&m_text_style_right_high, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_right_high, FONT_COLOR_HIGH);
    lv_style_set_text_align(&m_text_style_right_high, LV_TEXT_ALIGN_RIGHT);

    lv_style_init(&m_text_style_right_disable);
    lv_style_set_text_font(&m_text_style_right_disable, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_right_disable, FONT_COLOR_GRAY);
    lv_style_set_text_align(&m_text_style_right_disable, LV_TEXT_ALIGN_RIGHT);

}

void win_lable_pop_msg_open(char* lable_msg)
{
    if(m_obj_label_msg)
    {       
        lv_obj_del(m_obj_label_msg);
        m_obj_label_msg = NULL;
    }
    
    m_obj_label_msg = lv_label_create(lv_scr_act());
    lv_obj_align(m_obj_label_msg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(m_obj_label_msg, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(m_obj_label_msg, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(m_obj_label_msg, &lv_font_montserrat_22, 0);
    lv_label_set_text(m_obj_label_msg, lable_msg);
}

void win_lable_pop_msg_close(void)
{
    if(m_obj_label_msg)
    {       
        lv_obj_del(m_obj_label_msg);
        m_obj_label_msg = NULL;
    }
}


