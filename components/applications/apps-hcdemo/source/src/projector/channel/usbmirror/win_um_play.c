/*
win_um_play.c
 */
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
//#include <lvgl/lvgl.h>
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"

//#include "menu_mgr.h"
#include "../local_mp/com_api.h"
#include "cast_api.h"
//#include "osd_com.h"
//#include "network_api.h"
#include "../../screen.h"

#ifdef USBMIRROR_SUPPORT

lv_obj_t *ui_um_play = NULL;
lv_obj_t *m_um_play_root = NULL;
lv_group_t *m_um_play_group = NULL;
lv_obj_t *m_msg_show = NULL;

static void event_handler(lv_event_t * e);
static volatile bool m_win_um_play = false;
//extern int cast_usb_mirror_start(void);
//extern int cast_usb_mirror_stop(void);

static int win_um_play_open(void *arg)
{
	// uint32_t msg_type;
	// msg_type = (uint32_t)arg;
	printf("%s()\n", __func__);
	//hccast_stop_services();
	
    cast_usb_mirror_start();

	m_um_play_root = lv_obj_create(ui_um_play);

    osd_draw_background(m_um_play_root, true);
    lv_obj_clear_flag(m_um_play_root, LV_OBJ_FLAG_SCROLLABLE);

    m_um_play_group = lv_group_create();
    m_um_play_group->auto_focus_dis = 1;
    lv_indev_set_group(indev_keypad, m_um_play_group);
    lv_obj_add_event_cb(m_um_play_root, event_handler, LV_EVENT_KEY, NULL); 
    lv_group_add_obj(m_um_play_group, m_um_play_root);


    m_msg_show = lv_label_create(m_um_play_root);
    lv_obj_align(m_msg_show, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(m_msg_show, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(m_msg_show, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(m_msg_show, &lv_font_montserrat_22, 0);
    lv_label_set_text_fmt(m_msg_show, "Please insert android or apple device!");



	return API_SUCCESS;
}

static int win_um_play_close(void *arg)
{
	//hccast_start_services();
	m_win_um_play = false;

    cast_usb_mirror_stop();

    if (m_um_play_group){
        lv_group_remove_all_objs(m_um_play_group);
        lv_group_del(m_um_play_group);
    }

    lv_obj_del(m_um_play_root);

	m_win_um_play = false;
	api_hotkey_act_set(true);

    return API_SUCCESS;
}

static void win_um_play_control(void *arg1, void *arg2)
{

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
	printf("-----------%s %d\n",__func__,ctl_msg->msg_type);
    if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_STOP ||
        ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_STOP){
    	m_win_um_play = false;
    	api_hotkey_act_set(true);
    	lv_obj_clear_flag(m_um_play_root, LV_OBJ_FLAG_HIDDEN);
	}else if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_START ||
        ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_START){
    	m_win_um_play = true;
    	api_hotkey_act_set(false);
    	lv_obj_add_flag(m_um_play_root, LV_OBJ_FLAG_HIDDEN);
	}

}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    //do not response the event when in usb mirror
    if (m_win_um_play)
    	return;

    if (ta == ui_um_play){
        if(event == LV_EVENT_SCREEN_LOAD_START) {
            win_um_play_open(NULL);
        } else if (event == LV_EVENT_SCREEN_UNLOAD_START) {
            win_um_play_close(NULL);
        } 
    }else if (ta == m_um_play_root){
        lv_indev_t *key_indev = lv_indev_get_act();
        if(event == LV_EVENT_KEY && key_indev->proc.state == LV_INDEV_STATE_PRESSED){
            uint32_t value = lv_indev_get_key(key_indev);
            if (value == LV_KEY_ESC)
            {
                _ui_screen_change(channel_scr,0,0);
            }
        }

    }
}

void ui_um_play_init(void)
{
    screen_entry_t um_play_entry;

    ui_um_play = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_um_play, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_um_play, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_opa(ui_um_play, LV_OPA_TRANSP, 0);

    um_play_entry.screen = ui_um_play;
    um_play_entry.control = win_um_play_control;
    api_screen_regist_ctrl_handle(&um_play_entry);
}

#endif

// win_des_t g_win_um_play = 
// {
//     .open = win_um_play_open,
//     .close = win_um_play_close,
//     .control = win_um_play_control,
// };
