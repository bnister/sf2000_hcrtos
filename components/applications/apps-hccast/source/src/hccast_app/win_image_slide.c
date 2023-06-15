/**
 * win_image_slide.c, use to play image files in slide show mode(URL, USB, SD etc)
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
//#include "win_mute.h"


#define IMAGE_SLIDE_WIN_H    40
#define IMAGE_SLIDE_WIN_W    500
#define IMAGE_SLIDE_WIN_X    10
#define IMAGE_SLIDE_WIN_Y    10


#define IMAGE_FILE_NAME_H    34
#define IMAGE_FILE_NAME_W    340
#define IMAGE_FILE_NAME_X    (IMAGE_SLIDE_WIN_W-100)
#define IMAGE_FILE_NAME_Y    ((IMAGE_SLIDE_WIN_H-IMAGE_FILE_NAME_H)>>1)


static media_handle_t *m_image_hld = NULL;
static lv_group_t *m_image_group = NULL;
static lv_obj_t *m_image_slide_root = NULL;
static lv_obj_t *m_lable_play_name = NULL;
static lv_obj_t *m_label_file_count = NULL;

static uint32_t valid_key_map[] = {
    V_KEY_UP,
    V_KEY_DOWN,
    V_KEY_LEFT,
    V_KEY_RIGHT,
    V_KEY_ENTER,
    V_KEY_PLAY,
    V_KEY_PAUSE,
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

static void create_slide_win(void)
{
    static lv_style_t m_text_style_left;
    static lv_style_t m_text_style_right;
    lv_style_init(&m_text_style_left);    
    lv_style_set_text_align(&m_text_style_left, LV_TEXT_ALIGN_LEFT);
    lv_style_init(&m_text_style_right);    
    lv_style_set_text_align(&m_text_style_right, LV_TEXT_ALIGN_RIGHT);

    m_image_slide_root = lv_obj_create(lv_scr_act());
    lv_obj_set_pos(m_image_slide_root, IMAGE_SLIDE_WIN_X, IMAGE_SLIDE_WIN_Y);
    lv_obj_set_size(m_image_slide_root, IMAGE_SLIDE_WIN_W, IMAGE_SLIDE_WIN_H);
    lv_obj_clear_flag(m_image_slide_root, LV_OBJ_FLAG_SCROLLABLE);


    //lv_obj_set_style_bg_opa(m_image_slide_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(m_image_slide_root, COLOR_DEEP_GREY, LV_PART_MAIN);
    lv_obj_set_style_border_opa(m_image_slide_root, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_radius(m_image_slide_root, 0, 0);

    lv_obj_add_event_cb(m_image_slide_root, event_handler, LV_EVENT_ALL, NULL); 
    lv_group_add_obj(m_image_group, m_image_slide_root);


    m_lable_play_name = lv_label_create(m_image_slide_root);
    lv_obj_add_style(m_lable_play_name, TEXT_STY_LEFT_NORMAL, 0); 
    lv_obj_set_size(m_lable_play_name, IMAGE_FILE_NAME_W, IMAGE_FILE_NAME_H);
    lv_obj_align(m_lable_play_name, LV_ALIGN_TOP_LEFT, 0, -8);
    lv_label_set_long_mode(m_lable_play_name, LV_LABEL_LONG_SCROLL_CIRCULAR);//LV_LABEL_LONG_DOT, LV_LABEL_LONG_SCROLL_CIRCULAR
    lv_label_set_text(m_lable_play_name, win_media_get_cur_file_name());

    m_label_file_count = lv_label_create(m_image_slide_root);
    lv_obj_add_style(m_label_file_count, TEXT_STY_RIGHT_NORMAL, 0); 
    //lv_obj_set_pos(m_label_file_count, IMAGE_FILE_NAME_W-100, 36);
    lv_obj_align(m_label_file_count, LV_ALIGN_TOP_RIGHT, 0, -8);

    file_list_t *file_list = win_media_get_cur_list();
    lv_label_set_text_fmt(m_label_file_count, "%d/%d\n", 
        file_list->item_index, file_list->file_count);


#if 0


    //create file name label
    m_lable_play_name = lv_label_create(m_image_slide_root);
    //lv_obj_set_pos(m_lable_play_name, IMAGE_FILE_NAME_X,IMAGE_FILE_NAME_Y);
    lv_obj_set_size(m_lable_play_name, IMAGE_FILE_NAME_W, IMAGE_FILE_NAME_H);
    lv_obj_set_style_text_font(m_lable_play_name, &lv_font_montserrat_22, 0);
    //lv_obj_set_style_bg_color(m_lable_play_name, COLOR_DEEP_GREY, 0);

    lv_label_set_long_mode(m_lable_play_name, LV_LABEL_LONG_SCROLL_CIRCULAR);//LV_LABEL_LONG_DOT, LV_LABEL_LONG_SCROLL
    lv_label_set_text(m_lable_play_name, win_media_get_cur_file_name());
    //lv_obj_add_style(m_lable_play_name, &m_text_style_right, 0); 
    lv_obj_align(m_lable_play_name, LV_ALIGN_TOP_LEFT, 0, 2);

    lv_label_t *m_label_file_count;
    m_label_file_count = lv_label_create(m_image_slide_root);
    lv_obj_align(m_label_file_count, LV_ALIGN_TOP_RIGHT, 0, 2);
    lv_label_set_text(m_label_file_count, "1/10");
#endif
}

static int win_image_slide_open(void *arg)
{
    file_list_t *file_list = NULL;
    char *file_name;

    if (NULL == arg){
        printf("%s(), line:%d. no play name!\n", __FUNCTION__, __LINE__);
        return API_FAILURE;
    }

    file_list = win_media_get_cur_list();
    if (NULL == m_image_hld){
        m_image_hld = media_open(file_list->media_type);
    }

    file_name = (char*)arg;
    media_play(m_image_hld, file_name);

    //regist key device to group, so that the object in the group can 
    //get the key event.    
    m_image_group = lv_group_create();

    //user need get the LV_KEY_NEXT and LV_KEY_PREV key, so disable auto focus.
    m_image_group->auto_focus_dis = 1;
    key_regist(m_image_group);

    create_slide_win();


    return API_SUCCESS;
}

static win_ctl_result_t win_image_key_act(uint32_t key)
{
    win_ctl_result_t ret = WIN_CTL_DONE;
    file_list_t *file_list = NULL;
    char *play_name;
    char *file_name;

    media_state_t play_state = media_get_state(m_image_hld);

    switch (key)
    {
    case V_KEY_PLAY:
        break;
    case V_KEY_PAUSE:
        break;
    case V_KEY_UP:
    case V_KEY_DOWN:
    case V_KEY_PREV:
    case V_KEY_NEXT:
        file_list = win_media_get_cur_list();
        if (V_KEY_UP == key || V_KEY_PREV == key)
            play_name = win_media_get_pre_file(file_list);
        else
            play_name = win_media_get_next_file(file_list);
        if (play_name){
            if (MEDIA_STOP != play_state)
                media_stop(m_image_hld);
            media_play(m_image_hld, play_name);
            file_name = win_media_get_cur_file_name();
            lv_label_set_text(m_lable_play_name, file_name);

            lv_label_set_text_fmt(m_label_file_count, "%d/%d\n", 
	            file_list->item_index, file_list->file_count);

        }
        break;
    case V_KEY_LEFT:
        break;
    case V_KEY_RIGHT:
        break;
    case V_KEY_EXIT:
    case V_KEY_MENU:
        if (MEDIA_STOP != play_state){
            media_stop(m_image_hld);
        }
        ret = WIN_CTL_POPUP_CLOSE;
        break;
        break;

    };
    return ret;
}

static win_ctl_result_t win_image_msg_act(control_msg_t *ctl_msg)
{
    win_ctl_result_t ret = WIN_CTL_NONE;


    return ret;
}

static win_ctl_result_t win_image_slide_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;

    if (MSG_TYPE_KEY == ctl_msg->msg_type){
        ret = win_image_key_act(ctl_msg->msg_code);
    }else{
        ret = win_image_msg_act(ctl_msg);
    }

    return ret;    
}

static int win_image_slide_close(void *arg)
{
    lv_group_remove_all_objs(m_image_group);
    lv_group_del(m_image_group);
    lv_obj_del(m_image_slide_root);

    if (m_image_hld)
        media_close(m_image_hld);

    m_image_hld = NULL;

    return API_SUCCESS;   
}

win_des_t g_win_image_slide =
{
    .open = win_image_slide_open,
    .close = win_image_slide_close,
    .control = win_image_slide_control,
};

