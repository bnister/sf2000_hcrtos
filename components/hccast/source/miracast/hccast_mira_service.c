#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __linux__
    #include <sys/prctl.h>
    #include <hccast/miracast_api.h>
#else
    #include <miracast/miracast_api.h>
#endif

#include <hccast_dsc.h>

#include "hccast_mira_avplayer.h"
#include "hccast_mira.h"
#include "hccast_wifi_mgr.h"
#include <hccast_scene.h>
#include <hccast_dlna.h>
#include <hccast_air.h>
#include <hccast_net.h>
#include <hccast_log.h>

//#define P2P_CTRL_IFACE_NAME     "p2p0"

static char g_cur_ssid[WIFI_MAX_SSID_LEN] = {0};
static int g_mira_start = 0;
static E_P2P_STATE g_mira_connect_stat = P2P_STATE_NONE;
static pthread_mutex_t g_mira_svr_mutex = PTHREAD_MUTEX_INITIALIZER;
static hccast_wifi_ap_info_t g_mira_cur_ap;

static bool g_mira_starting = false;

#ifdef HC_RTOS
    #include <linux/interrupt.h>
#endif

hccast_mira_event_callback mira_callback = NULL;

enum
{
    MIRA_WIFI_STAT_NONE,
    MIRA_WIFI_STAT_PREVIOUS,
    MIRA_WIFI_STAT_ONGOING,
};

static unsigned char g_mira_wifi_stat = MIRA_WIFI_STAT_NONE;

int hccast_mira_get_stat(void)
{
    return g_mira_start;
}

bool hccast_mira_is_starting(void)
{
    bool stat = g_mira_starting;
    hccast_log(LL_DEBUG, "g_mira_starting: %d\n", g_mira_starting);

    return stat;
}

void *hccast_mira_wifi_reconnect_thread(void *arg)
{
    (void)arg;
#ifdef __linux__
    prctl(PR_SET_NAME, __func__);
#endif

    hccast_mira_service_stop();
    hccast_wifi_mgr_connect(&g_mira_cur_ap);

    if (hccast_wifi_mgr_get_connect_status())
    {
        hccast_wifi_mgr_udhcpc_stop();
        hccast_wifi_mgr_udhcpc_start();
        pthread_mutex_lock(&g_mira_svr_mutex);
        hccast_wifi_mgr_p2p_set_connect_stat(false);
        pthread_mutex_unlock(&g_mira_svr_mutex);
    }
    else
    {
        pthread_mutex_lock(&g_mira_svr_mutex);
        hccast_wifi_mgr_p2p_set_connect_stat(false);
        pthread_mutex_unlock(&g_mira_svr_mutex);
        hccast_wifi_mgr_udhcpc_stop();
        hccast_wifi_mgr_disconnect();
    }

    g_mira_wifi_stat = MIRA_WIFI_STAT_NONE;
    pthread_detach(pthread_self());
    return NULL;
}

void *hccast_mira_hostap_enable_thread(void *arg)
{
    (void)arg;
#ifdef __linux__
    prctl(PR_SET_NAME, __func__);
#endif

    pthread_mutex_lock(&g_mira_svr_mutex);
    hccast_wifi_mgr_p2p_set_connect_stat(false);
    pthread_mutex_unlock(&g_mira_svr_mutex);

    int started = hccast_mira_get_stat();
    if (started) // solve service start after service stop.
    {
        hccast_mira_service_stop();
    }

#ifdef __linux__
    hccast_wifi_mgr_hostap_start();
#else
    hccast_wifi_mgr_hostap_enable();
    hccast_wifi_mgr_udhcpd_start();
#endif

    if (started) // solve service start after service stop.
    {
        hccast_mira_service_start();
    }

    pthread_detach(pthread_self());
    return NULL;
}

void hccast_mira_stop_services()
{
#ifdef SUPPORT_AIRCAST
    if (hccast_air_service_is_start())
    {
        hccast_air_service_stop();
    }
#endif

#ifdef SUPPORT_DLNA
    if (hccast_dlna_service_is_start())
    {
        hccast_dlna_service_stop();
    }
#endif
}

void hccast_mira_start_services()
{
#ifdef SUPPORT_AIRCAST
    hccast_air_service_start();
#endif

#ifdef SUPPORT_DLNA
    hccast_dlna_service_start();
#endif
}

static void *hccast_mira_stop_services_thread(void *arg)
{
    (void)arg;
    hccast_log(LL_NOTICE, "hccast_mira_stop_services_thread run...\n");
    hccast_mira_stop_services();
    return NULL;
}

int hccast_mira_wfd_event_cb(const wfd_status_t event, const void *data)
{
    (void)data;

    hccast_log(LL_INFO, "%s: wfd event: %d\n", __FUNCTION__, event);

    switch (event)
    {
    case WFD_SET_SSID_DONE:
        if (hccast_wifi_mgr_p2p_get_connect_stat())
        {
            hccast_wifi_mgr_p2p_set_enable(true);

            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_SSID_DONE, NULL, NULL);
            }
        }
        break;

    case WFD_CONNECTED:
    {
        if (mira_callback)
        {
            mira_callback(HCCAST_MIRA_CONNECTED, NULL, NULL);
        }
        break;
    }

    case WFD_DISCONNECTED:
    {
        if (mira_callback)
        {
            mira_callback(HCCAST_MIRA_DISCONNECT, NULL, NULL);
        }
        break;
    }

    case WFD_START_PLAYER:
    {
        if (mira_callback)
        {
            mira_callback(HCCAST_MIRA_START_DISP, NULL, NULL);
        }
        break;
    }

    case WFD_STOP_PLAYER:
    {
        if (mira_callback)
        {
            mira_callback(HCCAST_MIRA_STOP_DISP, NULL, NULL);
        }
        break;
    }

    default:
        break;
    }

    return 0;
}

int  hccast_mira_player_init()
{
    return mira_av_player_init();
}

static wfd_manage_func_t func =
{
    ._p2p_device_get_ip     = (void *)hccast_wifi_mgr_p2p_get_ip,
    ._p2p_device_event      = (void *)hccast_mira_wfd_event_cb,
    ._p2p_device_get_rtsp_port = (void *)hccast_wifi_mgr_p2p_get_rtsp_port,
};

int hc_miracast_wfd_manage_init()
{
    miracast_ioctl(WFD_CMD_SET_MANAGE_FUNC, (unsigned long)&func, (unsigned long)0);

    return 0;
}

int hccast_miracast_service_init(hccast_mira_event_callback func)
{
    hccast_mira_event_callback callback = func;
    return 0;
}

int hccast_mira_update_p2p_state(hccast_p2p_state_e state)
{
    hccast_log(LL_DEBUG, "state: %d\n", state);

    switch (state)
    {
    case HCCAST_P2P_STATE_NONE:
        miracast_update_p2p_status(P2P_STATE_NONE);
        break;

    case HCCAST_P2P_STATE_LISTEN:
        hccast_net_ifconfig(HCCAST_P2P_INF, "128.0.0.1", "255.0.0.0", NULL);
        hccast_net_set_if_updown(HCCAST_P2P_INF, HCCAST_NET_IF_UP);
        if (HCCAST_SCENE_MIRACAST == hccast_get_current_scene())
        {
            hccast_net_ifconfig(HCCAST_WIFI_INF, HCCAST_HOSTAP_IP, HCCAST_HOSTAP_MASK, HCCAST_HOSTAP_GW);

            bool connected = hccast_wifi_mgr_p2p_get_connect_stat();
            hccast_log(LL_NOTICE, "mira connected: %d, last ssid: \"%s\"\n", connected, g_mira_cur_ap.ssid);
            if (connected)
            {
                if (MIRA_WIFI_STAT_PREVIOUS == g_mira_wifi_stat)
                {
                    pthread_t tid = 0;
                    if (pthread_create(&tid, NULL, (void *)hccast_mira_wifi_reconnect_thread, NULL) != 0)
                    {
                        hccast_log(LL_ERROR, "create hccast_mira_wifi_reconnect_thread failed!\n");
                    }

                    g_mira_wifi_stat = MIRA_WIFI_STAT_ONGOING;
                }
                else if (MIRA_WIFI_STAT_NONE == g_mira_wifi_stat)
                {
                    pthread_t tid = 0;
                    if (pthread_create(&tid, NULL, (void *)hccast_mira_hostap_enable_thread, NULL) != 0)
                    {
                        hccast_log(LL_ERROR, "create hccast_mira_wifi_reconnect_thread failed!\n");
                    }
                }

                if ((hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
                {
                    hccast_log(LL_WARNING, "Cur scene is doing USB MIRROR\n");
                }
                else
                {
                    hccast_scene_switch(HCCAST_SCENE_NONE);
                }
            }
        }

        hccast_scene_set_mira_is_restarting(0);
        miracast_update_p2p_status(P2P_STATE_LISTEN);
        break;

    case HCCAST_P2P_STATE_CONNECTING:
        if (HCCAST_SCENE_MIRACAST != hccast_get_current_scene())
        {
            hccast_net_ifconfig(HCCAST_WIFI_INF, "129.0.0.1", "255.0.0.0", NULL);
            hccast_net_ifconfig(HCCAST_P2P_INF, HCCAST_P2P_IP, NULL, NULL);

            pthread_mutex_lock(&g_mira_svr_mutex);
            hccast_wifi_mgr_p2p_set_connect_stat(true);
            pthread_mutex_unlock(&g_mira_svr_mutex);

            memset(&g_mira_cur_ap, 0, sizeof(g_mira_cur_ap));
            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_GET_CUR_WIFI_INFO, (void *)&g_mira_cur_ap, NULL);
            }

            hccast_log(LL_INFO, "%d: cur wifi connect ssid %s.\n", __LINE__, g_mira_cur_ap.ssid);
            if (strlen(g_mira_cur_ap.ssid) > 0)
            {
                g_mira_wifi_stat = MIRA_WIFI_STAT_PREVIOUS;
#ifdef HC_RTOS
                pthread_t tid = 0;
                if (pthread_create(&tid, NULL, (void *)hccast_mira_stop_services_thread, NULL) != 0)
                {
                    hccast_log(LL_ERROR, "create hccast_mira_stop_services_thread failed!\n");
                }
#endif
                hccast_wifi_mgr_udhcpc_stop();
                hccast_wifi_mgr_disconnect();
            }
            else
            {
                g_mira_wifi_stat = MIRA_WIFI_STAT_NONE;
#ifdef __linux__
                hccast_wifi_mgr_hostap_stop();
#else
                hccast_wifi_mgr_udhcpd_stop();
                hccast_wifi_mgr_hostap_disenable();
#endif
                hccast_mira_stop_services();
            }

            hccast_scene_switch(HCCAST_SCENE_MIRACAST);

            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_CONNECT, NULL, NULL);
            }
        }

        miracast_update_p2p_status(P2P_STATE_GONEGO_OK);
        break;

    case HCCAST_P2P_STATE_CONNECTED:
        miracast_update_p2p_status(P2P_STATE_PROVISIONING_DONE);
        break;

    case HCCAST_P2P_STATE_CONNECT_FAILED:
        miracast_update_p2p_status(P2P_STATE_IDLE);
        break;

    case HCCAST_P2P_STATE_DISCONNECTED:
        break;

    default:
        break;
    }

    return state;
}

/**
 * It disconnects the Miracast connection.
 *
 * @return 0
 */
int hccast_mira_disconnect()
{
    if (g_mira_start)
    {
        miracast_disconnect();
    }

    return 0;
}

/**
 * It stops the Miracast service.
 */
int hccast_mira_service_stop()
{
    hccast_scene_set_mira_restart_flag(0);

    pthread_mutex_lock(&g_mira_svr_mutex);
    if (hccast_mira_is_starting())
    {
        hccast_log(LL_NOTICE, "need waite mira started, then stop it!\n");
    }

    if (g_mira_start == 0)
    {
        pthread_mutex_unlock(&g_mira_svr_mutex);
        return 0;
    }

    g_mira_start = 0;
    hccast_log(LL_NOTICE, "%s mira stop begin.\n", __func__);
    miracast_stop();
    hccast_wifi_mgr_p2p_set_enable(false);
    pthread_mutex_unlock(&g_mira_svr_mutex);
    hccast_wifi_mgr_p2p_uninit();
    hccast_log(LL_NOTICE, "%s mira stop done.\n", __func__);
    return 0;
}

/**
 * It starts the Miracast service.
 *
 * @return 0: success; <0: failed.
 */
int hccast_mira_service_start()
{
    char dev_name[MIRA_NAME_LEN] = "hccast_mira";

    pthread_mutex_lock(&g_mira_svr_mutex);
    if (g_mira_start)
    {
        pthread_mutex_unlock(&g_mira_svr_mutex);
        return -1;
    }

#ifdef HC_RTOS
    int is_support_miracast = hccast_wifi_mgr_get_support_miracast();
    hccast_log(LL_NOTICE, "####: is_support_miracast: %d\n", is_support_miracast);
#endif

    g_mira_starting = true;

    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_DEVICE_NAME, dev_name, NULL);
    }

    hccast_wifi_p2p_param_t p2p_param   = {0};
    p2p_param.device_name               = dev_name;
    p2p_param.func_update_state         = (void *)hccast_mira_update_p2p_state;

    int ret = hccast_wifi_mgr_p2p_init(&p2p_param);
    if (ret)
    {
        hccast_log(LL_ERROR, "hc_wifi_service_p2p_init failed!\n");
        goto EXIT;
    }

    hccast_wifi_mgr_p2p_device_init();
    hccast_wifi_mgr_p2p_set_enable(true);
    miracast_start();
    g_mira_start = 1;
    hccast_log(LL_NOTICE, "%s mira start done.\n", __func__);

EXIT:
    g_mira_starting = false;
    pthread_mutex_unlock(&g_mira_svr_mutex);

    return 0;
}

int hccast_mira_service_init(hccast_mira_event_callback func)
{
    hccast_log(LL_NOTICE, "%s\n", miracast_get_version());

    hc_miracast_wfd_manage_init();
    hccast_mira_player_init();

    mira_callback = func;
    return 0;
}

int hccast_mira_service_uninit()
{
    mira_callback = NULL;
    return 0;
}

int hccast_mira_service_ioctl(int cmd, void *arg)
{
    switch (cmd)
    {
    case HCCAST_MIRA_CMD_SET_RESOLUTION:
    {
        if (arg == NULL)
        {
            return -1;
        }

        int *res = (int *)arg;
        if (HCCAST_MIRA_RES_1080P30 == *res)
        {
            hccast_mira_set_default_resolution(WFD_1080P30);
        }
        else if (HCCAST_MIRA_RES_720P30 == *res)
        {
            hccast_mira_set_default_resolution(WFD_720P30);
        }
        else if (HCCAST_MIRA_RES_480P60 == *res)
        {
            hccast_mira_set_default_resolution(WFD_480P60);
        }
        else if (HCCAST_MIRA_RES_VESA1400 == *res)
        {
            hccast_mira_set_default_resolution(WFD_VESA_1400);
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

int hccast_mira_service_set_resolution(hccast_mira_res_e res)
{
    if (HCCAST_MIRA_RES_1080P30 == res)
    {
        hccast_mira_set_default_resolution(WFD_1080P30);
    }
    else if (HCCAST_MIRA_RES_720P30 == res)
    {
        hccast_mira_set_default_resolution(WFD_720P30);
    }
    else if (HCCAST_MIRA_RES_480P60 == res)
    {
        hccast_mira_set_default_resolution(WFD_480P60);
    }
    else if (HCCAST_MIRA_RES_VESA1400 == res)
    {
        hccast_mira_set_default_resolution(WFD_VESA_1400);
    }

    return 0;
}
