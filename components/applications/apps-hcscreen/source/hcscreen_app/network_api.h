/**
 * NETWORK_SUPPORT.h
 */

#ifndef __NETWORK_SUPPORT_H__
#define __NETWORK_SUPPORT_H__

#include <hccast/hccast_wifi_mgr.h>

#ifdef __cplusplus
extern "C" {
#endif

int network_init(void);
int network_deinit(void);
int network_connect(void);

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
void network_wifi_module_set(int wifi_module);
int network_wifi_module_get(void);
int hostap_get_connect_count(void);
char *wifi_local_ip_get(void);
char *wifi_connected_phone_ip_get(void);
#else // Ethernet
char *eth_local_ip_get(void);
#endif
#endif // NETWORK_SUPPORT
void hccast_start_services(void);
void hccast_stop_services(void);
int app_get_wifi_connect_status(void);
void app_set_wifi_connect_status(int status);
int network_disconnect(void);
int network_plugout_reboot(void);

char *app_get_connecting_ssid();
//#define HOSTAP_CHANNEL_24G  6     //ref data_mgr.h
//#define HOSTAP_CHANNEL_5G   36    // ref data_mgr.h

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif    

