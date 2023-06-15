#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
//#include <lvgl/lvgl.h>
#include "../lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"

#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"

static int win_cast_play_open(void *arg)
{
	printf("%s()!\n", __FUNCTION__);
	return API_SUCCESS;
}

static int win_cast_play_close(void *arg)
{
	printf("%s()!\n", __FUNCTION__);
    lv_obj_clean(lv_scr_act());

    return API_SUCCESS;
}

static win_ctl_result_t win_cast_play_control(void *arg1, void *arg2)
{

	printf("%s()!\n", __FUNCTION__);

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_SKIP;

    if(ctl_msg->msg_type == MSG_TYPE_CAST_DLNA_STOP ||
        ctl_msg->msg_type == MSG_TYPE_CAST_AIRCAST_STOP ||
        ctl_msg->msg_type == MSG_TYPE_CAST_MIRACAST_STOP){

    	printf("Entering: %s, line:%d\n", __FUNCTION__, __LINE__);
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
