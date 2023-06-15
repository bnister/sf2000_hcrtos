/*
network_api.c: use for network, include wifi, etc

 */

#include "app_config.h"

#ifdef WIFI_SUPPORT

#include <math.h>
#include <stdio.h> //printf()
#include <stdlib.h>
#include <string.h> //memcpy()
#include <unistd.h> //usleep()
#include <pthread.h>

#include <hccast/hccast_dhcpd.h>
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_httpd.h>

#include <hccast/hccast_media.h>
#include <hccast/hccast_net.h>
#include <cjson/cJSON.h>

#include <hccast/hccast_net.h>
#include "network_api.h"
#include <hcuapi/dis.h>
#include "cast_api.h"
#include "app_config.h"
#include "cast_log.h"
#include "../wifi/wifi.h"
#include <arpa/inet.h>

#define UUID_HEADER "HCcast"

#include "com_api.h"
#include "factory_setting.h"
#include "tv_sys.h"

/**********************************************************************
NETWORK_UPGRADE_URL:
config name: %s_upgrade_config.json == produdct_hcscreen_rtos_upgrade_config.jsonp
example:http://119.3.89.190:8080/upgrade_package/HC15A210_hcscreen_rtos_upgrade_config.jsonp
**********************************************************************/
#ifdef __linux__
//#define NETWORK_UPGRADE_URL "http://119.3.89.190:8080/upgrade_package/%s_hcprojector_linux_upgrade_config.jsonp"
#define NETWORK_UPGRADE_URL "http://test/%s_test.jsonp"
#else
//#define NETWORK_UPGRADE_URL "http://119.3.89.190:8080/upgrade_package/%s_hcprojector_rtos_upgrade_config.jsonp"
#define NETWORK_UPGRADE_URL "http://test/%s_test.jsonp"
#endif


wifi_config_t m_wifi_config = {0};

static char g_connecting_ssid[WIFI_MAX_SSID_LEN] = {0};

static int m_probed_wifi_module = 0;
static int hostap_connect_count = 0;
static int factary_init = 0;
static int hostap_discover_ok = 0;
static int m_wifi_connect_status = 0;

static int wifi_mgr_callback_func(hccast_wifi_event_e event, void* in, void* out);

#ifdef HTTPD_SERVICE_SUPPORT
static int httpd_callback_func(hccast_httpd_event_e event, void* in, void* out);
#endif

static void media_callback_func(hccast_media_event_e msg_type, void* param);
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
static void hostap_config_init(void);
#endif
#endif

#ifdef AUTO_HTTP_UPGRADE_SUPPORT
static char m_http_upgrade_check_first = 0;
int network_upgrade_start();
#endif

static void network_probed_wifi(void);

static pthread_mutex_t g_wifi_status_mutex = PTHREAD_MUTEX_INITIALIZER;


char *app_get_connecting_ssid()
{
    return g_connecting_ssid;
}

int app_get_wifi_connect_status()
{
    int status;
    pthread_mutex_lock(&g_wifi_status_mutex);
    status = m_wifi_connect_status;
    pthread_mutex_unlock(&g_wifi_status_mutex);
    return status;
}

void app_set_wifi_connect_status(int status)
{
    pthread_mutex_lock(&g_wifi_status_mutex);
    m_wifi_connect_status = status;
    pthread_mutex_unlock(&g_wifi_status_mutex);
}

void hccast_start_services(void)
{
    if (hccast_get_current_scene() != HCCAST_SCENE_NONE)
    {
        hccast_scene_switch(HCCAST_SCENE_NONE);
    }

    printf("[%s]  begin start services.\n", __func__);

    hccast_dlna_service_stop();
    hccast_dlna_service_start();
    hccast_air_service_stop();
    hccast_air_service_start();
    hccast_mira_service_start();
}

void hccast_stop_services(void)
{
    printf("[%s]  begin stop services.\n", __func__);

    hccast_dlna_service_stop();
    hccast_air_service_stop();
    hccast_mira_service_stop();
}


static void hccast_ap_dlna_aircast_start(void)
{
    hccast_dlna_service_start();
    hccast_air_service_start();
}

static void hccast_ap_dlna_aircast_stop(void)
{
    hccast_dlna_service_stop();
    hccast_air_service_stop();
}

bool app_wifi_connect_status_get(void){
    return m_wifi_config.bConnected;
}

#ifdef WIFI_SUPPORT
static int wifi_mgr_callback_func(hccast_wifi_event_e event, void* in, void* out)
{
    log(DEMO, INFO, "[%s] event: %d", __func__,event);
    control_msg_t ctl_msg = {0};

    switch (event)
    {
        case HCCAST_WIFI_SCAN:
        {
            //WiFi start scan...
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_SCANNING;
            api_control_send_msg(&ctl_msg);
            break;
        }
        case HCCAST_WIFI_SCAN_RESULT:
        {
            hccast_wifi_scan_result_t *res = (hccast_wifi_scan_result_t*)out;

            log(DEMO, INFO, "AP NUM: %d\n***********", res->ap_num);
            int j;
            wifi_list_set_zero();
            for (int i = 0; i < res->ap_num; i++)
            {
                log(DEMO, INFO, "ssid: %s, quality: %d", res->apinfo[i].ssid, res->apinfo[i].quality);
                
                if((j=sysdata_get_wifi_index_by_ssid(res->apinfo[i].ssid)) < 0){
                     wifi_list_add(&res->apinfo[i]);
                }else{
                    if(saved_wifi_sig_strength_max_id<0 && sysdata_wifi_ap_get_auto(j)){
                        saved_wifi_sig_strength_max_id = j;
                    }
                    
                    hccast_wifi_ap_info_t *info = sysdata_get_wifi_info_by_index(j);
                    if(info){
                        info->quality = res->apinfo[i].quality;
                        info->encryptMode = res->apinfo[i].encryptMode;
                    }
                    if(saved_wifi_sig_strength_max_id>=0){
                        info = sysdata_get_wifi_info_by_index(saved_wifi_sig_strength_max_id);
                        if(info && info->quality < res->apinfo[i].quality){
                            if(sysdata_wifi_ap_get_auto(j)){
                                saved_wifi_sig_strength_max_id = j;
                            }                            
                        }
                    }
                }
               
            }

            log(DEMO, INFO, "\n***********");
            
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_SCAN_DONE;
            api_control_send_msg(&ctl_msg);
			
			//Do not free here, because the res buffer is not malloc here.
			//The buffer should be free after using hccast_wifi_mgr_scan(res)
			//free(res);

            break;
        }
        case HCCAST_WIFI_CONNECT:
        {
            memset(m_wifi_config.local_ip, 0, sizeof(m_wifi_config.local_ip));
            if (in!=NULL)
            {
                memcpy(g_connecting_ssid, (char*)in, strlen((char*)in));
            }
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTING;
            api_control_send_msg(&ctl_msg);
            break;
        }
        case HCCAST_WIFI_CONNECT_SSID:
        {
            log(DEMO, INFO, "SSID: %s", (char*)out);
            break;
        }
        case HCCAST_WIFI_CONNECT_RESULT: //station
        {
            hccast_udhcp_result_t* result = (hccast_udhcp_result_t*) out;
            if (result)
            {
                log(DEMO, INFO, "state: %d", result->stat);
                printf("wifi connect result: %s!\n", result->stat ? "OK" : "fail");
                if (result->stat)
                {
                    log(DEMO, INFO, "ifname: %s", result->ifname);
                    log(DEMO, INFO, "ip addr: %s", result->ip);
                    log(DEMO, INFO, "mask addr: %s", result->mask);
                    log(DEMO, INFO, "gw addr: %s", result->gw);
                    log(DEMO, INFO, "dns: %s", result->dns);

                    hccast_net_ifconfig(result->ifname, result->ip, result->mask, result->gw);
                    hccast_net_set_dns(result->ifname, result->dns);

					if((hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
					{
						printf("Cur scene is doing USB MIRROR\n");
					}
					else
					{
                        if (network_service_enable_get()){
                            printf("hccast_start_services\n");
                            hccast_start_services();
                        }
						  
					}
			
                    m_wifi_config.sta_ip_ready = true;
                    m_wifi_config.bConnectedByPhone = false;
                    m_wifi_config.host_ap_ip_ready = false;
                    m_wifi_config.bConnected = true;

                    strncpy(m_wifi_config.local_ip, result->ip, MAX_IP_STR_LEN);
                    wifi_get_udhcp_result(result);
                    ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTED;
                    api_control_send_msg(&ctl_msg);
        #ifdef AUTO_HTTP_UPGRADE_SUPPORT
                    if(m_http_upgrade_check_first == 0)
                    {
                        m_http_upgrade_check_first = 1;
                        network_upgrade_start();
                    }
        #endif

                }
                else
                {
                    hccast_wifi_mgr_udhcpc_stop();
                    hccast_wifi_mgr_disconnect_no_message();

                    m_wifi_config.bConnected = false;  
                    m_wifi_config.bConnectedByPhone = false;
                    m_wifi_config.host_ap_ip_ready = false;
                    m_wifi_config.sta_ip_ready = false;            
                    memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
                    memset(m_wifi_config.local_ip, 0, sizeof(m_wifi_config.local_ip));

                    usleep(50*1000);
                    ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL;
                    api_control_send_msg(&ctl_msg);

					if((hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
					{
						printf("Cur scene is doing USB MIRROR\n");
					}
					else
					{
                        if (network_service_enable_get())
                        {    
                            hccast_mira_service_start();
                            app_wifi_switch_work_mode(WIFI_MODE_AP);
                        }

					}	
                    m_wifi_config.sta_ip_ready = false;
                    m_wifi_config.bConnected = false;
                    m_wifi_config.bConnectedByPhone = false;

                }
            }

            break;
        }

        case HCCAST_WIFI_DISCONNECT:
        {
            m_wifi_config.bConnected = false;  
            m_wifi_config.bConnectedByPhone = false;
            m_wifi_config.host_ap_ip_ready = false;
            m_wifi_config.sta_ip_ready = false;            
            memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
            memset(m_wifi_config.local_ip, 0, sizeof(m_wifi_config.local_ip));
            if (hccast_wifi_mgr_p2p_get_connect_stat() == 0)
            {
                printf("%s Wifi has been disconnected, beging change to host ap mode\n",__func__);
                hccast_media_stop();
                hccast_ap_dlna_aircast_stop();
            #ifdef __HCRTOS__
                hccast_mira_service_stop();
            #endif
        
                if (network_service_enable_get()){
                    app_wifi_switch_work_mode(WIFI_MODE_AP);
			#ifndef __linux__
        	        hccast_mira_service_start();
			#endif
				}
            }
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_DISCONNECTED;
            api_control_send_msg(&ctl_msg);
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

                hostap_discover_ok = 1;

                log(WIFI, INFO, "addr: %s", inet_ntoa(tmp_addr));

                strncpy(m_wifi_config.connected_phone_ip, inet_ntoa(tmp_addr), MAX_IP_STR_LEN);


            }
            m_wifi_config.sta_ip_ready = false;
            m_wifi_config.bConnected = false;  
            //if (network_service_enable_get())
            //    hccast_ap_dlna_aircast_start();

            break;
        }

        default:
            break;
    }

    return 0;

}


char *app_wifi_local_ip_get(void)
{
    return m_wifi_config.local_ip;
}

char *app_wifi_connected_phone_ip_get(void)
{
    return m_wifi_config.connected_phone_ip;
}

static void *wifi_disconnect_thread(void *args)
{
    printf("----------------------------wifi_disconnect_thread is running.-----------------------------\n");
    hccast_wifi_mgr_udhcpc_stop();
    hccast_wifi_mgr_disconnect();
    return NULL;
}

static void *wifi_connect_thread(void *args)
{
    hccast_wifi_ap_info_t *ap_wifi = (hccast_wifi_ap_info_t*)args;
    int index;
    printf("----------------------------wifi_connect_thread is running.-----------------------------\n");

    hccast_stop_services();

    app_wifi_switch_work_mode(WIFI_MODE_STATION);

    memcpy(g_connecting_ssid, ap_wifi->ssid, sizeof(g_connecting_ssid));
    hccast_wifi_mgr_connect(ap_wifi);
    if (hccast_wifi_mgr_get_connect_status())
    {
        hccast_wifi_mgr_udhcpc_stop();
        api_sleep_ms(100);
        hccast_wifi_mgr_udhcpc_start();

        index = sysdata_check_ap_saved(ap_wifi);
        printf("ssid index: %d\n",index);
        if(index >= 0)//set the index ap to first.
        {
             sysdata_wifi_ap_delete(index);
        }

        sysdata_wifi_ap_save(ap_wifi);

        projector_sys_param_save();
        sleep(1);
    }
    else
    {
        hccast_wifi_mgr_udhcpc_stop();
        hccast_wifi_mgr_disconnect_no_message();
        m_wifi_config.bConnected = false;  
        m_wifi_config.bConnectedByPhone = false;
        m_wifi_config.host_ap_ip_ready = false;
        m_wifi_config.sta_ip_ready = false;
        memset(m_wifi_config.local_ip, 0, MAX_IP_STR_LEN);
        memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));

        usleep(50*1000);
        control_msg_t ctl_msg = {0};
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_DISCONNECTED;
        api_control_send_msg(&ctl_msg);
        app_wifi_switch_work_mode(WIFI_MODE_AP);
        hccast_mira_service_start();
    }

    free(ap_wifi);
    return NULL;
}

static void *wifi_switch_mode_thread(void* arg)
{
    int wifi_ch = 0;
    hccast_wifi_freq_mode_e mode = (hccast_wifi_freq_mode_e)arg;

    sys_param_t *sys_param = projector_get_sys_param();

    if (HCCAST_WIFI_FREQ_MODE_24G == mode)
    {
        wifi_ch = sys_param->app_data.cast_setting.wifi_ch;
    }
    else if (HCCAST_WIFI_FREQ_MODE_5G == mode)
    {
        wifi_ch = sys_param->app_data.cast_setting.wifi_ch5g;
    }

    //hccast_wifi_mgr_hostap_switch_mode(mode);
    hccast_wifi_mgr_hostap_switch_mode_ex(mode, wifi_ch, 0);

    pthread_detach(pthread_self());

    return NULL;
}

static void *wifi_switch_hs_channel_thread(void* arg)
{
    hccast_mira_service_stop();
    hccast_wifi_mgr_hostap_switch_channel((int)arg);
    hccast_mira_service_start();
    pthread_detach(pthread_self());

    return NULL;
}

#endif

static int hccast_itoa(char * str, unsigned int val)
{
    char *p;
    char *first_dig;
    char temp;
    unsigned t_val;
    int len = 0;
    p = str;
    first_dig = p;

    do
    {
        t_val = (unsigned)(val % 0x0a);
        val   /= 0x0a;
        *p++ = (char)(t_val + '0');
        len++;
    }
    while (val > 0);
    *p-- = '\0';

    do
    {
        temp = *p;
        *p   = *first_dig;
        *first_dig = temp;
        --p;
        ++first_dig;
    }
    while (first_dig < p);
    return len;
}


#ifdef HTTPD_SERVICE_SUPPORT
static int httpd_callback_func(hccast_httpd_event_e event, void* in, void* out)
{
    int ret = 0;
    char mac[6];
    int cur_scene = 0;
    int last_resolution = 0;
    hccast_wifi_ap_info_t *save_ap = NULL;
    hccast_wifi_ap_info_t *con_ap = NULL;
    hccast_wifi_ap_info_t *check_ap = NULL;
    hccast_wifi_ap_info_t *del_ap = NULL;
    int index;

    sys_param_t *sys_param = projector_get_sys_param();
    int ap_tv_sys;
    hccast_wifi_ap_info_t *ap_wifi = NULL;
    unsigned int version;
	int temp;
	char cur_ssid[WIFI_MAX_SSID_LEN] = {0};
	hccast_wifi_scan_result_t *scan_res = NULL;
    control_msg_t msg = {0};

    switch (event)
    {
        case HCCAST_HTTPD_GET_DEV_PRODUCT_ID:
            strcpy((char*)in, (char*)projector_get_some_sys_param(P_DEV_PRODUCT_ID));
            printf("product_id: %s\n", (char*)in);
            break;
        case HCCAST_HTTPD_GET_DEV_VERSION:
            version = projector_get_some_sys_param(P_DEV_VERSION);
            hccast_itoa((char*)in,version);
            printf("version: %s\n",(char*)in);
            break;
        case HCCAST_HTTPD_GET_MIRROR_MODE:
            ret = projector_get_some_sys_param(P_MIRROR_MODE);
            break;
        case HCCAST_HTTPD_SET_MIRROR_MODE:
            projector_set_some_sys_param(P_MIRROR_MODE, (int)in);
            break;
        case HCCAST_HTTPD_GET_AIRCAST_MODE:
            ret = projector_get_some_sys_param(P_AIRCAST_MODE);
            break;
        case HCCAST_HTTPD_SET_AIRCAST_MODE:
			temp = (int)in;
			if(temp != projector_get_some_sys_param(P_AIRCAST_MODE))
			{
                projector_set_some_sys_param(P_AIRCAST_MODE, temp);
	            if(hccast_get_current_scene() == HCCAST_SCENE_NONE)
	            {
	                hccast_air_service_stop();
	                hccast_air_service_start();
	            }
			}
            break;
        case HCCAST_HTTPD_GET_MIRROR_FRAME:
            ret = projector_get_some_sys_param(P_MIRROR_FRAME);
            break;
        case HCCAST_HTTPD_SET_MIRROR_FRAME:
            projector_set_some_sys_param(P_MIRROR_FRAME, (int)in);
            projector_sys_param_save();
            break;
        case HCCAST_HTTPD_GET_BROWSER_LANGUAGE:
            ret = projector_get_some_sys_param(P_BROWSER_LANGUAGE);
            break;
        case HCCAST_HTTPD_SET_BROWSER_LANGUAGE:
            projector_set_some_sys_param(P_BROWSER_LANGUAGE, (int)in);
            projector_sys_param_save();
            break;
        case HCCAST_HTTPD_GET_SYS_RESOLUTION:
            ret = projector_get_some_sys_param(P_SYS_RESOLUTION);
            break;
        case HCCAST_HTTPD_SET_SYS_RESOLUTION:
            ap_tv_sys = (int)in;
            if (ap_tv_sys != APP_TV_SYS_AUTO &&
                projector_get_some_sys_param(P_SYS_RESOLUTION) == ap_tv_sys)
            {
                printf("%s(), same tvsys:%d, not change TV sys\n",
                       __func__, ap_tv_sys);
                break;
            }

            last_resolution = projector_get_some_sys_param(P_SYS_RESOLUTION);

            ret = tv_sys_app_set(ap_tv_sys);

            if (API_SUCCESS == ret)
            {
                printf("%s(), line:%d. save app tv sys: %d!\n",
                       __func__, __LINE__, ap_tv_sys);
                projector_set_some_sys_param(P_SYS_RESOLUTION, ap_tv_sys);

                if(((last_resolution == APP_TV_SYS_4K)&&(ap_tv_sys != APP_TV_SYS_4K)) \
                   || ((last_resolution != APP_TV_SYS_4K)&&(ap_tv_sys == APP_TV_SYS_4K)) \
                   || ((last_resolution != APP_TV_SYS_AUTO)&&(ap_tv_sys == APP_TV_SYS_AUTO)) \
                   || ((last_resolution == APP_TV_SYS_AUTO)&&(ap_tv_sys != APP_TV_SYS_AUTO)) \
                  )
                {
                    cur_scene = hccast_get_current_scene();
                    if((cur_scene != HCCAST_SCENE_AIRCAST_PLAY) && (cur_scene != HCCAST_SCENE_AIRCAST_MIRROR))
                    {
                        if(hccast_air_service_is_start())
                        {
                            hccast_air_service_stop();
                            hccast_air_service_start();
                        }
                    }
                }

                projector_sys_param_save();
            }
            break;
        case HCCAST_HTTPD_GET_DEVICE_MAC:
            api_get_mac_addr(mac);
            memcpy(in, mac,sizeof(mac));
            break;
        case HCCAST_HTTPD_GET_DEVICE_NAME:
            strcpy((char*)in, (char*)projector_get_some_sys_param(P_DEVICE_NAME));
            break;
        case HCCAST_HTTPD_SET_DEVICE_NAME:
            projector_set_some_sys_param(P_DEVICE_NAME, (int)in);
            projector_sys_param_save();
            msg.msg_type = MSG_TYPE_NETWORK_DEV_NAME_SET;
            api_control_send_msg(&msg);
            break;
        case HCCAST_HTTPD_GET_DEVICE_PSK:
            strcpy((char*)in, (char*)projector_get_some_sys_param(P_DEVICE_PSK));
            break;
        case HCCAST_HTTPD_SET_DEVICE_PSK:
            projector_set_some_sys_param(P_DEVICE_PSK, (int)in);
            projector_sys_param_save();
            break;
        case HCCAST_HTTPD_SET_SYS_RESTART:
            printf("HCCAST_HTTPD_SET_SYS_RESTART\n");
            api_system_reboot();
            break;
        case HCCAST_HTTPD_SET_SYS_RESET:
            printf("HCCAST_HTTPD_SET_SYS_RESET\n");
            factary_init = 1;
            projector_factory_reset();
            api_system_reboot();
            break;
        case HCCAST_HTTPD_WIFI_AP_DISCONNECT:
        {
            pthread_t tid;
            memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
            pthread_create(&tid, NULL,wifi_disconnect_thread, NULL);
            break;
        }
        case HCCAST_HTTPD_CHECK_AP_SAVE:
        {
            check_ap = (hccast_wifi_ap_info_t*)in;
            save_ap = (hccast_wifi_ap_info_t *)out;
            index =sysdata_check_ap_saved(check_ap);

            if(index >= 0)
            {
                if(save_ap)
                {
                    strcpy(save_ap->ssid, sys_param->app_data.cast_setting.wifi_ap[index].ssid);
                    strcpy(save_ap->pwd, sys_param->app_data.cast_setting.wifi_ap[index].pwd);
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
            con_ap = (hccast_wifi_ap_info_t *)in;
            if(con_ap)
            {
                ap_wifi = (hccast_wifi_ap_info_t *)malloc(sizeof(hccast_wifi_ap_info_t));
                if(ap_wifi)
                {
                    pthread_t tid;
                    memcpy(ap_wifi,con_ap,sizeof(hccast_wifi_ap_info_t));
                    memcpy(g_connecting_ssid, con_ap->ssid, sizeof(g_connecting_ssid));
                    pthread_create(&tid, NULL,wifi_connect_thread,(void*)ap_wifi);
                }
                else
                {
                    return -1;
                }
            }
            break;
        }
        case HCCAST_HTTPD_DELETE_WIFI_INFO:
            del_ap = (hccast_wifi_ap_info_t *)in;
            if(del_ap)
            {
                index =sysdata_check_ap_saved(del_ap);
                printf("del_ap->ssid:%s, index: %d\n",del_ap->ssid,index);
                if(index >= 0)
                {
                    sysdata_wifi_ap_delete(index);
                    projector_sys_param_save();
                }
            }
            break;
        case HCCAST_HTTPD_GET_CUR_WIFI_INFO:
        {
            hccast_wifi_ap_info_t *cur_ap;
            hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
            cur_ap = sysdata_get_wifi_info(cur_ssid);
            if(cur_ap)
                memcpy(in,cur_ap,sizeof(hccast_wifi_ap_info_t));
            break;
        }
        case HCCAST_HTTPD_SHOW_PROGRESS:
        {
            printf("progress: %d\n",(int)in);
            msg.msg_type = MSG_TYPE_UPG_DOWNLOAD_PROGRESS;
            msg.msg_code = (uint32_t)in;
            api_control_send_msg(&msg);
            break;
        }
        case HCCAST_HTTPD_GET_UPLOAD_DATA_START:
        {
        	hccast_stop_services();//stop all services, avoid download data abort.
        	
            msg.msg_type = MSG_TYPE_NET_UPGRADE;
            api_control_send_msg(&msg);
            api_sleep_ms(500);

            break;
        }
        case HCCAST_HTTPD_GET_UPGRADE_FILE_BEGING:
        {
        	hccast_stop_services();//stop all services, avoid download data abort.
        	
            msg.msg_type = MSG_TYPE_NET_UPGRADE;
            api_control_send_msg(&msg);
            api_sleep_ms(500);
            break;
        }
        case HCCAST_HTTPD_GET_UPLOAD_DATA_FAILED:
        {
        	hccast_start_services();//restart all services.
        	
        	msg.msg_type = MSG_TYPE_UPG_STATUS;
        	msg.msg_code = UPG_STATUS_SERVER_FAIL;
        	api_control_send_msg(&msg);
            break;
        }
		case HCCAST_HTTPD_MSG_UPGRADE_SERVER_BAD:
        {
        	hccast_start_services();//restart all services.
        	
        	msg.msg_type = MSG_TYPE_UPG_STATUS;
        	msg.msg_code = UPG_STATUS_SERVER_FAIL;
        	api_control_send_msg(&msg);
            break;
        }
        case HCCAST_HTTPD_MSG_UPGRADE_BAD_RES:
        {
            //http server return 4xx.
            break;
        }
        case HCCAST_HTTPD_MSG_USER_UPGRADE_ABORT:
        {
        	hccast_start_services();//restart all services.

			msg.msg_type = MSG_TYPE_UPG_STATUS;
        	msg.msg_code = UPG_STATUS_USER_STOP_DOWNLOAD;
        	api_control_send_msg(&msg);
            break;
        }
        case HCCAST_HTTPD_MSG_UPGRADE_FILE_SUC:
        {
            //can block to upgrade.
            hccast_web_upgrade_info_st* info = NULL;
            info = (hccast_web_upgrade_info_st*)in;
            if(info)
            {
                printf("upgrade len: %d\n",info->length);
                printf("upgrade buf: %p\n",info->buf);
                //free(info->buf);
            }
            sys_upg_flash_burn(info->buf, info->length);

            break;
        }
        case HCCAST_HTTPD_MSG_UPLOAD_DATA_SUC:
        {
            hccast_web_upload_info_st* info = NULL;
            info = (hccast_web_upload_info_st*)in;
            if(info)
            {
                printf("upload len: %d\n",info->length);
                printf("upload buf: %p\n",info->buf);
                //free(info->buf);
            }
            sys_upg_flash_burn(info->buf, info->length);
            break;
        }
        case HCCAST_HTTPD_GET_WIFI_FREQ_MODE_EN:
            if(HCCAST_WIFI_FREQ_MODE_5G == hccast_wifi_mgr_freq_support_mode() \
               /*HCCAST_WIFI_FREQ_MODE_60G == hccast_wifi_mgr_freq_support_mode()*/)
            {
                ret = 1;
            }
            else
            {
                ret = 0;
            }
            break;
        case HCCAST_HTTPD_GET_WIFI_FREQ_MODE:
            return hccast_wifi_mgr_get_current_freq_mode();
        case HCCAST_HTTPD_SET_WIFI_FREQ_MODE:
        {
            if ((int)in == projector_get_some_sys_param(P_WIFI_MODE))
            {
                return 0;
            }

            projector_set_some_sys_param(P_WIFI_MODE, (int)in);
            projector_sys_param_save();
            hccast_wifi_mgr_hostap_stop();
            api_system_reboot();
#if 0            
#ifdef __linux__
            pthread_t tid;
            pthread_create(&tid, NULL, wifi_switch_mode_thread, (void *)in);
#else
            hccast_mira_service_stop();
            hccast_air_service_stop();
            hccast_dlna_service_stop();
            sleep(1);
            hccast_wifi_mgr_hostap_stop();
            hostap_config_init();

            if (HOSTAP_CHANNEL_AUTO == projector_get_some_sys_param(P_WIFI_CHANNEL))
            {
                //hccast_net_set_if_updown(HCCAST_HOSTAP_INF, HCCAST_NET_IF_UP);
                //hccast_wifi_mgr_trigger_scan(HCCAST_HOSTAP_INF);

                int *argv[2];
                hccast_wifi_mgr_get_best_channel(2, argv);
                hccast_wifi_mgr_set_best_channel(argv[0], argv[1]);
            }

            hccast_wifi_mgr_hostap_start();
            hccast_mira_service_start();
#endif
#endif
            break;
        }
        case HCCAST_HTTPD_GET_WIFI_HS_CHANNEL:
        {
            int ch = projector_get_some_sys_param(P_WIFI_CHANNEL);
            if (HOSTAP_CHANNEL_AUTO == ch)
            {
                return ch;
            }

#if 1       // rtos throught wpas get freq possible error.
            ch = hccast_wifi_mgr_get_current_freq();
#endif
            return ch;
        }
        case HCCAST_HTTPD_GET_WIFI_HS_CHANNEL_BY_FREQ_MODE:
        {
            if (HCCAST_WIFI_FREQ_MODE_24G == (int)in)
            {
                return projector_get_some_sys_param(P_WIFI_CHANNEL_24G);
            }
            else if (HCCAST_WIFI_FREQ_MODE_5G == (int)in)
            {
                return projector_get_some_sys_param(P_WIFI_CHANNEL_5G);
            }

            return projector_get_some_sys_param(P_WIFI_CHANNEL);
        }
        case HCCAST_HTTPD_SET_WIFI_HS_CHANNEL:
        {
            int wifi_ch = (int)in;
            if (wifi_ch == projector_get_some_sys_param(P_WIFI_CHANNEL))
            {
                return 0;
            }

            if ((wifi_ch >= 0 && wifi_ch <= 14) || (wifi_ch >= 34 && wifi_ch <= 196))
            {
                projector_set_some_sys_param(P_WIFI_CHANNEL, wifi_ch);
            }

            projector_sys_param_save();
#ifdef __linux__
#else
            int wifi_mode = projector_get_some_sys_param(P_WIFI_MODE);
            if (HOSTAP_CHANNEL_AUTO == wifi_ch)
            {
                //hccast_net_set_if_updown(HCCAST_HOSTAP_INF, HCCAST_NET_IF_UP);
                //hccast_wifi_mgr_trigger_scan(HCCAST_HOSTAP_INF);

                int *argv[2];
                hccast_wifi_mgr_get_best_channel(2, argv);
                hccast_wifi_mgr_set_best_channel(argv[0], argv[1]);

                if (1 == wifi_mode) // 24G
                {
                    wifi_ch = (int)argv[0];
                }
                else if (2 == wifi_mode) // 5G
                {
                    wifi_ch = (int)argv[1];
                }/*
                else if (3 == app_data->wifi_mode) // 6G
                {
                    wifi_ch = (int)argv[1];
                }*/
                else
                {
                    wifi_ch = (int)argv[0];
                }
            }
#endif
            pthread_t tid;
            pthread_create(&tid, NULL,wifi_switch_hs_channel_thread, (void*)wifi_ch);
            break;            
        }

        case HCCAST_HTTPD_GET_CUR_SCENE_PLAY:

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

            break;
        case HCCAST_HTTPD_STOP_MIRA_SERVICE:
            hccast_mira_service_stop();
            sleep(1);
            break;
        case HCCAST_HTTPD_START_MIRA_SERVICE:
            hccast_mira_service_start();
            break;
	case HCCAST_HTTPD_GET_MIRROR_ROTATION:
		ret = projector_get_some_sys_param(P_MIRROR_ROTATION);
		break;
	case HCCAST_HTTPD_SET_MIRROR_ROTATION:
        temp = (int)in;
        if(projector_get_some_sys_param(P_MIRROR_ROTATION) != temp)
        {
            projector_set_some_sys_param(P_MIRROR_ROTATION, temp);
            projector_sys_param_save();
        #ifdef USBMIRROR_SUPPORT
            cast_usb_mirror_rotate_init();
        #endif
        }	

	    break;
	case HCCAST_HTTPD_GET_MIRROR_VSCREEN_AUTO_ROTATION:
		ret = projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION);
		break;
	case HCCAST_HTTPD_SET_MIRROR_VSCREEN_AUTO_ROTATION:
        temp = (int)in;
        if(projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION) != temp)
        {
            projector_set_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION, temp);
            projector_sys_param_save();
        #ifdef USBMIRROR_SUPPORT
            cast_usb_mirror_rotate_init();
        #endif
        }	

	    break;    
	    
	case HCCAST_HTTPD_GET_WIFI_CONNECT_STATUS:
		ret = hccast_wifi_mgr_get_connect_status();	
		break;
	case HCCAST_HTTPD_GET_CUR_WIFI_SSID:
		ret = hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
		memcpy(in,cur_ssid,sizeof(cur_ssid));
		break;
	case HCCAST_HTTPD_WIFI_SCAN	:
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
        case HCCAST_HTTPD_WIFI_STAT_CHECK_BUSY:
            {
                ret = 0;
                ret |= hccast_wifi_mgr_is_connecting();
                ret |= hccast_wifi_mgr_is_scaning();
            }
            break;
    case HCCAST_HTTPD_GET_UPGRADE_URL:    
        sprintf(in, NETWORK_UPGRADE_URL, projector_get_some_sys_param(P_DEV_PRODUCT_ID));     
        break;
    default :
        break;
    }

    return ret;
}
#endif

#ifdef DLNA_SUPPORT
static void media_callback_func(hccast_media_event_e msg_type, void* param)
{
    control_msg_t ctl_msg = {0};
    ctl_msg.msg_code = (uint32_t)param;

    switch (msg_type)
    {
        case HCCAST_MEDIA_EVENT_PARSE_END:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PARSE_END\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_PLAYING:
            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_PLAY;
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PLAYING\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_PAUSE:
            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_PAUSE;
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PAUSE\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_BUFFERING:
            ctl_msg.msg_type = MSG_TYPE_MEDIA_BUFFERING;
            ctl_msg.msg_code = (uint32_t)param;
            //printf("[%s] %d   HCCAST_MEDIA_EVENT_BUFFERING, %d%%\n", __func__, __LINE__, (int)param);
            break;
        case HCCAST_MEDIA_EVENT_PLAYBACK_END:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PLAYBACK_END\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_STOP;
            break;
        case HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_MEDIA_VIDEO_DECODER_ERROR;
            break;
        case HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_MEDIA_AUDIO_DECODER_ERROR;
            break;
        case HCCAST_MEDIA_EVENT_VIDEO_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_VIDEO_NOT_SUPPORT\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_MEDIA_VIDEO_NOT_SUPPORT;
            ctl_msg.msg_code = (uint32_t)param;
            break;
        case HCCAST_MEDIA_EVENT_AUDIO_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_AUDIO_NOT_SUPPORT\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_MEDIA_AUDIO_NOT_SUPPORT;
            ctl_msg.msg_code = (uint32_t)param;
            break;
        case HCCAST_MEDIA_EVENT_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_NOT_SUPPORT\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_MEDIA_NOT_SUPPORT;
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_DLNA:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_DLNA, media_type: %d\n", __func__, __LINE__,(hccast_media_type_e)param);

            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_START;
            ctl_msg.msg_code = (uint32_t)param;
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST, media_type: %d\n", __func__, __LINE__,(hccast_media_type_e)param);
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRCAST_START;
            ctl_msg.msg_code = (uint32_t)param;
            break;
        case HCCAST_MEDIA_EVENT_URL_SEEK:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_SEEK  position: %ds\n", __func__, __LINE__, (int)param / 1000);
            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_SEEK;
            break;
        case HCCAST_MEDIA_EVENT_SET_VOLUME:
            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_VOL_SET;
            ctl_msg.msg_code = (uint32_t)param;
            printf("[%s] %d   HCCAST_MEDIA_EVENT_SET_VOLUME  volume: %d\n", __func__, __LINE__, (int)param);
            break;
        case HCCAST_MEDIA_EVENT_GET_MIRROR_ROTATION:
            *(int*)param = projector_get_some_sys_param(P_MIRROR_ROTATION);           
            break;  
        case HCCAST_MEDIA_EVENT_GET_FLIP_MODE:
        {
            int flip_mode;
            int rotate;     
            int flip;
            api_get_flip_info(&rotate, &flip);
            flip_mode = (rotate & 0xffff) << 16 | (flip & 0xffff);
            *(int*)param = flip_mode;
            break;
        }
        default:
            break;
    }

    if (0 != ctl_msg.msg_type)
        api_control_send_msg(&ctl_msg);

    if (MSG_TYPE_CAST_DLNA_STOP == ctl_msg.msg_type)
    {
        //While air mirror preempt dlna(air play->air mirror), dlna stop and air mirror start,
        //sometime it is still in dlna play UI(not exit to win root UI),
        //the next air mirror play is starting, then the UI/logo may block the air mirror playing.
        //So here exit callback function wait for win cast root UI opening
        // printf("[%s] wait cast root start tick: %d\n",__func__,(int)time(NULL));
        if (cast_main_ui_wait_ready)
            cast_main_ui_wait_ready(20000);
        // printf("[%s] wait cast root end tick: %d\n",__func__,(int)time(NULL));
    }

    if ((MSG_TYPE_CAST_DLNA_START == ctl_msg.msg_type) || (MSG_TYPE_CAST_AIRCAST_START == ctl_msg.msg_type))
    {
        //Wait the win dlna is open, then start play to avoid the OSD
        // close slowly.
        // printf("[%s] wait dlna menu open start tick: %d\n",__func__,(int)time(NULL));
        if (dlna_ui_wait_ready)
            dlna_ui_wait_ready(20000);
        // printf("[%s] wait dlna menu open end tick: %d\n",__func__,(int)time(NULL));
    }
}
#endif


#ifdef WIFI_SUPPORT

#ifdef __linux__
static void network_probed_wifi(void)
{
    if (0 == access("/var/lib/misc/RTL8188FTV.probe", F_OK))
    {
        printf("Wi-Fi: RTL8188FTV\n");
        m_probed_wifi_module = HCCAST_NET_WIFI_8188FTV;
    }
    else if (0 == access("/var/lib/misc/RTL8811FTV.probe", F_OK))
    {
        printf("Wi-Fi: RTL8811FTV\n");
        m_probed_wifi_module = HCCAST_NET_WIFI_8811FTV;
    }

    //it must have node wlan0 to make wifi work.
    if (access("/var/run/wpa_supplicant/wlan0", F_OK)){
        m_probed_wifi_module = 0;
    }


    hccast_wifi_mgr_set_wifi_model(m_probed_wifi_module);
}

static int is_file_exist(char *file_name)
{
    if(access(file_name,F_OK) == 0){
        return 1;
    }else{
        //printf("%s is not exist\n", file_name);
        return 0;
    }
}

static int network_wifi_is_ready(void)
{
    int ready = 0;

    if (
        is_file_exist("/var/run/hostapd/wlan0") && 
        is_file_exist("/var/run/wpa_supplicant/p2p0") && 
        is_file_exist("/var/run/wpa_supplicant/wlan0")
       ){
        ready = 1;
    }else{
        ready = 0;
    }

    return ready;
}

#endif


void network_wifi_module_set(int wifi_module)
{
    m_probed_wifi_module = wifi_module;
    hccast_wifi_mgr_set_wifi_model(wifi_module);
}

int network_wifi_module_get(void)
{
#ifdef __linux__
    if (0 == network_wifi_is_ready())
        m_probed_wifi_module = 0;
#endif
    return m_probed_wifi_module;
}

int hostap_get_connect_count(void)
{
    return hostap_connect_count;
}

static void hostap_set_connect_count(int count)
{
    hostap_connect_count = count;
}


static int needfresh = 0;
static int hostap_connect_update(void)
{
    int cur_connect = 0;
    int pre_connect = 0;
    control_msg_t ctl_msg = {0};

    //hostap not ready.
    if((hccast_wifi_mgr_get_hostap_status() == 0) || hccast_wifi_mgr_p2p_get_connect_stat() || factary_init)
    {
        needfresh = 0;
        hostap_set_connect_count(0);
        return 0;
    }


    if(hccast_wifi_mgr_p2p_get_connect_stat())
    {
        //printf("########%s mira connect do not anything.#########\n", __FUNCTION__);
        return 0;
    }

    cur_connect = hccast_wifi_mgr_hostap_get_sta_num(NULL);
    pre_connect = hostap_get_connect_count();

#if 0
    if(hostap_discover_ok == 0)
    {
        //wait real connect success at first time.
        cur_connect = 0;
        pre_connect = 0;
    }
#endif

    if (cur_connect != pre_connect)
    {
        printf("########%s cur_connect=%d pre_connect=%d#########\n", __FUNCTION__, cur_connect, pre_connect);
        hostap_set_connect_count(cur_connect);
        if (cur_connect == 0)
        {
            //send msg notify ui no phone had connect to dongle.
            //if (needfresh == 1)
            {
                printf("====================== no phone connect===============\n");
                ctl_msg.msg_type = MSG_TYPE_NETWORK_DEVICE_BE_DISCONNECTED;
                api_control_send_msg(&ctl_msg);
                m_wifi_config.bConnectedByPhone = false;
                m_wifi_config.host_ap_ip_ready = false;


                needfresh = 0;
                hostap_discover_ok = 0;
                hccast_ap_dlna_aircast_stop();
            }
        }
        else
        {
            //send msg notify ui have phone connect to dongle.
            printf("====================== new phone connect===============\n");
            if(needfresh == 0)
            {
                needfresh = 1;
                if (network_service_enable_get())
                    hccast_ap_dlna_aircast_start();
            }  
            ctl_msg.msg_type = MSG_TYPE_NETWORK_DEVICE_BE_CONNECTED;
            m_wifi_config.bConnectedByPhone = true;
            m_wifi_config.host_ap_ip_ready = true;
            api_control_send_msg(&ctl_msg); 
        }
    }

    return 0;
}

static void *hostap_connect_thread(void *args)
{
    printf("----------------------------hccast_hostap_connect_thread is running.-----------------------------\n");
    while (1)
    {
        hostap_connect_update();
        api_sleep_ms(200);
    }
    return NULL;
}


static void hostap_connect_detect_init()
{
    pthread_t tid;

    if (pthread_create(&tid, NULL,hostap_connect_thread, NULL) < 0)
    {
        printf("Create hccast_hostap_connect_thread error.\n");
    }
}
#endif

int network_init(void)
{
#ifdef WIFI_SUPPORT

#if 0

#define APP_HOSTAP_IP_START_ADDR ("192.168.5.10")
#define APP_HOSTAP_IP_END_ADDR   ("192.168.5.100")
#define APP_HOSTAP_LOCAL_IP_ADDR ("192.168.5.1")

    udhcp_conf_t udhcpd_conf =
    {
        .func           = NULL, // use hccast default cb, otherwise, use user defined cb
        .ifname         = UDHCP_IF_WLAN0,
        .ip_start_def   = APP_HOSTAP_IP_START_ADDR,
        .ip_end_def     = APP_HOSTAP_IP_END_ADDR,
        .ip_host_def    = APP_HOSTAP_LOCAL_IP_ADDR,
    };
    hccast_wifi_mgr_set_udhcpd_conf(udhcpd_conf);

#endif


    hccast_wifi_mgr_init(wifi_mgr_callback_func);
#endif

#ifdef HTTPD_SERVICE_SUPPORT
    hccast_httpd_service_init(httpd_callback_func);
#endif

#ifdef DLNA_SUPPORT
    hccast_media_init(media_callback_func);
#endif

    cast_init();
#ifdef WIFI_SUPPORT
  #ifdef __linux__
    network_probed_wifi();
  #endif
    hostap_connect_detect_init();
#endif

#ifdef HTTPD_SERVICE_SUPPORT
    if(network_service_enable_get())
    {
        hccast_httpd_service_start();
    }    
#endif

    return API_SUCCESS;
}

int network_deinit(void)
{
#ifdef WIFI_SUPPORT
    hccast_wifi_mgr_uninit();
#endif

#ifdef HTTPD_SERVICE_SUPPORT
    hccast_httpd_service_uninit();
#endif    

    hccast_mira_service_uninit();
    hccast_dlna_service_uninit();
    return API_SUCCESS;
}

#ifdef WIFI_SUPPORT
static void hostap_config_init(void)
{
    hccast_wifi_hostap_conf_t conf = {0};

    int wifi_mode = hccast_wifi_mgr_freq_support_mode();

    //! The wpas conf will be overwritten here
    conf.mode    = projector_get_some_sys_param(P_WIFI_MODE);
    if (conf.mode <= wifi_mode)
    {
        //conf.mode = projector_get_some_sys_param(P_WIFI_MODE);
        conf.channel = projector_get_some_sys_param(P_WIFI_CHANNEL);
    }
    else
    {
        conf.mode = wifi_mode;

        if (HOSTAP_CHANNEL_AUTO != projector_get_some_sys_param(P_WIFI_CHANNEL))
        {
            if (HCCAST_WIFI_FREQ_MODE_24G == conf.mode)
            {
                conf.channel = projector_get_some_sys_param(P_WIFI_CHANNEL_24G);
            }
            else if (HCCAST_WIFI_FREQ_MODE_5G == conf.mode)
            {
                conf.channel = projector_get_some_sys_param(P_WIFI_CHANNEL_5G);
            }
            else
            {
                conf.channel = projector_get_some_sys_param(P_WIFI_CHANNEL);
            }
        }
    }

    log(DEMO, INFO, "#### supp_mode:%d, conf.mode:%d, conf.ch:%d\n", wifi_mode, conf.mode, conf.channel);

    //strcpy(conf.country_code, "CN");
    strncpy(conf.pwd, (char*)projector_get_some_sys_param(P_DEVICE_PSK), sizeof(conf.pwd));
    strncpy(conf.ssid, (char*)projector_get_some_sys_param(P_DEVICE_NAME), sizeof(conf.ssid));

#ifdef __linux__
    hccast_wifi_mgr_hostap_set_conf(&conf);
#else
    hccast_wifi_mgr_hostap_store_conf(&conf);
#endif
}

volatile bool m_wifi_connecting = false;
volatile bool m_wifi_init_done = false;
bool app_wifi_init_done(void)
{
    return m_wifi_init_done;
}

bool get_m_wifi_connecting(){
    return m_wifi_connecting;
}

#define WIFI_CHECK_TIME 50000000
static void *network_connect_task(void *arg)
{
    hccast_wifi_ap_info_t wifi_ap;
    uint32_t loop_cnt = WIFI_CHECK_TIME/100;
    m_wifi_connecting = true;
    char connect_fail = 0;
    control_msg_t ctl_msg = {0};

    while(!network_wifi_module_get())
    {
        api_sleep_ms(100);
        if (0 == loop_cnt--){
            connect_fail = 1;
            goto connect_exit;
        }
    }

    network_init();

    //step 1: http server startup

    sysdata_init_device_name();
    hostap_config_init();

//return NULL;

#if 1
    //step 2:
    if (projector_get_some_sys_param(P_WIFI_ONOFF) && sysdata_wifi_ap_get(&wifi_ap))
    {
        app_wifi_switch_work_mode(WIFI_MODE_STATION);

        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTING;
        api_control_send_msg(&ctl_msg);

        //Get wifi AP from flash, connect wifi
        printf("%s(), line:%d, connect to %s, pwd:%s\n", __FUNCTION__, __LINE__, \
               wifi_ap.ssid, wifi_ap.pwd);
        memcpy(g_connecting_ssid, wifi_ap.ssid, sizeof(g_connecting_ssid));
        hccast_wifi_mgr_connect(&wifi_ap);
        if (hccast_wifi_mgr_get_connect_status())
        {
            hccast_wifi_mgr_udhcpc_stop();
            api_sleep_ms(100);
            hccast_wifi_mgr_udhcpc_start();
            api_sleep_ms(100);
        }
        else
        {
            printf("%s(), line:%d. connect timeout!\n", __func__, __LINE__);
            m_wifi_config.bConnected = false;  
            m_wifi_config.bConnectedByPhone = false;
            m_wifi_config.host_ap_ip_ready = false;
            m_wifi_config.sta_ip_ready = false;
            memset(m_wifi_config.local_ip, 0, MAX_IP_STR_LEN);
            memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));            

            usleep(50*1000);
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL;
            api_control_send_msg(&ctl_msg);

            hccast_wifi_mgr_udhcpc_stop();
            hccast_wifi_mgr_disconnect_no_message();

            if(network_service_enable_get())
            {
                app_wifi_switch_work_mode(WIFI_MODE_AP);
                hccast_mira_service_start();
            }
        }
    }
    else
    {
        if(network_service_enable_get())
        {
            //No wif AP in flash, entering AP mode
            app_wifi_switch_work_mode(WIFI_MODE_AP);

            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_STATUS_UPDATE;
            api_control_send_msg(&ctl_msg);
            hccast_mira_service_start();

            printf("%s(), line:%d. AP mode start\n", __FUNCTION__, __LINE__);
        }
    }
#endif
connect_exit:
    m_wifi_connecting = false;
    if (connect_fail)    
        m_wifi_init_done = false;
    else
        m_wifi_init_done = true;

    return NULL;
}
#endif

#ifdef __HCRTOS__
static char net_read = 0;
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
#endif
//Connet wifi ap if there is valid wifi AP information in flash. Otherwise
//set device to AP mdoe
int network_connect(void)
{
#ifdef WIFI_SUPPORT

    if (m_wifi_connecting)
        return API_SUCCESS;

    pthread_t thread_id = 0;
    pthread_attr_t attr;
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, network_connect_task, NULL))
    {
        return API_FAILURE;
    }
    pthread_attr_destroy(&attr);
#else // 

	static udhcp_conf_t eth_udhcpc_conf =
	{
	    .func = udhcpc_cb,
	    .ifname = UDHCP_IF_NONE,
	    .pid    = 0,
	    .run    = 0
	};

#ifdef NETWORK_API
    udhcpc_start(&eth_udhcpc_conf);

    while(!net_read)
    {
        sleep(1);
    }

    hccast_start_services();
#endif
#endif // WIFI_SUPPORT

    return API_SUCCESS;
}



static volatile bool m_service_enable = false;
/*
check if the network service can be 
start freedom. For some scene, dlna,miracast,aircast,
hostap,etc only start in cast window.
 */
bool network_service_enable_get()
{
    return m_service_enable;
}

void network_service_enable_set(bool start)
{
    m_service_enable = start;
}

wifi_config_t *app_wifi_config_get(void)
{
    return &m_wifi_config;
}



#ifdef AUTO_HTTP_UPGRADE_SUPPORT

static void dump_buf(char *tag, uint8_t *buf, int len)
{
     int i;
     
     printf("******%s*******\n", tag);
     for(i = 0; i < len; i++)
    {
         if(i %16==0)
             printf("\n");
           printf("%02x ", buf[i]);
    }
    printf("\n\n");
}

static char m_upgrade_url[256] = {0};
#define UPGRADE_CONFIG_FILE_LEN 4096

static char *network_check_upgrade_url(void)
{
    //Read network upgrade config.
    char *buffer = NULL;
    int config_len = 0;
    char request_url[256];
    char product_id[32];
    uint32_t local_version;

    strcpy((char*)product_id, (char*)projector_get_some_sys_param(P_DEV_PRODUCT_ID));
    snprintf(request_url, sizeof(request_url), NETWORK_UPGRADE_URL, product_id);

    buffer = (char*)malloc(UPGRADE_CONFIG_FILE_LEN);
    if(buffer == NULL)
    {   
        printf("%s buffer malloc error\n",__func__);
        return NULL;
    }
    memset(buffer, 0, UPGRADE_CONFIG_FILE_LEN);

    printf("*request_url:%s\n", request_url);
    config_len = hccast_upgrade_download_config(request_url, buffer, UPGRADE_CONFIG_FILE_LEN);
    if (config_len <= 0){
        printf("%s(), line:%d. download upgrade config fail!\n", __func__, __LINE__);
        if(buffer)
        {
            free(buffer);
        }
        return NULL;
    }
    //printf("upgrade config:\n%s\n", buffer);


    //Get the product_id, version of upgrade config
    cJSON* cjson_obj = NULL;
    cJSON* cjson_force_upgrade = NULL;
    cJSON* cjson_product = NULL;
    cJSON* cjson_version = NULL;
    cJSON* cjson_url = NULL;
    char* cjson_data = NULL;
    int skip_len = 0;
    int cjson_data_len = 0;
    int i = 0;

    cjson_data = strstr(buffer, "jsonp_callback(");
    if(cjson_data == NULL)
    {
        printf("%s can not parse cjson_data\n",__func__);
        goto fail_exit;
    }

    
    skip_len = strlen("jsonp_callback(");
    cjson_data += skip_len;
    
    cjson_data_len = strlen(cjson_data);
    for(i = cjson_data_len; i >= 0; i--)
    {
        if(cjson_data[i] == ')')
        {
            cjson_data[i] = '\0';
            break;
        }
    }
 
    //printf("cjson_data:\n%s\n",cjson_data);
    cjson_obj = cJSON_Parse(cjson_data);
    if (!cjson_obj){
        printf("%s(), line:%d. parse config json error\n", __func__, __LINE__);
        goto fail_exit;
    }

    cjson_url = cJSON_GetObjectItem(cjson_obj, "url");
    cjson_version = cJSON_GetObjectItem(cjson_obj, "version");
    cjson_product = cJSON_GetObjectItem(cjson_obj, "product");
    cjson_force_upgrade = cJSON_GetObjectItem(cjson_obj, "force_upgrade");
    if (!cjson_url || !cjson_version || !cjson_product || !cjson_force_upgrade)
    {
        printf("No upgrade file!\n");
        goto fail_exit;
    }        
    printf("web file:%s\n", cjson_url->valuestring);

    local_version = (uint32_t)projector_get_some_sys_param(P_DEV_VERSION);
    long long version_val = atoll(cjson_version->valuestring);
    uint32_t web_version = (uint32_t)version_val;
    int force_upgrade = (cjson_force_upgrade->type == cJSON_True) ? 1 : 0;

    printf("web version: %u, local version:%u, web_product:%s, local_product:%s, force_upgrade:%d\n", \
        (unsigned int)web_version, (unsigned int)local_version, cjson_product->valuestring, product_id, force_upgrade);

    if(force_upgrade)
    {
        if(strcmp(cjson_product->valuestring, product_id) == 0)
        {   
            if (web_version <= local_version){
                printf("only old version, do not upgrade!\n");
                goto fail_exit;
            }
        }
        else
        {
            printf("product name not match!\n");
            goto fail_exit;
        }
    }
    else
    {
        printf("not support force upgrade!\n");
        goto fail_exit;
    }

    snprintf(m_upgrade_url, sizeof(m_upgrade_url), "%s", cjson_url->valuestring);

    if (cjson_obj)
        cJSON_Delete(cjson_obj);

    if(buffer)
        free(buffer);

    return m_upgrade_url;

fail_exit:
    if (cjson_obj)
        cJSON_Delete(cjson_obj);
    
    if(buffer)
        free(buffer);
        
    return NULL;
}

char *network_get_upgrade_url(void)
{
    return m_upgrade_url;
}


static void *network_upgrade_task(void *arg)
{
    //step 1
    //Check if wifi network(station mode) is ready.
    //
    int loop_cnt = WIFI_CHECK_TIME/100;
    while (!(network_wifi_module_get() && hccast_wifi_mgr_get_connect_status())){
        api_sleep_ms(100);
        //if network is not ready, return
        if (0 == loop_cnt--){
            printf("%s(), line:%d. network not ready, dont update!", __func__, __LINE__);
            return NULL;
        }
    }

    //step 2
    //Read network upgrade config.
    char* upgrade_url = NULL;
    upgrade_url = network_check_upgrade_url();
    if (NULL == upgrade_url)
        return NULL;

    //step 3 
    //Download corresponding upgraded file.
    printf("* updatde request_url:%s\n", upgrade_url);
    hccast_web_uprade_download_start(upgrade_url);

}

int network_upgrade_start()
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, network_upgrade_task, NULL)) {
        return -1;
    }
    pthread_attr_destroy(&attr);

    return 0;
        
}
#endif

static void wifi_do_connect(hccast_wifi_ap_info_t *wifi_info)
{
    char wifi_ap_exist = 0;
    hccast_wifi_ap_info_t wifi_ap;
    control_msg_t ctl_msg = {0};
    int index;

    if (NULL == wifi_info){
        if (sysdata_wifi_ap_get(&wifi_ap))
            wifi_ap_exist = 1;
    }else{
        memcpy(&wifi_ap, wifi_info, sizeof(hccast_wifi_ap_info_t));
        wifi_ap_exist = 1;
    }

    if (wifi_ap_exist)
    {
        app_wifi_switch_work_mode(WIFI_MODE_STATION);

        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTING;
        api_control_send_msg(&ctl_msg);

        //Get wifi AP from flash, connect wifi
        printf("%s(), line:%d, connect to %s, pwd:%s\n", __FUNCTION__, __LINE__, \
               wifi_ap.ssid, wifi_ap.pwd);

        memcpy(g_connecting_ssid, wifi_ap.ssid, sizeof(g_connecting_ssid));
        hccast_wifi_mgr_connect(&wifi_ap);
        if (hccast_wifi_mgr_get_connect_status())
        {
            hccast_wifi_mgr_udhcpc_stop();
            api_sleep_ms(100);
            hccast_wifi_mgr_udhcpc_start();
            api_sleep_ms(100);

        index = sysdata_check_ap_saved(&wifi_ap);
        printf("ssid index: %d\n",index);
        if(index >= 0)//set the index ap to first.
        {
            sysdata_wifi_ap_delete(index);
        }

        sysdata_wifi_ap_save(&wifi_ap);
        projector_sys_param_save();
        sleep(1);

        }
        else
        {
            printf("%s(), line:%d. connect timeout!\n", __func__, __LINE__);
            m_wifi_config.bConnected = false;  
            m_wifi_config.bConnectedByPhone = false;
            m_wifi_config.host_ap_ip_ready = false;
            m_wifi_config.sta_ip_ready = false;
            memset(m_wifi_config.local_ip, 0, MAX_IP_STR_LEN);
            memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
    
            usleep(50*1000);
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL;
            api_control_send_msg(&ctl_msg);
            hccast_wifi_mgr_udhcpc_stop();
            hccast_wifi_mgr_disconnect_no_message();

            if(network_service_enable_get())
            {
                app_wifi_switch_work_mode(WIFI_MODE_AP);
                hccast_mira_service_start();
            }    
        }
    }
    else
    {
        //No wif AP in flash, entering AP mode
        if(network_service_enable_get())
        {
            app_wifi_switch_work_mode(WIFI_MODE_AP);
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTED;
            api_control_send_msg(&ctl_msg);
            hccast_mira_service_start();
        }
        printf("%s(), line:%d. AP mode start\n", __FUNCTION__, __LINE__);
    }

}

static void *wifi_connect_task(void *arg)
{
    hccast_wifi_ap_info_t *wifi_ap = (hccast_wifi_ap_info_t*)arg;
    m_wifi_connecting = true;

    if (network_wifi_module_get()){
        wifi_do_connect(wifi_ap);
    }

    m_wifi_connecting = false;

    return NULL;
}

int app_wifi_reconnect(hccast_wifi_ap_info_t *wifi_ap)
{
    if (m_wifi_connecting)
        return 1;

    pthread_t thread_id = 0;
    pthread_attr_t attr;
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    pthread_create(&thread_id, &attr, wifi_connect_task, (void*)wifi_ap);
    pthread_attr_destroy(&attr);

    return 0;
}

int app_wifi_switch_work_mode(WIFI_MODE_e wifi_mode)
{
    char delay_flag = 0;
    
    if (WIFI_MODE_STATION == wifi_mode){
        if (hccast_wifi_mgr_get_hostap_status()){      
            hccast_wifi_mgr_hostap_stop();
            delay_flag = 1;
        }
    #ifdef __HCRTOS__
        if (!hccast_wifi_mgr_get_station_status()){
            if (delay_flag)
                api_sleep_ms(50);            
            hccast_wifi_mgr_enter_sta_mode();
        }
    #endif        
    } else if (WIFI_MODE_AP == wifi_mode){
    #ifdef __HCRTOS__
        if (hccast_wifi_mgr_get_station_status()){
            hccast_wifi_mgr_exit_sta_mode();
            delay_flag = 1;
        }
    #endif
        if (!hccast_wifi_mgr_get_hostap_status())   {   
            if (delay_flag)
                api_sleep_ms(50);

    #ifdef __linux__
    #else
            hostap_config_init();
    #endif
            hccast_wifi_mgr_hostap_start();
        }
    } else if (WIFI_MODE_NONE == wifi_mode){
        if (hccast_wifi_mgr_get_hostap_status()){      
            hccast_wifi_mgr_hostap_stop();
            delay_flag = 1;
        }
    #ifdef __HCRTOS__
        if (hccast_wifi_mgr_get_station_status()){
            if (delay_flag)
                api_sleep_ms(50);            
            hccast_wifi_mgr_exit_sta_mode();
        }
    #endif        
    }
    m_wifi_config.mode = wifi_mode;
}


#ifdef LIBCURL_SUPPORT

#include <curl/curl.h>

typedef struct{
    char url[1024];
    char file_name[128];
    uint8_t *data;
    uint32_t size;
    uint32_t data_pos;

    void *user_data;
    net_dowload_cb cb_func;
    FILE *fp;
}NET_DOWNLOAD_PARAM_s;

static uint32_t _curl_cb(void *data, uint32_t size, uint32_t nmemb, void *user_data)
{
    FILE* fp = NULL;  
    NET_DOWNLOAD_PARAM_s *down_param = (NET_DOWNLOAD_PARAM_s*)user_data;
    uint32_t realsize = size * nmemb;

    //printf("%s(), line: %d. size:%d, nmemb:%d\n", __func__, __LINE__, size, nmemb);    
    printf(".");
    fp = down_param->fp;
    if (fp)
    {
        fwrite(data, size, nmemb, fp);
    }
    else
    {
        if (down_param->data && (down_param->data_pos <= down_param->size))
        {
            memcpy((down_param->data+down_param->data_pos), data, realsize);
        }
        else
            return 0; //stop download.
    }

    //return the real size, else if network would not continue download ...
    down_param->data_pos += realsize;
    return realsize;  
}

static void *_network_data_down_task(void *param)
{
    FILE *fp = NULL;
    NET_DOWNLOAD_PARAM_s *down_param;
    CURL *curl_handle = NULL;
    CURLcode retcCode;

    down_param = (NET_DOWNLOAD_PARAM_s*)param;
    do {
        curl_handle = curl_easy_init();
        if (!curl_handle)
        {
            printf("%s(), line: %d. , curl_easy_init fail\n", __func__, __LINE__);    
            break;
        }

        if (strlen(down_param->file_name))
        {
            fp = fopen(down_param->file_name, "wb+");
            if (!fp)
            {
                printf("%s(), line: %d. , open %s fail\n", __func__, __LINE__, down_param->file_name);    
                break;
            }
            down_param->fp = fp;
        }
        curl_easy_setopt(curl_handle, CURLOPT_URL, down_param->url);

        /* send all data to this function  */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, _curl_cb);

        /* we pass our 'down_param' struct to the callback function */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)down_param);

        curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5);  
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);  

        //do not vefificate host, else if curl_easy_perform return error: CURLE_PEER_FAILED_VERIFICATION
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);  
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);  


        /* send a request */
        retcCode = curl_easy_perform(curl_handle);

        if (fp)
            fclose(fp);

        if (retcCode)
        {   
            const char* pError = curl_easy_strerror(retcCode);        
            printf("%s(), pError:%s(%d)!\n", __func__, pError, retcCode);  
        }
        else
        {
            if (down_param->cb_func)
                down_param->cb_func(down_param->user_data);
            printf("%s(), download OK, file length:%d!\n", __func__, down_param->data_pos);  
        }


    }while(0);

    if (curl_handle)
        curl_easy_cleanup(curl_handle);
    if (down_param)
        free(down_param);

    return NULL;
}

//Download URL to file or memory in block mode or none-block mode.
//url: the url to download
//file_name: Download URL data and save to file if it is not NULL. Choose one of file_name and data buffer
//data:  Download URL data the the buffer if it is not NULL. Choose one of file_name and data buffer
//size:  Use with parameter data, the size of data buffer 
//net_cb: Call the callback function while finish download if it is non-block mode.
//user_data: The user data will pass back to user in callback function
//block: blocked mode or none-block mode. The api return when download finished in block mode.
// The api return immediately in none-block mode, download finished while net_cb is called.
int api_network_download(char *url, char *file_name, uint8_t *data, uint32_t size, \
        net_dowload_cb net_cb, void *user_data, bool block)
{
    pthread_attr_t attr;
    pthread_t thread_id = 0;
    NET_DOWNLOAD_PARAM_s *down_param;
    down_param = malloc(sizeof(NET_DOWNLOAD_PARAM_s));
    if (!down_param)
        return -1;

    printf("%s(), url:%s\n", __func__, url);
    memset(down_param, 0, sizeof(NET_DOWNLOAD_PARAM_s));
    strncpy(down_param->url, url, sizeof(down_param->url));
    if (file_name)
        strncpy(down_param->file_name, file_name, sizeof(down_param->file_name));
    down_param->user_data = user_data;
    down_param->cb_func = net_cb;
    down_param->data = data;
    down_param->size = size;

    if (block)
    {    
        _network_data_down_task((void *)down_param);
    }
    else
    {
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x2000);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
        if (pthread_create(&thread_id, NULL, _network_data_down_task, (void *)down_param)) {
            printf("pthread_create receive_cvbs_in_event_func fail\n");
            return -1;
        }
        pthread_attr_destroy(&attr);

    }
    return 0;
}

#endif



#endif
