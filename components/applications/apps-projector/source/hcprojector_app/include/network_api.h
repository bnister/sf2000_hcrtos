/**
 * network_api.h
 */

#ifndef __NETWORK_API_H__
#define __NETWORK_API_H__

#ifdef WIFI_SUPPORT
#include <hccast/hccast_wifi_mgr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_IP_STR_LEN 32

typedef enum{
    WIFI_MODE_NONE,
    WIFI_MODE_STATION,
    WIFI_MODE_AP,
}WIFI_MODE_e;


typedef struct
{
    char local_ip[MAX_IP_STR_LEN];
    char connected_phone_ip[MAX_IP_STR_LEN];

    bool bPlugged;           // 1: Plugged   0: UnPlugged
    bool bDeviceEnabled;     // 1: Enabled   0: Disabled
    bool bDeviceReady;       // 1: Ready 0: Not Ready
    bool bConnected;         // 1: Connect to a wifi AP; 0: Not connect to wifi ap
    bool bConnectedByPhone;  // 1: Connected by Phone; 0: Not connect by phone
    bool host_ap_ip_ready; //AP mode, station client get ip from device successful.
    bool sta_ip_ready; //station mode, device get ip from host clinet successful.
    WIFI_MODE_e mode;

} wifi_config_t;

typedef void (*net_dowload_cb)(void *user_data);

int network_init(void);
int network_deinit(void);
int network_connect(void);
void network_wifi_module_set(int wifi_module);
int network_wifi_module_get(void);
int hostap_get_connect_count(void);
char *app_wifi_local_ip_get(void);
char *app_wifi_connected_phone_ip_get(void);

void hccast_start_services(void);
void hccast_stop_services(void);

bool network_service_enable_get(void);
void network_service_enable_set(bool start);
wifi_config_t *app_wifi_config_get(void);
char *network_get_upgrade_url(void);
int app_wifi_reconnect(hccast_wifi_ap_info_t *wifi_ap);
int app_wifi_switch_work_mode(WIFI_MODE_e wifi_mode);
char *app_get_connecting_ssid();
bool app_wifi_connect_status_get(void);
bool app_wifi_init_done(void);

int api_network_download(char *url, char *file_name, uint8_t *data, uint32_t size, \
        net_dowload_cb net_cb, void *user_data, bool block);

// #define HOSTAP_CHANNEL_24G  6
// #define HOSTAP_CHANNEL_5G   36

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
#endif    

