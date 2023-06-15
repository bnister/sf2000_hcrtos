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
#include <ffplayer.h>

#include "lv_drivers/display/fbdev.h"
#include "../../screen.h"
#include "../../factory_setting.h"

extern void *mp_get_cur_player_hdl(void);

int set_flip_mode(flip_mode_e mode)
{
    void *player = mp_get_cur_player_hdl();// get from mediaplayer module, to be cont.
    
    int rotate = projector_get_some_sys_param(P_INIT_ROTATE);
    int h_flip = projector_get_some_sys_param(P_INIT_H_FLIP);
    int v_flip = projector_get_some_sys_param(P_INIT_V_FLIP);
    printf("rotate = %u h_flip %u v_flip = %u\n",rotate,h_flip,v_flip);
    switch (mode){
        case FLIP_MODE_ROTATE_180:
            if(rotate<180)
                rotate+=180;
            else
                rotate-=180;
            fbdev_set_rotate(rotate, h_flip, v_flip);
            printf("180\n");
            if(player !=NULL)
                hcplayer_change_rotate_type(player, ROTATE_TYPE_180);
             //else
              //  printf(" hcplayer_create(&init_args)  need set ROTATE_TYPE_180 \n");
            break;
        case  FLIP_MODE_H_MIRROR:
            h_flip = ~(0xff&h_flip);
            fbdev_set_rotate(rotate, h_flip, v_flip);
            printf("h_mirror\n");
            if(player !=NULL)
                hcplayer_change_mirror_type(player, MIRROR_TYPE_LEFTRIGHT);
             //else
             //   printf(" hcplayer_create(&init_args)  need set MIRROW_TYPE_LEFTRIGHT \n");
            break;
        case FLIP_MODE_V_MIRROR:
            v_flip = ~(0xff&v_flip);
            fbdev_set_rotate(rotate, h_flip, v_flip);
            printf("v_mirror\n");
            if(player !=NULL)
                hcplayer_change_rotate_type(player, MIRROR_TYPE_UPDOWN);
             //else
              //  printf(" hcplayer_create(&init_args)  need set MIRROW_TYPE_UPDOWN \n");
            break;
        case FLIP_MODE_NORMAL:
            fbdev_set_rotate(rotate,h_flip,v_flip);
            printf("normal\n");
            if(player !=NULL){
                hcplayer_change_rotate_type(player, MIRROR_TYPE_NONE);
            }
         default:
            break;
    }

     //set hdmi rx/cvbs rx flip mode
#if PROJECTER_C2_VERSION
    if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI||projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI2)
#else
    if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI)
#endif
     {
#if  HDMI_RX_FLIP_SMOTHLY
        hdmi_rx_set_flip_mode(mode);
#else
        //hdmi_rx_leave();
        //hdmi_rx_enter();
#endif

    }
    else if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_CVBS){
        cvbs_rx_stop();
        cvbs_rx_start();
    }
    return 0;
}





