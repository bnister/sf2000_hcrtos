//
// Created by huailong.zhou on 22-8-11.
//

#ifndef LVGL_SETUP_H
#define LVGL_SETUP_H

#include "lvgl/lvgl.h"
#include "../screen.h"

#include <sys/types.h>
#ifdef __HCRTOS__
#include <kernel/lib/fdt_api.h>
#endif

#define STR_LANGUAGE(A) #A

#define HEAD_H 30
#define NEW_WIDGET_LINE_NUM 3


enum  tab_id {
    TAB_PICTURE = 1,
    TAB_SOUND,
    TAB_SETTING,
#ifdef BLUETOOTH_SUPPORT
    TAB_BT,
#endif
#ifdef KEYSTONE_SUPPORT
    TAB_KEYSTONE,
#endif    
    TAB_MAX,
};

typedef enum list_sub_param_type_{
    LIST_PARAM_TYPE_STR,
    LIST_PARAM_TYPE_INT
} list_sub_param_type;

typedef union list_sub_param_
{
    char* str;
    int str_id;
} list_sub_param;


typedef struct mode_items_
{
    lv_obj_t *obj;
    int index;
} mode_items;


typedef struct {
    //const char* name;
    uint8_t name;
    union {
        //const char* v1;
        int16_t v1;
        int8_t v2;
    } value;
    
    
    bool is_number;
    bool is_disabled;
    void (*event_func)(lv_event_t*);
} choose_item;

typedef void(*widget)(lv_obj_t* btn);
typedef int(*btn_matrix_func)(int);
typedef void(*focus_next_or_pre)(lv_group_t*);
typedef bool(*run_cb)(void);

typedef struct {
    uint16_t len;
    uint8_t* change_lang;
    const char** str_p_p;
    const char **str_p_s;
} btnmatrix_p;

typedef struct {
    uint16_t len;
    //uint8_t* change_lang;
    const char** str_p_p;
    int *str_id_vec;
} btnmatrix_p1;

typedef struct{
    lv_font_t **font_pp;
    char **lang_pp;
} label_p;

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

typedef enum _BT_DEV_POWER_STATUS_
{
    BT_DEV_POWER_STATUS_DEFAULT=0,
    BT_DEV_POWER_STATUS_WORK,
}bt_dev_power_status;

typedef char* str_n_vec[2];
typedef str_n_vec * str_n_vec_p;


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
lv_style_set_border_color(STYLE, lv_color_make(140,140,198));\
lv_style_set_bg_color(STYLE, lv_color_make(101,101,177));\
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

// void set_label_text(lv_obj_t * label,int id, char* color);
// void set_label_text_with_font(lv_obj_t * label,int  id, char* color, lv_font_t* font);
void set_label_text1(lv_obj_t * label,int id, char* color);
void set_label_text_with_font1(lv_obj_t * label,int  id, char* color, lv_font_t* font);
// void set_label_text2(lv_obj_t* obj, uint16_t label_id, uint16_t font_id);
//btnmatrix_p* language_choose_add_label(lv_obj_t* label,const char* p, uint8_t len);
void language_choose_add_label1(lv_obj_t* obj, uint32_t label_id);
void language_choose_add_label_with_font(lv_obj_t* obj, uint32_t label_id, lv_font_t **font_pp);
//btnmatrix_p1* language_choose_add_btns(lv_obj_t* label, int *p, uint8_t len);

// void set_btnmatrix_language_with_font(lv_obj_t* obj, int id, lv_font_t *font);
// void set_btnmatrix_language(lv_obj_t * obj,int  id);
// void set_btns_lang_with_font(lv_obj_t* obj, int id, lv_font_t *font);
// void set_btns_lang(lv_obj_t *obj, int id);
void set_btns_lang2(lv_obj_t* btns, int len, int font_id, int *p);


const char* get_some_language_str(const char *str, int index);
const char* get_some_language_str1(str_n_vec str_v, int index);
void del_setup_slave_scr_obj();
extern lv_obj_t* setup_slave_root;

void keystone_screen_init(lv_obj_t*);
void focus_list_init(lv_obj_t *parent);
void turn_to_setup_root(void);
void turn_to_main_scr();
int set_color_temperature(int mode);

lv_obj_t* create_widget_head(lv_obj_t* parent, int title, int h);
void create_widget_foot(lv_obj_t* parent, int h, void* user_data);
lv_obj_t *create_widget_btnmatrix(lv_obj_t *parent,int w, int h,const int* btn_map, int len);
lv_obj_t *create_widget_btnmatrix1(lv_obj_t *parent,int w, int h,const char** btn_map);
lv_obj_t *create_new_widget(int w, int h);
lv_obj_t* create_list_obj1(lv_obj_t *parent, int w, int h);
lv_obj_t* create_list_sub_text_obj3 (lv_obj_t *parent,int w, int h, char* str1);
lv_obj_t* create_list_sub_text_obj1(lv_obj_t *parent,int w, int h, int str1, int font_id);
lv_obj_t* create_list_sub_text_obj4(lv_obj_t *parent, int w, int h, int str1);
void del_bt_wait_anim();
lv_obj_t* create_message_box(char* str);
lv_obj_t* create_message_box1(int msg_id, int btn_id1, int btn_id2, lv_event_cb_t cb,int w, int h);
lv_obj_t* loader_with_arc(char* str, lv_anim_exec_xcb_t exec_cb);
void set_display_zoom_when_sys_scale();
extern int get_display_x();
extern int get_display_y();
extern int get_display_h();
extern int get_display_v();
extern int get_cur_osd_h();
extern int get_cur_osd_v();
extern int bt_event1(unsigned long event, unsigned long param);
extern bool app_bt_is_connected();
#ifdef USB_AUTO_UPGRADE
int sys_upg_usb_check(uint32_t timeout);
#endif
#endif //LVGL_SETUP_H
