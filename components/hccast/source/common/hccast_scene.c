#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#ifdef SUPPORT_AIRCAST
#ifdef HC_RTOS
#include <aircast/aircast_api.h>
#else
#include <hccast/aircast_api.h>
#endif
#include <hccast_air.h>
#endif
#include <hccast_media.h>
#include <hccast_scene.h>
#include <hccast_log.h>
#include <hccast_wifi_mgr.h>
#include <hccast_mira.h>
#include <hccast_dlna.h>
#include <hccast_air.h>
#include <hccast_net.h>

static int current_scene = HCCAST_SCENE_NONE;
static int last_scene = HCCAST_SCENE_NONE;
static int last_scene_switch_tick = 0;
static int scene_switching = 0;
static pthread_mutex_t g_mira_scene_mutex = PTHREAD_MUTEX_INITIALIZER;

static int scene_mira_check_tick = 0;
static int scene_mira_need_restart = 0;
static int scene_mira_begin_restarting = 0;
hccast_scene_air_event_callback g_scene_air_func;


void hccast_scene_air_event_init(hccast_scene_air_event_callback air_cb)
{
    if(air_cb)
    {
        g_scene_air_func = air_cb;
        hccast_log(LL_INFO,"%s init scene air event\n",__func__);
    }
}

void hccast_scene_air_event_callback_func(int event_type, void* param)
{
    if (g_scene_air_func)
    {
        g_scene_air_func(event_type, param);
    }
}

unsigned long hccast_scene_switch_get_tick(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0)
    {
        hccast_log(LL_ERROR,"[warn]: hc_scene_switch fail\n");
        return 0;
    }
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void hccast_scene_switch_sleep_ms(unsigned long tick)
{
    usleep(tick * 1000);
}

void hccast_scene_switch_stop_url_play()
{
    hccast_media_stop();
}

void hccast_scene_playback_end_reset(void)
{
    if (hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY)
        hccast_scene_reset(HCCAST_SCENE_AIRCAST_PLAY, HCCAST_SCENE_NONE);
    else if (hccast_get_current_scene() == HCCAST_SCENE_DLNA_PLAY)
        hccast_scene_reset(HCCAST_SCENE_DLNA_PLAY, HCCAST_SCENE_NONE);
}

int hccast_scene_get_mira_is_restarting()
{
    return scene_mira_begin_restarting;
}

void hccast_scene_set_mira_is_restarting(int flag)
{
    scene_mira_begin_restarting = flag;
}

void hccast_scene_set_mira_restart_flag(int restart)
{
    scene_mira_need_restart = restart;
    scene_mira_check_tick = 0;
    hccast_log(LL_NOTICE, "set scene_mira_need_restart(%d)\n", scene_mira_need_restart);
}

int hccast_scene_get_mira_restart_flag(void)
{
    return scene_mira_need_restart;
}

void hccast_scene_update_mira_check_tick()
{
    if (scene_mira_check_tick && (current_scene == HCCAST_SCENE_NONE))
    {
        scene_mira_check_tick = hccast_scene_switch_get_tick();
        hccast_log(LL_INFO,"update scene_mira_check_tick\n");
    }
}

void hccast_scene_mira_stop(void)
{
#ifdef SUPPORT_MIRACAST
    hccast_mira_service_stop();
    hccast_net_set_if_updown(HCCAST_P2P_INF, HCCAST_NET_IF_DOWN);
#endif
}

void hccast_scene_stop_all_network_service(void)
{
#ifdef SUPPORT_UM
    hccast_log(LL_NOTICE,"%s: begin \n",__FUNCTION__);
#ifdef SUPPORT_DLNA
    if(hccast_dlna_service_is_start())
    {
        hccast_dlna_service_stop();
    }
#endif

#ifdef SUPPORT_AIRCAST
    if(hccast_air_service_is_start())
    {
        hccast_air_service_stop();
    }
#endif

#ifdef SUPPORT_MIRACAST
    scene_mira_need_restart = 0;
    scene_mira_check_tick = 0;
    hccast_scene_mira_stop();
#endif

    hccast_log(LL_NOTICE,"%s: done \n",__FUNCTION__);
#endif
}

void hccast_scene_start_all_network_service(void)
{
#ifdef SUPPORT_UM

#ifdef SUPPORT_DLNA
    hccast_dlna_service_start();
#endif

#ifdef SUPPORT_AIRCAST
    hccast_air_service_start();
#endif

#ifdef SUPPORT_MIRACAST
    //when wifi is connecting, miracast will restart after the wifi connect ok.
    if(hccast_wifi_mgr_get_wifi_is_connecting() == 0)
    {
        scene_mira_need_restart = 1;
    }
#endif

    hccast_log(LL_NOTICE,"%s: done \n",__FUNCTION__);

#endif
}

int hccast_scene_switch(int next_scene)
{
    int wait_time = 0;
#ifdef SUPPORT_MIRACAST
    while ((hccast_scene_get_mira_is_restarting()) && (next_scene != HCCAST_SCENE_NONE))
    {
        hccast_scene_switch_sleep_ms(20);
    }
#endif

    scene_switching = 1;
    if (next_scene != current_scene)
    {
        hccast_log(LL_NOTICE,"%s: From %d to %d , %d\n", __FUNCTION__, current_scene, next_scene, hccast_scene_switch_get_tick());
        last_scene = current_scene;
        current_scene = next_scene;

#ifdef SUPPORT_MIRACAST
        if ((next_scene != HCCAST_SCENE_MIRACAST) && (next_scene != HCCAST_SCENE_NONE) && (next_scene != HCCAST_SCENE_IUMIRROR && next_scene != HCCAST_SCENE_AUMIRROR))
        {
            pthread_mutex_lock(&g_mira_scene_mutex);
            if (hccast_mira_get_stat())
            {
                hccast_log(LL_INFO,"\n\nis begin to stop mira\n");
                hccast_scene_mira_stop();
                scene_mira_need_restart = 1;
                scene_mira_check_tick = 0;
                hccast_log(LL_INFO,"\n\nstop mira done\n");

            }
            pthread_mutex_unlock(&g_mira_scene_mutex);
        }
#endif

        if ((next_scene != HCCAST_SCENE_AIRCAST_MIRROR && next_scene != HCCAST_SCENE_AIRCAST_PLAY)\
            && last_scene != HCCAST_SCENE_AIRCAST_PLAY)
        {
#ifdef SUPPORT_AIRCAST
            extern int hccast_air_mirror_get_skip_url(void);
            extern void hccast_air_mirror_set_skip_url(int flag);

            if (hccast_air_ap_audio_stat())
            {
                hccast_log(LL_INFO,"%s: stop aircast audio, tick: %d\n", __FUNCTION__, hccast_scene_switch_get_tick());
                if (hccast_air_mirror_get_skip_url())
                {
                    hccast_scene_air_event_callback_func(AIRCAST_USER_AUDIO_STOP, 0);
                    hccast_scene_air_event_callback_func(AIRCAST_USER_MIRROR_STOP, 0);
                    hccast_air_mirror_set_skip_url(0);
                }
                else
                {
                    hccast_scene_air_event_callback_func(AIRCAST_USER_AUDIO_STOP, 0);
                }    

                unsigned long tick = hccast_scene_switch_get_tick();
                while (hccast_air_ap_audio_stat() && (hccast_scene_switch_get_tick() - tick) < 2000)
                {
                    hccast_log(LL_DEBUG,"%s: wait aircast audio stop, tick: %d\n", __FUNCTION__, hccast_scene_switch_get_tick());
                    hccast_scene_switch_sleep_ms(20);
                }
            }
#endif
        }

        if (last_scene == HCCAST_SCENE_DLNA_PLAY)
        {
#ifdef SUPPORT_DLNA
            hccast_scene_switch_stop_url_play();
            if (next_scene != HCCAST_SCENE_DLNA_PLAY)
            {
                hccast_dlna_service_stop();
                hccast_dlna_service_start();
            }

            hccast_log(LL_INFO,"%s %d stop scene %d\n", __func__,__LINE__,last_scene);
#endif
        }
        else if (last_scene == HCCAST_SCENE_AIRCAST_PLAY )
        {
#ifdef SUPPORT_AIRCAST
            hccast_scene_switch_stop_url_play();
            hccast_scene_switch_sleep_ms(100);
            //it must be dlna seturl, next step will stop aircast.
            if (next_scene != HCCAST_SCENE_AIRCAST_MIRROR)
            {
                hccast_air_mediaplayer_2_aircast_event(AIRCAST_VIDEO_USEREXIT, 0);
                hccast_scene_switch_sleep_ms(50);
                if (next_scene != HCCAST_SCENE_NONE)
                {
                    hccast_air_service_stop();
                    hccast_air_service_start();
                }
            }
#endif
            hccast_log(LL_INFO,"%s %d stop scene %d\n", __func__,__LINE__,last_scene);
        }
        else if (last_scene == HCCAST_SCENE_AIRCAST_MIRROR)
        {
#ifdef SUPPORT_AIRCAST
            if (next_scene != HCCAST_SCENE_AIRCAST_PLAY)
            {
                hccast_scene_air_event_callback_func(AIRCAST_USER_MIRROR_STOP, 0);
                wait_time = 5000;//5s for wait back menu.
            }
#endif
            hccast_log(LL_INFO,"%s %d stop scene %d\n", __func__,__LINE__,last_scene);
        }
        else if (last_scene == HCCAST_SCENE_MIRACAST)
        {
            hccast_log(LL_INFO,"%s %d stop scene %d\n", __func__,__LINE__,last_scene);
        }

#ifdef SUPPORT_AIRCAST
        //wait mirror stop.
        if (last_scene == HCCAST_SCENE_AIRCAST_MIRROR && next_scene != HCCAST_SCENE_AIRCAST_PLAY)
        {
            unsigned long tick = hccast_scene_switch_get_tick();
            //we need wait menu open success.
            while (hccast_air_ap_mirror_stat()&& ((hccast_scene_switch_get_tick() - tick) < wait_time))
            {
                hccast_scene_switch_sleep_ms(20);
            }
        }
#endif

#ifdef SUPPORT_UM
        if(next_scene == HCCAST_SCENE_IUMIRROR || next_scene == HCCAST_SCENE_AUMIRROR)
        {
            hccast_scene_stop_all_network_service();
        }
#endif
    }
    else
    {
        hccast_log(LL_NOTICE,"it is nothing to do for scene switch %d \n", current_scene);
    }
    hccast_log(LL_NOTICE,"%s done, %d\n", __FUNCTION__, hccast_scene_switch_get_tick());
    last_scene_switch_tick = hccast_scene_switch_get_tick();
    scene_switching = 0;
}

void hccast_scene_reset(int cur_scene, int next_scene)
{
#ifdef SUPPORT_UM
    if((cur_scene == HCCAST_SCENE_IUMIRROR || cur_scene == HCCAST_SCENE_AUMIRROR) && next_scene == HCCAST_SCENE_NONE)
    {
        hccast_scene_start_all_network_service();
    }
#endif

    if (cur_scene == current_scene)
    {
        hccast_log(LL_NOTICE,"%s: From %d to %d , %d\n", __FUNCTION__, current_scene, next_scene, hccast_scene_switch_get_tick());
        current_scene = next_scene;
    }
    else
    {
        hccast_log(LL_INFO,"it is nothing to do for scene_reset (%d - %d - %d) \n", cur_scene, next_scene, current_scene);
    }
}

int hccast_get_current_scene(void)
{
    return current_scene;
}

void hccast_set_current_scene(int scene)
{
    current_scene = scene;
}

int hccast_scene_get_switching()
{
    return scene_switching;
}

int hccast_scene_switch_happened(int elapse)
{
    if (scene_switching || (hccast_scene_switch_get_tick() - last_scene_switch_tick) < elapse)
    {
        hccast_log(LL_NOTICE,"[%s][%d] scene_switching: %d\n", __func__, __LINE__, scene_switching);
        return 1;
    }

    return 0;
}

void hccast_scene_mira_restart_check()
{
#ifdef SUPPORT_MIRACAST
    int mira_restart_time = 8;

    if (((current_scene == HCCAST_SCENE_NONE )
         || (((current_scene == HCCAST_SCENE_DLNA_PLAY) || (current_scene == HCCAST_SCENE_AIRCAST_PLAY)) && (hccast_media_get_status() == HCCAST_MEDIA_STATUS_STOP)))
#ifdef SUPPORT_AIRCAST
        && !hccast_air_is_playing_music()
#endif
       )
    {
        if (scene_mira_need_restart)
        {
            if (!scene_mira_check_tick)
            {
                scene_mira_check_tick = hccast_scene_switch_get_tick();
                hccast_log(LL_NOTICE,"mira will restart after %ds\n", mira_restart_time);
            }

            if (((hccast_scene_switch_get_tick() - scene_mira_check_tick) > (mira_restart_time * 1000))
                && (scene_switching != 1) && (hccast_mira_get_stat() == 0))
            {
                scene_mira_begin_restarting = 1;

                hccast_log(LL_INFO,"\n\nscene begin to restart mira \n");
                pthread_mutex_lock(&g_mira_scene_mutex);

                if(current_scene != HCCAST_SCENE_NONE)
                {
                    current_scene = HCCAST_SCENE_NONE;//reset scene every time when mira restart. for is nothing to do case.
                    hccast_log(LL_NOTICE,"%s reset scene\n",__func__);
                }
                hccast_mira_service_start();

                scene_mira_need_restart = 0;
                scene_mira_check_tick = 0;
                hccast_log(LL_INFO,"restart mira done \n\n");

                pthread_mutex_unlock(&g_mira_scene_mutex);
            }
            return;
        }
    }

    scene_mira_check_tick = 0;
#endif
}

static void *hccast_scene_thread(void *args)
{
    hccast_log(LL_NOTICE,"hccast_scene_thread is running.\n");
    while (1)
    {
        hccast_scene_mira_restart_check();

        hccast_scene_switch_sleep_ms(200);
    }
}

void hccast_scene_init(void)
{
    pthread_t tid;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    if (pthread_create(&tid, NULL, hccast_scene_thread, NULL) < 0)
    {
        hccast_log(LL_WARNING,"[scene]:Create hccast_scene_thread error.\n");
    }
}
