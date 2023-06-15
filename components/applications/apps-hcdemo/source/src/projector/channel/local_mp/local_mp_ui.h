// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.0.5
// LVGL VERSION: 8.2
// PROJECT: dmx_ui_Project

#ifndef _LOCAL_MP_UI_H
#define _LOCAL_MP_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#define NEW_FUNC
extern lv_obj_t * ui_mainpage;
extern lv_obj_t * ui_imgmovie;
extern lv_obj_t * ui_imgmusic;
extern lv_obj_t * ui_imgphoto;
extern lv_obj_t * ui_imgtext;
extern lv_obj_t * ui_btnmovie;
extern lv_obj_t * ui_labmovie;
extern lv_obj_t * ui_btnmusic;
extern lv_obj_t * ui_labmusic;
extern lv_obj_t * ui_btnphoto;
extern lv_obj_t * ui_labphoto;
extern lv_obj_t * ui_btntext;
extern lv_obj_t * ui_labtext;
extern lv_obj_t * ui_usb_img;
extern lv_obj_t * ui_usb_lab;
extern lv_obj_t * ui_subpage;
extern lv_obj_t * ui_btnback;
extern lv_obj_t * ui_labback;
extern lv_obj_t * ui_btnrootdir;
extern lv_obj_t * ui_labrootdir;
extern lv_obj_t * ui_title_on;
extern lv_obj_t * ui_Image2;
extern lv_obj_t * ui_Image3;
extern lv_obj_t * ui_titie_;
extern lv_obj_t * ui_fspage;
extern lv_obj_t * ui_fsobj0;
extern lv_obj_t * ui_fsimg0;
extern lv_obj_t * ui_fsobj1;
extern lv_obj_t * ui_fsimg1;
extern lv_obj_t * ui_fsobj2;
extern lv_obj_t * ui_fsimg2;
extern lv_obj_t * ui_fsobj3;
extern lv_obj_t * ui_fsimg3;
extern lv_obj_t * ui_fsobj4;
extern lv_obj_t * ui_fsimg4;
extern lv_obj_t * ui_fsobj5;
extern lv_obj_t * ui_fsimg5;
extern lv_obj_t * ui_fsobj6;
extern lv_obj_t * ui_fsimg6;
extern lv_obj_t * ui_fsobj7;
extern lv_obj_t * ui_fsimg7;
extern lv_obj_t * ui_fsobj8;
extern lv_obj_t * ui_fsimg8;
extern lv_obj_t * ui_fsobj9;
extern lv_obj_t * ui_fsimg9;
extern lv_obj_t * ui_fsobj10;
extern lv_obj_t * ui_fsimg10;
extern lv_obj_t * ui_fsobj11;
extern lv_obj_t * ui_fsimg11;
extern lv_obj_t * ui_fsbarimg;
extern lv_obj_t * ui_fscount;
extern lv_obj_t * ui_fspath;
extern lv_obj_t * ui_fstitle;
extern lv_obj_t * ui_ctrl_bar;
extern lv_obj_t * ui_play_bar;
extern lv_obj_t * ui_playbar;
extern lv_obj_t * ui_winbar;
extern lv_obj_t * ui_barbtn0;
extern lv_obj_t * ui_barlab;
extern lv_obj_t * ui_barbtn1;
extern lv_obj_t * ui_barlab1;
extern lv_obj_t * ui_barbtn2;
extern lv_obj_t * ui_barlab2;
extern lv_obj_t * ui_barbtn3;
extern lv_obj_t * ui_barlab3;
extern lv_obj_t * ui_barbtn4;
extern lv_obj_t * ui_barlab4;
extern lv_obj_t * ui_barbtn5;
extern lv_obj_t * ui_barlab5;
extern lv_obj_t * ui_barbtn6;
extern lv_obj_t * ui_barlab6;
extern lv_obj_t * ui_cur_time;
extern lv_obj_t * ui_total_time;
extern lv_obj_t * ui_str;
extern lv_obj_t * ui_playname;
extern lv_obj_t * ui_playstate;
extern lv_obj_t * ui_pervimg;
extern lv_obj_t * ui_nextimg;
extern lv_obj_t * ui_state;
extern lv_obj_t * ui_speed;
extern lv_obj_t * ui_error_tip;
extern lv_obj_t * ui_tip_img;
extern lv_obj_t * ui_tip_lab;
extern lv_obj_t * ui_win_zoom;
extern lv_obj_t * ui_win_name;
extern lv_obj_t * ui_name;
extern lv_obj_t * ui_file_info;
extern lv_obj_t * ui_info_s;
extern lv_obj_t * ui_info_s1;
extern lv_obj_t * ui_info_t_;
extern lv_obj_t * ui_info_t1;
extern lv_group_t * main_group;
extern lv_group_t * sub_group;
extern lv_group_t * fs_group;
extern lv_group_t * play_bar_group;
extern lv_font_t select_font_media[3];
extern lv_font_t select_font_mplist[3];

extern char* movie_k;
extern char* music_k ;
extern char* photo_k ;
extern char* text_k ;
extern char* back_k ;
extern char* rootdir_k;
extern char* bar_play_k ;
extern char* bar_pause_k ;
extern char* bar_ff_k ;
extern char* bar_fb_k ;
extern char* bar_next_k ;
extern char* bar_prev_k ;
extern char* bar_round_k ;
LV_IMG_DECLARE(ui_img_dmp_media_type_movie_png);    // assets\dmp_media_type_movie.png
LV_IMG_DECLARE(ui_img_dmp_media_type_music_png);    // assets\dmp_media_type_music.png
LV_IMG_DECLARE(ui_img_dmp_media_type_photo_png);    // assets\dmp_media_type_photo.png
LV_IMG_DECLARE(ui_img_dmp_media_type_txt_png);    // assets\dmp_media_type_txt.png
LV_IMG_DECLARE(usb_icon);
LV_IMG_DECLARE(ui_img_thumbnail_upfolder_png);    // assets\Thumbnail_Upfolder.png
LV_IMG_DECLARE(ui_img_patition_png);    // assets\Patition.png
LV_IMG_DECLARE(ui_img_folder_png);
LV_IMG_DECLARE(black_bg);
LV_IMG_DECLARE(ui_img_idb_usb_attached_png);    // assets\IDB_USB_Attached.png
LV_IMG_DECLARE(ui_img_idb_usb_detached_png);    // assets\IDB_USB_Detached.png
LV_IMG_DECLARE(ui_img_file_select_info_bar_png);    // assets\File_Select_Info_Bar.png
LV_IMG_DECLARE(IDB_Movie_Arrow_R);
LV_IMG_DECLARE(Hint_FB);
LV_IMG_DECLARE(Hint_FF);
LV_IMG_DECLARE(Hint_Next);
LV_IMG_DECLARE(Hint_Pause);
LV_IMG_DECLARE(Hint_Play);
LV_IMG_DECLARE(Hint_Playlist);
LV_IMG_DECLARE(Hint_Prev);
LV_IMG_DECLARE(Hint_RepeatAB);
LV_IMG_DECLARE(Hint_Rotate);
LV_IMG_DECLARE(IDB_FileSelect_movie);
LV_IMG_DECLARE(IDB_FileSelect_music);
LV_IMG_DECLARE(IDB_FileSelect_photo);
LV_IMG_DECLARE(IDB_FileSelect_text);
LV_IMG_DECLARE(IDB_Icon_unsupported);
LV_FONT_DECLARE(font32_china);
LV_FONT_DECLARE(myFont1);
LV_FONT_DECLARE(font40_china);
void ui_init(void);
void ui_mainpage_screen_init(void);
void ui_subpage_screen_init(void);
void ui_fspage_screen_init(void);
void ui_ctrl_bar_screen_init(void);
void ui_ebook_screen_init(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
