#ifndef __AUMIRROR_API_H__
#define __AUMIRROR_API_H__

enum AUM_EVENT_TYPE
{
    AUM_EVT_DEVICE_ADD = 1,
    AUM_EVT_DEVICE_REMOVE,
    AUM_EVT_MIRROR_START,
    AUM_EVT_MIRROR_STOP,
    AUM_EVT_SET_TVSYS,
    AUM_EVT_IGNORE_NEW_DEVICE,
    AUM_EVT_AV_CONFIG,
    AUM_EVT_SERVER_MSG,
    AUM_EVT_UPG_DOWNLOAD_PROGRESS,
    AUM_EVT_GET_UPGRADE_DATA,
    AUM_EVT_SET_SCREEN_ROTATE,
    AUM_EVT_SET_AUTO_ROTATE,
    AUM_EVT_SET_FULL_SCREEN,
    AUM_EVT_SET_SCREEN_MODE,
};

enum AUM_IO_CMD
{
    AUM_CMD_SET_SETTING = 1,
    AUM_CMD_SET_APP_URL,
    AUM_CMD_SET_UPG_STATUS,
    AUM_CMD_SET_DATA_EN,
    AUM_CMD_SET_VIDEO_LOW_LATENCY,
    AUM_CMD_SET_AOA_DESCRIPTION,
    AUM_CMD_SET_AV_FUNC,
    AUM_CMD_SET_VIDEO_ROTATE,
    AUM_CMD_SET_EVENT_CB,
    AUM_CMD_STOP_MIRRORING,
    AUM_CMD_SET_UPGRADE_BUF,
};

enum AUM_MIRROR_MODE
{
    AUM_MIRROR_RESAUTO = 0,
    AUM_MIRROR_1080P60,
    AUM_MIRROR_720P60,
    AUM_MIRROR_480P60,
};

enum AUM_UPG_STATUS
{
    AUM_UPG_DOWLOADING = 1,
    AUM_UPG_UPDATING,
    AUM_UPG_FINISH,
    AUM_UPG_DATA_ERROR,
    AUM_UPG_FAILED,
};

typedef struct
{
    unsigned char *buf;
    unsigned int len;
} aum_upg_buf_obj_t;

typedef struct
{
    unsigned int resolution;
    unsigned int tvsys;
    unsigned int support_audio;
    unsigned int sw_version;
    char product_id[32];
    char fw_url[256];
    unsigned int screen_rotate;
	unsigned int auto_rotate;
	unsigned int full_screen;
} aum_setting_info_t;

typedef struct
{
    unsigned int mode;  // 0 - Horizontal; 1 - Vertical
    unsigned int video_width;
    unsigned int video_height;
    unsigned int screen_width;
    unsigned int screen_height;
} aum_screen_mode_t;

typedef struct
{
    int (*_video_open)(int um_type);
    void (*_video_close)();
    int (*_video_feed)(unsigned char *data, unsigned int len,
                       unsigned long long pts, unsigned int rotate, int last_slice,
                       unsigned int width, unsigned int height, unsigned char play_mode);
    void (*_video_rotate)(int rotate_en);
	void (*_video_mode)(int b_fullscreen);
    void (*_screen_mode)(aum_screen_mode_t *screen_mode);
    int (*_audio_open)();
    void (*_audio_close)();
    int (*_audio_feed)(int type, unsigned char *buf, int length, unsigned long long pts);
    void (*_set_timebase)(unsigned int time_ms);
} aum_av_func_t;

typedef void (*anf_evt_cb)(int event_type, void *param1, void *param2);

void aum_ioctl(int cmd, void *param1, void *param2);
void aum_start(void);
void aum_stop(void);

#endif
