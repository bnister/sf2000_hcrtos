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
#include "app_config.h"
extern lv_obj_t * ui_mainpage;

extern lv_obj_t * ui_subpage;

extern lv_obj_t * ui_fspage;

// for ctrl_bar
extern lv_obj_t * ui_ctrl_bar;
extern lv_obj_t * ui_play_bar;
extern lv_obj_t * ui_playbar;
extern lv_obj_t * ui_winbar;
extern lv_obj_t * ui_playname;
extern lv_obj_t * ui_playstate;
extern lv_obj_t * ui_speed;

extern lv_obj_t * ui_ebook_txt;
extern lv_obj_t * ui_ebook_label;
extern lv_obj_t * ui_ebook_label_page;

extern lv_obj_t *subtitles_obj;
extern lv_obj_t *subtitles_obj_pic;

extern lv_group_t * main_group;
extern lv_group_t * sub_group;
extern lv_group_t * fs_group;
extern lv_group_t * play_bar_group;
extern lv_font_t select_font_media[3];
extern lv_font_t select_font_mplist[3];

// for mainpage
extern lv_obj_t * mp_statebar;
extern lv_obj_t * main_cont1;
#define CONT_WINDOW_CNT  12
extern lv_obj_t * obj_item[CONT_WINDOW_CNT];
extern lv_obj_t * obj_labelitem[CONT_WINDOW_CNT];

//for ctrlbar
#define MAX_BARBTN_ITEM     15
extern lv_obj_t* ctrlbarbtn[MAX_BARBTN_ITEM];

LV_IMG_DECLARE(IDB_Icon_Home_Movie);    
LV_IMG_DECLARE(IDB_Icon_Home_Music);    
LV_IMG_DECLARE(IDB_Icon_Home_Photo);   
LV_IMG_DECLARE(IDB_Icon_Home_Txt);    
LV_IMG_DECLARE(usb_icon);
LV_IMG_DECLARE(Thumbnail_Upfolder);    
LV_IMG_DECLARE(IDB_Patition_Icon);    
LV_IMG_DECLARE(cultraview_folder);
LV_IMG_DECLARE(black_bg);
LV_IMG_DECLARE(IDB_File_Select_Info_Bar);    
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
LV_IMG_DECLARE(Hint_Info);
LV_IMG_DECLARE(IDB_Hint_ClosePanel);
LV_IMG_DECLARE(IDB_Hint_Zoom_Move);
LV_IMG_DECLARE(IDB_Hint_Music_Off);
LV_IMG_DECLARE(IDB_Hint_Music_On);
LV_IMG_DECLARE(IDB_Hint_mute);
LV_IMG_DECLARE(Hint_Stop);
LV_IMG_DECLARE(Hint_SF);
LV_IMG_DECLARE(Hint_Step);
LV_IMG_DECLARE(Hint_Setup);
LV_IMG_DECLARE(Hint_ZoomIn);
LV_IMG_DECLARE(Hint_ZoomOut);
LV_IMG_DECLARE(IDB_Hint_Rotate_Left);
LV_IMG_DECLARE(IDB_Hint_Rotate_Right);
LV_IMG_DECLARE(IDB_Hint_CaptureLogo);
LV_IMG_DECLARE(IDB_FileSelect_movie);
LV_IMG_DECLARE(IDB_FileSelect_music);
LV_IMG_DECLARE(IDB_FileSelect_photo);
LV_IMG_DECLARE(IDB_FileSelect_text);
LV_IMG_DECLARE(IDB_Icon_unsupported);

#ifdef  LVGL_RESOLUTION_240P_SUPPORT
// it need to slightly change the UI display code for the different resolutions
    #define STYLE_OUTLINE_W   1
    #define MAINPAGE_BTNPCT_W 175
    #define SUBPAGE_BTNPCT_W 150
    #define IMGICON_Y_OFS -5
    //for fslist page
    #define CONT_W 45
    #define CONT_H 45
    #define OBJ_X_STEP 70
    #define OBJ_Y_STEP 60
    #define OBJ_X_START 32
    #define OBJ_Y_START 45
    #define TITLE_Y_OFS 0
    #define CATALOG1_W 220
    #define CATALOG1_X_OFS 2
    #define CATALOG_X_OFS -2
    #define OBJITEM_Y_OFS 0
    #define CTRLBAR_BTNPCT_W 95
    #define CTRLBAR_BTNPCT_H 95
    #define WINBAR_PADHOR   0
    #define PLAYBAR_X_OFS   0
    #define PLAYBAR_Y_OFS    -3
    #define CTRLBAR_LABPCT_H    14
    #define PLAYTIME_X_OFS    1
    #define PLAYTIME_Y_OFS    -5
    #define PLAYNAME_Y_OFS    0
    #define PLAYSTA_X_OFS    10
    #define PLAYSPD_X_OFS    2
    #define BTNLAB_Y_OFS    -2
    #define CTRLBAR_SMY_SIZE   &lv_font_montserrat_18
    #define CTRLBAR_PCT_H   32 
    #define CTRLBAR_BTN_W   35
    #define CTRLBAR_BTN_H   35
    #define BARBTN_OUTLINE 2
    #define BARBTN_PAD_BOTTOM   0
    #define CTRLBARLAT_H    5
	#define PREVIEW_WIN_W_PCT   33
    #define PREVIEW_WIN_H_PCT   34
    #define PREVIEW_WIN_BORDER 3
    #define PREVIEW_WINNAME_Y_OFS   3
    #define MPLIST_WIN_X_OFS -15 
    #define MPLIST_WIN_Y_OFS 10
    #define MPINFO_WIN_X_OFS -15
    #define MPINFO_WIN_Y_OFS 40 
    #define MPINFO_WIN_W_PCT 45
    #define MPINFO_BTN_PADVER 1
    #define MPPLAYLIST_WIN_W_PCT 45
    #define MPPLAYLIST_LAB_PADVER  0
    #define MPPLAYLIST_LAB_PANHOR  2
    #define SPECT_CHART_X_OFS   10 
    #define SPECT_CHART_Y_OFS   -25
    #define SPECT_CHART_PADGAP  1 
    #define BSLIST_WIN_W_PCT  45
    #define CHECKBOX_LAB_W  150
    #define CHECKBOX_LAB_X_OFS  22
    #define EBOOK_LAB_PADHOR    5
    #define EBOOK_PAGELAB_PADHOR  -48
    #define ZOOMMOVE_LABSIZE   &lv_font_montserrat_14
    #define ZOOMMOVE_CONT_SIZE 35

#else 
    // used for default lvgl resolution 720P (1280*720) 
    #define STYLE_OUTLINE_W   3
    #define MAINPAGE_BTNPCT_W 80
    #define SUBPAGE_BTNPCT_W 70
    #define IMGICON_Y_OFS -20
    //for fslist page
    #define CONT_W 150
    #define CONT_H 150
    #define OBJ_X_STEP 243
    #define OBJ_Y_STEP 200
    #define OBJ_X_START 200
    #define OBJ_Y_START 110
    #define TITLE_Y_OFS 20
    #define CATALOG1_W 800
    #define CATALOG1_X_OFS 10
    #define CATALOG_X_OFS -10
    #define OBJITEM_Y_OFS 8
    #define CTRLBAR_BTNPCT_W 74
    #define CTRLBAR_BTNPCT_H 67
    #define WINBAR_PADHOR   5
    #define PLAYBAR_X_OFS   -8
    #define PLAYBAR_Y_OFS    -10
    #define CTRLBAR_LABPCT_H    12
    #define PLAYTIME_X_OFS    30
    #define PLAYTIME_Y_OFS    -5
    #define PLAYNAME_Y_OFS    10
    #define PLAYSTA_X_OFS    30
    #define PLAYSPD_X_OFS    10
    #define BTNLAB_Y_OFS    -10
    #define CTRLBAR_SMY_SIZE    &lv_font_montserrat_36
    #define CTRLBAR_PCT_H   31 
    #define CTRLBAR_BTN_W   120
    #define CTRLBAR_BTN_H   110
    #define BARBTN_OUTLINE 5
    #define BARBTN_PAD_BOTTOM   1
    #define CTRLBARLAT_H    10
    #define PREVIEW_WIN_W_PCT   34
    #define PREVIEW_WIN_H_PCT   34
    #define PREVIEW_WIN_BORDER 5
    #define PREVIEW_WINNAME_Y_OFS   3
    #define MPLIST_WIN_X_OFS -50 
    #define MPLIST_WIN_Y_OFS 10
    #define MPINFO_WIN_X_OFS -50
    #define MPINFO_WIN_Y_OFS 150
    #define MPINFO_WIN_W_PCT 28
    #define MPINFO_BTN_PADVER 3
    #define MPPLAYLIST_WIN_W_PCT 28
    #define MPPLAYLIST_LAB_PADVER  0
    #define MPPLAYLIST_LAB_PANHOR  5
    #define SPECT_CHART_X_OFS   100 
    #define SPECT_CHART_Y_OFS   -100
    #define SPECT_CHART_PADGAP  3 
    #define BSLIST_WIN_W_PCT 28 
    #define CHECKBOX_LAB_W 300 
    #define CHECKBOX_LAB_X_OFS  40  
    #define EBOOK_LAB_PADHOR 20    
    #define EBOOK_PAGELAB_PADHOR  -200 
    #define ZOOMMOVE_LABSIZE   &lv_font_montserrat_28
    #define ZOOMMOVE_CONT_SIZE 100

#endif // DEBUG

void ui_mainpage_screen_init(void);
void ui_subpage_screen_init(void);
void ui_fspage_screen_init(void);
void ui_ctrl_bar_screen_init(void);
void ui_ebook_screen_init(void);

int create_mainpage_scr(void);
int clear_mainpage_scr(void);

int create_subpage_scr(void);
int clear_subpage_scr(void);

int create_fspage_scr(void);
int clear_fapage_scr(void);
int create_ctrlbarpage_scr(lv_obj_t *p,lv_event_cb_t cb_func);
int clear_ctrlbarpage_scr(void);
int create_ctrlbar_in_photo(lv_obj_t * parent);
int create_ctrlbar_in_music(lv_obj_t * parent);
int create_ctrlbar_in_video(lv_obj_t *parent);


int clear_child_win(lv_obj_t * child_obj);
int create_ebook_scr(lv_obj_t * p,lv_event_cb_t cb_func);



#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
