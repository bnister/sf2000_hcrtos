#include "volume.h"
#include "../screen.h"
#include "../setup/setup.h"
#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "../factory_setting.h"

lv_obj_t *volume_scr = NULL;
lv_group_t *volume_g = NULL;
lv_obj_t *volume_bar;
static lv_timer_t *timer = NULL, *timer_mute = NULL;
lv_group_t* pre_group=NULL;
lv_obj_t* prev_obj = NULL;
lv_obj_t *icon = NULL;
int volume_num = 0;

LV_IMG_DECLARE(volume_mute);
LV_IMG_DECLARE(volume_open);

void set_volume1(uint8_t vol){
    int snd_fd = -1;

    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0) {
        printf ("open snd_fd %d failed\n", snd_fd);
        return ;
    }                                                                 

    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &vol);
    
    vol = 0;
    ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &vol);
    printf("volume is %d", vol);
    close(snd_fd);
}

static void timer_handler(lv_timer_t* timer1){
    
    //change_screen(prev_scr);
     projector_set_some_sys_param(P_VOLUME, volume_num);
    projector_sys_param_save();
    lv_obj_del(volume_bar);
    volume_bar = NULL;
    lv_group_set_default(pre_group);
    lv_indev_set_group(indev_keypad, pre_group);
    lv_group_focus_obj(prev_obj);
#if PROJECTER_C2_VERSION
    if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI||projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI2)
#else
    if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI)
#endif
        fb_show_onoff(0);
    // last_scr = prev_scr;
    // _ui_screen_change(lv_disp_get_scr_prev(lv_disp_get_default()), 0, 0);
    timer = NULL;
}

static void timer_handler1(lv_timer_t* timer1){
    lv_obj_del(icon);
    timer_mute = NULL;
    icon = NULL;
}

static void event_handler(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
   

    if (code == LV_EVENT_KEY){
        lv_timer_pause(timer);
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_DOWN || key == LV_KEY_UP){
            uint8_t count = lv_obj_get_child_cnt(target);
            if(key == LV_KEY_UP && volume_num < 100 && count < (++volume_num)/4){
                create_balance_ball(target,8, 4);
            }
            if(key == LV_KEY_DOWN && volume_num > 0 && count > (--volume_num)/4){
                lv_obj_del(lv_obj_get_child(target, count-1));
            }
            char current_volume[5];
            memset(current_volume, 0, 5);
            sprintf(current_volume, "%d", volume_num);
            if (volume_num > -1 && volume_num < 101){
                lv_obj_t *sub = lv_obj_get_child(target->parent, 2);
                lv_obj_t* label = lv_obj_get_child(sub, 0);
                lv_label_set_text(label, current_volume);
                set_volume1(volume_num);
            }

        }
        if (key == LV_KEY_ESC || key == LV_KEY_ENTER){
           
            lv_timer_ready(timer);
            lv_timer_resume(timer);
            return ;
        }
        lv_timer_resume(timer);
        lv_timer_reset(timer);
    }
    // else if(code == LV_EVENT_FOCUSED){
    //     if(timer){
    //         lv_timer_reset(timer);
    //         lv_timer_resume(timer);
    //         return;
    //     }
        
    // // } else if(code == LV_EVENT_SCREEN_UNLOADED){
    // //     if(timer){
    // //         lv_timer_pause(timer);
    // //     }
    // //     return;
    // }

}


void volume_screen_init(void ){
    volume_scr =lv_layer_top();
    volume_g = lv_group_create();
    set_volume1(projector_get_some_sys_param(P_VOLUME));
    // lv_obj_set_style_bg_color(volume_scr, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    // lv_obj_clear_flag(volume_scr, LV_OBJ_FLAG_SCROLLABLE);
    // lv_obj_set_style_bg_color(volume_scr, lv_palette_lighten(LV_PALETTE_GREY,1), 0);
    // lv_obj_add_event_cb(volume_scr, event_handler, LV_EVENT_SCREEN_LOADED, 0);
    // lv_obj_add_event_cb(volume_scr, event_handler, LV_EVENT_SCREEN_UNLOADED, 0);
    
    // volume_g = lv_group_create();
    // lv_group_t* g = lv_group_get_default();
    // lv_group_set_default(volume_g);

    
    //lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
    
    // lv_group_set_default(g);
}

void del_volume(){
    if(timer){
        lv_timer_del(timer);
        timer = NULL;
    }
    if(volume_bar){
        lv_obj_del(volume_bar);
        volume_bar = NULL;
    }
}

void create_volume(){
    pre_group = lv_group_get_default();
    prev_obj = lv_group_get_focused(pre_group);
    lv_group_set_default(volume_g);
    lv_indev_set_group(indev_keypad, volume_g);
    volume_bar = create_display_bar_widget(volume_scr, 70, 9);
    lv_obj_set_style_bg_opa(volume_bar, LV_OPA_100, 0);
    //lv_obj_add_event_cb(volume, event_handler, LV_EVENT_ALL, 0);
    volume_num =  projector_get_some_sys_param(P_VOLUME);
    lv_obj_set_style_bg_opa(volume_bar, LV_OPA_80, 0);
    lv_obj_set_style_outline_width(volume_bar, 0, 0);
    static char* volume_str = {"Volume\0" "声音\0" "Volume"}; 
    create_display_bar_name_part(volume_bar, get_some_language_str(volume_str, projector_get_some_sys_param(P_OSD_LANGUAGE)), 25, 100);
    //lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
    printf("%d\n", projector_get_some_sys_param(P_VOLUME));
    lv_obj_t * container = create_display_bar_main(volume_bar, 65, 44,  projector_get_some_sys_param(P_VOLUME)/4, 4);
    //lv_group_add_obj(lv_group_get_default(), container);

    lv_obj_add_event_cb(container, event_handler, LV_EVENT_ALL, 0);

    create_display_bar_show(volume_bar, 10, 100, projector_get_some_sys_param(P_VOLUME));

    timer = lv_timer_create(timer_handler, 3000, container);
    lv_timer_set_repeat_count(timer,1);
    lv_timer_reset(timer);
}



void create_mute_icon(){
    static bool is_mute = true;
    if(is_mute){
        if(!icon){
            icon = lv_img_create(volume_scr);
            lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 30,30); 
        }
        lv_img_set_src(icon, &volume_mute);
        if(timer_mute){
            lv_timer_pause(timer_mute);
        }

        is_mute = false;   
    }else{
        if(!icon){
            icon = lv_img_create(volume_scr);
            lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 30,30); 
        }
        lv_img_set_src(icon, &volume_open);
        //lv_obj_clean(volume_scr);
        is_mute = true;
        if(timer_mute){
            lv_timer_resume(timer_mute);
            lv_timer_reset(timer_mute);
        }else{
            timer_mute = lv_timer_create(timer_handler1, 3000, NULL);
            lv_timer_set_repeat_count(timer_mute,1);
            lv_timer_reset(timer_mute);  
        }
    }
    
}
