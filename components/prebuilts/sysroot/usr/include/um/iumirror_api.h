#ifndef __IUMIRROR_API_H__
#define __IUMIRROR_API_H__

enum IUM_EVENT_TYPE
{
    IUM_EVT_DEVICE_ADD,
    IUM_EVT_DEVICE_REMOVE,
    IUM_EVT_MIRROR_START,
    IUM_EVT_MIRROR_STOP,
    IUM_EVT_SAVE_PAIR_DATA,
    IUM_EVT_GET_PAIR_DATA,
    IUM_EVT_NEED_USR_TRUST,
    IUM_EVT_USR_TRUST_DEVICE,
    IUM_EVT_CREATE_CONN_FAILED,
    IUM_EVT_CANNOT_GET_AV_DATA,
    IUM_EVT_UPG_DOWNLOAD_PROGRESS,
    IUM_EVT_GET_UPGRADE_DATA,
    IUM_EVT_SAVE_UUID,
    IUM_EVT_CERT_INVALID,
    IUM_EVT_SET_ROTATE,
    IUM_EVT_FAKE_LIB,
    IUM_EVT_NO_DATA,
};

enum IUM_IO_CMD
{
    IUM_CMD_SET_DATA_EN = 1,
    IUM_CMD_SET_AV_FUNC,
    IUM_CMD_SET_UUID,
    IUM_CMD_RESET_MIRRORING,
    IUM_CMD_SET_EVENT_CB,
    IUM_CMD_INIT,
    IUM_CMD_STOP_MIRRORING,
    IUM_CMD_SET_RESOLUTION,
    IUM_CMD_SET_UPGRADE_BUF,
    IUM_CMD_ENABLE_BUFFERING,
};

typedef struct
{
    unsigned char *buf;
    unsigned int len;
    unsigned int crc;
    unsigned char crc_chk_ok;
} ium_upg_buf_obj_t;

typedef struct
{
    int (*_video_open)(int um_type);
    void (*_video_close)();
    int (*_video_feed)(unsigned char *data, unsigned int len,
                       unsigned long long pts, unsigned int rotate, int last_slice,
                       unsigned int width, unsigned int height, unsigned char b_airplay);
    void (*_video_rotate)(int rotate_en);
    int (*_audio_open)();
    void (*_audio_close)();
    int (*_audio_feed)(int type, unsigned char *buf, int length, unsigned long long pts);
    void (*_set_timebase)(unsigned int time_ms);
    void (*_av_reset)();
    void (*_video_pause)(int pause);
    void (*_set_fps)(unsigned int fps);
} ium_av_func_t;

typedef void (*ium_evt_cb)(int event_type, void *param1, void *param2);

void ium_ioctl(int cmd, void *param1, void *param2);
void ium_start(void);
void ium_stop(void);

#endif //_IUM_SERVICE_H_
