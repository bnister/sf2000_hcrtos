#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "hccast_wifi_ctrl.h"
#include "hccast_p2p_ctrl.h"
#include "hccast_log.h"
#include "hccast_wifi_mgr.h"
#include "hccast_dhcpd.h"
#include "hccast_net.h"

hccast_wifi_mgr_event_callback wifi_event_callback = NULL;
static volatile int hostap_en = 0;
static volatile int station_en = 0;
static hccast_net_wifi_model_e g_wifi_model = HCCAST_NET_WIFI_NONE;
static pthread_mutex_t g_hostap_mutex = PTHREAD_MUTEX_INITIALIZER;

char HCCAST_HOSTAP_IP[32]   = {"192.168.68.1"};
char HCCAST_HOSTAP_MASK[32] = {"255.255.255.0"};
char HCCAST_HOSTAP_GW[32]   = {"192.168.68.1"};
char HCCAST_P2P_IP[32]      = {"192.168.49.1"};
char HCCAST_P2P_MASK[32]    = {"255.255.255.0"};
char HCCAST_P2P_GW[32]      = {0};

#define HCCAST_WIFI_CONNECTING_TIMEOUT  (30)
#define HCCAST_WIFI_SCAN_TIMEOUT        (15)

/**
 * Get the current status of scanning AP
 *
 * @param NULL
 *
 * @return >= 0: The current state of the wifi scan: 1:scanning other: other.
 * < 0: error.
 */
int hccast_wifi_mgr_get_scan_status()
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    hccast_wifi_status_result_t res;

    ret = wifi_ctrl_get_status(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, &res);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "get scan status error!\n");
        return ret;
    }

    if (!strcmp(res.wpa_state, "SCANNING"))
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    return ret;
}

/**
 * Get the current status of connecting AP
 *
 * @param NULL
 *
 * @return >= 0: The return value is the current connection status of the device.
 * < 0: error.
 */
int hccast_wifi_mgr_get_connect_status()
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    hccast_wifi_status_result_t res = {0};

#ifdef HC_RTOS
    if (hccast_wifi_mgr_get_hostap_status())
    {
        hccast_log(LL_INFO, "%d get_connect_status: %d.\n", __LINE__, ret);
        return ret;
    }
#endif

    ret = wifi_ctrl_get_status(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, &res);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "get connect status failed!\n");
        return ret;
    }

    if (strlen(res.ssid) > 0 && !strcmp(res.wpa_state, "COMPLETED"))
    {
        ret = 1;
    }
    else
    {
        ret = 0;
        hccast_log(LL_INFO, "get_connect_status: %s.\n", res.wpa_state);
    }

    return ret;
}

/**
 * Get the SSID of the connected AP
 *
 * @param ssid: the name of the current connected AP
 * @param len: the length of the ssid buffer
 *
 * @return >= 0: the status of the wifi connection; < 0: error.
 */
int hccast_wifi_mgr_get_connect_ssid(char *ssid, size_t len)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    hccast_wifi_status_result_t res = {0};

    if (NULL == ssid)
    {
        hccast_log(LL_ERROR, "param error!\n");
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        return ret;
    }

#ifdef HC_RTOS
    if (hccast_wifi_mgr_get_hostap_status())
    {
        memset(ssid, 0, len);
        return ret;
    }
#endif

    ret = wifi_ctrl_get_status(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, &res);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "get connect ssid failed!\n");
        return ret;
    }

    if (strlen(res.ssid) > 0 && !strcmp(res.wpa_state, "COMPLETED"))
    {
        if (strlen(res.ssid) <= len)
        {
            memcpy(ssid, res.ssid, sizeof(res.ssid));
        }
        else
        {
            hccast_log(LL_WARNING, "ssid len overflow. (%d/%d)\n", strlen(res.ssid), len);
            memcpy(ssid, res.ssid, len);
        }

        ret = 1;
    }
    else
    {
        ret = 0;
    }

    if (wifi_event_callback)
    {
        wifi_event_callback(HCCAST_WIFI_CONNECT_SSID, NULL, ssid);
    }

    return ret;
}

/**
 * It gets the list of networks (wpas save).
 *
 * @param res: The result of the operation.
 *
 * @return < 0: failed, 0: success
 */
int hccast_wifi_mgr_get_list_net(hccast_wifi_list_net_result_t *res)
{
    return wifi_ctrl_get_list_net(WIFI_CTRL_IFACE_NAME, res);
}

/**
 * It removes a network from the list of networks.
 *
 * @param net_id the network ID of the network to be deleted.
 *
 * @return < 0: failed, 0: success
 */
int hccast_wifi_mgr_rm_list_net(char *net_id)
{
    return wifi_ctrl_do_remove_net_by_id(WIFI_CTRL_IFACE_NAME, net_id);
}

/**
 * It scans for available APs and returns the result in the `wifi_ctrl_scan_result_t` structure
 *
 * @param scan_res: the scan result
 * @return < 0: failed, 0:success
 */
int hccast_wifi_mgr_scan(hccast_wifi_scan_result_t *scan_res)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    int cnt = 0;
    int len = 0;
    int status = 0;
    int rescan = 1;
    struct timespec tv;
    struct timespec tv_last;

    hccast_net_set_if_updown(P2P_CTRL_IFACE_NAME, HCCAST_NET_IF_UP);

RESCAN:

    clock_gettime(CLOCK_MONOTONIC, &tv_last);

    if (wifi_event_callback)
    {
        wifi_event_callback(HCCAST_WIFI_SCAN, NULL, NULL);
    }

    ret = wifi_ctrl_do_scan();
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "scan failed!\n");
        goto EXIT;
    }
    else
    {
#ifdef __linux__
#ifdef SUPPORT_MIRACAST
        if (hccast_mira_get_stat())
        {
            ret = wifi_ctrl_wait_scan_timeout(HCCAST_WIFI_SCAN_TIMEOUT);
            hccast_log(LL_NOTICE, "wifi_ctrl_wait_scan_timeout ret = %d.\n", ret);
            status = hccast_wifi_mgr_get_scan_status();
        }
        else
        {
            do
            {
                usleep(500 * 1000);
                status = hccast_wifi_mgr_get_scan_status();
            }
            while (++cnt <= (HCCAST_WIFI_SCAN_TIMEOUT / 5) * 10 && status && wifi_ctrl_is_scanning());
        }
#else
        do
        {
            usleep(500 * 1000);
            status = hccast_wifi_mgr_get_scan_status();
        }
        while (++cnt <= (HCCAST_WIFI_SCAN_TIMEOUT / 5) * 10 && status && wifi_ctrl_is_scanning());
#endif // SUPPORT_MIRACAST
#else  // HC_RTOS
        do
        {
            usleep(500 * 1000);
            status = hccast_wifi_mgr_get_scan_status();
        }
        while (++cnt <= (HCCAST_WIFI_SCAN_TIMEOUT / 5) * 10 && status && wifi_ctrl_is_scanning());
#endif // __linux__
    }

    if (wifi_ctrl_is_scanning())
    {
        clock_gettime(CLOCK_MONOTONIC, &tv);

        ret = wifi_ctrl_get_aplist(scan_res);
        if (ret < 0)
        {
            hccast_log(LL_ERROR, "scan get ap list failed!\n");
            goto EXIT;
        }

        if (((tv.tv_sec - tv_last.tv_sec < 3) || (scan_res->ap_num == 0)) && (rescan == 1))
        {
            hccast_log(LL_NOTICE, "^^^^^ %ld To be rescan once time. ^^^^^\n", tv.tv_sec - tv_last.tv_sec);
            rescan = 0;
            usleep(500 * 1000);
            goto RESCAN;
        }

        if (wifi_event_callback)
        {
            wifi_event_callback(HCCAST_WIFI_SCAN_RESULT, NULL, scan_res);
        }
    }

EXIT:

    wifi_ctrl_do_op_reset();

    return ret;
}

int hccast_wifi_mgr_is_connecting(void)
{
    return wifi_ctrl_is_connecting();
}

int hccast_wifi_mgr_is_scanning(void)
{
    return wifi_ctrl_is_scanning();
}

/**
 * This function is called by the HCCast application to abort the current WiFi operation
 *
 * @return always HCCAST_WIFI_ERR_NO_ERROR.
 * @note: need to be used with hccast_wifi_mgr_op_reset
 */
int hccast_wifi_mgr_op_abort()
{
    wifi_ctrl_do_op_abort();
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It resets the wifi op stat for HCCAST_WIFI_STAT_IDLE.
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 * @note: need to be used with hccast_wifi_mgr_op_abort
 */
int hccast_wifi_mgr_op_reset()
{
    wifi_ctrl_do_op_reset();
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * Used to connect to an AP.
 * @param ap_info: conn AP info
 * @return < 0: err (-2: wpas no run)
 * @return 0:failed, 1: success
 */
int hccast_wifi_mgr_connect(const hccast_wifi_ap_info_t *ap_info)
{
    int cnt = 0;
    int status = 0;
    int ret = HCCAST_WIFI_ERR_NO_ERROR;

    if (!wifi_ctrl_is_initted())
    {
        wifi_ctrl_init(wifi_event_callback);
    }

    /*
    ret = wifi_ctrl_hostap_interface_enable(0);
    if (ret < 0)
    {
        printf("Err: %s %d ret = %d\n", __func__, __LINE__, ret);
        return -1;
    }
    */

    hccast_net_set_if_updown(P2P_CTRL_IFACE_NAME, HCCAST_NET_IF_DOWN);
    wifi_ctrl_set_connecting_by_user(true);

    ret = wifi_ctrl_do_ap_connect(ap_info);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "connect failed!\n");
        goto EXIT;
    }
    else
    {
        if (wifi_event_callback)
        {
            wifi_event_callback(HCCAST_WIFI_CONNECT, (void *)ap_info->ssid, NULL);
        }

#ifdef __linux__
        ret = wifi_ctrl_wait_connect_timeout(HCCAST_WIFI_CONNECTING_TIMEOUT);
        hccast_log(LL_INFO, "wifi_ctrl_wait_connect_timeout ret = %d.\n", ret);
        status = hccast_wifi_mgr_get_connect_status();
#else
        do
        {
            usleep(200 * 1000);
            status = hccast_wifi_mgr_get_connect_status();
        }
        while (++cnt <= (HCCAST_WIFI_CONNECTING_TIMEOUT / 2) * 10 && !status && wifi_ctrl_is_connecting());
#endif
    }

    ret = status;
    hccast_log(LL_INFO, "connect_status = %s.\n", status ? "connected" : "disconnect");

EXIT:
    wifi_ctrl_do_op_reset();

    hccast_net_set_if_updown(P2P_CTRL_IFACE_NAME, HCCAST_NET_IF_UP);
    wifi_ctrl_set_connecting_by_user(false);

    return ret;
}

/**
 * This function is used to disconnect AP, with cb message
 *
 * @return <0: failed; 0: success
 */
int hccast_wifi_mgr_disconnect()
{
    wifi_ctrl_set_disconnecting_by_user(true);
    wifi_ctrl_do_ap_disconnect();
    hccast_net_ifconfig(WIFI_CTRL_IFACE_NAME, "0.0.0.0", NULL, NULL);
    if (wifi_event_callback)
    {
        wifi_event_callback(HCCAST_WIFI_DISCONNECT, NULL, NULL);
    }

    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 *  This function is used to disconnect AP, without cb message
 *
 * @return 0
 */
int hccast_wifi_mgr_disconnect_no_message()
{
    wifi_ctrl_set_disconnecting_by_user(true);
    wifi_ctrl_do_ap_disconnect();
    hccast_net_ifconfig(WIFI_CTRL_IFACE_NAME, "0.0.0.0", NULL, NULL);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It get the frequency of the wifi model capacity.
 * @return hccast_wifi_freq_mode_e
 *
 * @note rtos dont support.
 */
hccast_wifi_freq_mode_e hccast_wifi_mgr_freq_support_mode()
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;

#ifdef __linux__
    char buf[256] = {0};
    ret = wifi_ctrl_get_capability("channels", buf, sizeof(buf));
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "get freq support mode failed!\n");
    }

    if (strstr(buf, "Mode[AD] Channels:"))
    {
        return HCCAST_WIFI_FREQ_MODE_60G;
    }
    else if (strstr(buf, "Mode[A] Channels:"))
    {
        return HCCAST_WIFI_FREQ_MODE_5G;
    }
    else if (strstr(buf, "Mode[B] Channels:") || strstr(buf, "Mode[G] Channels:"))
    {
        return HCCAST_WIFI_FREQ_MODE_24G;
    }
#else
    if (hccast_wifi_mgr_get_wifi_model() > HCCAST_NET_WIFI_60G)
    {
        return HCCAST_WIFI_FREQ_MODE_60G;
    }
    else if (hccast_wifi_mgr_get_wifi_model() > HCCAST_NET_WIFI_5G)
    {
        return HCCAST_WIFI_FREQ_MODE_5G;
    }
    else if (hccast_wifi_mgr_get_wifi_model() > HCCAST_NET_WIFI_24G)
    {
        return HCCAST_WIFI_FREQ_MODE_24G;
    }
#endif

    return HCCAST_WIFI_FREQ_MODE_24G;
}

hccast_wifi_freq_mode_e hccast_wifi_mgr_get_current_freq_mode()
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    hccast_wifi_freq_mode_e mode = HCCAST_WIFI_FREQ_MODE_NONE;
    int host_en = hccast_wifi_mgr_get_hostap_status();
    if (host_en)
    {
        hccast_wifi_hostap_status_result_t hostap_result;

        memset(&hostap_result, 0, sizeof(hccast_wifi_hostap_status_result_t));
        if (wifi_ctrl_hostap_get_status(&hostap_result) == 0)
        {
            if (hostap_result.channel >= 1 && hostap_result.channel <= 14)
            {
                //params.listen_channel = hostap_result.channel;
                mode = HCCAST_WIFI_FREQ_MODE_24G;
            }
            else
            {
                //params.listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
                mode = HCCAST_WIFI_FREQ_MODE_5G;
            }
        }
        else
        {
            mode = HCCAST_WIFI_FREQ_MODE_24G;
        }
    }
    else
    {
        hccast_wifi_status_result_t status_res = {0};
        ret = wifi_ctrl_get_status(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, &status_res);
        if (ret < 0)
        {
            hccast_log(LL_ERROR, "get current freq mode failed!\n");
        }

        /* 2.4G: 2412~2484 MHZ, 5G: 4915~5825 MHZ, 6G: 5955~7115 MHZ */
        if (status_res.freq > 0 && status_res.freq <= 2484)  // 2.4G
        {
            mode = HCCAST_WIFI_FREQ_MODE_24G;
        }
        else if (status_res.freq >= 4915 && status_res.freq <= 5825) // 5G
        {
            mode = HCCAST_WIFI_FREQ_MODE_5G;
        }
        else // 6G
        {
            mode = HCCAST_WIFI_FREQ_MODE_60G;
        }
    }

    hccast_log(LL_NOTICE, "freq mode: %d\n", mode);

    return mode;
}

int hccast_wifi_mgr_get_current_freq()
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    int channel = 0;
    int host_en = hccast_wifi_mgr_get_hostap_status();
    if (host_en)
    {
        hccast_wifi_hostap_status_result_t hostap_result;

        memset(&hostap_result, 0, sizeof(hccast_wifi_hostap_status_result_t));
        if (wifi_ctrl_hostap_get_status(&hostap_result) == 0)
        {
            channel = hostap_result.channel;
        }
        else
        {
            channel = -1;
        }
    }
    else
    {
        hccast_wifi_status_result_t status_res = {0};
        ret = wifi_ctrl_get_status(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, &status_res);
        if (ret < 0)
        {
            hccast_log(LL_ERROR, "wifi_ctrl_get_status failed!\n");
        }

        /* 2.4G: 2412~2484 MHZ, 5G: 4915~5825 MHZ, 6G: 5955~7115 MHZ */
        if (status_res.freq > 0 && status_res.freq <= 2484)  // 2.4G
        {
            channel = (status_res.freq - 2412) / 5 + 1;
        }
        else if (status_res.freq >= 4915 && status_res.freq <= 5825) // 5G
        {
            channel = (status_res.freq - 5000) / 5;
        }
        else // 6G
        {
            channel = -1;
        }
    }

    hccast_log(LL_NOTICE, "channel: %d\n", channel);

    return channel;
}

/**
 * It gets the signal poll result include rssi/linkspeed/noise/frequency.
 *
 * @param result the result of the signal polling
 */
int hccast_wifi_mgr_get_signal_poll(hccast_wifi_signal_poll_result_t *result)
{
    return wifi_ctrl_get_signal_poll(result);
}

/**
 * It gets the number of stations connected to the AP.
 *
 * @param result the result of the hostap status
 *
 * @return The number of stations connected to the hostapd.
 */
int hccast_wifi_mgr_hostap_get_sta_num(hccast_wifi_hostap_status_result_t *result)
{
    static int num = 0;
    hccast_wifi_hostap_status_result_t hostap_result;

    memset(&hostap_result, 0, sizeof(hccast_wifi_hostap_status_result_t));
    if (wifi_ctrl_hostap_get_status(&hostap_result) == 0)
    {
        num = hostap_result.num_sta;
    }

    return num;
}

static void udhcpd_cb(unsigned int yiaddr)
{
    struct in_addr tmp_addr =
    {
        .s_addr = yiaddr
    };

    hccast_log(LL_INFO, "udhcpd lease cb: %s\n", inet_ntoa(tmp_addr));

    //wifi_hostap_status_result_st *result;

    //wifi_ctrl_hostap_get_status(result);
    if (wifi_event_callback)
        wifi_event_callback(HCCAST_WIFI_HOSTAP_OFFER, NULL, (void *)yiaddr);
}

static udhcp_conf_t g_hostap_udhcpd_conf =
{
    .func = udhcpd_cb,
    .ifname = UDHCP_IF_WLAN0,
    .pid    = 0,
    .run    = 0
};

static char udhcpd = 0;

/**
 * It starts the udhcpd server (ifname: wlan0)
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_udhcpd_start()
{
    if (!udhcpd)
    {
        udhcpd = 1;
        udhcpd_start(&g_hostap_udhcpd_conf);
    }

    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It stops the udhcpd server (ifname: wlan0)
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_udhcpd_stop()
{
    hccast_log(LL_INFO, "%s\n", __func__);
    udhcpd = 0;
    udhcpd_stop(&g_hostap_udhcpd_conf);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It gets the status of the hostap.
 *
 * @param conf a pointer to a structure of type hccast_wifi_hostap_status_result_t.
 */
int hccast_wifi_mgr_hostap_get_status(hccast_wifi_hostap_status_result_t *conf)
{
    return wifi_ctrl_hostap_get_status(conf);
}

/**
 * It sets the hostap configuration.
 *
 * @param conf a pointer to a structure of type hccast_wifi_hostap_conf_t.
 */
int hccast_wifi_mgr_hostap_set_conf(hccast_wifi_hostap_conf_t *conf)
{
    return wifi_ctrl_hostap_set_conf(conf);
}

/**
 * It switches the wifi mode to 24G or 5G mode.
 *
 * @param mode The frequency mode to switch to.
 */
int hccast_wifi_mgr_hostap_switch_mode(hccast_wifi_freq_mode_e mode)
{
    return wifi_ctrl_hostap_switch_mode(mode);
}

/**
 * It switches the wifi mode to 24G or 5G mode.
 *
 * @param mode The frequency mode to switch to.
 * @param channel The frequency for channel.
 * @param flag The mode option (res)
 */
int hccast_wifi_mgr_hostap_switch_mode_ex(hccast_wifi_freq_mode_e mode, int channel, int flag)
{
    return wifi_ctrl_hostap_switch_mode_ex(mode, channel, flag);
}

/**
 * It switches the wifi channel to 24G or 5G channel list.
 *
 * @param channel The hostap channel to switch to.
 */
int hccast_wifi_mgr_hostap_switch_channel(int channel)
{
    return wifi_ctrl_hostap_switch_channel(channel);
}

/**
 * It starts the hostapd daemon, configures the IP address of the interface, and starts the udhcpd
 * daemon
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_hostap_start()
{
    pthread_mutex_lock(&g_hostap_mutex);
    if (hostap_en == 0)
    {
        hccast_wifi_mgr_udhcpc_stop();
#ifdef HC_RTOS
        wifi_ctrl_do_op_abort();
        wifi_ctrl_hostap_thread_start();
        wifi_ctrl_do_op_reset();
#else
        hccast_wifi_mgr_hostap_enable();
#endif
        hccast_net_ifconfig(WIFI_CTRL_IFACE_NAME, HCCAST_HOSTAP_IP, HCCAST_HOSTAP_MASK, HCCAST_HOSTAP_GW);
        hccast_wifi_mgr_udhcpd_start();
        hostap_en = 1;
        printf("===================%s: hostap start done.====================\n", __func__);
    }
    pthread_mutex_unlock(&g_hostap_mutex);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It starts the hostapd daemon, configures the interface, and starts the udhcpd daemon
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_hostap_restart()
{
    pthread_mutex_lock(&g_hostap_mutex);
    hccast_wifi_mgr_hostap_enable();
    hccast_net_ifconfig(WIFI_CTRL_IFACE_NAME, HCCAST_HOSTAP_IP, HCCAST_HOSTAP_MASK, HCCAST_HOSTAP_GW);
    hccast_wifi_mgr_udhcpd_start();
    hostap_en = 1;
    printf("===================%s: hostap restart done.====================\n", __func__);
    pthread_mutex_unlock(&g_hostap_mutex);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It stops the hostapd service.
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_hostap_stop()
{
    pthread_mutex_lock(&g_hostap_mutex);
    if (hostap_en)
    {
        hccast_net_ifconfig(WIFI_CTRL_IFACE_NAME, "0.0.0.0", NULL, NULL);
#ifdef HC_RTOS
        hccast_wifi_mgr_op_abort();
        wifi_ctrl_hostap_thread_stop();
        hccast_wifi_mgr_op_reset();
#else
        hccast_wifi_mgr_hostap_disenable();
#endif
        hccast_wifi_mgr_udhcpd_stop();
        hostap_en = 0;
        printf("===================%s: hostap stop done.====================\n", __func__);
    }
    pthread_mutex_unlock(&g_hostap_mutex);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * > This function returns the status of the hostapd service
 *
 * @return The hostap_en variable is being returned.
 */
int hccast_wifi_mgr_get_hostap_status()
{
    int ret;

    pthread_mutex_lock(&g_hostap_mutex);
    ret = hostap_en;
    pthread_mutex_unlock(&g_hostap_mutex);

    return ret;
}

static void udhcpc_cb(unsigned int data)
{
    hccast_udhcp_result_t *in = (hccast_udhcp_result_t *)data;

    if (wifi_event_callback && in)
    {
        wifi_event_callback(HCCAST_WIFI_CONNECT_RESULT, NULL, in);
    }
}

static udhcp_conf_t g_sta_udhcpc_conf =
{
    .func   = udhcpc_cb,
    .ifname = UDHCP_IF_WLAN0,
    .pid    = 0,
    .run    = 0,
    .option = UDHCPC_ABORT_IF_NO_LEASE,
};

/**
 * It starts the udhcpc client (ifname: wlan0)
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_udhcpc_start()
{
    udhcpc_start(&g_sta_udhcpc_conf);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It stops the udhcpc client (ifname: wlan0).
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 */
int hccast_wifi_mgr_udhcpc_stop()
{
    udhcpc_stop(&g_sta_udhcpc_conf);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It enables the hostap interface.
 */
int hccast_wifi_mgr_hostap_enable()
{
#ifdef HC_RTOS
    wifi_ctrl_hostap_interface_enable(true);
#else
    wifi_ctrl_hostap_interface_enable(false);
    usleep(500);
    wifi_ctrl_hostap_interface_enable(true);
    usleep(500);
#endif
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It disables the hostap interface.
 */
int hccast_wifi_mgr_hostap_disenable()
{
    return wifi_ctrl_hostap_interface_enable(false);
}

/**
 *  Set according to wifi model
 *
 * @param wifi_model 0 for AP mode, 1 for STA mode
 *
 * @return the global variable wifi_model
 */
int hccast_wifi_mgr_set_wifi_model(int wifi_model)
{
    return g_wifi_model =  wifi_model;
}

/**
 * It returns the value of the global variable g_wifi_model.
 *
 * @return the global variable wifi_model.
 */
int hccast_wifi_mgr_get_wifi_model()
{
    return g_wifi_model;
}

/**
 * It initializes the P2P interface
 *
 * @param p2p_param the parameter structure of the p2p module, which is defined as follows:
 *
 * @return The return value is the return value of the function p2p_ctrl_init.
 */
int hccast_wifi_mgr_p2p_init(hccast_wifi_p2p_param_t *p2p_param)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    int i = -1;
    p2p_param_st params = {0};

    memcpy(params.wifi_ifname, WIFI_CTRL_IFACE_NAME, sizeof(params.wifi_ifname));
    memcpy(params.p2p_ifname, P2P_CTRL_IFACE_NAME, sizeof(params.p2p_ifname));
    memcpy(params.device_name, p2p_param->device_name, sizeof(params.device_name));

    params.state_update_func = (p2p_ctrl_state_callback)p2p_param->func_update_state;

    int host_en = hccast_wifi_mgr_get_hostap_status();
    if (host_en)
    {
        hccast_wifi_hostap_status_result_t hostap_result;

        memset(&hostap_result, 0, sizeof(hccast_wifi_hostap_status_result_t));
        if (wifi_ctrl_hostap_get_status(&hostap_result) == 0)
        {
            if (1 == hostap_result.channel || 6 == hostap_result.channel || 11 == hostap_result.channel)
            {
                params.listen_channel = hostap_result.channel;
            }
            else
            {
                params.listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
            }
        }
        else
        {
            params.listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
        }
    }
    else
    {
        hccast_wifi_status_result_t status_res = {0};
        ret = wifi_ctrl_get_status(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, &status_res);
        if (ret < 0)
        {
            hccast_log(LL_ERROR, "wifi_ctrl_get_status failed!\n");
        }

        /* 2.4G: 2412~2484 MHZ, 5G: 4915~5825 MHZ, 6G: 5955~7115 MHZ */
        if (status_res.freq > 0 && status_res.freq <= 2484)  // 2.4G
        {
            int channel = (status_res.freq - 2407) / 5;
            if ((channel == 1) || (channel >= 11))
            {
                params.listen_channel = 6;
            }
            else
            {
                params.listen_channel = 1;
            }
        }
        else if (status_res.freq >= 4915 && status_res.freq <= 5825) // 5G
        {
            params.listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
        }
        else // 6G
        {
            params.listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
        }

        hccast_log(LL_NOTICE, "sta wpa_state: %s, freq: %d, listen_channel: %d\n", \
                   status_res.wpa_state, status_res.freq, params.listen_channel);
    }

    params.oper_channel = HCCAST_P2P_OPER_CH_DEFAULT;
    ret = p2p_ctrl_init(&params);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "p2p_ctrl_init!\n");
        return ret;
    }

    //wifi_ctrl_do_remove_net_all(P2P_CTRL_IFACE_NAME);

    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It un-initializes the p2p_ctrl module.
 *
 * @return The return value is the return value of the function p2p_ctrl_uninit().
 */
int hccast_wifi_mgr_p2p_uninit()
{
    return p2p_ctrl_uninit();
}

/**
 * > Initialize the P2P device
 *
 * @return The return value is the return value of p2p_ctrl_device_init().
 */
void hccast_wifi_mgr_p2p_device_init()
{
    return p2p_ctrl_device_init();
}

/**
 * when p2p connected, get p2p role (go).
 */
int hccast_wifi_mgr_p2p_is_go()
{
    return p2p_ctrl_device_is_go();
}

/**
 * It enables the p2p device.
 *
 * @param enable true to enable P2P, false to disable P2P.
 *
 * @return enables value.
 */
bool hccast_wifi_mgr_p2p_set_enable(bool enable)
{
    return p2p_ctrl_set_enable(enable);
}

/**
 *  get p2p device enable status
 */
bool hccast_wifi_mgr_p2p_get_enable()
{
    return p2p_ctrl_get_enable();
}

/**
 * It returns the IP address of the device.
 *
 * @return The IP address of the device.
 */
unsigned int hccast_wifi_mgr_p2p_get_ip()
{
    return p2p_ctrl_get_device_ip();
}

/**
 * It returns the RTSP port of the device.
 *
 * @return The RTSP port of the device.
 */
int hccast_wifi_mgr_p2p_get_rtsp_port()
{
    return p2p_ctrl_get_device_rtsp_port();
}

/**
 * It returns the connection status of the P2P connection.
 */
int hccast_wifi_mgr_p2p_get_connect_stat()
{
    return p2p_ctrl_get_connect_stat();
}

/**
 * It sets the connection status of the P2P connection.
 *
 * @param stat true if connected, false if not connected
 */
int hccast_wifi_mgr_p2p_set_connect_stat(bool stat)
{
    return p2p_ctrl_set_connect_stat(stat);
}

/**
 * get the wifi is currently connecting flag
 */
int hccast_wifi_mgr_get_wifi_is_connecting()
{
    return wifi_ctrl_is_connecting();
}

/**
 * Initialize the WiFi manager
 *
 * @param func The callback function that will be called when the WiFi connection status changes.
 *
 * @return 0
 */
int hccast_wifi_mgr_init(hccast_wifi_mgr_event_callback func)
{
    wifi_event_callback = func;
#ifdef __linux__
    wifi_ctrl_init(wifi_event_callback);
#endif
    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * > This function is used to unregister the callback function of the WiFi module
 *
 * @return 0
 */
int hccast_wifi_mgr_uninit()
{
    wifi_event_callback = NULL;

    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It sets the udhcpd configuration
 *
 * @param conf udhcpd configuration
 *
 * @return HCCAST_WIFI_ERR_NO_ERROR
 *
 * @note: conf.func == NULL will use hccast default callback func,
 * otherwise, will use conf.func when udhcpd offer ip success.
 */
int hccast_wifi_mgr_set_udhcpd_conf(udhcp_conf_t conf)
{
    memcpy(&g_hostap_udhcpd_conf, &conf, sizeof(g_hostap_udhcpd_conf));

    if (!conf.func)
    {
        g_hostap_udhcpd_conf.func = udhcpd_cb;
    }

    if (UDHCP_IF_WLAN0 == conf.ifname && strlen(conf.ip_host_def) > 0)
    {
        memcpy(HCCAST_HOSTAP_IP, conf.ip_host_def, sizeof(HCCAST_HOSTAP_IP));
        memcpy(HCCAST_HOSTAP_GW, conf.ip_host_def, sizeof(HCCAST_HOSTAP_IP));
    }

    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It sets the udhcpd configuration
 *
 * @param conf udhcpd configuration
 *
 * @return 0: success.
 *
 * @note: conf.func == NULL will use hccast default callback func,
 * otherwise, will use conf.func when udhcpc get ip success or failed.
 */
int hccast_wifi_mgr_set_udhcpc_conf(udhcp_conf_t conf)
{
    memcpy(&g_sta_udhcpc_conf, &conf, sizeof(g_sta_udhcpc_conf));

    if (!conf.func)
    {
        g_sta_udhcpc_conf.func = udhcpc_cb;
    }

    return HCCAST_WIFI_ERR_NO_ERROR;
}

/**
 * It sets the IP address, netmask and gateway for the host network.
 *
 * @param inf the interface name, such as wlan0, p2p0, etc.
 * @param ip the IP address of the interface
 * @param netmask such as 255.255.255.0
 * @param gateway the gateway of the network
 *
 * @return 0: success, < 0: failed
 */
int hccast_wifi_mgr_set_host_network(char *inf, char *ip, char *netmask, char *gateway)
{
    if (!inf || !ip || !netmask)
    {
        hccast_log(LL_ERROR, "%s param error!", __func__);
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    if (strcasecmp(inf, "wlan0") == 0)
    {
        if (strlen(ip) > 0)
        {
            memcpy(HCCAST_HOSTAP_IP, ip, sizeof(HCCAST_HOSTAP_IP));
        }

        if (strlen(netmask) > 0)
        {
            memcpy(HCCAST_HOSTAP_MASK, netmask, sizeof(HCCAST_HOSTAP_IP));
        }

        if (gateway && strlen(gateway) > 0)
        {
            memcpy(HCCAST_HOSTAP_GW, gateway, sizeof(HCCAST_HOSTAP_IP));
        }
    }
    else if (strcasecmp(inf, "p2p0") == 0)
    {
        if (strlen(ip) > 0)
        {
            memcpy(HCCAST_P2P_IP, ip, sizeof(HCCAST_P2P_IP));
        }

        if (strlen(netmask) > 0)
        {
            memcpy(HCCAST_P2P_MASK, netmask, sizeof(HCCAST_P2P_MASK));
        }

        if (gateway && strlen(gateway) > 0)
        {
            memcpy(HCCAST_P2P_GW, gateway, sizeof(HCCAST_P2P_GW));
        }
    }
    else
    {
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    return HCCAST_WIFI_ERR_NO_ERROR;
}

// ! The above functions are usual (include hclinux/hcrtos)
// ! The following functions are only for hcrtos

#ifdef HC_RTOS
int hccast_wifi_mgr_enter_sta_mode(void)
{
    int ret;
    if (1 == station_en)
    {
        return 0;
    }

    hccast_wifi_mgr_op_abort();
    ret = wifi_ctrl_sta_thread_start();
    if (0 == ret)
    {
        station_en = 1;
    }
    else
    {
        station_en = 0;
    }

    hccast_wifi_mgr_op_reset();

    return ret;
}

int hccast_wifi_mgr_exit_sta_mode(void)
{
    int ret;
    if (0 == station_en)
    {
        return 0;
    }

    hccast_wifi_mgr_op_abort();
    ret = wifi_ctrl_sta_thread_stop();
    if (0 == ret)
    {
        station_en = 0;
    }

    hccast_wifi_mgr_op_reset();

    return ret;
}

/**
 * > This function returns the status of the hostapd service
 *
 * @return The hostap_en variable is being returned.
 */
int hccast_wifi_mgr_get_station_status(void)
{
    return station_en;
}

int hccast_wifi_mgr_p2p_wps_pbc()
{
    return wifi_ctrl_p2p_wps_pbc();
}

int hccast_wifi_mgr_trigger_scan(char *inf)
{
    return p2p_ctrl_iwlist_scan_cmd(inf);
}

#include <sys/ioctl.h>
#include <uapi/linux/wireless.h>
int hccast_wifi_mgr_get_best_channel(int argc, int **argv)
{
    (void)argc;
    (void)argv;
    uint32_t val = 0;;
    struct iwreq iwr;
    int ioctl_sock = -1;
    memset(&iwr, 0, sizeof(iwr));
    ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (ioctl_sock < 0)
    {
        hccast_log(LL_ERROR, "socket[PF_INET,SOCK_DGRAM]");
        return HCCAST_WIFI_ERR_SOCKET_FAILED;
    }

    strlcpy(iwr.ifr_name, "wlan0", IFNAMSIZ);
    iwr.u.data.pointer = &val;
    iwr.u.data.length = sizeof(val);
    if (ioctl(ioctl_sock, IW_PRIV_IOCTL_BEST_CHANNEL, &iwr) < 0)
    {
        hccast_log(LL_ERROR, "ioctl[IW_PRIV_IOCTL_BEST_CHANNEL]");
        close(ioctl_sock);
        return HCCAST_WIFI_ERR_IOCTL_FAILED;
    }

    hccast_log(LL_NOTICE, "%s:%d,2.4G: %lu, 5G: %lu\n", __func__, __LINE__, val & 0xFFFF, (val >> 16) & 0xFFFF);

    if (argc >= 1)
    {
        argv[0] = val & 0xFFFF;
        argv[1] = (val >> 16) & 0xFFFF;
    }

    close(ioctl_sock);

    return HCCAST_WIFI_ERR_NO_ERROR;
}

int hccast_wifi_mgr_set_best_channel(int *ch24G, int *ch5G)
{
    if (!ch24G || !ch5G)
    {
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    return wifi_ctrl_hostap_set_best_channel((int)ch24G, (int)ch5G);
}

int hccast_wifi_mgr_hostap_store_conf(hccast_wifi_hostap_conf_t *conf)
{
    return wifi_ctrl_hostap_store_config(conf);
}

int hccast_wifi_mgr_get_support_miracast()
{
    uint32_t val = 0;;
    struct iwreq iwr;
    int ioctl_sock = -1;
    memset(&iwr, 0, sizeof(iwr));
    ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (ioctl_sock < 0)
    {
        hccast_log(LL_ERROR, "socket[PF_INET,SOCK_DGRAM]");
        return HCCAST_WIFI_ERR_SOCKET_FAILED;
    }

    strlcpy(iwr.ifr_name, "wlan0", IFNAMSIZ);
    iwr.u.data.pointer = &val;
    iwr.u.data.length = sizeof(val);
    if (ioctl(ioctl_sock, IW_PRIV_IOCTL_IS_SUPPORT_MIRACAST, &iwr) < 0)
    {
        hccast_log(LL_ERROR, "ioctl[IW_PRIV_IOCTL_BEST_CHANNEL]");
        close(ioctl_sock);
        return HCCAST_WIFI_ERR_IOCTL_FAILED;
    }

    close(ioctl_sock);

    return val;
}

#endif // HC_RTOS
