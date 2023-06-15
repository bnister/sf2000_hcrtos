#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <wpa_ctrl.h>
#include <pthread.h>

#ifdef __linux__
    #include <sys/prctl.h>
#endif

#include "hccast_net.h"
#include "hccast_wifi_mgr.h"
#include "hccast_wifi_ctrl.h"
#include "hccast_p2p_ctrl.h"
#include "hccast_mira.h"
#include "hccast_log.h"

static pthread_mutex_t g_wifi_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_wifi_p2p_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  g_wifi_p2p_cond;

static hccast_wifi_stat_e g_wifi_op_stat = HCCAST_WIFI_STAT_NONE;

/* ***************************************** */

int wifi_ctrl_msgCb_func(char *msg, size_t len)
{
    hccast_log(LL_INFO, "%s: len:%d, %s\n", __func__, len, msg);
    return 0;
}

/**
 * The above function is used to split a string into several substrings.
 *
 * @param src The source string to be split.
 * @param separator the separator to use to split the string.
 * @param dest The address of the array that receives the substrings.
 * @param num the number of substrings
 *
 * @return the number of words in the string.
 */
static void split(char *src, const char *separator, char **dest, int *num)
{
    char *pNext;
    char *saveptr;
    int count = 0;
    if (src == NULL || strlen(src) == 0)
        return;
    if (separator == NULL || strlen(separator) == 0)
        return;
    pNext = (char *)strtok_r(src, separator, &saveptr);
    while (pNext != NULL)
    {
        *dest++ = pNext;
        ++count;
        pNext = (char *)strtok_r(NULL, separator, &saveptr);
    }
    *num = count;
}

static int result_get(char *str, char *key, char *val, int val_len)
{
    if (NULL == str || NULL == key || NULL == val)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return -1;
    }

    //printf("query key = %s\n", key);
    char keys[64] = {0};
    char vals[256] = {0};
    char *token;
    char *saveptr;

    char *strs = strdup(str);
    token = strtok_r(strs, "\n", &saveptr);

    while (token != NULL)
    {
        sscanf(token, "%[^=]=%[^'\n']", keys, vals);

        if (!strcmp(key, keys))
        {
            memcpy(val, vals, val_len);
            break;
        }

        token = strtok_r(NULL, "\n", &saveptr);
    }

    free(strs);

    return 0;
}

static int get_line_from_buf(int index, char *line, char *buf)
{
    int i = 0;
    int j;
    int endcnt = -1;
    char *linestart = buf;
    int len;

    if (NULL == line || NULL == buf)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return -1;
    }

    while (1)
    {
        if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == '\0')
        {
            endcnt++;
            if (index == endcnt)
            {
                len = &buf[i] - linestart;
                strncpy(line, linestart, len);
                line[len] = '\0';
                return 0;
            }
            else
            {
                /* update linestart */
                for (j = i + 1; buf[j];)
                {
                    if (buf[j] == '\n' || buf[j] == '\r')
                        j++;
                    else
                        break;
                }
                if (!buf[j])
                    return -1;
                linestart = &buf[j];
                i = j;
            }
        }

        if (!buf[i])
            return -1;
        i++;
    }
}

#ifdef USE_IFNAME_DYN_GET

#include <dirent.h>

/**
 * It recursively traverses the /var/run/XXX directory and returns the names of all the network
 * interfaces it finds
 *
 * @param directory the directory to search for the interface name
 * @param ifname the interface name
 * @param n the number of interfaces
 *
 * @return the number of interfaces found.
 */
int get_ifname_forme_run_dir(const char *directory, char *ifname[], int *n)
{
    DIR *dir;
    static struct dirent *dirp;
    char sub_dir_path[512];
    *n = 0;

    // open directory
    if ((dir = opendir(directory)) == 0) return -1;

    while ((dirp = readdir(dir)) != NULL)
    {
        if (strncmp(dirp->d_name, ".", 1) == 0) continue;

        if (DT_SOCK == dirp->d_type /*&& 0 == strncasecmp (dirp->d_name, "wlan", 4)*/)
        {
            //printf("type:%d, name = %s/%s\n", dirp->d_type, directory, dirp->d_name);
            ifname[(*n)++] = dirp->d_name;
        }

        if (dirp->d_type == DT_DIR)
        {
            sprintf(sub_dir_path, "%s/%s", directory, dirp->d_name);
            get_ifname_forme_run_dir(sub_dir_path, ifname, n);
        }
    }

    closedir(dir);

    return 0;
}
#endif

/**
 * It runs a command on the wpa_supplicant or hostapd process
 *
 * @param ifname the interface name of the wireless network card, such as wlan0, p2p0, etc.
 * @param mode HCCAST_WIFI_MODE_HOSTAP or HCCAST_WIFI_MODE_STA
 * @param cmd the command to be executed
 * @param result the result of the command execution
 * @param len the length of the result buffer
 *
 * @return The return value  0 - success, other - failed.
 *
 * common CMD:
 *  "STATUS"
 *  "STATUS-DRIVER"
 *  "STATUS-VERBOSE"
 *  "SIGNAL_POLL"
 *  ""
 */
int wifi_ctrl_run_cmd(const char *ifname, hccast_wifi_mode_e mode, char *cmd, char *result, unsigned int *len)
{
    char path[128] = {"\0"};

#ifdef HC_RTOS
    int port = 0;

    if (eloop_is_run() != 1)
    {
        hccast_log(LL_ERROR, "%s: WPAS NO RUN!\n", __func__);
        return HCCAST_WIFI_ERR_CMD_WPAS_NO_RUN;
    }
#endif

    if (NULL == ifname || NULL == cmd || NULL == result || NULL == len || mode >= HCCAST_WIFI_MODE_KEEP)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    if (HCCAST_WIFI_MODE_HOSTAP == mode)
    {
        sprintf(path, WIFI_CTRL_PATH_HOSTAP"/%s", ifname);
#ifdef HC_RTOS
        if (strcmp(ifname, HS_CTRL_IFACE_NAME) == 0)
        {
            port = WPA_CTRL_HOSTAPD_IFACE_PORT;
        }
        else if (strcmp(ifname, P2P_CTRL_IFACE_NAME) == 0)
        {
            port = WPA_CTRL_P2P_IFACE_PORT;
        }
        else
        {
            hccast_log(LL_ERROR, "%s mode error! (%d)\n", __func__, mode);
            return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        }
#endif
    }
    else if (HCCAST_WIFI_MODE_STA == mode)
    {
        sprintf(path, WIFI_CTRL_PATH_STA"/%s", ifname);
#ifdef HC_RTOS
        if (strcmp(ifname, WIFI_CTRL_IFACE_NAME) == 0)
        {
            port = WPA_CTRL_WPA_IFACE_PORT;
        }
        else if (strcmp(ifname, P2P_CTRL_IFACE_NAME) == 0)
        {
            port = WPA_CTRL_P2P_IFACE_PORT;
        }
        else
        {
            hccast_log(LL_ERROR, "%s mode error! (%d)\n", __func__, mode);
            return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        }
#endif
    }
    else
    {
        hccast_log(LL_ERROR, "mode error! (%d)\n", mode);
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

#ifdef __linux__
    if (access(path, F_OK)) // no exist
    {
        hccast_log(LL_ERROR, "ctrl_iface %s non-existent!\n", path);
        return HCCAST_WIFI_ERR_CMD_WPAS_NO_RUN;
    }
    else
    {
        //printf("ctrl_iface: %s ", path);
    }
#endif

    pthread_mutex_lock(&g_wifi_mutex);
#ifdef HC_RTOS
    struct wpa_ctrl *wpa_ctrl = wpa_ctrl_open(path, port);
#else
    struct wpa_ctrl *wpa_ctrl = wpa_ctrl_open(path);
#endif
    if (!wpa_ctrl)
    {
        hccast_log(LL_ERROR, "wpa_ctrl_open failed: %s!\n", strerror(errno));
        pthread_mutex_unlock(&g_wifi_mutex);
        return HCCAST_WIFI_ERR_CMD_WPAS_OPEN_FAILED;
    }

    int ret = wpa_ctrl_request(wpa_ctrl, cmd, strlen(cmd), result, len, (void *)wifi_ctrl_msgCb_func);
    result[*len] = 0;
    pthread_mutex_unlock(&g_wifi_mutex);

    hccast_log(LL_SPEW, "result:%s\n", result);

    wpa_ctrl_close(wpa_ctrl);

    return ret;
}

/**
 * It runs a command on the wpa_supplicant process
 *
 * @param ifname the name of the interface, such as wlan0
 * @param mode the mode of the wifi, which can be HCCAST_WIFI_MODE_HOSTAP or HCCAST_WIFI_MODE_STA etc.
 * @param cmd the command to be executed
 * @param result the result of the command execution
 * @param len the length of the result
 *
 * @return The return value is the result of the wpa_ctrl_request function.
 *
 * NOTE: When not keeping status, will be called wifi_ctrl_run_cmd()
 * HCCAST_WIFI_MODE_XXX_KEEP enable keeping, HCCAST_WIFI_MODE_XXX disable keeping.
 */
int wifi_ctrl_run_cmd_keep(char *ifname, hccast_wifi_mode_e mode, char *cmd, char *result, unsigned int *len)
{
    char path[128] = {"\0"};
    struct wpa_ctrl *wpa_ctrl = NULL;
    static struct wpa_ctrl *hs_wpas_ctrl_keep    = NULL;
    static struct wpa_ctrl *sta_wpas_ctrl_keep   = NULL;

#ifdef HC_RTOS
    int port = 0;

    if (eloop_is_run() != 1)
    {
        hccast_log(LL_ERROR, "%s: WPAS NO RUN!\n", __func__);
        return HCCAST_WIFI_ERR_CMD_WPAS_NO_RUN;
    }
#endif

    if (ifname == NULL || NULL == cmd || NULL == result || NULL == len)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    if (HCCAST_WIFI_MODE_HOSTAP_KEEP == mode)
    {
        if (hs_wpas_ctrl_keep)
        {
            wpa_ctrl = hs_wpas_ctrl_keep;
        }
        else
        {
            sprintf(path, WIFI_CTRL_PATH_HOSTAP"/%s", ifname);
#ifdef HC_RTOS
            if (strcmp(ifname, HS_CTRL_IFACE_NAME) == 0)
            {
                port = WPA_CTRL_HOSTAPD_IFACE_PORT;
            }
            else if (strcmp(ifname, P2P_CTRL_IFACE_NAME) == 0)
            {
                port = WPA_CTRL_P2P_IFACE_PORT;
            }
            else
            {
                hccast_log(LL_ERROR, "%s mode error! (%d)\n", __func__, mode);
                return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
            }

            wpa_ctrl = hs_wpas_ctrl_keep = wpa_ctrl_open(path, port);
#else
            wpa_ctrl = hs_wpas_ctrl_keep = wpa_ctrl_open(path);
#endif
        }
    }
    else if (HCCAST_WIFI_MODE_HOSTAP == mode)
    {
        if (hs_wpas_ctrl_keep)
        {
            wpa_ctrl = hs_wpas_ctrl_keep;
        }
    }
    else if (HCCAST_WIFI_MODE_STA_KEEP == mode)
    {
        if (sta_wpas_ctrl_keep)
        {
            wpa_ctrl = sta_wpas_ctrl_keep;
        }
        else
        {
            sprintf(path, WIFI_CTRL_PATH_STA"/%s", ifname);
#ifdef HC_RTOS
            if (strcmp(ifname, WIFI_CTRL_IFACE_NAME) == 0)
            {
                port = WPA_CTRL_WPA_IFACE_PORT;
            }
            else if (strcmp(ifname, P2P_CTRL_IFACE_NAME) == 0)
            {
                port = WPA_CTRL_P2P_IFACE_PORT;
            }
            else
            {
                hccast_log(LL_ERROR, "%s mode error! (%d)\n", __func__, mode);
                return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
            }

            wpa_ctrl = sta_wpas_ctrl_keep = wpa_ctrl_open(path, port);
#else
            wpa_ctrl = sta_wpas_ctrl_keep = wpa_ctrl_open(path);
#endif
        }
    }
    else if (HCCAST_WIFI_MODE_STA == mode)
    {
        if (sta_wpas_ctrl_keep)
        {
            wpa_ctrl = sta_wpas_ctrl_keep;
        }
    }
    else
    {
        hccast_log(LL_ERROR, "mode error! (%d)\n", mode);
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    if (!wpa_ctrl)
    {
        hccast_log(LL_WARNING, "wpas ctrl no keep, will call wifi_ctrl_run_cmd.\n");
        return wifi_ctrl_run_cmd(ifname, mode - 0x10, cmd, result, len);;
    }

    pthread_mutex_lock(&g_wifi_mutex);
    int ret = wpa_ctrl_request(wpa_ctrl, cmd, strlen(cmd), result, len, (void *)wifi_ctrl_msgCb_func);
    result[*len] = 0;
    pthread_mutex_unlock(&g_wifi_mutex);

    hccast_log(LL_SPEW, "result:\n%s\n", result);

    if ((mode != HCCAST_WIFI_MODE_HOSTAP_KEEP) && (mode != HCCAST_WIFI_MODE_STA_KEEP))
    {
        if (wpa_ctrl)
        {
            wpa_ctrl_close(wpa_ctrl);
            wpa_ctrl = NULL;
            if (mode == HCCAST_WIFI_MODE_HOSTAP)
            {
                hs_wpas_ctrl_keep = NULL;
            }
            else if (mode == HCCAST_WIFI_MODE_STA)
            {
                sta_wpas_ctrl_keep = NULL;
            }
            else
            {
                hs_wpas_ctrl_keep = NULL;
                sta_wpas_ctrl_keep = NULL;
            }
        }
    }

    return ret;
}

/**
 * It gets the current status of the WiFi interface
 *
 * @param ifname the interface name, such as wlan0, p2p0, etc.
 * @param mode the mode of the interface, STA or AP
 * @param result the result of the query
 */
int wifi_ctrl_get_status(const char *ifname, hccast_wifi_mode_e mode, hccast_wifi_status_result_t *result)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char val[512] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    char *cmd = "STATUS";

    if (NULL == result || NULL == ifname)
    {
        hccast_log(LL_ERROR, "param error!\n");
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        goto ERROR;
    }

    memset(result, 0x00, sizeof(hccast_wifi_status_result_t));
    ret = wifi_ctrl_run_cmd(ifname, mode, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        goto ERROR;
    }

    if (strncmp(reply, "FAIL", 4) == 0)
    {
        hccast_log(LL_ERROR, "%s (FAIL)\n", cmd);
        ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        goto ERROR;
    }

    result_get(reply, "bssid", result->bssid, sizeof(result->bssid));

    if (result_get(reply, "freq", val, sizeof(val)) == 0)
    {
        result->freq = strtol(val, NULL, 10);
    }

    result_get(reply, "ssid", val, sizeof(val));
    wifi_ctrl_chinese_conversion(val);

    memcpy(result->ssid, val, sizeof(result->ssid));

    if (result_get(reply, "id", val, sizeof(val)) == 0)
    {
        result->id = strtol(val, NULL, 10);
    }

    result_get(reply, "mode", result->mode, sizeof(result->mode));
    result_get(reply, "pairwise_cipher", result->pairwise_cipher, sizeof(result->pairwise_cipher));
    result_get(reply, "group_cipher", result->group_cipher, sizeof(result->group_cipher));
    result_get(reply, "key_mgmt", result->key_mgmt, sizeof(result->key_mgmt));
    result_get(reply, "wpa_state", result->wpa_state, sizeof(result->wpa_state));
    result_get(reply, "ip_address", result->ip_address, sizeof(result->ip_address));
    result_get(reply, "address", result->address, sizeof(result->address));
    result_get(reply, "uuid", result->uuid, sizeof(result->uuid));
    result_get(reply, "p2p_device_address", result->p2p_device_address, sizeof(result->p2p_device_address));

ERROR:
    return ret;
}

/**
 * It gets the signal strength of the current connection
 *
 * @param result the result of the signal poll
 */
int wifi_ctrl_get_signal_poll(hccast_wifi_signal_poll_result_t *result)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char val[512] = {0};
    char reply[1024] = {0};

    unsigned int len = sizeof(reply) - 1;
    char *cmd = "SIGNAL_POLL";

    if (NULL == result)
    {
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        hccast_log(LL_ERROR, "[%s-%d]: param error!\n", __func__, __LINE__);
        goto ERROR;
    }

    memset(result, 0x00, sizeof(hccast_wifi_signal_poll_result_t));

    ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        goto ERROR;
    }

    if (strncmp(reply, "FAIL", 4) == 0)
    {
        ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        hccast_log(LL_ERROR, "%s (FAIL)\n", cmd);
        goto ERROR;
    }

    if (result_get(reply, "RSSI", val, sizeof(val)) == 0)
    {
        result->rssi = strtol(val, NULL, 10);
    }

    if (result_get(reply, "LINKSPEED", val, sizeof(val)) == 0)
    {
        result->linkspeed = strtol(val, NULL, 10);
    }

    if (result_get(reply, "FREQUENCY", val, sizeof(val)) == 0)
    {
        result->frequency = strtol(val, NULL, 10);
    }

    if (result_get(reply, "NOISE", val, sizeof(val)) == 0)
    {
        result->noise = strtol(val, NULL, 10);
    }

ERROR:
    return ret;
}

/**
 * This function is used to get the list of APs that the device connected (save)
 *
 * @param info_res The structure of the networks information returned by the function
 *
 * @return
 */
int wifi_ctrl_get_list_net(char *ifname, hccast_wifi_list_net_result_t *res)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[2048] = {0};
    unsigned int len = sizeof(reply) - 1;
    char ssid[256] = {0};
    char *cmd = "LIST_NETWORKS";

    if (NULL == ifname || NULL == res)
    {
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        hccast_log(LL_ERROR, "param error!\n");
        return ret;
    }

    ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        return ret;
    }

    char line[1024] = {0};
    int i = 0, j = 0;

    for (i = 1; !get_line_from_buf(i, line, reply); i++, j++)
    {
        memset(ssid, 0, sizeof(ssid));

        // network id / ssid / bssid / flags  <----- i = 0;
        sscanf(line, "%s\t%[^'\t']\t%s\t%s", res->netinfo[j].net_id, ssid, res->netinfo[j].bssid, res->netinfo[j].flags);
        if (strlen(ssid) == 0)
        {
            hccast_log(LL_ERROR, "list ssid is null!\n");
            --j;
            continue;
        }

        wifi_ctrl_chinese_conversion(ssid);

        memcpy(res->netinfo[j].ssid, ssid, sizeof(res->netinfo[j].ssid));

        hccast_log(LL_INFO, "net_id: %s ssid: %s\n", res->netinfo[j].net_id, res->netinfo[j].ssid);
    }

    res->net_num = i - 1;

    return ret;
}

/**
 * It removes a network from the wpa_supplicant.conf file.
 *
 * @param net_id the network id of the network to be removed.
 *
 * @return The return value is the network id of the newly created network.
 */
int wifi_ctrl_do_remove_net_by_id(char *ifname, char *net_id)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1048] = {0};
    unsigned int len = sizeof(reply) - 1;
    char cmd[128] = {0};

    sprintf(cmd, "REMOVE_NETWORK %s", net_id);
    hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
    ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        return ret;
    }

    return ret;
}

/**
 * It removes all the networks from the interface.
 *
 * @param ifname the interface name of the wifi device, such as wlan0
 *
 * @return The return value is the number of networks that were removed.
 */
int wifi_ctrl_do_remove_net_all(char *ifname)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1048] = {0};
    unsigned int len = sizeof(reply) - 1;
    char cmd[128] = {0};

    sprintf(cmd, "REMOVE_NETWORK all");
    hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
    ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        return ret;
    }

    return ret;
}

int wifi_ctrl_p2p_lock()
{
    return pthread_mutex_lock(&g_wifi_p2p_mutex);
}

int wifi_ctrl_p2p_unlock()
{
    return pthread_mutex_unlock(&g_wifi_p2p_mutex);
}

int wifi_ctrl_signal()
{
    return pthread_cond_signal(&g_wifi_p2p_cond);
}

int wifi_ctrl_wait_scan_timeout(unsigned int sec)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    struct timespec tv;
    wifi_ctrl_p2p_lock();
    clock_gettime(CLOCK_MONOTONIC, &tv);
    tv.tv_sec += sec;
    ret = pthread_cond_timedwait(&g_wifi_p2p_cond, &g_wifi_p2p_mutex, &tv);
    wifi_ctrl_p2p_unlock();

    return ret;
}

int wifi_ctrl_wait_connect_timeout(unsigned int sec)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    struct timespec tv;
    wifi_ctrl_p2p_lock();
    clock_gettime(CLOCK_MONOTONIC, &tv);
    tv.tv_sec += sec;
    ret = pthread_cond_timedwait(&g_wifi_p2p_cond, &g_wifi_p2p_mutex, &tv);
    wifi_ctrl_p2p_unlock();

    return ret;
}

/**
 * It connects to a WiFi AP
 *
 * @param ap_info the structure of the AP information, including the SSID and password of the AP.
 *
 * @return The return value is the number of bytes that were written to the file.
 */
int wifi_ctrl_do_ap_connect(const hccast_wifi_ap_info_t *ap_info)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    int net_id = -1;
    int i = 0;
    char cmd[256] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    int scan_ssid = 1;//for find out hidden wifi.

    if (NULL == ap_info)
    {
        hccast_log(LL_ERROR, "ap = null!\n");
        return ret;
    }

    // step 1a: check ap ssid exist?
    hccast_wifi_list_net_result_t *net_res = NULL;
    net_res = (hccast_wifi_list_net_result_t *)calloc(sizeof(hccast_wifi_list_net_result_t), 1);
    if (net_res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return HCCAST_WIFI_ERR_MEM;
    }

    ret = wifi_ctrl_get_list_net(WIFI_CTRL_IFACE_NAME, net_res);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
        if (net_res)
        {
            free(net_res);
        }
        return ret;
    }

    for (i = 0; i < net_res->net_num; i++)
    {
        hccast_log(LL_INFO, "%d net id: %s, ssid: %s\n", i, net_res->netinfo[i].net_id, net_res->netinfo[i].ssid);
        if (!strncmp(net_res->netinfo[i].ssid, ap_info->ssid, sizeof(net_res->netinfo[i].ssid)))
        {
            net_id = atoi(net_res->netinfo[i].net_id);
            break;
        }
    }

    if (i >= APLIST_NUM)
    {
        hccast_log(LL_WARNING, "aplist num is full, will set net_id: 0!\n");
        net_id = 0;
    }
    else if (-1 == net_id)
    {
        // step 1: ADD_NETWORK
        sprintf(cmd, "%s", "ADD_NETWORK");
        hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
        ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
            if (net_res)
            {
                free(net_res);
            }
            return ret;
        }
        net_id = atoi(reply);
    }
    else
    {
        hccast_log(LL_NOTICE, "-> foud id: %d, ssid: \"%s\"\n", net_id, net_res->netinfo[i].ssid);
    }

    // step 2a: set_network 0 ssid "ap"
    memset(reply, 0, sizeof(reply));
    len = sizeof(reply) - 1;
    sprintf(cmd, "SET_NETWORK %d ssid \"%s\"", net_id, ap_info->ssid);
    hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s, ret = %d", cmd, ret);
        if (net_res)
        {
            free(net_res);
        }
        return ret;
    }

    if (!strcmp(reply, "ok"))
    {
        hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);

        if (net_res)
        {
            free(net_res);
        }
        return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
    }

    // step 2b: bssid 0 "bssid"
#if 0
    memset(reply, 0, sizeof(reply));
    len = sizeof(reply) - 1;
    sprintf(cmd, "BSSID %d \"%s\"", net_id, ap_info->bssid);
    hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        printf("%s, ret = %d", cmd, ret);
        if (net_res)
        {
            free(net_res);
        }
        return ret;
    }

    if (!strcmp(reply, "ok"))
    {
        hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);

        if (net_res)
        {
            free(net_res);
        }
        return -1;
    }
#endif

    // step 3a: WPA3
    if (strlen(ap_info->pwd) >= 8 \
            && HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_SAE == ap_info->encryptMode)
    {
        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d sae_password \"%s\"", net_id, ap_info->pwd); //
        hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
        ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

            if (net_res)
            {
                free(net_res);
            }
            return ret;
        }

        if (!strcmp(reply, "ok"))
        {
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);

            if (net_res)
            {
                free(net_res);
            }
            return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d key_mgmt SAE", net_id); //
        hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
        ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

            if (net_res)
            {
                free(net_res);
            }
            return ret;
        }

        if (!strcmp(reply, "ok"))
        {
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);

            if (net_res)
            {
                free(net_res);
            }
            return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d ieee80211w 2", net_id); //
        hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
        ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

            if (net_res)
            {
                free(net_res);
            }
            return ret;
        }

        if (!strcmp(reply, "ok"))
        {
            hccast_log(LL_ERROR, "%s\n", reply);

            if (net_res)
            {
                free(net_res);
            }
            return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }
    }
    // step 3b: WPA/WPA2
    else if (strlen(ap_info->pwd) >= 8 && \
             (HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_TKIP == ap_info->encryptMode \
              || HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_AES == ap_info->encryptMode \
              || HCCAST_WIFI_ENCRYPT_MODE_WPAPSK_AES == ap_info->encryptMode) \
            )
    {
        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d psk \"%s\"", net_id, ap_info->pwd);
        hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
        ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

            if (net_res)
            {
                free(net_res);
            }
            return ret;
        }

        if (!strcmp(reply, "ok"))
        {
            hccast_log(LL_ERROR, "%s\n", reply);

            if (net_res)
            {
                free(net_res);
            }
            return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }
    }
    // step 3c: WEP
    else if (strlen(ap_info->pwd) >= 8 && \
             (HCCAST_WIFI_ENCRYPT_MODE_OPEN_WEP == ap_info->encryptMode \
              || HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP == ap_info->encryptMode)
            )
    {
        // SET_NETWORK 0 key_mgmt NONE
        // SET_NETWORK 0 wep_key0 passwd
        // SET_NETWORK 0 wep_tx_keyidx 0
        // SET_NETWORK %d auth_alg SHARED (only share mode)
        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d key_mgmt NONE", net_id);
        hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
        ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

            if (net_res)
            {
                free(net_res);
            }
            return ret;
        }

        if (!strcmp(reply, "ok"))
        {
            hccast_log(LL_ERROR, "%s\n", reply);

            if (net_res)
            {
                free(net_res);
            }
            return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d wep_key%d %s", net_id, ap_info->keyIdx - 1, ap_info->pwd);
        hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
        ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

            if (net_res)
            {
                free(net_res);
            }
            return ret;
        }

        if (!strcmp(reply, "ok"))
        {
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);

            if (net_res)
            {
                free(net_res);
            }
            return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d wep_tx_keyidx %d", net_id, ap_info->keyIdx - 1);
        hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
        ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

            if (net_res)
            {
                free(net_res);
            }
            return ret;
        }

        if (!strcmp(reply, "ok"))
        {
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);

            if (net_res)
            {
                free(net_res);
            }
            return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }

        if (HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP == ap_info->encryptMode)
        {
            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;
            sprintf(cmd, "SET_NETWORK %d auth_alg SHARED", net_id);
            hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
            ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

                if (net_res)
                {
                    free(net_res);
                }
                return ret;
            }

            if (!strcmp(reply, "ok"))
            {
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);

                if (net_res)
                {
                    free(net_res);
                }
                return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            }
        }
    }
    // step 3d: OPEN
    else
    {
        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d key_mgmt NONE", net_id);
        hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
        ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

            if (net_res)
            {
                free(net_res);
            }
            return ret;
        }

        if (!strcmp(reply, "ok"))
        {
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);

            if (net_res)
            {
                free(net_res);
            }
            return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }
    }

#if 0
    // step 4: enable_network 0
    memset(reply, 0, sizeof(reply));
    len = sizeof(reply) - 1;
    sprintf(cmd, "ENABLE_NETWORK %d", net_id);
    printf("cmd: %s\n", cmd);
    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        printf("%s, ret = %d\n", cmd, ret);
        return ret;
    }

    printf("reply = \n%s\n", reply);
    if (!strcmp(reply, "ok"))
    {
        printf("%s\n", reply);
        return -1;
    }
#endif

DO_CONN:
    if (ap_info->special_ap)
    {
        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d scan_ssid %d", net_id, scan_ssid);
        hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
        ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

            if (net_res)
            {
                free(net_res);
            }
            return ret;
        }
    }

    // step 5: select_network 0
    memset(reply, 0, sizeof(reply));
    len = sizeof(reply) - 1;
    sprintf(cmd, "SELECT_NETWORK %d", net_id);
    hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

        if (net_res)
        {
            free(net_res);
        }
        return ret;
    }
    //printf("reply = %s\n", reply);
    if (!strcmp(reply, "ok"))
    {
        hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);

        if (net_res)
        {
            free(net_res);
        }
        return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
    }

    if (net_res)
    {
        free(net_res);
    }

    return ret;
}

/**
 * This function is used to disconnect APs
 */
int wifi_ctrl_do_ap_disconnect()
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    int net_id = -1;
    char cmd[256] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    // disconnect
    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, "DISCONNECT", reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        return ret;
    }

    // get list net works
    memset(reply, 0, sizeof(reply));
    len = sizeof(reply) - 1;

    hccast_wifi_list_net_result_t *net_res = NULL;
    net_res = (hccast_wifi_list_net_result_t *)calloc(sizeof(hccast_wifi_list_net_result_t), 1);
    if (net_res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return HCCAST_WIFI_ERR_MEM;
    }

    ret = wifi_ctrl_get_list_net(WIFI_CTRL_IFACE_NAME, net_res);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);

        if (net_res)
        {
            free(net_res);
        }
        return ret;
    }

    // disable_network
    for (int i = 0; i < net_res->net_num; i++)
    {
        if (!strstr(net_res->netinfo[i].flags, "DISABLED"))
        {
            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;
            sprintf(cmd, "DISABLE_NETWORK %s", net_res->netinfo[i].net_id);
            hccast_log(LL_DEBUG, "cmd: %s\n", cmd);
            ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s run error!\n", cmd);

                if (net_res)
                {
                    free(net_res);
                }
                return ret;
            }
        }
    }

    if (net_res)
    {
        free(net_res);
    }

    return ret;
}

/**
 * This function is used to scan the available APs
 */
int wifi_ctrl_do_scan()
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    char *cmd = "SCAN";

    g_wifi_op_stat |= HCCAST_WIFI_STAT_SCANNING;
    g_wifi_op_stat &= ~HCCAST_WIFI_STAT_IDLE;

#ifdef HC_RTOS    // rtos exist wifi mode switch, dont use wifi_ctrl_run_cmd_keep
    ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
#else
    ret = wifi_ctrl_run_cmd_keep(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA_KEEP, cmd, reply, &len);
#endif

    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
    }

    //printf("reply:\n%s\n", reply);

    return ret;
}

/**
 * Used to abort the current wifi block operation.
 */
void wifi_ctrl_do_op_abort()
{
    if (HCCAST_WIFI_STAT_SCANNING == (g_wifi_op_stat & HCCAST_WIFI_STAT_SCANNING) \
            || HCCAST_WIFI_STAT_CONNECTING == (g_wifi_op_stat & HCCAST_WIFI_STAT_CONNECTING))
    {
        wifi_ctrl_p2p_lock();
        wifi_ctrl_signal();
        wifi_ctrl_p2p_unlock();
    }

    g_wifi_op_stat |= HCCAST_WIFI_STAT_IDLE;
    g_wifi_op_stat &= ~(HCCAST_WIFI_STAT_SCANNING | HCCAST_WIFI_STAT_CONNECTING);
}

/**
 * Used to abort the current wifi scanning block operation.
 */
void wifi_ctrl_do_op_scanning_abort()
{
    if (HCCAST_WIFI_STAT_SCANNING == (g_wifi_op_stat & HCCAST_WIFI_STAT_SCANNING))
    {
        wifi_ctrl_p2p_lock();
        wifi_ctrl_signal();
        wifi_ctrl_p2p_unlock();
    }

    g_wifi_op_stat |= HCCAST_WIFI_STAT_IDLE;
    g_wifi_op_stat &= ~HCCAST_WIFI_STAT_SCANNING;
}

/**
 * Used to abort the current wifi connecting block operation.
 */
void wifi_ctrl_do_op_connecting_abort()
{
    if (HCCAST_WIFI_STAT_CONNECTING == (g_wifi_op_stat & HCCAST_WIFI_STAT_CONNECTING))
    {
        wifi_ctrl_p2p_lock();
        wifi_ctrl_signal();
        wifi_ctrl_p2p_unlock();
    }

    g_wifi_op_stat |= HCCAST_WIFI_STAT_IDLE;
    g_wifi_op_stat &= ~HCCAST_WIFI_STAT_CONNECTING;
}

/**
 * Used to reset the current wifi operation stat for HCCAST_WIFI_STAT_IDLE.
 */
void wifi_ctrl_do_op_reset()
{
    g_wifi_op_stat |= HCCAST_WIFI_STAT_IDLE;
}

/**
 * Set the wifi op stat for connecting .
 *
 * @param flag true or false
 *
 * @return The value of connecting stat.
 */
int wifi_ctrl_set_connecting_by_user(bool flag)
{
    if (flag)
    {
        g_wifi_op_stat |= HCCAST_WIFI_STAT_CONNECTING;
        g_wifi_op_stat &= ~HCCAST_WIFI_STAT_IDLE;
    }
    else
    {
        g_wifi_op_stat |= HCCAST_WIFI_STAT_IDLE;
        g_wifi_op_stat &= ~HCCAST_WIFI_STAT_CONNECTING;
    }

    return g_wifi_op_stat & HCCAST_WIFI_STAT_CONNECTING;
}

/**
 * Set the wifi op stat for disconnecting .
 *
 * @param flag true or false
 *
 * @return The value of disconnecting stat.
 */
int wifi_ctrl_set_disconnecting_by_user(bool flag)
{
    if (flag)
    {
        g_wifi_op_stat |= HCCAST_WIFI_STAT_DISCONNECTING;
        g_wifi_op_stat &= ~HCCAST_WIFI_STAT_IDLE;
    }
    else
    {
        g_wifi_op_stat |= HCCAST_WIFI_STAT_IDLE;
        g_wifi_op_stat &= ~HCCAST_WIFI_STAT_DISCONNECTING;
    }

    return g_wifi_op_stat & HCCAST_WIFI_STAT_DISCONNECTING;
}

int wifi_ctrl_is_connecting()
{
    return g_wifi_op_stat & HCCAST_WIFI_STAT_CONNECTING;
}

int wifi_ctrl_is_disconnecting()
{
    return g_wifi_op_stat & HCCAST_WIFI_STAT_DISCONNECTING;
}

int wifi_ctrl_is_scanning()
{
    return g_wifi_op_stat & HCCAST_WIFI_STAT_SCANNING;
}

/**
 * Get the scan result of the AP list
 *
 * @param scan_res the pointer to the structure of the scan result
 */
int wifi_ctrl_get_aplist(hccast_wifi_scan_result_t *scan_res)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    //char val[512] = {0};
    char reply[4096] = {0};
    unsigned int len = sizeof(reply) - 1;
    char *cmd = "SCAN_RESULTS";

    g_wifi_op_stat |= HCCAST_WIFI_STAT_IDLE;
    g_wifi_op_stat &= ~HCCAST_WIFI_STAT_SCANNING;

    if (NULL == scan_res)
    {
        hccast_log(LL_ERROR, "param error!\n");
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        goto ERROR;
    }

#ifdef HC_RTOS    // rtos exist wifi mode switch, dont use wifi_ctrl_run_cmd_keep
    ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, "SCAN_RESULTS", reply, &len);
#else
    ret = wifi_ctrl_run_cmd_keep(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, "SCAN_RESULTS", reply, &len);
#endif

    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        goto ERROR;
    }

    char line[1024] = {0};
    char bssid[18]  = {0};
    char freq[64]   = {0};
    char signal[64] = {0};
    char flags[256] = {0};
    char ssid[256]  = {0};
    int i = 0, j = 0;

    for (i = 1; !get_line_from_buf(i, line, reply); i++, j++)
    {
        memset(ssid, 0, sizeof(ssid));

        // bssid / frequency / signal level / flags / ssid
        sscanf(line, "%s %s %s %s %[^'\n']", bssid, freq, signal, flags, ssid);
        if (strlen(ssid) == 0)
        {
            --j;
            continue;
        }

        wifi_ctrl_chinese_conversion(ssid);
        memcpy(scan_res->apinfo[j].ssid, ssid, sizeof(scan_res->apinfo[j].ssid));

        int sig = atoi(signal);
#ifdef __linux__
        // 2 * (dBm + 100)
        scan_res->apinfo[j].quality = sig > -50 ? 100 : 2 * (sig + 100);
#else
        if (sig >= 0)
        {
            scan_res->apinfo[j].quality = sig > 50 ? 100 : 2 * sig;
        }
        else
        {
            scan_res->apinfo[j].quality = sig > -50 ? 100 : 2 * (sig + 100);
        }
#endif
        scan_res->apinfo[j].freq = strtol(freq, NULL, 10);

        if (strstr(flags, "WPA2-PSK") || strstr(flags, "WPA-PSK"))
        {
            scan_res->apinfo[j].encryptMode = HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_AES;
        }
        else if (strstr(flags, "WPA2-SAE") || strstr(flags, "+SAE") || strstr(flags, "WPA2--CCMP"))
        {
            scan_res->apinfo[j].encryptMode = HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_SAE;
        }
        else if (strstr(flags, "WEP"))
        {
            scan_res->apinfo[j].encryptMode = HCCAST_WIFI_ENCRYPT_MODE_OPEN_WEP;
        }
        else if (strstr(flags, "ESS"))
        {
            scan_res->apinfo[j].encryptMode = HCCAST_WIFI_ENCRYPT_MODE_NONE;
        }

        //printf("%02d %-32s %d\n", i, scan_res->apinfo[j].ssid, scan_res->apinfo[j].quality);
    }

    scan_res->ap_num = j;
    hccast_log(LL_INFO, "scan ap_num = %d.\n", scan_res->ap_num);

ERROR:
    return ret;
}

/**
 * It enables or disables the hostap interface
 *
 * @param enable 1 for enable, 0 for disable
 *
 * @return 0: success, < 0 failed.
 */
int wifi_ctrl_hostap_interface_enable(bool enable)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    //char val[512] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, enable ? "ENABLE" : "DISABLE", reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s %d: %d\n", __func__, __LINE__, enable);
        return ret;
    }

#ifdef HC_RTOS // hcrtos
    if (enable)
    {
        if (HCCAST_NET_WIFI_8733BU != hccast_wifi_mgr_get_wifi_model())
        {
            p2p_ctrl_device_abort_scan();
        }

        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, "RELOAD", reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %d\n", __func__, __LINE__, enable);
            return ret;
        }
    }
#endif

#if 0
    if (enable)
    {
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, "INTERFACE wlan0", reply, &len);
        if (ret < 0 || len == 0)
        {
            ret = -1;
            printf("%s: cmd:%s\n", __func__, "INTERFACE p2p0");
            return ret;
        }

        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, "disconnect", reply, &len);
        if (ret < 0 || len == 0)
        {
            ret = -1;
            printf("%s: cmd:%s\n", __func__, "disconnect");
            return ret;
        }
    }
    else
    {
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, "INTERFACE p2p0", reply, &len);
        if (ret < 0 || len == 0)
        {
            ret = -1;
            printf("%s: cmd:%s\n", __func__, "INTERFACE p2p0");
            return ret;
        }

        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, "disconnect", reply, &len);
        if (ret < 0 || len == 0)
        {
            ret = -1;
            printf("%s: cmd:%s\n", __func__, "disconnect");
            return ret;
        }
    }
#endif

    return ret;
}

/**
 * It gets the status of the hostapd
 *
 * @param result the result of the command execution
 *
 * @return the status of the hostapd.
 */
int wifi_ctrl_hostap_get_status(hccast_wifi_hostap_status_result_t *result)
{
    int ret          = HCCAST_WIFI_ERR_NO_ERROR;
    char val[64]     = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    if (NULL == result)
    {
        hccast_log(LL_ERROR, "param error!\n");
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        return ret;
    }

    ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, "STATUS", reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
        return ret;
    }

    result_get(reply, "state", result->status, sizeof(result->status));
    result_get(reply, "ssid[0]", result->ssid, sizeof(result->ssid));

    if (result_get(reply, "channel", val, sizeof(val)) == 0)
    {
        result->channel = strtol(val, NULL, 10);
    }

    if (result_get(reply, "num_sta[0]", val, sizeof(val)) == 0)
    {
        result->num_sta = strtol(val, NULL, 10);
    }

    return ret;
}

int wifi_ctrl_hostap_set_pwd(const char *pwd)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char cmd[128] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    if (NULL == pwd)
    {
        hccast_log(LL_ERROR, "param error!\n");
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        return ret;
    }

    if (strlen(pwd) >= 8)
    {
        // SET wps_state 2
        // SET wpa 2
        // SET wpa_key_mgmt WPA-PSK
        // SET wpa_pairwise CCMP
        // SET wpa_passphrase 12345678
        // SET rsn_pairwise CCMP

        snprintf(cmd, sizeof(cmd), "SET wps_state 2");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET wpa 2");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET wpa_key_mgmt WPA-PSK");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET wpa_pairwise CCMP");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET rsn_pairwise CCMP");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET wpa_passphrase %s", pwd);
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }
    }
    else if (strlen(pwd) > 0 && strlen(pwd) < 8)
    {
        // nothing
    }
    else
    {
        snprintf(cmd, sizeof(cmd), "SET wps_state 0");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET wpa 0");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }
    }

    return ret;
}

/**
 * It sets the hostapd configuration
 *
 * @param conf the configuration of the AP
 *
 * @return The return value is the result of the command.
 */
int wifi_ctrl_hostap_set_conf(hccast_wifi_hostap_conf_t *conf)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char cmd[128] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    int channel = 0;
    int mode_5G = 0;

    if (NULL == conf)
    {
        hccast_log(LL_ERROR, "param error!\n");
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        return ret;
    }

    if (strlen(conf->ssid) > 0)
    {
        snprintf(cmd, sizeof(cmd), "SET ssid %s", conf->ssid);
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }
    }

    switch (conf->mode)
    {
    case HCCAST_WIFI_FREQ_MODE_NONE:
        break;
    case HCCAST_WIFI_FREQ_MODE_5G:
    {
        mode_5G = 1;
        snprintf(cmd, sizeof(cmd), "SET hw_mode a");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        if (conf->channel == 165)
        {
            snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20][HT20-ONLY]"); // [HT20-ONLY]: define custom field.
        }
        else
        {
            snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20][SHORT-GI-40][HT40+]");
        }
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        channel = 36;
        break;
    }
    /*
    case HCCAST_WIFI_FREQ_MODE_60G:
    {
        snprintf(cmd, sizeof(cmd), "SET hw_mode ad");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            ret = -1;
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret
        }

        channel = 3;
        ht_capab = 1;
        break;
    }
    */
    case HCCAST_WIFI_FREQ_MODE_24G:
    default:
    {
        snprintf(cmd, sizeof(cmd), "SET hw_mode g");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20]");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        channel = 6;
    }
    }

    if (strlen(conf->country_code) > 0)
    {
        snprintf(cmd, sizeof(cmd), "SET country_code %s", conf->country_code);
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }
    }

    if (conf->channel >= 1)
    {
        snprintf(cmd, sizeof(cmd), "SET channel %d", conf->channel);
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }
    }
    else
    {
        snprintf(cmd, sizeof(cmd), "SET channel %d", channel);
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }
    }

    wifi_ctrl_hostap_set_pwd(conf->pwd);

    ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, "RELOAD", reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
        return ret;
    }

#ifdef HC_RTOS
    if (mode_5G)
    {
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, "DISABLE", reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, "ENABLE", reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        hccast_log(LL_NOTICE, "%s %d reenable hostapd.\n", __func__, __LINE__);
    }
#endif

    return ret;
}

/**
 * It changes the wifi mode to the specified mode
 *
 * @param mode the frequency mode of the wifi, which can be 5G, 60G, or 24G.
 *
 * @return The return value is the return value of the function wifi_ctrl_run_cmd.
 */
int wifi_ctrl_hostap_switch_mode(hccast_wifi_freq_mode_e mode)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char cmd[128] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    int channel = 0;

    switch (mode)
    {
    case HCCAST_WIFI_FREQ_MODE_NONE:
        break;
    case HCCAST_WIFI_FREQ_MODE_5G:
    {
        snprintf(cmd, sizeof(cmd), "SET hw_mode a");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20][SHORT-GI-40][HT40+]");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        channel = 36;
        break;
    }
    /*
    case HCCAST_WIFI_FREQ_MODE_60G:
    {
        snprintf(cmd, sizeof(cmd), "SET hw_mode ad");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            ret = -1;
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret
        }

        channel = 3;
        ht_capab = 1;
        break;
    }
    */
    case HCCAST_WIFI_FREQ_MODE_24G:
    default:
    {
        snprintf(cmd, sizeof(cmd), "SET hw_mode g");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20]");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        channel = 6;
    }
    }

    snprintf(cmd, sizeof(cmd), "SET channel %d", channel);
    ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
        return ret;
    }

    ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, "RELOAD", reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
        return ret;
    }

    return ret;
}

/**
 * It changes the wifi mode to the specified mode
 *
 * @param mode the frequency mode of the wifi, which can be 5G, 60G, or 24G.
 *
 * @param channel the frequency for channel (Must correspond to mode)
 *
 * @flag hostap option (res)
 *
 * @return The return value is the return value of the function wifi_ctrl_run_cmd.
 */
int wifi_ctrl_hostap_switch_mode_ex(hccast_wifi_freq_mode_e mode, int channel, int flag)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char cmd[128] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    switch (mode)
    {
    case HCCAST_WIFI_FREQ_MODE_NONE:
        break;
    case HCCAST_WIFI_FREQ_MODE_5G:
    {
        if (channel < 34 || channel > 196)
        {
            hccast_log(LL_ERROR, "%s: param error! mode: %d, channel:%d\n", __func__, mode, channel);
        }

        snprintf(cmd, sizeof(cmd), "SET hw_mode a");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        if (channel == 165)
        {
            snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20][HT20-ONLY]"); // [HT20-ONLY]: define custom field.
        }
        else
        {
            snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20][SHORT-GI-40][HT40+]");
        }
        //snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20][SHORT-GI-40][HT40+]");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        break;
    }
    /*
    case HCCAST_WIFI_FREQ_MODE_60G:
    {
        snprintf(cmd, sizeof(cmd), "SET hw_mode ad");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            ret = -1;
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret
        }

        channel = 3;
        ht_capab = 1;
        break;
    }
    */
    case HCCAST_WIFI_FREQ_MODE_24G:
    default:
    {
        if (channel < 0 || channel > 14)
        {
            hccast_log(LL_ERROR, "%s: param error! mode: %d, channel:%d\n", __func__, mode, channel);
        }

        snprintf(cmd, sizeof(cmd), "SET hw_mode g");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20] [SHORT-GI-20]");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }
    }
    }

    snprintf(cmd, sizeof(cmd), "SET channel %d", channel);
    ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
        return ret;
    }

    ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, "RELOAD", reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
        return ret;
    }

    return ret;
}

/**
 * It changes the hostap wifi channel to the specified mode
 *
 * @param channel the channel of the wifi, which can be 5G, 60G, or 24G channel list.
 *
 * @return The return value is the return value of the function wifi_ctrl_run_cmd.
 */
int wifi_ctrl_hostap_switch_channel(int channel)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char cmd[128] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    int mode = HCCAST_WIFI_FREQ_MODE_NONE;

    if (channel > 0 && channel <= 14)
    {
        mode = HCCAST_WIFI_FREQ_MODE_24G;
    }
    else if (channel >= 34 && channel <= 196)
    {
        mode = HCCAST_WIFI_FREQ_MODE_5G;
    }

    hccast_log(LL_INFO, "channel = %d, mode: %d\n", channel, mode);

    switch (mode)
    {
    case HCCAST_WIFI_FREQ_MODE_NONE:
        break;
    case HCCAST_WIFI_FREQ_MODE_5G:
    {
        snprintf(cmd, sizeof(cmd), "SET hw_mode a");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20][SHORT-GI-40][HT40+]");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }
        break;
    }
    /*
    case HCCAST_WIFI_FREQ_MODE_60G:
    {
        snprintf(cmd, sizeof(cmd), "SET hw_mode ad");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            ret = -1;
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret
        }

        channel = 3;
        ht_capab = 1;
        break;
    }
    */
    case HCCAST_WIFI_FREQ_MODE_24G:
    default:
    {
        snprintf(cmd, sizeof(cmd), "SET hw_mode g");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }

        snprintf(cmd, sizeof(cmd), "SET ht_capab [SHORT-GI-20]");
        ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
            return ret;
        }
    }
    break;
    }

    snprintf(cmd, sizeof(cmd), "SET channel %d", channel);
    ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
        return ret;
    }

    ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, "RELOAD", reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
        return ret;
    }

    return ret;
}

/**
 * It converts a string like "\xE4\xB8\xAD\xE6\x96\x87" to "中文"
 *
 * @param srcStr The string to be converted.
 *
 * @return the address of the first character in the string.
 */
char *wifi_ctrl_chinese_conversion(char *srcStr)
{
    int src_i, dest_i;
    int srcLen = 0;

    if (NULL == srcStr)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return NULL;
    }

    srcLen = strlen(srcStr);

    for (src_i = 0, dest_i = 0; src_i < srcLen; src_i++, dest_i++)
    {
        if (srcStr[src_i] == '\\' && (srcStr[src_i + 1] == 'x' || srcStr[src_i + 1] == 'X'))
        {
            char tmp[3] = "";
            tmp[0] = srcStr[src_i + 2];
            tmp[1] = srcStr[src_i + 3];
            srcStr[dest_i] = strtoul(tmp, NULL, 16);
            src_i += 3;
        }
        else
        {
            srcStr[dest_i] = srcStr[src_i];
        }
    }
    srcStr[dest_i] = '\0';

    return srcStr;
}

/**
 * It gets the capability of the wifi device
 *
 * @param type the type of capability to get.
 * @param res the result of the command
 * @param res_len the length of the buffer pointed to by res.
 *
 * @return The reply is a string of the form:
 */
int wifi_ctrl_get_capability(char *type, char *res, size_t res_len)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char cmd[512] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    sprintf(cmd, "GET_CAPABILITY %s", type);
    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        return ret;
    }

    if (res_len >= len)
    {
        memcpy(res, reply, strlen(reply));
    }

    return ret;
}

static pthread_t g_wifi_tid = -1;
bool g_wifi_thread_running = false;

void *wifi_ctrl_disconnect_thread(void *arg)
{
    hccast_wifi_mgr_event_callback func = (hccast_wifi_mgr_event_callback) arg;
    if (func)
    {
        func(HCCAST_WIFI_DISCONNECT, NULL, NULL);
    }

    return NULL;
}

void *wifi_ctrl_thread(void *arg)
{
#ifdef __linux__
    prctl(PR_SET_NAME, __func__);
#endif

    int aErr = -1;
    int wlan_fd = -1;
    char ctrl_iface[64];
    struct wpa_ctrl *wlan_recv = NULL;

    hccast_wifi_mgr_event_callback func = (hccast_wifi_mgr_event_callback) arg;

#ifdef __linux__
    // wlan
    snprintf(ctrl_iface, sizeof(ctrl_iface), WIFI_CTRL_PATH_STA"/%s", WIFI_CTRL_IFACE_NAME);
    if (access(ctrl_iface, F_OK)) // no exist
    {
        hccast_log(LL_ERROR, "ctrl_iface: %s non-existent!\n", ctrl_iface);
        goto EXIT;
    }
#else
    snprintf(ctrl_iface, sizeof(ctrl_iface), WIFI_CTRL_PATH_STA"/%s", WIFI_CTRL_IFACE_NAME);
#endif

#ifdef HC_RTOS
    wlan_recv = wpa_ctrl_open(ctrl_iface, WPA_CTRL_WPA_IFACE_PORT);
#else
    wlan_recv = wpa_ctrl_open(ctrl_iface);
#endif
    if (!wlan_recv)
    {
        hccast_log(LL_ERROR, "wlan_recv wpa_ctrl_open failed.\n");
        goto EXIT;
    }

    aErr = wpa_ctrl_attach(wlan_recv);
    if (aErr)
    {
        hccast_log(LL_ERROR, "wlan_recv wpa_ctrl_attach failed!\n");
        goto EXIT;
    }

    wlan_fd = wpa_ctrl_get_fd(wlan_recv);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(wlan_fd, &fds);

    char aResp[1024] = {0};
    size_t aRespN;
    struct timeval tv = { 0, 0 };

    struct timespec disconnected_tp = {0};
    bool disconnected_flag = false;

    while (g_wifi_thread_running)
    {
        FD_ZERO(&fds);
        FD_SET(wlan_fd, &fds);
        //FD_SET(p2p_fd, &fds);
        tv.tv_usec = 500 * 1000;

        aErr = select(wlan_fd + 1, &fds, NULL, NULL, &tv);
        if (aErr < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            hccast_log(LL_ERROR, "select error!\n");
            goto EXIT;
        }
        else if (aErr == 0)
        {
            if (disconnected_flag)
            {
                struct timespec now_tp = {0};
                clock_gettime(CLOCK_MONOTONIC, &now_tp);

                if (hccast_wifi_mgr_get_connect_status() == 1)
                {
                    memset(&disconnected_tp, 0, sizeof(disconnected_tp));
                    disconnected_flag = false;
                    hccast_log(LL_NOTICE, "EVENT: WLAN RECONNECTED OK\n");
                }

                if (disconnected_flag && llabs(disconnected_tp.tv_sec - now_tp.tv_sec) >= 60)
                {
                    hccast_log(LL_NOTICE, "EVENT: WLAN DISCONNECT\n");
                    wifi_ctrl_do_ap_disconnect();
                    disconnected_flag = false;
#ifdef __linux__
                    if (func)
                    {
                        func(HCCAST_WIFI_DISCONNECT, NULL, NULL);
                    }
#else
                    pthread_t tid;
                    pthread_create(&tid, NULL, wifi_ctrl_disconnect_thread, func);
                    pthread_detach(tid);
#endif
                }
            }

            continue;
        }

        if (FD_ISSET(wlan_fd, &fds))
        {
            //while (wpa_ctrl_pending(wlan_recv) > 0)
            {
                aRespN = sizeof aResp - 1;
                aErr = wpa_ctrl_recv(wlan_recv, aResp, &aRespN);
                if (aErr)
                {
                    hccast_log(LL_ERROR, "wpa recv error.\n");
                    goto EXIT;
                }
                aResp[aRespN] = '\0';
                hccast_log(LL_DEBUG, " %s\n", aResp);

                if (!strncmp(aResp + 3, WPA_EVENT_CONNECTED, strlen(WPA_EVENT_CONNECTED)) && wifi_ctrl_is_connecting())
                {
                    hccast_log(LL_NOTICE, "Event: WLAN CONNECTED SUCCESS\n");
#ifdef __linux__
                    wifi_ctrl_p2p_lock();
                    wifi_ctrl_signal();
                    wifi_ctrl_p2p_unlock();
#endif
                }
                else if (!strncmp(aResp + 3, WPA_EVENT_DISCONNECTED, strlen(WPA_EVENT_DISCONNECTED)))
                {
                    if (!disconnected_flag && !hccast_wifi_mgr_p2p_get_connect_stat() && !wifi_ctrl_is_connecting() && !wifi_ctrl_is_disconnecting())
                    {
                        disconnected_flag = true;
                        clock_gettime(CLOCK_MONOTONIC, &disconnected_tp);

                        hccast_log(LL_NOTICE, "recv wifi disconnect, mira conn: %d, wifi conn: %d\n", hccast_wifi_mgr_p2p_get_connect_stat(), wifi_ctrl_is_connecting());
                    }
                    else
                    {
                        wifi_ctrl_set_disconnecting_by_user(false);
                    }
                }
            }
        }
    }

EXIT:

    if (wlan_recv)
    {
        aErr = wpa_ctrl_detach(wlan_recv);
        wpa_ctrl_close(wlan_recv);
    }

    g_wifi_thread_running = false;
    g_wifi_tid = -1;

    hccast_log(LL_NOTICE, "<%s end!>\n", __func__);

    return NULL;
}

bool wifi_ctrl_is_initted()
{
    return g_wifi_thread_running;
}

/**
 * It creates a thread that runs the wifi_ctrl_thread function
 *
 * @param params: NULL
 *
 * @return 0: success, <0: failed.
 */
int wifi_ctrl_init(hccast_wifi_mgr_event_callback func)
{
    if (g_wifi_thread_running)
    {
        hccast_log(LL_NOTICE, "wifi already init!\n");
        return HCCAST_WIFI_ERR_INITTED;
    }

    pthread_condattr_t attr;

#ifdef __linux__
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&g_wifi_p2p_cond, &attr);
#else
    pthread_cond_init(&g_wifi_p2p_cond, NULL);
#endif

#ifdef __linux__
    pthread_condattr_destroy(&attr);
#endif

    pthread_mutex_lock(&g_wifi_mutex);
    g_wifi_thread_running = true;
    if (pthread_create(&g_wifi_tid, NULL, wifi_ctrl_thread, func) != 0)
    {
        g_wifi_thread_running = false;
        hccast_log(LL_ERROR, "create wifi thread error.\n");
        pthread_mutex_unlock(&g_wifi_mutex);
        return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
    }

    //pthread_detach(g_wifi_tid);
    pthread_mutex_unlock(&g_wifi_mutex);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

int wifi_ctrl_uninit()
{
    if (g_wifi_thread_running)
    {
        pthread_mutex_lock(&g_wifi_mutex);
        g_wifi_thread_running = false;
        pthread_cond_destroy(&g_wifi_p2p_cond);
        pthread_mutex_unlock(&g_wifi_mutex);
        if (g_wifi_tid != (unsigned)-1)
        {
            pthread_join(g_wifi_tid, NULL);
            g_wifi_tid = -1;
        }
    }

    return HCCAST_WIFI_ERR_NO_ERROR;
}


#ifdef HC_RTOS
#include <getopt.h>

extern int hostapd_main(int argc, char *argv[]);
extern int wpa_supplicant_main(int argc, char *argv[]);
int eloop_is_run(void);

static pthread_t g_hostap_tid = 0;
static pthread_t g_sta_tid = 0;

static hccast_wifi_hostap_conf_t g_hostap_conf;

static void *wifi_hostap_thread(void *args)
{
    optind = 0;
    opterr = 0;
    optopt = 0;
    char port1[16] = {0};
    char port2[16] = {0};
    int *ret = (int *) args;

    sprintf(port1, "%d", WPA_CTRL_P2P_IFACE_PORT);
    sprintf(port2, "%d", WPA_NETLINK_P2P_IFACE_PORT_OUT);

    //start hostapd and p2p2 wpa.
    //wpa_supplicant -H -i p2p0  -x 9890 -X 9883  -Dwext -c /etc/wpa_supplicant.conf -C /var/run/wpa_supplicant
    char *argv[20];
    argv[0] = "wpa_supplicant";
    argv[1] = "-H";
    argv[2] = "-i";
    argv[3] = "p2p0";
    argv[4] = "-x";
    argv[5] = port1;
    argv[6] = "-X";
    argv[7] = port2;
    argv[8] = "-Dwext";
    argv[9] = "-c";
    argv[10] = "/etc/wpa_supplicant.conf";
    argv[11] = "-C";
    argv[12] = "/var/run/wpa_supplicant";
    argv[13] = NULL;

    wpa_supplicant_main(13, argv);

    if (ret != NULL)
    {
        *ret = -1;
    }

    hccast_log(LL_NOTICE, "^^^^^^^^^^^^^^%s %d exit^^^^^^^^^^^^\n", __func__, __LINE__);
    return NULL;
}

/**
 * It starts the hostap thread
 *
 * @return The return value is the thread ID of success, failed < 0.
 * @note: only use for hcrtos
 */
int wifi_ctrl_hostap_thread_start(void)
{
    pthread_attr_t attr;
    int stack_size = 64 * 1024;
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    hccast_wifi_hostap_status_result_t result;

    if (g_hostap_tid > 0)
    {
        hccast_log(LL_WARNING, "wifi_hostap_thread has been started\n");
        return HCCAST_WIFI_ERR_INITTED;
    }

    ret = pthread_attr_init(&attr);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "%s pthread_attr_init error.\n", __func__);
        return ret;
    }

    ret = pthread_attr_setstacksize(&attr, stack_size);
    if (ret != 0)
    {
        hccast_log(LL_ERROR, "%s pthread_attr_setstacksize error.\n", __func__);
        return ret;
    }

    int thread_ret = 0;
    ret = pthread_create(&g_hostap_tid, &attr, wifi_hostap_thread, &thread_ret);
    if (ret != 0)
    {
        hccast_log(LL_ERROR, "%s pthread_create error.\n", __func__);
        return ret;
    }

    while (1)
    {
        if (thread_ret != 0)
        {
            break;
        }

        if (eloop_is_run() == 1)
        {
            hccast_log(LL_NOTICE, "wifi_ctrl_hostapd_thread_start ok\n");
            break;
        }

        usleep(100 * 1000);
    }

    wifi_ctrl_do_op_reset();

    if (HCCAST_NET_WIFI_8733BU != hccast_wifi_mgr_get_wifi_model())
    {
        p2p_ctrl_device_abort_scan();
    }

    wifi_ctrl_hostap_set_conf(&g_hostap_conf);

    return ret;
}

int wifi_ctrl_hostap_terminate(void)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    ret = wifi_ctrl_run_cmd(HS_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_HOSTAP, "TERMINATE", reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
        return ret;
    }

    return ret;
}

int wifi_ctrl_hostap_thread_stop()
{
    wifi_ctrl_hostap_terminate();
    if (g_hostap_tid > 0)
    {
        pthread_join(g_hostap_tid, NULL);
    }
    g_hostap_tid = 0;

    return HCCAST_WIFI_ERR_NO_ERROR;
}

int wifi_ctrl_hostap_store_config(hccast_wifi_hostap_conf_t *conf)
{
    memcpy(&g_hostap_conf, conf, sizeof(hccast_wifi_hostap_conf_t));
    return HCCAST_WIFI_ERR_NO_ERROR;
}

int wifi_ctrl_hostap_set_best_channel(int ch24G, int ch5G)
{
    if (1 == g_hostap_conf.mode)
    {
        g_hostap_conf.channel = ch24G;
        hccast_log(LL_NOTICE, "24G best channel is %d\n", ch24G);
    }
    else if (2 == g_hostap_conf.mode)
    {
        g_hostap_conf.channel = ch5G;
        hccast_log(LL_NOTICE, "5G best channel is %d\n", ch5G);
    }

    return HCCAST_WIFI_ERR_NO_ERROR;
}

static void *wifi_sta_thread(void *args)
{
    optind = 0;
    opterr = 0;
    optopt = 0;
    char port1[16] = {0};
    char port2[16] = {0};
    char port3[16] = {0};
    char port4[16] = {0};
    int *ret = (int *) args;

    sprintf(port1, "%d", WPA_CTRL_IFACE_PORT);
    sprintf(port2, "%d", WPA_NETLINK_WPA_IFACE_PORT_OUT);
    sprintf(port3, "%d", WPA_CTRL_P2P_IFACE_PORT);
    sprintf(port4, "%d", WPA_NETLINK_P2P_IFACE_PORT_OUT);

    //start sta and p2p2 wpa.
    //wpa_supplicant -i wlan0 -Dwext -x 9877 -X 9881 -c /etc/wpa_supplicant.conf -C /var/run/wpa_supplicant -N -i p2p0  -x 9890 -X 9883  -Dwext -c /etc/wpa_supplicant.conf -C /var/run/wpa_supplicant
    char *argv[25];
    argv[0] = "wpa_supplicant";
    argv[1] = "-i";
    argv[2] = "wlan0";
    argv[3] = "-Dwext";
    argv[4] = "-x";
    argv[5] = port1;
    argv[6] = "-X";
    argv[7] = port2;
    argv[8] = "-c";
    argv[9] = "/etc/wpa_supplicant.conf";
    argv[10] = "-C";
    argv[11] = "/var/run/wpa_supplicant";
    argv[12] = "-N";
    argv[13] = "-i";
    argv[14] = "p2p0";
    argv[15] = "-x";
    argv[16] = port3;
    argv[17] = "-X";
    argv[18] = port4;
    argv[19] = "-Dwext";
    argv[20] = "-c";
    argv[21] = "/etc/wpa_supplicant.conf";
    argv[22] = "-C";
    argv[23] = "/var/run/wpa_supplicant";
    argv[24] = NULL;
    wpa_supplicant_main(24, argv);

    if (ret != NULL)
    {
        *ret = -1;
    }

    hccast_log(LL_NOTICE, "^^^^^^^^^^^^^^%s %d exit^^^^^^^^^^^^\n", __func__, __LINE__);
    return NULL;
}

int wifi_ctrl_sta_thread_start()
{
    pthread_attr_t attr;
    int stack_size = 64 * 1024;
    int ret = HCCAST_WIFI_ERR_NO_ERROR;

    if (g_sta_tid > 0)
    {
        hccast_log(LL_WARNING, "wifi_ctrl_sta_thread_start has been started\n");
        return HCCAST_WIFI_ERR_INITTED;
    }

    ret = pthread_attr_init(&attr);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "%s pthread_attr_init error.\n", __func__);
        return ret;
    }

    ret = pthread_attr_setstacksize(&attr, stack_size);
    if (ret != 0)
    {
        hccast_log(LL_ERROR, "%s pthread_attr_setstacksize error.\n", __func__);
        return ret;
    }

    int thread_ret = 0;
    ret = pthread_create(&g_sta_tid, &attr, wifi_sta_thread, &thread_ret);
    if (ret != 0)
    {
        hccast_log(LL_ERROR, "%s pthread_create error.\n", __func__);
        return ret;
    }

    while (1)
    {
        if (thread_ret != 0)
        {
            break;
        }

        if (eloop_is_run() == 1)
        {
            hccast_log(LL_NOTICE, "wifi_ctrl_sta_thread_start ok\n");
            break;
        }

        usleep(100 * 1000);
    }
    extern hccast_wifi_mgr_event_callback wifi_event_callback;
    if (wifi_event_callback)
    {
        wifi_ctrl_init(wifi_event_callback);
    }

    return HCCAST_WIFI_ERR_NO_ERROR;
}

int wifi_ctrl_sta_terminate(void)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, "TERMINATE", reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "ret = %d, reply: %s\n", ret, reply);
        return ret;
    }

    return ret;
}

int wifi_ctrl_sta_thread_stop()
{
    wifi_ctrl_uninit();
    wifi_ctrl_sta_terminate();
    if (g_sta_tid > 0)
    {
        pthread_join(g_sta_tid, NULL);
    }
    g_sta_tid = 0;

    return HCCAST_WIFI_ERR_NO_ERROR;
}

int wifi_ctrl_p2p_wps_pbc()
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    char *cmd = "WPS_PBC";

    ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
    }

    return ret;
}

#endif // HC_RTOS
