#include <stdio.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
/*#include "lv_gpu_hcge.h"*/
#include <fcntl.h>
#include <sys/ioctl.h>

#include <kernel/fb.h>
#include <hcuapi/fb.h>
#include <kernel/lib/console.h>
#include <sys/poll.h>
#include <hcuapi/input.h>
#include <hcuapi/input-event-codes.h>
#include <kernel/module.h>
#include <hcuapi/standby.h>
#include <hcuapi/dis.h>
#include "channel/local_mp/com_api.h"
#include <lvgl/hc_src/hc_lvgl_init.h>

#include "screen.h"
#include "factory_setting.h"
#include "./volume/volume.h"
#include "channel/local_mp/key.h"
#include "./screen.h"
#ifdef USBMIRROR_SUPPORT
#include "channel/usbmirror/cast_api.h"
#endif
static int fd_key;
static int fd_adc_key=-1;
// static lev_indev_t * indev_keypad;
lv_indev_t* indev_keypad;
static lv_group_t * g;
SCREEN_TYPE_E cur_scr = 0, last_scr=0;
SCREEN_TYPE_E prev_scr = 0;
static TaskHandle_t projector_thread = NULL;
void* lv_mem_adr=NULL;
static int key_init(void);
static int dis_show_onoff(bool on_off);
static bool is_mute = false;
static uint8_t vol_back = 0;

extern void check_usb_hotplug();

static uint8_t get_mp_i_src(){
    return lv_scr_act() == ui_mainpage ? 0 :
            lv_scr_act() == ui_subpage ? 1 :
            lv_scr_act() == ui_fspage ? 2 : 3;
}

void set_aspect_ratio(dis_tv_mode_e ratio){
   struct dis_aspect_mode aspect = { 0 };
    
   int fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return;
	}

    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    if(ratio == DIS_TV_4_3)
        aspect.dis_mode = DIS_LETTERBOX; //DIS_PANSCAN,  DIS_LETTERBOX
    else if(ratio == DIS_TV_16_9)
        aspect.dis_mode = DIS_PILLBOX; //DIS_PILLBOX, DIS_VERTICALCUT
    else if(ratio == DIS_TV_AUTO){
        aspect.dis_mode = DIS_NORMAL_SCALE;
    }
	ioctl(fd , DIS_SET_ASPECT_MODE , &aspect);

	close(fd);


}

void set_zoom(int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h){
    struct dis_zoom dz = { 0 };
    dz.layer = DIS_LAYER_MAIN;
    dz.distype = DIS_TYPE_HD;
    dz.src_area.x = s_x;
    dz.src_area.y = s_y;
    dz.src_area.w = s_w;
    dz.src_area.h = s_h;
    dz.dst_area.x = d_x;
    dz.dst_area.y = d_y;
    dz.dst_area.w = d_w;
    dz.dst_area.h = d_h;

    int fd = -1;

    fd = open("/dev/dis", O_WRONLY);
    if(fd < 0){
        return;
    }
    ioctl(fd , DIS_SET_ZOOM , &dz);
    close(fd);
}
 
static int dis_show_onoff(bool on_off)
{
    int fd = -1;
    struct dis_win_onoff winon = { 0 };

    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0) {
        return -1;
    }


    winon.distype = DIS_TYPE_HD;
    winon.layer =  DIS_LAYER_MAIN;
    winon.on = on_off ? 1 : 0;
    
    ioctl(fd, DIS_SET_WIN_ONOFF, &winon);
    close(fd);

    return 0;
}

int  fb_show_onoff(int  on)
{
    int fbfd = -1;
    
    return 0;// ddr latency
    
    fbfd = open("/dev/fb0", O_RDWR);
    if(fbfd == -1) {
    	perror("Error: cannot open framebuffer device");
    	return -1;
    }
    if(on){
        if (ioctl(fbfd, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
        	perror("ioctl(FBIOBLANK)");
             close(fbfd);
        	return -1;
        }
    }
    else{
        if (ioctl(fbfd, FBIOBLANK, FB_BLANK_NORMAL)!= 0) {
            perror("ioctl(FBIOBLANK)");
            close(fbfd);
            return  -1;
        }
    }
    close(fbfd);
    return 0;
}

//static SCREEN_TYPE_E projector_get_cur_screen();

void change_screen(enum SCREEN_TYPE stype)
{
	cur_scr = stype;
    switch (stype) {
    case SCREEN_CHANNEL:
        lv_group_set_default(channel_g);
        lv_indev_set_group(indev_keypad, channel_g);        
        fb_show_onoff(1);
        break;
    case SCREEN_SETUP:
        lv_group_set_default(setup_g);
        lv_indev_set_group(indev_keypad, setup_g);
        fb_show_onoff(1);
        break;
    // case SCREEN_VOLUME:
    //     lv_group_set_default(volume_g);
    //     lv_indev_set_group(indev_keypad, volume_g);
    //     break;
    default:
        lv_group_set_default(g);
        lv_indev_set_group(indev_keypad, g);
    }
}

/*enum SCREEN_TYPE projector_get_cur_screen()
{
	return cur_scr;
}*/
void _ui_screen_change(lv_obj_t * target,  int spd, int delay)
{
    lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_MOVE_TOP, spd, delay, false);
}

/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
	fd_key = open("/dev/input/event0", O_RDONLY);
    fd_adc_key = open("/dev/input/event1", O_RDONLY);
}

/* Description:
*        hotkeys proc, such as POWER/MENU/SETUP keys etc.
 * return 0:  hotkeys consumed by app, don't send to lvgl;  others: lvgl keys
 */
static uint32_t key_preproc(uint32_t act_key)
{
	uint32_t ret = 0;
        
      if(act_key != 0)
      {	
        	if(KEY_POWER == act_key){
                    enter_standby();
            }
        	else if(KEY_MENU/*KEY_CHANNEL*/ == act_key){
        		printf(">>Channel key\n");
        		change_screen(SCREEN_CHANNEL);
        	}
        	else if(KEY_EPG/*KEY_SETUP*/ == act_key){
        		printf(">>Setup key\n");      	
        		change_screen(SCREEN_SETUP);
                if(lv_scr_act() == setup_scr){
                    ret = act_key;
                }
        	}
        	else if(KEY_VOLUMEUP == act_key|| KEY_VOLUMEDOWN == act_key){
        		printf(">>Volume key\n");      			
        		if(lv_scr_act() == channel_scr || lv_scr_act() == setup_scr){
                    ;
                }else if(is_mute){
                    is_mute = !is_mute;
                    media_mute(is_mute);
                    projector_set_some_sys_param(P_VOLUME, vol_back);
                    create_mute_icon();
                    create_volume();
                    fb_show_onoff(1);
                }else if(lv_obj_get_child_cnt(lv_layer_top()) != 0){
                    ret = act_key;
                }
                else{ 
                     create_volume();                     
                     fb_show_onoff(1);
                    //change_screen(SCREEN_VOLUME);
                }  
        	}else if(KEY_MUTE == act_key){
                
                
                is_mute = !is_mute;
            
                if(is_mute){
                    vol_back = projector_get_some_sys_param(P_VOLUME);
                }
                media_mute(is_mute);
                if (is_mute){
                    projector_set_some_sys_param(P_VOLUME, 0);
                   
                }else{
                    projector_set_some_sys_param(P_VOLUME, vol_back);
                }
                
                
                 create_mute_icon();
            }else if(KEY_RIGHTSHIFT == act_key){
                set_next_flip_mode();
                projector_sys_param_save();
            }
            else {
        		printf(">>lvgl keys: %d\n", (int)act_key);//map2lvgl_key(act_key);
        		ret = act_key;
        	}	
    }

    return ret;
}

/* ir key code map to lv_keys*/
static uint32_t keypad_key_map2_lvkey(uint32_t act_key)
{
        switch(act_key) {
			case KEY_UP:
			    act_key = LV_KEY_UP;
			    break;
			case KEY_DOWN:
			    act_key = LV_KEY_DOWN;
			    break;
			case KEY_LEFT:
			    act_key = LV_KEY_LEFT;
			    break;
			case KEY_RIGHT:
			    act_key = LV_KEY_RIGHT;
			    break;
            case KEY_OK:
                act_key = LV_KEY_ENTER;
                break;
            case KEY_NEXT:
                act_key = LV_KEY_NEXT;
                break;
            case KEY_PREVIOUS:
                act_key = LV_KEY_PREV;
                break;
            case KEY_VOLUMEUP:
                act_key = LV_KEY_UP;
                break;
            case KEY_VOLUMEDOWN:
                act_key = LV_KEY_DOWN;
                break;
            case KEY_EXIT:
                act_key = LV_KEY_ESC;
                break;
            case KEY_EPG:
                act_key = LV_KEY_HOME;
                break;
            default:
        	    // act_key = 0;//0x10000/*USER_KEY_FLAG */| act_key;
                act_key = USER_KEY_FLAG | act_key;
        	    break;
        }
    return act_key;
}
/*Get the currently being pressed key.  0 if no key is pressed*/
static uint32_t keypad_read_key(lv_indev_data_t  *lv_key)
{
    /*Your code comes here*/
    struct pollfd pfd;
    struct input_event t;
    uint32_t ret = 0;
    static uint32_t pressed_key = 0;
    static uint32_t cnt = 0;
	memset(&t, 0, sizeof(t));
    pfd.fd = fd_key;
    pfd.events = POLLIN | POLLRDNORM;
    if(poll(&pfd, 1, 1) > 0)
    {
        if (read(fd_key, &t, sizeof(t)) != sizeof(t))
		return 0;
    }
	else
    {
        if(fd_adc_key>=0)
        {
            pfd.fd = fd_adc_key;
            pfd.events = POLLIN | POLLRDNORM;
            if (poll(&pfd, 1, 1) <= 0){
                    return 0;
            }
            if (read(fd_adc_key, &t, sizeof(t)) != sizeof(t))
            return 0;
        }
    }
    if(t.type == EV_KEY)
    {
        if(t.value == 1)// pressed
        {
                ret = key_preproc(t.code);
                lv_key->state = (ret==0)?LV_INDEV_STATE_REL:LV_INDEV_STATE_PR;
                pressed_key = ret;
                cnt = 0;
        }
        else if(t.value == 0)// released
        {
                lv_key->state = LV_INDEV_STATE_REL;
                ret = pressed_key;
                pressed_key = 0;
        }
    }
    else if(t.type == EV_MSC)// repeat key
    {
        cnt++;
        if(cnt%2 == 0 && pressed_key!=0){
            ret = pressed_key;
            lv_key->state = LV_INDEV_STATE_PR;
        }
    }

    lv_key->key = keypad_key_map2_lvkey(ret);
    return ret;
}

/*Will be called by the library to read the mouse*/
 void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    /*Get whether the a key is pressed and save the pressed key*/
    keypad_read_key(data);
}
static int key_init(void)
{
    static lv_indev_drv_t keypad_driver;

    keypad_init();
    lv_indev_drv_init(&keypad_driver);
    keypad_driver.type = LV_INDEV_TYPE_KEYPAD;
    keypad_driver.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&keypad_driver);

    g = lv_group_create();
    lv_group_set_default(g);
    lv_indev_set_group(indev_keypad, g);

    return 0;
}
static void message_ctrl_process(void)
{
    control_msg_t ctl_msg;
    screen_ctrl ctrl_fun = NULL;
//    lv_disp_t * dispp = lv_disp_get_default();
    lv_obj_t *screen;
    int ret = -1;

//    screen = dispp->act_scr;
    screen = lv_scr_act();
    do
    {
        ret = api_control_receive_msg(&ctl_msg);
        if (0 != ret){
            if (0 != ret)
            break;
        }
        if (screen)
        {
            ctrl_fun = api_screen_get_ctrl(screen);
            if (ctrl_fun)
                ctrl_fun((void*)&ctl_msg, NULL);
        }
    }while(0);
    check_usb_hotplug();
}

static void projector_task(void *pvParameters)
{
    /* system param init*/
    projector_factory_reset();
    projector_sys_param_load();
    api_system_init();
    app_ffplay_init();

    hc_lvgl_init();

    key_init();
#ifdef USBMIRROR_SUPPORT
	cast_usb_mirror_init();
#endif
    lv_disp_t * dispp = lv_disp_get_default();   
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE),  \
                                        lv_palette_main(LV_PALETTE_RED),  false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    projector_sys_param_load();

    channel_screen_init();
 
    // lv_disp_load_scr(channel_scr); 
    // lv_group_set_default(channel_g);
    // lv_indev_set_group(indev_keypad, channel_g);
   

    setup_screen_init();
    hdmi_screen_init();
    cvbs_screen_init();
    volume_screen_init();
    //local mp screen 
    ui_mainpage_screen_init();
    ui_subpage_screen_init();
    ui_fspage_screen_init();
	ui_ctrl_bar_screen_init();
	ui_ebook_screen_init();
    //lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_TRANSP, 0);
    //lv_obj_set_style_border_opa(lv_scr_act(), LV_OPA_TRANSP, 0);
    // Load default screen,  load from system param later.
   	#ifdef USBMIRROR_SUPPORT
    ui_um_play_init();
	#endif  
    last_scr = cur_scr = prev_scr = SCREEN_CHANNEL;
  /* if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI){
        hdmi_rx_enter();
   }*/
   change_screen(projector_get_some_sys_param(P_CUR_CHANNEL));
   static uint8_t mp_i_scr = 0;
   dis_show_onoff(0);
    /*Handle LitlevGL tasks (tickless mode)*/
    BT_first_power_on();
    while(1) {
            if(cur_scr != last_scr)
            {
                del_volume();
                del_setup_slave_scr_obj();
                //printf("MAX_SCR_NUM: %d \n", MAX_SCR_NUM);
                switch(cur_scr)
                {
                    case SCREEN_CHANNEL:
                            if(last_scr == SCREEN_CHANNEL_MP){
                                mp_i_scr = get_mp_i_src();
                            }
                           printf("cur scr is %d\n", mp_i_scr);
                            _ui_screen_change(channel_scr,0,0);
                            hdmi_rx_leave();
                            break;
                    case SCREEN_SETUP:
                            if(last_scr == SCREEN_CHANNEL_MP){
                                mp_i_scr = get_mp_i_src();
                            }
                            hdmi_rx_leave();
                           printf("cur scr is %d\n", mp_i_scr);
                            _ui_screen_change(setup_scr,0,0);
                            break;
                    case SCREEN_CHANNEL_HDMI:
                            _ui_screen_change(hdmi_scr, 0, 0);
                            hdmi_rx_enter();
                            break;
                    #if PROJECTER_C2_VERSION
                    case SCREEN_CHANNEL_HDMI2:
                            _ui_screen_change(hdmi_scr, 0, 0);
                            hdmi_rx_enter();
                            break;
                    #endif
                    case SCREEN_CHANNEL_CVBS:
                            _ui_screen_change(cvbs_scr, 0, 0);
                            break;
                    case SCREEN_CHANNEL_MP : 

                            if(mp_i_scr == 0){
                                _ui_screen_change(ui_mainpage, 0, 0);
                              
                            }else if(mp_i_scr == 1){
                                _ui_screen_change(ui_subpage, 0, 0);
                                // bool has_flag = false;
                                // if (lv_obj_has_flag(ui_fspage, LV_OBJ_FLAG_HIDDEN)){
                                //     lv_obj_clear_flag(ui_fspage, LV_OBJ_FLAG_HIDDEN);
                                //     has_flag = true;
                                // }
                                // if(has_flag){
                                //     lv_obj_add_flag(ui_fspage, LV_OBJ_FLAG_HIDDEN);
                                // }
                            }else{
                                _ui_screen_change(ui_fspage, 0, 0);
                            }
                            break; 
					#ifdef USBMIRROR_SUPPORT    
            		case SCREEN_CHANNEL_USB_CAST:
		                if (ui_um_play)
		                    _ui_screen_change(ui_um_play, 0, 0);
		                break;
        			#endif   
                    default:break;
                }
              
                    prev_scr = last_scr;
                    last_scr = cur_scr;
             
            }
    		//medai_message_ctrl_process();	
    		message_ctrl_process();
            lv_task_handler();
            usleep(10000);
	/*lv_tick_inc(2);*/
    }

}

static void lvgl_exit(void)
{
	lv_deinit();
}
static int lvgl_stop(int argc, char **argv)
{
	struct fb_var_screeninfo var;
	int fbfd = 0;

	fbfd = open(FBDEV_PATH, O_RDWR);
	if (fbfd == -1)
		return -1;

	ioctl(fbfd, FBIOBLANK, FB_BLANK_NORMAL);
	ioctl(fbfd, FBIOGET_VSCREENINFO, &var);
	var.yoffset = 0;
	var.xoffset = 0;
	ioctl(fbfd, FBIOPUT_VSCREENINFO, &var);
	close(fbfd);

	if (projector_thread != NULL)
		vTaskDelete(projector_thread);
	lvgl_exit();

	fbdev_exit();

	return 0;
}
static int projector_start(int argc, char **argv)
{
	// start projector main task.
	xTaskCreate(projector_task, (const char *)"projector_solution", 0x2000/*configTASK_STACK_DEPTH*/,
		    NULL, portPRI_TASK_NORMAL, &projector_thread);
		    return 0;
}

static int projector_auto_start(void)
{
	projector_start(0, NULL);
	return 0;
}

__initcall(projector_auto_start)

