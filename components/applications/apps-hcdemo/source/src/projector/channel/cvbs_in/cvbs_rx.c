#include <fcntl.h>
#include <unistd.h>
#include <kernel/vfs.h>
#include <stdio.h>
#include <kernel/io.h>
#include <getopt.h>
#include <malloc.h>

#include <kernel/lib/console.h>
#include <kernel/completion.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/tvdec.h>
#include <hcuapi/tvtype.h>
#include <hcuapi/vidmp.h>
#include <stdlib.h>
#include "factory_setting.h"
#include "screen.h"
#include "../../setup/setup.h"
#include <bluetooth.h>

static int tv_dec_fd = -1;
static enum TVTYPE tv_sys = TV_NTSC;
static bool tv_dec_started = false;
int cvbs_rx_start(void);
int cvbs_rx_stop(void);

int cvbs_rx_start(void)
{
    int ret = -1;
    sys_param_t * psys_param;
    psys_param = projector_get_sys_param();
    rotate_type_e rotate_type=ROTATE_TYPE_0;
    mirror_type_e mirror_type=MIRROR_TYPE_NONE;
    int rotate = projector_get_some_sys_param(P_INIT_ROTATE);
    int h_flip = projector_get_some_sys_param(P_INIT_H_FLIP);
    int v_flip = projector_get_some_sys_param(P_INIT_V_FLIP);
    if(tv_dec_started == true)
    {
        return 0;
    }

    tv_dec_fd = open("/dev/tv_decoder" , O_WRONLY);
    if(tv_dec_fd < 0)
    {
        return -1;
    }

    if(psys_param->sysdata.flip_mode == FLIP_MODE_NORMAL){
            if(rotate==0)
                rotate_type=ROTATE_TYPE_0;
            else if(rotate==90)
                rotate_type=ROTATE_TYPE_270;
            else if(rotate==180)
                rotate_type=ROTATE_TYPE_180;
            else if(rotate==270)
                rotate_type=ROTATE_TYPE_90;
            else
                rotate_type=ROTATE_TYPE_180;
            if(h_flip)
                mirror_type = MIRROR_TYPE_LEFTRIGHT;
            if(v_flip)
            {
                if(rotate_type<ROTATE_TYPE_180)
                    rotate_type+=ROTATE_TYPE_180;
                else
                    rotate_type-=ROTATE_TYPE_180;
                mirror_type = MIRROR_TYPE_LEFTRIGHT;
            }
        }
    else{
        if(psys_param->sysdata.flip_mode == FLIP_MODE_ROTATE_180){
            if(rotate==0)
                rotate_type=ROTATE_TYPE_180;
            else if(rotate==90)
                rotate_type=ROTATE_TYPE_90;
            else if(rotate==180)
                rotate_type=ROTATE_TYPE_0;
            else if(rotate==270)
                rotate_type=ROTATE_TYPE_270;
            else
                rotate_type=ROTATE_TYPE_180;
            if(h_flip)
                mirror_type = MIRROR_TYPE_LEFTRIGHT;
            if(v_flip)
            {
                if(rotate_type<ROTATE_TYPE_180)
                    rotate_type+=ROTATE_TYPE_180;
                else
                    rotate_type-=ROTATE_TYPE_180;
                mirror_type = MIRROR_TYPE_LEFTRIGHT;
            }
        }
        else if(psys_param->sysdata.flip_mode == FLIP_MODE_H_MIRROR){
            if(rotate==0)
                rotate_type=ROTATE_TYPE_0;
            else if(rotate==90)
                rotate_type=ROTATE_TYPE_270;
            else if(rotate==180)
                rotate_type=ROTATE_TYPE_180;
            else if(rotate==270)
                rotate_type=ROTATE_TYPE_90;
            else
                rotate_type=ROTATE_TYPE_0;
            if(h_flip==0)
                mirror_type = MIRROR_TYPE_UPDOWN;
            if(v_flip)
            {
                if(rotate_type<ROTATE_TYPE_180)
                    rotate_type+=ROTATE_TYPE_180;
                else
                    rotate_type-=ROTATE_TYPE_180;
                mirror_type = MIRROR_TYPE_NONE;
            }
        }
        else if(psys_param->sysdata.flip_mode == FLIP_MODE_V_MIRROR){
            if(rotate==0)
                rotate_type=ROTATE_TYPE_180;
            else if(rotate==90)
                rotate_type=ROTATE_TYPE_90;
            else if(rotate==180)
                rotate_type=ROTATE_TYPE_0;
            else if(rotate==270)
                rotate_type=ROTATE_TYPE_270;
            else
                rotate_type=ROTATE_TYPE_180;
            if(h_flip==0)
                mirror_type = MIRROR_TYPE_UPDOWN;
            if(v_flip)
            {
                if(rotate_type<ROTATE_TYPE_180)
                    rotate_type+=ROTATE_TYPE_180;
                else
                    rotate_type-=ROTATE_TYPE_180;
                mirror_type = MIRROR_TYPE_NONE;
            }
        }
    }
    printf("%s %d rotate_type =%d mirror_type =%d\n",__FUNCTION__,__LINE__,rotate_type,mirror_type);
    if(rotate>ROTATE_TYPE_0)
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_DATA_PATH , TVDEC_VIDEO_TO_DE_ROTATE);
    else
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_DATA_PATH , TVDEC_VIDEO_TO_DE);
    ret = ioctl(tv_dec_fd, TVDEC_SET_VIDEO_ROTATE_MODE , rotate_type);
    ioctl(tv_dec_fd, TVDEC_SET_VIDEO_MIRROR_MODE , mirror_type);
    if(ret != 0){
        printf("CVBS_SET_VIDEO_STOP_MODE failed\n");
        return -1;
    }

    if(bt_get_connet_state()>=BT_CONNECT_STATUS_CONNECTED)
	{
		bluetooth_set_gpio_mutu(1);
		app_set_i2so_gpio_mute(1);
	}
	else
	{
		bluetooth_set_gpio_mutu(0);
		app_set_i2so_gpio_mute(0);
	}

    ioctl(tv_dec_fd , TVDEC_START , tv_sys);
    printf("tv_dec start ok\n");
    tv_dec_started = true;
    return 0;
}

int cvbs_rx_stop(void)
{
    if(tv_dec_fd >= 0)
    {
        tv_dec_started = false;

        ioctl(tv_dec_fd , TVDEC_STOP);
        close(tv_dec_fd);
        tv_dec_fd = -1;
        return 0;
    }
    else
    {
        return -1;
    }
   
}

