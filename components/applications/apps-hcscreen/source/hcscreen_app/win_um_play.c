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

#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
#include "osd_com.h"
#include "network_api.h"


static volatile bool m_win_um_play = false;
static int win_um_play_open(void *arg)
{
	// uint32_t msg_type;
	// msg_type = (uint32_t)arg;
	printf("%s()\n", __func__);
	//hccast_stop_services();
	m_win_um_play = true;
	return API_SUCCESS;
}

static int win_um_play_close(void *arg)
{
	//hccast_start_services();
	m_win_um_play = false;
    return API_SUCCESS;
}

static win_ctl_result_t win_um_play_control(void *arg1, void *arg2)
{

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_SKIP;

    if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_STOP ||
        ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_STOP){
        ret = WIN_CTL_POPUP_CLOSE;
	}
	return ret;
}

/*
bool win_um_play_get(void)
{
	return m_win_um_play;
}
*/

win_des_t g_win_um_play = 
{
    .open = win_um_play_open,
    .close = win_um_play_close,
    .control = win_um_play_control,
};
