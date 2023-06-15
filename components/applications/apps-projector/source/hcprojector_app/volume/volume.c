#include "volume.h"
#include "screen.h"
#include "setup.h"
#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include <hcuapi/input-event-codes.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "key_event.h"
#include "com_api.h"
#include "osd_com.h"

lv_obj_t *volume_scr = NULL;

lv_group_t *volume_g = NULL;
lv_obj_t *volume_bar;
static lv_timer_t *volume_timer = NULL, *timer_mute = NULL;
lv_group_t* pre_group=NULL;
lv_obj_t* prev_obj = NULL;
lv_obj_t *icon = NULL;
int volume_num = 0;

LV_IMG_DECLARE(volume_mute);
LV_IMG_DECLARE(volume_open);

static void timer_handler(lv_timer_t* timer1);
void set_volume1(uint8_t vol);
static int key_pre_proc(int key);
static void event_handler(lv_event_t* e);
#ifdef CVBS_AUDIO_I2SI_I2SO
// set i2si initial volume
static void set_i2si_volume(uint8_t vol){
    int snd_fd = -1;
	snd_fd = open("/dev/sndC0i2si", O_WRONLY);	
    if (snd_fd < 0) {
        printf ("open snd_fd %d failed\n", snd_fd);
        return ;
    } 
	
    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &vol);
	vol = 0;
    ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &vol);
    //printf("i2si volume is %d", vol);
	close(snd_fd);
}
#endif

/**
 * set i2so volume.
 * set bluetooth volume on C2 board.(special case)
 * set i2si volume in cvbs-in that using i2si->i2so apath.
*/
void set_volume1(uint8_t vol){
    int snd_fd = -1;
	
	//set i2so 
    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0) {
        printf ("open snd_fd %d failed\n", snd_fd);
        return ;
    }                                                                 
#ifdef BLUETOOTH_SUPPORT
#if PROJECTER_C2_D3000_VERSION
	if(api_get_bt_connet_status() == BT_CONNECT_STATUS_CONNECTED)
        bluetooth_set_music_vol(vol);
    else
    {
        if(vol>0)
            bluetooth_set_music_vol(60 + vol*3/10);
        else
            bluetooth_set_music_vol(0);
    }
#else
    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &vol);
#endif
#else
    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &vol);
#endif
    vol = 0;
    ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &vol);
    printf("%s volume is %d\n",__func__, vol);
    close(snd_fd);
	
#ifdef CVBS_AUDIO_I2SI_I2SO
	// set i2si vol
	//if(cvbs_is_playing())// in cvbs,maybe drop this condition 
	{
		snd_fd = open("/dev/sndC0i2si", O_WRONLY);
		if (snd_fd < 0) {
			printf ("open sndC0i2si failed\n");
			return ;
		}
		ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &vol);
		close(snd_fd);
	}
#endif
	
}

static void timer_handler(lv_timer_t* timer1){
    
    //change_screen(prev_scr);
     projector_set_some_sys_param(P_VOLUME, volume_num);
    projector_sys_param_save();
    if(volume_bar){
        lv_obj_del(volume_bar);
        volume_bar = NULL;        
    }

    set_keystone_disable(false);    
    if(lv_obj_is_valid(prev_obj)){
        lv_group_set_default(pre_group);
        lv_indev_set_group(indev_keypad, pre_group);
        lv_group_focus_obj(prev_obj);       
    }

// #if PROJECTER_C2_VERSION
//     if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI||projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI2)
// #else
//     if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI)
// #endif
    // last_scr = prev_scr;
    // _ui_screen_change(lv_disp_get_scr_prev(lv_disp_get_default()), 0, 0);
   volume_timer = NULL;
}

static void timer_handler1(lv_timer_t* timer1){
    if(icon){
        lv_obj_del(icon);
        icon = NULL;       
    }


    timer_mute = NULL;
}

static int key_pre_proc(int key){
    int key1 = USER_KEY_FLAG^key;
    if(key1 == KEY_VOLUMEUP){
        
        return LV_KEY_UP;
    }else if(key1 == KEY_VOLUMEDOWN){
        return LV_KEY_DOWN;
    }
    return key;
}

static void event_handler(lv_event_t* e){
     lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
   

    if (code == LV_EVENT_KEY){
        lv_timer_pause(volume_timer);
        uint32_t key = key_pre_proc(lv_indev_get_key(lv_indev_get_act())) ;
        if (key == LV_KEY_UP || key==LV_KEY_DOWN){
            int8_t count = lv_obj_get_child_cnt(target);
            if(key == LV_KEY_UP && count<33){
                count++;
                create_balance_ball(target,8, 3);
            }
            if(key == LV_KEY_DOWN && (--count)>=0){
                lv_obj_del(lv_obj_get_child(target, count));
            }
            volume_num = count>0 ? count*3 : 0;
            if (volume_num > -1 && volume_num < 101){
                lv_obj_t *sub = lv_obj_get_child(target->parent, 2);
                lv_obj_t* label = lv_obj_get_child(sub, 0);
                lv_label_set_text_fmt(label, "%d", volume_num/3);
                set_volume1(volume_num);
            }

        }
        if (key == LV_KEY_ESC || key == LV_KEY_ENTER){
           
            lv_timer_ready(volume_timer);
            lv_timer_resume(volume_timer);
            return ;
        }
        lv_timer_resume(volume_timer);
        lv_timer_reset(volume_timer);
    }

}


void volume_screen_init(void ){
    volume_scr =lv_layer_top();
    volume_g = lv_group_create();
    set_volume1(projector_get_some_sys_param(P_VOLUME));
#ifdef CVBS_AUDIO_I2SI_I2SO
	set_i2si_volume(projector_get_some_sys_param(P_VOLUME));
#endif
}

void del_volume(){
    if(volume_timer){
        lv_timer_del(volume_timer);
       volume_timer = NULL;
    }
    if(volume_bar){
        lv_obj_del(volume_bar);
        volume_bar = NULL;
        set_keystone_disable(false);
    }
}

void create_volume(){
    if(!volume_bar){
        set_keystone_disable(true);
        pre_group = lv_group_get_default();
        prev_obj = lv_group_get_focused(pre_group);
        lv_group_set_default(volume_g);
        lv_indev_set_group(indev_keypad, volume_g);
        volume_bar = create_display_bar_widget(volume_scr, 70, 9);
        //lv_obj_add_event_cb(volume, event_handler, LV_EVENT_ALL, 0);
        volume_num =  projector_get_some_sys_param(P_VOLUME);
        lv_obj_set_style_bg_opa(volume_bar, LV_OPA_60, 0);
        lv_obj_set_style_outline_width(volume_bar, 0, 0);
        create_display_bar_name_part(volume_bar, api_rsc_string_get(STR_VOLUME), 25, 100);
        //lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
        printf("%d\n", projector_get_some_sys_param(P_VOLUME));
        lv_obj_t * container = create_display_bar_main(volume_bar, 65, 44,  projector_get_some_sys_param(P_VOLUME)/3, 3);
        //lv_group_add_obj(lv_group_get_default(), container);

        lv_obj_add_event_cb(container, event_handler, LV_EVENT_ALL, 0);

        create_display_bar_show(volume_bar, 10, 100, projector_get_some_sys_param(P_VOLUME)/3);
        if(!volume_timer){
            volume_timer = lv_timer_create(timer_handler, 3000, container);
            lv_timer_set_repeat_count(volume_timer,1);
            lv_timer_reset(volume_timer);            
        }
        
    }else{
        if(volume_timer){
            lv_timer_reset(volume_timer);
        }
    }

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
