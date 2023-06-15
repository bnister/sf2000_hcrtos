/*
win_upgrade.c: show the upgrade progress
 */

#include "app_config.h"

#ifdef WIFI_SUPPORT

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
//#include <lvgl/lvgl.h>
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
//#include "menu_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "screen.h"
#include "win_cast_root.h"

lv_obj_t *ui_network_upgrade;

static lv_obj_t *m_obj_upg_root = NULL;
static lv_obj_t *m_label_upg_msg = NULL;
static lv_obj_t *m_label_progress = NULL;
static lv_obj_t *m_upg_bar = NULL;

#ifdef LVGL_RESOLUTION_240P_SUPPORT
  #define WIN_UPG_W     (OSD_MAX_WIDTH*2)/3
  #define WIN_UPG_H     (OSD_MAX_HEIGHT>> 1)

  #define UPG_BAR_W (WIN_UPG_W-40)
  #define UPG_BAR_H 20

  #define UPG_STATE_W       UPG_BAR_W
  #define UPG_STATE_H       40
#else
  #define WIN_UPG_W		(OSD_MAX_WIDTH >> 1)
  #define WIN_UPG_H		(OSD_MAX_HEIGHT>> 2)

  #define UPG_BAR_W	(WIN_UPG_W-80)
  #define UPG_BAR_H	20

  #define UPG_STATE_W		UPG_BAR_W
  #define UPG_STATE_H		40
#endif

static lv_timer_t *m_exit_timer = NULL;

static msg_type_t m_upg_type = MSG_TYPE_NET_UPGRADE;
static bool m_flash_burning = false;

static void event_handler(lv_event_t * e);

static void win_upgrade_progress_update(int progress)
{
	int percent;

	if (progress <= 0)
		percent = 0;
	if (progress > 100)
		percent = 100;

	percent = progress;
	lv_bar_set_value(m_upg_bar, percent, LV_ANIM_OFF);
	lv_label_set_text_fmt(m_label_progress, "%d%%", percent);

	if (percent < 50)
		lv_obj_set_style_text_color(m_label_progress, lv_color_hex(0xffffff), 0); //white
	else
		lv_obj_set_style_text_color(m_label_progress, lv_color_hex(0), 0); //black
}

static lv_group_t *m_upgrade_group  = NULL;
static int win_upgrade_open(void *arg)
{
	printf("%s()!\n", __FUNCTION__);

    m_flash_burning = false;
    api_hotkey_enable_set(NULL, 0);

	//upgrade frame
	m_obj_upg_root = lv_obj_create(ui_network_upgrade);
    lv_obj_clear_flag(m_obj_upg_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(m_obj_upg_root, WIN_UPG_W, WIN_UPG_H);
    lv_obj_center(m_obj_upg_root);
    lv_obj_set_style_bg_color(m_obj_upg_root, COLOR_DEEP_GREY, LV_PART_MAIN); //grey
    lv_obj_set_style_border_opa(m_obj_upg_root, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(m_obj_upg_root, 10, 0);

    m_upgrade_group = lv_group_create();
    key_set_group(m_upgrade_group);
    lv_group_add_obj(m_upgrade_group, m_obj_upg_root);
    lv_obj_add_event_cb(m_obj_upg_root, event_handler, LV_EVENT_KEY, NULL);

    //upgrade message
    m_label_upg_msg = lv_label_create(m_obj_upg_root);
    //lv_obj_add_style(m_label_upg_msg, TEXT_STY_MID_NORMAL, 0);
    lv_obj_set_style_text_color(m_label_upg_msg, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(m_label_upg_msg, osd_font_get(FONT_NORMAL), LV_PART_MAIN);

    //lv_obj_set_size(m_label_upg_msg, UPG_STATE_W, UPG_STATE_H);
    lv_obj_align(m_label_upg_msg, LV_ALIGN_TOP_LEFT, 10, 10);
    if (MSG_TYPE_USB_UPGRADE == m_upg_type)
        lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_USB_UPG_READ));
    else if (MSG_TYPE_NET_UPGRADE == m_upg_type)
        lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_NET_UPG_DOWNLOAD));

    //upgrade progress bar
    m_upg_bar = lv_bar_create(m_obj_upg_root);
	lv_obj_align(m_upg_bar, LV_ALIGN_BOTTOM_MID, 0, -10);
	lv_obj_set_size(m_upg_bar, UPG_BAR_W, UPG_BAR_H);
	lv_bar_set_range(m_upg_bar, 0, 100);

    //upgrade progress txt
    m_label_progress = lv_label_create(m_upg_bar);
    lv_obj_align(m_label_progress, LV_ALIGN_CENTER, 0, 0);

	win_upgrade_progress_update(0);

	return API_SUCCESS;
}

static int win_upgrade_close(void *arg)
{
	printf("%s()!\n", __FUNCTION__);

    hccast_web_set_user_abort(1);
    m_flash_burning = false;

	if (m_exit_timer){
	    lv_timer_pause((m_exit_timer));
	    lv_timer_del(m_exit_timer);
	}

    if (m_upgrade_group){
        lv_group_remove_all_objs(m_upgrade_group);
        lv_group_del(m_upgrade_group);
    }

	m_exit_timer = NULL;
	lv_obj_del(m_obj_upg_root);


    api_hotkey_disable_clear();
    return API_SUCCESS;
}

static void upgrade_exit_timer_cb(lv_timer_t *t)
{
    control_msg_t msg = {0};

    msg.msg_type = MSG_TYPE_CLOSE_WIN;
    api_control_send_msg(&msg);

}

static void win_upgrade_exit_timer()
{
	if (NULL == m_exit_timer)
		m_exit_timer = lv_timer_create(upgrade_exit_timer_cb, 5000, NULL);
	else
		lv_timer_reset(m_exit_timer);
}

static void win_upgrade_status_update(uint32_t status)
{

	//wait for a time, then exit upgrade window
	int win_upgrade_exit = 1;

	printf("%s(), line:%d, status:%d\n", __FUNCTION__, __LINE__, (int)status);

    switch (status)
    {
    case UPG_STATUS_USB_OPEN_FILE_ERR:
    	lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_USB_UPG_OPEN_FILE_FAIL));
    	break;
    case UPG_STATUS_USB_FILE_TOO_LARGE:
    	lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_USB_UPG_FILE_LARGE));
    	break;
	case UPG_STATUS_SERVER_FAIL:
    	lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_NET_UPG_HTTP_BAD));
    	break;
	case UPG_STATUS_USER_STOP_DOWNLOAD:
        lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_UPG_USER_CANCEL));
    	break;
    case UPG_STATUS_USB_READ_ERR:
        lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_USB_UPG_READ_FAIL));
    	break;
    case UPG_STATUS_PRODUCT_ID_MISMATCH:
        lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_UPG_PRODUCT_ID_MISMATCH));
    	break;
    case UPG_STATUS_VERSION_IS_OLD:
        lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_UPG_SW_VER_NOT_UPG));
    	break;
    case UPG_STATUS_FILE_CRC_ERROR:
        lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_UPG_SW_ILLGAL));
        break;
    case UPG_STATUS_FILE_UNZIP_ERROR:
        lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_UPG_UNZIP_FAIL));
        break;
    case UPG_STATUS_BURN_START:
        lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_UPG_BURNING));
    	win_upgrade_exit = 0;
    	break;
    case UPG_STATUS_BURN_OK:
        if (MSG_TYPE_USB_UPGRADE == m_upg_type)
            lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_USB_UPG_OK));
        else
            lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_NET_UPG_OK));
    	win_upgrade_exit = 0;
    	break;
    case UPG_STATUS_BURN_FAIL:
        lv_label_set_text(m_label_upg_msg, api_rsc_string_get(STR_UPG_BURN_FAIL));
    	win_upgrade_exit = 0;
    	break;
    default:
    	break;
    }
    if (win_upgrade_exit) //create a timer, wait some time then exit for showing message for a while
    	win_upgrade_exit_timer();

}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (ta == ui_network_upgrade){
        if(event == LV_EVENT_SCREEN_LOAD_START) {
            win_upgrade_open(NULL);
        } else if (event == LV_EVENT_SCREEN_UNLOAD_START) {
            win_upgrade_close(NULL);
        } 
    }else if (ta == m_obj_upg_root){
        if (event == LV_EVENT_KEY){
            lv_indev_t *key_indev = lv_indev_get_act();
            uint32_t value = lv_indev_get_key(key_indev);            
            if (key_indev->proc.state == LV_INDEV_STATE_PRESSED){
                if (value == LV_KEY_ESC && !m_flash_burning) { 
                    api_scr_go_back();
                    //_ui_screen_change(ui_wifi_cast_root, 0, 0);
                }
            }
        }
    }

}

static void win_upgrade_msg_proc(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    switch (ctl_msg->msg_type)
    {
    case MSG_TYPE_UPG_STATUS:
    	win_upgrade_status_update(ctl_msg->msg_code);
    	break;
    case MSG_TYPE_UPG_DOWNLOAD_PROGRESS:
    	win_upgrade_progress_update(ctl_msg->msg_code);
    	break;
    case MSG_TYPE_UPG_BURN_PROGRESS:
        m_flash_burning = true;
    	win_upgrade_progress_update(ctl_msg->msg_code);
    	break;
    case MSG_TYPE_CLOSE_WIN:
		// ret = WIN_CTL_POPUP_CLOSE;
		break;

    default:
    	break;
    }

}

static void win_upgrade_control(void *arg1, void *arg2)
{

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

	if (ctl_msg->msg_type == MSG_TYPE_KEY){
    //remote or pan key process
    } else if (ctl_msg->msg_type > MSG_TYPE_KEY) {
		win_upgrade_msg_proc(arg1, NULL);
    }
    else{
        // ret = WIN_CTL_SKIP;
    }	
}

void ui_network_upgrade_init(void)
{
    screen_entry_t upgrade_entry;

    ui_network_upgrade = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_network_upgrade, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_network_upgrade, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_opa(ui_network_upgrade, LV_OPA_TRANSP, 0);

    upgrade_entry.screen = ui_network_upgrade;
    upgrade_entry.control = win_upgrade_control;
    api_screen_regist_ctrl_handle(&upgrade_entry);
}

void win_upgrade_type_set(uint32_t upgrade_type)
{
    m_upg_type = upgrade_type;
}

// win_des_t g_win_upgrade = 
// {
//     .open = win_upgrade_open,
//     .close = win_upgrade_close,
//     .control = win_upgrade_control,
// };


#endif