/*
win_dlna_play.c: used for cast dlna playing
 */
#include "app_config.h"

#ifdef DLNA_SUPPORT

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <hcuapi/codec_id.h>
#include <hccast/hccast_media.h>

#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"

#ifdef __linux__
#include <linux/input.h>
#else
#include <hcuapi/input.h>
#endif

#include <hcuapi/input-event-codes.h>


#include "obj_mgr.h"
#include "os_api.h"
#include "cast_api.h"
#include "com_api.h"
//#include "menu_mgr.h"
#include "osd_com.h"
#include "key_event.h"
#include "win_vol.h"
#include "screen.h"
#include "win_mute.h"
#include "win_cast_root.h"
#include "mul_lang_text.h"
#include "factory_setting.h"
    
#define PLAYER_BAR_H    160
#define PLAYER_BAR_W    (OSD_MAX_WIDTH-80)
#define PLAYER_BAR_X    (OSD_MAX_WIDTH-PLAYER_BAR_W)/2
#define PLAYER_BAR_Y    (OSD_MAX_HEIGHT-PLAYER_BAR_H-20)

#define SLIDE_W    (PLAYER_BAR_W - 300)
#define SLIDE_H    50
#define SLIDE_X    (PLAYER_BAR_W - SLIDE_W)/2
#define SLIDE_Y    90

#define PLAY_BAR_HIDE_TIME  (5000) //the playbar would hide after 10 seconds if no key

lv_obj_t * ui_cast_dlna = NULL;

static lv_obj_t *m_play_bar;
static lv_obj_t *m_lable_play_name;
static lv_obj_t *m_slider_obj;
static lv_obj_t *play_time_obj;
static lv_obj_t *total_time_obj;
static lv_obj_t *m_img_play;

static lv_obj_t *m_label_photo_frame = NULL;
static lv_obj_t *m_label_photo_flag = NULL;

static const lv_font_t *font_small;
static const lv_font_t *font_large;

static lv_group_t *m_player_group  = NULL;
static lv_timer_t *sec_counter_timer = NULL;
static lv_timer_t *bar_show_timer = NULL;

static  bool m_play_bar_on = true;
static hccast_media_type_e m_dlna_media_type;
static uint16_t m_push_type;

LV_IMG_DECLARE(img_lv_demo_music_slider_knob)
LV_IMG_DECLARE(img_lv_demo_music_btn_play)
LV_IMG_DECLARE(img_lv_demo_music_btn_pause)

static volatile bool m_win_dlna_open = false;

static int win_dlna_play_open(void *arg);
static int win_dlna_play_close(void *arg);

#define KEY_EVENT_SUPPORT
//MUTE UI contorl by system, ont in dlna
#define MUTE_CONTROL_BY_SYS

static void format_time(uint32_t time, char *time_fmt)
{
    uint32_t hour;
    uint32_t min;
    uint32_t second;
    uint32_t hours_24 = 24*3600;

    if (0 == time || time > hours_24){
        sprintf(time_fmt, "00:00:00");
        return;
    }

    hour = time / 3600;
    min = (time % 3600) / 60;
    second = time % 60;
    if (hour > 0)
        sprintf(time_fmt, "%02d:%02d:%02d", (int)hour, (int)min, (int)second);
    else
        sprintf(time_fmt, "%02d:%02d", (int)min, (int)second);

}

static void sec_timer_cb(lv_timer_t * t)
{
    uint32_t play_time = 0;
    uint32_t total_time = 0;
    char time_fmt[16];

    if (!m_play_bar || !m_play_bar_on)
        return;

    play_time = hccast_media_get_position()/1000;
    format_time(play_time, time_fmt);
    lv_label_set_text(play_time_obj, time_fmt);
    lv_slider_set_value(m_slider_obj, play_time, LV_ANIM_ON);

     total_time = hccast_media_get_duration()/1000;
    format_time(total_time, time_fmt);
    //printf("total time: %s\n", time_fmt);
    lv_label_set_text(total_time_obj, time_fmt);
    if (total_time > 0)
        lv_slider_set_range(m_slider_obj, 0, total_time);
}


static void play_bar_onoff(bool on)
{
    if (!m_play_bar)
        return;

    if (on){
        lv_obj_clear_flag(m_play_bar, LV_OBJ_FLAG_HIDDEN);
    }else{
        lv_obj_add_flag(m_play_bar, LV_OBJ_FLAG_HIDDEN);
    }
    //printf("%s(), open show: %d\n", __FUNCTION__, show);
    m_play_bar_on = on;

}


static void bar_show_timer_cb(lv_timer_t * t)
{

    play_bar_onoff(false);
    if (bar_show_timer){
        lv_timer_pause(bar_show_timer);
    }
}

#ifdef KEY_EVENT_SUPPORT

static uint32_t valid_key_map[] = {
    V_KEY_0,
    V_KEY_1,
    V_KEY_2,
    V_KEY_3,
    V_KEY_4,
    V_KEY_5,
    V_KEY_6,
    V_KEY_7,
    V_KEY_8,
    V_KEY_9,
    V_KEY_MUTE,
    V_KEY_V_UP,
    V_KEY_V_DOWN,
    V_KEY_PREV, //play previous
    V_KEY_NEXT, //play next
    V_KEY_UP,
    V_KEY_DOWN,
    V_KEY_ENTER,
    V_KEY_PLAY,
    V_KEY_PAUSE,
    V_KEY_FF,
    V_KEY_FB,
    V_KEY_SF,
    V_KEY_SB,
    V_KEY_LEFT,
    V_KEY_RIGHT,
    V_KEY_STOP,
    V_KEY_EXIT,
    V_KEY_MENU,
    V_KEY_INFO,
};

static bool key_filter(uint32_t vkey)
{
    int i;
    int count = sizeof(valid_key_map)/sizeof(valid_key_map[0]);
    for (i = 0; i < count; i++) {
        if (valid_key_map[i] == vkey)
            return true;
    }
    return false;
}
static void event_handler(lv_event_t * e)
{
    
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
//    lv_obj_t * cur_obj = NULL;


    if (ta == ui_cast_dlna){
       if(event == LV_EVENT_SCREEN_LOAD_START) {
            win_dlna_play_open(NULL);
        }
        if(event == LV_EVENT_SCREEN_UNLOAD_START) {
            win_dlna_play_close(NULL);
        }
    } else if (ta == m_play_bar){
        lv_indev_t *key_indev = lv_indev_get_act();
        if(event == LV_EVENT_KEY){
            uint32_t value = lv_indev_get_key(key_indev);
            if (key_indev->proc.state == LV_INDEV_STATE_PRESSED){
                if (value == LV_KEY_ESC)
                {
                    win_exit_to_cast_root_by_key_set(true);
                    _ui_screen_change(ui_wifi_cast_root,0,0);
                    return;
                    //change_screen(SCREEN_CHANNEL);
                }
            } else if (key_indev->proc.state == LV_INDEV_STATE_RELEASED){

            }

            uint32_t vkey = VKEY_NULL;
            vkey = key_convert_vkey(value);
			if(vkey == V_KEY_HOME)
			{
				win_exit_to_cast_root_by_key_set(true);
			}
            if (!key_filter(vkey))
                return;
            //printf("media vkey: %d...\n", vkey);
            api_control_send_key(vkey);
        }
    }
}
#endif



static void create_win_player(lv_obj_t *parent)
{
    static lv_style_t m_bar_style;


    m_play_bar_on = true;
    font_small = &lv_font_montserrat_22;
    font_large = &lv_font_montserrat_32;

    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_style_init(&m_bar_style);

    //create bar frame
    m_play_bar = lv_obj_create(parent);
    lv_obj_clear_flag(m_play_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_pos(m_play_bar, PLAYER_BAR_X, PLAYER_BAR_Y);
    lv_obj_set_size(m_play_bar, PLAYER_BAR_W, PLAYER_BAR_H);
    lv_obj_set_style_bg_color(m_play_bar, COLOR_DEEP_GREY, 0); //grey
    //lv_obj_set_style_border_opa(m_play_bar, LV_OPA_TRANSP, 0);
    //lv_obj_set_style_radius(m_play_bar, 20, 0);

    //create file name label
    m_lable_play_name = lv_label_create(m_play_bar);
    lv_obj_set_pos(m_lable_play_name, 40,4);
    lv_obj_set_size(m_lable_play_name, 400, 60);
    lv_obj_set_style_text_font(m_lable_play_name, font_small, 0);
    lv_obj_set_style_text_color(m_lable_play_name, lv_color_hex(0x8a86b8), 0);
    lv_label_set_long_mode(m_lable_play_name, LV_LABEL_LONG_SCROLL_CIRCULAR);//LV_LABEL_LONG_DOT, LV_LABEL_LONG_SCROLL

    if (MSG_TYPE_CAST_DLNA_START == m_push_type)
        lv_label_set_text(m_lable_play_name, "DLNA");
    else if (MSG_TYPE_CAST_AIRCAST_START == m_push_type)
        lv_label_set_text(m_lable_play_name, "Aircast");

    //create slide bar
    m_slider_obj = lv_slider_create(m_play_bar);
    lv_obj_set_style_anim_time(m_slider_obj, 100, 0);
    lv_obj_add_flag(m_slider_obj, LV_OBJ_FLAG_CLICKABLE); /*No input from the slider*/
    lv_obj_set_size(m_slider_obj, SLIDE_W, SLIDE_H);
    lv_obj_align(m_slider_obj, LV_ALIGN_CENTER, 0, 42);

    lv_obj_set_height(m_slider_obj, 6);
    lv_obj_set_grid_cell(m_slider_obj, LV_GRID_ALIGN_STRETCH, 1, 4, LV_GRID_ALIGN_CENTER, 1, 1);

    lv_obj_set_style_bg_img_src(m_slider_obj, &img_lv_demo_music_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(m_slider_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(m_slider_obj, 30, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(m_slider_obj, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(m_slider_obj, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(m_slider_obj, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(m_slider_obj, 0, 0);

    //play/pause icon
    m_img_play = lv_img_create(m_play_bar);
    lv_img_set_src(m_img_play, &img_lv_demo_music_btn_pause);
    lv_obj_align(m_img_play, LV_ALIGN_CENTER, 0, -36);


    //create play time text
    play_time_obj = lv_label_create(m_play_bar);
    lv_obj_set_style_text_font(play_time_obj, font_small, 0);
    lv_obj_set_style_text_color(play_time_obj, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(play_time_obj, "00:00:00");
    lv_obj_set_pos(play_time_obj, 10, SLIDE_Y-4);

    //create total time text
    total_time_obj = lv_label_create(m_play_bar);
    lv_obj_set_style_text_font(total_time_obj, font_small, 0);
    lv_obj_set_style_text_color(total_time_obj, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(total_time_obj, "00:00:00");
    lv_obj_set_pos(total_time_obj, SLIDE_X+SLIDE_W+6, SLIDE_Y-4);
    //lv_obj_set_grid_cell(play_time_obj, LV_GRID_ALIGN_END, 5, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    m_label_photo_frame = lv_obj_create(parent);
    lv_obj_clear_flag(m_label_photo_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(m_label_photo_frame, 40, 40);
    lv_obj_set_size(m_label_photo_frame, 100, 40);
    lv_obj_set_style_border_opa(m_label_photo_frame, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(m_label_photo_frame, 0, 0);
    lv_obj_set_style_bg_color(m_label_photo_frame, COLOR_DEEP_GREY, 0); //grey

    m_label_photo_flag = lv_label_create(m_label_photo_frame);
    lv_obj_set_style_text_font(m_label_photo_flag, font_small, 0);
    lv_obj_align(m_label_photo_flag, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(m_label_photo_flag, lv_color_hex(0x8a86b8), 0);

    if (HCCAST_MEDIA_PHOTO == m_dlna_media_type){
        lv_label_set_text(m_label_photo_flag, "Photo");
        play_bar_onoff(false);
    } else if (HCCAST_MEDIA_MUSIC == m_dlna_media_type){
        lv_label_set_text(m_label_photo_flag, "Music");
    } else if (HCCAST_MEDIA_MOVIE == m_dlna_media_type){
        lv_obj_add_flag(m_label_photo_frame, LV_OBJ_FLAG_HIDDEN);
    }

}

static uint32_t m_hotkey[] = {KEY_POWER, KEY_VOLUMEUP, \
                    KEY_VOLUMEDOWN, KEY_MUTE, KEY_ROTATE_DISPLAY, KEY_FLIP,KEY_CAMERA_FOCUS,KEY_FORWARD,KEY_BACK,KEY_HOME};

static int win_dlna_play_open(void *arg)
{
    win_cast_set_m_stop_service_exit(1);
    printf("%s(), line: %d!\n", __func__, __LINE__);

    api_hotkey_enable_set(m_hotkey, sizeof(m_hotkey)/sizeof(m_hotkey[0]));
    api_set_display_aspect(DIS_TV_16_9, DIS_PILLBOX);

#ifdef KEY_EVENT_SUPPORT    
    //regist key device to group, so that the object in the group can 
    //get the key event.    
    m_player_group = lv_group_create();
    m_player_group->auto_focus_dis = 1;
    key_set_group(m_player_group);

#endif

    //uint32_t param = (uint32_t)arg;
    uint32_t param = win_cast_play_param();
    
    api_osd_off_time(1000);
	
    m_dlna_media_type = (hccast_media_type_e)(param & 0xFFFF);
    m_push_type = (param >> 16) & 0xFFFF;
    if (HCCAST_MEDIA_MUSIC == m_dlna_media_type){
        api_logo_show(MUSIC_LOGO);
    }

    //create_win_player(lv_scr_act());
    
    // printf("%s(), line: %d!, lv_scr_act: 0x%x, ui_cast_dlna: 0x%x\n", 
    //     __func__, __LINE__, lv_scr_act(), ui_cast_dlna);
    
    create_win_player(ui_cast_dlna);

#ifdef KEY_EVENT_SUPPORT    
    lv_obj_add_event_cb(m_play_bar, event_handler, LV_EVENT_KEY, NULL); 
    lv_group_add_obj(m_player_group, m_play_bar);
    lv_group_focus_obj(m_play_bar);
#endif

    if (HCCAST_MEDIA_MOVIE == m_dlna_media_type ||
        HCCAST_MEDIA_MUSIC == m_dlna_media_type){
       bar_show_timer = lv_timer_create(bar_show_timer_cb, PLAY_BAR_HIDE_TIME, NULL);
       sec_counter_timer = lv_timer_create(sec_timer_cb, 1000, NULL);
    }

    win_data_buffing_open(ui_cast_dlna);

    //extern void api_set_i2so_gpio_mute_auto(void);
    api_set_i2so_gpio_mute_auto();

    api_ffmpeg_player_get_regist(hccast_media_player_get);

    m_win_dlna_open = true;
	return API_SUCCESS;
}

static int win_dlna_play_close(void *arg)
{
    (void)arg;
    win_volume_close(NULL);

    if (win_exit_to_cast_root_by_key_get()){
        hccast_media_stop_by_key();
        api_sleep_ms(100);
    }    

    if (HCCAST_MEDIA_MOVIE == m_dlna_media_type ||
        HCCAST_MEDIA_MUSIC == m_dlna_media_type)    
    {
        lv_timer_pause(sec_counter_timer);
        lv_timer_del(sec_counter_timer);

        lv_timer_pause(bar_show_timer);
        lv_timer_del(bar_show_timer);

        bar_show_timer = NULL;
        sec_counter_timer = NULL;
    }

    if (m_player_group){
        lv_group_remove_all_objs(m_player_group);
        lv_group_del(m_player_group);
        lv_group_set_default(NULL);
        m_player_group = NULL;
    }

    win_msgbox_msg_close();

    lv_obj_del(m_label_photo_frame);
    lv_obj_del(m_play_bar);
	api_logo_off();
    m_label_photo_frame = NULL;
    m_play_bar = NULL;

    win_data_buffing_close();
    m_win_dlna_open = false;

    api_ffmpeg_player_get_regist(NULL);

    printf("%s(), line: %d!\n", __func__, __LINE__);

    api_hotkey_disable_clear();
    api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);
	return API_SUCCESS;
}


static void win_dlna_play_bar_show(bool always_show)
{
    play_bar_onoff(true);
    lv_timer_reset(bar_show_timer);
    if (always_show)
        lv_timer_pause(bar_show_timer);
    else
        lv_timer_resume(bar_show_timer);
    
}

#define INTERVAL_JUMP_MAX  (300*1000) //300 seconds
#define INTERVAL_JUMP_STEP  (10*1000) //10 seconds
#define INTERVAL_JUMP_ADD  (5*1000) //5 seconds, if long press, adding jump interval.
static void win_dlna_key_seek_do(uint32_t key)
{
    static uint32_t dlna_interval = INTERVAL_JUMP_STEP; 
    uint32_t total_time = hccast_media_get_duration();
    uint32_t play_time = hccast_media_get_position();
    uint64_t jump_pos = 0; //20s
    static uint32_t tick_last = 0;

    if (0 == total_time || play_time >= total_time)
        return;

    if (tick_last && (api_sys_tick_get()-tick_last) < 800)
        dlna_interval += INTERVAL_JUMP_ADD;
    else
        dlna_interval = INTERVAL_JUMP_STEP;

    tick_last = api_sys_tick_get();

    if (dlna_interval > INTERVAL_JUMP_MAX)
        dlna_interval = INTERVAL_JUMP_MAX;

    if (key == V_KEY_LEFT){
    //seek backward
        if (play_time < dlna_interval){
            jump_pos =  1000;
        }
        else{
            jump_pos = (uint64_t)(play_time-dlna_interval);
        }

        hccast_media_seek_by_key(jump_pos);
    } else {
    //seek forward
        if (play_time+dlna_interval >= total_time){
            jump_pos = total_time-1000;
        }
        else{
            jump_pos = (uint64_t)(play_time+dlna_interval);
        }
        
        hccast_media_seek_by_key(jump_pos);
    }
}

static void win_dlna_play_pause(void)
{
    int media_status = -1; 

    if (HCCAST_MEDIA_MOVIE != m_dlna_media_type && 
        HCCAST_MEDIA_MUSIC != m_dlna_media_type)
        return;

    media_status = hccast_media_get_status();
    if (HCCAST_MEDIA_STATUS_PAUSED == media_status){
        hccast_media_resume_by_key();
        lv_img_set_src(m_img_play, &img_lv_demo_music_btn_play);
        win_dlna_play_bar_show(false);
    } else if (HCCAST_MEDIA_STATUS_PLAYING == media_status){
        hccast_media_pause_by_key();
        lv_img_set_src(m_img_play, &img_lv_demo_music_btn_pause);
        win_dlna_play_bar_show(true);
    }    

}
static void win_dlna_key_act(uint32_t key)
{
#ifdef KEY_EVENT_SUPPORT 

    control_msg_t ctl_msg = {0};   
    int media_status = -1; 

    //TEST TEST just test, because IR key press 1 times, but 
    //transfter 2 key.
    static int mute_count = 0;

    media_status = hccast_media_get_status();
    switch (key)
    {
    case V_KEY_EXIT:
    case V_KEY_MENU:
        // ret = WIN_CTL_POPUP_CLOSE;
        break;
  #if 1        
    //test UI

    case V_KEY_PLAY:
        if (HCCAST_MEDIA_MOVIE == m_dlna_media_type ||
            HCCAST_MEDIA_MUSIC == m_dlna_media_type){
            lv_img_set_src(m_img_play, &img_lv_demo_music_btn_play);
            win_dlna_play_bar_show(false);
            if (HCCAST_MEDIA_STATUS_PAUSED == media_status){
                hccast_media_resume_by_key();
            }
        }
        break;
    case V_KEY_PAUSE:
        if (HCCAST_MEDIA_MOVIE == m_dlna_media_type ||
            HCCAST_MEDIA_MUSIC == m_dlna_media_type){
            lv_img_set_src(m_img_play, &img_lv_demo_music_btn_pause);
            win_dlna_play_bar_show(true);
            if (HCCAST_MEDIA_STATUS_PLAYING == media_status){
                hccast_media_pause_by_key();
            }
        }
        break;
    case V_KEY_LEFT: //for seek
    case V_KEY_RIGHT: //for seek
        if (HCCAST_MEDIA_MOVIE == m_dlna_media_type ||
            HCCAST_MEDIA_MUSIC == m_dlna_media_type){
            win_dlna_play_bar_show(false);
            if (HCCAST_MEDIA_STATUS_PAUSED == media_status ||
                HCCAST_MEDIA_STATUS_PLAYING == media_status){
                win_dlna_key_seek_do(key);
            }
        }
        break;
    case V_KEY_ENTER:
    #if 1
        win_dlna_play_pause();
    #else
       if (HCCAST_MEDIA_MOVIE == m_dlna_media_type ||
           HCCAST_MEDIA_MUSIC == m_dlna_media_type){
            win_dlna_play_bar_show(false);
        }
    #endif
        break;

    case V_KEY_V_UP:
    case V_KEY_V_DOWN:
        win_volume_set((key << 16), 1);
        break;

#ifndef MUTE_CONTROL_BY_SYS        
    case V_KEY_MUTE:
        if (mute_count++ > 0){
            win_mute_on_off(true);
            mute_count = 0;
        }
        break;
#endif

//////////////////////////////////////
/// simulate the medie error code process
    case 1:
        ctl_msg.msg_type = MSG_TYPE_MEDIA_VIDEO_DECODER_ERROR;
        api_control_send_msg(&ctl_msg);
        break;
    case 2:
        ctl_msg.msg_type = MSG_TYPE_MEDIA_AUDIO_DECODER_ERROR;
        api_control_send_msg(&ctl_msg);
        break;
    case 3:
        ctl_msg.msg_type = MSG_TYPE_MEDIA_VIDEO_NOT_SUPPORT;
        ctl_msg.msg_code = HC_AVCODEC_ID_HEVC;
        api_control_send_msg(&ctl_msg);
        break;
    case 4:
        ctl_msg.msg_type = MSG_TYPE_MEDIA_AUDIO_NOT_SUPPORT;
        ctl_msg.msg_code = HC_AVCODEC_ID_EAC3;
        api_control_send_msg(&ctl_msg);
        break;
    case 5:
        ctl_msg.msg_type = MSG_TYPE_MEDIA_NOT_SUPPORT;
        api_control_send_msg(&ctl_msg);
        break;
//////////////////////////////////////
///
  #endif
    }
#endif
}

static void meida_play_infor_update(hccast_media_type_e old_type, hccast_media_type_e new_type)
{
    if (HCCAST_MEDIA_MOVIE == old_type){
        if (HCCAST_MEDIA_MUSIC == new_type){
            lv_obj_clear_flag(m_label_photo_frame, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(m_label_photo_flag, "Music");
            api_logo_show(MUSIC_LOGO);
        } else if (HCCAST_MEDIA_PHOTO == new_type){
            lv_obj_clear_flag(m_label_photo_frame, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(m_label_photo_flag, "Photo");
        }
    } else if (HCCAST_MEDIA_MUSIC == old_type){
        if (HCCAST_MEDIA_MOVIE == new_type){
            api_logo_off();
            lv_obj_add_flag(m_label_photo_frame, LV_OBJ_FLAG_HIDDEN);
        } else if (HCCAST_MEDIA_PHOTO == new_type){
            api_logo_off();
            lv_obj_clear_flag(m_label_photo_frame, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(m_label_photo_flag, "Photo");
        }
    } else if (HCCAST_MEDIA_PHOTO == old_type){
        if (HCCAST_MEDIA_MOVIE == new_type){
            lv_obj_add_flag(m_label_photo_frame, LV_OBJ_FLAG_HIDDEN);
        } else if (HCCAST_MEDIA_MUSIC == new_type){
            lv_obj_clear_flag(m_label_photo_frame, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(m_label_photo_flag, "Music");
            api_logo_show(MUSIC_LOGO);
        } 
    }
}


static int media_code_id_get(char *code_str, int code_id)
{

    if (code_id >= HC_AVCODEC_ID_FIRST_AUDIO && code_id < HC_AVCODEC_ID_ADPCM_IMA_QT){
        strcpy(code_str, "pcm");
        return API_SUCCESS;
    }
    else if (code_id >= HC_AVCODEC_ID_ADPCM_IMA_QT && code_id < HC_AVCODEC_ID_AMR_NB){
        strcpy(code_str, "adpcm");
        return API_SUCCESS;
    }

    switch (code_id)
    {
    case HC_AVCODEC_ID_HEVC:
        strcpy(code_str, "h265");
        break;
    case HC_AVCODEC_ID_VP9:
        strcpy(code_str, "vp9");
        break;
    case HC_AVCODEC_ID_AMV:
        strcpy(code_str, "amv");
        break;
    case HC_AVCODEC_ID_DTS:
        strcpy(code_str, "dts");
        break;
    case HC_AVCODEC_ID_EAC3:
        strcpy(code_str, "eac3");
        break;
    case HC_AVCODEC_ID_APE:
        strcpy(code_str, "ape");
        break;

    default:
        strcpy(code_str, "unknown");
        break;
    }
    return API_SUCCESS;
}

static void msgbox_exit_func(void *user_data)
{
    if(hccast_media_get_status() != HCCAST_MEDIA_STATUS_STOP)
        return;
        
    (void)user_data;
    control_msg_t msg = {0};
    msg.msg_type = MSG_TYPE_CLOSE_WIN;
    api_control_send_msg(&msg);

    printf("%s(): exit dlna play!\n", __func__);

}

static void win_dlna_msg_act(control_msg_t *ctl_msg)
{
    hccast_media_type_e dlna_media_type_old = m_dlna_media_type;
    uint32_t value;

    switch ((msg_type_t)ctl_msg->msg_type)
    {
    case MSG_TYPE_CAST_DLNA_STOP:
        // ret = WIN_CTL_POPUP_CLOSE;
        
        _ui_screen_change(ui_wifi_cast_root,0,0);
        break;
    case MSG_TYPE_CAST_DLNA_PLAY:
    case MSG_TYPE_CAST_DLNA_SEEK:
        if (HCCAST_MEDIA_MOVIE == m_dlna_media_type ||
            HCCAST_MEDIA_MUSIC == m_dlna_media_type){
            if (MSG_TYPE_CAST_DLNA_PLAY == (msg_type_t)ctl_msg->msg_type )
                lv_img_set_src(m_img_play, &img_lv_demo_music_btn_play);
            win_dlna_play_bar_show(false);
        }
        break;
    case MSG_TYPE_CAST_DLNA_PAUSE:
        if (HCCAST_MEDIA_MOVIE == m_dlna_media_type ||
            HCCAST_MEDIA_MUSIC == m_dlna_media_type){
            lv_img_set_src(m_img_play, &img_lv_demo_music_btn_pause);
            win_dlna_play_bar_show(true);
        }
        break;
    case MSG_TYPE_CAST_DLNA_VOL_SET:
        win_volume_set((uint8_t)ctl_msg->msg_code, 1);
        break;
    case MSG_TYPE_CAST_DLNA_MUTE:
    #ifdef MUTE_CONTROL_BY_SYS        
        api_key_queue_send(KEY_MUTE);
    #else
        win_mute_on_off(true);
    #endif        
        break;

    case MSG_TYPE_CAST_DLNA_START:
    case MSG_TYPE_CAST_AIRCAST_START:
        win_msgbox_msg_close();
        
        m_dlna_media_type = (hccast_media_type_e)(ctl_msg->msg_code);

        if (HCCAST_MEDIA_MOVIE == m_dlna_media_type ||
            HCCAST_MEDIA_MUSIC == m_dlna_media_type){
            lv_img_set_src(m_img_play, &img_lv_demo_music_btn_play);
            play_bar_onoff(true);

            if (NULL == bar_show_timer){
                bar_show_timer = lv_timer_create(bar_show_timer_cb, PLAY_BAR_HIDE_TIME, NULL);
                sec_counter_timer = lv_timer_create(sec_timer_cb, 1000, NULL);
            }

            lv_timer_reset(bar_show_timer);
            lv_timer_resume(bar_show_timer);
        }
        meida_play_infor_update(dlna_media_type_old, m_dlna_media_type);
        win_data_buffing_open(ui_cast_dlna);

        break;
    case MSG_TYPE_MEDIA_BUFFERING:
        value = ctl_msg->msg_code;
        win_data_buffing_update(value);
        break;

    case MSG_TYPE_MEDIA_VIDEO_DECODER_ERROR:
        win_msgbox_msg_open(STR_VIDEO_DATA_ERR, 0, NULL, NULL);
        break;
    case MSG_TYPE_MEDIA_AUDIO_DECODER_ERROR:
        //show messeage for some time then close. because video may playing normal.
        win_msgbox_msg_open(STR_AUDIO_DATA_ERR, 10000, NULL, NULL);
        break;
    case MSG_TYPE_MEDIA_VIDEO_NOT_SUPPORT:
    {
        char code_id_str[32] = {0};
        value = ctl_msg->msg_code;
        media_code_id_get(code_id_str, value);
        printf("Video format(%s) not support!", code_id_str);
        win_msgbox_msg_open(STR_VIDEO_FORMAT_NOT_SUPPORT, 0, NULL, NULL);
        printf("%s(), line:%d. Video format(%s/%d) not support!", __func__, __LINE__,
            code_id_str, (int)value);
        break;
    }
    case MSG_TYPE_MEDIA_AUDIO_NOT_SUPPORT:
    {
        char code_id_str[32] = {0};
        value = ctl_msg->msg_code;
        media_code_id_get(code_id_str, value);
        win_msgbox_msg_open(STR_AUDIO_FORMAT_NOT_SUPPORT, 10000, NULL, NULL);
        printf("%s(), line:%d. Audio format(%s/%d) not support!", __func__, __LINE__,
            code_id_str, (int)value);
        break;
    }
    case MSG_TYPE_NETWORK_WIFI_DISCONNECTED:
        //show message for 5 seconds then exit the UI
        win_msgbox_msg_open(STR_WIFI_DISCONNECT, 5000, msgbox_exit_func, NULL);
        break;

    case MSG_TYPE_MEDIA_NOT_SUPPORT:
        //show message for 5 seconds then exit the UI
        win_msgbox_msg_open(STR_MEDIA_CONTAINER_NOT_SUPPORT, 5000, msgbox_exit_func, NULL);
        break;
    case MSG_TYPE_CLOSE_WIN:
        _ui_screen_change(ui_wifi_cast_root,0,0);

        break;
    default:
        break;
    }
}

static void win_dlna_play_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

    if (MSG_TYPE_KEY == ctl_msg->msg_type){
        win_dlna_key_act(ctl_msg->msg_code);
    }else{
	    win_dlna_msg_act(ctl_msg);
    }
}

static bool win_dlna_wait_open(uint32_t timeout)
{
    uint32_t count;
    count = timeout/20;

    while(count--){
        if (m_win_dlna_open)
            break;
        api_sleep_ms(20);
    }
    printf("%s(), m_win_dlna_open(%d):%d\n", __func__, m_win_dlna_open, (int)count);
    return m_win_dlna_open; 
}


void ui_cast_dlna_init(void)
{
    screen_entry_t dlna_entry;

    ui_cast_dlna = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_cast_dlna, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_cast_dlna, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_opa(ui_cast_dlna, LV_OPA_TRANSP, 0);

    dlna_entry.screen = ui_cast_dlna;
    dlna_entry.control = win_dlna_play_control;
    api_screen_regist_ctrl_handle(&dlna_entry);
    cast_dlna_ui_wait_init(win_dlna_wait_open);
}


// win_des_t g_win_dlna_play =
// {
//     .open = win_dlna_play_open,
//     .close = win_dlna_play_close,
//     .control = win_dlna_play_control,
// };


#endif