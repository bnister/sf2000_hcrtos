#include "app_config.h"

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

#ifdef __linux__
#include <linux/fb.h>
#include <linux/input.h>
#else
#include <kernel/fb.h>
#include <kernel/lib/console.h>
#include <kernel/module.h>
#endif
#include <hcuapi/input.h>

#include <hcuapi/fb.h>
#include <sys/poll.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/standby.h>
#include <hcuapi/dis.h>
#include <hcuapi/audsink.h>

#ifdef CAST_SUPPORT
#include <hccast/hccast_com.h>
#endif
#include <hcuapi/gpio.h>
#include "com_api.h"
#include <lvgl/hc_src/hc_lvgl_init.h>
#include <hcuapi/sys-blocking-notify.h>
#include "setup.h"
#include "screen.h"
#include "factory_setting.h"
#include "./volume/volume.h"
#include "key_event.h"
#include "channel/local_mp/mp_mainpage.h"
#include "channel/local_mp/mp_fspage.h"
#include "channel/local_mp/mp_ctrlbarpage.h"
#include "channel/local_mp/local_mp_ui.h"
#include "channel/local_mp/mp_bsplayer_list.h"
#include "vmotor.h"
#include "tv_sys.h"
#include "channel/cast/win_cast_root.h"
#include "osd_com.h"
#include "mul_lang_text.h"
#if defined(USBMIRROR_SUPPORT) || defined(AIRCAST_SUPPORT) || defined(MIRACAST_SUPPORT)
#include "channel/cast/cast_api.h"
#endif

#ifdef WIFI_SUPPORT
#include "network_api.h"
#endif


static int fd_key;
static int fd_adc_key=-1;
uint32_t act_key_code = -1;
lv_indev_t* indev_keypad;
static lv_group_t * g;
SCREEN_TYPE_E cur_scr = 0, last_scr=0;
SCREEN_TYPE_E prev_scr = 0;

#ifdef __HCRTOS__
static TaskHandle_t projector_thread = NULL;
#endif

void* lv_mem_adr=NULL;
static int key_init(void);
bool is_mute = false;
static uint8_t vol_back = 0;
static bool remote_control_disable = false;
void set_remote_control_disable(bool b);

void set_remote_control_disable(bool b){
    remote_control_disable = b;
}


//static SCREEN_TYPE_E projector_get_cur_screen();

void key_set_group(lv_group_t *key_group)
{
    lv_group_set_default(key_group);
    lv_indev_set_group(indev_keypad, key_group);        
}


void change_screen(enum SCREEN_TYPE stype)
{
#ifndef MAIN_PAGE_SUPPORT  //disable main page
	if(stype==SCREEN_CHANNEL_MAIN_PAGE)
        return ;
#endif	
#ifndef CVBSIN_SUPPORT  //disable cvbs page
	if(stype==SCREEN_CHANNEL_CVBS)
		return ;
#endif	

    cur_scr = stype;
    switch (stype) {
    case SCREEN_CHANNEL:
        break;
    case SCREEN_SETUP:
        break;
    // case SCREEN_VOLUME:
    //     lv_group_set_default(volume_g);
    //     lv_indev_set_group(indev_keypad, volume_g);
    //     break;
    case SCREEN_CHANNEL_MAIN_PAGE:
        break; 
#ifdef HDMIIN_SUPPORT		
    case SCREEN_CHANNEL_HDMI:
        break;
#endif		
    case SCREEN_CHANNEL_CVBS:
       
        break;
   
#ifdef WIFI_SUPPORT   
    case SCREEN_WIFI:
        break;        
#endif        

    default:
        key_set_group(g);
		break;
    }
}

/*enum SCREEN_TYPE projector_get_cur_screen()
{
	return cur_scr;
}*/
static lv_obj_t *m_last_scr = NULL;
void _ui_screen_change(lv_obj_t * target,  int spd, int delay)
{
    m_last_scr = lv_scr_act();
    lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_MOVE_TOP, spd, delay, false);
}

lv_obj_t *api_scr_go_back(void)
{
    if (m_last_scr)
        _ui_screen_change(m_last_scr, 0, 0);
}

/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
	fd_key = open("/dev/input/event0", O_RDONLY);
    fd_adc_key = open("/dev/input/event1", O_RDONLY);
}
int get_ir_key(void)
{
    return fd_key;
}

int get_adk_key(void)
{
    return fd_adc_key;
}

// int is_epg_code(){//当key code为epg时，返回code否则返回0
//     if(act_key_code == KEY_EPG){
//         act_key_code = -1;
//         return KEY_EPG;
//     }
//     return 0;
// }


/* Description:
*        hotkeys proc, such as POWER/MENU/SETUP keys etc.
 * return 0:  hotkeys consumed by app, don't send to lvgl;  others: lvgl keys
 */
static uint32_t key_preproc(uint32_t act_key)
{
	uint32_t ret = 0;
        
    //check if the key is disabled for hot key
    if (!api_hotkey_enable_get(act_key))
        return act_key;

    if(act_key != 0)
   {	
        act_key_code = act_key;
        if(remote_control_disable){
            
        }
    	else if(KEY_POWER == act_key){
        #ifdef LVGL_MBOX_STANDBY_SUPPORT
            win_open_lvmbox_standby();
            ret = V_KEY_POWER;
        #else
            enter_standby();
        #endif
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
            ctrlbar_reset_mpbacklight(); //key to reset backlight 
    		if(lv_scr_act() == channel_scr || lv_scr_act() == setup_scr){
                ;
            }else if(is_mute){
                is_mute = !is_mute;
                api_media_mute(is_mute);
                projector_set_some_sys_param(P_VOLUME, vol_back);
                create_mute_icon();
                create_volume();
                ret = act_key;
            }else if(volume_bar){
                ret = act_key;
            }
            else{ 
                 create_volume();                     
                //change_screen(SCREEN_VOLUME);
                ret = act_key;
            }  
    	}else if(KEY_MUTE == act_key){
            is_mute = !is_mute;
            ctrlbar_reset_mpbacklight();
            if(is_mute){
                vol_back = projector_get_some_sys_param(P_VOLUME);
            }
            api_media_mute(is_mute);
            if (is_mute){
                projector_set_some_sys_param(P_VOLUME, 0);
               
            }else{
                projector_set_some_sys_param(P_VOLUME, vol_back);
            }
            
            create_mute_icon();
            ret = act_key;
        }
        else if(KEY_ROTATE_DISPLAY  == act_key || act_key == KEY_FLIP){
            printf("act_key %d\n", (int)act_key);
            set_next_flip_mode();
            projector_sys_param_save();
            api_logo_reshow();
        }else if(KEY_KEYSTONE == act_key && cur_scr != SCREEN_SETUP){
            change_keystone();
        }
    #ifdef MIRROR_ES_DUMP_SUPPORT
        //dump mirror ES data to U-disk
        else if(KEY_BLUE == act_key)
        {
            extern void api_mirror_dump_enable_set(bool enable);
            static bool dump_enable = true;
            if (USB_STAT_MOUNT == mmp_get_usb_stat())
            {
                if (dump_enable)
                    create_message_box("Enable mirror ES dump!");
                else
                    create_message_box("Disable mirror ES dump!");
            }
            else
            {
                dump_enable = false;
                create_message_box("No USB-disk, disable mirror ES dump!");
            }
            api_mirror_dump_enable_set(dump_enable);
            dump_enable = !dump_enable;
        }
    #endif
        else if(act_key == KEY_HOME)
        {
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
			ret = act_key;
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
    case KEY_ENTER:
        act_key = LV_KEY_ENTER;
        break;
    case KEY_NEXT:
        act_key = LV_KEY_NEXT;
        break;
    case KEY_PREVIOUS:
        act_key = LV_KEY_PREV;
        break;
    // case KEY_VOLUMEUP:
    //     act_key = LV_KEY_UP;
    //     break;
    // case KEY_VOLUMEDOWN:
    //     act_key = LV_KEY_DOWN;
    //     break;
    case KEY_EXIT:
        act_key = LV_KEY_ESC;
        break;
    case KEY_ESC:
        act_key = LV_KEY_ESC;
        break;
    case KEY_EPG:
        act_key = LV_KEY_HOME;
        break;
    #ifdef PROJECTOR_VMOTOR_SUPPORT
    case KEY_CAMERA_FOCUS:
        ctrlbar_reset_mpbacklight();    //reset backlight with key 
        vMotor_Roll_cocus();
        act_key = 0;
        break;
    case KEY_FORWARD:
    case KEY_BACK:
        ctrlbar_reset_mpbacklight();    //reset backlight with key
        vMotor_set_step_count(192);
        if(act_key==KEY_FORWARD){
            vMotor_set_direction(BMOTOR_STEP_FORWARD);
        }else{
            vMotor_set_direction(BMOTOR_STEP_BACKWARD);
        }
        act_key = 0;
        break;
    #endif
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
    struct input_event *t;
    uint32_t ret = 0;
    KEY_MSG_s *key_msg = NULL;

    key_msg = api_key_msg_get();
    if (!key_msg){
        return 0;
    }
    t = &key_msg->key_event;

    // printf("key_type = %d t->value =%d adc_key_count =%d\n",key_msg ->key_type,t->value,adc_key_count);
    if(t->value == 1)// pressed
    {
        ret = key_preproc(t->code);
        lv_key->state = (ret==0)?LV_INDEV_STATE_REL:LV_INDEV_STATE_PR;
    }
    else if(t->value == 0)// released
    {
            ret = t->code;
            lv_key->state = LV_INDEV_STATE_REL;
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

static void com_message_process(control_msg_t *ctl_msg)
{
    if (!ctl_msg)
        return;

    switch(ctl_msg->msg_type)
    {
#ifdef CAST_SUPPORT
    case MSG_TYPE_AIR_INVALID_CERT:
    {
        extern bool cast_is_demo(void);
        static lv_obj_t *demo_label = NULL;
        if (NULL == demo_label)
        {   //demo_label = lv_label_create(main_page_scr);
            demo_label = lv_label_create(lv_layer_top());
            
            lv_obj_set_style_text_font(demo_label, &lv_font_montserrat_22, 0); 
            lv_obj_set_style_text_color(demo_label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_align(demo_label, LV_ALIGN_TOP_LEFT, 30, 30);
            if (cast_is_demo())
                lv_label_set_text(demo_label, "demo");
            else
                lv_label_set_text(demo_label, " ");
        }
        break;
    }
    case MSG_TYPE_HDMI_TX_CHANGED:
    {
        extern void restart_air_service_by_hdmi_change(void);
        restart_air_service_by_hdmi_change();
        break;
    }
#endif
#ifdef AUTO_HTTP_UPGRADE_SUPPORT    
    case MSG_TYPE_NET_UPGRADE:
    {
        if (ui_network_upgrade){
            win_upgrade_type_set(ctl_msg->msg_type);
            _ui_screen_change(ui_network_upgrade,0,0);
        }
        break;
    }
#endif

    default:
        break;
    }

}
/**
 * @description: Logic handle for removing and inserting storage devices 
 * @param :
 * @return {*}
 * @author: Yanisin
 */
static void stroage_hotplug_handle() //do it in while(1)
{
    static int count = 0;
    int usb_state=mmp_get_usb_stat();
    partition_info_t * cur_partition_info=mmp_get_partition_info();
    static int m_stroage_count = 0;
    if(cur_partition_info==NULL){
        return ;
    }
    if(m_stroage_count!=cur_partition_info->count){
        if(m_stroage_count<cur_partition_info->count){  //mean mount stroage dev 
            if(cur_scr==SCREEN_CHANNEL_MP&&lv_scr_act()==ui_mainpage){
                lv_event_send(mp_statebar,LV_EVENT_REFRESH,0);
            }
            #ifdef USB_AUTO_UPGRADE
            sys_upg_usb_check(1000);
            #endif
        }else if(m_stroage_count>cur_partition_info->count){    //mean umount stroage dev
            if(cur_scr ==SCREEN_CHANNEL_MP){
                if(lv_scr_act()==ui_mainpage){
                    lv_event_send(mp_statebar,LV_EVENT_REFRESH,0);
                }else if(lv_scr_act()==ui_subpage){
                    _ui_screen_change(ui_mainpage,0, 0);
                }else if(lv_scr_act()==ui_fspage||lv_scr_act()==ui_ctrl_bar||lv_scr_act()==ui_ebook_txt){
                    if(api_check_partition_used_dev_ishotplug()){
                        _ui_screen_change(ui_mainpage,0, 0);
                        app_media_list_all_free();
                        clear_all_bsplayer_mem();//for bs player music filelist 
                    }
                }
            }else if(cur_scr ==  SCREEN_SETUP || cur_scr == SCREEN_CHANNEL){
                if(api_check_partition_used_dev_ishotplug()){
                    media_player_close();
                    app_set_screen_submp(SCREEN_SUBMP0);
                }
            }
            // refresh the filelist_t in other scene
            if(api_check_partition_used_dev_ishotplug()){
                    app_media_list_all_free();
                    clear_all_bsplayer_mem(); 
            }
        }
        m_stroage_count = cur_partition_info->count;
    }
}

static void loop_watchdog_feed(void)
{
    static int counter = 0;
    if (counter ++ > 80){
    #ifdef __HCRTOS__
      #ifndef WATCHDOG_KERNEL_FEED
        api_watchdog_feed();
      #endif
    #else
        api_watchdog_feed();
    #endif
        counter = 0;
    }
}


static void message_ctrl_process(void)
{
    control_msg_t ctl_msg = {0,};
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

    com_message_process(&ctl_msg);

    stroage_hotplug_handle();

    loop_watchdog_feed();
}


#ifdef __HCRTOS__
static void projector_task(void *pvParameters)
#else
int main(int argc, char *argv[])
#endif
{
    printf("\n\n ********* Welcome to Hichip world! *********\n\n");    

   /* system param init*/
    projector_factory_init();
    projector_sys_param_load();
    api_system_init();
    app_ffplay_init();

    hc_lvgl_init();
    tv_sys_app_start_set(1);

    key_init();
	gpio_configure(PINPAD_T13,GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_T13,1);
    #ifdef SYS_ZOOM_SUPPORT
    printf("sys zoom support\n");
    sys_scala_init();
    #endif

//Init cast here to get cerificated information in advance
#ifdef AIRCAST_SUPPORT    
    hccast_air_service_init(hccast_air_callback_event);
#endif    

#ifdef USBMIRROR_SUPPORT  
    cast_usb_mirror_init(); 
#endif

#ifdef CAST_SUPPORT
  #if CONFIG_APPS_PROJECTOR_SPDIF_OUT
    uint32_t snd_devs;
    snd_devs = AUDSINK_SND_DEVBIT_I2SO | AUDSINK_SND_DEVBIT_SPO;
    hccast_com_media_control(HCCAST_CMD_SND_DEVS_SET, snd_devs);
  #endif
#endif    

#ifdef WIFI_SUPPORT
    //service would be enabled in cast UI
    network_service_enable_set(false);
    network_connect();
#endif    

#ifdef __linux__
    api_usb_dev_check();
#endif    

    lv_disp_t * dispp = lv_disp_get_default();   
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE),  \
                                        lv_palette_main(LV_PALETTE_RED),  false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    projector_sys_param_load();
    setup_screen_init();
    channel_screen_init();

#ifdef WIFI_SUPPORT    
    ui_network_upgrade_init();
#endif    

#ifdef DLNA_SUPPORT
    ui_cast_dlna_init();
#endif

#if defined(MIRACAST_SUPPORT) || defined(AIRCAST_SUPPORT) 
    ui_wifi_cast_init();
    ui_cast_play_init();
#endif    

    // lv_disp_load_scr(channel_scr); 
    // lv_group_set_default(channel_g);
    // lv_indev_set_group(indev_keypad, channel_g);
#ifdef  USBMIRROR_SUPPORT
	ui_um_play_init();
#endif
   
    hdmi_screen_init();
    cvbs_screen_init();
#ifdef WIFI_SUPPORT          
    wifi_screen_init();    
#endif	
    main_page_init();
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
   
    last_scr = cur_scr = prev_scr = SCREEN_CHANNEL;
  /* if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI){
        hdmi_rx_enter();
   }*/
   //projector_set_some_sys_param(, SCREEN_CHANNEL_MAIN_PAGE);
   change_screen(projector_get_some_sys_param(P_CUR_CHANNEL));
   api_dis_show_onoff(0);
    /*Handle LitlevGL tasks (tickless mode)*/
    
    api_key_get_init();
    
    while(1) 
    {
        if(cur_scr != last_scr)
        {
            del_volume();
            del_setup_slave_scr_obj();
        #ifdef LVGL_MBOX_STANDBY_SUPPORT
            win_del_lvmbox_standby();
        #endif
            //printf("MAX_SCR_NUM: %d \n", MAX_SCR_NUM);
            switch(cur_scr)
            {
            case SCREEN_CHANNEL:
                _ui_screen_change(channel_scr,0,0);
                break;
            case SCREEN_SETUP: 
                _ui_screen_change(setup_scr,0,0);
                break;
         #ifdef  WIFI_SUPPORT    
            case SCREEN_WIFI:
                _ui_screen_change(wifi_scr, 0, 0);
                break;

            case SCREEN_CHANNEL_WIFI_CAST:
                if (ui_wifi_cast_root){
                    _ui_screen_change(ui_wifi_cast_root, 0, 0);
                }
                break;
        #endif

        #ifdef  USBMIRROR_SUPPORT    
            case SCREEN_CHANNEL_USB_CAST:
                if (ui_um_play){
                    _ui_screen_change(ui_um_play, 0, 0);
                }
                break;
            #endif   
#ifdef HDMIIN_SUPPORT			
            case SCREEN_CHANNEL_HDMI:
                media_player_close();
                _ui_screen_change(hdmi_scr, 0, 0);
                break;
        #if PROJECTER_C2_VERSION
            case SCREEN_CHANNEL_HDMI2:
                _ui_screen_change(hdmi_scr, 0, 0);
                break;
        #endif
#endif		
            case SCREEN_CHANNEL_CVBS:
                media_player_close();
                _ui_screen_change(cvbs_scr, 0, 0);
                break;
            case SCREEN_CHANNEL_MAIN_PAGE:
                _ui_screen_change(main_page_scr, 0, 0);
                media_player_close();

            #if defined(CAST_SUPPORT)&&defined(WIFI_SUPPORT)
				cast_stop_service();
            #endif

                //hdmi_rx_leave();
                //cvbs_rx_stop();
                break;
            case SCREEN_CHANNEL_MP : 
                //hdmi_rx_leave();
                //cvbs_rx_stop();
                switch(get_screen_submp())
                {
                case SCREEN_SUBMP0 : 
                    _ui_screen_change(ui_mainpage, 0, 0);
                    break;
                case SCREEN_SUBMP1: 
                    _ui_screen_change(ui_subpage, 0, 0);
                    break;
                case SCREEN_SUBMP2: 
                    _ui_screen_change(ui_fspage, 0, 0);
                    break;  
                case SCREEN_SUBMP3:
                    if(mp_get_cur_player_hdl()==NULL)     //from other page enter 
                    {
                        _ui_screen_change(ui_mainpage, 0, 0);
                        break;
                    }
                    _ui_screen_change(ui_ctrl_bar, 0, 0);
                    break;                                                                                                      
                }
                break;
                default:break;
            }
      
            prev_scr = last_scr;
            last_scr = cur_scr;
         
        }
    
        message_ctrl_process(); 
    #ifdef BACKLIGHT_MONITOR_SUPPORT
        api_pwm_backlight_monitor_loop();
    #endif
        lv_task_handler();           

        usleep(10000);

    #ifdef BLUETOOTH_SUPPORT
        static bool bt_first = true;
        if(bt_first){
            BT_first_power_on();
            bt_first = false;
        }                
    #endif
	/*lv_tick_inc(2);*/
    }

}

#ifdef __HCRTOS__
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
    printf("\n******** Welcome to HC cast world! ********n\n");

	projector_start(0, NULL);
	return 0;
}

__initcall(projector_auto_start)

#endif