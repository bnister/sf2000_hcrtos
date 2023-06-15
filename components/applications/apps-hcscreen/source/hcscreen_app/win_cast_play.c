#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#ifdef __HCRTOS__
#include <miracast/miracast_api.h>
#else
#include <hccast/miracast_api.h>
#endif

//#include <lvgl/lvgl.h>
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"

#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
#include "osd_com.h"
#include "win_cast_root.h"

enum{
    CAST_TYPE_AIR,
    CAST_TYPE_MIRA,
};
static uint32_t m_cast_type = CAST_TYPE_AIR;

static volatile bool m_win_cast_play_open = false;

bool win_cast_play_wait_open(uint32_t timeout)
{
    uint32_t count;
    count = timeout/20;

    while(count--){
        if (m_win_cast_play_open)
            break;
        api_sleep_ms(20);
    }
    printf("%s(), m_win_cast_play_open(%d):%d\n", __func__, m_win_cast_play_open, count);
    return m_win_cast_play_open; 
}

static int win_cast_play_open(void *arg)
{
    printf("%s(), line: %d!\n", __func__, __LINE__);
    uint32_t msg_type;
    msg_type = (uint32_t)arg;

#if 0//CASTING_CLOSE_FB_SUPPORT	
    api_osd_show_onoff(false);
#endif	
    if (msg_type == MSG_TYPE_CAST_AIRMIRROR_START)
        m_cast_type = CAST_TYPE_AIR;
    else
        m_cast_type = CAST_TYPE_MIRA;

	m_win_cast_play_open = true;
	return API_SUCCESS;
}

static int win_cast_play_close(void *arg)
{
    printf("%s(), line: %d!\n", __func__, __LINE__);
    //lv_obj_clean(lv_scr_act());
#if 0//CASTING_CLOSE_FB_SUPPORT	    
    api_osd_show_onoff(true);
#endif
    win_msgbox_msg_close();

    if (win_exit_to_cast_root_by_key_get()){
        if (CAST_TYPE_MIRA == m_cast_type)
            hccast_mira_disconnect();
        else
            hccast_air_service_stop();
    }

    win_exit_to_cast_root_by_key_set(false);
	m_win_cast_play_open = false;
    return API_SUCCESS;
}

static win_ctl_result_t win_cast_play_control(void *arg1, void *arg2)
{

	printf("%s()!\n", __FUNCTION__);

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_SKIP;

    if(ctl_msg->msg_type == MSG_TYPE_CAST_AIRMIRROR_STOP ||
        ctl_msg->msg_type == MSG_TYPE_CAST_MIRACAST_STOP){

        ret = WIN_CTL_POPUP_CLOSE;
	}
	else if(ctl_msg->msg_type == MSG_TYPE_AIR_MIRROR_BAD_NETWORK)
	{
		win_msgbox_msg_open("Please try reconnect!", 1000*60*60, NULL, NULL);	
	}
    else if(ctl_msg->msg_type == MSG_TYPE_USB_WIFI_PLUGOUT)
    {
        win_exit_to_cast_root_by_key_set(true);
        ret = WIN_CTL_POPUP_CLOSE;
    }

	return ret;
}

win_des_t g_win_cast_play = 
{
    .open = win_cast_play_open,
    .close = win_cast_play_close,
    .control = win_cast_play_control,
};
