/*
data_mgr.h
used for save the config data, and read config data from nonvolatile memory
(nor/nand flash)

 */

#ifndef __DATA_MGR_H__
#define __DATA_MGR_H__


#ifdef __cplusplus
extern "C" {
#endif


#ifdef WIFI_SUPPORT
#include <hccast/hccast_wifi_mgr.h>
#endif

#include <hcuapi/dis.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/sysdata.h>

#define MAX_DEV_NAME	32
#define MAX_DEV_PSK	32
#define MAX_WIFI_SAVE	5
#define SSID_NAME "Hccast"
#define DEVICE_PSK "12345678"

#define HOATAP_CHANNEL_24G  6
#define HOATAP_CHANNEL_5G   36
#define MAC_ADDR_LEN	6

typedef enum{
	APP_TV_SYS_480P = 1,
	APP_TV_SYS_576P,
	APP_TV_SYS_720P,
	APP_TV_SYS_1080P,
	APP_TV_SYS_4K,

	APP_TV_SYS_AUTO,

}app_tv_sys_t;

#ifndef WIFI_SUPPORT
typedef struct _hccast_wifi_ap_info_t_
{
    char    ssid[16];               // AP's SSID
    int     encryptMode;       // Encrypt mode
    char    keyIdx;                                 // for WEP key index
    char    pwd[16];                  // Password
    char    numCharInPwd;                           // Number of characters in pwd
    int     quality;                                // rssi strength
    int     special_ap;                             // 1: hiden ap, 0: normal
    int     freq;
} hccast_wifi_ap_info_t;
#endif

typedef struct app_data{
	uint32_t data_crc32; //do not modify.
	
	char cast_dev_name[MAX_DEV_NAME];
	char cast_dev_psk[MAX_DEV_PSK];
	char mac_addr[MAC_ADDR_LEN];
	char cast_dev_name_changed; //if the flag is set, used the cast_dev_name for cast name
	hccast_wifi_ap_info_t wifi_ap[MAX_WIFI_SAVE];
	app_tv_sys_t resolution;
	int browserlang;//1-englist;2-chn;3-traditional chn
	int ratio;
	int mirror_mode;//1-standard.
    int mirror_frame;//0-30FPS,1-60FPS
    int aircast_mode;//0-mirror-stream, 1-mirror-only, 2-Auto
    int wifi_mode;   // 1: 2.4G, 2: 5G, 3: 60G (res)
    int wifi_ch;     // hostap channel
}app_data_t;

typedef struct sysdata sys_data_t;

int data_mgr_load(void);
app_data_t *data_mgr_app_get(void);
sys_data_t *data_mgr_sys_get(void);
int data_mgr_save(void);
void data_mgr_wifi_ap_save(hccast_wifi_ap_info_t *wifi_ap);
void data_mgr_wifi_ap_delete(int index);
void data_mgr_wifi_ap_clear(int index);
bool data_mgr_wifi_ap_get(hccast_wifi_ap_info_t *wifi_ap);
int data_mgr_check_ap_saved(hccast_wifi_ap_info_t* check_wifi);
int data_mgr_init_device_name(void);
hccast_wifi_ap_info_t *data_mgr_get_wifi_info(char* ssid);
char *data_mgr_get_device_name(void);
char *data_mgr_get_device_psk(void);

int date_mgr_get_device_wifi_mode(void);
int date_mgr_get_device_wifi_channel(void);

void data_mgr_app_tv_sys_set(app_tv_sys_t app_tv_sys);
enum TVTYPE data_mgr_tv_type_get(void);
uint8_t data_mgr_volume_get(void);
void data_mgr_volume_set(uint8_t vol);
void data_mgr_factory_reset(void);
void data_mgr_cast_dev_name_changed_set(int set);

app_tv_sys_t data_mgr_app_tv_sys_get(void);
int data_mgr_de_tv_sys_get(void);
void data_mgr_mirror_mode_set(int mode);
void data_mgr_aircast_mode_set(int mode);
void data_mgr_mirror_frame_set(int frame);
void data_mgr_browserlang_set(int language);
void data_mgr_cast_dev_name_changed_set(int set);

#ifdef __cplusplus
} /*extern "C"*/
#endif



#endif