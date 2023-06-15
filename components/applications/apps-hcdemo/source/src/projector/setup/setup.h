//
// Created by huailong.zhou on 22-8-11.
//

#ifndef LVGL_SETUP_H
#define LVGL_SETUP_H

#include "lvgl/lvgl.h"
#include "../screen.h"




#define INIT_VISIBLE() \
        visible[0]=selected_language < LANGUAGE_LEN-2 ? selected_language : selected_language < LANGUAGE_LEN-1 ? selected_language-1 : selected_language-2; \
        visible[1]=selected_language < LANGUAGE_LEN-2 ? selected_language+1 : selected_language < LANGUAGE_LEN-1 ? selected_language : selected_language-1;\
        visible[2]=selected_language < LANGUAGE_LEN-2 ? selected_language+2 : selected_language < LANGUAGE_LEN-1 ? selected_language+1 : selected_language;
#define STR_LANGUAGE(A) #A

#define HEAD_H 30
#define NEW_WIDGET_LINE_NUM 6

typedef struct mode_items_
{
    lv_obj_t *obj;
    int index;
} mode_items;


typedef struct {
    const char* name;
    union {
        const char* v1;
        int8_t v2;
    } value;
    
    
    bool is_number;
    bool is_disabled;
    void (*event_func)(lv_event_t*);
} choose_item;

typedef void(*widget)(lv_obj_t* btn);
typedef int(*btn_matrix_func)(int);
typedef void(*focus_next_or_pre)(lv_group_t*);

typedef struct {
    uint16_t len;
    uint8_t* change_lang;
    const char** str_p_p;
    const char **str_p_s;
} btnmatrix_p;

typedef enum{
    NO_USB,
    NO_SOFTWARE,
    READY_UPDATE,
} SOFTWARE_UPDATE;

typedef enum _E_BT_SCAN_STATUS_
{
    BT_SCAN_STATUS_DEFAULT,
    BT_SCAN_STATUS_GET_DATA_SEARCHING,
    BT_SCAN_STATUS_GET_DATA_SEARCHED,
    BT_SCAN_STATUS_GET_DATA_FINISHED,
}bt_scan_status;

typedef enum _E_BT_CONNECT_STATUS_
{
    BT_CONNECT_STATUS_DEFAULT,
    BT_CONNECT_STATUS_CONNECTING,
    BT_CONNECT_STATUS_DISCONNECTED,
    BT_CONNECT_STATUS_CONNECTED,
    BT_CONNECT_STATUS_GET_CONNECTED_INFO,
}bt_connect_status_e;

typedef enum _BT_DEV_POWER_STATUS_
{
    BT_DEV_POWER_STATUS_DEFAULT=0,
    BT_DEV_POWER_STATUS_WORK,
}bt_dev_power_status;


#define INIT_STYLE_BG(STYLE) \
lv_style_init(STYLE);\
lv_style_set_pad_all(STYLE, 0);\
lv_style_set_pad_gap(STYLE, 0);\
lv_style_set_border_width(STYLE, 0);\
lv_style_set_outline_width(STYLE, 0);



#define INIT_STYLE_ITEM(STYLE) \
 lv_style_init(STYLE);\
lv_style_set_radius(STYLE, 0);\
lv_style_set_border_width(STYLE, 1);\
lv_style_set_border_opa(STYLE, LV_OPA_50);\
lv_style_set_border_color(STYLE, lv_palette_darken(LV_PALETTE_GREY,2));\
lv_style_set_bg_color(STYLE, lv_palette_darken(LV_PALETTE_GREY, 1));\
lv_style_set_border_side(STYLE, LV_BORDER_SIDE_INTERNAL);

#define set_pad_and_border_and_outline(obj) do { \
    lv_obj_set_style_pad_hor(obj, 0, 0);\
    lv_obj_set_style_border_width(obj, 0, 0); \
     lv_obj_set_style_outline_width(obj,0,0);\
} while(0)

void create_balance_ball(lv_obj_t* parent, lv_coord_t radius, lv_coord_t width);
lv_obj_t * create_display_bar_widget(lv_obj_t *parent, int w, int h);
lv_obj_t *create_display_bar_name_part(lv_obj_t* parent,char* name, int w, int h);
lv_obj_t * create_display_bar_main(lv_obj_t* parent, int w, int h, int ball_count, int width);
lv_obj_t *create_display_bar_show(lv_obj_t* parent, int w, int h, int num);

void set_label_text(lv_obj_t * label,int id, char* color);
void set_label_text_with_font(lv_obj_t * label,int  id, char* color, lv_font_t* font);
btnmatrix_p* language_choose_add_label(lv_obj_t* label,const char* p, uint8_t len);
void language_choose_add_label1(lv_obj_t *label, uint32_t i);
void set_btnmatrix_language(lv_obj_t * obj,int  id);
const char* get_some_language_str(const char *str, int index);
void del_setup_slave_scr_obj();
extern lv_obj_t* slave_scr;

void keystone_screen_init(lv_obj_t*);
void turn_to_setup_scr(void);
void turn_to_main_scr();
int set_color_temperature(int mode);

lv_obj_t* create_widget_head(lv_obj_t* parent, char *title, int h);
void create_widget_foot(lv_obj_t* parent, int h, void* user_data);
lv_obj_t *create_new_widget(int w, int h);

void bt_setting_widget1(lv_obj_t *btn);
void del_bt_wait_anim();
int bt_event(unsigned long event, unsigned long param);
void create_list_sub_obj(lv_obj_t *parent, char *str);
void remove_list_sub_obj(lv_obj_t *parent, int id);
void remove_list_sub_objs(lv_obj_t *parent, int start, int end);
extern bt_connect_status_e bt_get_connet_state(void);
#endif //LVGL_SETUP_H
