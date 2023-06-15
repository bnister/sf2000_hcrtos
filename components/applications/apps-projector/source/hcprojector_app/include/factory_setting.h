#include "app_config.h"

#include <stdio.h>
#include <hcuapi/sysdata.h>

#ifdef WIFI_SUPPORT
#include <hccast/hccast_wifi_mgr.h>
#endif

#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif
#define BLUETOOTH_MAC_LEN 6
#define BLUETOOTH_NAME_LEN 128

#define MAX_DEV_NAME    32
#define MAX_DEV_PSK 32
#define MAX_WIFI_SAVE   5
#define SSID_NAME "Hccast"
#define DEVICE_PSK "12345678"

#define HOSTAP_CHANNEL_24G  1
#define HOSTAP_CHANNEL_5G   36
#define HOSTAP_CHANNEL_AUTO 0

#define MAC_ADDR_LEN    6
#define VMOTOR_COUNT_SETP_FORWARD_MAX 37

typedef enum {
    P_PICTURE_MODE,
    P_CONTRAST,
    P_BRIGHTNESS,
    P_SHARPNESS,
    P_COLOR,
    P_HUE,
    P_COLOR_TEMP,
    P_NOISE_REDU,
    P_SOUND_MODE,
    P_BALANCE,
    P_BT_SETTING,
    P_BT_MAC,
    P_BT_NAME,
    P_TREBLE,
    P_BASS,
    P_OSD_LANGUAGE,
    P_ASPECT_RATIO,
    P_CUR_CHANNEL,
    P_FLIP_MODE,
    P_OSD_TIME,
    P_VOLUME,
    P_RESTORE,
    P_UPDATE,
    P_AUTOSLEEP,
    P_KEYSTONE,
    P_KEYSTONE_TOP,
    P_KEYSTOME_BOTTOM,
    P_VERSION_INFO,
    P_DEV_PRODUCT_ID,
    P_DEV_VERSION,
    P_MIRROR_MODE,
    P_AIRCAST_MODE,
    P_MIRROR_FRAME,
    P_BROWSER_LANGUAGE,
    P_SYS_RESOLUTION,
    P_DEVICE_NAME,
    P_DEVICE_PSK,
    P_WIFI_MODE,
    P_WIFI_CHANNEL,
    P_MIRROR_ROTATION,
    P_MIRROR_VSCREEN_AUTO_ROTATION,
    P_DE_TV_SYS,
    P_VMOTOR_COUNT,
    P_WIFI_ONOFF,
	P_WIFI_AUTO_CONN,
    P_WIFI_CHANNEL_24G,
    P_WIFI_CHANNEL_5G,
    P_WINDOW_SCALE,
    P_SYS_ZOOM_DIS_MODE,
    P_SYS_ZOOM_OUT_COUNT,

} projector_sys_param;


// typedef enum flip_type{
//     FLIP_NORMAL,
//     FLIP_WH_REVERSE,
//     FLIP_MIRROR,
//     FLIP_180,
// }flip_type_e;



typedef enum picture_mode{
    PICTURE_MODE_STANDARD,
    PICTURE_MODE_DYNAMIC,

    PICTURE_MODE_MILD,
    PICTURE_MODE_USER,
}picture_mode_e;

typedef enum color_temp_{
    COLOR_TEMP_COLD,
    COLOR_TEMP_STANDARD,
    COLOR_TEMP_WARM,
}color_temp_e;

typedef enum noise_redu_{
    NOISE_REDU_OFF,
    NOISE_REDU_LOWER,
    NOISE_REDU_MEDI,
    MOISE_REDU_HIGH,
}noise_redu_e;

typedef enum sound_mode{
    SOUND_MODE_STANDARD,
    SOUND_MODE_MOVIE,
    SOUND_MODE_MUSIC,
    SOUND_MODE_SPORTS,
    SOUND_MODE_USER
} sound_mode_e;

typedef enum bluetooth_onoff_{
    BLUETOOTH_OFF,
    BLUETOOTH_ON
} bluetooth_onoff;

typedef enum auto_sleep{
    AUTO_SLEEP_OFF,
    AUTO_SLEEP_ONE_HOUR,
    AUTO_SLEEP_TWO_HOURS,
    AUTO_SLEEP_THREE_HOURS
} auto_sleep_e;

typedef enum aspect_ratio_{
    ASPECT_RATIO_AUTO,
    ASPECT_RATIO_4_3,
    ASPECT_RATIO_16_9,
    ZOOM_IN,
    ZOOM_OUT
}aspect_ratio_e;

typedef enum osd_time_{
    OSD_TIME_OFF,
    OSD_TIME_5S,
    OSD_TIEM_10S,
    OSD_TIME_15S,
    OSD_TIEM_20S,
    OSD_TIME_25S,
    OSD_TIME_30S
} osd_time_e;

typedef struct pictureset{
    uint8_t picture_mode;
    uint8_t contrast;
    uint8_t brightness;
    uint8_t sharpness;
    uint8_t color;
    uint8_t hue;
    uint8_t color_temp;
    uint8_t noise_redu;
}pictureset_t;

typedef struct soundset{
    uint8_t sound_mode;
    int8_t balance;
    uint8_t bt_setting;
#ifdef BLUETOOTH_SUPPORT    
    struct bluetooth_slave_dev bt_dev;
#endif    
    int8_t treble;
    int8_t bass;
}soundset_t;

typedef struct optionset{
    uint8_t osd_language;
    uint8_t aspect_ratio;
    uint keystone_top_w;
    uint keystone_bottom_w;
    uint8_t auto_sleep;
    uint8_t osd_time;
    uint8_t resved[2];
}optionset_t;

typedef enum{
    APP_TV_SYS_480P = 1,
    APP_TV_SYS_576P,
    APP_TV_SYS_720P,
    APP_TV_SYS_1080P,
    APP_TV_SYS_4K,

    APP_TV_SYS_AUTO,

}app_tv_sys_t;

typedef struct wifi_cast_setting{
    char cast_dev_name[MAX_DEV_NAME];
    char cast_dev_psk[MAX_DEV_PSK];
    char mac_addr[MAC_ADDR_LEN];
    char cast_dev_name_changed; //if the flag is set, used the cast_dev_name for cast name
#ifdef WIFI_SUPPORT    
    hccast_wifi_ap_info_t wifi_ap[MAX_WIFI_SAVE];
    uint16_t wifi_auto_conn;
    uint8_t wifi_onoff;
#endif    
    int browserlang;//1-englist;2-chn;3-traditional chn
    int ratio;
    int mirror_mode;//1-standard.
    int mirror_frame;//0-30FPS,1-60FPS
    int aircast_mode;//0-mirror-stream, 1-mirror-only, 2-Auto
    int wifi_mode;          // 1: 24G, 2: 5G, 3: 60G (res)
    int wifi_ch;            // 24G hostap channel
    int mirror_rotation;    //0-disable, 1-enable.
    int mirror_vscreen_auto_rotation;//0-disable, 1-enable.
    int wifi_ch5g;          // 5G hostap wifi channel
}wifi_cast_setting_t;

typedef struct sys_scale_setting{
    // int left;
    // int top;
    // int h_mul;
    // int v_mul;
    // int main_layer_h;
    // int main_layer_v;
    int dis_mode;
    int zoom_out_count;
} sys_scale_setting_t;

typedef struct appdata{
    uint8_t cur_channel;
    uint8_t flip_mode;
    uint8_t volume;
    uint8_t vmotor_count;
    app_tv_sys_t resolution;

    pictureset_t pictureset; 
    soundset_t soundset;
    optionset_t optset;
    wifi_cast_setting_t cast_setting;
    #ifdef SYS_ZOOM_SUPPORT
    sys_scale_setting_t scale_setting;
    #endif

}app_data_t;


typedef struct sys_param{
    // boot/upgrade parameters, 
    struct sysdata sys_data;  
    //projector prameters
    struct appdata app_data;

}sys_param_t;

void projector_factory_init(void);
void projector_factory_reset(void);
int projector_sys_param_load(void);
int projector_sys_param_save(void);

extern sys_param_t * projector_get_sys_param(void);
void projector_set_some_sys_param(projector_sys_param param, int v);
int projector_get_some_sys_param(projector_sys_param param);
#ifdef BLUETOOTH_SUPPORT
  unsigned char* projector_get_bt_mac();
  char* projector_get_bt_name();
  void projector_set_bt_dev(struct bluetooth_slave_dev *dev);
  struct bluetooth_slave_dev *projector_get_bt_dev();
#endif

#ifdef WIFI_SUPPORT
int8_t get_save_wifi_flag();
void set_save_wifi_flag_zero();
int sysdata_check_ap_saved(hccast_wifi_ap_info_t* check_wifi);
void sysdata_wifi_ap_save(hccast_wifi_ap_info_t *wifi_ap);
hccast_wifi_ap_info_t *sysdata_get_wifi_info(char* ssid);
bool sysdata_wifi_ap_get(hccast_wifi_ap_info_t *wifi_ap);
hccast_wifi_ap_info_t *sysdata_get_wifi_info_by_index(int i);
int sysdata_get_wifi_index_by_ssid(char *ssid);
int sysdata_get_saved_wifi_count();
void sysdata_wifi_ap_set_auto(int index);
void sysdata_wifi_ap_set_nonauto(int index);
bool sysdata_wifi_ap_get_auto(int index);
#endif

void sysdata_wifi_ap_delete(int index);

void sysdata_app_tv_sys_set(app_tv_sys_t app_tv_sys);
int sysdata_init_device_name(void);
char* projector_get_version_info();

