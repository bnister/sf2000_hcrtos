#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include <hcuapi/audsink.h>
#include <hccast/hccast_com.h>
#include <time.h>
#include "com_api.h"
#include "network_api.h"
#include "lvgl/lvgl.h"
#include "../lvgl/src/misc/lv_types.h"
#include "key.h"
#include "data_mgr.h"
#include "hotplug_mgr.h"
#include "osd_com.h"
#include "tv_sys.h"
#include "cast_api.h"

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
static char m_wifi_module_name[32];
#endif
#endif

static void exit_console(int signo)
{
    printf("%s(), signo: %d, error: %s\n", __FUNCTION__, signo, strerror(errno));

    cast_deinit();

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
    hotplug_deinit();
#endif
#endif
    api_watchdog_stop();

    exit(0);
}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
//    lv_obj_t * obj = lv_event_get_target(e);
    uint32_t vkey = VKEY_NULL;
    lv_group_t *group = lv_group_get_default();

    if (code == LV_EVENT_KEY)
    {
        uint32_t value = lv_indev_get_key(lv_indev_get_act());
        vkey = key_convert_vkey(value);

        if (V_KEY_UP == vkey)
        {
            lv_group_focus_prev(group);
        }
        else if (V_KEY_DOWN == vkey)
        {
            lv_group_focus_next(group);
        }
        else
        {
            api_control_send_key(vkey);
        }
    }
}

static void *watchdog_task(void *arg)
{
    while (1)
    {
        usleep(3 * 1000 * 1000);
        api_watchdog_feed();
    }
}

static void start_watchdog_task()
{
    pthread_t pid;

#ifdef WATCHDOG_KERNEL_FEED
    return ;
#endif

    pthread_create(&pid, NULL, watchdog_task, NULL);
}

#ifdef __linux__
int main(int argc, char *argv[])
#else
void * main_cast(void *arg)
#endif
{
    printf("Welcom to HC cast!\n");

#ifdef __linux__
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
    if (argc == 2)
    {
        strncpy(m_wifi_module_name, argv[1], sizeof(m_wifi_module_name) - 1);
        // wifi_api_set_module(m_wifi_module_name);
        printf("please modprobe %s!\n", m_wifi_module_name);
    }
#endif
#endif
#else // rtos
    api_romfs_resources_mount();
#endif // __linux__

    app_ffplay_init();
    api_logo_show(NULL);

#ifdef __linux__
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
    /* if wifi is exist, probe it */
    system("/etc/wifiprobe.sh &");
#endif
#endif // NETWORK_SUPPORT

    signal(SIGTERM, exit_console); //kill signal
    signal(SIGINT, exit_console); //Ctrl+C signal
    signal(SIGSEGV, exit_console); //segmentation fault, invalid memory
    signal(SIGBUS, exit_console);  //bus error, memory addr is not aligned.

#endif // __linux__

    start_watchdog_task();

    api_system_init();
    api_video_init();
    api_audio_init();
    //api_logo_show(BACK_LOGO);
    data_mgr_load();

    tv_sys_app_start_set(1);

#if CONFIG_APPS_PROJECTOR_SPDIF_OUT
    uint32_t snd_devs;
    snd_devs = AUDSINK_SND_DEVBIT_I2SO | AUDSINK_SND_DEVBIT_SPO;
    hccast_com_media_control(HCCAST_CMD_SND_DEVS_SET, snd_devs);
#endif


#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
#ifdef __linux__
    extern void lv_fb_hotplug_support_set(bool enable);
    lv_fb_hotplug_support_set(false);
#endif // __linux__    
    hotplug_init();
#endif
    //network_init();
    network_connect();
#endif // NETWORK_SUPPORT

    api_lvgl_init(OSD_MAX_WIDTH, OSD_MAX_HEIGHT);

#if 0 //frank test
    frank_demo_test();
#else
    hc_control_init();
#endif

    /*Handle LitlevGL tasks (tickless mode)*/
    while (1)
    {
#ifdef DRAW_UI_SYNC
        hc_control_process();
#endif
        lv_task_handler();
        usleep(1000);//frank, the sleep time will result in the OSD UI flush
    }

    printf("hccast_demo exit!\n");

    return 0;
}
