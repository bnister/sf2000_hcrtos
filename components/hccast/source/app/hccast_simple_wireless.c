#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <netdb.h>

#include "hccast_mira.h"
#include "hccast_air.h"
#include "hccast_dlna.h"
#include "hccast_dhcpd.h"
#include "hccast_av.h"
#include "hccast_wifi_mgr.h"
#include "hccast_httpd.h"
#include "castapp_log.h"
#include <hccast_media.h>
#include <hccast_scene.h>
#include <hccast_net.h>

#define UUID_HEADER "HCcast"

#define MX_EVNTS (10)
#define EPL_TOUT (1000)

enum EPOLL_EVENT_TYPE
{
    EPOLL_EVENT_TYPE_KUMSG = 0,
    EPOLL_EVENT_TYPE_HOTPLUG_CONNECT,
    EPOLL_EVENT_TYPE_HOTPLUG_MSG,
};

struct epoll_event_data
{
    int fd;
    enum EPOLL_EVENT_TYPE type;
};

typedef struct _hccast_device_setting_st_
{
    unsigned char dev_name[SERVICE_NAME_MAX_LEN];
    unsigned char dev_psk[DEVICE_PSK_MAX_LEN];
    unsigned char upg_server[HTTP_UPG_SERVER_MAX_LEN];

    hccast_wifi_ap_info_t ap_info[WIFI_AP_MAX_NUM];
	
    unsigned char mac[7];
    unsigned char resolution;

    int lang;//1-englist;2-chn;3-traditional chn
    int ratio;
	
    int mirror_mode;//1-standard.
    int mirror_frame;//0-30FPS,1-60FPS
    int aircast_mode;//0-mirror-stream, 1-mirror-only

    int bright;
    int contrast;
    int saturation;
    int hue;
    int sharpness;
} hccast_device_setting_st;

static int castapp_dlna_started = 0;
static int castapp_aircast_started = 0;
static int castapp_miracast_started = 0;
static int castapp_probed_wifi = 0;
static struct epoll_event_data hotplg_data = { 0 };
static struct epoll_event_data hotplg_msg_data = { 0 };
static int epoll_fd = -1;
hccast_device_setting_st g_http_setting;
static int hostap_connect_count = 0;



void castapp_stop_all_services();

unsigned char uuid[64] = {0};

static int castapp_system_cmd(const char *cmd)
{
    pid_t pid;
    if (-1 == (pid = vfork()))
    {
        return 1;
    }
    if (0 == pid)
    {
        execl("/bin/sh", "sh", "-c", cmd, (char *)0);
        return  0;
    }
    else
    {
        wait(&pid);
    }
    return 0;
}

static void castapp_process_hotplug_msg(unsigned char *msg)
{
    //plug-out: ACTION=wifi-remove INFO=v0BDApF179
    //plug-in: ACTION=wifi-add INFO=v0BDApF179

    if (strstr(msg, "ACTION=wifi-remove"))
    {
        printf("Wi-Fi plug-out\n");
        castapp_probed_wifi = 0;
        return ;
    }

    if (strstr(msg, "ACTION=wifi-add"))
    {
        if (strstr(msg, "INFO=v0BDApF179"))
        {
            printf("Wi-Fi probed RTL8188FTV\n");
            castapp_probed_wifi = HCCAST_NET_WIFI_8188FTV;
            hccast_wifi_mgr_set_wifi_model(HCCAST_NET_WIFI_8188FTV);
            return ;
        }
        else if (strstr(msg, "INFO=v0BDApC811"))
        {
            printf("Wi-Fi probed RTL8811FTV\n");
            castapp_probed_wifi = HCCAST_NET_WIFI_8811FTV;
            hccast_wifi_mgr_set_wifi_model(HCCAST_NET_WIFI_8811FTV);
            return ;
        }
    }
}

static void *castapp_receive_event_func(void *arg)
{
    struct epoll_event events[MX_EVNTS];
    int n = -1;
    int i;
    struct sockaddr_in client;
    socklen_t len = sizeof(client);

    while (1)
    {
        n = epoll_wait(epoll_fd, events, MX_EVNTS, EPL_TOUT);
        if (n == -1)
        {
            if (EINTR == errno)
            {
                continue;
            }
            usleep(100 * 1000);
            continue;
        }
        else if (n == 0)
        {
            continue;
        }

        for (i = 0; i < n; i++)
        {
            struct epoll_event_data *d = (struct epoll_event_data *)events[i].data.ptr;
            int fd = (int)d->fd;
            enum EPOLL_EVENT_TYPE type = d->type;

            switch (type)
            {
                case EPOLL_EVENT_TYPE_HOTPLUG_CONNECT:
                {
                    printf("get hotplug connect...\n");
                    struct epoll_event ev;
                    int new_sock = accept(fd, (struct sockaddr *)&client, &len);
                    if (new_sock < 0)
                        break;

                    hotplg_msg_data.fd = new_sock;
                    hotplg_msg_data.type = EPOLL_EVENT_TYPE_HOTPLUG_MSG;
                    ev.events = EPOLLIN;
                    ev.data.ptr = (void *)&hotplg_msg_data;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &ev);
                    break;
                }
                case EPOLL_EVENT_TYPE_HOTPLUG_MSG:
                {
                    printf("get hotplug msg...\n");
                    unsigned char msg[128] = {0};
                    int len = 0;
                    len = read(fd, msg, sizeof(msg) - 1);
                    if (len > 0)
                    {
                        printf("%s\n", msg);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                        if (strstr(msg, "ACTION=wifi"))
                        {
                            castapp_process_hotplug_msg(msg);
                        }
                    }
                    else
                    {
                        printf("read hotplug msg fail\n");
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    return NULL;
}

static void castapp_detect_probed_wifi()
{
    pthread_attr_t attr;
    pthread_t tid;
    struct sockaddr_un serv;
    struct epoll_event ev;
    int hotplug_fd = -1;
    int ret;

    if (0 == access("/var/lib/misc/RTL8188FTV.probe", F_OK))
    {
        printf("Wi-Fi: RTL8188FTV\n");
        castapp_probed_wifi = HCCAST_NET_WIFI_8188FTV;
        hccast_wifi_mgr_set_wifi_model(HCCAST_NET_WIFI_8188FTV);
    }

    if (0 == access("/var/lib/misc/RTL8811FTV.probe", F_OK))
    {
        printf("Wi-Fi: RTL8811FTV\n");
        castapp_probed_wifi = HCCAST_NET_WIFI_8811FTV;
        hccast_wifi_mgr_set_wifi_model(HCCAST_NET_WIFI_8811FTV);
    }

    // start hot-plug detect
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    if (pthread_create(&tid, &attr, castapp_receive_event_func, (void *)NULL))
    {
        printf("pthread_create receive_event_func fail\n");
        goto out;
    }

    hotplug_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (hotplug_fd < 0)
    {
        printf("socket error\n");
        goto out;
    }
    else
    {
        printf("socket success\n");
    }

    unlink("/tmp/hotplug.socket");
    bzero(&serv, sizeof(serv));
    serv.sun_family = AF_LOCAL;
    strcpy(serv.sun_path, "/tmp/hotplug.socket");
    ret = bind(hotplug_fd, (struct sockaddr *)&serv, sizeof(serv));
    if (ret < 0)
    {
        printf("bind error\n");
        goto out;
    }
    else
    {
        printf("bind success\n");
    }

    ret = listen(hotplug_fd, 1);
    if (ret < 0)
    {
        printf("listen error\n");
        goto out;
    }
    else
    {
        printf("listen success\n");
    }

    epoll_fd = epoll_create1(0);
    hotplg_data.fd = hotplug_fd;
    hotplg_data.type = EPOLL_EVENT_TYPE_HOTPLUG_CONNECT;
    ev.events = EPOLLIN;
    ev.data.ptr = (void *)&hotplg_data;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, hotplug_fd, &ev) != 0)
    {
        printf("EPOLL_CTL_ADD hotplug fail\n");
        goto out;
    }
    else
    {
        printf("EPOLL_CTL_ADD hotplug success\n");
    }

out:
    return ;
}

static void castapp_httpd_start()
{
    printf("[%s-%d]\n", __func__, __LINE__);
    hccast_httpd_service_start();
}

static void castapp_httpd_stop()
{
    printf("[%s-%d]\n", __func__, __LINE__);
    hccast_httpd_service_stop();
}

static void castapp_dlna_start(char* uuid)
{
    char servicename[128] = {0};

    if (castapp_dlna_started)
    {
        return ;
    }

    castapp_dlna_started = 1;
    snprintf(servicename, sizeof(servicename), "%s-dlna", uuid);
    hccast_dlna_service_start();

    printf("[%s-%d]\n", __func__, __LINE__);

}

static void castapp_dlna_stop()
{
    if (castapp_dlna_started == 0)
    {
        return ;
    }
    castapp_dlna_started = 0;
    hccast_dlna_service_stop();
    printf("[%s-%d]\n", __func__, __LINE__);
}

static void castapp_miracast_start(unsigned char *uuid)
{
    char p2p_uuid[128] = { 0 };

    if (castapp_miracast_started)
    {
        return ;
    }

    castapp_miracast_started = 1;

    printf("[%s-%d]\n", __func__, __LINE__);

    sprintf(p2p_uuid, "%s-mira", uuid);
    hccast_mira_service_start();
}

static void castapp_miracast_stop()
{
    if (castapp_miracast_started == 0)
    {
        return ;
    }
    castapp_miracast_started = 0;
    printf("[%s-%d]\n", __func__, __LINE__);
    hccast_mira_service_stop();
}

static void castapp_aircast_start(char* uuid)
{
    char servicename[128] = {0};

    if (castapp_aircast_started)
    {
        return ;
    }

    castapp_aircast_started = 1;
    //snprintf(servicename, sizeof(servicename), "%s-itv", uuid);
    hccast_air_service_start();

    printf("[%s-%d]\n", __func__, __LINE__);
}

static void castapp_hostap_start()
{
    char cammand[128] = {0};
    sprintf(cammand, "hostapd_cli -i wlan0 enable");
    castapp_system_cmd(cammand);

    memset(cammand, 0, sizeof(cammand));
    sprintf(cammand, "ifconfig wlan0 192.168.68.1 netmask 255.255.255.0 broadcast 192.168.68.255");
    castapp_system_cmd(cammand);
}

static void castapp_hostap_stop()
{
    char cammand[128] = {0};
    sprintf(cammand, "hostapd_cli -i wlan0 disable");
    castapp_system_cmd(cammand);
}


static void castapp_do_connect_wifi(char* ssid, char* psk)
{
    char cammand[128] = {0};

    castapp_hostap_stop();

    sprintf(cammand, "wpa_cli -i wlan0 add_network");
    castapp_system_cmd(cammand);

    memset(cammand, 0, sizeof(cammand));
    sprintf(cammand, "wpa_cli -i wlan0 set_network 0 ssid  '\"%s\"' ", ssid);
    castapp_system_cmd(cammand);

    memset(cammand, 0, sizeof(cammand));
    sprintf(cammand, "wpa_cli -i wlan0 set_network 0 psk  '\"%s\"' ", psk);
    castapp_system_cmd(cammand);

    memset(cammand, 0, sizeof(cammand));
    sprintf(cammand, "wpa_cli -i wlan0 enable_network 0 ");
    castapp_system_cmd(cammand);

    memset(cammand, 0, sizeof(cammand));
    sprintf(cammand, "wpa_cli -i wlan0 select_network 0 ");
    castapp_system_cmd(cammand);

    memset(cammand, 0, sizeof(cammand));
    sprintf(cammand, "udhcpc -i wlan0 -q &");
    castapp_system_cmd(cammand);
    //hccast_wifi_mgr_udhcpc_start();

}

static void castapp_connect_wifi()
{
    unsigned char cmd[128] = {0};
    unsigned char ssid[WIFI_MAX_SSID_LEN] = {0};
    unsigned char psk[WIFI_MAX_PWD_LEN] = {0};
    castapp_stop_all_services();
    printf("Help:Please enter the same format as the following example:\n");
    printf("ssid=TP-LINK_A422\npsk=12345678\n");
    while (fgets(cmd, 128, stdin))
    {
        printf("cmd: %s\n", cmd);
        if (!memcmp(cmd, "cancel connect", strlen("cancel connect")))
        {
            printf("cancel connect wifi\n");
            return ;
        }
        else if (!memcmp(cmd, "ssid=", strlen("ssid=")))
        {
            memset(ssid, 0, sizeof(ssid));
            sscanf(cmd, "ssid=%s", ssid);
        }
        else if (!memcmp(cmd, "psk=", strlen("psk=")))
        {
            memset(psk, 0, sizeof(psk));
            sscanf(cmd, "psk=%s", psk);
        }
        else
        {
            printf("Invalid command\n");
        }

        if ((strlen(psk) > 0) && (strlen(ssid) > 0))
        {
            castapp_do_connect_wifi(ssid, psk);
            return ;
        }

        memset(cmd, 0, 128);
        usleep(10 * 1000);
    }

}

static void castapp_disconnect_wifi()
{
    unsigned char cammand[64] = {0};
    castapp_stop_all_services();

    sprintf(cammand, "wpa_cli -i wlan0 disable_network 0");
    castapp_system_cmd(cammand);

    memset(cammand, 0, sizeof(cammand));
    sprintf(cammand, "wpa_cli -i wlan0 remove_network 0");
    castapp_system_cmd(cammand);

    castapp_hostap_start();
}

static void castapp_killall_udhcpc()
{
    unsigned char cammand[64] = {0};

    sprintf(cammand, "killall -9 udhcpc");
    castapp_system_cmd(cammand);
}

static void castapp_udhcpd_cb(unsigned int yiaddr)
{
    struct in_addr addr;

    addr.s_addr = yiaddr;
    printf("udhcpd lease cb: %s\n", inet_ntoa(addr));
}

static void castapp_udhcpd_start()
{
    udhcpd_start(castapp_udhcpd_cb);
}

static void castapp_udhcpd_stop()
{
}

static void castapp_aircast_stop()
{
    if (castapp_aircast_started == 0)
    {
        return ;
    }
    castapp_aircast_started = 0;
    hccast_air_service_stop();
    printf("[%s-%d]\n", __func__, __LINE__);
}

void castapp_stop_all_services()
{
    castapp_miracast_stop();
    sleep(1);
    castapp_dlna_stop();
    castapp_aircast_stop();
}

static void castapp_aspect_ratio()
{
    unsigned char cmd[128] = {0};
    unsigned char tv_mode_str[16] = {0};
    unsigned char dis_mode_str[16] = {0};
    int tv_mode = 0xffffffff;
    int dis_mode = 0xffffffff;

    printf("DIS_TV_MODE:\n[0]--DIS_TV_4_3\n[1]--DIS_TV_16_9\n[2]--DIS_TV_AUTO\n");
    printf("DIS_MODE:\n[0]--DIS_PANSCAN\n[1]--DIS_PANSCAN_NOLINEAR\n[2]--DIS_LETTERBOX\n[3]--DIS_PILLBOX\n[4]--DIS_VERTICALCUT\n");
    printf("[5]--DIS_NORMAL_SCALE\n[6]--DIS_LETTERBOX149\n[7]--DIS_AFDZOOM\n[8]--DIS_PANSCAN43ON169\n[9]--DIS_COMBINED_SCALE\n[10]--DIS_VERTICALCUT_149\n");
    printf("example: \ntv_mode=1\ndis_mode=3\n");
    while (fgets(cmd, 128, stdin))
    {
        printf("cmd: %s\n", cmd);
        if (!memcmp(cmd, "cancel", strlen("cancel")))
        {
            printf("cancel set aspect ratio\n");
            return ;
        }
        else if (!memcmp(cmd, "tv_mode", strlen("tv_mode")))
        {
            sscanf(cmd, "tv_mode=%s", tv_mode_str);
            tv_mode = atoi(tv_mode_str);
        }
        else if (!memcmp(cmd, "dis_mode", strlen("dis_mode")))
        {
            sscanf(cmd, "dis_mode=%s", dis_mode_str);
            dis_mode = atoi(dis_mode_str);
        }
        else
        {
            printf("Invalid command\n");
        }

        if ((tv_mode != 0xffffffff) && (dis_mode != 0xffffffff))
        {
            hccast_set_aspect_mode(tv_mode, dis_mode, DIS_SCALE_ACTIVE_IMMEDIATELY);
            return ;
        }

        memset(cmd, 0, 128);
        usleep(10 * 1000);
    }

}

void hccast_start_services()
{
    if (hccast_get_current_scene() != HCCAST_SCENE_NONE)
    {
        hccast_scene_switch(HCCAST_SCENE_NONE);
    }

    printf("[%s]  begin start services.\n", __func__);

    hccast_dlna_service_start();
    hccast_air_service_start();
    hccast_mira_service_start();
}

void hccast_stop_services()
{
    printf("[%s]  begin stop services.\n", __func__);

    hccast_dlna_service_stop();
    hccast_air_service_stop();
    hccast_mira_service_stop();
}


void hccast_ap_dlna_aircast_start()
{
    hccast_dlna_service_start();
    hccast_air_service_start();
}

void hccast_ap_dlna_aircast_stop()
{
    hccast_dlna_service_stop();
    hccast_air_service_stop();
}

int hccast_wifi_mgr_callback_func(hccast_wifi_event_e event, void* in, void* out)
{
    log(SIMPLE, INFO, "[%s] event: %d", __func__, event);

    switch (event)
    {
        case HCCAST_WIFI_SCAN:
        {
            break;
        }
        case HCCAST_WIFI_SCAN_RESULT:
        {
            hccast_wifi_scan_result_t *res = (hccast_wifi_scan_result_t*)out;

            log(SIMPLE, INFO, "AP NUM: %d\n***********", res->ap_num);

            for (int i = 0; i < res->ap_num; i++)
            {
                log(SIMPLE, INFO, "ssid: %s, freq:%d , quality: %d", res->apinfo[i].ssid, \
                    res->apinfo[i].freq, res->apinfo[i].quality);
            }

            log(SIMPLE, INFO, "\n***********");

            break;
        }
        case HCCAST_WIFI_CONNECT:
        {
            break;
        }

        case HCCAST_WIFI_CONNECT_SSID:
        {
            log(SIMPLE, INFO, "SSID: %s", (char*)out);
            break;
        }

        case HCCAST_WIFI_CONNECT_RESULT:
        {
            hccast_udhcp_result_t* result = (hccast_udhcp_result_t*) out;
            if (result)
            {
                log(SIMPLE, INFO, "state: %d", result->stat);
                if (result->stat)
                {
                    log(SIMPLE, INFO, "ifname: %s", result->ifname);
                    log(SIMPLE, INFO, "ip addr: %s", result->ip);
                    log(SIMPLE, INFO, "mask addr: %s", result->mask);
                    log(SIMPLE, INFO, "gw addr: %s", result->gw);
                    log(SIMPLE, INFO, "dns: %s", result->dns);

                    hccast_net_ifconfig(result->ifname, result->ip, result->mask, result->gw);
                    hccast_net_set_dns(result->ifname, result->dns);
                    hccast_start_services();
                }
                else
                {
                    hccast_wifi_mgr_hostap_start();
                }


            }

            break;
        }

        case HCCAST_WIFI_DISCONNECT:
        {
            if (hccast_wifi_mgr_p2p_get_connect_stat() == 0)
            {
                hccast_wifi_mgr_hostap_start();
            }
            break;
        }

        case HCCAST_WIFI_HOSTAP_OFFER:
        {
            if (out)
            {
                struct in_addr tmp_addr =
                {
                    .s_addr = (unsigned int)out
                };

                log(WIFI, INFO, "addr: %s", inet_ntoa(tmp_addr));
            }

            hccast_ap_dlna_aircast_start();
        }

        default:
            break;
    }

    return 0;
}

void hccast_screen_config_default_init()
{
    memset(&g_http_setting, 0, sizeof(g_http_setting));

    g_http_setting.lang = 2;
    g_http_setting.mirror_frame = 1;
    g_http_setting.mirror_mode = 1;
    g_http_setting.aircast_mode = 0;
    g_http_setting.resolution = 4;
    sprintf(g_http_setting.dev_name,"hccast");
    sprintf(g_http_setting.dev_psk,"12345678");
    sprintf(g_http_setting.upg_server,"www.baidu.com");
    g_http_setting.bright = 50;
    g_http_setting.contrast = 50;
    g_http_setting.saturation = 50;
    g_http_setting.hue = 50;
    g_http_setting.sharpness = 50;
}

int hccast_check_ap_saved(char* check_ssid)
{
    int i = 0;
    int index = -1;

    for(i = 0; i < 5; i++)
    {
        if(strcmp(check_ssid, g_http_setting.ap_info[i].ssid) == 0)
        {
            index = i;
            return index;
        }
    }

    return index;
}

void hccast_save_wifi_ap(hccast_wifi_ap_info_t *wifi_ap)
{
    int i = 0;

    for(i = 3; i >= 0; i--)
    {
        memcpy(&(g_http_setting.ap_info[i+1]), &(g_http_setting.ap_info[i]), sizeof( hccast_wifi_ap_info_t));
    }
    memcpy(g_http_setting.ap_info, wifi_ap, sizeof(hccast_wifi_ap_info_t));
}

void hccast_delete_wifi_ap(int index)
{
    int i = 0;

    if(index > 4)
    {
        return;
    }

    for(i = index; i < 4; i++)
    {
        memcpy(&(g_http_setting.ap_info[i]), &(g_http_setting.ap_info[i+1]), sizeof(hccast_wifi_ap_info_t));
    }
    memset(&(g_http_setting.ap_info[4]),0x00,sizeof(hccast_wifi_ap_info_t));
}


int hccast_httpd_callback_func(hccast_httpd_event_e event, void* in, void* out)
{
    //log(SIMPLE, INFO, "[%s] event: %d", __func__,event);
    int ret = 0;
    char mac[6];
    hccast_wifi_ap_info_t *save_ap = NULL;
    hccast_wifi_ap_info_t *con_ap = NULL;
    hccast_wifi_ap_info_t *check_ap = NULL;
    hccast_wifi_ap_info_t *del_ap = NULL;
    hccast_wifi_scan_result_t *scan_res = NULL;
    char cur_ssid[WIFI_MAX_SSID_LEN] = {0};
    int index;

    switch (event)
    {
        case HCCAST_HTTPD_GET_MIRROR_MODE:
            ret = g_http_setting.mirror_mode;
            break;
        case HCCAST_HTTPD_SET_MIRROR_MODE:
            g_http_setting.mirror_mode = (int)in;
            break;
        case HCCAST_HTTPD_GET_AIRCAST_MODE:
            ret = g_http_setting.aircast_mode;
            break;
        case HCCAST_HTTPD_SET_AIRCAST_MODE:
            g_http_setting.aircast_mode = (int)in;
            break;
        case HCCAST_HTTPD_GET_MIRROR_FRAME:
            ret = g_http_setting.mirror_frame;
            break;
        case HCCAST_HTTPD_SET_MIRROR_FRAME:
            g_http_setting.mirror_frame = (int)in;
            break;
        case HCCAST_HTTPD_GET_BROWSER_LANGUAGE:
            ret = g_http_setting.lang;
            break;
        case HCCAST_HTTPD_SET_BROWSER_LANGUAGE:
            g_http_setting.lang = (int)in;
            break;
        case HCCAST_HTTPD_GET_SYS_BRIGHT:
            ret = g_http_setting.bright;
            break;
        case HCCAST_HTTPD_SET_SYS_BRIGHT:
            g_http_setting.bright = (int)in;
            break;
        case HCCAST_HTTPD_GET_SYS_CONTRAST:
            ret = g_http_setting.contrast;
            break;
        case HCCAST_HTTPD_SET_SYS_CONTRAST:
            g_http_setting.contrast = (int)in;
            break;
        case HCCAST_HTTPD_GET_SYS_SATURATION:
            ret = g_http_setting.saturation;
            break;
        case HCCAST_HTTPD_SET_SYS_SATURATION:
            g_http_setting.saturation = (int)in;
            break;
        case HCCAST_HTTPD_GET_SYS_HUE:
            ret = g_http_setting.hue;
            break;
        case HCCAST_HTTPD_SET_SYS_HUE:
            g_http_setting.hue = (int)in;
            break;
        case HCCAST_HTTPD_GET_SYS_SHARPNESS:
            ret = g_http_setting.sharpness;
            break;
        case HCCAST_HTTPD_SET_SYS_SHARPNESS:
            g_http_setting.sharpness = (int)in;
            break;
        case HCCAST_HTTPD_GET_SYS_RESOLUTION:
            ret = g_http_setting.resolution;
            break;
        case HCCAST_HTTPD_SET_SYS_RESOLUTION:
            g_http_setting.resolution = (int)in;
            break;
        case HCCAST_HTTPD_GET_DEVICE_MAC:
            hccast_net_get_mac(castapp_probed_wifi, mac);
            strncpy((char*)in, mac,sizeof(mac));
            break;
        case HCCAST_HTTPD_GET_DEVICE_NAME:
            strcpy((char*)in, g_http_setting.dev_name);
            break;
        case HCCAST_HTTPD_SET_DEVICE_NAME:
            strncpy(g_http_setting.dev_name, (char*)in,SERVICE_NAME_MAX_LEN);
            printf("device name: %s\n",g_http_setting.dev_name);
            break;
        case HCCAST_HTTPD_GET_DEVICE_PSK:
            strcpy((char*)in, g_http_setting.dev_psk);
            break;
        case HCCAST_HTTPD_SET_DEVICE_PSK:
            strncpy(g_http_setting.dev_psk, (char*)in,DEVICE_PSK_MAX_LEN);
            printf("device psk: %s\n",g_http_setting.dev_psk);
            break;

        case HCCAST_HTTPD_SET_SYS_RESTART:
            log(SIMPLE, INFO, "HCCAST_HTTPD_SET_SYS_RESTART");
            break;
        case HCCAST_HTTPD_SET_SYS_RESET:
            log(SIMPLE, INFO, "HCCAST_HTTPD_SET_SYS_RESET");
            hccast_screen_config_default_init();
            break;
        case HCCAST_HTTPD_WIFI_AP_DISCONNECT:
            hccast_wifi_mgr_udhcpc_stop();
            hccast_wifi_mgr_disconnect();
            break;
        case HCCAST_HTTPD_CHECK_AP_SAVE:
            check_ap = (hccast_wifi_ap_info_t*)in;
            save_ap = (hccast_wifi_ap_info_t *)out;
            index =hccast_check_ap_saved(check_ap->ssid);

            if(index >= 0)
            {
                if(save_ap)
                {
                    strcpy(save_ap->ssid, g_http_setting.ap_info[index].ssid);
                    strcpy(save_ap->pwd, g_http_setting.ap_info[index].pwd);
                }
                ret = 0;
            }
            else
            {
                ret = -1;
            }

            break;
        case HCCAST_HTTPD_WIFI_AP_CONNECT:
            con_ap = (hccast_wifi_ap_info_t *)in;
            if(con_ap)
            {
                hccast_stop_services();
                hccast_wifi_mgr_hostap_stop();

                hccast_wifi_mgr_connect(con_ap);
                if (hccast_wifi_mgr_get_connect_status())
                {
                    hccast_wifi_mgr_udhcpc_start();
                    hccast_save_wifi_ap(con_ap);
                    sleep(1);
                }
                else
                {
                    hccast_wifi_mgr_udhcpc_stop();
                    hccast_wifi_mgr_disconnect();
                }
            }
            break;
        case HCCAST_HTTPD_DELETE_WIFI_INFO:
            del_ap = (hccast_wifi_ap_info_t *)in;
            if(del_ap)
            {
                int index =hccast_check_ap_saved(del_ap->ssid);
                printf("del_ap->ssid:%s, index: %d\n",del_ap->ssid,index);
                if(index >= 0)
                {
                    hccast_delete_wifi_ap(index);
                }
            }
            break;

        case HCCAST_HTTPD_GET_WIFI_FREQ_MODE_EN:
        case HCCAST_HTTPD_GET_WIFI_FREQ_MODE:
            ret = hccast_wifi_mgr_freq_support_mode();
            break;
        case HCCAST_HTTPD_SET_WIFI_FREQ_MODE:
        {
#if 1
            hccast_wifi_freq_mode_e mode = (hccast_wifi_freq_mode_e)in;
            log(SIMPLE, INFO, "mode: %d", mode);
            hccast_wifi_mgr_hostap_switch_mode(mode);
#else
            int val = (int)in;
            if (1 == val)
            {
                hccast_dlna_service_start();
                hccast_mira_service_start();
            }
            else if(2 == val)
            {
                hccast_dlna_service_stop();
                hccast_mira_service_stop();
            }
#endif
            break;
        }
        case HCCAST_HTTPD_GET_WIFI_CONNECT_STATUS:
            ret = hccast_wifi_mgr_get_connect_status();
            break;
        case HCCAST_HTTPD_GET_CUR_WIFI_SSID:
            ret = hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
            memcpy(in,cur_ssid,sizeof(cur_ssid));
            break;
        case HCCAST_HTTPD_WIFI_SCAN :
            scan_res = (hccast_wifi_scan_result_t*)in;
            if(scan_res)
            {
                hccast_wifi_mgr_scan(scan_res);
            }
            else
            {
                printf("HCCAST_HTTPD_WIFI_SCAN error\n");
            }
            break;
        default :
            break;
    }

    return ret;
}


int hccast_dlna_callback_func(hccast_dlna_event_e event, void* in, void* out)
{
    log(SIMPLE, INFO, "[%s] event: %d", __func__, event);
    switch (event)
    {
        case HCCAST_DLNA_GET_DEVICE_NAME:
        {
            if (in)
            {
                unsigned char mac[8] = {0};
                unsigned char uuid[64] = {0};
                if (hccast_net_get_mac(castapp_probed_wifi, mac))
                {
                    sprintf((char *)in, "%s-%.2X%.2X%.2X_dlna", UUID_HEADER, mac[3], mac[4], mac[5]);
                }
                else
                {
                    sprintf((char *)in, "%s-000000_dlna", UUID_HEADER);
                }
            }
            break;
        }

        default:
            break;

    }

    return 0;
}

int hccast_mira_callback_func(hccast_mira_event_e event, void* in, void* out)
{
    if (event != 9)
    {
        log(SIMPLE, INFO, "[%s] event: %d", __func__, event);
    }

    switch (event)
    {
        case HCCAST_MIRA_GET_DEVICE_NAME:
        {
            if (in)
            {
                unsigned char mac[8] = {0};
                unsigned char uuid[64] = {0};
                if (hccast_net_get_mac(castapp_probed_wifi, mac))
                {
                    sprintf((char *)in, "%s-%.2X%.2X%.2X_mira", UUID_HEADER, mac[3], mac[4], mac[5]);
                }
                else
                {
                    sprintf((char *)in, "%s-000000_mira", UUID_HEADER);
                }
            }
            break;
        }

        case HCCAST_MIRA_STOP_DISP:
        {
            break;
        }
        case HCCAST_MIRA_CONNECTED:
        {
            break;
        }
        case HCCAST_MIRA_GET_CUR_WIFI_INFO:
        {
            char cur_ssid[WIFI_MAX_SSID_LEN] = {0};
            hccast_wifi_ap_info_t cur_ap;

            hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
            snprintf(cur_ap.ssid, sizeof(cur_ssid),"%s", cur_ssid);
            cur_ap.special_ap = 0;//default,normal wifi.
            memcpy(in,&cur_ap,sizeof(cur_ap));

            break;
        }
        default:
            break;

    }

    return 0;
}

int hccast_hostap_get_connect_count(void)
{
    return hostap_connect_count;
}

void hccast_hostap_set_connect_count(int count)
{
    hostap_connect_count = count;
}

int hccast_hostap_connect_update(void)
{
    int cur_connect = 0;
    int pre_connect = 0;
    static int needfresh = 0;

    if(hccast_mira_get_stat())
    {
        //printf("########%s mira connect do not anything.#########\n", __FUNCTION__);
        return 0;
    }

    cur_connect = hccast_wifi_mgr_hostap_get_sta_num(NULL);
    pre_connect = hccast_hostap_get_connect_count();
    if (cur_connect != pre_connect)
    {
        printf("########%s cur_connect=%d pre_connect=%d#########\n", __FUNCTION__, cur_connect, pre_connect);
        hccast_hostap_set_connect_count(cur_connect);
        if (cur_connect == 0)
        {
            //send msg notify ui no phone had connect to dongle.
            if (needfresh == 1)
            {
                printf("====================== no phone connect===============\n");
                needfresh = 0;
                hccast_ap_dlna_aircast_stop();
            }
        }
        else
        {
            //send msg notify ui have phone connect to dongle.
            if (needfresh == 0)
            {
                printf("====================== new phone connect===============\n");
                needfresh = 1;
            }
        }
    }

    return 0;
}

static void *hccast_hostap_connect_thread(void *args)
{
    printf("----------------------------hccast_hostap_connect_thread is running.-----------------------------\n");
    while (1)
    {
        hccast_hostap_connect_update();
        usleep(200*1000);
    }
}


void hccast_hostap_connect_detect_init()
{
    pthread_t tid;

    if (pthread_create(&tid, NULL,hccast_hostap_connect_thread, NULL) < 0)
    {
        printf("Create hccast_hostap_connect_thread error.\n");
    }
}


int hccast_air_callback_event(int msg_type, void* in, void* out)
{
    unsigned char mac[8] = {0};

    switch (msg_type)
    {
        case HCCAST_AIR_GET_SERVICE_NAME:

            if (hccast_net_get_mac(castapp_probed_wifi, mac))
            {
                sprintf((char *)in, "%s-%.2X%.2X%.2X_itv", UUID_HEADER, mac[3], mac[4], mac[5]);
            }
            else
            {
                sprintf((char *)in, "%s-000000_itv", UUID_HEADER);
            }

            break;

        case HCCAST_AIR_GET_NETWORK_DEVICE:
            sprintf((char *)in, "%s", "wlan0");
            break;

        case HCCAST_AIR_MIRROR_START:
            printf("[%s]HCCAST_AIR_MIRROR_START\n", __func__);
            break;
        case HCCAST_AIR_MIRROR_STOP:
            printf("[%s]HCCAST_AIR_MIRROR_STOP\n", __func__);
            break;
        case HCCAST_AIR_AUDIO_START:
            printf("[%s]HCCAST_AIR_AUDIO_START\n", __func__);
            break;
        case HCCAST_AIR_AUDIO_STOP:
            printf("[%s]HCCAST_AIR_AUDIO_STOP\n", __func__);
            break;

        default:
            break;
    }
    return 0;
}

void hccast_media_callback_func(hccast_media_event_e msg_type, void* param)
{
    switch (msg_type)
    {
        case HCCAST_MEDIA_EVENT_PARSE_END:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PARSE_END\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_PLAYING:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PLAYING\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_PAUSE:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PAUSE\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_BUFFERING:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_BUFFERING\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_PLAYBACK_END:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PLAYBACK_END\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_NOT_SUPPORT\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_DLNA:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_DLNA\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_URL_SEEK:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_SEEK  position: %ds\n", __func__, __LINE__, (int)param / 1000);
            break;
        case HCCAST_MEDIA_EVENT_SET_VOLUME:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_SET_VOLUME  volume: %d\n", __func__, __LINE__, (int)param);
            break;
        default:
            break;
    }

}

#define HOSTAP_CHANNEL_24G  1
#define HOSTAP_CHANNEL_5G   44

int main(int argc, char *argv[])
{
    unsigned char mac[8] = {0};
    unsigned char cmd[64] = {0};
    int probed_wifi = 0;

    printf("HCcast demo application\n");

#ifdef __linux__
    /* if wifi is exist, probe it */
    system("/etc/wifiprobe.sh &");
    sleep(10);
#endif

    castapp_detect_probed_wifi();

    //Wait for Wi-Fi probe
    while (0 == castapp_probed_wifi)
    {
        usleep(500 * 1000);
    }

    if (hccast_net_get_mac(castapp_probed_wifi, mac))
    {
        //sprintf(uuid, "%s-%.2X%.2X%.2X%.2X%.2X%.2X", UUID_HEADER, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        sprintf(uuid, "%s-%.2X%.2X%.2X", UUID_HEADER, mac[3], mac[4], mac[5]);
    }
    printf("UUID: %s\n", uuid);

    hccast_wifi_mgr_init(hccast_wifi_mgr_callback_func);
    hccast_httpd_service_init(hccast_httpd_callback_func);
    hccast_mira_service_init(hccast_mira_callback_func);
    hccast_dlna_service_init(hccast_dlna_callback_func);
    hccast_air_service_init(hccast_air_callback_event);
    hccast_media_init(hccast_media_callback_func);
    hccast_scene_init();
    hccast_set_aspect_mode(1, 3, DIS_SCALE_ACTIVE_IMMEDIATELY);//16:9 as default.
    hccast_screen_config_default_init();
    hccast_hostap_connect_detect_init();

    hccast_wifi_hostap_conf_t conf = {0};
    hccast_wifi_hostap_status_result_t res = {0};

    hccast_wifi_mgr_hostap_get_status(&res);
    printf("[SIMPLE][INFO]: hostap get status:\nchannel:%d, ssid:'%s'\n", res.channel, res.ssid);
    if (res.channel > 0 && res.channel < 15)
    {
        conf.channel = HOSTAP_CHANNEL_24G;
    }
    else
    {
        conf.channel = HOSTAP_CHANNEL_5G;
    }

    hccast_wifi_freq_mode_e mode = hccast_wifi_mgr_freq_support_mode();
    if (HCCAST_WIFI_FREQ_MODE_5G == mode)
    {
        conf.mode = HCCAST_WIFI_FREQ_MODE_5G; // 0: default(.conf), 1: g(2.4 GHz), 2: a(5 GHz), 3: ad(60 GHz)
        conf.channel = HOSTAP_CHANNEL_5G;
    }
    else if (HCCAST_WIFI_FREQ_MODE_24G == mode)
    {
        conf.mode = HCCAST_WIFI_FREQ_MODE_24G; // 0: default(.conf), 1: g(2.4 GHz), 2: a(5 GHz), 3: ad(60 GHz)
        conf.channel = HOSTAP_CHANNEL_24G;
    }

    conf.mode = HCCAST_WIFI_FREQ_MODE_24G; // 0: default(.conf), 1: g(2.4 GHz), 2: a(5 GHz), 3: ad(60 GHz)
    conf.channel = HOSTAP_CHANNEL_24G;

    //strcpy(conf.country_code, "CN"); //
    strncpy(conf.pwd, "12345678", sizeof(conf.pwd));
    strncpy(conf.ssid, (char*)uuid, sizeof(conf.ssid));

    hccast_wifi_mgr_hostap_set_conf(&conf);

    //first startup system is hostap mode.
    hccast_wifi_mgr_hostap_start();
    hccast_httpd_service_start();
    //hccast_dlna_service_start();
    //hccast_air_service_start();
    hccast_mira_service_start();

    while (1)
    {
        sleep(5);
    }

    return 0;
}
