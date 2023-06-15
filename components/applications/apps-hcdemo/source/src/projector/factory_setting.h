#include <stdio.h>
#include <hcuapi/sysdata.h>
#include <bluetooth.h>
#define BLUETOOTH_MAC_LEN 6
#define BLUETOOTH_NAME_LEN 128

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
    P_VOLUME,
    P_RESTORE,
    P_UPDATE,
    P_AUTOSLEEP,
    P_KEYSTONE,
    P_KEYSTONE_TOP,
    P_KEYSTOME_BOTTOM,
    P_INIT_ROTATE,
    P_INIT_H_FLIP,
    P_INIT_V_FLIP,
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

typedef enum {
    English = 0,
    Chinese,
    French,
    Russian,
    German,
    Japanese,
    Korean,
    Spanish,
    Portuguese,
} OSD_LANGUAGE;

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
    struct bluetooth_slave_dev bt_dev;
    int8_t treble;
    int8_t bass;
}soundset_t;

typedef struct optionset{
    uint8_t osd_language;
    uint8_t aspect_ratio;
    uint keystone_top_w;
    uint keystone_bottom_w;
    uint8_t auto_sleep;
    uint8_t resved[2];
}optionset_t;

typedef struct projector_setting{
    uint16_t init_rotate;//0 90 180 270
    uint8_t init_h_flip;//0 1
    uint8_t init_v_flip;//0 1
    uint8_t cur_channel;
    uint8_t flip_mode;
    uint8_t volume;
    uint8_t resved[1];

    pictureset_t pictureset; 
    soundset_t soundset;
    optionset_t optset;

}projector_setting_t;
typedef struct sys_param{
    // boot/upgrade parameters, 
    struct sysdata sysdata;  
    //projector prameters
    struct projector_setting projector_sysdata;

}sys_param_t;

extern void projector_factory_reset(void);
extern int projector_sys_param_load(void);
extern int projector_sys_param_save(void);
extern sys_param_t * projector_get_sys_param(void);
void projector_set_some_sys_param(projector_sys_param param, int v);
int projector_get_some_sys_param(projector_sys_param param);
unsigned char* projector_get_bt_mac();
char* projector_get_bt_name();
void projector_set_bt_dev(struct bluetooth_slave_dev *dev);
struct bluetooth_slave_dev *projector_get_bt_dev();
