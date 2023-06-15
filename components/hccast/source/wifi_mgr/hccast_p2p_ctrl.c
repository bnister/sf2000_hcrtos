#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <wpa_ctrl.h>

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include "hccast_log.h"
#include "hccast_dhcpd.h"
#include "hccast_p2p_ctrl.h"
#include "hccast_wifi_ctrl.h"
#include "hccast_wifi_mgr.h"
#include "hccast_net.h"

static struct wpa_ctrl *g_p2p_ifrecv = NULL, *g_p2p_ifsend = NULL;
static pthread_t g_p2p_tid = 0;
static pthread_mutex_t g_p2p_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_p2p_thread_running = false;
static bool g_p2p_device_is_go = false;
static bool g_p2p_enable = false;
struct in_addr g_ip_addr;
static p2p_param_st g_mira_param;
static int64_t timestamp_listen = 0;
static bool g_p2p_connect = false;

// P2P: Starting Listen timeout(5,0) on freq=2437 based on callback  ----- timeout(pending_listen_sec, pending_listen_usec)
// P2P: Set timeout (state=LISTEN_ONLY): 5.020000 sec  ---- Add 20 msec extra wait to avoid race condition
#define P2P_LISTEN_TIMEOUT (300000000LL)
#define max_args 10

#define USE_WPAS_P2P_CONF_FILE

/* ***************************************** */

int p2p_ctrl_get_connect_stat(void)
{
    return g_p2p_connect;
}

int p2p_ctrl_set_connect_stat(bool stat)
{
    return g_p2p_connect = !!stat;
}

static int64_t get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000LL + tv.tv_usec;
}

static int tokenize_cmd(char *cmd, char *argv[])
{
    char *pos = NULL;
    int argc = 0;

    pos = cmd;
    for (;;)
    {
        while (*pos == ' ')
            pos++;
        if (*pos == '\0')
            break;
        argv[argc] = pos;
        argc++;
        if (argc == max_args)
            break;
        if (*pos == '"')
        {
            char *pos2 = strrchr(pos, '"');
            if (pos2)
                pos = pos2 + 1;
        }
        while (*pos != '\0' && *pos != ' ')
            pos++;
        if (*pos == ' ')
            *pos++ = '\0';
    }

    return argc;
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
    char *token = NULL;
    char *saveptr = NULL;

    char *strs = strdup(str);
    token = strtok_r(strs, " ", &saveptr);

    while (token != NULL)
    {
        sscanf(token, "%[^=]=%[^'\n']", keys, vals);

        if (!strcmp(key, keys))
        {
            //printf("keys: %s, vals: %s.\n", keys, vals);
            strncpy(val, vals, val_len);
            break;
        }

        token = strtok_r(NULL, " ", &saveptr);
    }

    free(strs);

    return 0;
}

void sendRequest(struct wpa_ctrl *iIf, char *iSend)
{
    pthread_mutex_lock(&g_p2p_mutex);
    hccast_log(LL_SPEW, ">>> %s\n", iSend);
    char aResp[64] = {0};
    size_t aRespN = sizeof aResp - 1;
    int aErr = wpa_ctrl_request(iIf, iSend, strlen(iSend), aResp, &aRespN, NULL);
    if (aErr)
    {
        hccast_log(LL_ERROR, "wpa_ctrl_request failed, ret (%d)!\n", aErr);
    }

    aResp[aRespN] = '\0';
    hccast_log(LL_SPEW, "    %s\n", aResp);
    pthread_mutex_unlock(&g_p2p_mutex);
}

int p2p_ctrl_find_group(const char *peer_dev)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char aSend[256] = {0};
    //This array size if 16K,please use malloc otherwise will task overflow.
    hccast_wifi_list_net_result_t *res = NULL;
    res = (hccast_wifi_list_net_result_t *)calloc(sizeof(hccast_wifi_list_net_result_t), 1);
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return HCCAST_WIFI_ERR_MEM;
    }

    ret = wifi_ctrl_get_list_net(P2P_CTRL_IFACE_NAME, res);
    for (int i = 0; i < res->net_num; i++)
    {
        if (strstr(res->netinfo[i].bssid, peer_dev))
        {
            free(res);
            return strtol(res->netinfo[i].net_id, NULL, 10);
        }
    }

    free(res);
    return ret;
}

/**
 * If the p2p interface non-exist a persistent group, then create a persistent group.
 */
void p2p_ctrl_create_persistent_group()
{
    hccast_wifi_status_result_t status_result = {0};
    wifi_ctrl_get_status(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, &status_result);

    hccast_wifi_list_net_result_t *res = NULL;
    res = (hccast_wifi_list_net_result_t *)calloc(sizeof(hccast_wifi_list_net_result_t), 1);
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return ;
    }
    wifi_ctrl_get_list_net(P2P_CTRL_IFACE_NAME, res);

    bool persistent_found = false;
    for (int i = 0; i < res->net_num; i++)
    {
        if (!strcasecmp(res->netinfo[i].bssid, status_result.p2p_device_address))
        {
            persistent_found = true;
            break;
        }
    }

    if (!persistent_found)
    {
        char aSend[256] = {0};
        sprintf(aSend, "SET p2p_ssid_postfix %s%c%c%c%c", "_hccast_", \
                status_result.p2p_device_address[12], status_result.p2p_device_address[13], \
                status_result.p2p_device_address[15], status_result.p2p_device_address[16]);
        sendRequest(g_p2p_ifsend, aSend);
        sendRequest(g_p2p_ifsend, "P2P_GROUP_ADD persistent");
    }

    free(res);
}

void p2p_ctrl_remove_nonpersistent_group()
{
    char aSend[256] = {0};

    hccast_wifi_list_net_result_t *res = NULL;
    res = (hccast_wifi_list_net_result_t *)calloc(sizeof(hccast_wifi_list_net_result_t), 1);
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return ;
    }

    wifi_ctrl_get_list_net(P2P_CTRL_IFACE_NAME, res);
    for (int i = 0; i < res->net_num; i++)
    {
        if (!strstr(res->netinfo[i].flags, "[PERSISTENT]"))
        {
            sprintf(aSend, "REMOVE_NETWORK %s", res->netinfo[i].net_id);
            sendRequest(g_p2p_ifsend, aSend);
        }
    }

    free(res);
}

void p2p_ctrl_remove_curr_group()
{
    char aSend[256] = {0};

    hccast_wifi_list_net_result_t *res = NULL;
    res = (hccast_wifi_list_net_result_t *)calloc(sizeof(hccast_wifi_list_net_result_t), 1);
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return ;
    }

    wifi_ctrl_get_list_net(P2P_CTRL_IFACE_NAME, res);
    for (int i = 0; i < res->net_num; i++)
    {
        if (strstr(res->netinfo[i].flags, "[CURRENT]"))
        {
            sprintf(aSend, "REMOVE_NETWORK %s", res->netinfo[i].net_id);
            sendRequest(g_p2p_ifsend, aSend);
            break;
        }
    }

    free(res);
}


/*
> p2p_peer 9a:ac:cc:96:2d:6b
9a:ac:cc:96:2d:6b
pri_dev_type=10-0050F204-5
device_name=Android_3db5
manufacturer=
model_name=
model_number=
serial_number=
config_methods=0x80
dev_capab=0x25
group_capab=0x0
level=0
age=1
listen_freq=2437
wps_method=not-ready
interface_addr=00:00:00:00:00:00
member_in_go_dev=00:00:00:00:00:00
member_in_go_iface=00:00:00:00:00:00
go_neg_req_sent=0
go_state=unknown
dialog_token=0
intended_addr=00:00:00:00:00:00
country=__
oper_freq=0
req_config_methods=0x0
flags=[REPORTED][PEER_WAITING_RESPONSE]
status=1
invitation_reqs=0
wfd_subelems=00000600101c440032
*/
int p2p_ctrl_peer(const char *peer_dev, p2p_peer_result_st *result)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char cmd[64] = {0};
    char val[512] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    if (NULL == result || NULL == peer_dev)
    {
        hccast_log(LL_ERROR, "param error!\n");
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        goto ERROR;
    }

    sprintf(cmd, "P2P_PEER %s", peer_dev);
    memset(result, 0x00, sizeof(p2p_peer_result_st));
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

    result_get(reply, "device_name", result->device_name, sizeof(result->device_name));
    result_get(reply, "wfd_subelems", result->wfd_subelems, sizeof(result->wfd_subelems));

    if (result_get(reply, "config_methods", val, sizeof(val)) == 0)
    {
        result->config_methods = strtol(val, NULL, 16);
    }
    if (result_get(reply, "dev_capab", val, sizeof(val)) == 0)
    {
        result->dev_capab = strtol(val, NULL, 16);
    }
    if (result_get(reply, "group_capab", val, sizeof(val)) == 0)
    {
        result->group_capab = strtol(val, NULL, 16);
    }
    if (result_get(reply, "listen_freq", val, sizeof(val)) == 0)
    {
        result->listen_freq = strtol(val, NULL, 16);
    }

ERROR:
    return ret;
}

void p2p_ctrl_device_init()
{
    hccast_log(LL_NOTICE, "%s p2p device init\n", __func__);

    sendRequest(g_p2p_ifsend, "DISCONNECT");
    sendRequest(g_p2p_ifsend, "P2P_FLUSH");
    //sendRequest(g_p2p_ifsend, "P2P_FIND 1");
    //usleep(1000);

    timestamp_listen = get_time_us();
    sendRequest(g_p2p_ifsend, "P2P_LISTEN");
    g_mira_param.state_update_func(HCCAST_P2P_STATE_LISTEN);
}

/**
 * It opens a connection to the wpa_supplicant control interface, and returns the file descriptor for
 * the connection
 *
 * @return The file descriptor of the control interface for recv.
 */
int p2p_ctrl_wpas_init()
{
    hccast_log(LL_NOTICE, "Enter %s!\n", __func__);

    char ctrl_iface[64];
    int err = HCCAST_WIFI_ERR_NO_ERROR;

    if (g_p2p_ifsend || g_p2p_ifrecv)
    {
        return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
    }

    snprintf(ctrl_iface, sizeof(ctrl_iface), WIFI_CTRL_PATH_STA "/%s", (char *)g_mira_param.p2p_ifname);
    if (access(ctrl_iface, F_OK)) // no exist
    {
        hccast_log(LL_ERROR, "ctrl_iface: %s non-existent!\n", ctrl_iface);
        return HCCAST_WIFI_ERR_CMD_WPAS_NO_RUN;
    }

    hccast_log(LL_INFO, "ctrl_iface: %s\n", ctrl_iface);

    for (int i = 0; i < 15; ++i)
    {
        g_p2p_ifsend = wpa_ctrl_open(ctrl_iface);
        if (g_p2p_ifsend)
        {
            break;
        }

        usleep(100);
    }

    if (!g_p2p_ifsend)
    {
        hccast_log(LL_ERROR, "g_p2p_ifsend open error!\n");
        return HCCAST_WIFI_ERR_CMD_WPAS_OPEN_FAILED;
    }

    g_p2p_ifrecv = wpa_ctrl_open(ctrl_iface);
    if (!g_p2p_ifrecv)
    {
        hccast_log(LL_ERROR, "g_p2p_ifrecv open error!\n");
        wpa_ctrl_close(g_p2p_ifsend);
        g_p2p_ifsend = NULL;
        return HCCAST_WIFI_ERR_CMD_WPAS_OPEN_FAILED;
    }

    err = wpa_ctrl_attach(g_p2p_ifrecv);
    if (err)
    {
        hccast_log(LL_ERROR, "g_p2p_ifrecv attach error!\n");
        wpa_ctrl_close(g_p2p_ifrecv);
        wpa_ctrl_close(g_p2p_ifsend);
        g_p2p_ifrecv = NULL;
        g_p2p_ifsend = NULL;
        return HCCAST_WIFI_ERR_CMD_WPAS_ATTACH_FAILED;
    }

    return wpa_ctrl_get_fd(g_p2p_ifrecv);
}

/**
 * It initializes the P2P wpas attr interface
 */
void p2p_ctrl_wpas_attr_init()
{
    char aSend[256] = {0};

#ifdef USE_WPAS_P2P_CONF_FILE
    sendRequest(g_p2p_ifsend, "RECONFIGURE");
#endif

    sprintf(aSend, "SET device_name %s", g_mira_param.device_name);
    sendRequest(g_p2p_ifsend, aSend);

    //sendRequest(g_p2p_ifsend, "WFD_SUBELEM_SET 0 000600111c440006");
    //sendRequest(g_p2p_ifsend, "WFD_SUBELEM_SET 7 00020001");

#ifdef USE_WPAS_P2P_CONF_FILE
    sendRequest(g_p2p_ifsend, "SET update_config 1");
#endif

    //sendRequest(g_p2p_ifsend, "SET wifi_display 1");                     // add+
    sendRequest(g_p2p_ifsend, "SET config_methods virtual_push_button physical_display keypad");
    sendRequest(g_p2p_ifsend, "SET device_type 10-0050F204-5");
    sendRequest(g_p2p_ifsend, "SET manufacturer hichip");
    sendRequest(g_p2p_ifsend, "SET persistent_reconnect 1");
    sendRequest(g_p2p_ifsend, "SET p2p_listen_reg_class 81");
    sendRequest(g_p2p_ifsend, "SET p2p_oper_reg_class 115");
    sendRequest(g_p2p_ifsend, "SET p2p_go_intent 7");
    //sendRequest(g_p2p_ifsend, "SET p2p_go_vht 1");
    //sendRequest(g_p2p_ifsend, "SET p2p_go_ht40 1");

    hccast_wifi_status_result_t status_result = {0};
    wifi_ctrl_get_status(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, &status_result);
    sprintf(aSend, "SET p2p_ssid_postfix %s%c%c%c%c", "_hccast_", \
            status_result.p2p_device_address[12], status_result.p2p_device_address[13], \
            status_result.p2p_device_address[15], status_result.p2p_device_address[16]);

    sendRequest(g_p2p_ifsend, aSend);

    sprintf(aSend, "SET p2p_listen_channel %d", g_mira_param.listen_channel);
    sendRequest(g_p2p_ifsend, aSend);

    sprintf(aSend, "SET p2p_oper_channel %d", g_mira_param.oper_channel);
    sendRequest(g_p2p_ifsend, aSend);
}

void p2p_ctrl_device_enable()
{
    sendRequest(g_p2p_ifsend, "DRIVER wfd-enable");
    usleep(200 * 1000);

    sendRequest(g_p2p_ifsend, "P2P_LISTEN");
    g_mira_param.state_update_func(HCCAST_P2P_STATE_LISTEN);
}

void p2p_ctrl_device_disable()
{
    sendRequest(g_p2p_ifsend, "DRIVER wfd-disable");
    usleep(200 * 1000);
    sendRequest(g_p2p_ifsend, "P2P_FLUSH");
    g_mira_param.state_update_func(HCCAST_P2P_STATE_NONE);
}

/**
 * get p2p device enable status
 *
 * @return g_p2p_enable
 */
bool p2p_ctrl_get_enable()
{
    return g_p2p_enable;
}

/**
 * It sets the global variable g_p2p_enable to the value of the parameter enable
 *
 * @param enable true to enable, false to disable
 *
 * @return The value of g_p2p_enable
 */
bool p2p_ctrl_set_enable(bool enable)
{
    g_p2p_enable = enable;
    if (enable)
    {
        hccast_log(LL_NOTICE, "%s: p2p device enable.\n", __FUNCTION__);
        p2p_ctrl_device_enable();
    }
    else
    {
        hccast_log(LL_NOTICE, "%s: p2p device disenable.\n", __FUNCTION__);
        p2p_ctrl_device_disable();
    }

    return g_p2p_enable;
}

int p2p_ctrl_device_is_go()
{
    return g_p2p_device_is_go;
}

unsigned int p2p_ctrl_get_device_ip()
{
    if (g_ip_addr.s_addr)
    {
        return g_ip_addr.s_addr;
    }

    return 0;
}

int p2p_ctrl_get_device_rtsp_port()
{
    return MIRA_RTSP_PORT;
}

static void udhcpd_cb(unsigned int yiaddr)
{
    g_ip_addr.s_addr = yiaddr;
    hccast_log(LL_NOTICE, "offer peer ip: %s\n", inet_ntoa(g_ip_addr));

    g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
    usleep(150 * 1000);
    hccast_net_ifconfig(P2P_CTRL_IFACE_NAME, HCCAST_P2P_IP, HCCAST_P2P_MASK, NULL);
    g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTED);
}

static void udhcpc_cb(unsigned int data)
{
    hccast_udhcp_result_t *in = (hccast_udhcp_result_t *)data;
    if (in && in->stat)
    {
        inet_aton(in->gw, &g_ip_addr);
        hccast_log(LL_NOTICE, "got peer ip: %s\n", inet_ntoa(g_ip_addr));
        hccast_net_ifconfig(in->ifname, in->ip, in->mask, NULL);
        g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTED);
    }
}

static udhcp_conf_t g_p2p_udhcpc_conf =
{
    .func   = udhcpc_cb,
    .ifname = UDHCP_IF_P2P0,
    .pid    = 0,
    .run    = 0,
    .option = UDHCPC_QUIT_AFTER_LEASE | UDHCPC_ABORT_IF_NO_LEASE,
};

int p2p_ctrl_udhcpc_start()
{
    udhcpc_start(&g_p2p_udhcpc_conf);
    return 0;
}

int p2p_ctrl_udhcpc_stop()
{
    udhcpc_stop(&g_p2p_udhcpc_conf);
    return 0;
}

/**
 * It's a thread that listens for events from the wpa_supplicant daemon
 *
 * @param arg the socket fd for attach wpas ctrl.
 *
 * @return The return value is a pointer to the new thread.
 */

void *p2p_ctrl_thread(void *arg)
{
    prctl(PR_SET_NAME, __func__);

    int fd = (int) arg;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    char aSend[256] = {0};
    char aResp[1024] = {0};
    size_t aRespN = 0;
    struct timeval tv = { 0, 0 };
    char peer_dev_addr[18] = {0};
    bool wps_flag = false;
    int err = 0;

    udhcp_conf_t g_p2p_udhcpd_conf =
    {
        .func   = udhcpd_cb,
        .ifname = UDHCP_IF_P2P0,
        .pid    = 0,
        .run    = 0
    };

    udhcpd_start(&g_p2p_udhcpd_conf);

AGAIN:
    g_p2p_device_is_go  = false;
    wps_flag = false;
    memset(&g_ip_addr, 0, sizeof(g_ip_addr));
    //p2p_ctrl_device_init();

    while (g_p2p_thread_running)
    {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_usec = 100 * 1000;

        err = select(fd + 1, &fds, NULL, NULL, &tv);
        if (err < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            hccast_log(LL_ERROR, "select error!\n");
            goto EXIT;
        }
        else if (err == 0)
        {
            int64_t time_now = get_time_us();
            if (p2p_ctrl_get_enable() && !p2p_ctrl_get_connect_stat() && llabs(time_now - timestamp_listen) >= P2P_LISTEN_TIMEOUT)
            {
                timestamp_listen = time_now;
                hccast_log(LL_INFO, "listen timeout(%lld s), re listen now...\n", P2P_LISTEN_TIMEOUT / 1000000);
                //sendRequest(g_p2p_ifsend, "P2P_LISTEN");
                p2p_ctrl_device_init();
            }

            continue;
        }
        if (FD_ISSET(fd, &fds))
        {
            aRespN = sizeof aResp - 1;
            err = wpa_ctrl_recv(g_p2p_ifrecv, aResp, &aRespN);
            if (err)
            {
                hccast_log(LL_ERROR, "wpa recv error!\n");
                goto EXIT;
            }

            aResp[aRespN] = '\0';
            hccast_log(LL_DEBUG, " %s\n", aResp);

            if (!strncmp(aResp + 3, P2P_EVENT_GO_NEG_REQUEST, strlen(P2P_EVENT_GO_NEG_REQUEST)))
            {
                // <3>P2P-GO-NEG-REQUEST 9a:ac:cc:96:2d:6b dev_passwd_id=4 go_intent=15
                hccast_log(LL_NOTICE, "EVENT: P2P_EVENT_GO_NEG_REQUEST!\n");

                char *argv[10];
                int argc = 0;
                char *pos = NULL;

                char *copy = strdup(aResp + 3);
                argc = tokenize_cmd(copy, argv);
                free(copy);

                if (argc < 1)
                {
                    hccast_log(LL_ERROR, "tokenize_cmd!\n");
                    continue;
                }

                char *dev_addr = argv[1];

                int  go_intent = 0;
                if (argc > 3)
                {
                    pos = strstr(argv[3], "go_intent=");
                    go_intent = atoi(pos + 10);
                }

                memset(aSend, 0, sizeof(aSend));
                strcat(aSend, "P2P_CONNECT ");
                strcat(aSend, dev_addr);
                strcat(aSend, " pbc persistent");

                if (0 == go_intent)
                {
                    strcat(aSend, " go_intent=15");
                }
                else
                {
                    p2p_peer_result_st p2p_peer = {0};
                    p2p_ctrl_peer(dev_addr, &p2p_peer);

                    if ((p2p_peer.group_capab & 1) != 0)
                    {
                        strcat(aSend, " join");
                    }
                    else
                    {
                        strcat(aSend, " go_intent=7");
                    }
                }

                hccast_log(LL_INFO, "p2p aSend: %s\n", aSend);
                //sendRequest(g_p2p_ifsend, aSend);

                char reply[1024] = {0};
                unsigned int len = sizeof(reply) - 1;

                for (int i = 0; i < 3; i++)
                {
                    int ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, aSend, reply, &len);
                    if (ret < 0 || len == 0 || strncmp(reply, "FAIL", 4) == 0)
                    {
                        ret = -2;
                        hccast_log(LL_ERROR, "%s run error!\n", aSend);
                        usleep(500 * 1000);
                    }
                    else
                    {
                        break;
                    }
                }
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_PROV_DISC_PBC_REQ, strlen(P2P_EVENT_PROV_DISC_PBC_REQ)))
            {
                hccast_log(LL_DEBUG, "Event: DISC PBC REQ!\n");
                hccast_log(LL_DEBUG, "wps_flag %d\n", wps_flag);

                if (!wps_flag)
                {
                    sendRequest(g_p2p_ifsend, "WPS_PBC");
                    wps_flag = true;
                }

                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_GO_NEG_SUCCESS, strlen(P2P_EVENT_GO_NEG_SUCCESS)))
            {
                // P2P-GO-NEG-SUCCESS role=client freq=2437 ht40=1 peer_dev=9a:ac:cc:96:2d:6b peer_iface=9a:ac:cc:96:2d:6b wps_method=PBC
                hccast_log(LL_INFO, "EVENT: P2P_EVENT_GO_NEG_SUCCESS!\n");

                char val[32] = {0};
                if (result_get(aResp, "role", val, sizeof(val)) == 0)
                {
                    if (!strcasecmp(val, "client"))
                    {
                        g_p2p_device_is_go = false;
                    }
                    else if (!strcasecmp(val, "go"))
                    {
                        g_p2p_device_is_go = true;
                    }
                    else
                    {
                        g_p2p_device_is_go = false;
                    }
                }
#if 0
                if (log_level & LOG_LEVEL_DEBUG)
                {
                    int  freq = 0;
                    if (result_get(aResp, "freq", val, sizeof(val)) == 0)
                    {
                        freq = strtol(val, NULL, 10);
                    }
                    memset(peer_dev_addr, 0, sizeof(peer_dev_addr));
                    result_get(aResp, "peer_dev", peer_dev_addr, sizeof(peer_dev_addr));

                    hccast_log(LL_DEBUG, "Go Neo Result (role: '%s', freq: %d, peer_dev: '%s')\n", g_p2p_device_is_go ? "GO" : "client", freq, peer_dev_addr);
                }
#endif
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_INVITATION_ACCEPTED, strlen(P2P_EVENT_INVITATION_ACCEPTED)))
            {
                // <3>P2P-INVITATION-ACCEPTED sa=9a:ac:cc:96:2d:6b persistent=1
                hccast_log(LL_NOTICE, "Event: INVITATION ACCEPTED!\n");
#if 0
                if (log_level & LOG_LEVEL_DEBUG)
                {
                    char val[32] = {0};
                    int  persistent = 0;

                    char *ptr = NULL;
                    ptr = strstr(aResp, "sa=");
                    if (ptr)
                    {
                        result_get(aResp, "sa", peer_dev_addr, sizeof(peer_dev_addr));

                        if (result_get(aResp, "persistent", val, sizeof(val)) == 0)
                        {
                            persistent = strtol(val, NULL, 10);
                        }
                    }

                    hccast_log(LL_DEBUG, "Result(sa: '%s', persistent: %d)\n", peer_dev_addr, persistent);
                }
#endif
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
            }
            else if (strstr(aResp + 3, "Trying to associate"))
            {
                hccast_log(LL_NOTICE, "Event: TRYING TO ASSOCIATE!\n");
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_GROUP_STARTED, strlen(P2P_EVENT_GROUP_STARTED)))
            {
                //<3>P2P-GROUP-STARTED p2p0 client ssid="DIRECT-XXX" freq=2462
                // psk=0d8a759a26a6cbe8c6ae7735ba39b4f3ff35b6c6002f3549e94003c34f88ffbe go_dev_addr=02:2e:2d:9d:78:58
                // [PERSISTENT] ip_addr=192.168.137.247 ip_mask=255.255.255.0 go_ip_addr=192.168.137.1
                hccast_log(LL_NOTICE, "Event: GROUP STARTED!\n");

                char *argv[10];
                int argc = 0;

                char *copy = strdup(aResp + 3);
                argc = tokenize_cmd(copy, argv);

                char *go_or_client = argv[2];
                if (strncasecmp(go_or_client, "client", strlen("client")) == 0)
                {
                    char ip_addr[32] = {0}, ip_mask[32] = {0}, go_ip_addr[32] = {0}, *ptr = NULL;
                    ptr = strstr(aResp, "ip_addr=");
                    if (ptr)
                    {
                        result_get(ptr, "ip_addr", ip_addr, sizeof(ip_addr));
                        result_get(ptr, "ip_mask", ip_mask, sizeof(ip_mask));
                        result_get(ptr, "go_ip_addr", go_ip_addr, sizeof(go_ip_addr));

                        hccast_log(LL_DEBUG, "result (ip_addr: '%s', go_ip_addr: '%s')\n", ip_addr, go_ip_addr);

                        inet_aton(go_ip_addr, &g_ip_addr);
                        hccast_net_ifconfig(P2P_CTRL_IFACE_NAME, ip_addr, ip_mask, go_ip_addr);
                        g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTED);
                    }
                    else
                    {
                        p2p_ctrl_udhcpc_stop();
                        p2p_ctrl_udhcpc_start();
                    }
                }
                else if(strncasecmp(go_or_client, "GO", strlen("GO")) == 0)
                {
                }

                hccast_log(LL_NOTICE, "%s %d: go_or_client: %s\n", __func__, __LINE__, go_or_client);

                free(copy);
            }
            else if (!strncmp(aResp + 3, WPA_EVENT_CONNECTED, strlen(WPA_EVENT_CONNECTED)))
            {
                // as Client
#ifdef USE_WPAS_P2P_CONF_FILE
                sendRequest(g_p2p_ifsend, "SAVE_CONFIG");
#endif
                hccast_log(LL_INFO, "Event: CONNECTED!\n");
            }
            else if (!strncmp(aResp + 3, WPA_EVENT_DISCONNECTED, strlen(WPA_EVENT_DISCONNECTED)))
            {
                hccast_log(LL_NOTICE, "EVENT: DISCONNECT!\n");
                g_mira_param.state_update_func(HCCAST_P2P_STATE_DISCONNECTED);
                wps_flag = false;
                //goto again;
            }
            else if (!strncmp(aResp + 3, AP_STA_CONNECTED, strlen(AP_STA_CONNECTED)) && g_p2p_device_is_go)
            {
                // as Go
#ifdef USE_WPAS_P2P_CONF_FILE
                sendRequest(g_p2p_ifsend, "SAVE_CONFIG");
#endif
                hccast_log(LL_INFO, "EVENT: AP-STA-CONNECTED!\n");
            }
            else if (!strncmp(aResp + 3, AP_STA_DISCONNECTED, strlen(AP_STA_DISCONNECTED)) && g_p2p_device_is_go)
            {
                hccast_log(LL_NOTICE, "EVENT: AP-STA-DISCONNECTED!\n");
                g_mira_param.state_update_func(HCCAST_P2P_STATE_DISCONNECTED);
                wps_flag = false;
                //goto again;
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_GROUP_FORMATION_FAILURE, strlen(P2P_EVENT_GROUP_FORMATION_FAILURE)))
            {
                hccast_log(LL_NOTICE, "EVENT: GROUP FORMATION FAILURE!\n");
                sendRequest(g_p2p_ifsend, "P2P_FLUSH");
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECT_FAILED);
                goto AGAIN;
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_DEVICE_LOST, strlen(P2P_EVENT_DEVICE_LOST)))
            {
                hccast_log(LL_INFO, "EVENT: DEVICE LOST!\n");
            }
            else if (!strncmp(aResp + 3, WPA_EVENT_TERMINATING, strlen(WPA_EVENT_TERMINATING)))
            {
                g_p2p_thread_running = false;
                break;
            }
            else if (!strncmp(aResp + 3, WPA_EVENT_SCAN_RESULTS, strlen(WPA_EVENT_SCAN_RESULTS)) \
                     && wifi_ctrl_is_scanning() && !p2p_ctrl_get_connect_stat())
            {
                hccast_log(LL_INFO, "EVENT: SCAN RESULTS!\n");
                wifi_ctrl_p2p_lock();
                wifi_ctrl_signal();
                wifi_ctrl_p2p_unlock();
            }
        }
    }

EXIT:
    sendRequest(g_p2p_ifsend, "REMOVE_NETWORK all");

    udhcpd_stop(&g_p2p_udhcpd_conf);
    //p2p_ctrl_device_disable();

    err = wpa_ctrl_detach(g_p2p_ifrecv);
    wpa_ctrl_close(g_p2p_ifrecv);
    wpa_ctrl_close(g_p2p_ifsend);

    g_p2p_ifrecv = NULL;
    g_p2p_ifsend = NULL;

    g_p2p_thread_running = false;
    g_p2p_enable = false;
    g_p2p_tid = 0;

    hccast_log(LL_NOTICE, "%s end!\n", __func__);

    return NULL;
}

/**
 * It creates a thread that runs the p2p_ctrl_thread function
 *
 * @param params: p2p device params
 *
 * @return 0: success, <0: failed.
 */
int p2p_ctrl_init(p2p_param_st *params)
{
    if (NULL == params)
    {
        hccast_log(LL_ERROR, "miracast param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    memcpy(&g_mira_param, params, sizeof(p2p_param_st));

    int fd = p2p_ctrl_wpas_init(); // need g_mira_param value.
    if (fd <= 0)
    {
        hccast_log(LL_NOTICE, "P2P init failed!\n");
        return fd;
    }

    p2p_ctrl_wpas_attr_init();

    pthread_mutex_lock(&g_p2p_mutex);
    if (g_p2p_thread_running)
    {
        hccast_log(LL_NOTICE, "P2P already init!\n");
        pthread_mutex_unlock(&g_p2p_mutex);
        return 0;
    }

    g_p2p_thread_running = true;
    if (pthread_create(&g_p2p_tid, NULL, p2p_ctrl_thread, (int *)fd) != 0)
    {
        hccast_log(LL_ERROR, "create p2p thread error!\n");
        g_p2p_thread_running = false;
        pthread_mutex_unlock(&g_p2p_mutex);
        return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
    }

    //pthread_detach(g_p2p_tid);
    pthread_mutex_unlock(&g_p2p_mutex);
    return 0;
}

/**
 *  This function is used to stop the p2p thread
 */
int p2p_ctrl_uninit()
{
    if (g_p2p_thread_running)
    {
        pthread_mutex_lock(&g_p2p_mutex);
        g_p2p_thread_running = false;
        pthread_mutex_unlock(&g_p2p_mutex);
        if (g_p2p_tid != 0)
        {
            pthread_join(g_p2p_tid, NULL);
            g_p2p_tid = 0;
        }
    }

    return 0;
}