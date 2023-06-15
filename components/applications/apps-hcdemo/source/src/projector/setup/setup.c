#include "lvgl/lvgl.h"
#include "../screen.h"
#include "setup.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <hcuapi/fb.h>
#include <hcuapi/dis.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include "setup_helper.h"
#include <bluetooth.h>
#include <hcfota.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "../factory_setting.h"
#include <kernel/lib/fdt_api.h>
#include "../channel/local_mp/local_mp_ui.h"
#define LANGUAGE_LEN  2
#define FLIP_MODE_LEN 4

lv_obj_t *setup_scr = NULL;
lv_group_t *setup_g = NULL;
lv_timer_t *timer_setting = NULL, *timer_setting_clone = NULL;
uint32_t MY_EVENT_UNLOADED;
uint32_t MY_EVENT_LOADED;

static sys_param_t *sys_param_p;
lv_obj_t* slave_scr = NULL;
static uint16_t last_setting_key=0;
static int btnmatrix_choose_id = 0;
static lv_obj_t *cur_btn = NULL;


//蓝牙
// struct bluetooth_slave_dev devs_info[5]={0};
// struct bluetooth_slave_dev *devs_info_t[5]={NULL, NULL, NULL, NULL,NULL};
// int connected_bt_index = 0;

extern lv_obj_t *bluetooth_obj;
// lv_obj_t *wait_anim = NULL;
// static int found_bt_num=0;
// static lv_timer_t * wait_anim_timer = NULL;
// static lv_timer_t *bt_check_timer = NULL;
// static bt_scan_status scan_status;
// static bt_connect_status_e connet_status;
//static bool is_bt_check = false;



// static char* labels = {
//     {"Picture Mode", "图像模式"},
//     {"Contrast", "对比度"},
//     {"Brightness", "亮度"},
//     {"Color", "颜色"},
//     {"Sharpness", "清晰度"},
//     {"Color Temperature", "色温"},
//     {"Sound Mode", "声音模式"},
//     {"Treble", "高音"},
//     {"Bass", "低音"},
//     {"Balance", "平衡"},
//     {"Bt Setting", "蓝牙设置"},
//     {"Osd language", "语言设置"},
//     {"Flip", "投屏模式"},
//     {"Aspect Ratio", "画面比例"},
//     {"Restore Factory Default", "恢复出厂设置"},
//     {"Software Update", "软件升级"},
//     {"Keystone", "梯形校正"}
// }


static char* picture_mode_k = {"Picture Mode\0" "图像模式\0" "Modes D'image"};
static const char * picture_mode_v[NEW_WIDGET_LINE_NUM*2] = {"Standard\0标准\0Standard" ,"\n", "Dynamic\0动态\0Dynamique","\n",
                                                             "Mild\0温和\0Léger","\n", "User\0用户\0Utilisateur", "\n", " ", "\n", " ", ""};

static char* contrast_k = {"Contrast\0" "对比度\0" "Contraste"};

static char* brightness_k = {"Brightness\0" "亮度\0" "Luminosité"};

static char* color_k = {"Color\0" "颜色\0" "Couleur"};

static char* sharpness_k = {"Sharpness\0" "清晰度\0" "Netteté"};

static char* color_temperature_k = {"Color Temperature\0" "色温\0" "Température de couleur"};
static const char *color_temperature_v[] =   {"Cold\0冷色温\0froid", "\n", "Standard\0标准\0Standard", "\n", "Warm\0暖色温\0chaud", "\n",
                                              " ", "\n", " ", "\n", " ", ""};

                             

static char* sound_mode_k = {"Sound Mode\0" "声音模式\0" "Mode de son"};
static const char* sound_mode_v[NEW_WIDGET_LINE_NUM*2]  = {"Standard\0标准\0" "Standard", "\n","Movie\0电影\0" "Film", "\n", "Music\0音乐\0" "Musique", "\n",
                                                         "Sports\0运动\0" "Sports", "\n","User\0用户\0" "Musique","\n", " ", ""};

static char* treble_k = {"Treble\0" "高音\0" "Haut"};

static char* bass_k = {"Bass\0" "低音\0" "Basse"};

static char* balance_k = {"Balance\0" "平衡\0" "équilibre"};

static char* bt_setting_k = {"BT Setting\0" "蓝牙设置\0" "Paramètres BT"};


static char* osd_language_k = {"Osd Language\0" "语言设置\0" "Osd Langue"};
static const char* osd_language_v[LANGUAGE_LEN+1] = {"English\0英语\0anglais", "Chinese\0简体中文\0chinois", ""};//, "French\0法语\0français", "Russian\0俄语\0russe",
                                                  //"German\0德语\0allemand", "Japanese\0日语\0japonais", "Korean\0韩语\0coréen", "Spanish\0西班牙语\0espagnol", "Portuguese\0葡萄牙语\0portugais",""};

static char* flip_k = {"Flip\0" "投屏模式\0" "Mode de coulée"};


static int flip_mode_vec[FLIP_MODE_LEN] = {FLIP_MODE_NORMAL, FLIP_MODE_ROTATE_180, FLIP_MODE_H_MIRROR, FLIP_MODE_V_MIRROR};
static char* flip_v[FLIP_MODE_LEN] = {"Front Table\0" "桌上正投\0" "Front Table", "Front Ceiling\0 吊装正投\0 Front Ceiling", "Back Table\0桌上背投\0Back Table", "Back Ceiling\0" "吊装背投\0" "Back Ceiling"};
static char* aspect_ratio_k = {"Aspect Ratio\0" "画面比例\0" "Rapport d'aspect"};
static const char* aspect_ratio_v[] = { "Auto\0自动\0Auto", "\n", "4:3\0 4:3\0 4:3","\n", "16:9\0 16:9\0 16:9", "\n", " ", "\n", " ", "\n", " ",""};//,  "Zoom In \0放大\0zoom avant", "\n", "Zoom Out \0缩小\0zoom arrière", "\n", " ", 

static char* restore_factory_default_k = {"Restore Factory Default\0" "恢复出厂设置\0" "Restaurer les paramètres d'usine"};
static char* software_update_k = {"Software Update\0" "软件升级（USB）\0" "Mise à jour du logiciel"};
static char* software_no_device = {"Can't detect USB\0" "不能发现USB\0" "Can't detect USB "};
static char* software_no_software = {"Can't detect Software\0" "不能发现软件\0" "Can't detect Software"};
static char* keystone_k = {"Keystone\0" "梯形校正\0" "Clef de voûte"};
static const char* auto_sleep_v[] = {"Off \0关闭\0de", "\n", "60m \0 60分钟\0 60m", "\n", "120m \0 120分钟\0 120m", "\n", "180m \0 180分钟\0 180m", "\n", " ", "\n", " ", ""};
static char* auto_sleep_k = {"Auto Sleep\0" "自动休眠\0" "Veille automatique"};
static char* None = " ";
static char *picture_mode = {"PICTURE\0" "图像\0" "IMAGE"};
static char *sound_mode = {"SOUND\0" "声音\0" "SON"};
static char *option_mode = {"OPTION\0" "选项\0" "OPTION\0"};
static const char* foot_btn_map[] = {"Move\0移动\0déplacer", "Menu\0菜单\0Menu","Ok\0确定\0Ok", "Off \0离开\0 Off"};
static const char* foot_sure = "Enter\0确定\0Enter";
static const char* foot_menu = "Menu\0菜单\0Menu";
static OSD_LANGUAGE selected_language = English;
static OSD_LANGUAGE pre_selected_language = English;

static mode_items picture_mode_items[4];
static mode_items sound_mode_items[2];



static int visible[3];

static int selected_page = 0;
static lv_obj_t *pg1, *pg2, *pg3;
static lv_obj_t *tab_btns, *foot;
static lv_obj_t* tabv = NULL;


static lv_style_t style_bg;
extern lv_font_t select_font_channel[3];
lv_font_t select_font[3];
extern lv_font_t select_font_media[3];
extern lv_font_t select_font_mplist[3];

LV_FONT_DECLARE(font_china)
LV_FONT_DECLARE(myFont1)
LV_IMG_DECLARE(MAINMENU_IMG_PICTURE)
LV_IMG_DECLARE(MAINMENU_IMG_PICTURE_FOCUS1)
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
static lv_style_t style_item;

extern int set_flip_mode(flip_mode_e mode);
extern int set_keystone(int top_w, int bottom_w);
extern int set_color_temperature(int mode);
extern int media_mute(bool mute);


//static void update_tab_btns(void);



static void create_setup(void);
//static void clear_setup(void);
static void event_handler(lv_event_t* e);
static void slave_scr_event_handler(lv_event_t *e);
static void time_cb(lv_timer_t* timer_setting);
// void noise_reduction_btnmatrix_event(lv_event_t* e);
void sound_mode_btnmatrix_event(lv_event_t* e);
void bt_setting_btnmatrix_event(lv_event_t* e);
void color_temp_btnmatrix_event(lv_event_t* e);
void auto_sleep_btnsmatrix_event(lv_event_t *e);
// int display_bar_event(lv_event_t* e, int num, int lower, int high, int div, int width);
void picture_mode_event(lv_event_t* e);
void contrast_event(lv_event_t* e);
void brightness_event(lv_event_t* e);
void color_event(lv_event_t* e);
void sharpness_event(lv_event_t* e);
void color_temperature_event(lv_event_t* e);
// void noise_reduction_event(lv_event_t* e);
// int set_noise_redu(int v);
void sound_mode_event(lv_event_t* e);
int change_soundMode_event_(int k);
void treble_event(lv_event_t* e);
void bass_event(lv_event_t* e);
void balance_event(lv_event_t* e);
void change_pictureMode_event(lv_event_t* e);
int change_pictureMode_event_(int k);
// void change_balance_event(lv_event_t* e);
// void change_sharpness_event(lv_event_t* e);
// void change_brightness_event(lv_event_t* e);
// void change_color_event(lv_event_t* e);
void change_volume_event(lv_event_t* e);
// void change_contrast_event(lv_event_t* e);
// void change_treble_event(lv_event_t* e);
// void change_bass_event(lv_event_t *e);
void setup_item_event_(lv_event_t* e, widget widget1, int item);
void bt_setting_event(lv_event_t* e);
//static void create_message_box(char* str);
//static void bluetooth_wait();
extern bool str_is_black(char *str);
//static int bt_event(unsigned long event, unsigned long param);
void osd_language_event(lv_event_t* e);
void foot_event(lv_event_t* e);
void flip_event(lv_event_t* e);
void aspect_radio_event(lv_event_t* e);
static void aspect_ratio_btnmatrix_event(lv_event_t *e);
static int aspect_ratio_btnmatrix_event_(int);
void restore_factory_default_event(lv_event_t* e);
void software_update_event(lv_event_t* e);
static void software_update_bar_event(lv_event_t* e);
void keystone_event(lv_event_t *e);
void auto_sleep_event(lv_event_t *e);
static int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata);

void osd_btnmatrix_event(lv_event_t* e);
void btnmatrix_event(lv_event_t* e, btn_matrix_func f);
void return_event(lv_event_t* e);
void tabv_event(lv_event_t* e);
void tabv_btns_event(lv_event_t* e);
void btn_event(lv_event_t* e);
void main_scr_event(lv_event_t* e);
static void set_picture(const void *scr, lv_obj_draw_part_dsc_t *dsc);

static bool is_digit(const char* str);
static void set_change_lang(btnmatrix_p *p, int index, int v);
void set_enhance1(int value, uint8_t op);
int set_twotone(int mode, int bass, int treble);
int set_balance(int v);
void _change_language(lv_obj_t* obj, OSD_LANGUAGE id,  lv_font_t *font);
void change_language(uint8_t id);

void set_label_text1(lv_obj_t *label,OSD_LANGUAGE id, char* color);

void label_set_text_color(lv_obj_t* label,const char* text, char* color);
lv_obj_t* create_item(lv_obj_t* section, choose_item * chooseItem);

lv_obj_t *new_widget_(lv_obj_t*, char* title, const char**,uint32_t index, int len, int w, int h);
lv_obj_t *create_widget_btnmatrix(lv_obj_t *parent,int w, int h,const char** btn_map, int len);
void picture_mode_widget( lv_obj_t*);
// void contrast_widget(lv_obj_t*);
// void brightness_widget(lv_obj_t*);
// void color_widget(lv_obj_t*);
// void sharpness_widget(lv_obj_t*);
void volume_widget();
void color_temp_widget(lv_obj_t*);
// void noise_reduction_widget(lv_obj_t*);


lv_obj_t* create_picutre_page(lv_obj_t* parent);
lv_obj_t* create_sound_page(lv_obj_t* parent);
lv_obj_t* create_setting_page(lv_obj_t* parent);
lv_obj_t* create_page_(lv_obj_t* parent, choose_item * data, int len);

void sound_mode_widget(lv_obj_t* e);
// void treble_widget(lv_obj_t* e);
// void bass_widget(lv_obj_t* e);
// void balance_widget(lv_obj_t* e);
void bt_setting_widget(lv_obj_t* e);
void osd_language_widget(lv_obj_t* e);
void flip_widget(lv_obj_t* e);
void aspect_ratio_widget(lv_obj_t* e);
void restore_factory_default_widget(lv_obj_t* e);
void software_update_widget(lv_obj_t* e);
void auto_sleep_widget(lv_obj_t *btn);
int set_auto_sleep(int mode);
static void auto_sleep_timer_handle(lv_timer_t *tiemr_);

void swap_color(lv_color_t* color1, lv_color_t* color2);
void flip(lv_color_t* buf, int dir);


void delete_from_list_event(lv_event_t* e);



void setup_screen_init(void){
    select_font[0] = lv_font_montserrat_26;
    select_font[1] = font_china;
    select_font[2] = myFont1;
    last_setting_key=0;
    sys_param_p =  projector_get_sys_param();
    setup_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_opa(setup_scr, LV_OPA_50, 0);
    lv_obj_set_style_bg_color(setup_scr, lv_palette_main(LV_PALETTE_BLUE), 0);
    slave_scr = lv_layer_top();
    //lv_obj_set_style_bg_color(slave_scr, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);
    /*Create a style for the shadow*/
    // static lv_style_t style_shadow;
    // lv_style_init(&style_shadow);
    // lv_style_set_text_opa(&style_shadow, LV_OPA_30);
    // lv_style_set_text_color(&style_shadow, lv_color_black());
    lv_obj_add_event_cb(setup_scr, event_handler, LV_EVENT_SCREEN_LOADED, 0);
    lv_obj_add_event_cb(setup_scr, event_handler, LV_EVENT_SCREEN_UNLOADED, 0);
    //lv_obj_add_event_cb(slave_scr, slave_scr_event_handler, LV_EVENT_ALL, 0);
    MY_EVENT_LOADED = lv_event_register_id();
    setup_g = lv_group_create();
    lv_group_t* g = lv_group_get_default();
    create_setup();
    lv_group_set_default(g);
    set_flip_mode(projector_get_some_sys_param(P_FLIP_MODE));
}

static void create_setup(void){
        lv_group_set_default(setup_g);

        tabv = lv_tabview_create(setup_scr, LV_DIR_TOP,LV_PCT(8));
        set_pad_and_border_and_outline(tabv);
    //    lv_obj_set_size(tabv,LV_PCT(100),LV_PCT(92));
        lv_obj_set_style_bg_color(tabv, lv_color_make(0,0,0), 0);
        //lv_obj_set_style_opa(tabv, LV_OPA_50, 0);

        pg1 = lv_tabview_add_tab(tabv, "#ffffff "LV_SYMBOL_VIDEO"#");
        pg2 = lv_tabview_add_tab(tabv, "#ffffff "LV_SYMBOL_AUDIO"#");
        pg3 = lv_tabview_add_tab(tabv, "#ffffff "LV_SYMBOL_LIST"#");

        create_picutre_page(pg1);
        create_sound_page(pg2);
        create_setting_page(pg3);

        lv_coord_t pad_width = (lv_coord_t)(lv_disp_get_hor_res(NULL)/11);

        tab_btns = lv_tabview_get_tab_btns(tabv);
        lv_group_focus_obj(tab_btns);
        lv_btnmatrix_set_btn_ctrl_all(tab_btns, LV_BTNMATRIX_CTRL_RECOLOR);
        lv_btnmatrix_clear_btn_ctrl_all(tab_btns, LV_BTNMATRIX_CTRL_CHECKABLE);
        lv_obj_set_style_pad_hor(tab_btns,(lv_coord_t)(pad_width*4), 0);
        lv_obj_set_style_bg_color(tab_btns, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_add_event_cb(tab_btns, tabv_btns_event, LV_EVENT_ALL, 0);

        //lv_obj_t *cont = lv_tabview_get_content(tabv);
        //lv_obj_set_style_bg_opa(cont, LV_OPA_50, 0);

        foot = lv_obj_create(setup_scr);
        lv_obj_set_size(foot,LV_PCT(100),LV_PCT(8));
        //language_choose_add_label(foot, (char *)foot_btn_map, 5);
        //set_btnmatrix_language(foot, selected_language);

        lv_obj_set_style_bg_color(foot, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_align(foot, LV_ALIGN_BOTTOM_MID, 0, 0);
        //lv_btnmatrix_set_btn_ctrl_all(foot, LV_BTNMATRIX_CTRL_RECOLOR);
        lv_obj_set_style_border_width(foot, 0, 0);
        lv_obj_set_style_outline_width(foot, 0 ,0);
        lv_obj_set_style_pad_all(foot, 0, 0);
       lv_obj_set_style_pad_gap(foot,0,0);
        lv_obj_set_style_radius(foot,0,0 );
      
        lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
        lv_obj_t *obj,*label,*img;
        lv_img_dsc_t* img_dsc[4] = {&MENU_IMG_LIST_MOVE, &MENU_IMG_LIST_TABLE, &MENU_IMG_LIST_OK, &MENU_IMG_LIST_MENU};
        //int8_t align_x = 0;
        for(int i=0; i<4;i++){
            obj = lv_obj_create(foot);
             lv_obj_set_size(obj, LV_PCT(24), LV_PCT(100));
            //lv_obj_align(obj, LV_ALIGN_LEFT_MID, LV_PCT(align_x), 0);
            //align_x=+25;
            lv_obj_set_style_pad_ver(obj, 0, 0);
            set_pad_and_border_and_outline(obj);
            
            lv_obj_set_style_radius(obj, 0, 0);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);

            obj = lv_label_create(obj);
            set_pad_and_border_and_outline(obj);
            lv_obj_set_style_pad_top(obj, 2, 0);
            lv_obj_set_size(obj, LV_PCT(60), LV_PCT(100));
            lv_obj_center(obj);
            lv_label_set_text(obj, " ");

            img = lv_img_create(obj);
            lv_img_set_src(img, img_dsc[i]);
            lv_obj_align(img, LV_ALIGN_TOP_LEFT, 0, 0);
            label = lv_label_create(obj);
            lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_TOP, 2, 0);
            lv_label_set_recolor(label, true);
            language_choose_add_label(label, foot_btn_map[i], 0);
            set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");
        }
       // lv_obj_add_event_cb(foot, foot_event, LV_EVENT_ALL, 0);
}


void create_balance_ball(lv_obj_t* parent, lv_coord_t radius, lv_coord_t width){
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_size(obj,LV_PCT(width),LV_PCT(100));
    lv_obj_set_style_bg_color(obj, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);
}

lv_obj_t * create_display_bar_widget(lv_obj_t *parent, int w, int h){
    //lv_obj_set_style_bg_color(parent, lv_color_make(12,12,12), 0);

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
    lv_obj_set_style_text_font(label, &select_font[projector_get_some_sys_param(P_OSD_LANGUAGE)], 0);
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
    lv_obj_set_style_text_font(label, &select_font[0], 0);
    lv_obj_center(label);
 
    return obj;
}


lv_obj_t* salve_scr_obj = NULL;
void del_setup_slave_scr_obj(){
    del_bt_wait_anim();
     if(salve_scr_obj){
        lv_obj_del(salve_scr_obj);
        salve_scr_obj = NULL;
    }
}
static void timer_setting_handler(lv_timer_t* timer_setting1){
    change_screen(projector_get_some_sys_param(P_CUR_CHANNEL));
    //lv_obj_clean(slave_scr);
    del_setup_slave_scr_obj();
    lv_obj_set_style_bg_opa(slave_scr, LV_OPA_0, 0);
    timer_setting = NULL;
    if(cur_btn && lv_obj_has_state(cur_btn, LV_STATE_PRESSED)){
        lv_obj_clear_state(cur_btn, LV_STATE_PRESSED);
         lv_obj_set_style_bg_color(cur_btn, lv_color_make(100,99,100), 0);
    }
    if(bluetooth_obj){
        bluetooth_obj = NULL;
    }
}

static void time_cb(lv_timer_t* timer_setting1){
    turn_to_setup_scr();
}


static void event_handler(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SCREEN_LOADED){
        // if (lv_obj_get_child_cnt(slave_scr) > 0){
        //   
        //     //return;
        // }
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
            lv_group_focus_obj(cur_btn);
            return;
        }
        lv_group_focus_obj(tab_btns);
        lv_tabview_set_act(tabv, 0, LV_ANIM_OFF);
        lv_btnmatrix_set_selected_btn(tab_btns, 0);
        if(cur_btn && lv_obj_has_state(cur_btn, LV_STATE_PRESSED)){
        lv_obj_clear_state(cur_btn, LV_STATE_PRESSED);
         lv_obj_set_style_bg_color(cur_btn, lv_color_make(100,99,100), 0);
    }
        timer_setting = lv_timer_create(timer_setting_handler, 15000, 0);
        lv_timer_set_repeat_count(timer_setting, 1);
        lv_timer_reset(timer_setting);
    } else if(code == LV_EVENT_SCREEN_UNLOADED){
        if(timer_setting){
            printf("DEL TIMER\n");
            lv_timer_del(timer_setting);
            timer_setting = NULL;
        }
       // lv_obj_clean(slave_scr);
        lv_obj_set_style_bg_opa(slave_scr, LV_OPA_0, 0);
        printf("slave_scr opa is %d\n", lv_obj_get_style_bg_opa(slave_scr, LV_PART_MAIN));

    }
}



lv_obj_t* create_picutre_page(lv_obj_t* parent){
    bool is_disable = projector_get_some_sys_param(P_PICTURE_MODE) ==  PICTURE_MODE_USER ? false : true;
    choose_item picture_items[] = {
            {.name=picture_mode_k,.value.v1=picture_mode_v[projector_get_some_sys_param(P_PICTURE_MODE)*2],.is_number = false,.is_disabled= false,.event_func=picture_mode_event},

            {.name=contrast_k, .value.v2= projector_get_some_sys_param(P_CONTRAST),.is_number=true,.is_disabled=is_disable, .event_func=contrast_event},

            {.name=brightness_k, .value.v2=projector_get_some_sys_param(P_BRIGHTNESS),.is_number=true, .is_disabled=is_disable,.event_func= brightness_event},

            {.name=color_k, .value.v2=projector_get_some_sys_param(P_COLOR),.is_number=true, .is_disabled=is_disable, .event_func=color_event},

            {.name=sharpness_k, .value.v2= projector_get_some_sys_param(P_SHARPNESS),.is_number=true, .is_disabled=is_disable, .event_func=sharpness_event},

            {.name=color_temperature_k, .value.v1=color_temperature_v[projector_get_some_sys_param(P_COLOR_TEMP)*2],.is_number=false,.is_disabled=false, .event_func=color_temperature_event},
    };
    set_enhance1(projector_get_some_sys_param(P_CONTRAST), P_CONTRAST);
    set_enhance1(projector_get_some_sys_param(P_BRIGHTNESS), P_BRIGHTNESS);
    set_enhance1(projector_get_some_sys_param(P_COLOR), P_COLOR);
    set_enhance1(projector_get_some_sys_param(P_SHARPNESS), P_SHARPNESS);
    set_enhance1(projector_get_some_sys_param(P_HUE), P_HUE);
    set_color_temperature(projector_get_some_sys_param(P_COLOR_TEMP));
    lv_obj_t * obj = create_page_(parent, picture_items, 6);
    int obj_indexs[4] = {P_CONTRAST, P_BRIGHTNESS, P_COLOR, P_SHARPNESS};
    for(int i=0; i<4; i++){
        picture_mode_items[i].obj = lv_obj_get_child(obj, i+1);
        picture_mode_items[i].index = obj_indexs[i];
    }
    lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(parent, 0), 0);
    language_choose_add_label(label, picture_mode, 0);
    set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");
    lv_obj_t *icon = lv_img_create(lv_obj_get_child(parent, 1));
    lv_img_set_src(icon, &MAINMENU_IMG_PICTURE);
    lv_obj_center(icon);
    return obj;
}


lv_obj_t* create_sound_page(lv_obj_t* parent){
    const char *devpath=NULL;
    bool is_disable = projector_get_some_sys_param(P_SOUND_MODE) == SOUND_MODE_USER ? false : true;
    char* bt_v = projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_OFF ? get_some_language_str("Off\0关闭\0Off", projector_get_some_sys_param(P_OSD_LANGUAGE)) :
                                                                                get_some_language_str("On\0打开\0On", projector_get_some_sys_param(P_OSD_LANGUAGE)) ;
    choose_item music_items[] = {
            {sound_mode_k,.value.v1=sound_mode_v[projector_get_some_sys_param(P_SOUND_MODE)*2],false, false, sound_mode_event},
            {treble_k, .value.v2=projector_get_some_sys_param(P_TREBLE) ,true, is_disable, treble_event},
            {bass_k,  .value.v2=projector_get_some_sys_param(P_BASS), true, is_disable, bass_event},
            {balance_k, .value.v2=projector_get_some_sys_param(P_BALANCE), true, false, balance_event},
            {bt_setting_k, .value.v1=bt_v,false, false, bt_setting_event}
    };
    set_twotone(SND_TWOTONE_MODE_USER, projector_get_some_sys_param(P_BASS), projector_get_some_sys_param(P_TREBLE));
   
    lv_obj_t *obj = create_page_(parent, music_items, 5);
    int obj_index[2] = {P_TREBLE, P_BASS};
     for(int i=0;i<2;i++){
        sound_mode_items[i].obj = lv_obj_get_child(obj, i+1);
        sound_mode_items[i].index = obj_index[i];
    }

    int np = fdt_node_probe_by_path("/hcrtos/bluetooth");
    if(np>0)
    {
        if(!fdt_get_property_string_index(np, "devpath", 0, &devpath))
        {
            if(bluetooth_init(devpath, bt_event) == 0){
                printf("%s %d bluetooth_init ok\n",__FUNCTION__,__LINE__);
            }else{
                printf("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);
            }
        }
    }
    else
        printf("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);

    lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(parent, 0), 0);
    language_choose_add_label(label, sound_mode, 0);
    set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");
    lv_obj_t *icon = lv_img_create(lv_obj_get_child(parent, 1));
    lv_img_set_src(icon, &MAINMENU_IMG_AUDIO);

    lv_obj_center(icon);
    return obj;
}

lv_obj_t* create_setting_page(lv_obj_t* parent){
    choose_item setting_items[] = {
            {osd_language_k, .value.v1=osd_language_v[projector_get_some_sys_param(P_OSD_LANGUAGE)],false , false, osd_language_event},
            {flip_k, .value.v1=flip_v[projector_get_some_sys_param(P_FLIP_MODE)],false, false, flip_event},
            {aspect_ratio_k , .value.v1=aspect_ratio_v[projector_get_some_sys_param(P_ASPECT_RATIO)*2],false, false, aspect_radio_event},
            {restore_factory_default_k, .value.v1=None ,false,false, restore_factory_default_event},
            {software_update_k, .value.v1=None,false, false, software_update_event},
            {keystone_k, .value.v1=None, false,false, keystone_event},
            {auto_sleep_k, .value.v1=auto_sleep_v[projector_get_some_sys_param(P_AUTOSLEEP)*2], false, false, auto_sleep_event}
    };
    set_aspect_ratio(projector_get_some_sys_param(P_ASPECT_RATIO));
    printf("keystone-dir: %d, keystone-step: %d", projector_get_some_sys_param(P_KEYSTONE_TOP), projector_get_some_sys_param(P_KEYSTOME_BOTTOM));
    set_keystone(projector_get_some_sys_param(P_KEYSTONE_TOP), projector_get_some_sys_param(P_KEYSTOME_BOTTOM));
    aspect_ratio_btnmatrix_event_(projector_get_some_sys_param(P_ASPECT_RATIO));
    lv_obj_t *obj = create_page_(parent, setting_items, 7);
    lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(parent, 0), 0);
    language_choose_add_label(label, option_mode, 0);
    set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");
    lv_obj_t *icon = lv_img_create(lv_obj_get_child(parent, 1));
    lv_img_set_src(icon, &MAINMENU_IMG_OPTIONS);
    lv_obj_center(icon);
    return obj;
}

lv_obj_t* create_page_(lv_obj_t* parent, choose_item * message, int len){
    //lv_obj_set_style_pad_left(parent, lv_disp_get_hor_res(NULL)/11, 0);
    //lv_obj_set_style_pad_right(parent, lv_disp_get_hor_res(NULL)/7, 0);
    lv_obj_set_style_pad_hor(parent, 0, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(parent, 0, 0);
    lv_obj_set_style_pad_ver(parent, 0, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_50, 0);

    lv_obj_t *obj1 = lv_obj_create(parent);
    lv_obj_set_style_radius(obj1, 0, 0);
    lv_obj_set_style_border_width(obj1, 0, 0);
    lv_obj_set_size(obj1,LV_PCT(22),LV_PCT(100));
    lv_obj_set_style_bg_color(obj1, lv_color_make(75,75,75), 0);
    //lv_obj_set_style_opa(obj1, LV_OPA_50, 0);

    lv_obj_t *label = lv_label_create(obj1);
    lv_label_set_recolor(label, true);
    lv_obj_center(label);


    obj1 = lv_obj_create(parent);
    lv_obj_set_size(obj1,LV_PCT(24),LV_PCT(100));
    lv_obj_set_style_bg_color(obj1, lv_color_make(75,75,75), 0);
    lv_obj_set_style_radius(obj1, 0, 0);
    lv_obj_set_style_border_width(obj1, 0, 0);
    //lv_obj_set_style_opa(obj1, LV_OPA_50, 0);

    lv_obj_t * menu = lv_obj_create(parent);
    set_pad_and_border_and_outline(menu);
    lv_obj_set_style_radius(menu, 0, 0);
    lv_obj_set_size(menu ,LV_PCT(54),LV_PCT(100));
    //lv_obj_set_pos(menu,LV_PCT(46) , 0);
    lv_obj_set_style_pad_ver(menu, (lv_coord_t )(lv_disp_get_ver_res(NULL)/6), 0);
    lv_obj_set_scrollbar_mode(menu, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(menu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(menu, 0, 0);
    lv_obj_set_style_bg_color(menu, lv_color_make(75,75,75), 0);
    //lv_obj_set_style_opa(menu, LV_OPA_50,0);
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

lv_obj_t* create_item(lv_obj_t* page, choose_item * chooseItem){
    lv_obj_t* cont = lv_btn_create(page);
    //lv_obj_set_user_data(cont, chooseItem->value);
    lv_obj_set_size(cont,LV_PCT(70),LV_PCT(14));
    lv_obj_set_style_clip_corner(cont, true, 0);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_set_style_bg_color(cont, lv_color_make(100,99,100), 0);
    lv_obj_set_style_opa(cont, LV_OPA_100, 0);
     lv_obj_add_event_cb(cont, chooseItem->event_func, LV_EVENT_ALL, cont);

    lv_obj_t* label = lv_label_create(cont);
    language_choose_add_label(label, chooseItem->name, 0);
    lv_label_set_recolor(label, true);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    if (!chooseItem->is_disabled){
        set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE),  "#ffffff ");
    }else{
        set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#afafaf ");
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_CLICKABLE);
    }

    label = lv_label_create(cont);

    lv_label_set_recolor(label, true);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
   

    if(!chooseItem->is_number && strcmp(chooseItem->value.v1, None) != 0){
        language_choose_add_label(label, chooseItem->value.v1, 0);
        if (!chooseItem->is_disabled){
            set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");
        }else{
            
            set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#afafaf ");
        }
    }else if(chooseItem->is_number){
        if(!chooseItem->is_disabled){
            lv_label_set_text_fmt(label,"#ffffff %d#",  chooseItem->value.v2);
        }else{
            lv_label_set_text_fmt(label,"#afafaf %d#",  chooseItem->value.v2);
        }
        lv_obj_set_style_text_font(label, &select_font[0], 0);
    }else{
        lv_label_set_text(label, None);
    }
    
    



    return cont;
}

lv_obj_t* create_widget_head(lv_obj_t* parent, char *title, int h){
    lv_obj_t* head = lv_obj_create(parent);
    set_pad_and_border_and_outline(head);
    lv_obj_set_style_pad_ver(head, 0, 0);
    lv_obj_set_size(head,LV_PCT(100),LV_PCT(h));
    lv_obj_set_style_radius(head, 0, 0);
    lv_obj_set_style_border_width(head, 0, 0);
    lv_obj_set_style_bg_color(head, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_t* label = lv_label_create(head);

    lv_label_set_recolor(label, true);
    language_choose_add_label(label, title, 0);
    set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");
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
    
    lv_obj_t *btn1,*btn2,*img, *label;
    for(int i=0; i<2; i++){
        btn1 = lv_obj_create(foot1);
        lv_obj_set_size(btn1,LV_PCT(50),LV_PCT(100));
        set_pad_and_border_and_outline(btn1);
        lv_obj_set_style_pad_ver(btn1, 0, 0);
        lv_obj_set_style_radius(btn1, 0, 0);
        lv_obj_set_style_bg_color(btn1, lv_palette_darken(LV_PALETTE_GREY, 3), 0);

        btn2 = lv_label_create(btn1);
        lv_obj_center(btn2);
        lv_obj_set_style_radius(btn2, 0, 0);
        set_pad_and_border_and_outline(btn2);
        lv_label_set_text(btn2, " ");
        lv_obj_set_style_pad_ver(btn2, 0, 0);
        lv_obj_set_size(btn2, LV_PCT(60), LV_PCT(100));
        lv_obj_set_flex_flow(btn2, LV_FLEX_FLOW_ROW);
        img = lv_img_create(btn2);
        lv_obj_align(img, LV_ALIGN_LEFT_MID, 0, 0);
        lv_img_set_src(img, i == 0 ? &MENU_IMG_LIST_OK : &MENU_IMG_LIST_MENU);
        label = lv_label_create(btn2);
        // lv_obj_center(label);
        lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_TOP, 2, 2);
        lv_label_set_recolor(label, true);
        if(i==0){
            language_choose_add_label(label, foot_sure, 0);
        }else{
            language_choose_add_label(label, foot_menu, 0);
        }
       set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");

    }
}

lv_obj_t *create_new_widget(int w, int h){
    lv_obj_t* page = lv_obj_create(slave_scr);
    salve_scr_obj = page;
    lv_obj_set_style_opa(page, LV_OPA_70, 0);
    lv_obj_set_size(page,LV_PCT(w),LV_PCT(h));
    lv_obj_center(page);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(page, 0, 0);
    lv_obj_set_style_pad_gap(page, 0, 0);
    set_pad_and_border_and_outline(page);
    lv_obj_set_style_pad_ver(page, 0, 0);
    lv_obj_set_style_shadow_width(page, 0, 0);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    return page;
}

lv_obj_t *create_widget_btnmatrix(lv_obj_t *parent,int w, int h,const char** btn_map, int len){
    lv_obj_t* matrix_bnt = lv_btnmatrix_create(parent);
    lv_group_focus_obj(matrix_bnt);
    lv_obj_set_size(matrix_bnt,LV_PCT(w),LV_PCT(h));
    language_choose_add_label(matrix_bnt, (char*)btn_map, len);
    set_btnmatrix_language(matrix_bnt, projector_get_some_sys_param(P_OSD_LANGUAGE));
    lv_btnmatrix_set_btn_ctrl_all(matrix_bnt,  LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(matrix_bnt, true);
    INIT_STYLE_BG(&style_bg);
    lv_obj_add_style(matrix_bnt, &style_bg, 0);
    INIT_STYLE_ITEM(&style_item);
    lv_obj_add_style(matrix_bnt, &style_item, LV_PART_ITEMS);
    return matrix_bnt;
}

lv_obj_t *new_widget_(lv_obj_t* btn,  char* title, const char** btn_map,uint32_t index, int len, int w, int h){
    lv_obj_t* image_mode = create_new_widget(33, 50);
    lv_obj_set_style_opa(image_mode, LV_OPA_100, 0);
    create_widget_head(image_mode, title, 15);
    lv_obj_t *mid = lv_obj_create(image_mode);
    lv_obj_set_style_bg_color(mid, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    set_pad_and_border_and_outline(mid);
    lv_obj_set_style_pad_ver(mid, 0, 0);
    lv_obj_set_style_radius(mid, 0, 0);

    lv_obj_set_size(mid, LV_PCT(100), LV_PCT(72));
    lv_obj_set_scrollbar_mode(mid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *matrix_btn = create_widget_btnmatrix(mid, LV_PCT((w == 0 ? 100 : w)), LV_PCT((h == 0 ? 100 : h)), btn_map, len);
    char* text = lv_label_get_text(lv_obj_get_child(btn, 1));
    char chs[10];
    memset(chs, 0, 10);
    strncpy(chs, text+8, strlen(text)-9);
    const char * map;
    for(int i=0; i<len; i++){
        map = get_some_language_str(btn_map[i], projector_get_some_sys_param(P_OSD_LANGUAGE));
        if (strcmp(chs, map) == 0){
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



static int picture_mode_pre_text[4];
void picture_mode_widget(lv_obj_t* btn){
    //static const char* title = "Picture Mode\0图像模式\0Picture Mode";
    lv_obj_t * obj = new_widget_(btn, picture_mode_k, picture_mode_v, projector_get_some_sys_param(P_PICTURE_MODE), 12, 0, 0);
    for(int i=0; i<4; i++){
        picture_mode_pre_text[i] = projector_get_some_sys_param(picture_mode_items[i].index);
    }
    lv_obj_add_event_cb(lv_obj_get_child(lv_obj_get_child(obj, 1),0), change_pictureMode_event, LV_EVENT_ALL, btn);
    // lv_timer_reset(timer_setting);
    // lv_timer_resume(timer_setting);
}

int change_pictureMode_event_(int k){
    //uint16_t k = lv_btnmatrix_get_selected_btn(target);
    projector_set_some_sys_param(P_PICTURE_MODE, k);
    
    uint8_t ops[4] = {P_CONTRAST, P_BRIGHTNESS, P_COLOR, P_SHARPNESS};
    
    
    char* values[4] = { k==0 ? "50" : k==1 ? "60" : "45",
                        k==0 ? "50" : k==1 ? "49" : "50",
                        k==0 ? "50" : k==1 ? "60" : "45",
                        k==0 ? "5" : k==1 ? "5" : "4"};
    lv_obj_t *obj;
    switch (k) {
        case PICTURE_MODE_STANDARD:
        case PICTURE_MODE_DYNAMIC:
        case PICTURE_MODE_MILD:
            for(int i=0; i< 4; i++){
                set_enhance1(strtol(values[i], NULL, 10), ops[i]);
                obj = picture_mode_items[i].obj;
                if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)){
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                }
                label_set_text_color(lv_obj_get_child(obj, 0), lv_label_get_text(lv_obj_get_child(obj, 0)), "#afafaf ");
                label_set_text_color(lv_obj_get_child(obj, 1), values[i], "#afafaf ");
                //lv_obj_set_style_text_font(lv_obj_get_child(obj, 1), &select_font[0], 0);
            }
            printf("\n");
            break;
        case PICTURE_MODE_USER:
            for (int i = 0; i < 4; i++) {
                set_enhance1(picture_mode_pre_text[i], ops[i]);
                obj = picture_mode_items[i].obj;
                if(!lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)){
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                }
                label_set_text_color(
                        lv_obj_get_child(obj, 0),
                        lv_label_get_text(
                                lv_obj_get_child(obj,
                                                 0)),
                        "#ffffff ");
                lv_label_set_text_fmt(lv_obj_get_child(obj, 1), "#ffffff %d#", picture_mode_pre_text[i]);
                //lv_obj_set_style_text_font(lv_obj_get_child(obj, 1), &select_font[0], 0);
            }
            printf("\n");
            break ;
        default:
            break;
    }

    return 0;
}

void change_pictureMode_event(lv_event_t* e){
    btnmatrix_event(e, change_pictureMode_event_);
}



void color_temp_widget(lv_obj_t* btn){
    static const char* title = "Color Temperate\0色温";
    lv_obj_t *obj = new_widget_(btn, title, color_temperature_v,projector_get_some_sys_param(P_COLOR_TEMP), 12,0,0);
    
    lv_obj_add_event_cb(lv_obj_get_child(lv_obj_get_child(obj, 1), 0), color_temp_btnmatrix_event, LV_EVENT_ALL, btn);
}

void color_temp_btnmatrix_event(lv_event_t* e){
    btnmatrix_event(e, set_color_temperature);
}




static int sound_mode_pre_text[2];
void sound_mode_widget(lv_obj_t* btn){
    // static const char *title = "Sound Mode\0声音模式\0Sound Mode";
    lv_obj_t * obj = new_widget_(btn, sound_mode_k, sound_mode_v,projector_get_some_sys_param(P_SOUND_MODE), 12,0,0);
    for(int i=0; i<2; i++){
        sound_mode_pre_text[i] = projector_get_some_sys_param(sound_mode_items[i].index);
    }
    lv_obj_add_event_cb(lv_obj_get_child(lv_obj_get_child(obj, 1), 0), sound_mode_btnmatrix_event, LV_EVENT_ALL, btn);

}

void sound_mode_btnmatrix_event(lv_event_t* e){
    btnmatrix_event(e, change_soundMode_event_);
}

int change_soundMode_event_(int k){
    lv_obj_t* obj;
    char *values[2] = {
        k==0 ? "0" : k==1 ? "5" : k==2 ? "6" : "-3" ,
         k==0 ? "0" : k==1 ? "5" : k==2 ? "5" : "-3" 
    };


    switch (k){
    case SOUND_MODE_STANDARD:
    case SOUND_MODE_MOVIE:
    case SOUND_MODE_MUSIC:
    case SOUND_MODE_SPORTS:
        set_twotone(k, atoi(values[0]), atoi(values[1]));
         for(int i=0; i< 2; i++){
                obj = sound_mode_items[i].obj;
                if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)){
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                }
                label_set_text_color(lv_obj_get_child(obj, 0), lv_label_get_text(lv_obj_get_child(obj, 0)), "#afafaf ");
                label_set_text_color(lv_obj_get_child(obj, 1), values[i], "#afafaf ");
            }
        break;
    case SOUND_MODE_USER:
        
        for (int i = 0; i < 2; i++) {
            
        obj = sound_mode_items[i].obj;
        if(i==0){
            set_twotone(k, P_TREBLE, sound_mode_pre_text[0]);
        }else{
            set_twotone(k, P_BASS, sound_mode_pre_text[1]);
        }
        
        if(!lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)){
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        label_set_text_color(
                lv_obj_get_child(obj, 0),
                lv_label_get_text(
                        lv_obj_get_child(obj,
                                            0)),
                "#ffffff ");
        lv_label_set_text_fmt(lv_obj_get_child(obj, 1), "#ffffff %d#", sound_mode_pre_text[i]);
    }
        break;   
    
    default:
        break;
    }
   
 return 0;
}



void osd_language_widget(lv_obj_t* btn){
    lv_obj_t *osd_lang_wid = create_new_widget(33,33);
    lv_obj_set_style_opa(osd_lang_wid, LV_OPA_100, 0);
    static const char* title = "OSD Language\0OSD语言";
    create_widget_head(osd_lang_wid, title, 33);

    lv_obj_t *osd_body = lv_obj_create(osd_lang_wid);
    set_pad_and_border_and_outline(osd_body);
    lv_obj_set_style_pad_ver(osd_body, 0, 0);
    lv_obj_set_size(osd_body,LV_PCT(100),LV_PCT(44));
    lv_obj_set_style_radius(osd_body, 0, 0);

    lv_obj_t *osd_body_sub = lv_obj_create(osd_body);
    set_pad_and_border_and_outline(osd_body_sub);
    lv_obj_set_style_pad_ver(osd_body_sub, 0, 0);
    lv_obj_set_size(osd_body_sub,LV_PCT(100),LV_PCT(100));
    lv_obj_set_style_radius(osd_body_sub, 0, 0);
    lv_obj_set_style_bg_color(osd_body_sub, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    lv_obj_set_scrollbar_mode(osd_body_sub, LV_SCROLLBAR_MODE_OFF);

    
    lv_obj_t *body_btnmatrix = create_widget_btnmatrix(osd_body_sub, 100, 100, osd_language_v, LANGUAGE_LEN+1);
    //lv_obj_t *body_btnmatrix = create_widget_btnmatrix(osd_body_sub, 300, 100, osd_language_v, LANGUAGE_LEN+1);
    lv_obj_add_event_cb(body_btnmatrix, osd_btnmatrix_event, LV_EVENT_ALL, btn);
    INIT_VISIBLE();
    //lv_obj_scroll_to(osd_body_sub, (lv_coord_t)(lv_disp_get_hor_res(NULL)/9* visible[0]), 0, LV_ANIM_OFF);
    lv_btnmatrix_set_selected_btn(body_btnmatrix, projector_get_some_sys_param(P_OSD_LANGUAGE));
    lv_btnmatrix_set_btn_ctrl(body_btnmatrix, projector_get_some_sys_param(P_OSD_LANGUAGE), LV_BTNMATRIX_CTRL_CHECKED);

    lv_obj_t *btn1 = lv_btn_create(osd_body);
lv_obj_set_style_shadow_width(btn1, 0, 0);
    lv_obj_set_style_radius(btn1, 0 , 0);
    lv_obj_set_size(btn1,LV_PCT(8) ,LV_PCT(100));
    lv_obj_align(btn1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(btn1, LV_OPA_20, 0);

    lv_obj_t *label1 = lv_label_create(btn1);
    lv_obj_set_style_bg_opa(label1, LV_OPA_0, 0);
    lv_label_set_text(label1, "<");
    lv_obj_center(label1);

    btn1 = lv_btn_create(osd_body);
    lv_obj_set_style_shadow_width(btn1, 0, 0);
    lv_obj_set_style_radius(btn1, 0 , 0);
    lv_obj_set_size(btn1,LV_PCT(8) ,LV_PCT(100));
    lv_obj_align(btn1, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(btn1, LV_OPA_20, 0);

    label1 = lv_label_create(btn1);
    lv_obj_set_style_bg_opa(label1, LV_OPA_0, 0);
    lv_label_set_text(label1, ">");
    lv_obj_center(label1);

    create_widget_foot(osd_lang_wid, 24, btn);
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
    language_choose_add_label(lv_obj_get_child(btn, 1), flip_v[i], 0);
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text(lv_obj_get_child(btn, 1), id, "#ffffff ");
}

static int aspect_ratio_btnmatrix_event_(int k){
    int w = lv_disp_get_hor_res(lv_disp_get_default());
    int h = lv_disp_get_ver_res(lv_disp_get_default());
    if(k == ASPECT_RATIO_AUTO){
        //set_zoom(0,0,w/2*3, h/2*3, 0, 0, w/2*3, h/2*3);
        set_aspect_ratio(DIS_TV_AUTO);
        projector_set_some_sys_param(P_ASPECT_RATIO, ASPECT_RATIO_AUTO);
    }else if(k==ASPECT_RATIO_4_3){
        //set_zoom(0,0,w/2*3, h/2*3, 0, 0, w/2*3, h/2*3);
        projector_set_some_sys_param(P_ASPECT_RATIO, ASPECT_RATIO_4_3);
        set_aspect_ratio(DIS_TV_4_3);
    }else if(k==ASPECT_RATIO_16_9){
        //set_zoom(0,0,w/2*3, h/2*3, 0, 0, w/2*3, h/2*3);
        projector_set_some_sys_param(P_ASPECT_RATIO, ASPECT_RATIO_16_9);
        set_aspect_ratio(DIS_TV_16_9);
    }else if(k==ZOOM_IN){
        // int fd = open("/dev/dis", O_RDONLY);
        // if(fd < 0){
        //     printf("open dis failed");
        //     return 0;
        // }
        // dis_screen_info_t info = {0};
        // ioctl(fd, DIS_GET_SCREEN_INFO, &info);
        set_aspect_ratio(DIS_TV_AUTO);
        projector_set_some_sys_param(P_ASPECT_RATIO, ZOOM_IN);
        set_zoom(w/8*3, h/8*3, w/4*3, h/4*3, 0, 0, w/2*3, h/2*3);
        //close(fd);
    }else if(k==ZOOM_OUT){
        //  int fd = open("/dev/dis", O_RDONLY);
        // if(fd < 0){
        //     printf("open dis failed");
        //     return 0;
        // }
        // dis_screen_info_t info = {0};
        // ioctl(fd, DIS_GET_SCREEN_INFO, &info);
        set_aspect_ratio(DIS_TV_AUTO);
        projector_set_some_sys_param(P_ASPECT_RATIO, ZOOM_OUT);

        set_zoom(0,0,w/2*3,h/2*3, w/8*3,h/8*3,w/4*3,h/4*3);
        //close(fd);
    }
    // lv_coord_t pad_width = (lv_coord_t)(lv_disp_get_hor_res(NULL)/11);
    // lv_obj_set_style_pad_hor(tab_btns,(lv_coord_t)(pad_width*4), 0);
    return 0;
}

static void aspect_ratio_btnmatrix_event(lv_event_t *e){
   btnmatrix_event(e, aspect_ratio_btnmatrix_event_);
}

void aspect_ratio_widget(lv_obj_t* btn){
    static const char* asp_str = "Aspect Ratio \0 画面比例\0Aspect Ratio";
    lv_obj_t * obj = new_widget_(btn,asp_str, aspect_ratio_v,projector_get_some_sys_param(P_ASPECT_RATIO), 12,0,0);
    lv_obj_add_event_cb(lv_obj_get_child(lv_obj_get_child(obj, 1), 0),  aspect_ratio_btnmatrix_event , LV_EVENT_ALL, btn);
}

void restory_factory_default_event_cb(lv_event_t *e){
    lv_obj_t * obj = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            if(lv_msgbox_get_active_btn(obj) == 0){
                projector_factory_reset();
                projector_sys_param_save();
                hw_watchdog_reset(10);
            } else{
                lv_obj_del(target->parent);
                turn_to_setup_scr();
            }
        }
    }
    
}

void restore_factory_default_widget(lv_obj_t* btn){
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    static const char * btns[3];
    btns[0] = get_some_language_str("Ok\0确定\0Ok", id);
    btns[1] = get_some_language_str("Close\0关闭\0Close", id);
    btns[2] = "";
    static const char* mbox1_str = "Ok?\0确定？\0Ok?";
    
    lv_obj_t * mbox1 = lv_msgbox_create(slave_scr, "", get_some_language_str(mbox1_str, id), btns, false);
    salve_scr_obj = mbox1;
    lv_obj_t *label = lv_msgbox_get_content(mbox1);
    lv_obj_set_style_text_font(label, &select_font[id], 0);

    lv_obj_t *btns_obj = lv_msgbox_get_btns(mbox1);
     lv_obj_set_style_text_font(btns_obj, &select_font[id], 0);
    
    lv_obj_set_style_bg_color(mbox1, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    lv_obj_add_event_cb(mbox1,restory_factory_default_event_cb, LV_EVENT_ALL,NULL);
    lv_obj_center(mbox1);
    lv_group_focus_obj(lv_msgbox_get_btns(mbox1));
}




static bool find_software(DIR *d, int depth, char str[][40]){
    struct dirent *file;
    struct stat sd;
    if(depth>=4){
        return false;
    }

    while((file = readdir(d)) != NULL){
        if(strncmp(file->d_name, ".", 1) == 0){
            continue;
        }
        if(strcmp(file->d_name+strlen(file->d_name)-4, ".bin") == 0){
           
           strcpy((char*)str[depth], file->d_name);

            return true;
        }
        char file_name[150];
        memset(file_name, 0, 150);
        strcat(file_name, "/media/");
        for(int i=0; i<depth; i++){
            strcat(file_name, str[i]);
        }
        strcat(file_name, file->d_name);
        if(stat(file_name, &sd)>=0 && S_ISDIR(sd.st_mode) && depth<4){
            DIR *temp;
            strcat(file_name, "/");
            if(!(temp = opendir(file_name))){
                continue;
            }
            strcpy((char*)str[depth], file->d_name);
            strcat((char*)str[depth], "/");
            bool result =  find_software(temp, depth+1, str);
            if(result){
                return result;
            }
        }
    }
    
    return false;
}

static int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata){
    if(event == HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS){
        lv_obj_t* obj = (lv_obj_t*)usrdata;
        lv_bar_set_value(lv_obj_get_child(obj, 0), param, LV_ANIM_OFF);
        char ss[5];
        memset(ss, 0, 4);
        sprintf(ss, "%ld%%", param);
        lv_label_set_text(lv_obj_get_child(obj, 1), ss);
    }
    return 0;
}

static void software_update_handler(void *target_){
    lv_obj_t *target = (lv_obj_t*)target_;
    lv_obj_t *obj = lv_obj_get_child(slave_scr, 0);
    int rc = hcfota_url((char*)target->user_data, hcfota_report, (unsigned long)target);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(obj);
    lv_obj_t *label = lv_label_create(obj);
    lv_obj_center(label);
    lv_label_set_recolor(label, true);
    if(rc==0){
        lv_label_set_text(label, "#ffffff upgrade sucess#!");
        lv_timer_del(timer_setting_clone);
         timer_setting_clone = NULL;
        hcfota_reboot(hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_USB_DEVICE, 0));
    }else{

        switch (rc){
            case HCFOTA_ERR_LOADFOTA:
                lv_label_set_text(label, "#ffffff load file err!#");
                break;
            case HCFOTA_ERR_HEADER_CRC:
                lv_label_set_text(label, "#ffffff header crc err!#");
                break;
            case HCFOTA_ERR_VERSION:
                lv_label_set_text(label, "#ffffff version err!#");
                break;
            case HCFOTA_ERR_DECOMPRESSS:
                lv_label_set_text(label, "#ffffff decompress err!#");
                break;
            default:
                break;
        }
        lv_timer_resume(timer_setting_clone);
        lv_timer_reset(timer_setting_clone);
        timer_setting = timer_setting_clone;
        timer_setting_clone = NULL;
    }
   
    lv_mem_free(target->user_data);
}
static void software_update_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    //lv_obj_t *btn = lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            lv_obj_del(target);
            turn_to_setup_scr();

           
        }else if(key == LV_KEY_HOME){
            lv_obj_del(target);
            turn_to_setup_scr();
        }
    }
}



void software_update_widget(lv_obj_t* btn){

    lv_obj_t *obj = lv_obj_create(slave_scr);
    salve_scr_obj = obj;
    lv_obj_set_style_radius(obj, 20, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_group_add_obj(setup_g, obj);
    lv_group_focus_obj(obj);
    lv_obj_add_event_cb(obj, software_update_event_handle, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(obj, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_size(obj,LV_PCT(25),LV_PCT(25));
    lv_obj_center(obj);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *label;
    DIR *d = opendir("/media/");
   if(!d){
       label = lv_label_create(obj);
       lv_obj_center(label);
       lv_label_set_recolor(label, true);
       language_choose_add_label(label, software_no_device, 0);
       set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");
       return;
   }
    char urls[4][40];
    memset(urls, 0, 160);
    
    
   if(!find_software(d, 0, urls)){
       label = lv_label_create(obj);
       lv_obj_center(label);
       lv_label_set_recolor(label, true);
       language_choose_add_label(label, software_no_software, 0);
       set_label_text(label, projector_get_some_sys_param(P_OSD_LANGUAGE), "#ffffff ");
       return;
   }
    char* url = lv_mem_alloc(150);
    memset(url, 0, 150);
    strcat(url, "/media/");
    for(int i=0; i<3; i++){
        printf("%s\n", urls[i]);
        strcat(url, urls[i]);
        if(strcmp(url+strlen(url)-4, ".bin") == 0){
            break;
        }
    }
    printf("%s\n", url);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t *obj1 = lv_obj_create(slave_scr);
    lv_obj_set_style_outline_width(obj1, 0, 0);
    lv_obj_set_style_border_width(obj1, 0, 0);

    lv_group_add_obj(setup_g, obj1);
    lv_group_focus_obj(obj1);
    lv_obj_set_style_bg_color(obj1, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_scrollbar_mode(obj1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(obj1, 20, 0);
    lv_obj_set_size(obj1,LV_PCT(35),LV_PCT(25));
    lv_obj_center(obj1);
    

    lv_obj_t *sub_obj = lv_obj_create(obj1);
    lv_obj_set_scrollbar_mode(sub_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_outline_width(sub_obj, 0, 0);
    lv_obj_set_style_border_width(sub_obj, 0, 0);
    lv_obj_set_style_pad_all(sub_obj, 0, 0);
    lv_obj_set_style_bg_color(sub_obj, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_center(sub_obj);
    lv_obj_set_size(sub_obj,LV_PCT(100), LV_PCT(45));
    lv_obj_set_flex_align(sub_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_flex_flow(sub_obj, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *bar = lv_bar_create(sub_obj);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_size(bar, LV_PCT(100), LV_PCT(45));
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);

    label = lv_label_create(sub_obj);
    lv_obj_set_size(label, LV_PCT(10), LV_PCT(50));
    lv_label_set_text(label, "0%");
    sub_obj->user_data = url;
    //lv_obj_add_event_cb(sub_obj, software_update_bar_event, LV_EVENT_ALL, obj);
    lv_timer_pause(timer_setting);
    timer_setting_clone = timer_setting;
    timer_setting = NULL;
    xTaskCreate(software_update_handler, "software update", 0x1000, sub_obj, portPRI_TASK_NORMAL , NULL);
    
}

static lv_obj_t *sleep_obj;
static lv_obj_t *auto_sleep_prev_obj = NULL;
static lv_timer_t *auto_sleep_sure_timer;
static lv_timer_t *auto_sleep_timer = NULL;
static int countdown = 60;
void auto_sleep_widget(lv_obj_t *btn){
    lv_obj_t * obj = new_widget_(btn, auto_sleep_k, auto_sleep_v,projector_get_some_sys_param(P_AUTOSLEEP), 12,0,0);
    lv_obj_add_event_cb(lv_obj_get_child(lv_obj_get_child(obj, 1), 0), auto_sleep_btnsmatrix_event, LV_EVENT_ALL, btn);
}

void auto_sleep_btnsmatrix_event(lv_event_t *e){
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

static void auto_sleep_timer_handle(lv_timer_t *timer_){
    
    sleep_obj = lv_obj_create(slave_scr);
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

int set_auto_sleep(int mode){
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


void setup_item_event_(lv_event_t* e, widget widget1, int item){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(target, 0);
    lv_obj_t * label1 = lv_obj_get_child(target, 1);
    
    if (code == LV_EVENT_CLICKED) {
//        slave_scr = lv_obj_create(NULL);
//        lv_scr_load(slave_scr);
//        widget1(target);
     } else if(code == LV_EVENT_PRESSED){
         lv_obj_set_style_bg_color(target, lv_palette_darken(LV_PALETTE_BLUE, 1), 0);
    }else if(code == LV_EVENT_DEFOCUSED){
         
        lv_obj_set_style_bg_color(target, lv_color_make(100,99,100), 0);
        char *text = lv_label_get_text(label);
        char *text1 = lv_label_get_text(label1);
        label_set_text_color(label, text, lv_obj_has_flag(target, LV_OBJ_FLAG_CLICKABLE) ? "#ffffff " : "#afafaf ");
        label_set_text_color(label1, text1, lv_obj_has_flag(target, LV_OBJ_FLAG_CLICKABLE) ? "#ffffff " : "#afafaf ");
    }
    else if (code == LV_EVENT_FOCUSED) {
        lv_obj_set_style_bg_color(target, lv_palette_darken(LV_PALETTE_BLUE, 1), 0);
        lv_obj_clear_state(target, LV_STATE_PRESSED);
        char *text = lv_label_get_text(label);
        label_set_text_color(label, text, "#000000 ");
        text = lv_label_get_text(label1);
        label_set_text_color(label1, text, "#000000 ");

    }else if (code == LV_EVENT_KEY) {
        lv_timer_pause(timer_setting);
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_DOWN || key == LV_KEY_UP) {
            //lv_obj_set_style_bg_color(target, lv_palette_darken(LV_PALETTE_BLUE, 1), LV_STATE_FOCUSED);
            lv_group_t* lvGroup = lv_group_get_default();
            focus_next_or_pre focusNextOrPre;
            focusNextOrPre = key == LV_KEY_DOWN ? lv_group_focus_next : lv_group_focus_prev;
            focusNextOrPre(lvGroup);
            lv_obj_t *focused = lv_group_get_focused(lvGroup);
            uint16_t  pg_id = lv_tabview_get_tab_act(tabv);
            while ((focused->parent->parent != target->parent->parent) || !lv_obj_has_flag(focused, LV_OBJ_FLAG_CLICKABLE)){
                if (focused == tab_btns || focused == foot){
                    break;
                }
                lv_obj_set_style_bg_color(focused, lv_color_make(100,99,100), 0);
                focusNextOrPre(lvGroup);
                focused = lv_group_get_focused(lvGroup);
            }
            cur_btn = focused;
            lv_tabview_set_act(tabv, pg_id, LV_ANIM_OFF);
           
        }else if (key == LV_KEY_ENTER || key == LV_KEY_RIGHT){
           if(widget1){
                widget1(target);
                if(item != P_FLIP_MODE){
                    lv_obj_set_style_bg_opa(slave_scr, LV_OPA_50, 0);
                }
           }else{
                int v = projector_get_some_sys_param(item);    
                if(item == P_CONTRAST || item == P_COLOR ||  item == P_BRIGHTNESS){
                        if(v >= 0 && v<100){
                            v += 1;
                            set_enhance1(v, item); 
                        }
                }else if(item == P_BASS || item == P_TREBLE){
                    if(v>=-10 && v<10){
                        v+=1;
                        if(item == P_BASS){
                            set_twotone(SND_TWOTONE_MODE_USER, v, projector_get_some_sys_param(P_TREBLE));
                        }else{
                            set_twotone(SND_TWOTONE_MODE_USER, projector_get_some_sys_param(P_BASS), v);
                        }
                    }
                }else if(item == P_BALANCE){
                    if(v >=-24 && v<24){
                        v+=1;
                        set_balance(v);
                    }
                }else if(item == P_SHARPNESS){
                    if(v>=0 && v<10){
                        v+=1;
                        set_enhance1(v, item);
                    }
                }
                lv_label_set_text_fmt(lv_obj_get_child(target, 1), "%d", v);
                projector_set_some_sys_param(item, v);
           }


        }else if(key == LV_KEY_LEFT){
            bool change = true;
            int v = projector_get_some_sys_param(item);
            if(item == P_CONTRAST || item == P_COLOR ||  item == P_BRIGHTNESS){
                if(v>0 && v <101){
                    v -= 1;
                    set_enhance1(v, item);
                }
            }else if(item == P_BASS || item == P_TREBLE){
                if(v>-10 && v<=10){
                    v-=1;
                    if(item == P_BASS){
                        set_twotone(SND_TWOTONE_MODE_USER, v, projector_get_some_sys_param(P_TREBLE));
                    }else{
                        set_twotone(SND_TWOTONE_MODE_USER, projector_get_some_sys_param(P_BASS), v);
                    }
                }
            }else if(item == P_BALANCE){
                if(v>-24 && v<=24){
                    v-=1;
                    set_balance(v);
                }
            }else if(item == P_SHARPNESS){
                if(v>0 && v<11){
                    v-=1;
                    set_enhance1(v, item);
                }
            }else{
                change = false;
            }
            if(change){
                lv_label_set_text_fmt(lv_obj_get_child(target, 1), "%d", v);
                projector_set_some_sys_param(item, v);
            }
        }
        else if (key == LV_KEY_ESC && key != last_setting_key){
            turn_to_main_scr();
//            lv_timer_del(settings_timer_setting);
            // lv_timer_ready(timer_setting);
            // lv_timer_resume(timer_setting);
            return ;
        }
        last_setting_key = key;
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
        
    }
    if (code == LV_EVENT_DRAW_PART_BEGIN){
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
void sharpness_event(lv_event_t* e){
    setup_item_event_(e, NULL, P_SHARPNESS);

}
void color_temperature_event(lv_event_t* e){
    setup_item_event_(e, color_temp_widget, P_COLOR_TEMP);

}
// void noise_reduction_event(lv_event_t* e){
//     setup_item_event_(e, noise_reduction_widget, true);

// }

void sound_mode_event(lv_event_t* e){
    setup_item_event_(e, sound_mode_widget, P_SOUND_MODE);

}
void treble_event(lv_event_t* e){
    setup_item_event_(e, NULL, P_TREBLE);

}
void bass_event(lv_event_t* e){
    setup_item_event_(e, NULL, P_BASS);

}
void balance_event(lv_event_t* e){
    setup_item_event_(e, NULL, P_BALANCE);

}


void set_enhance1(int value, uint8_t op){
    hcfb_enhance_t eh = {0};
    int fd = -1;
    int ret = 0;

    fd = open("/dev/fb0" , O_RDWR);
    if( fd < 0){
        printf("open /dev/fb0 failed, ret=%d\n", fd);
        return;
    }

    ret = ioctl(fd, HCFBIOGET_ENHANCE, &eh);
    if( ret != 0 ){
        printf("%s:%d: warning: HCFBIOGET_ENHANCE failed\n", __func__, __LINE__);
        close(fd);
        fd = -1;
        return;
    }

    switch (op) {
        case P_BRIGHTNESS:
            eh.brightness = value;
            projector_set_some_sys_param(P_BRIGHTNESS, value);
            break;

        case P_CONTRAST:
            eh.contrast = value<10 ? 10 : value;
            projector_set_some_sys_param(P_CONTRAST, value);
            break;

        case P_COLOR:
            eh.saturation = value;
            projector_set_some_sys_param(P_COLOR, value);
            break;

        case P_HUE:
            eh.hue = value;
            projector_set_some_sys_param(P_HUE, value);
            break;

        case P_SHARPNESS:
            eh.sharpness = value;
            projector_set_some_sys_param(P_SHARPNESS, value);
            break;

        default:
            break;
    }

     projector_sys_param_save();
    ret = ioctl(fd, HCFBIOSET_ENHANCE, &eh);
    if( ret != 0 ){
        printf("%s:%d: warning: HCFBIOSET_ENHANCE failed\n", __func__, __LINE__);
        close(fd);
        fd = -1;
        return;
    }

    close(fd);
    fd = -1;
}

int set_twotone(int mode, int bass, int treble){
    
	int snd_fd = -1;

	struct snd_twotone tt = {0};
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("twotone open snd_fd %d failed\n", snd_fd);
		return -1;
	}
    tt.tt_mode = mode;
    tt.onoff = 1;

    if(bass >= -10 && bass <= 10){
        tt.bass_index = bass;
        projector_set_some_sys_param(P_BASS, bass);
    }
    if(treble >= -10 && treble <= 10){
        tt.treble_index = treble;
        projector_set_some_sys_param(P_TREBLE, treble);
    }
    projector_set_some_sys_param(P_SOUND_MODE, mode);
    projector_sys_param_save();

	ioctl(snd_fd, SND_IOCTL_SET_TWOTONE, &tt);
	close(snd_fd);
	return 0;
}

int set_balance(int v){
    int snd_fd = -1;

    struct snd_lr_balance lr = {0};

    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("lr_balance open snd_fd %d failed\n", snd_fd);
		return -1;
	}

    lr.lr_balance_index = v;
    lr.onoff = 1;

    projector_set_some_sys_param(P_BALANCE, v);
    projector_sys_param_save();
    ioctl(snd_fd, SND_IOCTL_SET_LR_BALANCE, &lr);
	close(snd_fd);
	return 0;
}

// void change_balance_event(lv_event_t* e){
//     int v = display_bar_event(e, projector_get_some_sys_param(P_BALANCE), -24, 24, 2, 4);
//     set_balance(v);
// }

// void change_treble_event(lv_event_t* e){
//     int v =  display_bar_event(e, projector_get_some_sys_param(P_TREBLE), -10, 10, 1, 5);
//     set_twotone(SND_TWOTONE_MODE_USER, projector_get_some_sys_param(P_BASS), v);
// }

// void change_bass_event(lv_event_t *e){
//     int v =  display_bar_event(e, projector_get_some_sys_param(P_BASS), -10, 10, 1, 5);
//     set_twotone(SND_TWOTONE_MODE_USER, v, projector_get_some_sys_param(P_TREBLE));
// }

// void change_contrast_event(lv_event_t* e){
//     int v =  display_bar_event(e, projector_get_some_sys_param(P_CONTRAST), 0, 100, 4, 4);
//     set_enhance1(v, P_CONTRAST);
// }

// void change_brightness_event(lv_event_t* e){
//     int v = display_bar_event(e,projector_get_some_sys_param(P_BRIGHTNESS), 0, 100, 4, 4);
//     set_enhance1(v, P_BRIGHTNESS);
// }

// void change_color_event(lv_event_t* e){
//     int v = display_bar_event(e, projector_get_some_sys_param(P_COLOR), 0, 100, 4, 4);
//     set_enhance1(v, P_COLOR);
// }

// void change_sharpness_event(lv_event_t* e){
//     int v = display_bar_event(e, projector_get_some_sys_param(P_SHARPNESS), 0, 100, 4, 4);
//     set_enhance1(v, P_SHARPNESS);
// }

void bt_setting_event(lv_event_t* e){
    setup_item_event_(e, bt_setting_widget1, P_BT_SETTING);

}

void osd_language_event(lv_event_t* e){
    setup_item_event_(e, osd_language_widget, P_OSD_LANGUAGE);

}
void flip_event(lv_event_t* e){
    setup_item_event_(e, flip_widget, P_FLIP_MODE);

}
void aspect_radio_event(lv_event_t* e){
    setup_item_event_(e, aspect_ratio_widget, P_ASPECT_RATIO);
//    lv_event_code_t code = lv_event_get_code(e);

//    if(code == LV_EVENT_KEY){
//         uint32_t key = lv_indev_get_key(lv_indev_get_act());
//         if (key == LV_KEY_ENTER){
            
//             if(aspect_ratio == DIS_PANSCAN){
//                 set_aspect_ratio(DIS_NORMAL_SCALE);
//             }else{
//                 set_aspect_ratio(DIS_PANSCAN);
//             }
            
//         }
//    }

}
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

void btnmatrix_event(lv_event_t* e, btn_matrix_func func){
    lv_obj_t* target = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn= lv_event_get_user_data(e);
    uint32_t index = (uint32_t)btn->user_data;
    if (code == LV_EVENT_KEY){
        lv_timer_pause(timer_setting);
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_ENTER){
            uint32_t btn_id = lv_btnmatrix_get_selected_btn(target);
            const char* text = lv_btnmatrix_get_btn_text(target, btn_id);
            lv_obj_t* lab = lv_obj_get_child(btn, 1);
            lab->user_data = (void*)((btnmatrix_p*)target->user_data)->str_p_s[btn_id*2];
            btn->user_data = (void*)(btn_id*2);
//            memset(btn->user_data, 0, strlen(btn->user_data));
//            strcat(btn->user_data, text);
            lv_label_set_text(lab ,text);
            if (func){
                func(btn_id);
            }
            lv_obj_del(target->parent->parent);
           turn_to_setup_scr();
           return; 
        }
        if (key == LV_KEY_DOWN || key == LV_KEY_UP){
            static lv_obj_t *obj = NULL;
            obj == NULL && (obj = target);
            obj != target && ( obj = target);
            key == LV_KEY_DOWN && btnmatrix_choose_id++;
            key == LV_KEY_UP && btnmatrix_choose_id--;

            if (key == LV_KEY_DOWN && strcmp(lv_btnmatrix_get_btn_text(target, btnmatrix_choose_id), " ")  == 0){
                lv_btnmatrix_set_selected_btn(target, 0);
                btnmatrix_choose_id = 0;
            }
            if(key == LV_KEY_UP &&  btnmatrix_choose_id < 0){
                btnmatrix_choose_id = NEW_WIDGET_LINE_NUM-1;
                while (strcmp(lv_btnmatrix_get_btn_text(target, btnmatrix_choose_id), " ")  == 0){
                    --btnmatrix_choose_id;
                }
            }
            lv_btnmatrix_set_selected_btn(target, btnmatrix_choose_id);
            lv_btnmatrix_set_btn_ctrl(target, btnmatrix_choose_id, LV_BTNMATRIX_CTRL_CHECKED);


            //const char* text = lv_btnmatrix_get_btn_text(target, btnmatrix_choose_id);
            // lv_obj_t* lab = lv_obj_get_child(btn, 1);
            // lab->user_data = (void*)((btnmatrix_p*)target->user_data)->str_p_s[btnmatrix_choose_id*2];
            // *index = btnmatrix_choose_id*2;
            //lv_label_set_text(lab ,text);
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
            lv_obj_del(target->parent->parent);
            turn_to_setup_scr();
        }
        if(key ==LV_KEY_ESC ){
            if(index != btnmatrix_choose_id && func){
                func(index);
            }
            lv_obj_del(target->parent->parent);
            turn_to_setup_scr();
        }
        last_setting_key = key;
        lv_timer_reset(timer_setting);
        lv_timer_resume(timer_setting);
    }
    if(code == LV_EVENT_DRAW_PART_BEGIN) {
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        // if (lv_btnmatrix_get_selected_btn(target) == dsc->id){
        //     dsc->rect_dsc->outline_width = 0;
        // }
        dsc->rect_dsc->outline_width=0;
    }
}

void osd_btnmatrix_event(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *btn = lv_event_get_user_data(e);
    static int index = -2;
    index == -2 && (index = projector_get_some_sys_param(P_OSD_LANGUAGE));
    if(code == LV_EVENT_KEY){
        lv_timer_pause(timer_setting);
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
       
        if (key == LV_KEY_RIGHT){
            index = (index+1)%2;
            // index++;
            // if (index >= LANGUAGE_LEN){
            //     index = 0;
            //     visible[0] = 0;
            //     visible[1] = 1;
            //     visible[2] = 2;
            //     lv_obj_scroll_by(target->parent, (lv_coord_t )(lv_disp_get_hor_res(NULL)/9*(LANGUAGE_LEN-3)), 0, LV_ANIM_OFF);
            // }else if (index != visible[0] && index!= visible[1] && index != visible[2]){
            //     lv_obj_scroll_by(target->parent, (lv_coord_t )(-lv_disp_get_hor_res(NULL)/9), 0, LV_ANIM_OFF);
            //     visible[0] =  visible[1];
            //     visible[1] = visible[2];
            //     visible[2] = index;
            // }
        }
        if(key == LV_KEY_LEFT){
            index = (index-1)%2;
            // index--;
            // if (index < 0){
            //     index =LANGUAGE_LEN-1;
            //     visible[2] = index;
            //     visible[1] = index-1;
            //     visible[0] = index-2;
            //     lv_obj_scroll_by(target->parent, (lv_coord_t )(-lv_disp_get_hor_res(NULL)/9*(LANGUAGE_LEN-3)), 0, LV_ANIM_OFF);
            // }else if(index != visible[0] &&index != visible[1] && index != visible[2]){
            //     lv_obj_scroll_by(target->parent, (lv_coord_t )(lv_disp_get_hor_res(NULL)/9), 0, LV_ANIM_OFF);
            //     visible[2] = visible[1];
            //     visible[1] = visible[0];
            //     visible[0] = index;
            // }
        }
        lv_btnmatrix_set_btn_ctrl(target, index, LV_BTNMATRIX_CTRL_CHECKED);
        pre_selected_language = index;
        if (key == LV_KEY_HOME){
            index = projector_get_some_sys_param(P_OSD_LANGUAGE);
            lv_obj_del(target->parent->parent->parent);
            turn_to_setup_scr();
        } 
        if(key == LV_KEY_ESC){
            index = projector_get_some_sys_param(P_OSD_LANGUAGE);
            lv_obj_del(target->parent->parent->parent);
            turn_to_setup_scr();
        }
        if (key == LV_KEY_ENTER){
            uint16_t id  = lv_btnmatrix_get_selected_btn(target);
//            language_choose_ *languageChoose_i;
//            _LV_LL_READ(&labelBtnmList.label_list, languageChoose_i){
//                set_label_text(languageChoose_i, id, NULL);
//            }
//            language_choose *languageChoose_j;
//            _LV_LL_READ(&labelBtnmList.btnmatrix_list, languageChoose_j){
//                set_btnmatrix_language(languageChoose_j, id);
//            }
            if(id == English || id == Chinese){
                change_language(id);
                projector_set_some_sys_param(P_OSD_LANGUAGE, id);
                projector_sys_param_save();
                lv_label_set_text(lv_obj_get_child(btn, 1), get_some_language_str(osd_language_v[id], id));
            }
            lv_obj_del(target->parent->parent->parent);
            turn_to_setup_scr();
        }
        last_setting_key = key;
        lv_timer_reset(timer_setting);
        lv_timer_resume(timer_setting);
    }
    if(code == LV_EVENT_DRAW_PART_BEGIN) {
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        if (lv_btnmatrix_get_selected_btn(target) == dsc->id){
            //dsc->rect_dsc->bg_color = lv_palette_darken(LV_PALETTE_BLUE, 2);
            //dsc->rect_dsc->border_color = lv_palette_darken(LV_PALETTE_BLUE, 2);
            dsc->rect_dsc->radius = 0;
            dsc->rect_dsc->outline_width = 0;
            //dsc->rect_dsc->outline_color = lv_palette_darken(LV_PALETTE_BLUE, 2);
        }
    }
    if(code == LV_EVENT_DELETE){
        lv_obj_set_style_bg_opa(slave_scr, LV_OPA_0,0);
    }
}

void return_event(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED){
        turn_to_setup_scr();
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
        if(dsc->id == 0) {

            if (dsc->id == lv_tabview_get_tab_act(target->parent)){
                set_picture(&MAINMENU_IMG_PICTURE_FOCUS1, dsc);
            }else{
                set_picture(&MAINMENU_IMG_PICTURE_S_UNFOCUS, dsc);
            }
        }
        if(dsc->id == 1){
            if (dsc->id == lv_tabview_get_tab_act(target->parent)){
                set_picture(&MAINMENU_IMG_AUDIO_FOCUS, dsc);
            }else{
                set_picture(&MAINMENU_IMG_AUDIO_S_UNFOCUS, dsc);
            }
        }
        if(dsc->id == 2){
            if (dsc->id == lv_tabview_get_tab_act(target->parent)){
                set_picture(&MAINMENU_IMG_OPTION_FOCUS, dsc);
            }else{
                set_picture(&MAINMENU_IMG_OPTIONS_S_UNFOCUS, dsc);
            }
        }
    }
    if (code == LV_EVENT_KEY){
        lv_timer_pause(timer_setting);
        uint16_t  key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_ESC){
            lv_timer_resume(timer_setting);
            lv_timer_ready(timer_setting);
            return;
        }else if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT){
            uint16_t id = lv_btnmatrix_get_selected_btn(target);
            selected_page = id;
            lv_tabview_set_act(target->parent, id, LV_ANIM_OFF);
        }else if (key == LV_KEY_DOWN || key == LV_KEY_UP){
            uint16_t id = lv_btnmatrix_get_selected_btn(target);
            lv_obj_t *pg = id==0 ?pg1 :id==1 ?pg2 :pg3;
            focus_next_or_pre focusNextOrPre;
            focusNextOrPre = key == LV_KEY_DOWN ? lv_group_focus_next : lv_group_focus_prev;
            focusNextOrPre(lvGroup);
            target = lv_group_get_focused(lvGroup);
            lv_obj_t *label;
            char* text;
            while (target->parent->parent != pg){
                for(int i=0; i<2; i++){
                    label = lv_obj_get_child(target, i);
                    text = lv_label_get_text(label);
                    label_set_text_color(label, text, "#ffffff ");
                }
                lv_obj_set_style_bg_color(target, lv_color_make(100,99,100), 0);
                focusNextOrPre(lvGroup);
                target = lv_group_get_focused(lvGroup);
            }
            cur_btn = target;
        }
        last_setting_key = key;
        lv_timer_reset(timer_setting);
        lv_timer_resume(timer_setting);
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

//void tabv_event(lv_event_t* e){
//    lv_event_code_t code = lv_event_get_code(e);
//    if (code == LV_EVENT_KEY){
//        uint16_t key = lv_indev_get_key(lv_indev_get_act());
//        if (key == LV_KEY_ESC){
//            lv_timer_del(timer_setting);
//            turn_to_main_scr();
//            return ;
//        }
//    }
//}


void turn_to_setup_scr(){
    if (setup_scr){
        //lv_scr_load(setup_scr);
        lv_group_focus_obj(cur_btn);
    }
    // if(slave_scr){
    //     lv_obj_del(temp_scr);
    // }
    //out_setting = true;
    //lv_obj_clean(slave_scr);
    salve_scr_obj = NULL;
    lv_obj_set_style_bg_opa(slave_scr, LV_OPA_0, 0);
    projector_sys_param_save();
    // lv_timer_reset(timer_setting);
    // lv_timer_resume(timer_setting);
}

void turn_to_main_scr(){
    // lv_timer_del(timer_setting);
    // timer_setting = NULL;
    lv_timer_ready(timer_setting);
    lv_timer_resume(timer_setting);
    projector_sys_param_save();
    //change_screen(projector_get_some_sys_param(P_CUR_CHANNEL));
}

static bool is_digit(const char* str){
    if(strlen(str)>3){
        return false;
    }
    uint16_t ch = ' ';
    uint16_t i=0;
    while ((ch = str[i++]) != '\0'){
        if (!isdigit(ch) || ch == ':'){
            return false;
        }
    }
    return true;
}


const char* get_some_language_str(const char *str, int index){
    if (is_digit(str) ||  strcmp(str, " ") == 0 || strcmp(str, "") == 0 || strcmp(str, "\n")==0){
        return str;
    }
    uint8_t i=0;
    while (index){
        for(; str[i] != '\0';i++);
        index--;
        i++;
    }
    return str + i;
}


btnmatrix_p* language_choose_add_label(lv_obj_t* label,const char* p, uint8_t len){
    if (label->class_p == & lv_btnmatrix_class){
        lv_obj_add_event_cb(label, delete_from_list_event, LV_EVENT_DELETE, NULL);
        btnmatrix_p *btnmatrixP = (btnmatrix_p*)lv_mem_alloc(sizeof(btnmatrix_p));
        btnmatrixP->len = len;
        btnmatrixP->str_p_p = (const char **)lv_mem_alloc(sizeof(char *)*len);
        btnmatrixP->change_lang = (const uint8_t*)lv_mem_alloc(sizeof(uint8_t)*len);
        for(int i=0; i<len; i++){
            btnmatrixP->change_lang[i] = 1;
        }
        btnmatrixP->str_p_s = (const char **)p;
        label->user_data = (void*)btnmatrixP;
        return btnmatrixP;
    }else{
        label->user_data = p;
    }
    return NULL;
}

static void set_change_lang(btnmatrix_p *p, int index, int v){
    p->change_lang[index] = v;
}

void language_choose_add_label1(lv_obj_t *label, uint32_t i){
    label->user_data = (void*)i;
}

void delete_from_list_event(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (code == LV_EVENT_DELETE){
        lv_mem_free(((btnmatrix_p*)target->user_data)->str_p_p);
        lv_mem_free(((btnmatrix_p*)target->user_data)->change_lang);
        lv_mem_free(target->user_data);

    }
}

void set_label_text(lv_obj_t * label,int  id, char* color){
   set_label_text_with_font(label, id, color, &select_font[id]);
}

void set_label_text_with_font(lv_obj_t * label,int  id, char* color, lv_font_t* font){
    char* text = lv_label_get_text(label);
    if(label->user_data){
        char *str = get_some_language_str(label->user_data, id);
        if (color != NULL){
            //printf("%s\n", color);
            label_set_text_color(label, str,color);
        }else{
            char color_pre[9];
            strncpy(color_pre, text, 8);
            color_pre[8]='\0';
            if(color_pre[0] == '#'){
                label_set_text_color(label, str, color_pre);
            }else{
               lv_label_set_text(label, str);
            }
        }
        lv_obj_set_style_text_font(label, font, 0);
    }
    lv_obj_set_style_text_font(label, font, 0);
}



void set_btnmatrix_language(lv_obj_t * obj,int  id){
    btnmatrix_p *p = ((btnmatrix_p*)obj->user_data);
    for(int j=0; j<p->len; j++){
        if(p->change_lang[j]){
            p->str_p_p[j] = get_some_language_str(p->str_p_s[j], id);
        }else{
            p->str_p_p[j]  = get_some_language_str(p->str_p_s[j], 0);
        }
    }
    lv_btnmatrix_set_map(obj, p->str_p_p);
    lv_obj_set_style_text_font(obj, &select_font[id], 0);
}

void _change_language(lv_obj_t* obj, OSD_LANGUAGE id, lv_font_t *font){
    for(int i=0; i< lv_obj_get_child_cnt(obj); i++){
        lv_obj_t *temp = lv_obj_get_child(obj, i);
        if (temp->class_p == &lv_label_class && temp->user_data){
            set_label_text_with_font(temp, id, NULL, font);
            //set_label_text(temp, id, NULL);
        } else if(temp->class_p == &lv_btnmatrix_class && temp->user_data){
            set_btnmatrix_language(temp, id);
        }else{
            _change_language(temp, id, font);
        }
    }
}

void change_language(uint8_t id){
    lv_disp_t *disp = lv_disp_get_default();
    _change_language(slave_scr, id,&select_font[id]);
    _change_language(setup_scr, id,&select_font[id]);
    _change_language(channel_scr, id,&select_font_channel[id]);
    _change_language(ui_mainpage, id,&select_font_media[id]);
    _change_language(ui_subpage, id,&select_font_media[id]);
    _change_language(ui_fspage, id,&select_font_mplist[id]);
    _change_language(ui_ctrl_bar, id,&select_font_mplist[id]);

    for(int i=0; i<disp->screen_cnt; i++){
        // change_language_(disp->screens[i], id);
    }
}
