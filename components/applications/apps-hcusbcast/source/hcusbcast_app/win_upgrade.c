/*
win_upgrade.c: show the upgrade progress
 */
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
//#include <lvgl/lvgl.h>
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include "menu_mgr.h"
#include "com_api.h"
#include "osd_com.h"

static lv_obj_t *m_obj_upg_root = NULL;
static lv_obj_t *m_label_upg_msg = NULL;
static lv_obj_t *m_label_progress = NULL;
static lv_obj_t *m_upg_bar = NULL;

#define WIN_UPG_W		(OSD_MAX_WIDTH >> 1)
#define WIN_UPG_H		(OSD_MAX_HEIGHT>> 2)

#define UPG_BAR_W	(WIN_UPG_W-80)
#define UPG_BAR_H	20

#define UPG_STATE_W		UPG_BAR_W
#define UPG_STATE_H		40

static lv_timer_t *m_exit_timer = NULL;

static msg_type_t m_upg_type;

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

static int win_upgrade_open(void *arg)
{
    printf("%s()!\n", __FUNCTION__);

#if 0//CASTING_CLOSE_FB_SUPPORT	    
	api_osd_show_onoff(true);
#endif
        
    m_upg_type = (msg_type_t)arg;
    api_dis_show_onoff(0);
    //upgrad frame
    m_obj_upg_root = lv_obj_create(lv_scr_act());
    lv_obj_add_style(m_obj_upg_root, TEXT_STY_MID_NORMAL, 0);
    lv_obj_clear_flag(m_obj_upg_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(m_obj_upg_root, WIN_UPG_W, WIN_UPG_H);
    lv_obj_center(m_obj_upg_root);
    lv_obj_set_style_bg_color(m_obj_upg_root, COLOR_DEEP_GREY, LV_PART_MAIN); //grey
    lv_obj_set_style_border_opa(m_obj_upg_root, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(m_obj_upg_root, 10, 0);

    //upgrade message
    m_label_upg_msg = lv_label_create(m_obj_upg_root);
    lv_obj_add_style(m_label_upg_msg, TEXT_STY_MID_NORMAL, 0);
    //lv_obj_set_size(m_label_upg_msg, UPG_STATE_W, UPG_STATE_H);
    lv_obj_align(m_label_upg_msg, LV_ALIGN_TOP_LEFT, 10, 10);
    //if (MSG_TYPE_USB_UPGRADE == m_upg_type)
    lv_label_set_text(m_label_upg_msg, "USB upgrade, reading ...");
    // else if (MSG_TYPE_NET_UPGRADE == m_upg_type)
    //	lv_label_set_text(m_label_upg_msg, "Network upgrade, downloading ...");

    //upgrade progress bar
    m_upg_bar = lv_bar_create(m_obj_upg_root);
    lv_obj_align(m_upg_bar, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_size(m_upg_bar, UPG_BAR_W, UPG_BAR_H);
    lv_bar_set_range(m_upg_bar, 0, 100);
    //lv_obj_set_style_local_bg_color(bar1, LV_BAR_PART_BG, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    //lv_obj_set_style_local_bg_color(bar1, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_ORANGE);

    //upgrade progress txt
    m_label_progress = lv_label_create(m_upg_bar);
    //lv_obj_add_style(m_label_progress, TEXT_STY_MID_NORMAL, 0);
    //lv_obj_set_size(m_label_progress, UPG_STATE_W, UPG_STATE_H);
    lv_obj_align(m_label_progress, LV_ALIGN_CENTER, 0, 0);

    win_upgrade_progress_update(0);

    return API_SUCCESS;
}

static int win_upgrade_close(void *arg)
{
    printf("%s()!\n", __FUNCTION__);

    if (m_exit_timer){
        lv_timer_pause((m_exit_timer));
        lv_timer_del(m_exit_timer);
    }

    m_exit_timer = NULL;
    lv_obj_del(m_obj_upg_root);
    api_dis_show_onoff(1);

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

static win_ctl_result_t win_upgrade_status_update(uint32_t status)
{
	win_ctl_result_t ret = WIN_CTL_NONE;

	//wait for a time, then exit upgrade window
	int win_upgrade_exit = 1;

	printf("%s(), line:%d, status:%d\n", __FUNCTION__, __LINE__, status);

    switch (status)
    {
    case UPG_STATUS_USB_OPEN_FILE_ERR:
    	lv_label_set_text(m_label_upg_msg, "Open USB upgraded file fail!");
    	break;
    case UPG_STATUS_USB_FILE_TOO_LARGE:
    	lv_label_set_text(m_label_upg_msg, "Upgraded file too large!");
    	break;
	case UPG_STATUS_SERVER_FAIL:
    	lv_label_set_text(m_label_upg_msg, "HTTP server bad!");
    	break;
	case UPG_STATUS_USER_STOP_DOWNLOAD:
    	lv_label_set_text(m_label_upg_msg, "User download interrupt!");
    	break;
    case UPG_STATUS_USB_READ_ERR:
    	lv_label_set_text(m_label_upg_msg, "Read USB upgraded file error!");
    	break;
    case UPG_STATUS_PRODUCT_ID_MISMATCH:
    	lv_label_set_text(m_label_upg_msg, "Board product ID mismatch!");
    	break;
    case UPG_STATUS_VERSION_IS_OLD:
    	lv_label_set_text(m_label_upg_msg, "Software version is new, do not need upgrade!");
    	break;
    case UPG_STATUS_FILE_CRC_ERROR:
        lv_label_set_text(m_label_upg_msg, "Upgraded firmware is corrupted or illgal!");
        break;
    case UPG_STATUS_FILE_UNZIP_ERROR:
        lv_label_set_text(m_label_upg_msg, "Decompress upgraded firmware fail!");
        break;
    case UPG_STATUS_BURN_START:
    	lv_label_set_text(m_label_upg_msg, "Burning flash, DO NOT power off!");
    	win_upgrade_exit = 0;
    	break;
    case UPG_STATUS_BURN_OK:
        //if (MSG_TYPE_USB_UPGRADE == m_upg_type)
        //    lv_label_set_text(m_label_upg_msg, "Upgrade successful, Plugout U-disk, then reboot ...");
        //else
    	lv_label_set_text(m_label_upg_msg, "Upgrade successful, reboot now ...");
    	win_upgrade_exit = 0;
    	break;
    case UPG_STATUS_BURN_FAIL:
    	lv_label_set_text(m_label_upg_msg, "Flash burn fail, reboot now ...");
    	win_upgrade_exit = 0;
    	break;
    default:
    	break;
    }
    if (win_upgrade_exit) //create a timer, wait some time then exit for showing message for a while
    	win_upgrade_exit_timer();

    return ret;
}

static win_ctl_result_t win_upgrade_msg_proc(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;
    switch (ctl_msg->msg_type)
    {
    case MSG_TYPE_UPG_STATUS:
    	ret = win_upgrade_status_update(ctl_msg->msg_code);
    	break;
    case MSG_TYPE_UPG_DOWNLOAD_PROGRESS:
    	win_upgrade_progress_update(ctl_msg->msg_code);
    	break;
    case MSG_TYPE_UPG_BURN_PROGRESS:
    	win_upgrade_progress_update(ctl_msg->msg_code);
    	break;
    case MSG_TYPE_CLOSE_WIN:
		ret = WIN_CTL_POPUP_CLOSE;
		break;

    default:
    	break;
    }

    return ret;
}

static win_ctl_result_t win_upgrade_control(void *arg1, void *arg2)
{

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;

	if (ctl_msg->msg_type == MSG_TYPE_KEY){
    //remote or pan key process
    } else if (ctl_msg->msg_type > MSG_TYPE_KEY) {
		ret = win_upgrade_msg_proc(arg1, NULL);
    }
    else{
        ret = WIN_CTL_SKIP;
    }	
    return ret;
}

win_des_t g_win_upgrade = 
{
    .open = win_upgrade_open,
    .close = win_upgrade_close,
    .control = win_upgrade_control,
};
