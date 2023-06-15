#ifndef _HC_AIRCAST_H_
#define _HC_AIRCAST_H_

enum AIRCAST_EVENT_TYPE_C2A
{
    AIRCAST_EVT_NONE,
    AIRCAST_SET_URL,
    AIRCAST_STOP_PLAY,
    AIRCAST_GET_PLAY_INFO,
    AIRCAST_URL_SEEK,
    AIRCAST_URL_PAUSE,
    AIRCAST_URL_RESUME_PLAY,
    AIRCAST_MIRROR_START,
    AIRCAST_MIRROR_STOP,
    AIRCAST_SET_VOLUME,
    AIRCAST_PLAY_STATUS_CHANGED,
    AIRCAST_SET_PLAY_INFO,
    AIRCAST_GOT_AUDIO_MODE,
    AIRCAST_AUDIO_START,
    AIRCAST_AUDIO_STOP,
    AIRCAST_CONNECTIONS_ADD,
    AIRCAST_CONNECTIONS_REMOVE,
    AIRCAST_INVALID_CERT,
    AIRCAST_CONNRESET,
    AIRCAST_FAKE_LIB,
};

enum AIRCAST_EVENT_TYPE_A2C
{
    AIRCAST_A2C_NONE,
    AIRCAST_VIDEO_USEREXIT,
    AIRCAST_VIDEO_PAUSED,
    AIRCAST_VIDEO_PLAY,
    AIRCAST_VIDEO_LOADING,
    AIRCAST_VIDEO_ENDED,
    AIRCAST_USER_MIRROR_STOP,
    AIRCAST_USER_AUDIO_STOP,
    AIRCAST_MAX_EVNET,
};

enum AIRCAST_REQUEST_CMD
{    
    AIRCAST_GET_VOL_ID,
    AIRCAST_SET_LOG_EN,
    AIRCAST_SET_MLH_EN,
    AIRCAST_SET_VIDEO_WAIT_TIME,
    AIRCAST_SET_AUDIO_WAIT_TIME,
    AIRCAST_SET_MIRROR_MODE,
    AIRCAST_SET_AUDIO_DROP,
    AIRCAST_SET_VIDEO_LOW_LATENCY,
    AIRCAST_SET_YT_STATIC_BUF,
    AIRCAST_GET_MDNS_ADD_CALLBACK,
    AIRCAST_SET_AV_FUNC,
    AIRCAST_SET_AES_FUNC,
};

enum AIRCAST_MIRROR_MODE
{
    AIRCAST_MIRROR_ONLY,
    AIRCAST_MIRROR_WITH_HTTP_STREAM,
};

struct air_url_info
{
    unsigned char *url;
    unsigned int media_type;
    unsigned int cid;
};

struct mlhls_m3u8_info
{
    unsigned int id;
    int type;    
    int total_time;
    int total_len;
    char *m3u8;
    char *bs_url;
    unsigned char url_type;
};

typedef struct
{
    int (*_video_open)();
    void (*_video_close)();
    int (*_video_feed)(unsigned char *data, unsigned int len,
                       unsigned int pts, int last_slice,
                       unsigned int width, unsigned int height);
    int (*_audio_open)();
    void (*_audio_close)();
    int (*_audio_feed)(int type, unsigned char *buf, int length, unsigned int pts);
} aircast_av_func_t;

typedef struct
{
    void* (*dsc_aes_ctr_open)(int mmap_size);
    void* (*dsc_aes_cbc_open)(int mmap_size);
    void (*dsc_ctx_destroy)(void*ctx);
    int (*dsc_aes_decrypt)(void* ctx, unsigned char *key, unsigned char* iv, unsigned char *input, unsigned char *output, int size);
    int (*dsc_aes_encrypt)(void* ctx, unsigned char *key, unsigned char* iv, unsigned char *input, unsigned char *output, int size);
} aircast_dsc_func_t;


typedef void (*evt_cb)(int event_type, void *param);

int aircast_service_init();
int aircast_service_start(char *name,char* ifname);
int aircast_service_stop(void);
void aircast_set_event_callback(evt_cb event_cb);
int aircast_set_resolution(int width,int height,int refreshRate);
void aircast_event_notify(int event_type,void *param);
int aircast_ioctl(int req_cmd, void *param1,void *param2);
void aircast_log_level_set(int level);

#endif
