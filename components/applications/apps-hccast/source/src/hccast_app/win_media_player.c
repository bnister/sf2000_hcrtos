/**
 * win_media_player.c, use to play media files(URL, USB, SD etc)
 * win_media_list.c, use to list media files(USB)
 */


#ifdef __linux__
  #include <sys/msg.h>
  #include <termios.h>
  #include <poll.h>
#else
  #include <freertos/FreeRTOS.h>
  #include <freertos/task.h>
  #include <freertos/semphr.h>
  #include <freertos/queue.h>
  #include <kernel/lib/console.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include <ffplayer.h>
#include <hcuapi/codec_id.h>

#include "media_player.h"
#include "file_mgr.h"

#include "obj_mgr.h"
#include "os_api.h"
#include "com_api.h"
#include "menu_mgr.h"
#include "osd_com.h"
#include "key.h"
#include "win_media_list.h"
#include "win_mute.h"
#include "win_vol.h"

#define PLAYER_BAR_H    160
#define PLAYER_BAR_W    (OSD_MAX_WIDTH-80)
#define PLAYER_BAR_X    (OSD_MAX_WIDTH-PLAYER_BAR_W)/2
#define PLAYER_BAR_Y    (OSD_MAX_HEIGHT-PLAYER_BAR_H-20)

#define SLIDE_W    (PLAYER_BAR_W - 300)
#define SLIDE_H    50
#define SLIDE_X    (PLAYER_BAR_W - SLIDE_W)/2
#define SLIDE_Y    90

#define PLAY_BAR_HIDE_TIME  (5000) //the playbar would hide after 10 seconds if no key

static lv_obj_t *m_play_bar;
/*static lv_obj_t *m_lable_total_time;*/
static lv_obj_t *m_lable_play_name;
static lv_obj_t *m_slider_obj;
static lv_obj_t * play_time_obj;
static lv_obj_t * total_time_obj;
static lv_obj_t *m_img_play;
static lv_obj_t *m_label_speed;

static const lv_font_t *font_small;
static const lv_font_t *font_large;

static lv_group_t *m_player_group  = NULL;
static lv_timer_t *sec_counter_timer;
static lv_timer_t *bar_show_timer;

static bool m_play_bar_show = true;
static   char *m_play_path_name;
static   char *m_play_file_name;

static media_handle_t *m_player_hld[MEDIA_TYPE_COUNT] = {NULL,};
static media_handle_t *m_cur_media_hld = NULL;

pthread_mutex_t m_player_task_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MEDIA_IS_VIDEO() (m_cur_media_hld->type == MEDIA_TYPE_VIDEO)

LV_IMG_DECLARE(img_lv_demo_music_slider_knob)
LV_IMG_DECLARE(img_lv_demo_music_btn_play)
LV_IMG_DECLARE(img_lv_demo_music_btn_pause)

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
    lv_event_code_t code = lv_event_get_code(e);
//    lv_obj_t * obj = lv_event_get_target(e);
    uint32_t vkey = VKEY_NULL;
//    lv_obj_t * cur_obj = NULL;

    if(code == LV_EVENT_KEY){
        uint32_t value = lv_indev_get_key(lv_indev_get_act());
        vkey = key_convert_vkey(value);
        if (!key_filter(vkey))
            return;

        printf("media vkey: %lu...\n", vkey);
        api_control_send_key(vkey);
    }
}

static void format_time(uint32_t time, char *time_fmt)
{
    uint32_t hour;
    uint32_t min;
    uint32_t second;

    if (0 == time){
        sprintf(time_fmt, "00:00:00");
        return;
    }

    hour = time / 3600;
    min = (time % 3600) / 60;
    second = time % 60;
    if (hour > 0)
        sprintf(time_fmt, "%02lu:%02lu:%02lu", hour, min, second);
    else
        sprintf(time_fmt, "%02lu:%02lu", min, second);

}

static void sec_timer_cb(lv_timer_t * t)
{
    uint32_t play_time = 0;
    uint32_t total_time = 0;
    char time_fmt[16];

    play_time = media_get_playtime(m_cur_media_hld);
    format_time(play_time, time_fmt);
    lv_label_set_text(play_time_obj, time_fmt);
    lv_slider_set_value(m_slider_obj, play_time, LV_ANIM_ON);

    total_time = media_get_totaltime(m_cur_media_hld);
    format_time(total_time, time_fmt);
    //printf("total time: %s\n", time_fmt);
    lv_label_set_text(total_time_obj, time_fmt);
    if (total_time > 0)
        lv_slider_set_range(m_slider_obj, 0, total_time);
}

static void show_play_bar(bool show)
{
    // if (m_play_bar_show == show)
    //     return;

    if (show){
        lv_obj_clear_flag(m_play_bar, LV_OBJ_FLAG_HIDDEN);
    }else{
        lv_obj_add_flag(m_play_bar, LV_OBJ_FLAG_HIDDEN);
    }
    printf("%s(), open show: %d\n", __FUNCTION__, show);
    m_play_bar_show = show;

}

static void bar_show_timer_cb(lv_timer_t * t)
{
    show_play_bar(false);    lv_timer_pause(bar_show_timer);
}


static void create_win_player(lv_obj_t *parent)
{
    static lv_style_t m_bar_style;


    m_play_bar_show = true;
    font_small = &lv_font_montserrat_22;
    font_large = &lv_font_montserrat_32;

    lv_style_init(&m_bar_style);

    //create bar frame
    m_play_bar = lv_obj_create(parent);
    lv_obj_clear_flag(m_play_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_set_pos(m_play_bar, PLAYER_BAR_X, PLAYER_BAR_Y);
    lv_obj_set_size(m_play_bar, PLAYER_BAR_W, PLAYER_BAR_H);

    //lv_obj_set_style_bg_opa(parent, LV_OPA_10, 0);
    //lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, 50);
    lv_obj_set_style_bg_color(m_play_bar, COLOR_DEEP_GREY, LV_PART_MAIN); //grey

    lv_obj_add_event_cb(m_play_bar, event_handler, LV_EVENT_ALL, NULL); 
    lv_group_add_obj(m_player_group, m_play_bar);

    //create file name label
    m_lable_play_name = lv_label_create(m_play_bar);
    lv_obj_set_pos(m_lable_play_name, 40,4);
    lv_obj_set_size(m_lable_play_name, 400, 60);
    lv_obj_set_style_text_font(m_lable_play_name, font_small, LV_PART_MAIN);
    lv_obj_set_style_text_color(m_lable_play_name, lv_color_hex(0x8a86b8), LV_PART_MAIN);
    lv_label_set_long_mode(m_lable_play_name, LV_LABEL_LONG_SCROLL_CIRCULAR);//LV_LABEL_LONG_DOT, LV_LABEL_LONG_SCROLL
    m_play_file_name = win_media_get_cur_file_name();
    lv_label_set_text(m_lable_play_name, m_play_file_name);

    //create slide bar
    m_slider_obj = lv_slider_create(m_play_bar);
    lv_obj_set_style_anim_time(m_slider_obj, 100, 0);
    lv_obj_add_flag(m_slider_obj, LV_OBJ_FLAG_CLICKABLE); /*No input from the slider*/
    lv_obj_set_size(m_slider_obj, SLIDE_W, SLIDE_H);
    lv_obj_align(m_slider_obj, LV_ALIGN_CENTER, 0, 40);

    lv_obj_set_height(m_slider_obj, 6);
    lv_obj_set_grid_cell(m_slider_obj, LV_GRID_ALIGN_STRETCH, 1, 4, LV_GRID_ALIGN_CENTER, 1, 1);

    lv_obj_set_style_bg_img_src(m_slider_obj, &img_lv_demo_music_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(m_slider_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(m_slider_obj, 20, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(m_slider_obj, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(m_slider_obj, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(m_slider_obj, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(m_slider_obj, 0, 0);

    //play/pause icon
    m_img_play = lv_img_create(m_play_bar);
    lv_img_set_src(m_img_play, &img_lv_demo_music_btn_pause);
    lv_obj_align(m_img_play, LV_ALIGN_CENTER, 0, -36);

    //play speed
    m_label_speed = lv_label_create(m_play_bar);
    lv_obj_set_pos(m_label_speed, PLAYER_BAR_W-160,4);
    lv_obj_set_style_text_font(m_label_speed, font_small, 0);
    lv_obj_set_style_text_color(m_label_speed, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(m_label_speed, "");

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

    sec_counter_timer = lv_timer_create(sec_timer_cb, 1000, NULL);
}


static int win_media_player_open(void *arg)
{
    file_list_t *file_list = NULL;

    if (NULL == arg){
        printf("%s(), line:%d. no play name!\n", __FUNCTION__, __LINE__);
        return API_FAILURE;
    }

    file_list = win_media_get_cur_list();

    m_cur_media_hld = m_player_hld[file_list->media_type];
    if (NULL == m_cur_media_hld){
        m_cur_media_hld = media_open(file_list->media_type);
        m_player_hld[file_list->media_type] = m_cur_media_hld;
    }

    m_play_path_name = (char*)arg;

    //media_play(m_cur_media_hld, "/media/hdd/video/jingqi_duizhang.mp4");
    media_play(m_cur_media_hld, m_play_path_name);

    //regist key device to group, so that the object in the group can 
    //get the key event.    
    m_player_group = lv_group_create();

    //user need get the LV_KEY_NEXT and LV_KEY_PREV key, so disable auto focus.
    m_player_group->auto_focus_dis = 1;
    key_regist(m_player_group);

    create_win_player(lv_scr_act());

    if (MEDIA_IS_VIDEO())
        bar_show_timer = lv_timer_create(bar_show_timer_cb, PLAY_BAR_HIDE_TIME, NULL);



    return API_SUCCESS;
}

static int win_media_player_close(void *arg)
{
    file_list_t *file_list = NULL;
    win_volume_close(NULL);

    lv_timer_pause(sec_counter_timer);
    lv_timer_del(sec_counter_timer);

    if (MEDIA_IS_VIDEO()){
        lv_timer_pause(bar_show_timer);
        lv_timer_del(bar_show_timer);
    }

    lv_group_remove_all_objs(m_player_group);
    lv_group_del(m_player_group);
    lv_obj_del(m_play_bar);

    file_list = win_media_get_cur_list();
    if(m_cur_media_hld->state!=MEDIA_STOP)
        media_stop(m_cur_media_hld);
    media_close(m_cur_media_hld);
    m_player_hld[file_list->media_type] = NULL;
    m_cur_media_hld = NULL;

    return API_SUCCESS;
}

static char *m_str_ff[] = {"", ">> x2", " >> x4", ">> x8", ">> x16", ">> x24", ">> x32"};
static char *m_str_fb[] = {"", "<< x2", " << x4", "<< x8", "<< x16", "<< x24", "<< x32"};
static char *m_str_sf[] = {"", ">> 1/2", " >> 1/4", ">> 1/8", ">> 1/16", ">> 1/24"};
static char *m_str_sb[] = {"", "<< 1/2", " << 1/4", "<< 1/8", "<< 1/16", "<< 1/24"};
static void win_media_reflesh_speed(void)
{
    media_state_t play_state;
    uint8_t speed = 0;
    char **str_speed = NULL;

    play_state = media_get_state(m_cur_media_hld);
    speed = media_get_speed(m_cur_media_hld);
    if (0 == speed){
        lv_label_set_text(m_label_speed, ""); 
        return;
    }

    if (MEDIA_FF == play_state){
        str_speed = m_str_ff;
    }else if (MEDIA_FB == play_state){
        str_speed = m_str_fb;
    }else if (MEDIA_SF == play_state){
        str_speed = m_str_sf;
    }else if (MEDIA_SB == play_state){
        str_speed = m_str_sb;
    }

    lv_label_set_text(m_label_speed, str_speed[speed]);    
}


static void __media_seek_proc(uint32_t key)
{
    uint32_t total_time = media_get_totaltime(m_cur_media_hld);
    uint32_t play_time = media_get_playtime(m_cur_media_hld);
    uint32_t jump_interval = 10;
    uint32_t seek_time = 0;

    if (total_time < jump_interval)
        return;

    if (V_KEY_LEFT == key){//seek backward
        if (play_time > jump_interval)
            seek_time = play_time - jump_interval;
        else
            seek_time = 0;
    }else{ //seek forward
        if ((play_time + jump_interval) > total_time)
            seek_time = total_time;
        else
            seek_time = play_time + jump_interval;
    }
    printf("%s(), line: %d. seek_time: %d\n", __func__, __LINE__, seek_time);
    media_seek(m_cur_media_hld, seek_time);

}

static win_ctl_result_t win_media_key_act(uint32_t key)
{
    win_ctl_result_t ret = WIN_CTL_DONE;
    media_state_t play_state;
    file_list_t *file_list = NULL;
    char *play_name = NULL;

    play_state = media_get_state(m_cur_media_hld);

    switch (key)
    {
    case V_KEY_ENTER:
        if (MEDIA_PLAY == play_state){
            media_pause(m_cur_media_hld);
        }else if (MEDIA_STOP == play_state){
            media_play(m_cur_media_hld, m_cur_media_hld->play_name);
        }
        else{
            media_resume(m_cur_media_hld);
        }
        break;
    case V_KEY_PLAY:
        if (MEDIA_STOP == play_state){
            media_play(m_cur_media_hld, m_cur_media_hld->play_name);   
        } else if(MEDIA_PLAY != play_state){
            media_resume(m_cur_media_hld);    
        }
        break;
    case V_KEY_PAUSE:
        if (MEDIA_STOP == play_state)
            return ret;

        if (MEDIA_PAUSE == play_state){
            //step ???
        }else{
            media_pause(m_cur_media_hld); 
        }
        break;
    case V_KEY_FF:
        if (MEDIA_STOP != play_state)
            media_fastforward(m_cur_media_hld);
        break;
    case V_KEY_FB:
        if (MEDIA_STOP != play_state)
            media_fastbackward(m_cur_media_hld);
        break;
    case V_KEY_SF:
        if (MEDIA_STOP != play_state)
            media_slowforward(m_cur_media_hld);
        break;
    case V_KEY_SB:
        if (MEDIA_STOP != play_state)
            media_fastbackward(m_cur_media_hld);
        break;
    case V_KEY_LEFT:
    case V_KEY_RIGHT:
        if (MEDIA_STOP != play_state)
            __media_seek_proc(key);
        break;
    case V_KEY_V_UP:
    case V_KEY_V_DOWN:
        win_volume_set((key << 16), 1);
        return ret;
    case V_KEY_MUTE:
        win_mute_on_off(true);
        return ret;
    case V_KEY_PREV:
    case V_KEY_NEXT:
        file_list = win_media_get_cur_list();
        if (V_KEY_PREV == key)
            play_name = win_media_get_pre_file(file_list);
        else
            play_name = win_media_get_next_file(file_list);
        if (play_name){
            if (MEDIA_STOP != play_state)
                media_stop(m_cur_media_hld);
            media_play(m_cur_media_hld, play_name);
            m_play_file_name = win_media_get_cur_file_name();
            lv_label_set_text(m_lable_play_name, m_play_file_name);
        }
        break;
    case V_KEY_STOP:
        if (MEDIA_STOP != play_state)
            media_stop(m_cur_media_hld);
        break;
    case V_KEY_EXIT:
        if (MEDIA_IS_VIDEO() && m_play_bar_show){
            show_play_bar(false);
            return ret;
        }else{
            if (MEDIA_STOP != play_state){
                media_stop(m_cur_media_hld);
            }
            ret = WIN_CTL_POPUP_CLOSE;
            return ret;
        }
        break;
    case V_KEY_MENU:
        if (MEDIA_IS_VIDEO() && m_play_bar_show){
            show_play_bar(false);
            return ret;
        } else {
            if (MEDIA_STOP != play_state)
                media_stop(m_cur_media_hld);
            ret = WIN_CTL_POPUP_CLOSE;   
            return ret;
        }
        break;
    default:
        break;
    }


    play_state = media_get_state(m_cur_media_hld);
    if (MEDIA_PAUSE == play_state || MEDIA_STOP == play_state){
        lv_img_set_src(m_img_play, &img_lv_demo_music_btn_play);
    }else{
        lv_img_set_src(m_img_play, &img_lv_demo_music_btn_pause);
    }

    win_media_reflesh_speed();
    if (MEDIA_IS_VIDEO()){
        show_play_bar(true);
        lv_timer_reset(bar_show_timer);
        if (MEDIA_PLAY != play_state && MEDIA_STOP != play_state)
            lv_timer_pause(bar_show_timer);
        else{
            lv_timer_resume(bar_show_timer);
        }
    }


    return ret;
}

static win_ctl_result_t win_media_msg_act(control_msg_t *ctl_msg)
{
    win_ctl_result_t ret = WIN_CTL_NONE;

    if(ctl_msg->msg_type == MSG_TYPE_USB_DISK_UMOUNT)
        ret = WIN_CTL_POPUP_CLOSE;
    return ret;
}


static win_ctl_result_t win_media_player_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;

    if (MSG_TYPE_KEY == ctl_msg->msg_type){
        ret = win_media_key_act(ctl_msg->msg_code);
    }else{
        ret = win_media_msg_act(ctl_msg);
    }

    return ret;

}

win_des_t g_win_media_player =
{
    .open = win_media_player_open,
    .close = win_media_player_close,
    .control = win_media_player_control,
};


void media_play_test(void)
{
    if (!m_cur_media_hld)
        m_cur_media_hld = media_open(MEDIA_TYPE_VIDEO);
    media_play(m_cur_media_hld, "/media/hdd/video/jingqi_duizhang.mp4");
}

