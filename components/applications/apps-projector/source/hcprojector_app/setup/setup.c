#include "app_config.h"

#include "lvgl/lvgl.h"
#include "screen.h"
#include "setup.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hcuapi/snd.h>
#include <sys/types.h>

#include "app_config.h"
#include <sys/stat.h>

#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif
#include <hcfota.h>

#ifdef __HCRTOS__
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <kernel/lib/fdt_api.h>
#endif

#include <pthread.h>

#include "factory_setting.h"
#include "../channel/local_mp/local_mp_ui.h"
#include "com_api.h"
#include "osd_com.h"
#include "mul_lang_text.h"
#include "vmotor.h"
#include "hcstring_id.h"


#define FLIP_MODE_LEN 4

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define BTN_WIDTH_PCT 97
    #define TAB_SIZE_PCT  13
    #define VERSION_INFO_FONT &lv_font_montserrat_10
    #define TAB_BTNS_PAD_WIDTH_FACTOR 14
    #define ENG_FONT_NORMAL &lv_font_montserrat_14
    #define ENG_FONT_MIDDLE &lv_font_montserrat_10
    #define WIDGET_STR_FOOT_OFFSET_Y 0
    #define WIDGET_STR_FOOT_OFFSET_X 2
    #define SETUP_WIDGET_DEFAULT_W 50
    #define SETUP_WIDGET_DEFAULT_H 55
    #define SETUP_ITEM_VALUE_W_PCT -1
    #define SETUP_ITEM_VALUE_ALING_RIGHT 0
    #define ADJUST_BAR_W_PCT 8
    
#else
    #define BTN_WIDTH_PCT 80
    #define TAB_SIZE_PCT  8
    #define VERSION_INFO_FONT &lv_font_montserrat_26
    #define TAB_BTNS_PAD_WIDTH_FACTOR 11
    #define ENG_FONT_NORMAL &lv_font_montserrat_26
    #define ENG_FONT_MIDDLE &lv_font_montserrat_22
    #define WIDGET_STR_FOOT_OFFSET_Y -3
    #define WIDGET_STR_FOOT_OFFSET_X 5
    #define SETUP_WIDGET_DEFAULT_W 33
    #define SETUP_WIDGET_DEFAULT_H 50
    #define SETUP_ITEM_VALUE_W_PCT 45
    #define SETUP_ITEM_VALUE_ALING_RIGHT 1
    #define ADJUST_BAR_W_PCT 5
#endif

//#define ASPECT_RATIO_DISABLE 0




uint16_t tabv_act_id = TAB_MAX/2;

lv_obj_t *setup_scr = NULL;
lv_obj_t *setup_root = NULL;
lv_group_t *setup_g = NULL;
lv_timer_t *timer_setting = NULL, *timer_setting_clone = NULL;


static sys_param_t *sys_param_p;
lv_obj_t* setup_slave_root = NULL;
static uint16_t last_setting_key=0;
int btnmatrix_choose_id = 0;
static lv_obj_t *cur_btn = NULL;
extern lv_obj_t *bluetooth_obj;
extern lv_obj_t* bt_list_obj;
extern lv_obj_t* keystone_list;
extern language_type_t selected_language;
int color_temp_vec[12] = {STR_COLD, LINE_BREAK_STR, STR_STANDARD, LINE_BREAK_STR, STR_WARM, LINE_BREAK_STR
                            , BLANK_SPACE_STR, LINE_BREAK_STR, BLANK_SPACE_STR, LINE_BREAK_STR, BLANK_SPACE_STR, BTNS_VEC_END};
                             
extern int picture_mode_vec[];
extern int sound_mode_vec[];
extern int auto_sleep_vec[];
extern int osd_timer_vec[];
//extern int osd_lang_id_vec[];
int bt_v = STR_OFF;
int flip_vec[] = {STR_FRONT_TABLE, STR_FRONT_CEILING, STR_BACK_TABLE, STR_BACK_CEILING};
static int flip_mode_vec[FLIP_MODE_LEN] = {FLIP_MODE_REAR, FLIP_MODE_CEILING_REAR, FLIP_MODE_FRONT, FLIP_MODE_CEILING_FRONT};

mode_items picture_mode_items[4];
mode_items sound_mode_items[2];


static int selected_page = 0;
static lv_obj_t *pg1, *pg2, *pg3, *pg4, *pg5;
static lv_obj_t *pgs[TAB_MAX-1];

lv_obj_t *tab_btns, *foot;
static lv_obj_t* tabv = NULL;
static lv_style_t style_bg;

LV_IMG_DECLARE(MAINMENU_IMG_PICTURE)
LV_IMG_DECLARE(MAINMENU_IMG_PICTURE_FOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_PICTURE_S_UNFOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_OPTIONS_S_UNFOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_OPTION_FOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_AUDIO_S_UNFOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_AUDIO_FOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_AUDIO)
LV_IMG_DECLARE(MAINMENU_IMG_OPTIONS)
LV_IMG_DECLARE(MENU_IMG_LIST_MENU)
LV_IMG_DECLARE( MENU_IMG_POP_UP_RIGHT_ARROW)
LV_IMG_DECLARE(MENU_IMG_POP_UP_LEFT_ARROW)
LV_IMG_DECLARE(MENU_IMG_LIST_TABLE)
LV_IMG_DECLARE(MENU_IMG_LIST_OK)
LV_IMG_DECLARE(MENU_IMG_LIST_MOVE)
LV_IMG_DECLARE(bt_unfocus)
LV_IMG_DECLARE(bt_focus)
LV_IMG_DECLARE(keystone_unfocus)
LV_IMG_DECLARE(keystone_focus)

static lv_style_t style_item;

extern int set_flip_mode(flip_mode_e mode);
extern int set_keystone(int top_w, int bottom_w);
extern int set_color_temperature(int mode);
extern int set_twotone(int mode, int bass, int treble);

static void timer_cb1(lv_timer_t *timer);
static void timer_cb2(lv_timer_t *timer);

static void create_setup(void);
static lv_obj_t* add_adjust_num_obj(lv_obj_t *parent,  uint32_t data);
//static void clear_setup(void);
static void event_handler(lv_event_t* e);
static void slave_scr_event_handler(lv_event_t *e);
static void time_cb(lv_timer_t* timer_setting);
// void noise_reduction_btnmatrix_event(lv_event_t* e);

void bt_setting_btnmatrix_event(lv_event_t* e);
void color_temp_btnmatrix_event(lv_event_t* e);

// int display_bar_event(lv_event_t* e, int num, int lower, int high, int div, int width);
void picture_mode_event(lv_event_t* e);
void contrast_event(lv_event_t* e);
void brightness_event(lv_event_t* e);
void color_event(lv_event_t* e);
void hue_event(lv_event_t* e);
void sharpness_event(lv_event_t* e);
void color_temperature_event(lv_event_t* e);
// void noise_reduction_event(lv_event_t* e);
// int set_noise_redu(int v);
void sound_mode_event(lv_event_t* e);

void treble_event(lv_event_t* e);
void version_info_event(lv_event_t *e);
void bass_event(lv_event_t* e);
void balance_event(lv_event_t* e);

void change_volume_event(lv_event_t* e);

void setup_item_event_(lv_event_t* e, widget widget1, int item);

extern bool str_is_black(char *str);

void osd_language_event(lv_event_t* e);
void foot_event(lv_event_t* e);
void flip_event(lv_event_t* e);
//void aspect_radio_event(lv_event_t* e);
//static void aspect_ratio_btnmatrix_event(lv_event_t *e);
//static int aspect_ratio_btnmatrix_event_(int);
void restore_factory_default_event(lv_event_t* e);
void software_update_event(lv_event_t* e);
static void software_update_bar_event(lv_event_t* e);
void keystone_event(lv_event_t *e);
void auto_sleep_event(lv_event_t *e);
void osd_time_event(lv_event_t *e);
void window_scale_event(lv_event_t *e);
static int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata);
void btnmatrix_event(lv_event_t* e, btn_matrix_func f);
void return_event(lv_event_t* e);
void tabv_event(lv_event_t* e);
void tabv_btns_event(lv_event_t* e);
void btn_event(lv_event_t* e);
void main_scr_event(lv_event_t* e);
static void set_picture(const void *scr, lv_obj_draw_part_dsc_t *dsc);
static bool obj_has_ancestor(lv_obj_t *self, lv_obj_t *ancestor);
static bool is_digit(const char* str);
extern void set_enhance1(int value, uint8_t op);
extern int set_twotone(int mode, int bass, int treble);
extern int set_balance(int v);

lv_obj_t* create_item(lv_obj_t* section, choose_item * chooseItem);
void set_adjustable_value(uint32_t item);
lv_obj_t *new_widget_(lv_obj_t*, int title,const int*,uint32_t index, int len, int w, int h);
lv_obj_t *create_widget_btnmatrix(lv_obj_t *parent,int w, int h,const int* btn_map, int len);
extern void picture_mode_widget( lv_obj_t*);
void volume_widget();
void color_temp_widget(lv_obj_t*);
lv_obj_t* create_picutre_page(lv_obj_t* parent);
lv_obj_t* create_sound_page(lv_obj_t* parent);
lv_obj_t* create_setting_page(lv_obj_t* parent);
lv_obj_t* create_keystone_page(lv_obj_t* parent);
extern lv_obj_t* create_bt_page(lv_obj_t* parent);
lv_obj_t* create_page_(lv_obj_t* parent, choose_item * data, int len);
extern void sound_mode_widget(lv_obj_t* e);
extern void osd_language_widget(lv_obj_t* e);
void flip_widget(lv_obj_t* e);
//void aspect_ratio_widget(lv_obj_t* e);
void restore_factory_default_widget(lv_obj_t* e);
extern void software_update_widget(lv_obj_t* e);
extern void auto_sleep_widget(lv_obj_t *btn);
void swap_color(lv_color_t* color1, lv_color_t* color2);
void flip(lv_color_t* buf, int dir);
void delete_from_list_event(lv_event_t* e);
void timer_setting_handler(lv_timer_t* timer_setting1);
static void msg_timer_handle(lv_timer_t *timer_);
static lv_obj_t* create_list_sub_text_obj(lv_obj_t *parent,int w, int h, list_sub_param param, int type, int font_id);

static lv_obj_t* create_root_obj(lv_obj_t* scr){
    lv_obj_t* root = lv_obj_create(scr);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_center(root);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_pad_gap(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);

    return root;     
}

void setup_screen_init(void){
    sys_param_p =  projector_get_sys_param();
    setup_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_opa(setup_scr, LV_OPA_0, 0);
    
    setup_root = create_root_obj(setup_scr);
    lv_obj_set_style_bg_opa(setup_root, LV_OPA_50, 0);
    lv_obj_set_style_bg_color(setup_root, lv_color_make(32,32,64), 0);    

    setup_slave_root = create_root_obj(setup_scr);
    lv_obj_set_style_bg_opa(setup_slave_root, LV_OPA_0, 0);

    lv_obj_add_event_cb(setup_scr, event_handler, LV_EVENT_SCREEN_LOADED, 0);
    lv_obj_add_event_cb(setup_scr, event_handler, LV_EVENT_SCREEN_UNLOADED, 0);

    //MY_EVENT_LOADED = lv_event_register_id();
    setup_g = lv_group_create();
    lv_group_t* g = lv_group_get_default();
    create_setup();
    lv_group_set_default(g);
    set_flip_mode(projector_get_some_sys_param(P_FLIP_MODE));
}

static void create_setup(void){
        lv_group_set_default(setup_g);

        tabv = lv_tabview_create(setup_root, LV_DIR_TOP,LV_PCT(TAB_SIZE_PCT));
        set_pad_and_border_and_outline(tabv);
        //lv_obj_set_style_pad_ver(tabv, 0, 0);
        lv_obj_set_style_bg_opa(tabv, LV_OPA_50, 0);
        lv_obj_set_size(tabv,LV_PCT(100),LV_PCT(93));
        lv_obj_set_style_bg_color(tabv,  lv_color_make(0,0,0), 0);
        lv_obj_set_pos(tabv, 0, 0);
        
        lv_tabview_add_tab(tabv, "#ffffff <#");
        pgs[TAB_PICTURE-1] = lv_tabview_add_tab(tabv, " ");
        pgs[TAB_SOUND-1] = lv_tabview_add_tab(tabv, " ");
        pgs[TAB_SETTING-1] = lv_tabview_add_tab(tabv, " ");
    #ifdef BLUETOOTH_SUPPORT      
        pgs[TAB_BT-1] = lv_tabview_add_tab(tabv, " ");
    #endif
	
#ifdef KEYSTONE_SUPPORT
        pgs[TAB_KEYSTONE-1] = lv_tabview_add_tab(tabv, " ");
#endif
        lv_tabview_add_tab(tabv, "#ffffff >#");

        
        create_picutre_page(pgs[TAB_PICTURE-1]);
        create_sound_page(pgs[TAB_SOUND-1]);
        create_setting_page(pgs[TAB_SETTING-1]);
    #ifdef BLUETOOTH_SUPPORT
        create_bt_page(pgs[TAB_BT-1]);
    #endif
#ifdef KEYSTONE_SUPPORT
        create_keystone_page(pgs[TAB_KEYSTONE-1]);
#endif
        lv_coord_t pad_width = (lv_coord_t)(lv_disp_get_hor_res(NULL)/TAB_BTNS_PAD_WIDTH_FACTOR);

        tab_btns = lv_tabview_get_tab_btns(tabv);
        lv_group_focus_obj(tab_btns);
        
        lv_btnmatrix_set_btn_ctrl_all(tab_btns, LV_BTNMATRIX_CTRL_RECOLOR);
        lv_btnmatrix_clear_btn_ctrl_all(tab_btns, LV_BTNMATRIX_CTRL_CHECKABLE);
        lv_obj_set_style_pad_hor(tab_btns,(lv_coord_t)(pad_width*4), 0);
        lv_obj_set_style_bg_color(tab_btns, lv_color_make(32,32,64), 0);
        lv_obj_add_event_cb(tab_btns, tabv_btns_event, LV_EVENT_ALL, 0);
        
        foot = lv_obj_create(setup_root);
        lv_obj_set_size(foot,LV_PCT(100),LV_PCT(8));
        lv_obj_align(foot,  LV_ALIGN_BOTTOM_MID, 0, 0);//-LV_PCT(8)
        lv_obj_set_style_bg_color(foot, lv_color_make(32,32,64), 0);
        lv_obj_set_style_bg_opa(foot, LV_OPA_100, 0);
        lv_obj_set_style_border_width(foot, 2, 0);
        lv_obj_set_style_border_side(foot, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(foot, lv_color_make(32,32,64), 0);
        lv_obj_set_style_border_opa(foot, LV_OPA_100, 0);
        lv_obj_set_style_outline_width(foot, 0 ,0);
        lv_obj_set_style_pad_all(foot, 0, 0);
        lv_obj_set_style_pad_gap(foot,0,0);
        lv_obj_set_style_radius(foot,0,0 );
      
        lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
        lv_obj_t *obj,*label,*img;
        lv_img_dsc_t* img_dsc[4] = {&MENU_IMG_LIST_MOVE, &MENU_IMG_LIST_TABLE, &MENU_IMG_LIST_OK, &MENU_IMG_LIST_MENU};

        int foot_map[4] = {STR_FOOT_MOVE, STR_FOOT_MENU, STR_FOOT_SURE, STR_FOOT_OFF};
        for(int i=0; i<4;i++){
            obj = lv_obj_create(foot);
            lv_obj_set_size(obj, LV_PCT(24), LV_PCT(100));
            lv_obj_set_style_pad_ver(obj, 2, 0);
            lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
            //lv_obj_set_style_pad_hor(obj, LV_PCT(20), 0);
            set_pad_and_border_and_outline(obj);
            
            lv_obj_set_style_radius(obj, 0, 0);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);

            img = lv_img_create(obj);
            lv_img_set_src(img, img_dsc[i]);
            //lv_obj_align(img, LV_ALIGN_TOP_LEFT, LV_PCT(30), 0);
            label = lv_label_create(obj);
            lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_MID, 2, -10);
            lv_label_set_recolor(label, true);
            lv_obj_set_style_text_color(label, lv_color_white(), 0);
    
            //language_choose_add_label1(label, );
            set_label_text2(label, foot_map[i], FONT_NORMAL);
        }

       
}


void create_balance_ball(lv_obj_t* parent, lv_coord_t radius, lv_coord_t width){
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_size(obj,LV_PCT(width),LV_PCT(100));
    lv_obj_set_style_bg_color(obj, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);
}

lv_obj_t * create_display_bar_widget(lv_obj_t *parent, int w, int h){
    lv_obj_t *balance = lv_obj_create(parent);

    lv_obj_set_style_radius(balance, 0, 0);
    lv_obj_set_size(balance,LV_PCT(w),LV_PCT(h));
    lv_obj_set_style_outline_width(balance, 0, 0);
    lv_obj_align(balance, LV_ALIGN_BOTTOM_MID, LV_PCT(1), 0);
    lv_obj_set_style_bg_color(balance, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_bg_opa(balance, LV_OPA_50, 0);
    lv_obj_set_flex_flow(balance, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(balance, 0, 0);
    lv_obj_set_style_pad_gap(balance, 0, 0);
    lv_obj_set_flex_align(balance,LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    return balance;
}

lv_obj_t *create_display_bar_name_part(lv_obj_t* parent,char* name, int w, int h){
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    set_pad_and_border_and_outline(obj);
    lv_obj_align(obj, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(obj,0,0);
    lv_obj_set_size(obj,LV_PCT(w),LV_PCT(h));
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);

    lv_obj_t *label = lv_label_create(obj);

    lv_label_set_text(label, name);
    lv_obj_set_style_text_font(label,osd_font_get(FONT_MID), 0);
    lv_obj_center(label);

    return obj;
}

lv_obj_t * create_display_bar_main(lv_obj_t* parent, int w, int h, int ball_count, int width){
    lv_obj_t *container = lv_obj_create(parent);
    for(int i=0; i<ball_count; i++){
        create_balance_ball(container, 8, width);
    }
    // if(ball_count>0){
    //      lv_obj_t* first = lv_obj_get_child(container, 0);
    //      lv_obj_align(first, LV_ALIGN_LEFT_MID, 5, 0);
    // }
   

    lv_obj_set_style_bg_color(container, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_size(container,LV_PCT(w),LV_PCT(h));
    //set_pad_and_border_and_outline(container);
    lv_obj_set_style_outline_width(container, 0, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_ver(container, 0, 0);
    lv_obj_set_style_pad_gap(container, 1, 0);
    lv_obj_set_style_pad_hor(container, 1, 0);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    
    lv_group_add_obj(lv_group_get_default(), container);
    lv_group_focus_obj(container);
    return container;
}

lv_obj_t *create_display_bar_show(lv_obj_t* parent, int w, int h, int num){
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(obj,LV_PCT(w),LV_PCT(h));
    set_pad_and_border_and_outline(obj);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
  
     lv_obj_t *label = lv_label_create(obj);
    char s[4];
    memset(s, 0, 4);
    sprintf(s, "%d", num);
    lv_label_set_text(label, s);
    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);
    lv_obj_center(label);
 
    return obj;
}


lv_obj_t* slave_scr_obj = NULL;
void del_setup_slave_scr_obj(){
     if(slave_scr_obj){
        lv_obj_del(slave_scr_obj);
        turn_to_setup_root();
        slave_scr_obj = NULL;
    }
#ifdef BLUETOOTH_SUPPORT
    del_bt_wait_anim();
#endif
}
void timer_setting_handler(lv_timer_t* timer_setting1){
   
    if(timer_setting){
        printf("return main\n");
        change_screen(projector_get_some_sys_param(P_CUR_CHANNEL));
        del_setup_slave_scr_obj();
        lv_obj_set_style_bg_opa(setup_slave_root, LV_OPA_0, 0);
        lv_obj_clear_flag(setup_root, LV_OBJ_FLAG_HIDDEN);
        timer_setting = NULL;
        if(cur_btn && lv_obj_has_state(cur_btn, LV_STATE_USER_1)){
            lv_obj_clear_state(cur_btn, LV_STATE_USER_1);
            lv_obj_set_style_bg_opa(cur_btn, LV_OPA_0, 0);
            lv_obj_set_style_border_opa(cur_btn, LV_OPA_0, 0);
        }
    }

}

static void time_cb(lv_timer_t* timer_setting1){
    turn_to_setup_root();
}


static void event_handler(lv_event_t* e){
    #ifdef BLUETOOTH_SUPPORT
extern void bt_screen_event_handle(lv_event_t *e);
    bt_screen_event_handle(e);
    #endif
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SCREEN_LOADED){
         key_set_group(setup_g);
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
            lv_group_focus_obj(cur_btn);
            return;
        }
        lv_group_focus_obj(tab_btns);
        lv_btnmatrix_set_selected_btn(tab_btns, tabv_act_id);
        lv_tabview_set_act(tabv, tabv_act_id,  LV_ANIM_OFF);
#ifdef KEYSTONE_SUPPORT		
        if(tabv_act_id == TAB_KEYSTONE){
            lv_group_focus_obj(keystone_list);
        }
#endif		
        tabv_act_id = TAB_MAX/2;
        if(cur_btn && lv_obj_has_state(cur_btn, LV_STATE_USER_1)){
            lv_obj_clear_state(cur_btn, LV_STATE_USER_1);
            lv_obj_set_style_bg_opa(cur_btn, LV_OPA_0, 0);
            lv_obj_set_style_border_opa(cur_btn, LV_OPA_0, 0);
            //lv_obj_set_style_bg_color(cur_btn, lv_color_make(100,99,100), 0);
        }
        if(projector_get_some_sys_param(P_OSD_TIME) != OSD_TIME_OFF){
            timer_setting = lv_timer_create(timer_setting_handler, projector_get_some_sys_param(P_OSD_TIME)*5000, 0);
            lv_timer_set_repeat_count(timer_setting, 1);
            lv_timer_reset(timer_setting);
        }
    } else if(code == LV_EVENT_SCREEN_UNLOADED){
        if(timer_setting){
            printf("DEL TIMER\n");
            lv_timer_del(timer_setting);
            timer_setting = NULL;
        }
        lv_obj_set_style_bg_opa(setup_slave_root, LV_OPA_0, 0);
    }
}



lv_obj_t* create_picutre_page(lv_obj_t* parent){
    bool is_disable = projector_get_some_sys_param(P_PICTURE_MODE) ==  PICTURE_MODE_USER ? false : true;
    choose_item picture_items[] = {
            {.name=STR_PICTURE_MODE,.value.v1=picture_mode_vec[projector_get_some_sys_param(P_PICTURE_MODE)*2],.is_number = false,.is_disabled= false,.event_func=picture_mode_event},

            {.name=STR_CONSTRAST, .value.v2 = projector_get_some_sys_param(P_CONTRAST),.is_number=true,.is_disabled=is_disable, .event_func=contrast_event},

            {.name=STR_BRIGHTNESS, .value.v2 = projector_get_some_sys_param(P_BRIGHTNESS),.is_number=true, .is_disabled=is_disable,.event_func= brightness_event},

            {.name=STR_COLOR, .value.v2 = projector_get_some_sys_param(P_COLOR),.is_number=true, .is_disabled=is_disable, .event_func=color_event},

            {.name=STR_SHARPNESS, .value.v2 = projector_get_some_sys_param(P_SHARPNESS),.is_number=true, .is_disabled=is_disable, .event_func=sharpness_event},
            
            //{.name=STR_HUE, .value.v2=BLANK_SPACE_STR, .is_number=true, .is_disabled=true,.event_func=hue_event},

            {.name=STR_COLOR_TEMP, .value.v1=color_temp_vec[projector_get_some_sys_param(P_COLOR_TEMP)*2],.is_number=false,.is_disabled=false, .event_func=color_temperature_event},
    };
    set_enhance1(projector_get_some_sys_param(P_CONTRAST), P_CONTRAST);
    set_enhance1(projector_get_some_sys_param(P_BRIGHTNESS), P_BRIGHTNESS);
    set_enhance1(projector_get_some_sys_param(P_COLOR), P_COLOR);
    set_enhance1(projector_get_some_sys_param(P_SHARPNESS), P_SHARPNESS);
    set_enhance1(projector_get_some_sys_param(P_HUE), P_HUE);
    set_color_temperature(projector_get_some_sys_param(P_COLOR_TEMP));
    lv_obj_t * obj = create_page_(parent, picture_items, 6);
    int obj_indexs[4] = {P_CONTRAST, P_BRIGHTNESS, P_COLOR, P_SHARPNESS};
    lv_obj_t *drop_down_obj;
    for(int i=0; i<4; i++){
        picture_mode_items[i].obj = lv_obj_get_child(obj, i+1);
        picture_mode_items[i].index = obj_indexs[i];
    }

    lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(parent, 0), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    set_label_text2(label, STR_PICTURE_MODE_TITLE, FONT_NORMAL);
    lv_obj_t *icon = lv_img_create(lv_obj_get_child(parent, 1));
    lv_img_set_src(icon, &MAINMENU_IMG_PICTURE);
    lv_obj_center(icon);
    return obj;
}




lv_obj_t* create_sound_page(lv_obj_t* parent){
    
    bool is_disable = projector_get_some_sys_param(P_SOUND_MODE) == SOUND_MODE_USER ? false : true;
    
    if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON){
        bt_v =  STR_ON;
    }
    choose_item music_items[] = {
            {STR_SOUND_MODE,.value.v1=sound_mode_vec[projector_get_some_sys_param(P_SOUND_MODE)*2],false, false, sound_mode_event},
            {STR_TREBLE, .value.v2=projector_get_some_sys_param(P_TREBLE),true, is_disable, treble_event},//.value.v2=projector_get_some_sys_param(P_TREBLE) 
            {STR_BASS,  .value.v2=projector_get_some_sys_param(P_BASS), true, is_disable, bass_event},//.value.v2=projector_get_some_sys_param(P_BASS)
            {STR_BALANCE, .value.v2=projector_get_some_sys_param(P_BALANCE), true, false, balance_event},
    };
    set_twotone(SND_TWOTONE_MODE_USER, projector_get_some_sys_param(P_TREBLE), projector_get_some_sys_param(P_BASS));
    set_balance(projector_get_some_sys_param(P_BALANCE));
   
    lv_obj_t *obj = create_page_(parent, music_items, 4);
    int obj_index[2] = {P_TREBLE, P_BASS};
    lv_obj_t* drop_down_obj;
     for(int i=0;i<2;i++){
        sound_mode_items[i].obj = lv_obj_get_child(obj, i+1);
        sound_mode_items[i].index = obj_index[i];
    }

    lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(parent, 0), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    set_label_text2(label, STR_SOUND_MODE_TITLE, FONT_NORMAL);
    lv_obj_t *icon = lv_img_create(lv_obj_get_child(parent, 1));
    lv_img_set_src(icon, &MAINMENU_IMG_AUDIO);

    lv_obj_center(icon);
    return obj;
}

lv_obj_t* create_keystone_page(lv_obj_t* parent){
    lv_obj_t *obj = create_page_(parent, NULL, 0);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(obj, lv_obj_get_height(obj)/10, 0);
    keystone_screen_init(obj);
    #ifdef PROJECTOR_VMOTOR_SUPPORT
    focus_list_init(obj);
    vMotor_init();
    #endif
    
    printf("keystone-dir: %d, keystone-step: %d", projector_get_some_sys_param(P_KEYSTONE_TOP), projector_get_some_sys_param(P_KEYSTOME_BOTTOM));
    set_keystone(projector_get_some_sys_param(P_KEYSTONE_TOP), projector_get_some_sys_param(P_KEYSTOME_BOTTOM));
   
   
    lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(parent, 0), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    #ifdef PROJECTOR_VMOTOR_SUPPORT
    int i =  STR_ADJUST_MODE_TITLE;
    #else
    int i = STR_KEYSTONE;
    #endif
    set_label_text2(label, i, FONT_NORMAL);
    return obj;


}

lv_obj_t* create_setting_page(lv_obj_t* parent){
    choose_item setting_items[] = {
            {STR_OSD_LANG, .value.v1=STR_LANG,false , false, osd_language_event},
            {STR_FLIP, .value.v1=flip_vec[projector_get_some_sys_param(P_FLIP_MODE)],false, false, flip_event},
            //{STR_ASPECT_RATIO , .value.v1=aspect_vec[projector_get_some_sys_param(P_ASPECT_RATIO)*2],false, ASPECT_RATIO_DISABLE, aspect_radio_event},
            {STR_RESTORE_FACTORY_DEFAULT, .value.v1=BLANK_SPACE_STR ,false,false, restore_factory_default_event},
            {STR_UPGRADE, .value.v1=BLANK_SPACE_STR,false, false, software_update_event},
            {STR_AUTO_SLEEP, .value.v1=auto_sleep_vec[projector_get_some_sys_param(P_AUTOSLEEP)*2], false, false, auto_sleep_event},
            {STR_OSD_TIMER, .value.v1=osd_timer_vec[projector_get_some_sys_param(P_OSD_TIME)*2], false, false, osd_time_event},
            #ifdef SYS_ZOOM_SUPPORT
            {STR_WINDOW_SCALE,.value.v1=BLANK_SPACE_STR,false, false, window_scale_event},
            #endif
            {STR_VERSION_INFO, .value.v1=BLANK_SPACE_STR, false, false, version_info_event}
    };
    //set_aspect_ratio(projector_get_some_sys_param(P_ASPECT_RATIO));
   //aspect_ratio_btnmatrix_event_(projector_get_some_sys_param(P_ASPECT_RATIO));
    lv_obj_t *obj = create_page_(parent, setting_items, sizeof(setting_items)/sizeof(choose_item));

    lv_label_set_text(lv_obj_get_child(lv_obj_get_child(obj,sizeof(setting_items)/sizeof(choose_item)-1), 1), projector_get_version_info());
    lv_obj_set_style_text_font(lv_obj_get_child(lv_obj_get_child(obj, sizeof(setting_items)/sizeof(choose_item)-1), 1), VERSION_INFO_FONT,0);

    lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(parent, 0), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    //language_choose_add_label(label, option_mode, 0);
    //language_choose_add_label1(label, );
    set_label_text2(label, STR_OPTION_MODE_TITLE, FONT_NORMAL);
    lv_obj_t *icon = lv_img_create(lv_obj_get_child(parent, 1));
    lv_img_set_src(icon, &MAINMENU_IMG_OPTIONS);
    lv_obj_center(icon);
    return obj;
}

lv_obj_t* create_page_(lv_obj_t* parent, choose_item * message, int len){
    lv_obj_set_style_pad_hor(parent, 0, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(parent, 0, 0);
    lv_obj_set_style_pad_ver(parent, 0, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_50, 0);
    lv_obj_set_style_bg_color(parent, lv_color_make(101,101,177), 0);

    lv_obj_t *obj1 = lv_obj_create(parent);
    lv_obj_set_style_radius(obj1, 0, 0);
    lv_obj_set_style_border_width(obj1, 0, 0);
    lv_obj_set_size(obj1,LV_PCT(22),LV_PCT(100));
    lv_obj_set_style_bg_opa(obj1, LV_OPA_0, 0);
    lv_obj_set_style_text_opa(obj1, LV_OPA_100, 0);
    lv_obj_set_style_pad_hor(obj1, 0, 0);
    lv_obj_set_scrollbar_mode(obj1, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *label = lv_label_create(obj1);
    lv_label_set_recolor(label, true);
    lv_obj_center(label);


    obj1 = lv_obj_create(parent);
    lv_obj_set_size(obj1,LV_PCT(24),LV_PCT(100));
    lv_obj_set_style_bg_color(obj1, lv_color_make(75,75,75), 0);
    lv_obj_set_style_radius(obj1, 0, 0);
    lv_obj_set_style_pad_hor(obj1, 0, 0);
    lv_obj_set_style_border_width(obj1, 0, 0);
    lv_obj_set_style_bg_opa(obj1, LV_OPA_0, 0);
    lv_obj_set_style_img_opa(obj1, LV_OPA_100, 0);

    lv_obj_t * menu = lv_obj_create(parent);
    set_pad_and_border_and_outline(menu);
    lv_obj_set_style_radius(menu, 0, 0);
    lv_obj_set_size(menu ,LV_PCT(54),LV_PCT(100));
    lv_obj_set_style_pad_ver(menu, (lv_coord_t )(lv_disp_get_ver_res(NULL)/10), 0);
    lv_obj_set_scrollbar_mode(menu, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(menu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(menu, 0, 0);
    lv_obj_set_style_bg_opa(menu, LV_OPA_0,0);
    lv_obj_set_style_text_opa(menu, LV_OPA_100, 0);
    for(int i=0; i< len; i++){
        create_item(menu, message+i);
    }
    return menu;
}

void label_set_text_color(lv_obj_t* label,const char* text, char* color){
    char temp[50];
    memset(temp, 0, 50);
    strcat(temp, color);
    if (text[0] == '#'){
        strncpy(temp+8, text+8, strlen(text)-9);
    }else{
        strncpy(temp+8, text, strlen(text));
    }
    strcat(temp, "#");

    lv_label_set_text(label, temp);

   
}





static lv_timer_t *left_symbol_timer = NULL, *right_symbol_timer = NULL;

static void timer_cb1(lv_timer_t *timer){
    lv_obj_t *obj = (lv_obj_t*)(timer->user_data);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    left_symbol_timer = NULL;
}

static void timer_cb2(lv_timer_t *timer){
    lv_obj_t *obj = (lv_obj_t*)(timer->user_data);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    right_symbol_timer = NULL;
}



lv_obj_t* create_item(lv_obj_t* page, choose_item * chooseItem){
    lv_obj_t* cont = lv_btn_create(page);
    
    lv_obj_set_size(cont,LV_PCT(BTN_WIDTH_PCT),LV_PCT(11));
    lv_obj_set_style_pad_hor(cont, 3, 0);
    lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_FULL, 0);
    lv_obj_set_style_radius(cont, 5, 0);
    lv_obj_set_style_border_width(cont, 2, 0);
    lv_obj_set_style_border_color(cont, lv_color_white(), 0);
    lv_obj_set_style_border_opa(cont, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(cont, LV_OPA_100, LV_STATE_USER_1);
    
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
    lv_obj_set_style_bg_color(cont, lv_palette_darken(LV_PALETTE_BLUE, 2), 0);
    lv_obj_add_event_cb(cont, chooseItem->event_func, LV_EVENT_ALL, cont);

    lv_obj_t* label = lv_label_create(cont);
    //language_choose_add_label1(label, );
    lv_label_set_recolor(label, true);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

 
    if (!chooseItem->is_disabled){
        lv_obj_set_style_text_color(cont, lv_color_white(), 0);
        set_label_text2(label, chooseItem->name,  FONT_NORMAL);
    }else{
        lv_obj_set_style_text_color(cont, lv_color_make(125,125,125), 0);
        set_label_text2(label, chooseItem->name, FONT_NORMAL);
        lv_obj_add_state(cont, LV_STATE_DISABLED);
    }



    label = lv_label_create(cont);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    
    if(SETUP_ITEM_VALUE_W_PCT < 0){

         lv_obj_set_width(label, LV_SIZE_CONTENT);
    }else{
        lv_obj_set_width(label, LV_PCT(SETUP_ITEM_VALUE_W_PCT));
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    if(!chooseItem->is_number && chooseItem->value.v1 != BLANK_SPACE_STR){
        if (!chooseItem->is_disabled){
            set_label_text2(label, chooseItem->value.v1, FONT_NORMAL);
        }else{
            set_label_text2(label, chooseItem->value.v1, FONT_NORMAL);
        }
    }else if(chooseItem->is_number){
        lv_label_set_text_fmt(label,"%d", chooseItem->value.v2);
        lv_obj_set_style_text_font(label, &ENG_NORMAL, 0);
    }else{
        lv_label_set_text(label, " ");
    }

    return cont;
}

lv_obj_t* create_widget_head(lv_obj_t* parent,int title, int h){
    lv_obj_t* head = lv_obj_create(parent);
    set_pad_and_border_and_outline(head);
    lv_obj_set_style_pad_ver(head, 0, 0);
    lv_obj_set_size(head,LV_PCT(100),LV_PCT(h));
    lv_obj_set_style_border_width(head, 0, 0);
    lv_obj_set_style_bg_color(head, lv_color_make(32,32,64), 0);
    lv_obj_set_scrollbar_mode(head, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t* label = lv_label_create(head);

    lv_label_set_recolor(label, true);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    //language_choose_add_label1(label, );
    set_label_text2(label, title, FONT_NORMAL);
    lv_obj_set_style_radius(head, 0, 0);
    lv_obj_center(label);
    return label;
}

void create_widget_foot(lv_obj_t* parent, int h, void* user_data){
    lv_obj_t* foot1 = lv_obj_create(parent);
    lv_obj_set_size(foot1,LV_PCT(100),LV_PCT(h));
    lv_obj_set_flex_flow(foot1, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(foot1, 0, 0);
    lv_obj_set_style_pad_gap(foot1, 0, 0);
    set_pad_and_border_and_outline(foot1);
    lv_obj_set_style_bg_color(foot1, lv_color_make(32,32,64), 0);
    lv_obj_set_style_radius(foot1, 0, 0);

    
    lv_obj_t *btn1,*btn2,*img, *label;
    for(int i=0; i<2; i++){
        btn1 = lv_obj_create(foot1);
        lv_obj_set_size(btn1,LV_PCT(50),LV_PCT(100));
        set_pad_and_border_and_outline(btn1);
        lv_obj_set_style_pad_ver(btn1, 0, 0);
        lv_obj_set_style_radius(btn1, 0, 0);
        lv_obj_set_style_bg_opa(btn1, LV_OPA_0, 0);

        btn2 = lv_obj_create(btn1);
        lv_obj_remove_style_all(btn2);
        lv_obj_set_style_bg_opa(btn2, LV_OPA_0, 0);
        lv_obj_center(btn2);
        lv_obj_set_style_radius(btn2, 0, 0);
        set_pad_and_border_and_outline(btn2);
        lv_obj_set_style_pad_ver(btn2, 0, 0);
        lv_obj_set_size(btn2, LV_PCT(75), LV_PCT(100));
        //lv_obj_set_flex_flow(btn2, LV_FLEX_FLOW_ROW);
        img = lv_img_create(btn2);
        lv_obj_align(img, LV_ALIGN_LEFT_MID, 0, 0);
        lv_img_set_src(img, i == 0 ? &MENU_IMG_LIST_OK : &MENU_IMG_LIST_MENU);
        label = lv_label_create(btn2);
        // lv_obj_center(label);
        lv_obj_set_size(label, LV_SIZE_CONTENT,LV_SIZE_CONTENT);
        lv_point_t size;

        
         int label_id = 0;
        if(i==0){
            label_id = STR_FOOT_SURE;
        }else{
            label_id = STR_FOOT_MENU;
        }  
        set_label_text2(label, label_id, FONT_NORMAL);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        
        lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_MID, WIDGET_STR_FOOT_OFFSET_X, WIDGET_STR_FOOT_OFFSET_Y);
       //lv_obj_add_event_cb(label, foot_label_event_handle, LV_EVENT_DRAW_PART_BEGIN, 0);

    }
}

lv_obj_t *create_new_widget(int w, int h){
    lv_obj_t* page = lv_obj_create(setup_slave_root);
    slave_scr_obj = page;
    //lv_obj_set_style_opa(page, LV_OPA_100, 0);
    lv_obj_set_size(page,LV_PCT(w),LV_PCT(h));
    lv_obj_center(page);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(page, 20, 0);
    lv_obj_set_style_pad_gap(page, 0, 0);
    lv_obj_set_style_text_opa(page, LV_OPA_100, 0);
    lv_obj_set_style_img_opa(page, LV_OPA_100, 0);
    set_pad_and_border_and_outline(page);
    lv_obj_set_style_pad_ver(page, 0, 0);
    lv_obj_set_style_shadow_width(page, 0, 0);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(page, lv_color_black(), 0);
    return page;
}

lv_obj_t *create_widget_btnmatrix(lv_obj_t *parent,int w, int h,const int* btn_map, int len){
    lv_obj_t* matrix_btn = lv_btnmatrix_create(parent);
    lv_group_focus_obj(matrix_btn);    
    lv_obj_set_size(matrix_btn,LV_PCT(w),LV_PCT(h));
    //language_choose_add_btns(matrix_btn, , len);
    set_btns_lang2(matrix_btn, len, FONT_NORMAL, btn_map);
    //set_btnmatrix_language(matrix_btn, projector_get_some_sys_param(P_OSD_LANGUAGE));
    lv_btnmatrix_set_btn_ctrl_all(matrix_btn,  LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(matrix_btn, true);
    INIT_STYLE_BG(&style_bg);
    lv_obj_add_style(matrix_btn, &style_bg, LV_PART_MAIN);
    //lv_obj_set_style_text_color(matrix_btn, lv_color_white(), LV_PART_ITEMS);
    INIT_STYLE_ITEM(&style_item);
    lv_obj_add_style(matrix_btn, &style_item, LV_PART_ITEMS);

    return matrix_btn;
}


lv_obj_t *create_widget_btnmatrix1(lv_obj_t *parent,int w, int h,const char** btn_map){
    lv_obj_t* matrix_btn = lv_btnmatrix_create(parent);
    lv_group_focus_obj(matrix_btn);
    lv_obj_set_size(matrix_btn,LV_PCT(w),LV_PCT(h));
    //language_choose_add_btns(matrix_btn, , len);
   
    lv_btnmatrix_set_map(matrix_btn, btn_map);
    
    
    //set_btnmatrix_language(matrix_btn, projector_get_some_sys_param(P_OSD_LANGUAGE));
    lv_btnmatrix_set_btn_ctrl_all(matrix_btn,  LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(matrix_btn, true);
    INIT_STYLE_BG(&style_bg);
    lv_obj_add_style(matrix_btn, &style_bg, LV_PART_MAIN);
    //lv_obj_set_style_text_color(matrix_btn, lv_color_white(), LV_PART_ITEMS);
    INIT_STYLE_ITEM(&style_item);
    lv_obj_add_style(matrix_btn, &style_item, LV_PART_ITEMS);
    return matrix_btn;
}



lv_obj_t *new_widget_(lv_obj_t* btn,  int title, const int* btn_map,uint32_t index, int len, int w, int h){
    lv_obj_t* image_mode = create_new_widget(w<=0 ?SETUP_WIDGET_DEFAULT_W :w, h<=0 ?SETUP_WIDGET_DEFAULT_H :h);

    create_widget_head(image_mode, title, 15);

    lv_obj_t *matrix_btn = create_widget_btnmatrix(image_mode, 100, 72, btn_map, len);
    char* text = lv_label_get_text(lv_obj_get_child(btn, 1));
    // char chs[10];
    // memset(chs, 0, 10);
    // strncpy(chs, text+8, strlen(text)-9);
    const char * map;
    for(int i=0; i<len; i++){
        if(btn_map[i] == BLANK_SPACE_STR || btn_map[i] == LINE_BREAK_STR || btn_map[i] == BTNS_VEC_END){
            continue;
        }
        map = api_rsc_string_get(btn_map[i]);
        if (strcmp(text, map) == 0){
            lv_btnmatrix_set_selected_btn(matrix_btn, i/2);
            lv_btnmatrix_set_btn_ctrl(matrix_btn, i/2, LV_BTNMATRIX_CTRL_CHECKED);
            btnmatrix_choose_id = i/2;
        }
    }
    // btnmatrix_choose_id = index;
    // lv_btnmatrix_set_selected_btn(matrix_btn, index);
    // lv_btnmatrix_set_btn_ctrl(matrix_btn, index, LV_BTNMATRIX_CTRL_CHECKED);
    btn->user_data = (void*)index;
    //

    create_widget_foot(image_mode, 14, btn);
    return image_mode;
}


void color_temp_widget(lv_obj_t* btn){
    lv_obj_t *obj = new_widget_(btn, STR_COLOR_TEMP, color_temp_vec,projector_get_some_sys_param(P_COLOR_TEMP), 12,0,0);
    
    lv_obj_add_event_cb(lv_obj_get_child(obj, 1), color_temp_btnmatrix_event, LV_EVENT_ALL, btn);
}

void color_temp_btnmatrix_event(lv_event_t* e){
    btnmatrix_event(e, set_color_temperature);
}


static uint8_t get_next_flip_mode_i(){
    for(int i=0; i<FLIP_MODE_LEN; i++){
        if(flip_mode_vec[i] == projector_get_some_sys_param(P_FLIP_MODE)){
            return (i+1)%FLIP_MODE_LEN;
        }
    }
    return 0;
}
uint8_t set_next_flip_mode(){
    uint8_t i = get_next_flip_mode_i();
    projector_set_some_sys_param(P_FLIP_MODE, flip_mode_vec[i]);
    set_flip_mode(flip_mode_vec[i]);
    return i;
}
void flip_widget(lv_obj_t* btn){
    uint8_t i = set_next_flip_mode();
    //language_choose_add_label1(lv_obj_get_child(btn, 1), );
    //int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text2(lv_obj_get_child(btn, 1), flip_vec[i], FONT_NORMAL);
}


void restory_factory_default_event_cb(lv_event_t *e){
    lv_obj_t * obj = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            if(lv_msgbox_get_active_btn(obj) == 0){	
#ifdef BLUETOOTH_SUPPORT	
                bluetooth_del_all_device();
                bluetooth_poweroff();
#endif
                projector_factory_reset();
        #ifdef __HCRTOS__
                reset();
        #else
                api_system_reboot();
	#endif		
            } else{
                lv_obj_del(target->parent);
                turn_to_setup_root();
            }
        }
    }
    
}

void restore_factory_default_widget(lv_obj_t* btn){
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    static const char * btns[3];
    btns[0] = api_rsc_string_get(STR_RESTORE_OK_1);
    btns[1] = api_rsc_string_get(STR_RESTORE_CLOSE);
    btns[2] = "";

    
    lv_obj_t * mbox1 = lv_msgbox_create(setup_slave_root, "", api_rsc_string_get(STR_RESTORE_OK_2), btns, false);
    slave_scr_obj = mbox1;
   
    lv_obj_t *label = lv_msgbox_get_content(mbox1);
    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);

    lv_obj_t *btns_obj = lv_msgbox_get_btns(mbox1);
     lv_obj_set_style_text_font(btns_obj, osd_font_get(FONT_MID), 0);
    lv_coord_t btn_h = lv_font_get_line_height(osd_font_get(FONT_MID)) + LV_DPI_DEF / 10;
    lv_obj_set_style_height(btns_obj, btn_h, 0);

    lv_obj_set_style_bg_color(mbox1, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    lv_obj_add_event_cb(mbox1,restory_factory_default_event_cb, LV_EVENT_ALL,NULL);
    lv_obj_center(mbox1);
    lv_group_focus_obj(lv_msgbox_get_btns(mbox1));
}


void setup_item_event_(lv_event_t* e, widget widget1, int item){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(target, 0);
    lv_obj_t * label1 = lv_obj_get_child(target, 1);
    
    if (code == LV_EVENT_CLICKED) {

     } else if(code == LV_EVENT_PRESSED){
        lv_obj_clear_state(target, LV_STATE_PRESSED);
    }else if(code == LV_EVENT_DEFOCUSED){
        lv_obj_set_style_bg_opa(target, LV_OPA_0, 0);
        lv_obj_set_style_border_opa(target, LV_OPA_0, 0);
        lv_obj_set_style_text_color(target, !lv_obj_has_state(target, LV_STATE_DISABLED) ?lv_color_white(): lv_color_make(125,125,125), 0);
        
    }
    else if (code == LV_EVENT_FOCUSED) {
        lv_obj_set_style_bg_opa(target, LV_OPA_100, 0);
        lv_obj_set_style_border_opa(target, LV_OPA_100, 0);
        lv_obj_add_state(target, LV_STATE_USER_1);
        lv_obj_set_style_text_color(target, lv_color_black(), 0);

    }else if (code == LV_EVENT_KEY) {
        if(timer_setting)
            lv_timer_pause(timer_setting);
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_DOWN || key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
            
            lv_group_t* lvGroup = lv_group_get_default();
            lv_obj_clear_state(target,LV_STATE_USER_1);
            focus_next_or_pre focusNextOrPre;
            focusNextOrPre = key == LV_KEY_DOWN ? lv_group_focus_next : lv_group_focus_prev;
            focusNextOrPre = key == LV_KEY_RIGHT ? lv_group_focus_next : focusNextOrPre;
            focusNextOrPre(lvGroup);
            lv_obj_t *focused = lv_group_get_focused(lvGroup);
            uint16_t  pg_id = lv_tabview_get_tab_act(tabv);
            while ((focused->parent->parent != target->parent->parent) || lv_obj_has_state(focused, LV_STATE_DISABLED)){
                if (focused == tab_btns || focused == foot){
                    break;
                }
            #ifdef BLUETOOTH_SUPPORT
                if(focused == bt_list_obj){
                    focusNextOrPre(lvGroup);
                    focused = lv_group_get_focused(lvGroup); 
                    continue;                   
                }
            #endif
                lv_obj_clear_state(focused, LV_STATE_USER_1);
                lv_obj_set_style_bg_opa(focused, LV_OPA_0, 0);
                lv_obj_set_style_border_opa(focused, LV_OPA_0, 0);
                focusNextOrPre(lvGroup);
                focused = lv_group_get_focused(lvGroup);
            }
            cur_btn = focused;
            lv_tabview_set_act(tabv, pg_id, LV_ANIM_OFF);
           
        }else if (key == LV_KEY_ENTER){
            if(item == P_BASS || item == P_TREBLE || item == P_CONTRAST || item == P_COLOR ||
              item == P_BRIGHTNESS || item == P_HUE || item == P_SHARPNESS || item == P_BALANCE){
                // lv_group_focus_obj(lv_obj_get_child(target, 1));
                set_adjustable_value(item);
                // if(key == LV_KEY_RIGHT){
                //     lv_dropdown_open(lv_obj_get_child(lv_obj_get_child(target, 1), 0));
                // }
            }else if(widget1){
                if(item != P_FLIP_MODE){
                    lv_obj_add_flag(setup_root, LV_OBJ_FLAG_HIDDEN);
                    //lv_obj_set_style_bg_opa()
                }
                widget1(target);
           }


        }
        else if (key == LV_KEY_ESC || key == LV_KEY_HOME){
            lv_obj_clear_state(target, LV_STATE_USER_1);
            turn_to_main_scr();
            return ;
        }
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
    }
    else if (code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        dsc->rect_dsc->outline_width=0;
        dsc->rect_dsc->shadow_width=0;
    }
    
}

void picture_mode_event(lv_event_t* e) {
    setup_item_event_(e, picture_mode_widget, P_PICTURE_MODE);
}
void contrast_event(lv_event_t* e){
    setup_item_event_(e,  NULL, P_CONTRAST);
}
void brightness_event(lv_event_t* e){
    setup_item_event_(e,  NULL, P_BRIGHTNESS);
}
void color_event(lv_event_t* e){
    setup_item_event_(e,NULL, P_COLOR);

}

void hue_event(lv_event_t* e){
    setup_item_event_(e, NULL, P_HUE);
}
void sharpness_event(lv_event_t* e){
    setup_item_event_(e, NULL, P_SHARPNESS);

}
void color_temperature_event(lv_event_t* e){
    setup_item_event_(e, color_temp_widget, P_COLOR_TEMP);

}

void sound_mode_event(lv_event_t* e){
    setup_item_event_(e, sound_mode_widget, P_SOUND_MODE);

}
void treble_event(lv_event_t* e){
    setup_item_event_(e, NULL, P_TREBLE);

}
void version_info_event(lv_event_t *e){
    setup_item_event_(e, NULL, P_VERSION_INFO);
}
void bass_event(lv_event_t* e){
    setup_item_event_(e, NULL, P_BASS);

}
void balance_event(lv_event_t* e){
    setup_item_event_(e, NULL, P_BALANCE);

}


void osd_language_event(lv_event_t* e){
    setup_item_event_(e, osd_language_widget, P_OSD_LANGUAGE);

}
void flip_event(lv_event_t* e){
    setup_item_event_(e, flip_widget, P_FLIP_MODE);

}
// void aspect_radio_event(lv_event_t* e){
//     setup_item_event_(e, aspect_ratio_widget, P_ASPECT_RATIO);


// }
void restore_factory_default_event(lv_event_t* e){
    setup_item_event_(e, restore_factory_default_widget, P_RESTORE);

}
void software_update_event(lv_event_t* e){
    setup_item_event_(e, software_update_widget, P_UPDATE);

}

void keystone_event(lv_event_t *e){
    setup_item_event_(e, keystone_screen_init, P_KEYSTONE);
}

void auto_sleep_event(lv_event_t *e){
    setup_item_event_(e, auto_sleep_widget, P_AUTOSLEEP);
}

extern void osd_time_widget(lv_obj_t* btn);

void osd_time_event(lv_event_t *e){
    setup_item_event_(e, osd_time_widget, P_OSD_TIME);
}

void window_scale_event(lv_event_t *e){
    #ifdef SYS_ZOOM_SUPPORT
    extern void create_scale_widget(lv_obj_t* btn);
    setup_item_event_(e, create_scale_widget, P_WINDOW_SCALE);
    #endif
}

void btnmatrix_event(lv_event_t* e, btn_matrix_func func){
    lv_obj_t* target = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn= lv_event_get_user_data(e);
    uint32_t index = (uint32_t)btn->user_data;
    if (code == LV_EVENT_KEY){
        if(timer_setting){
            lv_timer_pause(timer_setting);
        }
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_ENTER){
            uint32_t btn_id = lv_btnmatrix_get_selected_btn(target);
            const char* text = lv_btnmatrix_get_btn_text(target, btn_id);
            lv_obj_t* lab = lv_obj_get_child(btn, 1);
            uint32_t font_id = GET_FONT_ID((int)(lab->user_data));
            lab->user_data = (void*)STORE_LABEL_AND_FONT_ID(((btnmatrix_p1*)target->user_data)->str_id_vec[btn_id*2], font_id);

            lv_label_set_text(lab ,text);
            if (func){
                func(btn_id);
            }
            lv_obj_del(target->parent);
           turn_to_setup_root();
           return; 
        }
        if (key == LV_KEY_DOWN || key == LV_KEY_UP){
            key == LV_KEY_DOWN && btnmatrix_choose_id++;
            key == LV_KEY_UP && btnmatrix_choose_id--;

            if (key == LV_KEY_DOWN && (btnmatrix_choose_id >= ((btnmatrix_p *)target->user_data)->len/2 || strcmp(lv_btnmatrix_get_btn_text(target, btnmatrix_choose_id), " ")  == 0)){
                lv_btnmatrix_set_selected_btn(target, 0);
                btnmatrix_choose_id = 0;
            }
            if(key == LV_KEY_UP &&  btnmatrix_choose_id < 0){
                btnmatrix_choose_id = ((btnmatrix_p *)target->user_data)->len/2-1;
                while (strcmp(lv_btnmatrix_get_btn_text(target, btnmatrix_choose_id), " ")  == 0){
                    --btnmatrix_choose_id;
                }
            }
            lv_btnmatrix_set_selected_btn(target, btnmatrix_choose_id);
            lv_btnmatrix_set_btn_ctrl(target, btnmatrix_choose_id, LV_BTNMATRIX_CTRL_CHECKED);
            if (func){
                func(btnmatrix_choose_id);
            }
        }
        if (key == LV_KEY_RIGHT || key == LV_KEY_LEFT){
            lv_btnmatrix_set_selected_btn(target, btnmatrix_choose_id);
        }
        if(key == LV_KEY_HOME){
            if(index != btnmatrix_choose_id && func){
                func(index);
            }
            lv_obj_del(target->parent);
            turn_to_setup_root();
        }
        if(key ==LV_KEY_ESC ){
            if(index != btnmatrix_choose_id && func){
                func(index);
            }
            lv_obj_del(target->parent);
            turn_to_setup_root();
        }
        last_setting_key = key;
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
    }
    if(code == LV_EVENT_DRAW_PART_BEGIN) {
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);

        dsc->rect_dsc->outline_width=0;
    }
}



void return_event(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED){
        turn_to_setup_root();
    }
}



static void set_picture(const void *scr, lv_obj_draw_part_dsc_t *dsc){
    lv_img_header_t header;
    lv_res_t res = lv_img_decoder_get_info(scr, &header);
    if(res != LV_RES_OK) return;

    lv_area_t a;
    a.x1 = dsc->draw_area->x1 + (lv_area_get_width(dsc->draw_area) - header.w) / 2;
    a.x2 = a.x1 + header.w - 1;
    a.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - header.h) / 2;
    a.y2 = a.y1 + header.h - 1;

    lv_draw_img_dsc_t img_draw_dsc;
    lv_draw_img_dsc_init(&img_draw_dsc);
    img_draw_dsc.recolor = lv_color_black();

    lv_draw_img(dsc->draw_ctx, &img_draw_dsc, &a, scr);
}

static bool obj_has_ancestor(lv_obj_t *self, lv_obj_t *ancestor){
    while (self->parent)
    {
        if(self->parent == ancestor){
            return true;
        }
        self = self->parent;
    }
    return false;
    
}


void tabv_btns_event(lv_event_t* e){

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    lv_group_t* lvGroup = lv_group_get_default();
    if (code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        dsc->rect_dsc->outline_width=0;
    }
    if (code == LV_EVENT_DRAW_PART_END){
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
        if(dsc->id == TAB_PICTURE) {
            if (dsc->id == lv_tabview_get_tab_act(target->parent)){
                set_picture(&MAINMENU_IMG_PICTURE_FOCUS, dsc);
            }else{
                set_picture(&MAINMENU_IMG_PICTURE_S_UNFOCUS, dsc);
            }
        }else if(dsc->id == TAB_SOUND){
            if (dsc->id == lv_tabview_get_tab_act(target->parent)){
                set_picture(&MAINMENU_IMG_AUDIO_FOCUS, dsc);
            }else{
                set_picture(&MAINMENU_IMG_AUDIO_S_UNFOCUS, dsc);
            }
        }else if(dsc->id == TAB_SETTING){
            if (dsc->id == lv_tabview_get_tab_act(target->parent)){
                set_picture(&MAINMENU_IMG_OPTION_FOCUS, dsc);
            }else{
                set_picture(&MAINMENU_IMG_OPTIONS_S_UNFOCUS, dsc);
            }
        }		
#ifdef BLUETOOTH_SUPPORT
		else if(dsc->id == TAB_BT){
             if (dsc->id == lv_tabview_get_tab_act(target->parent)){
                set_picture(&bt_focus, dsc);
            }else{
                set_picture(&bt_unfocus, dsc);
            }
        }
#endif		
#ifdef KEYSTONE_SUPPORT
	else if(dsc->id == TAB_KEYSTONE){
            if (dsc->id == lv_tabview_get_tab_act(target->parent)){
                set_picture(&keystone_focus, dsc);
            }else{
                set_picture(&keystone_unfocus, dsc);
            }
        }
#endif
    }
    if (code == LV_EVENT_KEY){
        if(timer_setting)
            lv_timer_pause(timer_setting);
        uint16_t  key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_ESC || key == LV_KEY_HOME){
            turn_to_main_scr();
            return;
        }else if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT){
            uint16_t id = lv_btnmatrix_get_selected_btn(target);
            if(key == LV_KEY_LEFT && id == 0){
                id = TAB_MAX-1;
                lv_btnmatrix_set_selected_btn(target, id);
            }
            if(key == LV_KEY_RIGHT && id == TAB_MAX){
                id = 1;
                lv_btnmatrix_set_selected_btn(target, id);
            }
            selected_page = id;
            lv_tabview_set_act(tabv, id, LV_ANIM_OFF);
        }else if (key == LV_KEY_DOWN || key == LV_KEY_UP || key == LV_KEY_ENTER){
            uint16_t id = lv_btnmatrix_get_selected_btn(target);
            lv_obj_t *pg = pgs[id-1];
            
            focus_next_or_pre focusNextOrPre;
            focusNextOrPre = (key == LV_KEY_DOWN || key == LV_KEY_ENTER)? lv_group_focus_next : lv_group_focus_prev;
            focusNextOrPre(lvGroup);
            target = lv_group_get_focused(lvGroup);
            lv_obj_t *label;
            char* text;
            
                while (pg==pgs[TAB_MAX-2] ? !obj_has_ancestor(target, pg) : target->parent->parent != pg){
                #ifdef BLUETOOTH_SUPPORT
                    if(target == bt_list_obj){
                        focusNextOrPre(lvGroup);
                        target = lv_group_get_focused(lvGroup);
                        continue;
                    }
                #endif
                    if(target == keystone_list){
                        focusNextOrPre(lvGroup);
                        target = lv_group_get_focused(lvGroup);
                        continue;
                    }
                    lv_obj_clear_state(target, LV_STATE_USER_1);
                    // lv_obj_set_style_text_color(target, lv_color_white(), 0);
                    // lv_obj_set_style_bg_opa(target, LV_OPA_0, 0);
                    // lv_obj_set_style_border_opa(target, LV_OPA_0, 0);
                    focusNextOrPre(lvGroup);
                    target = lv_group_get_focused(lvGroup);
                }
                cur_btn = target;           
                printf("id: %d\n", id);     
            lv_tabview_set_act(tabv, id, LV_ANIM_OFF);           

        }
        last_setting_key = key;
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting); 
        }

    }

}

void btn_event(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        dsc->rect_dsc->outline_width=0;
        dsc->rect_dsc->shadow_width=0;
    }
}




void turn_to_setup_root(){
    if (setup_scr){
        //lv_scr_load(setup_scr);
        lv_group_focus_obj(cur_btn);
    }

    slave_scr_obj = NULL;
    lv_obj_clear_flag(setup_root, LV_OBJ_FLAG_HIDDEN);
    projector_sys_param_save();

}

void turn_to_main_scr(){
    // if(timer_setting){
    //     lv_timer_ready(timer_setting);
    //     lv_timer_resume(timer_setting);
    // }else{
        change_screen(projector_get_some_sys_param(P_CUR_CHANNEL));
    //}

    projector_sys_param_save();
}

static void msg_timer_handle(lv_timer_t *timer_){
    lv_obj_t *obj = (lv_obj_t*)timer_->user_data;
    printf("del msgbox\n");
    lv_obj_del(obj);
}

lv_obj_t* create_message_box(char* str){
    printf("msgbox\n");

    lv_obj_t *con = lv_obj_create(lv_layer_top());
    lv_obj_set_style_text_color(con, lv_color_white(), 0);
    lv_obj_set_size(con, LV_SIZE_CONTENT, LV_PCT(10));
    lv_obj_center(con);
    lv_obj_set_style_bg_color(con, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    lv_obj_set_scrollbar_mode(con, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(con, LV_OPA_70, 0);
    lv_obj_set_style_border_width(con, 1, 0);
    lv_obj_set_style_border_color(con, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_outline_width(con, 0, 0);
    lv_obj_set_style_radius(con, 10, 0);

    lv_obj_t *label = lv_label_create(con);
    lv_obj_center(label);
    lv_label_set_text(label, str);
    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);

    lv_timer_t *timer = lv_timer_create(msg_timer_handle, 2000, con);
    lv_timer_set_repeat_count(timer, 1);
    lv_timer_reset(timer);
    return con;
}

lv_obj_t* create_message_box1(int msg_id, int btn_id1, int btn_id2, lv_event_cb_t cb, int w, int h){
    static const char * btns[] ={" ", " ", ""};
    btns[0] = api_rsc_string_get(btn_id1);
    btns[1] = api_rsc_string_get(btn_id2);
    lv_obj_t * mbox1 = lv_msgbox_create(lv_layer_top(), NULL,
    api_rsc_string_get(msg_id), btns, false);
    lv_obj_set_flex_flow(mbox1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mbox1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
   
    
    lv_obj_set_style_bg_color(mbox1, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    //lv_obj_set_style_bg_opa(mbox1, LV_OPA_70, 0);
    lv_obj_set_size(mbox1, lv_pct((w<=0 ? 25 : w)), lv_pct((h<=0 ?20 : h)));
     lv_obj_add_event_cb(mbox1, cb, LV_EVENT_ALL, NULL);
    lv_obj_t *obj1 = lv_msgbox_get_btns(mbox1);
    lv_obj_set_width(obj1, lv_pct(100));
    lv_obj_align(obj1, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_coord_t btn_h = lv_font_get_line_height(osd_font_get(FONT_MID)) + LV_DPI_DEF / 10;
    lv_obj_set_style_height(obj1, btn_h, 0);
   
    lv_obj_center(mbox1);
    lv_obj_set_style_text_font(mbox1, osd_font_get(FONT_MID), 0);
    lv_group_focus_obj(lv_msgbox_get_btns(mbox1));
    
    return mbox1;
}

static void set_angle(void * obj, int32_t v)
{
    lv_arc_set_value(obj, v);
}

static void loader_with_arc_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t* dsc = lv_event_get_draw_part_dsc(e);
        if(dsc->type ==  LV_ARC_DRAW_PART_BACKGROUND  || dsc->type ==  LV_ARC_DRAW_PART_FOREGROUND){
            dsc->radius = 400;
        }
        
    }
}

lv_obj_t* loader_with_arc(char* str, lv_anim_exec_xcb_t exec_cb){
    lv_obj_t * arc = lv_arc_create(lv_layer_top());
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);  /*To not allow adjusting by click*/
    lv_obj_center(arc);
    lv_obj_set_size(arc, lv_pct(18), lv_pct(32));
    //lv_obj_add_event_cb(arc, loader_with_arc_event_handle, LV_EVENT_ALL, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, exec_cb ? exec_cb : set_angle);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);    /*Just for the demo*/
    lv_anim_set_repeat_delay(&a, 500);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_start(&a);

    lv_obj_t *label = lv_label_create(arc);
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);
    lv_label_set_text(label, str);
    return arc;
}

lv_obj_t* create_list_obj1(lv_obj_t *parent, int w, int h){
    lv_obj_t *obj = lv_list_create(parent);
    lv_obj_set_style_radius(obj, 0, 0);
    
    //set_pad_and_border_and_outline(obj);
    //lv_obj_set_style_pad_ver(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    //lv_obj_set_style_pad_hor(obj, 5, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    
    // lv_obj_scroll_to_view(obj, LV_ANIM_OFF);
   lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_size(obj, LV_PCT(w), LV_PCT(h));
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_group_add_obj(lv_group_get_default(), obj);
    lv_group_focus_obj( obj);
    return obj;
}

lv_obj_t* create_list_sub_text_obj1(lv_obj_t *parent, int w, int h, int str1, int font_id){
    list_sub_param param;
    param.str_id = str1;
    lv_obj_t *list_label =  create_list_sub_text_obj(parent,w,h,param, LIST_PARAM_TYPE_INT,  font_id);
    lv_obj_set_style_text_align(list_label, LV_TEXT_ALIGN_CENTER,0);
   
    return list_label;
}

lv_obj_t* create_list_sub_text_obj4(lv_obj_t *parent, int w, int h, int str1){
    list_sub_param param;
    param.str = "";
    lv_obj_t *list_label =  create_list_sub_text_obj(parent,w,h,param, -1,  -1);
    lv_obj_set_style_text_align(list_label, LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_pad_top(list_label, 0, 0);

    lv_obj_t *label = lv_label_create(list_label);
    lv_obj_center(label);
    //language_choose_add_label1(label);
    set_label_text2(label ,str1, FONT_NORMAL);
    return list_label;
}




lv_obj_t* create_list_sub_text_obj3(lv_obj_t *parent,int w, int h, char* str1){
    list_sub_param param;
    param.str = str1;
    return create_list_sub_text_obj(parent,w,h,param, LIST_PARAM_TYPE_STR, -1);
}



static lv_obj_t* create_list_sub_text_obj(lv_obj_t *parent,int w, int h, list_sub_param param, int type, int font_id){
    lv_obj_t *list_label;
    list_label = lv_list_add_text(parent, " ");
    lv_obj_set_style_text_align(list_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_size(list_label,LV_PCT(w),LV_SIZE_CONTENT);
    lv_obj_set_style_pad_ver(list_label,0,0);
    // lv_label_set_long_mode(list_label,LV_LABEL_LONG_DOT);
    
    lv_obj_set_style_border_side(list_label, LV_BORDER_SIDE_FULL, LV_STATE_CHECKED);
    lv_obj_set_style_border_width(list_label, 2, 0);
    lv_obj_set_style_border_color(list_label, lv_color_white(), 0);
    lv_obj_set_style_border_opa(list_label, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(list_label, LV_OPA_100, LV_STATE_CHECKED);

    lv_obj_set_style_bg_opa(list_label, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(list_label, LV_OPA_100, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(list_label, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);

    lv_obj_set_style_text_color(list_label, lv_color_make(175, 175, 175), 0);
    lv_obj_set_style_text_color(list_label, lv_color_black(), LV_STATE_CHECKED);

    if(type == LIST_PARAM_TYPE_INT){
        set_label_text2(list_label, param.str_id, font_id < 0 ? FONT_NORMAL : font_id);        
    }else if(type == LIST_PARAM_TYPE_STR){
        lv_label_set_text(list_label, param.str);
        lv_obj_set_style_text_font(list_label, &lv_font_montserrat_26,0);
    }

   return list_label;
}

void adjust_bar_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *show_value = lv_obj_get_child(obj, 0);
    uint32_t item = (uint32_t)lv_event_get_user_data(e);
    static int cur_v=0;
    if(code == LV_EVENT_KEY){
        if(timer_setting){
            lv_timer_pause(timer_setting);
        }
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_UP || key == LV_KEY_RIGHT){
               cur_v+=1;

            if(item == P_BASS){
                if(cur_v>10){
                    cur_v=10;
                    return;
                }
                set_twotone(SND_TWOTONE_MODE_USER, projector_get_some_sys_param(P_TREBLE), cur_v);
            }else if (item == P_TREBLE){
                if(cur_v>10){
                    cur_v=10;
                    return;
                }
                set_twotone(SND_TWOTONE_MODE_USER, cur_v, projector_get_some_sys_param(P_BASS));
            }else if(item == P_CONTRAST || item == P_COLOR ||  item == P_BRIGHTNESS || item == P_HUE){
                if(cur_v>100){
                    cur_v=100;
                    return;
                }
                set_enhance1(cur_v, item);
            }else if(item == P_BALANCE){
                if(cur_v>24){
                    cur_v=24;
                    return;
                }
                set_balance(cur_v);
            }else if(item == P_SHARPNESS){
                if(cur_v>10){
                    cur_v=10;
                    return;
                }
                set_enhance1(cur_v, item);
            }
            lv_label_set_text_fmt(lv_obj_get_child(obj, 0), "%d", cur_v);
        }else if(key == LV_KEY_DOWN ||key == LV_KEY_LEFT){
           
            cur_v-=1;
            if(item == P_BASS){
                if(cur_v<-10){
                    cur_v=-10;
                    return;
                }
                set_twotone(SND_TWOTONE_MODE_USER, projector_get_some_sys_param(P_TREBLE), cur_v);
            }else if (item == P_TREBLE){
                if(cur_v<-10){
                    cur_v=-10;
                    return;
                }                
                set_twotone(SND_TWOTONE_MODE_USER, cur_v, projector_get_some_sys_param(P_BASS));
            }else if(item == P_CONTRAST || item == P_COLOR ||  item == P_BRIGHTNESS || item == P_HUE){
                if(cur_v<0){
                    cur_v=0;
                    return;
                }
                set_enhance1(cur_v, item);
            }else if(item == P_BALANCE){
                if(cur_v<-24){
                    cur_v=-24;
                    return;
                }
                set_balance(cur_v);
            }else if(item == P_SHARPNESS){
                if(cur_v<0){
                    cur_v=0;
                    return;
                }
                set_enhance1(cur_v, item);
            }
            lv_label_set_text_fmt(lv_obj_get_child(obj, 0), "%d", cur_v);
        }else if(key == LV_KEY_ESC || key==LV_KEY_ENTER ||  key == LV_KEY_HOME){
            lv_obj_del(obj->parent);
            lv_label_set_text_fmt(lv_obj_get_child(cur_btn, 1), "%d", cur_v);
            turn_to_setup_root();

        }
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }        
    }else if(code == LV_EVENT_FOCUSED){
        cur_v = projector_get_some_sys_param(item);
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        if(dsc && dsc->rect_dsc){
            dsc->rect_dsc->outline_width=0;           
        }

    }
}

lv_obj_t* create_adjust_bar(int name_id, int value, int min, int max){
    lv_obj_t *m_bar = lv_obj_create(setup_slave_root);
    slave_scr_obj = m_bar;
    lv_obj_align(m_bar, LV_ALIGN_TOP_MID, lv_pct(44), 20);
    lv_obj_set_size(m_bar, lv_pct(ADJUST_BAR_W_PCT), lv_pct(70));
    lv_obj_set_style_bg_color(m_bar, lv_color_hex(0x303030), 0);
    lv_obj_set_flex_flow(m_bar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(m_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_width(m_bar, 0, 0);
    lv_obj_clear_flag(m_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_hor(m_bar, 0, 0);
   
    

    // lv_obj_t *label_name = lv_label_create(m_bar);
    // lv_obj_set_size(label_name, lv_pct(100), LV_SIZE_CONTENT);
    // lv_label_set_text(label_name, api_rsc_string_get(name_id));
    // lv_obj_set_style_text_font(label_name, osd_font_get(FONT_SMALL), 0);
    // lv_obj_set_style_text_color(label_name, lv_color_hex(0x8a86b8), 0);
    // lv_obj_set_style_text_align(label_name, LV_TEXT_ALIGN_CENTER, 0);
    // lv_label_set_long_mode(label_name, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_obj_t *slider_obj = lv_slider_create(m_bar);
    lv_obj_set_size(slider_obj, lv_pct(60), lv_pct(100));
    //lv_obj_set_style_anim_time(slider_obj, 100, 0);
    lv_slider_set_range(slider_obj, min, max);
    lv_slider_set_value(slider_obj, value, LV_ANIM_OFF);
    lv_obj_set_style_bg_opa(slider_obj, LV_OPA_0, LV_PART_KNOB);
    // lv_obj_set_style_border_width(slider_obj, 0, 0);
    // lv_obj_set_style_outline_width(slider_obj, 0, 0);
    // lv_obj_set_style_shadow_width(slider_obj, 0, 0);
    

    lv_obj_t*  label_name = lv_label_create(slider_obj);
    lv_label_set_text_fmt(label_name,"%d", value);
    lv_obj_set_style_text_color(label_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_name, osd_font_get(FONT_NORMAL), 0);
    lv_obj_center(label_name);
    
    return slider_obj;
}

void set_adjustable_value(uint32_t item){
    lv_obj_add_flag(setup_root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *slider=NULL;
    // if(timer_setting){
    //     lv_timer_pause(timer_setting);            
    // }
    int v = projector_get_some_sys_param(item);
    switch (item)
    {
    case P_BASS:
        slider=create_adjust_bar(STR_BASS, v, -10, 10);
        break;
    case P_TREBLE:
        slider=create_adjust_bar(STR_TREBLE, v, -10, 10);
        break;
    case P_CONTRAST:
        slider=create_adjust_bar(STR_CONSTRAST, v, 0, 100);
        break;
    case P_COLOR:
        slider=create_adjust_bar(STR_COLOR, v, 0, 100);
        break;
    case P_BRIGHTNESS:
        slider=create_adjust_bar(STR_BRIGHTNESS, v, 0, 100);
        break;
    case P_HUE:
        slider=create_adjust_bar(STR_HUE, v, 0, 100);
        break;
    case P_BALANCE:
        slider=create_adjust_bar(STR_BALANCE, v, -24, 24);
        break;
    case P_SHARPNESS:
        slider=create_adjust_bar(STR_SHARPNESS, v, 0, 10);
        break;
    default:
        break;
    }

    if(slider){
        lv_obj_add_event_cb(slider, adjust_bar_event_handle, LV_EVENT_ALL, (void*)item);
        lv_group_focus_obj(slider);
    }
}
