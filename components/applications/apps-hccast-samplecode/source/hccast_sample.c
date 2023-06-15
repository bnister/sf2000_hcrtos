#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <kernel/lib/console.h>
#include <kernel/module.h>

//for net
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <kernel/notify.h>
#include <linux/notifier.h>
#include <hcuapi/sys-blocking-notify.h>
#include <hcuapi/dis.h>

#include <hccast/hccast_media.h>
#include <hccast/hccast_scene.h>
#include <hccast/hccast_air.h>
#include <hccast/hccast_dlna.h>
#include <hccast/hccast_mira.h>
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_httpd.h>
#include <hccast/hccast_um.h>
#include <hccast/hccast_dhcpd.h>
#include <hccast/hccast_net.h>

#include "hccast_sample.h"

static app_data_t m_app_data;
void *hccast_sample_wifi_disconnect_main(void *args);
void *hccast_sample_wifi_connect_main(void *args);
void *hccast_sample_hostapd_start_main(void *pvParameters);



#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT

wifi_model_st wifi_model_list[] =
{
    {"", "", HCCAST_NET_WIFI_NONE},
    {"rtl8188fu", "v0BDApF179", HCCAST_NET_WIFI_8188FTV},
    //{"rtl8811cu", "v0BDApC811", HCCAST_NET_WIFI_8811FTV},
};

wifi_model_st g_wifi_model = {0};
#endif
#endif

static int net_get_hwaddr(char *ifname, unsigned char *mac)
{
    struct ifreq ifr;
    int skfd;

    if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        printf("socket error\n");
        return -1;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0)
    {
        printf( "net_get_hwaddr: ioctl SIOCGIFHWADDR\n");
        close(skfd);
        return -1;
    }
    close(skfd);

    memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, IFHWADDRLEN);
    return 0;
}

static void hccast_sample_set_aspect_mode(dis_tv_mode_e ratio, 
                            dis_mode_e dis_mode, 
                            dis_scale_avtive_mode_e active_mode)
{
    int fd = open("/dev/dis", O_RDWR);
    if ( fd < 0)
        return;
    dis_aspect_mode_t aspect = {0};
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    aspect.active_mode = dis_mode;
    ioctl(fd, DIS_SET_ASPECT_MODE, &aspect);
    close(fd);
}


#ifdef WIFI_SUPPORT
void hccast_sample_data_mgr_wifi_ap_save(hccast_wifi_ap_info_t *wifi_ap)
{
    int8_t i = 0;

    for(i = MAX_WIFI_SAVE-2; i >= 0; i--)
    {
        memcpy(&(m_app_data.wifi_ap[i+1]), &(m_app_data.wifi_ap[i]), sizeof(hccast_wifi_ap_info_t));
    }

    memcpy(m_app_data.wifi_ap, wifi_ap, sizeof(hccast_wifi_ap_info_t));
}

void hccast_sample_data_mgr_wifi_ap_delete(int index)
{
    int i = 0;

    if(index > MAX_WIFI_SAVE-1)
    {
        return;
    }

    for(i = index; i < MAX_WIFI_SAVE-1; i++)
    {
        memcpy(&(m_app_data.wifi_ap[i]), &(m_app_data.wifi_ap[i+1]), sizeof(hccast_wifi_ap_info_t));
    }
    memset(&(m_app_data.wifi_ap[MAX_WIFI_SAVE-1]),0x00,sizeof(hccast_wifi_ap_info_t));

}

void hccast_sample_data_mgr_wifi_ap_clear(int index)
{
    if(index > MAX_WIFI_SAVE-1)
    {
        return;
    }
    memset(&(m_app_data.wifi_ap[index]),0x00,sizeof(hccast_wifi_ap_info_t));

}

/*
 */
bool hccast_sample_data_mgr_wifi_ap_get(hccast_wifi_ap_info_t *wifi_ap)
{
    if (strlen(m_app_data.wifi_ap[0].ssid))
    {
        memcpy(wifi_ap, m_app_data.wifi_ap, sizeof(hccast_wifi_ap_info_t));
        return true;
    }
    else
        return false;
}


int hccast_sample_data_mgr_check_ap_saved(hccast_wifi_ap_info_t* check_wifi)
{
    int i = 0;
    int index = -1;

    for(i = 0; i < MAX_WIFI_SAVE; i++)
    {
        if(strlen(m_app_data.wifi_ap[i].ssid) && strlen(check_wifi->ssid))
        {
            if(strcmp(check_wifi->ssid, m_app_data.wifi_ap[i].ssid) == 0)
            {
                index = i;
                return index;
            }
        }
    }

    return index;
}

hccast_wifi_ap_info_t *hccast_sample_data_mgr_get_wifi_info(char* ssid)
{
    int i = 0;

    for(i = 0; i < MAX_WIFI_SAVE; i++)
    {
        if(strcmp(ssid, m_app_data.wifi_ap[i].ssid) == 0)
        {
            return &m_app_data.wifi_ap[i];
        }
    }

    return NULL;
}
#endif

static void hccast_sample_data_default_init(void)
{
    printf("%s(), line:%d. reset data!\n", __FUNCTION__, __LINE__);
    unsigned char mac[6] = {0};
    static int inited = 0;
    if(inited)
    {
        return ;
    }

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
    memset(m_app_data.wifi_ap, 0, sizeof(m_app_data.wifi_ap));
#endif
#endif

    m_app_data.browserlang = 2;
    m_app_data.mirror_frame = 1;
    m_app_data.mirror_mode = 1;
    m_app_data.aircast_mode = 2;//Auto.
    m_app_data.resolution = APP_TV_SYS_AUTO;//APP_TV_SYS_1080P;
    m_app_data.mirror_rotation = 0;//default disable.

    m_app_data.wifi_mode = 1; // 1: 2.4, 2: 5G, 3: 60G
    m_app_data.cast_dev_name_changed = 0;
    memset(m_app_data.cast_dev_name, 0,WIFI_MAX_SSID_LEN );
    net_get_hwaddr("wlan0",mac);
    snprintf(m_app_data.cast_dev_name, WIFI_MAX_SSID_LEN, "%s-%02X%02X%02X",
             "Hccast", mac[3]&0xff, mac[4]&0xff, mac[5]&0xff);

    snprintf(m_app_data.cast_dev_psk,WIFI_MAX_PWD_LEN,"%s","12345678");
    inited = 1;
}

void hccast_sample_start_services(void)
{
    if (hccast_get_current_scene() != HCCAST_SCENE_NONE)
    {
        hccast_scene_switch(HCCAST_SCENE_NONE);
    }

    printf("[%s]  begin start services.\n", __func__);

#ifdef DLNA_SUPPORT
    hccast_dlna_service_start();
#endif
#ifdef AIRCAST_SUPPORT
    hccast_air_service_start();
#endif
#ifdef MIRACAST_SUPPORT
    hccast_mira_service_start();
#endif
}

void hccast_sample_stop_services(void)
{
    printf("[%s]  begin stop services.\n", __func__);
#ifdef DLNA_SUPPORT
    hccast_dlna_service_stop();
#endif
#ifdef AIRCAST_SUPPORT
    hccast_air_service_stop();
#endif
#ifdef MIRACAST_SUPPORT
    hccast_mira_service_stop();
#endif
}

static void hccast_sample_ap_dlna_aircast_start(void)
{
#ifdef DLNA_SUPPORT
    hccast_dlna_service_start();
#endif
#ifdef AIRCAST_SUPPORT
    hccast_air_service_start();
#endif
}

static void hccast_sample_ap_dlna_aircast_stop(void)
{
#ifdef DLNA_SUPPORT
    hccast_dlna_service_stop();
#endif
#ifdef AIRCAST_SUPPORT
    hccast_air_service_stop();
#endif
}


#ifdef AIRCAST_SUPPORT
static int hccast_air_callback_event(hccast_air_event_e event, void* in, void* out)
{
    app_data_t * app_data = &m_app_data;

    switch (event)
    {
        case HCCAST_AIR_GET_SERVICE_NAME:
            printf("[%s]HCCAST_AIR_GET_SERVICE_NAME\n",__func__);
            sprintf((char *)in, "%s_itv", app_data->cast_dev_name);
            break;
        case HCCAST_AIR_GET_NETWORK_DEVICE:
            printf("[%s]HCCAST_AIR_GET_NETWORK_DEVICE\n",__func__);
            sprintf((char *)in, "%s", "wlan0");
            break;
        case HCCAST_AIR_GET_MIRROR_RESOLUTION:
            printf("[%s]HCCAST_AIR_GET_MIRROR_RESOLUTION\n",__func__);
            break;
        case HCCAST_AIR_GET_MIRROR_FRAME:
            printf("[%s]HCCAST_AIR_GET_MIRROR_FRAME\n",__func__);
            break;
        case HCCAST_AIR_GET_MIRROR_MODE:
            printf("[%s]HCCAST_AIR_GET_MIRROR_MODE\n",__func__);
            *(int*)in = app_data->aircast_mode;
            break;
        case HCCAST_AIR_GET_NETWORK_STATUS:
            printf("[%s]HCCAST_AIR_GET_NETWORK_STATUS\n",__func__);
            *(int*)in = 0;
            break;
        case HCCAST_AIR_MIRROR_START:
            printf("[%s]HCCAST_AIR_MIRROR_START\n",__func__);
            break;
        case HCCAST_AIR_MIRROR_STOP:
            printf("[%s]HCCAST_AIR_MIRROR_STOP\n",__func__);
            break;
        case HCCAST_AIR_AUDIO_START:
            printf("[%s]HCCAST_AIR_AUDIO_START\n",__func__);
            break;
        case HCCAST_AIR_AUDIO_STOP:
            printf("[%s]HCCAST_AIR_AUDIO_STOP\n",__func__);
            break;
        case HCCAST_AIR_INVALID_CERT:
            printf("[%s],line:%d. HCCAST_AIR_INVALID_CERT\n",__func__, __LINE__);
            break;
        case HCCAST_AIR_CHECK_4K_MODE:
            *(int*)in = 0;
            break;
        case HCCAST_AIR_HOSTAP_MODE_SKIP_URL:
            printf("[%s]HCCAST_AIR_HOSTAP_MODE_SKIP_URL\n",__func__);
            break;
        case HCCAST_AIR_BAD_NETWORK:
            printf("[%s]HCCAST_AIR_BAD_NETWORK\n",__func__);
            break;
        case HCCAST_AIR_GET_MIRROR_ROTATION:
            *(int*)in = app_data->mirror_rotation;
            break;
        default:
            break;
    }
    return 0;
}
#endif

#ifdef DLNA_SUPPORT
static int hccast_dlna_callback_func(hccast_dlna_event_e event, void* in, void* out)
{
    char *str_tmp = NULL;
    app_data_t * app_data = &m_app_data;

    switch (event)
    {
        case HCCAST_DLNA_GET_DEVICE_NAME:
        {
            printf("[%s]HCCAST_DLNA_GET_DEVICE_NAME\n",__func__);
            if (in)
            {
                str_tmp = app_data->cast_dev_name;
                if (str_tmp)
                {
                    sprintf((char *)in, "%s_dlna", str_tmp);
                    printf("[%s]HCCAST_DLNA_GET_DEVICE_NAME:%s\n",__func__, str_tmp);
                }
            }
            break;
        }
        default:
            break;
    }
    return 0;
}
#endif

#ifdef MIRACAST_SUPPORT
static int hccast_mira_callback_func(hccast_mira_event_e event, void* in, void* out)
{
    char *str_tmp = NULL;
    app_data_t * app_data = &m_app_data;
    char cur_ssid[WIFI_MAX_SSID_LEN] = {0};

    switch (event)
    {
        case HCCAST_MIRA_GET_DEVICE_NAME:
        {
            printf("[%s]HCCAST_DLNA_GET_DEVICE_NAME\n",__func__);
            if (in)
            {
                str_tmp = app_data->cast_dev_name;
                if (str_tmp)
                {
                    sprintf((char *)in, "%s_mira", str_tmp);
                    printf("[%s]HCCAST_MIRA_GET_DEVICE_NAME: %s\n",__func__, str_tmp);
                }
            }
            break;
        }
        case HCCAST_MIRA_GET_CUR_WIFI_INFO:
        {
            printf("%s HCCAST_HTTPD_GET_CUR_WIFI_INFO \n",__func__);
            hccast_wifi_ap_info_t *cur_ap;
            hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
            cur_ap = hccast_sample_data_mgr_get_wifi_info(cur_ssid);
            if(cur_ap)
                memcpy(in, cur_ap,sizeof(hccast_wifi_ap_info_t));
            else
                memcpy(in, cur_ssid,sizeof(hccast_wifi_ap_info_t));

            break;
        }
        default:
            break;
    }
    return 0;
}
#endif

static void media_callback_func(hccast_media_event_e msg_type, void* param)
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

            break;
        case HCCAST_MEDIA_EVENT_PLAYBACK_END:

            break;
        case HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR:

            break;
        case HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR:

            break;
        case HCCAST_MEDIA_EVENT_VIDEO_NOT_SUPPORT:

            break;
        case HCCAST_MEDIA_EVENT_AUDIO_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_AUDIO_NOT_SUPPORT\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_NOT_SUPPORT\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_DLNA:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_DLNA, media_type: %d\n", __func__, __LINE__,(hccast_media_type_e)param);
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST, media_type: %d\n", __func__, __LINE__,(hccast_media_type_e)param);
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

static int httpd_callback_func(hccast_httpd_event_e event, void* in, void* out)
{
	int ret = 0;
#ifdef NETWORK_SUPPORT
    unsigned char mac[6];
    hccast_wifi_ap_info_t *save_ap = NULL;
    hccast_wifi_ap_info_t *con_ap = NULL;
    hccast_wifi_ap_info_t *check_ap = NULL;
    hccast_wifi_ap_info_t *del_ap = NULL;
    hccast_wifi_ap_info_t *ap_wifi = NULL;
    int index;
    app_data_t *app_data = &m_app_data;
    char cur_ssid[WIFI_MAX_SSID_LEN] = {0};
    hccast_wifi_scan_result_t *scan_res = NULL;
    int temp;

    switch (event)
    {
        case HCCAST_HTTPD_GET_DEV_PRODUCT_ID:
            strcpy((char*)in, "HC_RTOS");
            break;
        case HCCAST_HTTPD_GET_DEV_VERSION:
            strcpy((char*)in, "1970");
            break;
        case HCCAST_HTTPD_GET_MIRROR_MODE:
            ret = app_data->mirror_mode;
            break;
        case HCCAST_HTTPD_SET_MIRROR_MODE:
            app_data->mirror_mode = (int)in;
            break;
        case HCCAST_HTTPD_GET_AIRCAST_MODE:
            ret = app_data->aircast_mode;
            break;
        case HCCAST_HTTPD_SET_AIRCAST_MODE:
#ifdef AIRCAST_SUPPORT
            temp = (int)in;
            if(temp != app_data->aircast_mode)
            {
                app_data->aircast_mode = temp;
                if(hccast_get_current_scene() == HCCAST_SCENE_NONE)
                {
                    hccast_air_service_stop();
                    hccast_air_service_start();
                }
            }
#endif
            break;
        case HCCAST_HTTPD_GET_MIRROR_FRAME:
            ret = app_data->mirror_frame;
            break;
        case HCCAST_HTTPD_SET_MIRROR_FRAME:
            app_data->mirror_frame = (int)in;
            break;
        case HCCAST_HTTPD_GET_BROWSER_LANGUAGE:
            ret = app_data->browserlang;
            break;
        case HCCAST_HTTPD_SET_BROWSER_LANGUAGE:
            app_data->browserlang = (int)in;

            break;
        case HCCAST_HTTPD_GET_SYS_RESOLUTION:
            ret = app_data->resolution;
            break;
        case HCCAST_HTTPD_SET_SYS_RESOLUTION:
            break;
        case HCCAST_HTTPD_GET_DEVICE_MAC:
            net_get_hwaddr("wlan0",mac);
            memcpy(in, mac,sizeof(mac));
            break;
        case HCCAST_HTTPD_GET_DEVICE_NAME:
            strcpy((char*)in, app_data->cast_dev_name);
            break;
        case HCCAST_HTTPD_SET_DEVICE_NAME:
            strncpy(app_data->cast_dev_name, (char*)in,SERVICE_NAME_MAX_LEN);
            break;
        case HCCAST_HTTPD_GET_DEVICE_PSK:
            strcpy((char*)in, app_data->cast_dev_psk);
            break;
        case HCCAST_HTTPD_SET_DEVICE_PSK:
            strncpy(app_data->cast_dev_psk, (char*)in,DEVICE_PSK_MAX_LEN);
            break;
        case HCCAST_HTTPD_SET_SYS_RESTART:
            printf("HCCAST_HTTPD_SET_SYS_RESTART\n");
            break;
        case HCCAST_HTTPD_SET_SYS_RESET:
            printf("HCCAST_HTTPD_SET_SYS_RESET\n");
            break;
        case HCCAST_HTTPD_SHOW_PROGRESS:
        {
            break;
        }
        case HCCAST_HTTPD_GET_UPLOAD_DATA_START:
        {
            break;
        }
        case HCCAST_HTTPD_GET_UPGRADE_FILE_BEGING:
        {
            break;
        }
        case HCCAST_HTTPD_GET_UPLOAD_DATA_FAILED:
        {
            break;
        }
        case HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD:
        {
            break;
        }
        case HCCAST_HTTPD_MSG_UPGRADE_BAD_RES:
        {
            break;
        }
        case HCCAST_HTTPD_MSG_USER_UPGRADE_ABORT:
        {
            break;
        }
        case HCCAST_HTTPD_MSG_UPGRADE_FILE_SUC:
        {
            break;
        }
        case HCCAST_HTTPD_MSG_UPLOAD_DATA_SUC:
        {
            break;
        }
        case HCCAST_HTTPD_GET_WIFI_FREQ_MODE_EN:
            break;
        case HCCAST_HTTPD_GET_WIFI_FREQ_MODE:
            ret = app_data->wifi_mode;
            break;
        case HCCAST_HTTPD_SET_WIFI_FREQ_MODE:
            break;
        case HCCAST_HTTPD_GET_CUR_SCENE_PLAY:
#ifdef AIRCAST_SUPPORT
            if(hccast_air_ap_audio_stat())
            {
                ret = 1;
            }
            else if(hccast_get_current_scene() != HCCAST_SCENE_NONE)
            {
                if((hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY) || ((hccast_get_current_scene() == HCCAST_SCENE_DLNA_PLAY)))
                {
                    if(hccast_media_get_status() == HCCAST_MEDIA_STATUS_STOP)
                    {
                        ret = 0;
                    }
                    else
                    {
                        ret = 1;
                    }
                }
                else
                {
                    ret = 1;
                }
            }
            else
            {
                ret = 0;
            }
#endif
            break;
        case HCCAST_HTTPD_STOP_MIRA_SERVICE:
#ifdef MIRACAST_SUPPORT
            hccast_mira_service_stop();
            sleep(1);
#endif
            break;
        case HCCAST_HTTPD_START_MIRA_SERVICE:
#ifdef MIRACAST_SUPPORT
            hccast_mira_service_start();
#endif
            break;
        case HCCAST_HTTPD_GET_MIRROR_ROTATION:
            ret = app_data->mirror_rotation;
            break;
        case HCCAST_HTTPD_SET_MIRROR_ROTATION:
        {
            temp = (int)in;
            if(app_data->mirror_rotation != temp)
            {
                app_data->mirror_rotation = temp;
            }
        }
        break;

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
        case HCCAST_HTTPD_WIFI_AP_DISCONNECT:
        {
            printf("%s HCCAST_HTTPD_WIFI_AP_DISCONNECT \n",__func__);
            pthread_t tid;
            pthread_create(&tid, NULL,hccast_sample_wifi_disconnect_main, NULL);
            break;
        }
        case HCCAST_HTTPD_CHECK_AP_SAVE:
        {
            check_ap = (hccast_wifi_ap_info_t*)in;
            save_ap = (hccast_wifi_ap_info_t *)out;
            index = hccast_sample_data_mgr_check_ap_saved(check_ap);

            if(index >= 0)
            {
                if(save_ap)
                {
                    strcpy(save_ap->ssid, app_data->wifi_ap[index].ssid);
                    strcpy(save_ap->pwd, app_data->wifi_ap[index].pwd);
                }
                ret = 0;
            }
            else
            {
                ret = -1;
            }
            break;
        }
        case HCCAST_HTTPD_WIFI_AP_CONNECT:
        {
            printf("%s HCCAST_HTTPD_WIFI_AP_CONNECT \n",__func__);
            con_ap = (hccast_wifi_ap_info_t *)in;
            if(con_ap)
            {
                ap_wifi = (hccast_wifi_ap_info_t *)malloc(sizeof(hccast_wifi_ap_info_t));
                if(ap_wifi)
                {
                    pthread_t tid;
                    memcpy(ap_wifi,con_ap,sizeof(hccast_wifi_ap_info_t));
                    pthread_create(&tid, NULL,hccast_sample_wifi_connect_main,(void*)ap_wifi);
                }
                else
                {
                    return -1;
                }
            }
            break;
        }
        case HCCAST_HTTPD_DELETE_WIFI_INFO:
            printf("%s HCCAST_HTTPD_DELETE_WIFI_INFO \n",__func__);
            del_ap = (hccast_wifi_ap_info_t *)in;
            if(del_ap)
            {
                index =hccast_sample_data_mgr_check_ap_saved(del_ap);
                printf("del_ap->ssid:%s, index: %d\n",del_ap->ssid,index);
                if(index >= 0)
                {
                    hccast_sample_data_mgr_wifi_ap_delete(index);
                }
            }
            break;
        case HCCAST_HTTPD_GET_CUR_WIFI_INFO:
        {
            printf("%s HCCAST_HTTPD_GET_CUR_WIFI_INFO \n",__func__);
            hccast_wifi_ap_info_t *cur_ap;
            hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
            cur_ap = hccast_sample_data_mgr_get_wifi_info(cur_ssid);
            if(cur_ap)
                memcpy(in,cur_ap,sizeof(hccast_wifi_ap_info_t));
            break;
        }
        case HCCAST_HTTPD_GET_WIFI_CONNECT_STATUS:
            printf("%s HCCAST_HTTPD_GET_WIFI_CONNECT_STATUS \n",__func__);
            ret = hccast_wifi_mgr_get_connect_status();
            break;
        case HCCAST_HTTPD_GET_CUR_WIFI_SSID:
            printf("%s HCCAST_HTTPD_GET_CUR_WIFI_SSID \n",__func__);
            ret = hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
            memcpy(in,cur_ssid,sizeof(cur_ssid));
            break;
        case HCCAST_HTTPD_WIFI_SCAN:
            printf("%s HCCAST_HTTPD_WIFI_SCAN \n",__func__);
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
#endif
#endif
        default :
            break;
    }
#endif
    return ret;
}

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
static int wifi_mgr_callback_func(hccast_wifi_event_e event, void* in, void* out)
{
    printf("[%s %d]: event: %d\n", __func__, __LINE__, event);

    switch (event)
    {
        case HCCAST_WIFI_SCAN:
        {
            printf("%s:%d: event: HCCAST_WIFI_SCAN\n", __func__, __LINE__);
            break;
        }
        case HCCAST_WIFI_SCAN_RESULT:
        {
            printf("%s:%d: event: HCCAST_WIFI_SCAN_RESULT\n", __func__, __LINE__);
            hccast_wifi_scan_result_t *res = (hccast_wifi_scan_result_t*)out;
            printf("AP NUM: %d\n***********\n", res->ap_num);
            for(int i = 0; i < res->ap_num; i++)
            {
                printf("ssid: %s, quality: %d\n", res->apinfo[i].ssid, res->apinfo[i].quality);
            }
            printf("\n***********\n");

            break;
        }
        case HCCAST_WIFI_CONNECT:
        {
            printf("%s:%d: event: HCCAST_WIFI_CONNECT\n", __func__, __LINE__);
            break;
        }
        case HCCAST_WIFI_CONNECT_SSID:
        {
            printf("%s:%d: event: HCCAST_WIFI_CONNECT_SSID, ssid: %s\n", __func__, __LINE__, (char*)out);
            break;
        }
        case HCCAST_WIFI_CONNECT_RESULT:
        {
            hccast_udhcp_result_t* result = (hccast_udhcp_result_t *) out;

            printf("%s:%d: event: HCCAST_WIFI_CONNECT_RESULT\n", __func__, __LINE__);

            if (result)
            {
                printf("state: %d\n", result->stat);
                if (result->stat)
                {
                    printf("ip addr: %s\n", result->ip);
                    if((hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
                    {
                        printf("Cur scene is doing USB MIRROR\n");
                    }
                    else
                    {
                        hccast_sample_start_services();
                    }
                }
                else
                {
                    hccast_wifi_mgr_disconnect_no_message();
                    usleep(50*1000);
                    hccast_wifi_mgr_hostap_start();
                    if((hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
                    {
                        printf("Cur scene is doing USB MIRROR\n");
                    }
                    else
                    {
#ifdef MIRACAST_SUPPORT
                        hccast_mira_service_start();
#endif
                    }
                }
             }
            break;
        }
        case HCCAST_WIFI_DISCONNECT:
        {
            if (hccast_wifi_mgr_p2p_get_connect_stat() == 0)
            {
                printf("%s Wifi has been disconnected, beging change to host ap mode\n",__func__);
                hccast_media_stop();
                hccast_sample_ap_dlna_aircast_stop();
                hccast_wifi_mgr_exit_sta_mode();
                hccast_wifi_mgr_hostap_start();
            }

            break;
        }
        case HCCAST_WIFI_HOSTAP_OFFER:
        {
            printf("%s:%d: event: HCCAST_WIFI_HOSTAP_OFFER\n", __func__, __LINE__);
            hccast_sample_ap_dlna_aircast_start();
        }
        default:
            break;
    }

    return 0;
}

static int hccast_wifi_hotplug_notify(struct notifier_block *self, unsigned long action, void* dev)
{
    unsigned char found = 0;

    switch (action)
    {
        case USB_DEV_NOTIFY_CONNECT:
        {
            char path[64] = {0};
            FILE* fp;

            printf("Wi-Fi Plug In -> ");

            for (int i = 0; i < sizeof(wifi_model_list)/sizeof(wifi_model_list[0]); i++)
            {
                snprintf(path, sizeof(path) - 1, "/dev/bus/usb/%s", wifi_model_list[i].desc);
                fp = fopen(path, "rb");
                if (fp)
                {
                    fclose(fp);
                    found = 1;
                    printf("Model: %s\n", wifi_model_list[i].name);

                    memcpy(&g_wifi_model, &wifi_model_list[i], sizeof(g_wifi_model));
                    break;
                }
            }

            if (!found)
            {
                printf ("Unsupport Model\n");
            }
            // snd msg

            break;
        }
        case USB_DEV_NOTIFY_DISCONNECT:
        {
            printf("Wi-Fi Plug Out -> model: %s\n", g_wifi_model.name);

            memset (&g_wifi_model, 0, sizeof(g_wifi_model));
            // snd msg

            break;
        }
        default:
            return 0;
    }

    return NOTIFY_OK;
}

static struct notifier_block hccast_wifi_hotplug_nb =
{
    .notifier_call = hccast_wifi_hotplug_notify,
};

static void* hccast_wifi_hotplug_detect_probed(void* arg)
{
    FILE* fp;

    do
    {
        char path[64] = {0};
        for (int i = 0; i < sizeof(wifi_model_list)/sizeof(wifi_model_list[0]); i++)
        {
            snprintf(path, sizeof(path) - 1, "/dev/bus/usb/%s", wifi_model_list[i].desc);
            fp = fopen(path, "rb");
            if (fp)
            {
                fclose(fp);
                printf("detect wifi model: %s\n", wifi_model_list[i].name);

                memcpy(&g_wifi_model, &wifi_model_list[i], sizeof(g_wifi_model));
                goto EXIT;
            }
        }

        usleep(500*1000);
    }
    while (1);

EXIT:
    return NULL;
}

static int hccast_wifi_hotplug_init(void)
{
    sys_register_notify(&hccast_wifi_hotplug_nb);

    //pthread_t pid;
    //pthread_create(&pid, NULL, hccast_wifi_hotplug_detect_probed, NULL);
    hccast_wifi_hotplug_detect_probed(NULL);

    return 0;
}
#else
static void udhcpc_cb(unsigned int data)
{
    hccast_udhcp_result_t *in = (hccast_udhcp_result_t*)data;

    if (in)
    {
        printf("udhcpc got ip: %s\n", in->ip);
        net_read = 1;
    }
    else
    {
        net_read = 0;
    }
}

static udhcp_conf_t eth_udhcpc_conf =
{
    .func = udhcpc_cb,
    .ifname = UDHCP_IF_WLAN0,
    .pid    = 0,
    .run    = 0
};
#endif
#endif

static int hccast_sample_wl_loop(int argc, char *argv[])
{
    hccast_sample_data_default_init();

#ifdef DLNA_SUPPORT
    hccast_dlna_service_init(hccast_dlna_callback_func);
#endif

#ifdef AIRCAST_SUPPORT
    hccast_air_service_init(hccast_air_callback_event);
#endif

#ifdef MIRACAST_SUPPORT
    hccast_mira_service_init(hccast_mira_callback_func);
#endif

#if (defined(SUPPORT_DLNA) || (defined(SUPPORT_AIRCAST) && !defined(AIRCAST_SUPPORT_MIRROR_ONLY)))
    hccast_media_init(media_callback_func);
#endif

#ifdef NETWORK_SUPPORT
    hccast_httpd_service_init(httpd_callback_func);
#endif

#if defined(AIRCAST_SUPPORT) || defined(DLNA_SUPPORT) || defined(MIRACAST_SUPPORT)
    hccast_scene_init();
#endif

    hccast_sample_set_aspect_mode(1, 3, DIS_SCALE_ACTIVE_IMMEDIATELY); //16:9 as default.

#ifdef NETWORK_API
#ifdef WIFI_SUPPORT
    hccast_wifi_hotplug_init();
    hccast_wifi_mgr_init(wifi_mgr_callback_func);

    printf("##### wifi model: %s type: %d #####\n", g_wifi_model.name, g_wifi_model.type);

    hccast_wifi_mgr_set_wifi_model(g_wifi_model.type);
    hccast_sample_hostapd_start_main(NULL);
#else
    udhcpc_start(&eth_udhcpc_conf);
#endif
#endif

#ifdef MIRACAST_SUPPORT
    hccast_mira_service_start();
#endif

#ifdef AIRCAST_SUPPORT
    //hccast_air_service_start();
#endif

#ifdef DLNA_SUPPORT
    //hccast_dlna_service_start();
#endif

#ifdef NETWORK_SUPPORT
    hccast_httpd_service_start();
#endif

    while(1)
    {
        sleep(1);
    }

    return 0;
}

static void *hccast_sample_wl_main(void *pvParameters)
{
    hccast_sample_wl_loop(0, NULL);
    return NULL;
}

int hccast_sample_wl_start(int argc, char **argv)
{
    pthread_t pid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x10000);
    pthread_create(&pid,&attr,hccast_sample_wl_main,NULL);

    return 0;
}

#ifdef USBMIRROR_SUPPORT
static char ium_uuid[40] = {0};
static hccast_aum_param_t aum_param;

void ium_event_process_cb(int event, void *param1, void *param2)
{
    printf("ium event: %d\n", event);
}

void aum_event_process_cb(int event, void *param1, void *param2)
{
    printf("aum event: %d\n", event);
}


void *hccast_sample_um_main(void *pvParameters)
{
    hccast_um_param_t um_param;

    printf("HCCast USB mirrorring demo\n");

    um_param.screen_rotate_en = 0;
    um_param.screen_rotate_auto = 1;
    um_param.full_screen_en = 1;
    hccast_um_param_set(&um_param);

    if (hccast_um_init() < 0)
    {
        return NULL;
    }

    hccast_ium_start(ium_uuid, ium_event_process_cb);

    memset(&aum_param, 0, sizeof(aum_param));
    strcat(aum_param.product_id, "HCT-AT01");
    strcat(aum_param.fw_url, "http://119.3.89.190:8080/apk/elfcast-HCT-AT01.json");
    strcat(aum_param.apk_url, "http://119.3.89.190:8080/apk/elfcast.apk");
    strcat(aum_param.aoa_desc, "ElfCast-Screen_Mirror");
    aum_param.fw_version = 0;
    hccast_aum_start(&aum_param, aum_event_process_cb);

    while (1)
    {
        sleep(10);
    }
}
#endif

int hccast_sample_um_start(int argc, char **argv)
{
#ifdef USBMIRROR_SUPPORT
    pthread_t pid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x10000);
    pthread_create(&pid,&attr,hccast_sample_um_main,NULL);
#endif
    return 0;
}

int hccast_sample_um_stop_mirroring()
{
#ifdef USBMIRROR_SUPPORT
    hccast_ium_stop_mirroring();
    hccast_aum_stop_mirroring();
#endif

    return 0;
}

void *hccast_sample_hostapd_start_main(void *pvParameters)
{
#ifdef WIFI_SUPPORT
    app_data_t * app_data = &m_app_data;
    hccast_wifi_hostap_conf_t conf = {0};
    conf.mode = HCCAST_WIFI_FREQ_MODE_24G;
    conf.channel = 6;
    strncpy(conf.pwd, "12345678", sizeof(conf.pwd));
    strncpy(conf.ssid, app_data->cast_dev_name, sizeof(conf.ssid));
    hccast_wifi_mgr_hostap_store_conf(&conf);
    hccast_wifi_mgr_hostap_start();
#endif
    return NULL;
}

int hccast_sample_hostapd_start(int argc, char **argv)
{
#ifdef WIFI_SUPPORT
    hccast_sample_data_default_init();
    pthread_t pid;
    pthread_create(&pid,NULL,hccast_sample_hostapd_start_main,NULL);
#endif
    return 0;
}

void *hccast_sample_hostapd_stop_main(void *pvParameters)
{
#ifdef WIFI_SUPPORT
    hccast_wifi_mgr_hostap_stop();
#endif
    return NULL;
}

int hccast_sample_hostapd_stop(int argc, char **argv)
{
#ifdef WIFI_SUPPORT
    pthread_t pid;
    pthread_create(&pid,NULL,hccast_sample_hostapd_stop_main,NULL);
#endif
    return 0;
}

void *hccast_sample_wifi_scan_main(void *pvParameters)
{
#ifdef WIFI_SUPPORT
    hccast_wifi_scan_result_t *scan_res;
    scan_res = calloc(sizeof(hccast_wifi_scan_result_t), 1);

    if(scan_res)
        hccast_wifi_mgr_scan(scan_res);
#endif
    return NULL;
}

int hccast_sample_wifi_scan(int argc, char **argv)
{
#ifdef WIFI_SUPPORT
    pthread_t pid;

    hccast_wifi_mgr_init(wifi_mgr_callback_func);
    sleep(1);
    pthread_create(&pid,NULL,hccast_sample_wifi_scan_main,NULL);
#endif
    return 0;
}

void *hccast_sample_wifi_connect_main(void *args)
{
#ifdef WIFI_SUPPORT
    hccast_wifi_ap_info_t *ap_wifi = (hccast_wifi_ap_info_t*)args;
    int index;
    printf("----------------------------wifi_connect_thread is running.-----------------------------\n");

    hccast_sample_stop_services();
    hccast_wifi_mgr_hostap_stop();
    hccast_wifi_mgr_enter_sta_mode();

    hccast_wifi_mgr_connect(ap_wifi);
    if (hccast_wifi_mgr_get_connect_status())
    {
        hccast_wifi_mgr_udhcpc_start();

        index = hccast_sample_data_mgr_check_ap_saved(ap_wifi);
        printf("ssid index: %d\n",index);
        if(index >= 0)//set the index ap to first.
        {
            hccast_sample_data_mgr_wifi_ap_delete(index);
            hccast_sample_data_mgr_wifi_ap_save(ap_wifi);
        }
        else
        {
            hccast_sample_data_mgr_wifi_ap_save(ap_wifi);
        }

        sleep(1);
    }
    else
    {
        hccast_wifi_mgr_disconnect_no_message();
        hccast_wifi_mgr_exit_sta_mode();
        usleep(50*1000);
        hccast_wifi_mgr_hostap_start();
        //hccast_mira_service_start();
    }

    free(ap_wifi);

#endif
    return NULL;
}

int hccast_sample_wifi_connect(int argc, char **argv)
{
#ifdef WIFI_SUPPORT
    pthread_t pid;

    hccast_wifi_mgr_init(wifi_mgr_callback_func);
    sleep(1);
    //printf("%d, %s %s\n",argc,argv[0],argv[1]);
    hccast_wifi_ap_info_t *ap_wifi = (hccast_wifi_ap_info_t *)calloc(sizeof(hccast_wifi_ap_info_t), 1);
    if(ap_wifi)
    {
        if(argc == 2)
        {
            strcpy(ap_wifi->ssid, argv[1]);
            ap_wifi->encryptMode = HCCAST_WIFI_ENCRYPT_MODE_NONE;
            ap_wifi->special_ap = 0;
        }
        else if(argc == 3)
        {
            ap_wifi->encryptMode = HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_AES;
            ap_wifi->special_ap = 0;
            strcpy(ap_wifi->ssid, argv[1]);
            strcpy(ap_wifi->pwd, argv[2]);
        }
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x10000);
        pthread_create(&pid,&attr,hccast_sample_wifi_connect_main,(void*)ap_wifi);
    }

#endif
    return 0;
}

void *hccast_sample_wifi_disconnect_main(void *args)
{
#ifdef WIFI_SUPPORT
    printf("----------------------------wifi_disconnect_thread is running.-----------------------------\n");
    hccast_wifi_mgr_disconnect();
#endif
    return NULL;
}

int hccast_sample_wifi_disconnect(int argc, char **argv)
{
#ifdef WIFI_SUPPORT
    pthread_t pid;
    pthread_create(&pid,NULL,hccast_sample_wifi_disconnect_main,NULL);
#endif
    return 0;
}

