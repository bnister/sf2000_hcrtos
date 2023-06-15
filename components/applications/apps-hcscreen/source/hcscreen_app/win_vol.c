/*
win_vol.c: to set the output volume
 */
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/snd.h>

#include "key.h"
#include "com_api.h"
#include "osd_com.h"
#include "menu_mgr.h"
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include "data_mgr.h"
#include "win_mute.h"
#include "media_player.h"
#include "win_vol.h"

#define WIN_VOL_W	50
#define WIN_VOL_H	480

#define BAR_VOL_W	30
#define BAR_VOL_H	400

#define IMG_VOL_W	30
#define IMG_VOL_H	400

#define VOL_BAR_HIDE_TIME  (5000) //the playbar would hide after 10 seconds if no key

LV_IMG_DECLARE(im_volume);
LV_IMG_DECLARE(im_mute1);

static lv_obj_t *m_obj_vol_root = NULL;
static lv_obj_t *m_bar_vol = NULL;
static lv_obj_t *m_txt_vol = NULL;
static lv_obj_t *m_img_vol = NULL;

static  bool m_vol_bar_show = true;
static lv_timer_t *bar_show_timer = NULL;

// = 1, should set volume in the UI; = 0, only show UI.
static int m_set_vol = 1;

static void show_play_bar(bool show)
{
    if (show){
        lv_obj_clear_flag(m_obj_vol_root, LV_OBJ_FLAG_HIDDEN);
    }else{
        lv_obj_add_flag(m_obj_vol_root, LV_OBJ_FLAG_HIDDEN);
    }
    printf("%s(), open show: %d\n", __FUNCTION__, show);
    m_vol_bar_show = show;

}

static void bar_show_timer_cb(lv_timer_t * t)
{
    show_play_bar(false);
    lv_timer_pause(bar_show_timer);
}


static int win_vol_update(uint32_t param)
{
	uint16_t key = (uint16_t)((param & 0xffff0000) >> 16);
	uint8_t volume = (uint8_t)(param & 0xff);
	uint8_t vol_get = 0;

    if (m_set_vol){
		vol_get = data_mgr_volume_get();
	    if (V_KEY_V_UP == key){
	    	if (vol_get >= 90)
	    		volume = 100;
	    	else
	    		volume = vol_get + 10;
	    } else if (V_KEY_V_DOWN == key){
	    	if (vol_get <= 10)
	    		volume = 0;
	    	else
	    		volume = vol_get - 10;
	    }
    	media_set_vol(volume);
    	data_mgr_volume_set(volume);
    }

    if (!win_is_unmute()){
    	win_mute_on_off(true);
    }

	if (volume < 50)
		lv_obj_set_style_text_color(m_bar_vol, lv_color_hex(0xffffff), 0); //white
	else
		lv_obj_set_style_text_color(m_bar_vol, lv_color_hex(0), 0); //black

	if (volume > 0)
		lv_img_set_src(m_img_vol, &im_volume);
	else if (0 == volume)
		lv_img_set_src(m_img_vol, &im_mute1);


	lv_bar_set_value(m_bar_vol, volume, LV_ANIM_OFF);
    lv_label_set_text_fmt(m_txt_vol, "%d", volume);

    show_play_bar(true);
    lv_timer_reset(bar_show_timer);
    lv_timer_resume(bar_show_timer);

    return API_SUCCESS;
}

static int win_volume_open(void *arg)
{
	printf("%s()!\n", __FUNCTION__);

	//volume window
	m_obj_vol_root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_obj_vol_root, WIN_VOL_W, WIN_VOL_H);
	lv_obj_set_style_bg_color(m_obj_vol_root, COLOR_DEEP_GREY, LV_PART_MAIN); //grey
	lv_obj_set_style_border_opa(m_obj_vol_root, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_align(m_obj_vol_root, LV_ALIGN_TOP_RIGHT, -40, 40);
    lv_obj_clear_flag(m_obj_vol_root, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(m_obj_vol_root, 10, 0);

	//volume image
	m_img_vol = lv_img_create(m_obj_vol_root);
	lv_obj_align(m_img_vol, LV_ALIGN_TOP_MID, 0, -8);

    //volume bar
    m_bar_vol = lv_bar_create(m_obj_vol_root);
	lv_obj_align(m_bar_vol, LV_ALIGN_BOTTOM_MID, 0, 4);
	lv_obj_set_size(m_bar_vol, BAR_VOL_W, BAR_VOL_H);
	lv_bar_set_range(m_bar_vol, 0, 100);

	//lv_obj_set_style_local_bg_color(m_bar_vol, LV_BAR_PART_BG, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    //lv_obj_set_style_local_bg_color(m_bar_vol, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_ORANGE);

    //volume txt
    m_txt_vol = lv_label_create(m_bar_vol);
    //lv_obj_add_style(m_label_progress, TEXT_STY_MID_NORMAL, 0);
    //lv_obj_set_size(m_label_progress, UPG_STATE_W, UPG_STATE_H);
    lv_obj_align(m_txt_vol, LV_ALIGN_CENTER, 0, 0);

	bar_show_timer = lv_timer_create(bar_show_timer_cb, VOL_BAR_HIDE_TIME, NULL);

	win_vol_update((uint32_t)arg);

	return API_SUCCESS;
}

int win_volume_close(void *arg)
{
	(void)arg;
	printf("%s()!\n", __FUNCTION__);

	if (NULL == m_obj_vol_root)
		return API_SUCCESS;

	if (bar_show_timer){
	    lv_timer_pause(bar_show_timer);
	    lv_timer_del(bar_show_timer);
	}

	lv_obj_del(m_obj_vol_root);

	bar_show_timer = NULL;
	m_obj_vol_root = NULL;

	m_set_vol = 1;
    return API_SUCCESS;
}


static win_ctl_result_t win_volume_control(void *arg1, void *arg2)
{

	printf("%s()!\n", __FUNCTION__);

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;

	if (ctl_msg->msg_type == MSG_TYPE_KEY){
		// if (ctl_msg->msg_type == V_KEY_UP){
		// }
    } else if (ctl_msg->msg_type > MSG_TYPE_KEY) {
    }
    else{
        ret = WIN_CTL_SKIP;
    }	

	return ret;
}

int win_volume_set(uint32_t param, uint8_t set_vol)
{
	m_set_vol = set_vol;
	if (NULL == m_obj_vol_root)
		return win_volume_open((void*)param);
	else
		return win_vol_update(param);

}


win_des_t g_win_volume = 
{
    .open = win_volume_open,
    .close = win_volume_close,
    .control = win_volume_control,
};


