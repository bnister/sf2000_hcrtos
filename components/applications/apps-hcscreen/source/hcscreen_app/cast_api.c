/**
 * @file cast_api.c
 * @author your name (you@domain.com)
 * @brief hichip cast api
 * @version 0.1
 * @date 2022-01-21
 *
 * @copyright Copyright (c) 2022
 *
 */


#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <time.h>

#include <hccast/hccast_scene.h>
#include <hcuapi/dis.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "com_api.h"
#include "osd_com.h"
#include "cast_api.h"
#include "data_mgr.h"

#define CAST_SERVICE_NAME               "HCcast"
#define CAST_AIRCAST_SERVICE_NAME       "HCcast"
#define CAST_DLNA_FIRENDLY_NAME         "HCcast"
#define CAST_MIRACAST_NAME              "HCcast"

#define UUID_HEADER "HCcast"

static bool m_is_demo = false;

#ifndef DLNA_SUPPORT
//implement fake functions
int hccast_dlna_service_uninit(void)
{
    return 0;
}

int hccast_dlna_service_start(void)
{
    return 0;
}

int hccast_dlna_service_stop(void)
{
    return 0;
}

#endif


#ifndef AIRCAST_SUPPORT
//implement fake functions
int hccast_air_ap_mirror_stat(void)
{
    return 0;
}
int hccast_air_ap_audio_stat(void)
{
    return 0;
}
int hccast_air_service_start(void)
{
    return 0;
}
int hccast_air_service_stop(void)
{
    return 0;
}
void hccast_air_mdnssd_start(void)
{}
void hccast_air_mdnssd_stop(void)
{}
void hccast_air_mediaplayer_2_aircast_event(int type, void *param)
{}
int hccast_air_ap_get_mirror_frame_num(void)
{
    return 0;
}
int hccast_air_service_is_start(void)
{
    return 0;
}

#endif

#ifndef MIRACAST_SUPPORT
//implement fake functions


int hccast_mira_service_start(void)
{
    return 0;
}

int hccast_mira_service_stop(void)
{
    return 0;
}
int hccast_mira_player_init(void)
{
    return 0;
}

int hccast_mira_get_stat(void)
{
    return 0;
}

int hccast_mira_service_uninit(void)
{
    return 0;
}

#endif


//#ifndef USBMIRROR_SUPPORT
#if 0
int hccast_um_init(void)
{
    return 0;
}

int hccast_um_deinit(void)
{
    return 0;
}

int hccast_um_param_set(hccast_um_param_t *param)
{
    (void)param;
    return 0;
}

int hccast_ium_start(char *uuid, hccast_um_cb event_cb)
{
    (void)uuid;
    (void)event_cb;
    return 0;
}

int hccast_ium_stop(void)
{
    return 0;
}

int hccast_aum_start(hccast_aum_param_t *param, hccast_um_cb event_cb)
{
    (void)param;
    (void)event_cb;
    return 0;
}

int hccast_aum_stop(void)
{
    return 0;
}

#endif

int cast_get_service_name(cast_type_t cast_type, char *service_name, int length)
{
    unsigned char mac_addr[6] = {0};
    char service_prefix[32] = CAST_SERVICE_NAME;

    snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_SERVICE_NAME);
    if (0 != api_get_mac_addr((char*)mac_addr))
        memset(mac_addr, 0xff, sizeof(mac_addr));

    if (CAST_TYPE_AIRCAST == cast_type)
        snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_AIRCAST_SERVICE_NAME);
    else if (CAST_TYPE_DLNA == cast_type)
        snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_DLNA_FIRENDLY_NAME);
    else if (CAST_TYPE_MIRACAST == cast_type)
        snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_MIRACAST_NAME);

    snprintf(service_name, length, "%s-%02x%02x%02x",
             service_prefix, mac_addr[3]&0xFF, mac_addr[4]&0xFF, mac_addr[5]&0xFF);

    return 0;
}


#ifdef USBMIRROR_SUPPORT

void cast_um_set_dis_zoom(hccast_um_zoom_info_t *um_zoom_info)
{
    av_area_t src_rect;
    av_area_t dst_rect;

    memcpy(&src_rect, &um_zoom_info->src_rect, sizeof(av_area_t));
    memcpy(&dst_rect, &um_zoom_info->dst_rect, sizeof(av_area_t));

    cast_api_set_dis_zoom(&src_rect, &dst_rect, DIS_SCALE_ACTIVE_IMMEDIATELY);
}

static char m_ium_uuid[40] = {0};
static void ium_event_process_cb(int event, void *param1, void *param2)
{
    control_msg_t ctl_msg = {0};

    if (event != HCCAST_IUM_EVT_GET_FLIP_MODE)
        printf("ium event: %d\n", event);

    switch (event)
    {
        case HCCAST_IUM_EVT_DEVICE_ADD:
            break;
        case HCCAST_IUM_EVT_DEVICE_REMOVE:
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_DEVICE_REMOVE;
            break;
        case HCCAST_IUM_EVT_MIRROR_START:
            printf("%s(), line:%d. HCCAST_IUM_EVT_MIRROR_START\n", __func__, __LINE__);
            hccast_scene_switch(HCCAST_SCENE_IUMIRROR);
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_START;
            api_osd_off_time(5000);
            api_logo_off();
            break;
        case HCCAST_IUM_EVT_MIRROR_STOP:
            printf("%s(), line:%d. HCCAST_IUM_EVT_MIRROR_STOP\n", __func__, __LINE__);
            if(hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR)
            {
                hccast_scene_reset(HCCAST_SCENE_IUMIRROR,HCCAST_SCENE_NONE);
            }
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_STOP;
            break;
        case HCCAST_IUM_EVT_SAVE_PAIR_DATA: //param1: buf; param2: length
            break;
        case HCCAST_IUM_EVT_GET_PAIR_DATA: //param1: buf; param2: length
            break;
        case HCCAST_IUM_EVT_NEED_USR_TRUST:
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_NEED_TRUST;
            break;
        case HCCAST_IUM_EVT_USR_TRUST_DEVICE:
            break;
        case HCCAST_IUM_EVT_CREATE_CONN_FAILED:
            break;
        case HCCAST_IUM_EVT_CANNOT_GET_AV_DATA:
            break;
        case HCCAST_IUM_EVT_UPG_DOWNLOAD_PROGRESS: //param1: data len; param2: file len
            break;
        case HCCAST_IUM_EVT_GET_UPGRADE_DATA: //param1: hccast_ium_upg_bo_t
            break;
        case HCCAST_IUM_EVT_SAVE_UUID:
            break;
        case HCCAST_IUM_EVT_CERT_INVALID:
            m_is_demo = true;
            printf("[%s],line:%d. HCCAST_IUM_EVT_CERT_INVALID\n",__func__, __LINE__);

            break;
        case HCCAST_IUM_EVT_GET_FLIP_MODE:
        {
            int flip_mode;
            int rotate;
            int flip;
            api_get_flip_info(&rotate, &flip);
            flip_mode = (rotate & 0xffff) << 16 | (flip & 0xffff);
            *(int*)param1 = flip_mode;
            break;
        }

        default:
            break;
    }

    if (0 != ctl_msg.msg_type)
    {
        api_control_send_msg(&ctl_msg);
    }
}

static void aum_event_process_cb(int event, void *param1, void *param2)
{
    control_msg_t ctl_msg = {0};

    if (HCCAST_AUM_EVT_GET_FLIP_MODE != event)
        printf("aum event: %d\n", event);
    
    switch (event)
    {
        case HCCAST_AUM_EVT_DEVICE_ADD:
            printf("%s(), line:%d. HCCAST_AUM_EVT_DEVICE_ADD\n", __func__, __LINE__);
            hccast_scene_switch(HCCAST_SCENE_AUMIRROR);
            break;
        case HCCAST_AUM_EVT_DEVICE_REMOVE:
            break;
        case HCCAST_AUM_EVT_MIRROR_START:
            printf("%s(), line:%d. HCCAST_AUM_EVT_MIRROR_START\n", __func__, __LINE__);
            //hccast_scene_switch(HCCAST_SCENE_AUMIRROR);
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_START;
            api_osd_off_time(5000);
            api_logo_off();
            break;
        case HCCAST_AUM_EVT_MIRROR_STOP:
            printf("%s(), line:%d. HCCAST_AUM_EVT_MIRROR_STOP\n", __func__, __LINE__);
            if(hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR)
            {
                hccast_scene_reset(HCCAST_SCENE_AUMIRROR,HCCAST_SCENE_NONE);
            }
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_STOP;
            break;
        case HCCAST_AUM_EVT_IGNORE_NEW_DEVICE:
            break;
        case HCCAST_AUM_EVT_SERVER_MSG:
            break;
        case HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS:
            break;
        case HCCAST_AUM_EVT_GET_UPGRADE_DATA:
            break;
        case HCCAST_AUM_EVT_SET_SCREEN_ROTATE:
            break;
        case HCCAST_AUM_EVT_SET_AUTO_ROTATE:
            break;
        case HCCAST_AUM_EVT_SET_FULL_SCREEN:
            break;
        case HCCAST_AUM_EVT_GET_FLIP_MODE:
        {
            int flip_mode;
            int rotate;
            int flip;
            api_get_flip_info(&rotate, &flip);
            flip_mode = (rotate & 0xffff) << 16 | (flip & 0xffff);
            *(int*)param1 = flip_mode;
            break;
        }
        default:
            break;
    }
    if (0 != ctl_msg.msg_type)
    {
        api_control_send_msg(&ctl_msg);
    }
}

int cast_usb_mirror_init(void)
{
    hccast_um_param_t um_param = {0};

    hccast_um_init();
    if (hccast_um_init() < 0)
    {
        printf("%s(), line:%d. hccast_um_init() fail!\n", __func__, __LINE__);
        return API_FAILURE;
    }

    if (data_mgr_cast_rotation_get())
        um_param.screen_rotate_en = 1;
    else
        um_param.screen_rotate_en = 0;

    um_param.screen_rotate_auto = 1;
    um_param.full_screen_en = 1;
    hccast_um_param_set(&um_param);

    hccast_ium_init(ium_event_process_cb);

    return API_SUCCESS;
}

int cast_usb_mirror_deinit(void)
{
    if (hccast_um_deinit() < 0)
        return API_FAILURE;
    else
        return API_SUCCESS;
}

int cast_usb_mirror_start(void)
{
    int ret;
    hccast_aum_param_t aum_param = {0};

    ret = hccast_ium_start(m_ium_uuid, ium_event_process_cb);
    if (ret)
    {
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);
        return API_FAILURE;
    }

    strcat(aum_param.product_id, "HCT-AT01");
    strcat(aum_param.fw_url, "http://119.3.89.190:8080/apk/elfcast-HCT-AT01.json");
    strcat(aum_param.apk_url, "http://119.3.89.190:8080/apk/elfcast.apk");
    strcat(aum_param.aoa_desc, "ElfCast-Screen_Mirror");
    aum_param.fw_version = 0;
    ret = hccast_aum_start(&aum_param, aum_event_process_cb);
    if (ret)
    {
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);
        return API_FAILURE;
    }

    return API_SUCCESS;
}

int cast_usb_mirror_stop(void)
{
    hccast_aum_stop();
    hccast_ium_stop();
    return API_SUCCESS;
}
#endif

int cast_init(void)
{
#ifdef DLNA_SUPPORT
    hccast_dlna_service_init(hccast_dlna_callback_func);
#endif

#ifdef MIRACAST_SUPPORT
    hccast_mira_service_init(hccast_mira_callback_func);

    hccast_mira_res_e res = HCCAST_MIRA_RES_1080P30; // HCCAST_MIRA_RES_720P30
    hccast_mira_service_set_resolution(res);
#endif

#ifdef AIRCAST_SUPPORT
    hccast_air_service_init(hccast_air_callback_event);
    hccast_air_set_resolution(1920, 1080, 60);
#endif

    hccast_scene_init();
    api_set_aspect_mode(1, 3,DIS_SCALE_ACTIVE_IMMEDIATELY);//16:9 as default.

    return API_SUCCESS;
}

int cast_deinit(void)
{
#ifdef DLNA_SUPPORT
    hccast_dlna_service_stop();
    hccast_dlna_service_uninit();
#endif

#ifdef MIRACAST_SUPPORT
    hccast_mira_service_stop();
    hccast_mira_service_uninit();
#endif

#ifdef AIRCAST_SUPPORT
    hccast_air_service_stop();
#endif

#ifdef USBMIRROR_SUPPORT
    cast_usb_mirror_stop();
    cast_usb_mirror_deinit();
#endif

    return API_SUCCESS;
}

/*
static int cast_get_wifi_mac(unsigned char *mac)
{
    int ret = 0;
    int sock, if_count, i;
    struct ifconf ifc;
    struct ifreq ifr[10];

    if (!mac)
    {
        return 0;
    }

    memset(&ifc, 0, sizeof(struct ifconf));

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    ifc.ifc_len = 10 * sizeof(struct ifreq);
    ifc.ifc_buf = ifr;
    ioctl(sock, SIOCGIFCONF, &ifc);

    if_count = ifc.ifc_len / sizeof(struct ifreq);

    for (i = 0; i < if_count; i ++)
    {
        if (ioctl(sock, SIOCGIFHWADDR, &ifr[i]) == 0)
        {
            memcpy(mac, ifr[i].ifr_hwaddr.sa_data, 6);
            if (!strcmp(ifr[i].ifr_name, "wlan0"))
            {
                return 1;
            }
        }
    }

    return 0;
}
*/

void cast_api_set_dis_zoom(av_area_t *src_rect,
                         av_area_t *dst_rect,
                         dis_scale_avtive_mode_e active_mode)
{
    int dis_fd = open("/dev/dis" , O_WRONLY);

    if (dis_fd >= 0) 
    {
        struct dis_zoom dz;
        dz.distype = DIS_TYPE_HD;
        dz.layer = DIS_LAYER_MAIN;
        dz.active_mode = active_mode;
        memcpy(&dz.src_area, src_rect, sizeof(struct av_area));
        memcpy(&dz.dst_area, dst_rect, sizeof(struct av_area));
        ioctl(dis_fd, DIS_SET_ZOOM, &dz);
        close(dis_fd);
    }
}


#ifdef DLNA_SUPPORT
int hccast_dlna_callback_func(hccast_dlna_event_e event, void* in, void* out)
{
    log(DEMO, INFO, "[%s] event: %d", __func__,event);
    char *str_tmp = NULL;
    control_msg_t ctl_msg = {0};

    switch (event)
    {
        case HCCAST_DLNA_GET_DEVICE_NAME:
        {
            printf("[%s]HCCAST_DLNA_GET_DEVICE_NAME\n",__func__);
            if (in)
            {
                str_tmp = data_mgr_get_device_name();
                if (str_tmp)
                {
                    sprintf((char *)in, "%s_dlna", str_tmp);
                    printf("[%s]HCCAST_DLNA_GET_DEVICE_NAME:%s\n",__func__, str_tmp);
                }
            }
            break;
        }
        case HCCAST_DLNA_GET_HOSTAP_STATE:
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
            *(int*)in = hccast_wifi_mgr_get_hostap_status();
#endif
#endif
            break;
        case HCCAST_DLNA_HOSTAP_MODE_SKIP_URL:
            ctl_msg.msg_type = MSG_TYPE_DLNA_HOSTAP_SKIP_URL;
            break;
        default:
            break;
    }

    if (0 != ctl_msg.msg_type)
        api_control_send_msg(&ctl_msg);

    return 0;
}
#endif

#ifdef MIRACAST_SUPPORT

void cast_mira_set_dis_zoom(hccast_mira_zoom_info_t *mira_zoom_info)
{
    av_area_t src_rect;
    av_area_t dst_rect;
    int dis_active_mode;

    memcpy(&src_rect, &mira_zoom_info->src_rect, sizeof(av_area_t));
    memcpy(&dst_rect, &mira_zoom_info->dst_rect, sizeof(av_area_t));
    dis_active_mode = mira_zoom_info->dis_active_mode;

    cast_api_set_dis_zoom(&src_rect, &dst_rect, dis_active_mode);
}

int hccast_mira_callback_func(hccast_mira_event_e event, void* in, void* out)
{
    control_msg_t ctl_msg = {0};
    char *str_tmp = NULL;
    app_data_t * app_data = data_mgr_app_get();
    static int vdec_first_show = 0;
    static int ui_logo_close = 0;

#if 0
    if(event != HCCAST_MIRA_GET_MIRROR_ROTATION)
    {
        log(DEMO, INFO, "[%s] event: %d", __func__, event);
    }
#endif

    switch (event)
    {
        case HCCAST_MIRA_GET_DEVICE_NAME:
        {
            if (in)
            {
                str_tmp = data_mgr_get_device_name();
                if (str_tmp)
                {
                    sprintf((char *)in, "%s_mira", str_tmp);
                    log(DEMO, INFO, "HCCAST_MIRA_GET_DEVICE_NAME:%s\n", str_tmp);
                }
            }
            break;
        }
        case HCCAST_MIRA_SSID_DONE:
        {
            log(DEMO, DEBUG, "[%s]HCCAST_MIRA_SSID_DONE\n",__func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_SSID_DONE;
            break;
        }
        case HCCAST_MIRA_GET_CUR_WIFI_INFO:
        {
            log(DEMO, DEBUG, "[%s]HCCAST_MIRA_GET_CUR_WIFI_INFO\n",__func__);
            char cur_ssid[WIFI_MAX_SSID_LEN] = {0};
            hccast_wifi_ap_info_t *cur_ap;
            hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
            cur_ap = data_mgr_get_wifi_info(cur_ssid);
            if(cur_ap)
                memcpy(in,cur_ap,sizeof(hccast_wifi_ap_info_t));
            else
                memcpy(in, cur_ssid,sizeof(hccast_wifi_ap_info_t));
            break;
        }
        case HCCAST_MIRA_CONNECT:
        {
            //miracast connect start
            log(DEMO, DEBUG, "[%s]HCCAST_MIRA_CONNECT\n",__func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_CONNECTING;
            break;
        }
        case HCCAST_MIRA_CONNECTED:
        {
            //miracast connect success
            log(DEMO, DEBUG, "[%s]HCCAST_MIRA_CONNECTED\n",__func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_CONNECTED;
            break;
        }
        case HCCAST_MIRA_DISCONNECT:
        {
            //miracast disconnect
            log(DEMO, DEBUG, "[%s]HCCAST_MIRA_DISCONNECT\n",__func__);
            break;
        }

        case HCCAST_MIRA_START_DISP:
        {
            //miracast start
            printf("[%s] HCCAST_MIRA_START_DISP [%d:%d]\n",__func__, vdec_first_show, ui_logo_close);
#ifdef __linux__            
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_START;
#else 
            ui_logo_close = 1;
            api_logo_off2(0,0);
#endif            
            break;
        }
        case HCCAST_MIRA_START_FIRST_FRAME_DISP:
        {
            printf("[%s] HCCAST_MIRA_START_FIRST_FRAME_DISP [%d:%d]\n",__func__, vdec_first_show, ui_logo_close);
#ifndef __linux__    
            vdec_first_show = 1;
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_START;
#endif            
            break;
        }
        case HCCAST_MIRA_STOP_DISP:
        {
            //miracast stop
            printf("[%s] HCCAST_MIRA_STOP_DISP [%d:%d]\n",__func__, vdec_first_show, ui_logo_close);
            if((vdec_first_show == 0) && (ui_logo_close == 1))
            {   
                api_logo_show(NULL);
                ui_logo_close = 0;
                return 0;
            }
            vdec_first_show = 0;
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_STOP;
            break;
        }
        case HCCAST_MIRA_GET_MIRROR_ROTATION:
        {
            *(int*)in = app_data->mirror_rotation;
            break;
        }
        case HCCAST_MIRA_GET_MIRROR_VSCREEN_AUTO_ROTATION:
        {
            *(int*)in = app_data->mirror_vscreen_auto_rotation;
            break;
        }
        case HCCAST_MIRA_GET_FLIP_MODE:
        {
            int flip_mode;
            int rotate;
            int flip;
            api_get_flip_info(&rotate, &flip);
            flip_mode = (rotate & 0xffff) << 16 | (flip & 0xffff);
            *(int*)in = flip_mode;
            break;
        }
        case HCCAST_MIRA_GET_MIRROR_FULL_VSCREEN:
            *(int*)in = 1;
            break;
        case HCCAST_MIRA_SET_DIS_ZOOM_INFO:
            cast_mira_set_dis_zoom(in); 
            break;

        default:
            break;

    }

    if (0 != ctl_msg.msg_type)
    {
        api_control_send_msg(&ctl_msg);
    }

    if (MSG_TYPE_CAST_MIRACAST_STOP == ctl_msg.msg_type)
    {
        //printf("[%s] wait cast root start tick: %d\n",__func__,(int)time(NULL));
        bool win_cast_root_wait_open(uint32_t timeout);
        win_cast_root_wait_open(20000);
        //printf("[%s] wait cast root end tick: %d\n",__func__,(int)time(NULL));
    }

    if (MSG_TYPE_CAST_MIRACAST_START == ctl_msg.msg_type)
    {
        //printf("[%s] wait cast play start tick: %d\n",__func__,(int)time(NULL));
        bool win_cast_play_wait_open(uint32_t timeout);
        win_cast_play_wait_open(20000);
        //printf("[%s] wait cast play end tick: %d\n",__func__,(int)time(NULL));
#if CASTING_CLOSE_FB_SUPPORT	
        api_osd_show_onoff(false);
#endif
    }

    return 0;
}
#endif

#ifdef AIRCAST_SUPPORT
int hccast_air_callback_event(hccast_air_event_e event, void* in, void* out)
{
    control_msg_t ctl_msg = {0};
    app_data_t * app_data = data_mgr_app_get();

    switch (event)
    {
        case HCCAST_AIR_GET_SERVICE_NAME:
            printf("[%s]HCCAST_AIR_GET_SERVICE_NAME\n",__func__);
            sprintf((char *)in, "%s_itv", data_mgr_get_device_name());

            break;

        case HCCAST_AIR_GET_NETWORK_DEVICE:
            printf("[%s]HCCAST_AIR_GET_NETWORK_DEVICE\n",__func__);
            sprintf((char *)in, "%s", "wlan0");
            break;
        case HCCAST_AIR_GET_MIRROR_RESOLUTION:
            printf("[%s]HCCAST_AIR_GET_MIRROR_RESOLUTION\n",__func__);
            //*(int*)in = app_data->res;
            break;
        case HCCAST_AIR_GET_MIRROR_FRAME:
            printf("[%s]HCCAST_AIR_GET_MIRROR_FRAME\n",__func__);
            //*(int*)in = app_data->res;
            break;
        case HCCAST_AIR_GET_MIRROR_MODE:
            printf("[%s]HCCAST_AIR_GET_MIRROR_MODE\n",__func__);
            *(int*)in = app_data->aircast_mode;
            break;
        case HCCAST_AIR_GET_NETWORK_STATUS:
#ifdef WIFI_SUPPORT
            printf("[%s]HCCAST_AIR_GET_NETWORK_STATUS\n",__func__);
            *(int*)in = hccast_wifi_mgr_get_hostap_status();
#endif
            break;
        case HCCAST_AIR_MIRROR_START:
            printf("[%s]HCCAST_AIR_MIRROR_START\n",__func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRMIRROR_START;
            break;
        case HCCAST_AIR_MIRROR_STOP:
            printf("[%s]HCCAST_AIR_MIRROR_STOP\n",__func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRMIRROR_STOP;
            break;
        case HCCAST_AIR_AUDIO_START:
            printf("[%s]HCCAST_AIR_AUDIO_START\n",__func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRCAST_AUDIO_START;
            break;
        case HCCAST_AIR_AUDIO_STOP:
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRCAST_AUDIO_STOP;
            printf("[%s]HCCAST_AIR_AUDIO_STOP\n",__func__);
            break;
        case HCCAST_AIR_INVALID_CERT:
            ctl_msg.msg_type = MSG_TYPE_AIR_INVALID_CERT;
            m_is_demo = true;
            printf("[%s],line:%d. HCCAST_AIR_INVALID_CERT\n",__func__, __LINE__);
            break;
        case HCCAST_AIR_CHECK_4K_MODE:
            if(data_mgr_de_tv_sys_get() < TV_LINE_4096X2160_30)
            {
                *(int*)in = 0;
                printf("[%s] NOT 4K MODE, tv_sys:%d\n",__func__,data_mgr_de_tv_sys_get());

            }
            else
            {
                *(int*)in = 1;
                printf("[%s] NOW IS 4K MODE, tv_sys:%d\n",__func__,data_mgr_de_tv_sys_get());
            }

            break;
        case HCCAST_AIR_HOSTAP_MODE_SKIP_URL:
            ctl_msg.msg_type = MSG_TYPE_AIR_HOSTAP_SKIP_URL;
            printf("[%s]HCCAST_AIR_HOSTAP_MODE_SKIP_URL\n",__func__);
            break;
        case HCCAST_AIR_BAD_NETWORK:
            ctl_msg.msg_type = MSG_TYPE_AIR_MIRROR_BAD_NETWORK;
            printf("[%s]HCCAST_AIR_BAD_NETWORK\n",__func__);
            break;
        case HCCAST_AIR_GET_MIRROR_ROTATION:
            //printf("[%s]HCCAST_AIR_GET_MIRROR_ROTATION\n",__func__);
            *(int*)in = app_data->mirror_rotation;
            break;
        case HCCAST_AIR_GET_MIRROR_VSCREEN_AUTO_ROTATION:
            //printf("[%s]HCCAST_AIR_GET_MIRROR_ROTATION\n",__func__);
            *(int*)in = app_data->mirror_vscreen_auto_rotation;
            break;
        case HCCAST_AIR_GET_FLIP_MODE:
        {
            int flip_mode;
            int rotate;
            int flip;
            api_get_flip_info(&rotate, &flip);
            flip_mode = (rotate & 0xffff) << 16 | (flip & 0xffff);
            *(int*)in = flip_mode;
            break;
        }
        case HCCAST_AIR_URL_ENABLE_SET_DEFAULT_VOL:
            *(int*)in = 1;
            break; 
        case HCCAST_AIR_GET_MIRROR_FULL_VSCREEN:
            *(int*)in = 1;
            break;
        default:
            break;
    }

    if (0 != ctl_msg.msg_type)
        api_control_send_msg(&ctl_msg);

    if (MSG_TYPE_CAST_AIRMIRROR_STOP == ctl_msg.msg_type)
    {
        //While dlna preempt air mirror(air mirror->air play), air mirror stop and dlna start,
        //sometime it is still in cast play UI(not exit to win root UI),
        //the next dlna url play is starting, then the UI/logo may block the dlna playing.
        //So here exit callback function wait for win cast root UI opening
        printf("[%s] wait cast root start tick: %d\n",__func__,(int)time(NULL));
        bool win_cast_root_wait_open(uint32_t timeout);
        win_cast_root_wait_open(20000);
        printf("[%s] wait cast root end tick: %d\n",__func__,(int)time(NULL));
    }

    if (MSG_TYPE_CAST_AIRMIRROR_START == ctl_msg.msg_type)
    {
        printf("[%s] wait cast play start tick: %d\n",__func__,(int)time(NULL));
        bool win_cast_play_wait_open(uint32_t timeout);
        win_cast_play_wait_open(20000);
        printf("[%s] wait cast play end tick: %d\n",__func__,(int)time(NULL));
#if CASTING_CLOSE_FB_SUPPORT	
        api_osd_show_onoff(false);
#endif
    }

    return 0;
}
#endif

void cast_restart_services()
{
    if(hccast_get_current_scene() != HCCAST_SCENE_NONE)
    {
        hccast_scene_switch(HCCAST_SCENE_NONE);
    }

    printf("[%s]  begin restart services.\n",__func__);

    hccast_dlna_service_stop();
    hccast_dlna_service_start();

    hccast_air_service_stop();
    hccast_air_service_start();

}

void restart_air_service_by_hdmi_change(void)
{
#ifdef AIRCAST_SUPPORT
    int cur_scene = 0;
    cur_scene = hccast_get_current_scene();
    if((cur_scene != HCCAST_SCENE_AIRCAST_PLAY) && (cur_scene != HCCAST_SCENE_AIRCAST_MIRROR))
    {
        if(hccast_air_service_is_start())
        {
            hccast_air_service_stop();
            hccast_air_service_start();
        }
    }
#endif
}

bool cast_is_demo(void)
{
    return m_is_demo;
}

