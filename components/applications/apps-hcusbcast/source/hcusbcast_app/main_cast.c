#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include <hcuapi/audsink.h>
#include <hccast/hccast_com.h>
#include <time.h>
#include "com_api.h"
#include "lvgl/lvgl.h"
#include "../lvgl/src/misc/lv_types.h"
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
    api_watchdog_stop();

    exit(0);
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

    app_ffplay_init();
    
#ifdef __linux__
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


#ifdef __linux__
    extern void lv_fb_hotplug_support_set(bool enable);
    lv_fb_hotplug_support_set(false);
#endif // __linux__    
    hotplug_init();
    api_lvgl_init(OSD_MAX_WIDTH, OSD_MAX_HEIGHT);

#if 0 //frank test
    frank_demo_test();
#else
    hc_control_init();
#endif

    /*Handle LitlevGL tasks (tickless mode)*/
    while (1)
    {
        hc_control_process();
        lv_task_handler();
        usleep(1000);//frank, the sleep time will result in the OSD UI flush
    }

    printf("hccast_demo exit!\n");

    return 0;
}
