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

#include <hcuapi/input.h>
#include <hcuapi/input-event-codes.h>


//#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
#include "osd_com.h"
//#include "network_api.h"
#include "screen.h"
#include "mul_lang_text.h"
#include "hcstring_id.h"
#include "setup.h"
#include "factory_setting.h"
#include "win_cast_root.h"

#ifdef USBMIRROR_SUPPORT
// #define QR_UM_MSG_X        (20+180)
// #define QR_UM_MSG_Y        (0)
// #define QR_UM_MSG_W        300
// #define QR_UM_BOX_X        (-20)
// #define QR_UM_BOX_Y        (-100)
// #define QR_UM_BOX_W        160
#ifdef LVGL_RESOLUTION_240P_SUPPORT
#define QR_UM_MSG_X        2
#define QR_UM_MSG_Y        80
#define QR_UM_MSG_W        110
#define QR_UM_BOX_X        QR_UM_MSG_X+2
#define QR_UM_BOX_Y        QR_UM_MSG_Y+80
#define QR_UM_BOX_W        60

#else

#define QR_UM_MSG_X        (20)
#define QR_UM_MSG_Y        (220)
#define QR_UM_MSG_W        350
#define QR_UM_BOX_X        (QR_UM_MSG_X + 20)
#define QR_UM_BOX_Y        (QR_UM_MSG_Y+100)
#define QR_UM_BOX_W        160
#endif

int qr_um_x = QR_UM_BOX_X;
int qr_um_y = QR_UM_BOX_Y;

lv_obj_t *ui_um_play = NULL;
lv_obj_t *m_um_play_root = NULL;
lv_group_t *m_um_play_group = NULL;
lv_obj_t *m_msg_show = NULL;
lv_obj_t *m_label_qr_msg = NULL;
static lv_obj_t *m_cast_qr = NULL;
typedef enum{
    UM_TYPE_NONE,
    UM_YTPE_I, //apple device    
    UM_YTPE_A, //android devide
}UM_TYPE_E;

static void event_handler(lv_event_t * e);
static volatile bool m_playing = false;
static char m_um_type = UM_TYPE_NONE;

//extern int cast_usb_mirror_start(void);
//extern int cast_usb_mirror_stop(void);
typedef enum{
    QR_UM_CLEAR,
    QR_UM_SHOW_APK,
}qr_um_show_type_t;

static void win_um_update_qr_code(qr_um_show_type_t qr_type)
{

#ifndef HTTPD_SERVICE_SUPPORT
    return;
#endif

    /*Set data*/
    //const char * data = "https://lvgl.io";
    char qr_txt[128] = {0};
    char msg_txt[128] = {0};

    int str_id = 0;
    lv_obj_clear_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
    switch (qr_type)
    {
    case QR_UM_CLEAR:
        //sprintf(msg_txt,"");
        memset(msg_txt, 0, sizeof(msg_txt));
        lv_obj_add_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
        break;
    case QR_UM_SHOW_APK:
        str_id = STR_ANDROID_SCAN_QR;
        //1. 请扫码连接本设备

        sprintf(qr_txt, "http://119.3.89.190:8080/apk/elfcast.apk");
        break;
    default:
        break;
    }

    if (str_id)
    {
        //lv_label_set_text(m_label_qr_msg, msg_txt);
        win_cast_label_txt_set(m_label_qr_msg, str_id);
		// lv_label_set_text(m_label_qr_msg, "安卓设备请扫码安装软件\n");
    }
    // else
    //     lv_label_set_text(m_label_qr_msg, str_id);

    if (strlen(qr_txt))
        lv_qrcode_update(m_cast_qr, qr_txt, strlen(qr_txt));
	lv_obj_align(m_cast_qr, 0, qr_um_x, qr_um_y);
}

static int win_um_play_open(void *arg)
{
	// uint32_t msg_type;
	// msg_type = (uint32_t)arg;
	printf("%s()\n", __func__);
	//hccast_stop_services();
	
    m_um_type = UM_TYPE_NONE;
    cast_usb_mirror_start();
    set_display_zoom_when_sys_scale();
    api_set_display_aspect(DIS_TV_16_9, DIS_PILLBOX);    
    
	m_um_play_root = lv_obj_create(ui_um_play);

    osd_draw_background(m_um_play_root, true);
    lv_obj_clear_flag(m_um_play_root, LV_OBJ_FLAG_SCROLLABLE);

    m_um_play_group = lv_group_create();
    m_um_play_group->auto_focus_dis = 1;
    key_set_group(m_um_play_group);

    lv_obj_add_event_cb(m_um_play_root, event_handler, LV_EVENT_KEY, NULL); 
    lv_group_add_obj(m_um_play_group, m_um_play_root);
    lv_group_focus_obj(m_um_play_root);

    m_msg_show = lv_label_create(m_um_play_root);
    lv_obj_align(m_msg_show, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(m_msg_show, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(m_msg_show, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(m_msg_show, osd_font_get(FONT_MID), 0);

    win_cast_label_txt_set(m_msg_show, STR_INSERT_ANDROID_OR_APPLE);

	lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);
	m_cast_qr = lv_qrcode_create(m_um_play_root, QR_UM_BOX_W, fg_color, bg_color);
	lv_obj_align(m_cast_qr, 0, QR_UM_BOX_X, QR_UM_BOX_Y);
	/*Add a border with bg_color(white: 0)*/
	lv_obj_set_style_border_color(m_cast_qr, bg_color, 0);
	lv_obj_set_style_border_width(m_cast_qr, 5, 0);

	m_label_qr_msg = lv_label_create(m_um_play_root);
    lv_obj_align(m_label_qr_msg, 0, QR_UM_MSG_X, QR_UM_MSG_Y);
    lv_obj_set_style_bg_opa(m_label_qr_msg, LV_OPA_TRANSP, 0);
	lv_obj_set_width(m_label_qr_msg, QR_UM_MSG_W);
    lv_obj_set_style_text_color(m_label_qr_msg, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(m_label_qr_msg, osd_font_get(FONT_MID), 0);
	win_um_update_qr_code(QR_UM_SHOW_APK);

	return API_SUCCESS;
}

static int win_um_play_close(void *arg)
{
	//hccast_start_services();
    cast_usb_mirror_stop();

    if (m_um_play_group){
        lv_group_remove_all_objs(m_um_play_group);
        lv_group_del(m_um_play_group);
        lv_group_set_default(NULL);
    }

    lv_obj_del(m_um_play_root);

	m_playing = false;
    m_um_type = UM_TYPE_NONE;
    api_hotkey_disable_clear();

    //recover the dispaly aspect.
    api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);

    return API_SUCCESS;
}


static void win_um_play_stop(bool stop_by_key)
{
#if 1    
    if (stop_by_key)
    {
        if (UM_YTPE_I == m_um_type){
            hccast_ium_stop_mirroring();
        }
        else if (UM_YTPE_A == m_um_type){
            hccast_aum_stop_mirroring();
        }
    }
#endif    
    m_playing = false;
    m_um_type = UM_TYPE_NONE;
    api_hotkey_disable_clear();
    lv_obj_clear_flag(m_um_play_root, LV_OBJ_FLAG_HIDDEN);

}

static uint32_t m_hotkey[] = {KEY_POWER, KEY_VOLUMEUP, \
                    KEY_VOLUMEDOWN, KEY_MUTE, KEY_ROTATE_DISPLAY, KEY_FLIP,KEY_CAMERA_FOCUS,KEY_FORWARD,KEY_BACK,KEY_HOME};
static void win_um_play_control(void *arg1, void *arg2)
{

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

    if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_STOP ||
        ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_STOP){
        win_um_play_stop(false);
	}else if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_START ||
        ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_START){
            //sleep(2);
        
        set_display_zoom_when_sys_scale();
    	m_playing = true;
    	api_hotkey_enable_set(m_hotkey, sizeof(m_hotkey)/sizeof(m_hotkey[0]));
    	lv_obj_add_flag(m_um_play_root, LV_OBJ_FLAG_HIDDEN);

        if (ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_START)
            m_um_type = UM_YTPE_I;
        else
            m_um_type = UM_YTPE_A;

        //extern void api_set_i2so_gpio_mute_auto(void);
        api_set_i2so_gpio_mute_auto();
        

	}else if(ctl_msg->msg_type == MSG_TYPE_AUM_DEV_ADD ||
        ctl_msg->msg_type == MSG_TYPE_IUM_DEV_ADD){

    #ifdef MIRROR_ES_DUMP_SUPPORT
        extern bool api_mirror_dump_enable_get(char* folder);
        char dump_folder[64];

        if (USB_STAT_MOUNT != mmp_get_usb_stat())
        {
            printf("%s(), line: %d. No disk, disable dump!\n", __func__, __LINE__);
            hccast_um_es_dump_stop();
            return;
        }
        printf("%s(), line: %d. Statr USB mirror ES dump!\n", __func__, __LINE__);
        if (api_mirror_dump_enable_get(dump_folder)){
            hccast_um_es_dump_start(dump_folder);
        } else {
            hccast_um_es_dump_stop();
        }
    #endif    
    }

}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    //do not response the event when in usb mirror
    // if (m_playing)
    // 	return;

    if (ta == ui_um_play){
        if(event == LV_EVENT_SCREEN_LOAD_START) {
		    //cast_usb_mirror_init(); 
            win_um_play_open(NULL);
        } else if (event == LV_EVENT_SCREEN_UNLOAD_START) {
            win_um_play_close(NULL);
            //cast_usb_mirror_deinit();
        } 
    }else if (ta == m_um_play_root){
        lv_indev_t *key_indev = lv_indev_get_act();
        if(event == LV_EVENT_KEY && key_indev->proc.state == LV_INDEV_STATE_PRESSED){
            uint32_t value = lv_indev_get_key(key_indev);
            if (value == LV_KEY_ESC)
            {
                if (m_playing)
                    win_um_play_stop(true);
                else                    
                    change_screen(SCREEN_CHANNEL_MAIN_PAGE);
            }else if(value == FUNC_KEY_SCREEN_ROTATE){
                win_cast_mirror_rotate_switch();
            }
        }

    }
}

void ui_um_play_init(void)
{
    screen_entry_t um_play_entry;

    hccast_ium_set_resolution(1280, 720);
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
