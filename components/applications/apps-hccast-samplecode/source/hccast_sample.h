#ifndef __HCCAST_SAMPLE_H
#define __HCCAST_SAMPLE_H
#include <hccast/hccast_wifi_mgr.h>

#define APLISTNUM 64
#define WIFI_MAX_SSID_LEN   64
#define WIFI_MAX_PWD_LEN    32
#define MAC_ADDR_LEN	6
#define MAX_WIFI_SAVE	5

#ifdef NETWORK_SUPPORT
    #ifdef WIFI_SUPPORT
    typedef struct _wifi_model_st_
    {
        char name[16];
        char desc[16];
        int  type;
    } wifi_model_st;

    #else // 
    #endif // WIFI_SUPPORT
#endif // NETWORK_SUPPORT

typedef enum{
	APP_TV_SYS_480P = 1,
	APP_TV_SYS_576P,
	APP_TV_SYS_720P,
	APP_TV_SYS_1080P,
	APP_TV_SYS_4K,
	APP_TV_SYS_AUTO,
}app_tv_sys_t;
typedef struct app_data{
	uint32_t data_crc32; //do not modify.
	char cast_dev_name[WIFI_MAX_SSID_LEN];
	char cast_dev_psk[WIFI_MAX_PWD_LEN];
	char mac_addr[MAC_ADDR_LEN];
	char cast_dev_name_changed; //if the flag is set, used the cast_dev_name for cast name
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
	hccast_wifi_ap_info_t wifi_ap[MAX_WIFI_SAVE];
#endif
#endif
	app_tv_sys_t resolution;
	int browserlang;//1-englist;2-chn;3-traditional chn
	int ratio;
	int mirror_mode;//1-standard.
    int mirror_frame;//0-30FPS,1-60FPS
    int aircast_mode;//0-mirror-stream, 1-mirror-only, 2-Auto
    int wifi_mode;   // 1: 2.4G, 2: 5G, 3: 60G (res)
    int wifi_ch;     // hostap channel
    int mirror_rotation;//0-disable, 1-enable.
}app_data_t;

int hccast_sample_wl_start(int argc, char **argv);
int hccast_sample_um_start(int argc, char **argv);
int hccast_sample_um_stop_mirroring();
int hccast_sample_hostapd_stop(int argc, char **argv);
int hccast_sample_hostapd_start(int argc, char **argv);
int hccast_sample_wifi_scan(int argc, char **argv);
int hccast_sample_wifi_connect(int argc, char **argv);
int hccast_sample_wifi_disconnect(int argc, char **argv);

#endif
