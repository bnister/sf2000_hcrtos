#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <hcuapi/dis.h>
#include <hcuapi/vidmp.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef SUPPORT_FFPLAYER
#include <ffplayer.h>
#endif

#include "com_api.h"

#include "lv_drivers/display/fbdev.h"

extern void *mp_get_cur_player_hdl(void);

void api_get_rotate_by_flip_mode(flip_mode_e mode,
                             int *p_rotate_mode ,
                             int *p_h_flip ,
                             int *p_v_flip)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    
    switch(mode)
    {
        case FLIP_MODE_CEILING_REAR:
        {
            //printf("180\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 0;
            v_flip = 0;
            break;
        }

        case  FLIP_MODE_FRONT:
        {
            //printf("h_mirror\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 1;
            v_flip = 0;
            break;
        }

        case FLIP_MODE_CEILING_FRONT:
        {
           // printf("v_mirror\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 1;
            v_flip = 0;
            break;

        }
        case FLIP_MODE_REAR:
        {
            //printf("normal\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 0;
            v_flip = 0;
            break;
        }
        default:
            break;
    }

    *p_rotate_mode = rotate;
    *p_h_flip = h_flip;
    *p_v_flip = v_flip;

}

int set_flip_mode(flip_mode_e mode)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
//    media_handle_t *mp_hdl=mp_get_cur_player_hdl();
    int fbdev_rotate;

    int init_rotate = api_get_screen_init_rotate();
    int init_h_flip = api_get_screen_init_h_flip();
    int init_v_flip = api_get_screen_init_v_flip();
	
    api_get_rotate_by_flip_mode(mode, &rotate , &h_flip , &v_flip);
    api_transfer_rotate_mode_for_screen(init_rotate , init_h_flip , init_v_flip ,
                                        &rotate , &h_flip , &v_flip , &fbdev_rotate);
    fbdev_set_rotate(fbdev_rotate , h_flip , v_flip);
#ifdef SUPPORT_FFPLAYER    
    void *player = api_ffmpeg_player_get();
    if(player != NULL)
    {
        hcplayer_change_rotate_type(player , rotate);
        hcplayer_change_mirror_type(player , h_flip);
    }
#endif
    // if(mp_hdl!=NULL&&mp_hdl->type==MEDIA_TYPE_MUSIC){
    //     app_reset_mainlayer_pos(rotate, h_flip);
    // }

    printf("rotate = %u mode =%d h_flip =%d\n",rotate,mode, h_flip);

#if 0    
//set hdmi rx/cvbs rx flip mode
#ifdef HDMIIN_SUPPORT     
#if PROJECTER_C2_VERSION
    if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI||projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI2)
#else
    if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI)
#endif
    {
        hdmi_rx_set_flip_mode(rotate, h_flip);
    }
    else 
#endif		
	if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_CVBS){
        cvbs_rx_set_flip_mode(rotate , h_flip);
        //cvbs_rx_stop();
        //cvbs_rx_start();
    }
#endif    
    return 0;
}





