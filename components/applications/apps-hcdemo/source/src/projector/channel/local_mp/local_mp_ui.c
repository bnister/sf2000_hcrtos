// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.0.5
// LVGL VERSION: 8.2
// PROJECT: project_ui_Project

#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key.h"

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>
#include "win_media_list.h"

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
// #include "local_mp_ui_lld.h"
#include "mp_mainpage.h"
#include "mp_subpage.h"
#include "mp_fspage.h"
#include "mp_ctrlbarpage.h"
#include "mp_ebook.h"
#include "src/font/lv_font.h"
#include "../../setup/setup.h"
#include "../../factory_setting.h"
///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_mainpage;
lv_obj_t * ui_imgmovie;
lv_obj_t * ui_imgmusic;
lv_obj_t * ui_imgphoto;
lv_obj_t * ui_imgtext;
lv_obj_t * ui_btnmovie;
lv_obj_t * ui_labmovie;
lv_obj_t * ui_btnmusic;
lv_obj_t * ui_labmusic;
lv_obj_t * ui_btnphoto;
lv_obj_t * ui_labphoto;
lv_obj_t * ui_btntext;
lv_obj_t * ui_labtext;
lv_obj_t * ui_usb_img;
lv_obj_t * ui_usb_lab;
lv_obj_t * ui_subpage;
lv_obj_t * ui_btnback;
lv_obj_t * ui_labback;
lv_obj_t * ui_btnrootdir;
lv_obj_t * ui_labrootdir;
lv_obj_t * ui_title_on;
lv_obj_t * ui_Image2;
lv_obj_t * ui_Image3;
lv_obj_t * ui_titie_;
lv_obj_t * ui_fspage;
lv_obj_t * ui_fsobj0;
lv_obj_t * ui_fsimg0;
lv_obj_t * ui_fsobj1;
lv_obj_t * ui_fsimg1;
lv_obj_t * ui_fsobj2;
lv_obj_t * ui_fsimg2;
lv_obj_t * ui_fsobj3;
lv_obj_t * ui_fsimg3;
lv_obj_t * ui_fsobj4;
lv_obj_t * ui_fsimg4;
lv_obj_t * ui_fsobj5;
lv_obj_t * ui_fsimg5;
lv_obj_t * ui_fsobj6;
lv_obj_t * ui_fsimg6;
lv_obj_t * ui_fsobj7;
lv_obj_t * ui_fsimg7;
lv_obj_t * ui_fsobj8;
lv_obj_t * ui_fsimg8;
lv_obj_t * ui_fsobj9;
lv_obj_t * ui_fsimg9;
lv_obj_t * ui_fsobj10;
lv_obj_t * ui_fsimg10;
lv_obj_t * ui_fsobj11;
lv_obj_t * ui_fsimg11;
lv_obj_t * ui_fsbarimg;
lv_obj_t * ui_fscount;
lv_obj_t * ui_fspath;
lv_obj_t * ui_fstitle;
lv_obj_t * ui_ctrl_bar;
lv_obj_t * ui_play_bar;
lv_obj_t * ui_playbar;
lv_obj_t * ui_winbar;
lv_obj_t * ui_barbtn0;
lv_obj_t * ui_barlab;
lv_obj_t * ui_barbtn1;
lv_obj_t * ui_barlab1;
lv_obj_t * ui_barbtn2;
lv_obj_t * ui_barlab2;
lv_obj_t * ui_barbtn3;
lv_obj_t * ui_barlab3;
lv_obj_t * ui_barbtn4;
lv_obj_t * ui_barlab4;
lv_obj_t * ui_barbtn5;
lv_obj_t * ui_barlab5;
lv_obj_t * ui_barbtn6;
lv_obj_t * ui_barlab6;
lv_obj_t * ui_cur_time;
lv_obj_t * ui_total_time;
lv_obj_t * ui_str;
lv_obj_t * ui_playname;
lv_obj_t * ui_playstate;
lv_obj_t * ui_pervimg;
lv_obj_t * ui_nextimg;
lv_obj_t * ui_state;
lv_obj_t * ui_speed;
lv_obj_t * ui_error_tip;
lv_obj_t * ui_tip_img;
lv_obj_t * ui_tip_lab;
lv_obj_t * ui_win_zoom;
lv_obj_t * ui_win_name;
lv_obj_t * ui_name;
lv_obj_t * ui_file_info;
lv_obj_t * ui_info_s;
lv_obj_t * ui_info_s1;
lv_obj_t * ui_info_t_;
lv_obj_t * ui_info_t1;
lv_obj_t * ui_ebook_txt;
lv_obj_t * ui_ebook_label;
lv_obj_t * ui_ebook_label;
lv_obj_t * ui_ebook_label_page;
/*some event can not update in sq so user handle in here*/
lv_group_t * main_group;
lv_group_t * sub_group;
lv_group_t * fs_group;
lv_group_t * play_bar_group;
lv_group_t * ebook_group;
extern file_list_t *m_cur_file_list;
extern file_list_t  m_file_list[3]; 
extern obj_list_ctrl_t m_list_ctrl;
extern char * m_cur_fullname;
extern lv_obj_t *fs_lab[1024];
extern exit_code_t fsobj0_exit_code;
extern lv_obj_t * fs_obj[12];
extern int usb_state;
///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 32
    #error "LV_COLOR_DEPTH should be 32bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP !=0
    #error "#error LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings"
#endif

///////////////////// ANIMATIONS ////////////////////
//font text to do  
lv_font_t select_font_media[3];
lv_font_t select_font_mplist[3];
char* movie_k = {"Movie\0" "电影\0" "Movie\0"};
char* music_k = {"Music\0" "音乐\0" "Music\0"};
char* photo_k = {"Photo\0" "照片\0" "Photo\0"};
char* text_k = {"E-book\0" "电子书\0" "E-book\0"};
char* back_k = {"Back\0" "返回\0" "Back\0"};
char* rootdir_k = {"C\0" "C\0" "C\0"};
char* bar_play_k = {"Play\0" "播放\0" "Play\0"};
char* bar_pause_k = {"Pause\0" "暂停\0" "Pause\0"};
char* bar_ff_k = {"FF\0" "快进\0" "FF\0"};
char* bar_fb_k = {"FB\0" "快退\0" "FB\0"};
char* bar_next_k = {"Next\0" "下一首\0""Next\0"};
char* bar_prev_k = {"Prev\0" "上一首\0" "Prev\0"};
char* bar_round_k = {"Round\0" "重复\0" "Round\0"};

///////////////////// FUNCTIONS ////////////////////
static void ui_event_mainpage(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    // lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOAD_START) {
        mainpage_open();
    }
    if(event == LV_EVENT_SCREEN_UNLOAD_START) {
        mainpage_close();
    }
}
static void ui_event_subpage(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    // lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOAD_START) {
        subpage_open();
    }
    if(event == LV_EVENT_SCREEN_UNLOAD_START)
    {
        subpage_close();
    }

}
static void ui_event_fspage(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    // lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOAD_START) {
        media_fslist_open();
    }
    if(event == LV_EVENT_SCREEN_UNLOAD_START) {
        media_fslist_close();
    }
}
static void ui_event_ctrl_bar(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    // lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOAD_START) {
        play_bar_open();
    }
    if(event == LV_EVENT_SCREEN_UNLOAD_START) {
        play_bar_close();
    }
}
static void ui_event_ebook(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    // lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOAD_START) {
        ebook_open();
    }
    if(event == LV_EVENT_SCREEN_UNLOAD_START)
    {
        ebook_close();
    }

}


///////////////////// SCREENS ////////////////////
void ui_mainpage_screen_init(void)
{

    select_font_media[0] = lv_font_montserrat_40;
    select_font_media[1] = font40_china;
    select_font_media[2] = lv_font_montserrat_40;
    // ui_mainpage

    ui_mainpage = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_mainpage, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_mainpage, ui_event_mainpage, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(ui_mainpage, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_mainpage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_imgmovie

    ui_imgmovie = lv_img_create(ui_mainpage);
    lv_img_set_src(ui_imgmovie, &ui_img_dmp_media_type_movie_png);

    lv_obj_set_width(ui_imgmovie, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_imgmovie, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_imgmovie, 97);
    lv_obj_set_y(ui_imgmovie, 250);

    lv_obj_add_flag(ui_imgmovie, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_imgmovie, LV_OBJ_FLAG_SCROLLABLE);

    // ui_imgmusic

    ui_imgmusic = lv_img_create(ui_mainpage);
    lv_img_set_src(ui_imgmusic, &ui_img_dmp_media_type_music_png);

    lv_obj_set_width(ui_imgmusic, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_imgmusic, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_imgmusic, 392);
    lv_obj_set_y(ui_imgmusic, 250);

    lv_obj_add_flag(ui_imgmusic, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_imgmusic, LV_OBJ_FLAG_SCROLLABLE);

    // ui_imgphoto

    ui_imgphoto = lv_img_create(ui_mainpage);
    lv_img_set_src(ui_imgphoto, &ui_img_dmp_media_type_photo_png);

    lv_obj_set_width(ui_imgphoto, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_imgphoto, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_imgphoto, 687);
    lv_obj_set_y(ui_imgphoto, 250);

    lv_obj_add_flag(ui_imgphoto, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_imgphoto, LV_OBJ_FLAG_SCROLLABLE);

    // ui_imgtext

    ui_imgtext = lv_img_create(ui_mainpage);
    lv_img_set_src(ui_imgtext, &ui_img_dmp_media_type_txt_png);

    lv_obj_set_width(ui_imgtext, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_imgtext, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_imgtext, 985);
    lv_obj_set_y(ui_imgtext, 250);

    lv_obj_add_flag(ui_imgtext, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_imgtext, LV_OBJ_FLAG_SCROLLABLE);

    // ui_btnmovie

    ui_btnmovie = lv_btn_create(ui_mainpage);

    lv_obj_set_width(ui_btnmovie, 180);
    lv_obj_set_height(ui_btnmovie, 80);

    lv_obj_set_x(ui_btnmovie, -445);
    lv_obj_set_y(ui_btnmovie, 145);

    lv_obj_set_align(ui_btnmovie, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_btnmovie, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_btnmovie, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_btnmovie, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_btnmovie, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_btnmovie, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_btnmovie, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_btnmovie, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_btnmovie, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_btnmovie, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_btnmovie, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btnmovie, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(ui_btnmovie, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(ui_btnmovie, lv_color_hex(0xFAD665), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_btnmovie, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_btnmovie, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_btnmovie, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_color(ui_btnmovie, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_opa(ui_btnmovie, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_labmovie

    ui_labmovie = lv_label_create(ui_btnmovie);

    lv_obj_set_width(ui_labmovie, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_labmovie, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_labmovie, 0);
    lv_obj_set_y(ui_labmovie, 0);

    lv_obj_set_align(ui_labmovie, LV_ALIGN_CENTER);

    // lv_label_set_recolor(ui_labmovie, true); 不需要
    language_choose_add_label(ui_labmovie,movie_k,0);
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_labmovie, id,0,&select_font_media[id]);
    // lv_label_set_text(ui_labmovie, movie_k);


    // ui_btnmusic

    ui_btnmusic = lv_btn_create(ui_mainpage);

    lv_obj_set_width(ui_btnmusic, 180);
    lv_obj_set_height(ui_btnmusic, 80);

    lv_obj_set_x(ui_btnmusic, -150);
    lv_obj_set_y(ui_btnmusic, 145);

    lv_obj_set_align(ui_btnmusic, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_btnmusic, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_btnmusic, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_btnmusic, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_btnmusic, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_btnmusic, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_btnmusic, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_btnmusic, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_btnmusic, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_btnmusic, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_btnmusic, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btnmusic, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(ui_btnmusic, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(ui_btnmusic, lv_color_hex(0xFAD665), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_btnmusic, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_btnmusic, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_btnmusic, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_color(ui_btnmusic, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_opa(ui_btnmusic, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_labmusic

    ui_labmusic = lv_label_create(ui_btnmusic);

    lv_obj_set_width(ui_labmusic, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_labmusic, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_labmusic, 0);
    lv_obj_set_y(ui_labmusic, 0);

    lv_obj_set_align(ui_labmusic, LV_ALIGN_CENTER);

    // lv_label_set_text(ui_labmusic, music_k);
    language_choose_add_label(ui_labmusic,music_k,0);
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_labmusic, id,0,&select_font_media[id]);

    // ui_btnphoto

    ui_btnphoto = lv_btn_create(ui_mainpage);

    lv_obj_set_width(ui_btnphoto, 180);
    lv_obj_set_height(ui_btnphoto, 80);

    lv_obj_set_x(ui_btnphoto, 144);
    lv_obj_set_y(ui_btnphoto, 145);

    lv_obj_set_align(ui_btnphoto, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_btnphoto, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_btnphoto, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_btnphoto, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_btnphoto, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_btnphoto, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_btnphoto, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_btnphoto, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_btnphoto, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_btnphoto, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_btnphoto, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btnphoto, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(ui_btnphoto, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(ui_btnphoto, lv_color_hex(0xFAD665), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_btnphoto, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_btnphoto, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_btnphoto, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_color(ui_btnphoto, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_opa(ui_btnphoto, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_labphoto

    ui_labphoto = lv_label_create(ui_btnphoto);

    lv_obj_set_width(ui_labphoto, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_labphoto, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_labphoto, 0);
    lv_obj_set_y(ui_labphoto, 0);

    lv_obj_set_align(ui_labphoto, LV_ALIGN_CENTER);

    // lv_label_set_text(ui_labphoto, photo_k);
    language_choose_add_label(ui_labphoto,photo_k,0);
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_labphoto, id,0,&select_font_media[id]);

    // ui_btntext

    ui_btntext = lv_btn_create(ui_mainpage);

    lv_obj_set_width(ui_btntext, 180);
    lv_obj_set_height(ui_btntext, 80);

    lv_obj_set_x(ui_btntext, 444);
    lv_obj_set_y(ui_btntext, 145);

    lv_obj_set_align(ui_btntext, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_btntext, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_btntext, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_btntext, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_btntext, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_btntext, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_btntext, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_btntext, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_btntext, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_btntext, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_btntext, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btntext, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(ui_btntext, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(ui_btntext, lv_color_hex(0xFAD665), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_btntext, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_btntext, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_btntext, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_color(ui_btntext, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_opa(ui_btntext, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_labtext

    ui_labtext = lv_label_create(ui_btntext);

    lv_obj_set_width(ui_labtext, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_labtext, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_labtext, 0);
    lv_obj_set_y(ui_labtext, 0);

    lv_obj_set_align(ui_labtext, LV_ALIGN_CENTER);

    // lv_label_set_text(ui_labtext, text_k);
    language_choose_add_label(ui_labtext,text_k,0);
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_labtext, id,0,&select_font_media[id]);

    // ui_usb_img

    ui_usb_img = lv_img_create(ui_mainpage);

    lv_obj_set_width(ui_usb_img, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_usb_img, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_usb_img, 100);
    lv_obj_set_y(ui_usb_img, 20);

    lv_img_set_src(ui_usb_img,&usb_icon);
    lv_obj_add_flag(ui_usb_img, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_usb_img, LV_OBJ_FLAG_SCROLLABLE);

    // ui_usb_lab

    ui_usb_lab = lv_label_create(ui_mainpage);

    lv_obj_set_width(ui_usb_lab, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_usb_lab, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_usb_lab, 233);
    lv_obj_set_y(ui_usb_lab, 38);

    lv_label_set_text(ui_usb_lab, "NO Device");

    lv_obj_set_style_text_color(ui_usb_lab, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_usb_lab, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_usb_lab, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

}
void ui_subpage_screen_init(void)
{

    select_font_media[0] = lv_font_montserrat_40;
    select_font_media[1] = font40_china;
    select_font_media[2] = lv_font_montserrat_40;
    // ui_subpage

    ui_subpage = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_subpage, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_subpage, ui_event_subpage, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(ui_subpage, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_subpage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_btnback

    ui_btnback = lv_btn_create(ui_subpage);

    lv_obj_set_width(ui_btnback, 180);
    lv_obj_set_height(ui_btnback, 80);

    lv_obj_set_x(ui_btnback, -451);
    lv_obj_set_y(ui_btnback, 145);

    lv_obj_set_align(ui_btnback, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_btnback, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_btnback, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_btnback, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_btnback, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_btnback, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_btnback, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_btnback, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btnback, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(ui_btnback, lv_color_hex(0xFAD665), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_btnback, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_btnback, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_color(ui_btnback, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_labback
    ui_labback = lv_label_create(ui_btnback);

    lv_obj_set_width(ui_labback, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_labback, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_labback, 0);
    lv_obj_set_y(ui_labback, 0);

    lv_obj_set_align(ui_labback, LV_ALIGN_CENTER);

    // lv_label_set_text(ui_labback, back_k);
    language_choose_add_label(ui_labback,back_k,0);
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_labback, id,0,&select_font_media[id]);

    // ui_btnrootdir

    ui_btnrootdir = lv_btn_create(ui_subpage);

    lv_obj_set_width(ui_btnrootdir, 180);
    lv_obj_set_height(ui_btnrootdir, 80);

    lv_obj_set_x(ui_btnrootdir, -151);
    lv_obj_set_y(ui_btnrootdir, 145);

    lv_obj_set_align(ui_btnrootdir, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_btnrootdir, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_btnrootdir, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_btnrootdir, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_btnrootdir, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_btnrootdir, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_btnrootdir, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_btnrootdir, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_btnrootdir, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_btnrootdir, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_btnrootdir, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btnrootdir, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(ui_btnrootdir, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(ui_btnrootdir, lv_color_hex(0xFAD665), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_btnrootdir, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_btnrootdir, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_btnrootdir, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_color(ui_btnrootdir, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_opa(ui_btnrootdir, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_labrootdir

    ui_labrootdir = lv_label_create(ui_btnrootdir);

    lv_obj_set_width(ui_labrootdir, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_labrootdir, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_labrootdir, 0);
    lv_obj_set_y(ui_labrootdir, 0);

    lv_obj_set_align(ui_labrootdir, LV_ALIGN_CENTER);

    // lv_label_set_text(ui_labrootdir, "C");
    language_choose_add_label(ui_labrootdir,rootdir_k,0);
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_labrootdir, id,0,&select_font_media[id]);
    // ui_title_on

    ui_title_on = lv_img_create(ui_subpage);
    lv_img_set_src(ui_title_on, &ui_img_thumbnail_upfolder_png);

    lv_obj_set_width(ui_title_on, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_title_on, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_title_on, 143);
    lv_obj_set_y(ui_title_on, 311);

    lv_obj_add_flag(ui_title_on, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_title_on, LV_OBJ_FLAG_SCROLLABLE);

    // ui_Image2

    ui_Image2 = lv_img_create(ui_subpage);
    lv_img_set_src(ui_Image2, &ui_img_patition_png);

    lv_obj_set_width(ui_Image2, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Image2, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_Image2, 444);
    lv_obj_set_y(ui_Image2, 313);

    lv_obj_add_flag(ui_Image2, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_Image2, LV_OBJ_FLAG_SCROLLABLE);

    // ui_Image3

    ui_Image3 = lv_img_create(ui_subpage);
    // lv_img_set_src(ui_Image3, &ui_img_idb_usb_detached_png);

    lv_obj_set_width(ui_Image3, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Image3, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_Image3, 53);
    lv_obj_set_y(ui_Image3, 23);

    lv_obj_add_flag(ui_Image3, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_Image3, LV_OBJ_FLAG_SCROLLABLE);

    // ui_titie_

    ui_titie_ = lv_label_create(ui_subpage);

    lv_obj_set_width(ui_titie_, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_titie_, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_titie_, 120);
    lv_obj_set_y(ui_titie_, 40);

    // lv_label_set_text(ui_titie_, "");

    lv_obj_set_style_text_color(ui_titie_, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_titie_, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_titie_, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);

    // language_choose_add_label(ui_titie_,movie_k,0);  //add user data 
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_titie_, id,0,&select_font_media[id]);  //set user data to label_text and set text font 


}
void ui_fspage_screen_init(void)
{

    select_font_mplist[0] = lv_font_montserrat_28;
    select_font_mplist[1] = SiYuanHeiTi_Light_3000_28_1b;
    select_font_mplist[2] = lv_font_montserrat_28;
    // ui_fspage

    ui_fspage = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_fspage, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_fspage, ui_event_fspage, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(ui_fspage, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fspage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_fsobj0

    ui_fsobj0 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj0, 150);
    lv_obj_set_height(ui_fsobj0, 150);

    lv_obj_set_x(ui_fsobj0, 200);
    lv_obj_set_y(ui_fsobj0, 110);

    lv_obj_clear_flag(ui_fsobj0, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj0, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj0, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj0, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj0, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj0, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj0, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg0

    ui_fsimg0 = lv_img_create(ui_fsobj0);
    lv_img_set_src(ui_fsimg0, &ui_img_thumbnail_upfolder_png);

    lv_obj_set_width(ui_fsimg0, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg0, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg0, 0);
    lv_obj_set_y(ui_fsimg0, 0);

    lv_obj_set_align(ui_fsimg0, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg0, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg0, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg0, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg0, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg0, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg0, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj1

    ui_fsobj1 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj1, 150);
    lv_obj_set_height(ui_fsobj1, 150);

    lv_obj_set_x(ui_fsobj1, 443);
    lv_obj_set_y(ui_fsobj1, 110);

    lv_obj_clear_flag(ui_fsobj1, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj1, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj1, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj1, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj1, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg1

    ui_fsimg1 = lv_img_create(ui_fsobj1);
    lv_img_set_src(ui_fsimg1, &ui_img_idb_usb_detached_png);

    lv_obj_set_width(ui_fsimg1, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg1, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg1, 0);
    lv_obj_set_y(ui_fsimg1, 0);

    lv_obj_set_align(ui_fsimg1, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg1, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg1, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg1, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg1, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg1, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg1, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj2

    ui_fsobj2 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj2, 150);
    lv_obj_set_height(ui_fsobj2, 150);

    lv_obj_set_x(ui_fsobj2, 686);
    lv_obj_set_y(ui_fsobj2, 110);

    lv_obj_clear_flag(ui_fsobj2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj2, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj2, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj2, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj2, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg2

    ui_fsimg2 = lv_img_create(ui_fsobj2);
    lv_img_set_src(ui_fsimg2, &ui_img_folder_png);

    lv_obj_set_width(ui_fsimg2, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg2, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg2, 0);
    lv_obj_set_y(ui_fsimg2, 0);

    lv_obj_set_align(ui_fsimg2, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg2, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg2, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg2, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg2, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg2, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj3

    ui_fsobj3 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj3, 150);
    lv_obj_set_height(ui_fsobj3, 150);

    lv_obj_set_x(ui_fsobj3, 929);
    lv_obj_set_y(ui_fsobj3, 110);

    lv_obj_clear_flag(ui_fsobj3, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj3, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj3, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj3, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj3, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj3, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg3

    ui_fsimg3 = lv_img_create(ui_fsobj3);

    lv_obj_set_width(ui_fsimg3, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg3, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg3, 0);
    lv_obj_set_y(ui_fsimg3, 0);

    lv_obj_set_align(ui_fsimg3, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg3, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg3, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg3, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg3, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg3, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg3, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj4

    ui_fsobj4 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj4, 150);
    lv_obj_set_height(ui_fsobj4, 150);

    lv_obj_set_x(ui_fsobj4, 200);
    lv_obj_set_y(ui_fsobj4, 305);

    lv_obj_clear_flag(ui_fsobj4, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj4, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj4, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj4, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj4, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj4, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg4

    ui_fsimg4 = lv_img_create(ui_fsobj4);

    lv_obj_set_width(ui_fsimg4, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg4, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg4, 0);
    lv_obj_set_y(ui_fsimg4, 0);

    lv_obj_set_align(ui_fsimg4, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg4, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg4, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg4, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg4, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg4, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg4, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj5

    ui_fsobj5 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj5, 150);
    lv_obj_set_height(ui_fsobj5, 150);

    lv_obj_set_x(ui_fsobj5, 443);
    lv_obj_set_y(ui_fsobj5, 305);

    lv_obj_clear_flag(ui_fsobj5, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj5, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj5, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj5, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj5, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj5, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg5

    ui_fsimg5 = lv_img_create(ui_fsobj5);

    lv_obj_set_width(ui_fsimg5, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg5, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg5, 0);
    lv_obj_set_y(ui_fsimg5, 0);

    lv_obj_set_align(ui_fsimg5, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg5, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg5, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg5, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg5, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg5, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg5, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj6

    ui_fsobj6 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj6, 150);
    lv_obj_set_height(ui_fsobj6, 150);

    lv_obj_set_x(ui_fsobj6, 686);
    lv_obj_set_y(ui_fsobj6, 305);

    lv_obj_clear_flag(ui_fsobj6, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj6, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj6, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj6, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj6, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj6, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj6, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj6, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg6

    ui_fsimg6 = lv_img_create(ui_fsobj6);

    lv_obj_set_width(ui_fsimg6, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg6, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg6, 0);
    lv_obj_set_y(ui_fsimg6, 0);

    lv_obj_set_align(ui_fsimg6, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg6, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg6, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg6, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg6, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg6, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg6, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj7

    ui_fsobj7 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj7, 150);
    lv_obj_set_height(ui_fsobj7, 150);

    lv_obj_set_x(ui_fsobj7, 929);
    lv_obj_set_y(ui_fsobj7, 305);

    lv_obj_clear_flag(ui_fsobj7, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj7, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj7, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj7, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj7, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj7, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj7, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj7, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg7

    ui_fsimg7 = lv_img_create(ui_fsobj7);

    lv_obj_set_width(ui_fsimg7, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg7, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg7, 0);
    lv_obj_set_y(ui_fsimg7, 0);

    lv_obj_set_align(ui_fsimg7, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg7, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg7, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg7, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg7, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg7, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg7, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj8

    ui_fsobj8 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj8, 150);
    lv_obj_set_height(ui_fsobj8, 150);

    lv_obj_set_x(ui_fsobj8, 200);
    lv_obj_set_y(ui_fsobj8, 500);

    lv_obj_clear_flag(ui_fsobj8, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj8, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj8, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj8, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj8, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj8, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj8, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj8, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg8

    ui_fsimg8 = lv_img_create(ui_fsobj8);

    lv_obj_set_width(ui_fsimg8, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg8, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg8, 0);
    lv_obj_set_y(ui_fsimg8, 0);

    lv_obj_set_align(ui_fsimg8, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg8, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg8, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg8, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg8, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg8, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg8, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj9

    ui_fsobj9 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj9, 150);
    lv_obj_set_height(ui_fsobj9, 150);

    lv_obj_set_x(ui_fsobj9, 443);
    lv_obj_set_y(ui_fsobj9, 500);

    lv_obj_clear_flag(ui_fsobj9, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj9, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj9, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj9, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj9, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj9, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj9, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj9, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg9

    ui_fsimg9 = lv_img_create(ui_fsobj9);

    lv_obj_set_width(ui_fsimg9, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg9, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg9, 0);
    lv_obj_set_y(ui_fsimg9, 0);

    lv_obj_set_align(ui_fsimg9, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg9, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg9, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg9, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg9, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg9, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg9, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj10

    ui_fsobj10 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj10, 150);
    lv_obj_set_height(ui_fsobj10, 150);

    lv_obj_set_x(ui_fsobj10, 686);
    lv_obj_set_y(ui_fsobj10, 500);

    lv_obj_clear_flag(ui_fsobj10, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj10, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj10, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj10, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj10, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj10, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj10, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj10, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg10

    ui_fsimg10 = lv_img_create(ui_fsobj10);

    lv_obj_set_width(ui_fsimg10, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg10, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg10, 0);
    lv_obj_set_y(ui_fsimg10, 0);

    lv_obj_set_align(ui_fsimg10, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg10, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg10, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg10, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg10, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg10, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg10, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsobj11

    ui_fsobj11 = lv_obj_create(ui_fspage);

    lv_obj_set_width(ui_fsobj11, 150);
    lv_obj_set_height(ui_fsobj11, 150);

    lv_obj_set_x(ui_fsobj11, 929);
    lv_obj_set_y(ui_fsobj11, 500);

    lv_obj_clear_flag(ui_fsobj11, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_fsobj11, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsobj11, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsobj11, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_fsobj11, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(ui_fsobj11, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(ui_fsobj11, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ui_fsobj11, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsimg11

    ui_fsimg11 = lv_img_create(ui_fsobj11);

    lv_obj_set_width(ui_fsimg11, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsimg11, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsimg11, 0);
    lv_obj_set_y(ui_fsimg11, 0);

    lv_obj_set_align(ui_fsimg11, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_fsimg11, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsimg11, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_outline_color(ui_fsimg11, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_fsimg11, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_fsimg11, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_fsimg11, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_fsbarimg

    ui_fsbarimg = lv_img_create(ui_fspage);
    lv_img_set_src(ui_fsbarimg, &ui_img_file_select_info_bar_png);

    lv_obj_set_width(ui_fsbarimg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsbarimg, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fsbarimg, 99);
    lv_obj_set_y(ui_fsbarimg, 65);

    lv_obj_add_flag(ui_fsbarimg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsbarimg, LV_OBJ_FLAG_SCROLLABLE);

    // ui_fscount

    ui_fscount = lv_label_create(ui_fspage);

    lv_obj_set_width(ui_fscount, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fscount, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fscount, 509);
    lv_obj_set_y(ui_fscount, -280);

    lv_obj_set_align(ui_fscount, LV_ALIGN_CENTER);

    lv_label_set_text(ui_fscount, "");

    lv_obj_set_style_text_color(ui_fscount, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_fscount, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_fscount, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_fspath

    ui_fspath = lv_label_create(ui_fspage);

    lv_obj_set_width(ui_fspath, 800);
    lv_obj_set_height(ui_fspath, 40);

    lv_obj_set_x(ui_fspath, -125);
    lv_obj_set_y(ui_fspath, -273);

    lv_obj_set_align(ui_fspath, LV_ALIGN_CENTER);

    lv_label_set_text(ui_fspath, "/sda");
    lv_label_set_long_mode(ui_fspath,LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_obj_set_style_text_color(ui_fspath, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_fspath, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_fspath, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_fspath, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_fstitle

    ui_fstitle = lv_label_create(ui_fspage);

    lv_obj_set_width(ui_fstitle, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fstitle, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_fstitle, 594);
    lv_obj_set_y(ui_fstitle, 12);

    lv_label_set_text(ui_fstitle, "");

    lv_obj_set_style_text_color(ui_fstitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_fstitle, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_text_font(ui_fstitle, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);

    // language_choose_add_label(ui_fstitle,text_k,0);
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_fstitle, id,0,&select_font_mplist[id]);
}
extern void media_playbar_control(void *arg1, void *arg2);

void ui_ctrl_bar_screen_init(void)
{

    select_font_mplist[0] = lv_font_montserrat_28;
    select_font_mplist[1] = SiYuanHeiTi_Light_3000_28_1b;
    select_font_mplist[2] = lv_font_montserrat_28;

    // ui_ctrl_bar

    ui_ctrl_bar = lv_obj_create(NULL);

	screen_entry_t ctrl_bar_entry;
    ctrl_bar_entry.screen = ui_ctrl_bar;
    ctrl_bar_entry.control = media_playbar_control;
    api_screen_regist_ctrl_handle(&ctrl_bar_entry);
	
	lv_obj_clear_flag(ui_ctrl_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_ctrl_bar, ui_event_ctrl_bar, LV_EVENT_ALL, NULL);

    // ui_play_bar

    ui_play_bar = lv_obj_create(ui_ctrl_bar);

    lv_obj_set_width(ui_play_bar, 1280);
    lv_obj_set_height(ui_play_bar, 220);

    lv_obj_set_x(ui_play_bar, 0);
    lv_obj_set_y(ui_play_bar, 0);

    lv_obj_set_align(ui_play_bar, LV_ALIGN_BOTTOM_LEFT);

    lv_obj_clear_flag(ui_play_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_play_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_play_bar, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_play_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_playbar

    ui_playbar = lv_bar_create(ui_play_bar);
    lv_bar_set_range(ui_playbar, 0, 100);
    lv_bar_set_value(ui_playbar, 25, LV_ANIM_OFF);

    lv_obj_set_width(ui_playbar, 760);
    lv_obj_set_height(ui_playbar, 25);

    lv_obj_set_x(ui_playbar, 150);
    lv_obj_set_y(ui_playbar, -81);

    lv_obj_set_align(ui_playbar, LV_ALIGN_LEFT_MID);

    lv_obj_set_style_radius(ui_playbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_playbar, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_playbar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_playbar, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_playbar, lv_color_hex(0xFFFF00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_playbar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // ui_winbar

    ui_winbar = lv_obj_create(ui_play_bar);

    lv_obj_set_width(ui_winbar, 945);
    lv_obj_set_height(ui_winbar, 120);

    lv_obj_set_x(ui_winbar, 0);
    lv_obj_set_y(ui_winbar, 0);

    lv_obj_set_align(ui_winbar, LV_ALIGN_CENTER);

    lv_obj_clear_flag(ui_winbar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(ui_winbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_winbar, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_winbar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_winbar, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_winbar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_barbtn0

    ui_barbtn0 = lv_btn_create(ui_winbar);

    lv_obj_set_width(ui_barbtn0, 120);
    lv_obj_set_height(ui_barbtn0, 110);

    lv_obj_set_x(ui_barbtn0, 0);
    lv_obj_set_y(ui_barbtn0, 0);

    lv_obj_set_align(ui_barbtn0, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_barbtn0, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_barbtn0, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_barbtn0, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_barbtn0, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(ui_barbtn0, lv_color_hex(0x847A7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_barbtn0, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui_barbtn0, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(ui_barbtn0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_barbtn0, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_barbtn0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(ui_barbtn0, lv_color_hex(0x3260E9), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_barbtn0, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_barbtn0, 5, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_barbtn0, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_barlab

    ui_barlab = lv_label_create(ui_barbtn0);

    lv_obj_set_width(ui_barlab, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_barlab, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_barlab, 0);
    lv_obj_set_y(ui_barlab, 10);

    lv_obj_set_align(ui_barlab, LV_ALIGN_BOTTOM_MID);

    // lv_label_set_text(ui_barlab, "Play");

    lv_obj_set_style_text_font(ui_barlab, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    language_choose_add_label(ui_barlab,bar_play_k,0);
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_barlab, id,0,&select_font_mplist[id]);
    // ui_barbtn1

    ui_barbtn1 = lv_btn_create(ui_winbar);

    lv_obj_set_width(ui_barbtn1, 120);
    lv_obj_set_height(ui_barbtn1, 110);

    lv_obj_set_x(ui_barbtn1, 130);
    lv_obj_set_y(ui_barbtn1, 0);

    lv_obj_set_align(ui_barbtn1, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_barbtn1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_barbtn1, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_barbtn1, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_barbtn1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_bg_color(ui_barbtn1, lv_color_hex(0x323232), LV_STATE_DISABLED);
    // lv_obj_set_style_bg_opa(ui_barbtn1, 255, LV_STATE_DISABLED);
    // lv_obj_set_style_text_color(ui_barbtn1,lv_color_hex(0x646464), LV_STATE_DISABLED);
    lv_obj_set_style_outline_color(ui_barbtn1, lv_color_hex(0x847A7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_barbtn1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui_barbtn1, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(ui_barbtn1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_barbtn1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_barbtn1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_barbtn1, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(ui_barbtn1, lv_color_hex(0x3260E9), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_barbtn1, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_barbtn1, 5, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_barbtn1, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_barlab1

    ui_barlab1 = lv_label_create(ui_barbtn1);

    lv_obj_set_width(ui_barlab1, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_barlab1, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_barlab1, 0);
    lv_obj_set_y(ui_barlab1, 10);

    lv_obj_set_align(ui_barlab1, LV_ALIGN_BOTTOM_MID);

    // lv_label_set_text(ui_barlab1, "FB");

    lv_obj_set_style_text_align(ui_barlab1, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_barlab1, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    language_choose_add_label(ui_barlab1,bar_fb_k,0);
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_barlab1, id,0,&select_font_mplist[id]);

    // ui_barbtn2

    ui_barbtn2 = lv_btn_create(ui_winbar);

    lv_obj_set_width(ui_barbtn2, 120);
    lv_obj_set_height(ui_barbtn2, 110);

    lv_obj_set_x(ui_barbtn2, 260);
    lv_obj_set_y(ui_barbtn2, 0);

    lv_obj_set_align(ui_barbtn2, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_barbtn2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_barbtn2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_barbtn2, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_barbtn2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_bg_color(ui_barbtn2, lv_color_hex(0x323232),  LV_STATE_DISABLED);
    // lv_obj_set_style_bg_opa(ui_barbtn2, 255, LV_STATE_DISABLED);
    // lv_obj_set_style_text_color(ui_barbtn2,lv_color_hex(0x646464), LV_STATE_DISABLED);
    lv_obj_set_style_outline_color(ui_barbtn2, lv_color_hex(0x847A7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_barbtn2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui_barbtn2, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(ui_barbtn2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_barbtn2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_barbtn2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(ui_barbtn2, lv_color_hex(0x3260E9), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_barbtn2, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_barbtn2, 5, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_barbtn2, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_barlab2

    ui_barlab2 = lv_label_create(ui_barbtn2);

    lv_obj_set_width(ui_barlab2, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_barlab2, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_barlab2, 0);
    lv_obj_set_y(ui_barlab2, 10);

    lv_obj_set_align(ui_barlab2, LV_ALIGN_BOTTOM_MID);

    // lv_label_set_text(ui_barlab2, "FF");

    lv_obj_set_style_text_font(ui_barlab2, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    language_choose_add_label(ui_barlab2,bar_ff_k,0);
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_barlab2, id,0,&select_font_mplist[id]);

    // ui_barbtn3

    ui_barbtn3 = lv_btn_create(ui_winbar);

    lv_obj_set_width(ui_barbtn3, 120);
    lv_obj_set_height(ui_barbtn3, 110);

    lv_obj_set_x(ui_barbtn3, 390);
    lv_obj_set_y(ui_barbtn3, 0);

    lv_obj_set_align(ui_barbtn3, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_barbtn3, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_barbtn3, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_barbtn3, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_barbtn3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_barbtn3, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_set_style_outline_color(ui_barbtn3, lv_color_hex(0x847A7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_barbtn3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui_barbtn3, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(ui_barbtn3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_barbtn3, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_barbtn3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(ui_barbtn3, lv_color_hex(0x3260E9), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_barbtn3, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_barbtn3, 5, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_barbtn3, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_barlab3

    ui_barlab3 = lv_label_create(ui_barbtn3);

    lv_obj_set_width(ui_barlab3, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_barlab3, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_barlab3, 0);
    lv_obj_set_y(ui_barlab3, 10);

    lv_obj_set_align(ui_barlab3, LV_ALIGN_BOTTOM_MID);

    // lv_label_set_text(ui_barlab3, "Prev");

    lv_obj_set_style_text_font(ui_barlab3, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
   
    language_choose_add_label(ui_barlab3,bar_prev_k,0);
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_barlab3, id,0,&select_font_mplist[id]);

    // ui_barbtn4

    ui_barbtn4 = lv_btn_create(ui_winbar);

    lv_obj_set_width(ui_barbtn4, 120);
    lv_obj_set_height(ui_barbtn4, 110);

    lv_obj_set_x(ui_barbtn4, 520);
    lv_obj_set_y(ui_barbtn4, 0);

    lv_obj_set_align(ui_barbtn4, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_barbtn4, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_barbtn4, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_barbtn4, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_barbtn4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_barbtn4, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_set_style_outline_color(ui_barbtn4, lv_color_hex(0x847A7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_barbtn4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui_barbtn4, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(ui_barbtn4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_barbtn4, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_barbtn4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(ui_barbtn4, lv_color_hex(0x3260E9), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_barbtn4, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_barbtn4, 5, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_barbtn4, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_barlab4

    ui_barlab4 = lv_label_create(ui_barbtn4);

    lv_obj_set_width(ui_barlab4, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_barlab4, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_barlab4, 0);
    lv_obj_set_y(ui_barlab4, 10);

    lv_obj_set_align(ui_barlab4, LV_ALIGN_BOTTOM_MID);

    // lv_label_set_text(ui_barlab4, "Next");

    lv_obj_set_style_text_font(ui_barlab4, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    language_choose_add_label(ui_barlab4,bar_next_k,0);
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_barlab4, id,0,&select_font_mplist[id]);

    // ui_barbtn5

    ui_barbtn5 = lv_btn_create(ui_winbar);

    lv_obj_set_width(ui_barbtn5, 120);
    lv_obj_set_height(ui_barbtn5, 110);

    lv_obj_set_x(ui_barbtn5, 650);
    lv_obj_set_y(ui_barbtn5, 0);

    lv_obj_set_align(ui_barbtn5, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_barbtn5, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_barbtn5, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_barbtn5, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_barbtn5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_barbtn5, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_set_style_outline_color(ui_barbtn5, lv_color_hex(0x847A7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_barbtn5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui_barbtn5, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(ui_barbtn5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_barbtn5, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_barbtn5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(ui_barbtn5, lv_color_hex(0x3260E9), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_barbtn5, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_barbtn5, 5, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_barbtn5, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_barlab5

    ui_barlab5 = lv_label_create(ui_barbtn5);

    lv_obj_set_width(ui_barlab5, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_barlab5, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_barlab5, 0);
    lv_obj_set_y(ui_barlab5, 10);

    lv_obj_set_align(ui_barlab5, LV_ALIGN_BOTTOM_MID);

    // lv_label_set_text(ui_barlab5, "Stop");

    lv_obj_set_style_text_font(ui_barlab5, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    language_choose_add_label(ui_barlab5,bar_pause_k,0);
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_barlab5, id,0,&select_font_mplist[id]);

    // ui_barbtn6

    ui_barbtn6 = lv_btn_create(ui_winbar);

    lv_obj_set_width(ui_barbtn6, 120);
    lv_obj_set_height(ui_barbtn6, 110);

    lv_obj_set_x(ui_barbtn6, 780);
    lv_obj_set_y(ui_barbtn6, 0);

    lv_obj_set_align(ui_barbtn6, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_barbtn6, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_barbtn6, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(ui_barbtn6, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_barbtn6, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_barbtn6, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_set_style_outline_color(ui_barbtn6, lv_color_hex(0x847A7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(ui_barbtn6, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui_barbtn6, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(ui_barbtn6, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_barbtn6, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_barbtn6, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(ui_barbtn6, lv_color_hex(0x3260E9), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_barbtn6, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_barbtn6, 5, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_barbtn6, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // ui_barlab6

    ui_barlab6 = lv_label_create(ui_barbtn6);

    lv_obj_set_width(ui_barlab6, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_barlab6, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_barlab6, 0);
    lv_obj_set_y(ui_barlab6, 10);

    lv_obj_set_align(ui_barlab6, LV_ALIGN_BOTTOM_MID);

    // lv_label_set_text(ui_barlab6, "Round");

    lv_obj_set_style_text_font(ui_barlab6, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    language_choose_add_label(ui_barlab6,bar_round_k,0);
    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_barlab6, id,0,&select_font_mplist[id]);
    // ui_cur_time

    ui_cur_time = lv_label_create(ui_play_bar);

    lv_obj_set_width(ui_cur_time, 120);
    lv_obj_set_height(ui_cur_time, 30);

    lv_obj_set_x(ui_cur_time, 361);
    lv_obj_set_y(ui_cur_time, -82);

    lv_obj_set_align(ui_cur_time, LV_ALIGN_CENTER);

    lv_label_set_text(ui_cur_time, "");

    lv_obj_set_style_text_color(ui_cur_time, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_cur_time, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_cur_time, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_total_time

    ui_total_time = lv_label_create(ui_play_bar);

    lv_obj_set_width(ui_total_time, 120);
    lv_obj_set_height(ui_total_time, 30);

    lv_obj_set_x(ui_total_time, 508);
    lv_obj_set_y(ui_total_time, -82);

    lv_obj_set_align(ui_total_time, LV_ALIGN_CENTER);

    lv_label_set_text(ui_total_time, "");

    lv_obj_set_style_text_color(ui_total_time, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_total_time, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_total_time, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_str

    ui_str = lv_label_create(ui_play_bar);

    lv_obj_set_width(ui_str, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_str, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_str, 432);
    lv_obj_set_y(ui_str, -82);

    lv_obj_set_align(ui_str, LV_ALIGN_CENTER);

    lv_label_set_text(ui_str, "/");

    lv_obj_set_style_text_color(ui_str, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_str, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_str, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_playname

    ui_playname = lv_label_create(ui_play_bar);

    lv_obj_set_width(ui_playname, 511);
    lv_obj_set_height(ui_playname,40);
    lv_obj_set_x(ui_playname, -195);
    lv_obj_set_y(ui_playname, 84);

    lv_obj_set_align(ui_playname, LV_ALIGN_CENTER);

    lv_label_set_long_mode(ui_playname, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(ui_playname, "playname");

    lv_obj_set_style_text_color(ui_playname, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_playname, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_playname, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_text_font(ui_playname, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_playname, id,0,&select_font_mplist[id]);
    // ui_playstate

    ui_playstate = lv_label_create(ui_play_bar);

    lv_obj_set_width(ui_playstate, 126);
    lv_obj_set_height(ui_playstate, 30);
    lv_obj_set_x(ui_playstate, 342);
    lv_obj_set_y(ui_playstate, 87);

    lv_obj_set_align(ui_playstate, LV_ALIGN_CENTER);

    // lv_label_set_text(ui_playstate, "playstate");

    lv_obj_set_style_text_color(ui_playstate, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_playstate, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_playstate, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_playstate, id,0,&select_font_mplist[id]);

    // ui_pervimg

    ui_pervimg = lv_img_create(ui_play_bar);

    lv_obj_set_width(ui_pervimg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_pervimg, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_pervimg, 0);
    lv_obj_set_y(ui_pervimg, 0);

    lv_obj_set_align(ui_pervimg, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_pervimg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_pervimg, LV_OBJ_FLAG_SCROLLABLE);

    // ui_nextimg

    ui_nextimg = lv_img_create(ui_play_bar);

    lv_obj_set_width(ui_nextimg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_nextimg, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_nextimg, 0);
    lv_obj_set_y(ui_nextimg, 0);

    lv_obj_set_align(ui_nextimg, LV_ALIGN_RIGHT_MID);

    lv_obj_add_flag(ui_nextimg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_nextimg, LV_OBJ_FLAG_SCROLLABLE);

    // ui_state

    ui_state = lv_label_create(ui_ctrl_bar);

    // lv_obj_set_width(ui_state, LV_SIZE_CONTENT);
    // lv_obj_set_height(ui_state, LV_SIZE_CONTENT);

    // lv_obj_set_x(ui_state, -400);
    // lv_obj_set_y(ui_state, 80);

    // lv_obj_set_align(ui_state, LV_ALIGN_CENTER);

    lv_label_set_text(ui_state, "");

    // lv_obj_set_style_text_color(ui_state, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_text_opa(ui_state, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_text_font(ui_state, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    // id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    // set_label_text_with_font(ui_state, id,0,&select_font_mplist[id]);

    ui_speed = lv_label_create(ui_ctrl_bar);

    lv_obj_set_width(ui_speed, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_speed, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_speed, 453);
    lv_obj_set_y(ui_speed, 335);

    lv_obj_set_align(ui_speed, LV_ALIGN_CENTER);

    lv_label_set_text(ui_speed, "");

    lv_obj_set_style_text_color(ui_speed, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_speed, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_speed, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_img_src(ui_barbtn0,&Hint_Play,0);
    lv_obj_set_style_bg_img_src(ui_barbtn1,&Hint_FB,0);
    lv_obj_set_style_bg_img_src(ui_barbtn2,&Hint_FF,0);
    lv_obj_set_style_bg_img_src(ui_barbtn3,&Hint_Prev,0);
    lv_obj_set_style_bg_img_src(ui_barbtn4,&Hint_Next,0);
    lv_obj_set_style_bg_img_src(ui_barbtn5,&Hint_Pause,0);
    lv_obj_set_style_bg_img_src(ui_barbtn6,&Hint_Rotate,0);
   // ui_error_tip

    // ui_error_tip = lv_obj_create(ui_ctrl_bar);

    // lv_obj_set_width(ui_error_tip, 350);
    // lv_obj_set_height(ui_error_tip, 200);

    // lv_obj_set_x(ui_error_tip, 0);
    // lv_obj_set_y(ui_error_tip, -100);

    // lv_obj_set_align(ui_error_tip, LV_ALIGN_CENTER);

    // lv_obj_clear_flag(ui_error_tip, LV_OBJ_FLAG_SCROLLABLE);

    // lv_obj_set_style_radius(ui_error_tip, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_bg_color(ui_error_tip, lv_color_hex(0x4D72E0), LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_bg_opa(ui_error_tip, 100, LV_PART_MAIN | LV_STATE_DEFAULT);

    // // ui_tip_img

    // ui_tip_img = lv_img_create(ui_error_tip);

    // lv_obj_set_width(ui_tip_img, LV_SIZE_CONTENT);
    // lv_obj_set_height(ui_tip_img, LV_SIZE_CONTENT);

    // lv_obj_set_x(ui_tip_img, 0);
    // lv_obj_set_y(ui_tip_img, 0);

    // lv_img_set_src(ui_tip_img,&IDB_Icon_unsupported);
    // lv_obj_set_align(ui_tip_img, LV_ALIGN_CENTER);

    // lv_obj_add_flag(ui_tip_img, LV_OBJ_FLAG_ADV_HITTEST);
    // lv_obj_clear_flag(ui_tip_img, LV_OBJ_FLAG_SCROLLABLE);

    // // ui_tip_lab

    // ui_tip_lab = lv_label_create(ui_error_tip);

    // lv_obj_set_width(ui_tip_lab, LV_SIZE_CONTENT);
    // lv_obj_set_height(ui_tip_lab, LV_SIZE_CONTENT);

    // lv_obj_set_x(ui_tip_lab, 0);
    // lv_obj_set_y(ui_tip_lab, 10);

    // lv_obj_set_align(ui_tip_lab, LV_ALIGN_BOTTOM_MID);

    // lv_label_set_text(ui_tip_lab, "Unsupport Media Type");

    // lv_obj_set_style_text_color(ui_tip_lab, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_text_opa(ui_tip_lab, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_text_font(ui_tip_lab, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    // id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    // set_label_text_with_font(ui_tip_lab, id,0,&select_font_mplist[id]);
    // lv_obj_add_flag(ui_error_tip,LV_OBJ_FLAG_HIDDEN);

}
void ui_ebook_screen_init(void)
{
	ui_ebook_txt = lv_obj_create(NULL);
	lv_obj_clear_flag(ui_ebook_txt, LV_OBJ_FLAG_SCROLLABLE);
	//lv_obj_set_width(ui_ebook_txt, LV_SIZE_CONTENT);
    //lv_obj_set_height(ui_ebook_txt, LV_SIZE_CONTENT);
    lv_obj_add_event_cb(ui_ebook_txt, ui_event_ebook, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(ui_ebook_txt, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ebook_txt, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

	ui_ebook_label= lv_label_create(ui_ebook_txt);
	lv_label_set_long_mode(ui_ebook_label, LV_LABEL_LONG_WRAP); 
	lv_label_set_recolor(ui_ebook_label, true); 
	//lv_label_set_text(ui_ebook_label, "#0000ff Re-color# #ff00ff words# #ff0000 of a# label, align the lines to the center "
    //                          "and wrap long text automatically.akjsdkjasdaksjdnasdnknkjasdaskjdnwe"
    //                  "and wrap long text automatically.akjsdkjasdaksjdnasdnknkjasdaskjdnwe啊速度啦拉萨扩大吗撒了腹肌拉伤开发的阿里山扩大那可是九零年代");
    lv_obj_set_width(ui_ebook_label, 1230);
    lv_obj_set_height(ui_ebook_label, 680);

    lv_obj_set_x(ui_ebook_label, 20);
    lv_obj_set_y(ui_ebook_label, 10);

    lv_obj_set_align(ui_ebook_label, LV_ALIGN_BOTTOM_MID);
	lv_obj_set_style_text_color(ui_ebook_label,lv_color_hex(0xffffff),0);
    //lv_obj_set_style_text_font(ui_ebook_label, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
	//lv_obj_set_style_text_font(ui_ebook_label,&lv_font_montserrat_28,0);
	lv_obj_set_style_text_font(ui_ebook_label,&SiYuanHeiTi_Light_3000_28_1b,0);//suport chinese SiYuanHeiTi_Light_3000_28_1b
	ui_ebook_label_page= lv_label_create(ui_ebook_txt);
	lv_label_set_long_mode(ui_ebook_label_page, LV_LABEL_LONG_WRAP); 
	lv_label_set_recolor(ui_ebook_label_page, true); 

	lv_obj_set_width(ui_ebook_label_page, 200);
    lv_obj_set_height(ui_ebook_label_page, 40);
	
	lv_obj_set_x(ui_ebook_label_page, 1080);
	lv_obj_set_y(ui_ebook_label_page, 680);

	//lv_obj_set_align(ui_ebook_label_page, LV_ALIGN_BOTTOM_RIGHT);
	lv_obj_set_style_text_color(ui_ebook_label_page,lv_color_hex(0xffffff),0);

	lv_obj_set_style_text_font(ui_ebook_label_page, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_init(void)
{
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    ui_mainpage_screen_init();
    ui_subpage_screen_init();
    ui_fspage_screen_init();
    ui_ctrl_bar_screen_init();
    lv_disp_load_scr(ui_mainpage);
}
