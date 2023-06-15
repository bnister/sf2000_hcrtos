// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.0.5
// LVGL VERSION: 8.2
// PROJECT: project_ui_Project
#include "app_config.h"
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>
#include "win_media_list.h"
#include <hcuapi/vidmp.h>

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
// #include "local_mp_ui_lld.h"
#include "mp_mainpage.h"
#include "mp_subpage.h"
#include "mp_fspage.h"
#include "mp_ctrlbarpage.h"
#include "mp_ebook.h"
#include "src/font/lv_font.h"
#include "setup.h"
#include "factory_setting.h"

#include "mul_lang_text.h"

#include "media_spectrum.h"
#include "mp_spectdis.h"
///////////////////// VARIABLES ////////////////////
#if 1
lv_obj_t * ui_mainpage;
lv_obj_t * ui_subpage;
lv_obj_t * ui_fspage;
lv_obj_t * ui_ctrl_bar;

lv_obj_t * ui_play_bar;
lv_obj_t * ui_playbar;
lv_obj_t * ui_winbar;
lv_obj_t * ui_playname;
lv_obj_t * ui_playstate;
lv_obj_t * ui_speed;

lv_obj_t * ui_ebook_txt;
lv_obj_t * ui_ebook_label;
lv_obj_t * ui_ebook_label_page;
lv_obj_t *subtitles_obj;
lv_obj_t *subtitles_obj_pic;
/*some event can not update in sq so user handle in here*/
lv_group_t * main_group;
lv_group_t * sub_group;
lv_group_t * fs_group;
lv_group_t * play_bar_group;
lv_group_t * ebook_group;

#endif
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
        ctrlbarpage_open();
        #ifdef RTOS_SUBTITLE_SUPPORT
        create_subtitles_rect();
        #endif
    }
    if(event == LV_EVENT_SCREEN_UNLOAD_START) {
        #ifdef RTOS_SUBTITLE_SUPPORT
        del_subtitles_rect();
        #endif
        ctrlbarpage_close();
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


/**
 * @description: init screen in lvgl app,
 * just create a screen and add event ,weight display in screen load event cb
 * @return {*}
 * @author: Yanisin
 */
void ui_mainpage_screen_init(void)
{
    // just create a screen and add event ,weight display in event cb  
    ui_mainpage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_mainpage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_mainpage, ui_event_mainpage, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(ui_mainpage, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_mainpage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

}
void ui_subpage_screen_init(void)
{
    // ui_subpage
    ui_subpage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_subpage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_subpage, ui_event_subpage, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(ui_subpage, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_subpage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

}
extern void preview_player_message_ctrl(void * arg1,void *arg2);
void ui_fspage_screen_init(void)
{
    ui_fspage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_fspage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_fspage, ui_event_fspage, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(ui_fspage, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fspage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    //for message ctrl 
    screen_entry_t fspage_entry;
    fspage_entry.screen = ui_fspage;
    fspage_entry.control = preview_player_message_ctrl;
    api_screen_regist_ctrl_handle(&fspage_entry);
}

extern void media_playbar_control(void *arg1, void *arg2);
void ui_ctrl_bar_screen_init(void)
{

    // ui_ctrl_bar
    ui_ctrl_bar = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ctrl_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_ctrl_bar, ui_event_ctrl_bar, LV_EVENT_ALL, NULL);

    screen_entry_t ctrl_bar_entry;
    ctrl_bar_entry.screen = ui_ctrl_bar;
    ctrl_bar_entry.control = media_playbar_control;
    api_screen_regist_ctrl_handle(&ctrl_bar_entry);

}
void ui_ebook_screen_init(void)
{
	ui_ebook_txt = lv_obj_create(NULL);
	lv_obj_clear_flag(ui_ebook_txt, LV_OBJ_FLAG_SCROLLABLE); 
	lv_obj_set_width(ui_ebook_txt, LV_PCT(100));    
	lv_obj_set_height(ui_ebook_txt, LV_PCT(100));    
	lv_obj_add_event_cb(ui_ebook_txt, ui_event_ebook, LV_EVENT_ALL, NULL);    
	lv_obj_set_style_bg_color(ui_ebook_txt, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);    
	lv_obj_set_style_bg_opa(ui_ebook_txt, 255, LV_PART_MAIN | LV_STATE_DEFAULT); 
}
///////////////////// SCREENS ////////////////////


lv_obj_t* mp_statebar;
lv_obj_t * main_cont1=NULL;
#define SCR_MAINICON_NUM 4
lv_obj_t * create_storage_icon(lv_obj_t * p)
{
    lv_obj_t * mp_statebar=lv_obj_create(p);
    lv_obj_set_width(mp_statebar, lv_pct(100));
    lv_obj_set_height(mp_statebar, lv_pct(8));
    lv_obj_clear_flag(mp_statebar, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(mp_statebar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(mp_statebar, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(mp_statebar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(mp_statebar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(mp_statebar,lv_xpixel_transform(100),0);
    lv_obj_set_style_pad_ver(mp_statebar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(mp_statebar, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(mp_statebar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(mp_statebar,LV_FLEX_FLOW_ROW);
    lv_obj_set_style_text_font(mp_statebar, osd_font_get_by_langid(0,FONT_LARGE), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* usb_icon=lv_label_create(mp_statebar);
    lv_obj_set_size(usb_icon,LV_PCT(7),LV_SIZE_CONTENT);
    lv_label_set_text(usb_icon,"");
    lv_obj_t* sd_icon=lv_label_create(mp_statebar);
    lv_obj_set_size(sd_icon,LV_PCT(7),LV_SIZE_CONTENT);
    lv_label_set_text(sd_icon,"");
    partition_info_t * partition_info2=mmp_get_partition_info();

    if(partition_info2 != NULL){
        if(partition_info2->count==0){
            lv_obj_clean(mp_statebar);
            lv_obj_t* icon = lv_label_create(mp_statebar);
            set_label_text2(icon,STR_MP_NO_DEV,FONT_NORMAL);
        }else if(partition_info2->count>0){
            for(int i=0;i<partition_info2->count;i++){
                char *dev_name=glist_nth_data(partition_info2->dev_list,i);

                //rtos: sdaxx; linux: /media/hddxx or /media/sdxx
                if(strstr(dev_name,"sd") || strstr(dev_name,"hd")||strstr(dev_name,"usb")){
                    lv_label_set_text(usb_icon,LV_SYMBOL_USB);
                }
                if(strstr(dev_name,"mmc")){
                    lv_label_set_text(sd_icon,LV_SYMBOL_SD_CARD);
                }
            }
        }
    }else{
        lv_obj_clean(mp_statebar);
        lv_obj_t* icon = lv_label_create(mp_statebar);
        set_label_text2(icon,STR_MP_NO_DEV,FONT_NORMAL);
    }

    return mp_statebar;
}
static int create_subobj_in_cont(lv_obj_t* p)
{
    for(int i = 0; i <SCR_MAINICON_NUM;i++){
        lv_obj_t* obj=lv_obj_create(p);
        lv_obj_set_size(obj,LV_PCT(20),LV_PCT(53));
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t* img=lv_img_create(obj);
        lv_obj_set_align(img,LV_ALIGN_TOP_MID);

        lv_obj_t* btn=lv_btn_create(obj);
        lv_obj_set_size(btn,LV_PCT(MAINPAGE_BTNPCT_W),LV_PCT(25));
        lv_obj_set_align(btn,LV_ALIGN_BOTTOM_MID);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        // lv_obj_set_style_text_font(btn, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(btn, lv_color_hex(0xFAD665), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_opa(btn, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(btn, STYLE_OUTLINE_W, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_pad(btn, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_text_opa(btn, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_t* label=lv_label_create(btn);
        lv_obj_set_align(label, LV_ALIGN_CENTER);
        lv_obj_add_event_cb(btn,main_page_keyinput_event_cb,LV_EVENT_ALL,0);
    }
    // add img source and add text in label
    lv_img_set_src(lv_obj_get_child(lv_obj_get_child(p,0),0),&IDB_Icon_Home_Movie);
    lv_obj_t* temp_btn=lv_obj_get_child(lv_obj_get_child(p,0),1);
    set_label_text2(lv_obj_get_child(temp_btn,0), STR_MOVIE,FONT_LARGE);
    
    lv_img_set_src(lv_obj_get_child(lv_obj_get_child(p,1),0),&IDB_Icon_Home_Music);
    temp_btn=lv_obj_get_child(lv_obj_get_child(p,1),1);
    set_label_text2(lv_obj_get_child(temp_btn,0), STR_MUSIC,FONT_LARGE);

    lv_img_set_src(lv_obj_get_child(lv_obj_get_child(p,2),0),&IDB_Icon_Home_Photo);
    temp_btn=lv_obj_get_child(lv_obj_get_child(p,2),1);
    set_label_text2(lv_obj_get_child(temp_btn,0), STR_PHOTO,FONT_LARGE);

    lv_img_set_src(lv_obj_get_child(lv_obj_get_child(p,3),0),&IDB_Icon_Home_Txt);
    temp_btn=lv_obj_get_child(lv_obj_get_child(p,3),1);
    set_label_text2(lv_obj_get_child(temp_btn,0), STR_TEXT,FONT_LARGE);

    return 0;
}

static lv_obj_t* create_obj_cont(lv_obj_t* p)
{
    lv_obj_t* cont1=lv_obj_create(p);
    lv_obj_align_to(cont1,mp_statebar,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
    lv_obj_set_size(cont1,LV_PCT(100),LV_PCT(92));
    lv_obj_clear_flag(cont1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(cont1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cont1, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cont1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cont1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_flex_flow(cont1,LV_FLEX_FLOW_ROW_WRAP,0);
    lv_obj_set_flex_align(cont1,LV_FLEX_ALIGN_SPACE_AROUND,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    create_subobj_in_cont(cont1);

    return cont1;
}
int create_mainpage_scr(void)
{ 
    mp_statebar=create_storage_icon(ui_mainpage);
    main_cont1=create_obj_cont(ui_mainpage);    
    lv_obj_add_event_cb(mp_statebar,storage_icon_refresh_cb, LV_EVENT_REFRESH, NULL);
    return 0;
}

int clear_mainpage_scr(void)
{
    lv_obj_clean(ui_mainpage);
    return 0;
}





#define CONT2_W 200
#define CONT2_H 300

static lv_obj_t* create_flex_objcont2(lv_obj_t * p,int count){

	lv_obj_t *obj_cont = lv_obj_create(p);
	lv_obj_set_layout(obj_cont, LV_LAYOUT_FLEX);
	lv_obj_set_size(obj_cont, LV_PCT(100), LV_PCT(92));
    lv_obj_set_align(obj_cont, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_add_flag(obj_cont,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj_cont, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj_cont,0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj_cont,0,0);
    lv_obj_set_scrollbar_mode(obj_cont,LV_SCROLLBAR_MODE_OFF);               

    lv_obj_set_flex_flow(obj_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj_cont, LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    // lv_obj_set_style_pad_hor(obj_cont,100,0);
    // lv_obj_set_style_pad_gap(obj_cont,100,0); //分别子对象间的设置行和列间距
    
    for(int i = 0; i < count; i++) {
        lv_obj_t*  obj= lv_obj_create(obj_cont);
        lv_obj_set_size(obj,LV_PCT(23),LV_PCT(55));
        lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(obj,0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj,0,0);
        lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(obj,LV_SCROLLBAR_MODE_OFF);               
    }

    return obj_cont;

}

/**
 * @description: 
 * @param {int} count,just mean count of mount dev
 * @return {*}
 * @author: Yanisin
 */
lv_obj_t* create_storage_subicon(int count)
{
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    lv_obj_t* obj_cont=create_flex_objcont2(ui_subpage,count+1);
    lv_obj_t *ui_btnback = lv_btn_create(lv_obj_get_child(obj_cont,0));
    lv_obj_set_size(ui_btnback,LV_PCT(SUBPAGE_BTNPCT_W),LV_PCT(25));
    lv_obj_align(ui_btnback, LV_ALIGN_BOTTOM_MID,0,0);
    lv_obj_add_flag(ui_btnback, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_btnback, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_btnback, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_btnback,0,0);
    lv_obj_set_style_text_color(ui_btnback, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_btnback, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btnback, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(ui_btnback, lv_color_hex(0xFAD665), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_btnback, STYLE_OUTLINE_W, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_btnback, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_color(ui_btnback, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_opa(ui_btnback, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    lv_obj_t *ui_labback = lv_label_create(ui_btnback);
    lv_obj_set_width(ui_labback, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_labback, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_labback, 0);
    lv_obj_set_y(ui_labback, 0);
    lv_obj_set_align(ui_labback, LV_ALIGN_CENTER);
    set_label_text2(ui_labback, STR_BACK,FONT_LARGE);

    lv_obj_t *ui_Image1 = lv_img_create(lv_obj_get_child(obj_cont,0));
    lv_img_set_src(ui_Image1, &Thumbnail_Upfolder);
    lv_obj_set_width(ui_Image1, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Image1, LV_SIZE_CONTENT);
    lv_obj_align(ui_Image1, LV_ALIGN_CENTER,0,IMGICON_Y_OFS);
    lv_obj_add_flag(ui_Image1, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_Image1, LV_OBJ_FLAG_SCROLLABLE);

    for(int i=0; i<count;i++){
        lv_obj_t *partition_icon = lv_btn_create(lv_obj_get_child(obj_cont,i+1));
        lv_obj_set_size(partition_icon,LV_PCT(SUBPAGE_BTNPCT_W),LV_PCT(25));
        lv_obj_set_align(partition_icon, LV_ALIGN_BOTTOM_MID);
        lv_obj_add_flag(partition_icon, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_clear_flag(partition_icon, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(partition_icon, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(partition_icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(partition_icon,0,0);
        lv_obj_set_style_text_color(partition_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(partition_icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(partition_icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(partition_icon, osd_font_get_by_langid(0,FONT_LARGE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(partition_icon, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_opa(partition_icon, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(partition_icon, lv_color_hex(0xFAD665), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_opa(partition_icon, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(partition_icon, STYLE_OUTLINE_W, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_pad(partition_icon, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_text_color(partition_icon, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_text_opa(partition_icon, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

        lv_obj_t* ui_Image = lv_img_create(lv_obj_get_child(obj_cont,i+1));
        lv_img_set_src(ui_Image, &IDB_Patition_Icon);
        lv_obj_set_width(ui_Image, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_Image, LV_SIZE_CONTENT);
        lv_obj_add_flag(ui_Image, LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_clear_flag(ui_Image, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ui_Image,LV_ALIGN_CENTER,0,0);

        lv_obj_t * ui_labrootdir = lv_label_create(partition_icon);
        lv_obj_set_width(ui_labrootdir, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_labrootdir, LV_SIZE_CONTENT);
        lv_obj_set_align(ui_labrootdir, LV_ALIGN_CENTER);
        int partition_letter=67;    //mean 'C' in ASCII
        partition_letter=partition_letter+i;
        char  partition_letter2[2];
        sprintf(partition_letter2,"%c",partition_letter);
        set_label_text2(ui_labrootdir, 0,FONT_LARGE);  //set user data to label_text and set text font 
        lv_label_set_text(ui_labrootdir,partition_letter2);
    }
    return obj_cont;

}
int create_subpage_scr(void)
{
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    partition_info_t * part_info = mmp_get_partition_info();
    lv_obj_t* obj_cont=lv_obj_create(ui_subpage);
    lv_obj_set_size(obj_cont,LV_PCT(100),LV_PCT(8));
    lv_obj_clear_flag(obj_cont, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(obj_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj_cont, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj_cont, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(obj_cont,lv_xpixel_transform(100),0);
    lv_obj_set_style_pad_ver(obj_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj_cont, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(obj_cont, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(obj_cont,LV_FLEX_FLOW_ROW);

    lv_obj_t * ui_titie_ = lv_label_create(obj_cont);
    lv_obj_set_style_text_color(ui_titie_, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_titie_, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* obj_cont1= create_storage_subicon(part_info->count);

    for(int i=0;i<part_info->count+1;i++){
        lv_obj_add_event_cb(lv_obj_get_child(lv_obj_get_child(obj_cont1,i),0),sub_page_keyinput_event_cb,LV_EVENT_ALL,0);
    }
    switch (get_media_type())
    {
        case MEDIA_TYPE_VIDEO: 
            set_label_text2(ui_titie_, STR_MOVIE,FONT_LARGE);
            break;
        case MEDIA_TYPE_MUSIC: 
            set_label_text2(ui_titie_, STR_MUSIC,FONT_LARGE);
            break;
        case MEDIA_TYPE_PHOTO: 
            set_label_text2(ui_titie_, STR_PHOTO,FONT_LARGE);
            break;
        case MEDIA_TYPE_TXT: 
            set_label_text2(ui_titie_, STR_TEXT,FONT_LARGE);
            break;
        default: 
            break;
    }
    return 0; 
}
int clear_subpage_scr(void)
{
    lv_obj_clean(ui_subpage);
    return 0;
}


lv_obj_t * obj_item[CONT_WINDOW_CNT]={NULL};
lv_obj_t * obj_labelitem[CONT_WINDOW_CNT]={NULL};

/**
 * @description:  return a label out of objitem
 * @param {lv_obj_t*} p
 * @return {*}
 * @author: Yanisin
 */
lv_obj_t * objitem_style_init(lv_obj_t* scr,lv_obj_t* p)
{
    // obj style
    lv_obj_set_style_radius(p, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(p, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(p, 10, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    // lv_obj_set_style_shadow_opa(p, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_shadow_width(p,0,0);
    lv_obj_set_style_border_side(p, LV_BORDER_SIDE_FULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(p, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(p, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(p, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_clear_flag(p,LV_OBJ_FLAG_SCROLLABLE);
    // lv_obj_set_style_outline_color(p, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    // lv_obj_set_style_outline_opa(p, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    // lv_obj_set_style_outline_width(p, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    // create a img child 
    lv_obj_t* img_child = lv_img_create(p);
    lv_obj_set_width(img_child, LV_SIZE_CONTENT);
    lv_obj_set_height(img_child, LV_SIZE_CONTENT);
    lv_obj_set_align(img_child, LV_ALIGN_CENTER);
    lv_obj_add_flag(img_child, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(img_child, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_outline_color(img_child, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(img_child, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(img_child, 3, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(img_child, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);  
    //create a label buttom of scr
    lv_obj_t *label=lv_label_create(scr);
    lv_label_set_text(label,"");
    lv_obj_set_style_text_color(label,lv_color_hex(0xffffff),0);
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    lv_obj_set_style_text_font(label,&LISTFONT_3000,0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_width(label, CONT_W);
    lv_obj_set_height(label,LV_SIZE_CONTENT );       
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);     
    lv_obj_align_to(label,p,LV_ALIGN_OUT_BOTTOM_MID,0,OBJITEM_Y_OFS);

    return label;
}

int create_objcont(lv_obj_t * p){
    int k=0;
    for(int i=0;i<3;i++){
        for(int j=0;j<4;j++){
            obj_item[k] =lv_obj_create(p);
            lv_obj_set_size(obj_item[k],CONT_W,CONT_H);
            lv_obj_set_pos(obj_item[k],OBJ_X_START+(OBJ_X_STEP * j),OBJ_Y_START+(OBJ_Y_STEP * i));
            obj_labelitem[k]=objitem_style_init(p,obj_item[k]);
            lv_obj_add_event_cb(obj_item[k],fs_page_keyinput_event_cb, LV_EVENT_ALL, NULL);
            lv_group_add_obj(lv_group_get_default(),obj_item[k]);
            k++;
        }
    }
}

int create_fspage_scr(void)
{    
    lv_obj_t *ui_fstitle = lv_label_create(ui_fspage);
    lv_obj_set_width(ui_fstitle, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fstitle, LV_SIZE_CONTENT);
    lv_obj_align(ui_fstitle, LV_ALIGN_TOP_MID,0,TITLE_Y_OFS);
    // lv_label_set_text(ui_fstitle, "");
    lv_obj_set_style_text_color(ui_fstitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_fstitle, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    switch (get_media_type())
    {
        case MEDIA_TYPE_VIDEO: 
            set_label_text2(ui_fstitle, STR_MOVIE,FONT_LARGE);
            break;
        case MEDIA_TYPE_MUSIC: 
            set_label_text2(ui_fstitle, STR_MUSIC,FONT_LARGE);
            break;
        case MEDIA_TYPE_PHOTO: 
            set_label_text2(ui_fstitle, STR_PHOTO,FONT_LARGE);
            break;
        case MEDIA_TYPE_TXT: 
            set_label_text2(ui_fstitle, STR_TEXT,FONT_LARGE);
            break;
        default: 
            break;
    }

    // ui_fsbarimg
    lv_obj_t* ui_fsbarimg = lv_img_create(ui_fspage);
    lv_img_set_src(ui_fsbarimg, &IDB_File_Select_Info_Bar);
    lv_obj_set_width(ui_fsbarimg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fsbarimg, LV_SIZE_CONTENT);
    lv_obj_align_to(ui_fsbarimg,ui_fstitle,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_add_flag(ui_fsbarimg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_fsbarimg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* ui_fspath = lv_label_create(ui_fsbarimg);
    lv_obj_set_width(ui_fspath, CATALOG1_W);
    lv_obj_set_height(ui_fspath, LV_SIZE_CONTENT);
    lv_obj_align(ui_fspath, LV_ALIGN_LEFT_MID,CATALOG1_X_OFS,0);
    lv_label_set_text(ui_fspath, "/sda");
    lv_label_set_long_mode(ui_fspath,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(ui_fspath, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_fspath, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_fspath, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_text_font(ui_fspath,osd_font_get(FONT_MID),0);
    lv_obj_set_style_text_font(ui_fspath,osd_font_get_by_langid(1,FONT_MID),0);

    lv_obj_t* ui_fscount = lv_label_create(ui_fsbarimg);
    lv_obj_set_width(ui_fscount, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fscount, LV_SIZE_CONTENT);
    lv_obj_align(ui_fscount, LV_ALIGN_RIGHT_MID,CATALOG_X_OFS,0);
    lv_label_set_text(ui_fscount, "");
    lv_obj_set_style_text_color(ui_fscount, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_fscount, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_fscount,osd_font_get(FONT_MID),0);

    create_objcont(ui_fspage);
    return 0 ;
}

int clear_fapage_scr(void)
{
    lv_obj_clean(ui_fspage);
    int i;
    for(i=0;i< 12;i++)
    {
        if(lv_obj_is_valid(obj_labelitem[i])==false)
            obj_labelitem[i]=NULL;
    }
    return 0;
}

lv_obj_t* ctrlbarbtn[MAX_BARBTN_ITEM]={NULL};
lv_obj_t*  objcont_create_subbtn(lv_obj_t* p)
{
    lv_obj_t* obj = lv_btn_create(p);
    lv_obj_set_width(obj, CTRLBAR_BTN_W);
    lv_obj_set_height(obj, CTRLBAR_BTN_H);
    lv_obj_set_x(obj, 0);
    lv_obj_set_y(obj, 0);
    lv_obj_set_align(obj, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(obj, lv_color_hex(0x847A7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(obj, lv_color_hex(0x3260E9), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(obj, BARBTN_OUTLINE, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(obj, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x323232),  LV_PART_MAIN |LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(obj, 255, LV_STATE_DISABLED);
    lv_obj_set_style_pad_bottom(obj,BARBTN_PAD_BOTTOM,0);
#ifdef LVGL_RESOLUTION_240P_SUPPORT
    lv_obj_set_size(obj,LV_PCT(16),LV_PCT(100));
#endif 

    lv_obj_t* ui_barlab = lv_label_create(obj);
    lv_obj_set_width(ui_barlab, LV_SIZE_CONTENT);   
    lv_obj_set_height(ui_barlab, LV_SIZE_CONTENT);   
    lv_obj_set_align(ui_barlab, LV_ALIGN_BOTTOM_MID);
    lv_label_set_text(ui_barlab, "");
    return obj;

}

int create_ctrlbarpage_scr(lv_obj_t *p,lv_event_cb_t cb_func)
{
#if 1
    ui_play_bar = lv_obj_create(p);
    lv_obj_set_width(ui_play_bar, LV_PCT(100));
    lv_obj_set_height(ui_play_bar, LV_PCT(CTRLBAR_PCT_H));
    lv_obj_set_align(ui_play_bar, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_clear_flag(ui_play_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui_play_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_play_bar, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_play_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_play_bar,0,0);

    // ui_winbar
    ui_winbar = lv_obj_create(ui_play_bar);
    lv_obj_set_size(ui_winbar, LV_PCT(CTRLBAR_BTNPCT_W),LV_PCT(CTRLBAR_BTNPCT_H));
    lv_obj_set_align(ui_winbar, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_winbar, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_add_flag(ui_winbar, LV_OBJ_FLAG_SCROLL_WITH_ARROW);     /// Flags
    lv_obj_set_scrollbar_mode(ui_winbar, LV_SCROLLBAR_MODE_OFF);
    // lv_obj_set_scroll_snap_x(ui_winbar,  LV_SCROLL_SNAP_START);
    lv_obj_set_style_radius(ui_winbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_winbar, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_winbar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_winbar, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_winbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_winbar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(ui_winbar, WINBAR_PADHOR, LV_PART_MAIN | LV_STATE_DEFAULT);

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    lv_obj_set_style_pad_all(ui_winbar,0,0);
    lv_obj_set_style_pad_gap(ui_winbar,2,0);
#endif

    // ui_playbar
    ui_playbar = lv_slider_create(ui_play_bar);
    lv_obj_set_width(ui_playbar, LV_PCT(60));
    lv_obj_set_height(ui_playbar, LV_PCT(CTRLBAR_LABPCT_H));
    lv_obj_align_to(ui_playbar, ui_winbar,LV_ALIGN_OUT_TOP_LEFT,PLAYBAR_X_OFS,PLAYBAR_Y_OFS);
    // lv_obj_add_state(ui_playbar, LV_STATE_FOCUS_KEY);       /// States
    lv_obj_set_style_radius(ui_playbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_playbar, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_playbar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(ui_playbar, lv_color_hex(0x3260E9), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(ui_playbar, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ui_playbar, 5, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ui_playbar, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(ui_playbar, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_playbar, lv_color_hex(0xFFFF00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_playbar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_playbar, lv_color_hex(0xC4BABA), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_playbar, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
#ifdef LVGL_RESOLUTION_240P_SUPPORT
    lv_obj_set_style_pad_all(ui_playbar,3,LV_PART_KNOB | LV_STATE_DEFAULT);
#endif

    lv_obj_set_flex_flow(ui_winbar, LV_FLEX_FLOW_ROW);
    for(int i=0;i<MAX_BARBTN_ITEM;i++){
        ctrlbarbtn[i]=objcont_create_subbtn(ui_winbar);
        lv_obj_add_event_cb(ctrlbarbtn[i],cb_func, LV_EVENT_ALL, NULL);
    }
    lv_obj_add_event_cb(ui_playbar,cb_func, LV_EVENT_ALL, NULL);

    // ui_cur_time
    lv_obj_t * ui_cur_time = lv_label_create(ui_play_bar);
    lv_obj_set_width(ui_cur_time, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_cur_time, LV_SIZE_CONTENT);
    lv_obj_align_to(ui_cur_time,ui_playbar, LV_ALIGN_OUT_RIGHT_TOP,PLAYTIME_X_OFS,PLAYTIME_Y_OFS);
    lv_label_set_text(ui_cur_time, "00:00:00");
    lv_obj_set_style_text_color(ui_cur_time, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_cur_time, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_cur_time, osd_font_get(FONT_MID), LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_str
    lv_obj_t * ui_str = lv_label_create(ui_play_bar);
    lv_obj_set_width(ui_str, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_str, LV_SIZE_CONTENT);
    lv_obj_align_to(ui_str,ui_cur_time, LV_ALIGN_OUT_RIGHT_TOP,0,0);
    lv_label_set_text(ui_str, "/");
    lv_obj_set_style_text_color(ui_str, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_str, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_str,osd_font_get(FONT_MID), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_total_time = lv_label_create(ui_play_bar);
    lv_obj_set_width(ui_total_time, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_total_time, LV_SIZE_CONTENT);
    lv_obj_align_to(ui_total_time,ui_str, LV_ALIGN_OUT_RIGHT_TOP,0,0);
    lv_label_set_text(ui_total_time, "");
    lv_obj_set_style_text_color(ui_total_time, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_total_time, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_total_time, osd_font_get(FONT_MID), LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_playname
    ui_playname = lv_label_create(ui_play_bar);
    lv_obj_set_size(ui_playname,LV_PCT(60),LV_SIZE_CONTENT);
    lv_obj_align_to(ui_playname,ui_winbar, LV_ALIGN_OUT_BOTTOM_LEFT,0,PLAYNAME_Y_OFS);
#ifdef CONFIG_SOC_HC15XX
    lv_label_set_long_mode(ui_playname, LV_LABEL_LONG_CLIP);
#else
    lv_label_set_long_mode(ui_playname, LV_LABEL_LONG_SCROLL_CIRCULAR);
#endif 
    lv_label_set_text(ui_playname, "");
    lv_obj_set_style_text_color(ui_playname, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_playname, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_playname, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_playname,&LISTFONT_3000,0);

    ui_playstate = lv_label_create(ui_play_bar);
    lv_obj_align_to(ui_playstate,ui_playname, LV_ALIGN_OUT_RIGHT_TOP,PLAYSTA_X_OFS,0);
    lv_label_set_text(ui_playstate, "       ");
    lv_obj_set_style_text_color(ui_playstate, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_playstate, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_playstate,&LISTFONT_3000,0);

    ui_speed = lv_label_create(ui_play_bar);
    lv_obj_set_width(ui_speed, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_speed, LV_SIZE_CONTENT);
    lv_obj_align_to(ui_speed,ui_playstate, LV_ALIGN_OUT_RIGHT_TOP,PLAYSPD_X_OFS,0);
    lv_label_set_text(ui_speed, "");
    lv_obj_set_style_text_color(ui_speed, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_speed, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_speed, &LISTFONT_3000, LV_PART_MAIN | LV_STATE_DEFAULT);
#if 1
    // ui_pervimg
    lv_obj_t *ui_pervimg = lv_label_create(ui_play_bar);
    lv_obj_set_width(ui_pervimg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_pervimg, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_pervimg, 0);
    lv_obj_set_y(ui_pervimg, 0);
    lv_obj_align_to(ui_pervimg,ui_winbar,LV_ALIGN_OUT_LEFT_MID,0,BTNLAB_Y_OFS);
    lv_obj_add_flag(ui_pervimg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_pervimg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(ui_pervimg,osd_font_get_by_langid(0,FONT_LARGE),0);
    lv_label_set_text(ui_pervimg,LV_SYMBOL_LEFT);

    // ui_nextimg
    lv_obj_t *ui_nextimg = lv_label_create(ui_play_bar);
    lv_obj_set_width(ui_nextimg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_nextimg, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_nextimg, 0);
    lv_obj_set_y(ui_nextimg, 0);
    lv_obj_align_to(ui_nextimg,ui_winbar,LV_ALIGN_OUT_RIGHT_MID,0,BTNLAB_Y_OFS);
    lv_obj_add_flag(ui_nextimg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_nextimg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(ui_nextimg,osd_font_get_by_langid(0,FONT_LARGE),0);
    lv_label_set_text(ui_nextimg,LV_SYMBOL_RIGHT);
#endif     
#endif
    lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[1],&Hint_FB,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[2],&Hint_FF,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[3],&Hint_Prev,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[4],&Hint_Next,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[5],&Hint_Stop,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[6],&Hint_Rotate,0);

    set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[1],0),STR_FB,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[2],0),STR_FF,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[3],0),STR_PREV,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[4],0),STR_NEXT,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[5],0),STR_STOP,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[6],0),STR_LISTROUND,FONT_MID);

    //group will fouce on last obj,so disable scroll bar 
    lv_group_remove_obj(ui_playbar);
    lv_group_focus_obj(ctrlbarbtn[0]);

    lv_obj_clear_flag(ui_playbar,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_cur_time,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_total_time,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_str,LV_OBJ_FLAG_HIDDEN);
    return 0;
}

int clear_ctrlbarpage_scr(void)
{
    lv_obj_clean(ui_ctrl_bar);
    // if(chart_timer_hdl!=NULL)
    // {   
    //     lv_timer_del(chart_timer_hdl);
    //     chart_timer_hdl=NULL;
    // }
    return 0;
}

int create_ctrlbar_in_video(lv_obj_t *parent)
{
    //diff in com page
    lv_obj_set_style_bg_opa(ui_ctrl_bar, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);//try ok 

    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);

    lv_obj_set_style_bg_img_src(ctrlbarbtn[7],&Hint_Info,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[8],&Hint_Playlist,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[9],&Hint_ZoomIn,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[10],&Hint_ZoomOut,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[11],&Hint_SF,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[12],&Hint_Step,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[13],&Hint_Setup,0);

    set_label_text2(lv_obj_get_child(ctrlbarbtn[7],0),STR_INFO,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[8],0),STR_LIST,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[9],0),STR_ZOOMIN,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[10],0),STR_ZOOMOUT,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[11],0),STR_SF,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[12],0),STR_STEP,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[13],0), projector_get_some_sys_param(P_ASPECT_RATIO)==DIS_TV_AUTO ? STR_ASPECT_AUTO :
    projector_get_some_sys_param(P_ASPECT_RATIO) ==DIS_TV_4_3 ? STR_ASPECT_4_3 : STR_ASPECT_16_9,FONT_MID); 

    lv_obj_del(ctrlbarbtn[14]);

}

void set_label_with_piceffect(lv_obj_t* p,image_effect_t* img_effect )
{
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    switch(img_effect->mode)
    {
        case IMG_SHOW_NULL: 
            set_label_text2(lv_obj_get_child(p,0),STR_NO_EFFECT,FONT_MID);
            break;
        case IMG_SHOW_NORMAL: 
            set_label_text2(lv_obj_get_child(p,0),STR_NOR_EFFECT,FONT_MID);
            break;
        case IMG_SHOW_SHUTTERS: 
            set_label_text2(lv_obj_get_child(p,0),STR_SHUTTER_EFFECT,FONT_MID);    
            break;
        case IMG_SHOW_BRUSH: 
            set_label_text2(lv_obj_get_child(p,0),STR_BRUSH_EFFECT,FONT_MID);    
            break;
        case IMG_SHOW_SLIDE: 
            set_label_text2(lv_obj_get_child(p,0),STR_SLIDE_EFFECT,FONT_MID);    
            break;
        case IMG_SHOW_RANDOM: 
            set_label_text2(lv_obj_get_child(p,0),STR_RAND_EFFECT,FONT_MID);    
            break;
        case IMG_SHOW_FADE: 
            set_label_text2(lv_obj_get_child(p,0),STR_FADE_EFFECT,FONT_MID);    
            break; 
        default :
            break;
    }    

}
    
int create_ctrlbar_in_photo(lv_obj_t * parent)
{
    lv_obj_set_style_bg_opa(ui_ctrl_bar, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);//try ok 
    //diff in com page
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    lv_obj_add_flag(ui_playbar,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lv_obj_get_child(ui_play_bar,2),LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lv_obj_get_child(ui_play_bar,3),LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lv_obj_get_child(ui_play_bar,4),LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[1],&IDB_Hint_Rotate_Left,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[2],&IDB_Hint_Rotate_Right,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[7],&Hint_Info,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[8],&Hint_Playlist,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[9],&Hint_ZoomIn,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[10],&Hint_ZoomOut,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[11],&IDB_Hint_Zoom_Move,0);
    //to add func btn 
    lv_obj_set_style_bg_img_src(ctrlbarbtn[12],&IDB_Hint_Music_Off,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[13],&IDB_Hint_Music_On,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[14],&IDB_Hint_CaptureLogo,0);

    set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[1],0),STR_ROTATE_RIGHT_K,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[2],0),STR_ROTATE_LEFT,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[7],0),STR_INFO,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[8],0),STR_LIST,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[9],0),STR_ZOOMIN,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[10],0),STR_ZOOMOUT,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[11],0),STR_ZOOMMOVE,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[12],0),STR_MUSIC_ON,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[13],0),STR_MUSIC_SRC,FONT_MID); 
    set_label_with_piceffect(ctrlbarbtn[14],get_img_effect_mode());

    return 0;
}

int create_ctrlbar_in_music(lv_obj_t * parent)
{
    lv_obj_set_style_bg_opa(ui_ctrl_bar, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);//try ok 
    create_music_spectrum(ui_ctrl_bar);
    lv_obj_add_state(ctrlbarbtn[1],LV_STATE_DISABLED);
    lv_obj_add_state(ctrlbarbtn[2],LV_STATE_DISABLED);

    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);

    lv_obj_set_style_bg_img_src(ctrlbarbtn[7],&Hint_Info,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[8],&Hint_Playlist,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[9],&IDB_Hint_ClosePanel,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[10],&IDB_Hint_mute,0);

    set_label_text2(lv_obj_get_child(ctrlbarbtn[7],0),STR_INFO,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[8],0),STR_LIST,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[9],0),STR_PANEL_CLOSE,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[10],0),STR_MUTE,FONT_MID);

    for(int i=11;i<MAX_BARBTN_ITEM;i++){
        lv_obj_del(ctrlbarbtn[i]);
    }
}

int clear_child_win(lv_obj_t * child_obj)
{
    if(lv_obj_is_valid(child_obj))
    {
        lv_obj_del(child_obj);
    }
    return 0;
}

int create_ctrlbar_in_ebook(lv_obj_t *p)
{
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);

    lv_obj_add_flag(ui_playbar,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lv_obj_get_child(ui_play_bar,2),LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lv_obj_get_child(ui_play_bar,3),LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lv_obj_get_child(ui_play_bar,4),LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_FB,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[1],&Hint_FF,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[2],&Hint_Prev,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[3],&Hint_Next,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[4],&Hint_Stop,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[5],&IDB_Hint_Music_Off,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[6],&Hint_Playlist,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[7],&Hint_Info,0);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[8],&IDB_Hint_Music_On,0);

    set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_EBOOK_UP,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[1],0),STR_EBOOK_DOWN,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[2],0),STR_EBOOK_PREV,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[3],0),STR_EBOOK_NEXT,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[4],0),STR_STOP,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[5],0),STR_MUSIC_OFF,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[6],0),STR_LIST,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[7],0),STR_INFO,FONT_MID); 
    set_label_text2(lv_obj_get_child(ctrlbarbtn[8],0),STR_MUSIC_SRC,FONT_MID); 

    for(int i=9;i<MAX_BARBTN_ITEM;i++){
        lv_obj_del(ctrlbarbtn[i]);
    }
    
} 

int create_ebook_scr(lv_obj_t * p,lv_event_cb_t cb_func)
{
    ui_ebook_label= lv_label_create(p); 
    lv_obj_set_size(ui_ebook_label,LV_PCT(100),LV_PCT(93));
    lv_obj_align(ui_ebook_label,LV_ALIGN_TOP_LEFT,0,0);
	lv_label_set_long_mode(ui_ebook_label, LV_LABEL_LONG_WRAP);  
	lv_label_set_recolor(ui_ebook_label, true);     
	// lv_obj_align(ui_ebook_label, LV_ALIGN_TOP_MID,0,0);
	lv_obj_set_style_text_color(ui_ebook_label,lv_color_hex(0xffffff),0);
    lv_obj_set_style_pad_hor(ui_ebook_label,EBOOK_LAB_PADHOR,0);
	lv_obj_set_style_text_font(ui_ebook_label,&LISTFONT_3000,0);//suport chinese SiYuanHeiTi_Nor_7000_28_1b
	ui_ebook_label_page= lv_label_create(p);
	lv_label_set_long_mode(ui_ebook_label_page, LV_LABEL_LONG_WRAP); 
	lv_label_set_recolor(ui_ebook_label_page, true); 

	// lv_obj_set_width(ui_ebook_label_page, 200);
    // lv_obj_set_height(ui_ebook_label_page, 40);
	// lv_obj_set_x(ui_ebook_label_page, 1080);
	// lv_obj_set_y(ui_ebook_label_page, 680);
    lv_obj_set_size(ui_ebook_label_page,LV_PCT(20),LV_SIZE_CONTENT);
    lv_obj_align_to(ui_ebook_label_page,ui_ebook_label,LV_ALIGN_OUT_BOTTOM_RIGHT,EBOOK_PAGELAB_PADHOR,0);
	lv_obj_set_style_text_color(ui_ebook_label_page,lv_color_hex(0xffffff),0);
	lv_obj_set_style_text_font(ui_ebook_label_page, &LISTFONT_3000, LV_PART_MAIN | LV_STATE_DEFAULT);

    //add a ctrl bar win 
    create_ctrlbarpage_scr(p,cb_func);
    create_ctrlbar_in_ebook(NULL);
}
