#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <lvgl/lvgl.h>
#include <errno.h>

#include <time.h>
#include "com_api.h"
//#include "wifi_api.h"
#include "../lvgl/lvgl.h"
#include "../lvgl/src/misc/lv_types.h"
#include "key.h"
#include "data_mgr.h"

#include "osd_com.h"
#include <sys/time.h>
#include <kernel/fb.h>
#include <hcuapi/fb.h>
#include <kernel/lib/console.h>
#include <kernel/module.h>

static char m_wifi_module_name[32];
void app_ffplay_init(void);
#ifdef DRAW_UI_SYNC
void hc_control_process(void);
#endif
void hc_lvgl_main(void *pvParameters);
extern void lvgl_exit(void);
extern void fbdev_exit(void);
#ifndef FBDEV_PATH
#define FBDEV_PATH  "/dev/fb0"
#endif

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
//    lv_obj_t * obj = lv_event_get_target(e);
    uint32_t vkey = VKEY_NULL;
    lv_group_t *group = lv_group_get_default();

    if(code == LV_EVENT_KEY){
        uint32_t value = lv_indev_get_key(lv_indev_get_act());
        vkey = key_convert_vkey(value);

        if (V_KEY_UP == vkey){
            lv_group_focus_prev(group);
        } else if (V_KEY_DOWN == vkey){
            lv_group_focus_next(group);
        }else {
            api_control_send_key(vkey);
        }
    }
}


#if 0
static void lv_example_list_frank(void)
{
    /*Create a list*/
    static lv_obj_t *test_root = NULL;

    test_root = lv_obj_create(lv_scr_act());

    static lv_group_t *test_group;
    static lv_obj_t *list1;

    key_init();

    test_group = lv_group_create();
    key_regist(test_group);

    osd_draw_background(test_root, false);

    list1 = lv_list_create(test_root);
    lv_obj_set_size(list1, 180, 220);
    lv_obj_center(list1);

/*
    static lv_obj_t *btn11;
    uint8_t test_str[16];
    int i;
    for (i = 0; i < 1000; i ++){
        sprintf(test_str, "file %d", i);
        printf("add btn %d\n", i);
        //lv_list_add_text(m_list_media, "File");
        btn11 = lv_list_add_btn(list1, LV_SYMBOL_FILE, test_str);
        lv_obj_add_event_cb(btn11, event_handler, LV_EVENT_KEY, NULL);
    }
*/
    /*Add buttons to the list*/
    lv_obj_t * btn;
    lv_list_add_text(list1, "File");
    btn = lv_list_add_btn(list1, LV_SYMBOL_FILE, "New");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_DIRECTORY, "Open");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_SAVE, "Save");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_CLOSE, "Delete");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_EDIT, "Edit");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);

    lv_list_add_text(list1, "Connectivity");
    btn = lv_list_add_btn(list1, LV_SYMBOL_BLUETOOTH, "Bluetooth");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_GPS, "Navigation");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_USB, "USB");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_BATTERY_FULL, "Battery");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);

    lv_list_add_text(list1, "Exit");
    btn = lv_list_add_btn(list1, LV_SYMBOL_OK, "Apply");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_CLOSE, "Close");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_KEY, NULL);
}

void frank_demo_test()
{
//    lv_demo_music();
//    lv_example_list_1();
      lv_example_list_frank();   
}
#endif

static int lvgl_main(int argc, char *argv[])
{
    printf("Welcom to HC cast!\n");
    
    if (argc == 2){
        strncpy(m_wifi_module_name, argv[1], sizeof(m_wifi_module_name)-1);
        /*wifi_api_set_module(m_wifi_module_name);*/
        printf("please modprobe %s!\n", m_wifi_module_name);
    }

#if 0
    media_play_test();
#else    
    api_system_init();
    api_video_init();
    api_audio_init();
//    api_logo_show(BACK_LOGO);

    app_ffplay_init();
    data_mgr_load();

    api_lvgl_init(OSD_MAX_WIDTH, OSD_MAX_HEIGHT);

#if 0 //frank test
    frank_demo_test();
#else    
    hc_control_init();
#endif    
    
#endif    

    /*lv_example_sjpg_1();*/

    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {

    #ifdef DRAW_UI_SYNC
	hc_control_process();
    #endif
        
        lv_task_handler();
        usleep(1000);//frank, the sleep time will result in the OSD UI flush
    }

    printf("hccast_demo exit!\n");
    return 0;
}

void hc_lvgl_main(void *pvParameters)
{
	lvgl_main(0, NULL);
}

static TaskHandle_t lvgl_thread = NULL;
static int lvgl_start(int argc, char **argv)
{
	xTaskCreate(hc_lvgl_main, (const char *)"lvgl_main", configTASK_STACK_DEPTH,
		    NULL, portPRI_TASK_NORMAL, &lvgl_thread);

	return 0;
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

	if (lvgl_thread != NULL)
		vTaskDelete(lvgl_thread);
	lvgl_exit();

	fbdev_exit();

	return 0;
}

CONSOLE_CMD(lvgl_start, NULL, lvgl_start, CONSOLE_CMD_MODE_SELF, "start lvgl music demo")
CONSOLE_CMD(lvgl_stop, NULL, lvgl_stop, CONSOLE_CMD_MODE_SELF, "stop lvgl music demo")

static int lvgl_auto_start(void)
{
	lvgl_start(0, NULL);
	return 0;
}

__initcall(lvgl_auto_start)
