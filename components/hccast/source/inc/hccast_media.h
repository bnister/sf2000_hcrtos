#ifndef __HCCAST_MEDIA_H_
#define __HCCAST_MEDIA_H_
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    HCCAST_MEDIA_STATUS_STOP,
    HCCAST_MEDIA_STATUS_PLAYING,
    HCCAST_MEDIA_STATUS_PAUSED,
    HCCAST_MEDIA_STATUS_BUFFERING
} hccast_media_status_e;


typedef enum
{
    HCCAST_MEDIA_URL_DLNA,
    HCCAST_MEDIA_URL_AIRCAST,
} hccast_media_url_mode_e;

typedef enum
{
    HCCAST_MEDIA_MOVIE,
    HCCAST_MEDIA_MUSIC,
    HCCAST_MEDIA_PHOTO,
    HCCAST_MEDIA_INVALID,
} hccast_media_type_e;


typedef enum
{
    HCCAST_MEDIA_EVENT_URL_FROM_DLNA,
    HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST,
    HCCAST_MEDIA_EVENT_PARSE_END,
    HCCAST_MEDIA_EVENT_PLAYING,
    HCCAST_MEDIA_EVENT_PAUSE,
    HCCAST_MEDIA_EVENT_BUFFERING,
    HCCAST_MEDIA_EVENT_PLAYBACK_END,
    HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR, //video data decoded error
    HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR, //audio data decoded error
    HCCAST_MEDIA_EVENT_VIDEO_NOT_SUPPORT, //format not support
    HCCAST_MEDIA_EVENT_AUDIO_NOT_SUPPORT, //format not support
    HCCAST_MEDIA_EVENT_NOT_SUPPORT, // media container not support
    HCCAST_MEDIA_EVENT_URL_SEEK,
    HCCAST_MEDIA_EVENT_SET_VOLUME,
    HCCAST_MEDIA_EVENT_GET_MIRROR_ROTATION,
    HCCAST_MEDIA_EVENT_GET_FLIP_MODE,
} hccast_media_event_e;

typedef enum
{
    HCCAST_MEDIA_AIR_VIDEO_END,
    HCCAST_MEDIA_AIR_VIDEO_USEREXIT,
    HCCAST_MEDIA_AIR_VIDEO_PAUSE,
    HCCAST_MEDIA_AIR_VIDEO_PLAY,
    HCCAST_MEDIA_AIR_VIDEO_LOADING,
} hccast_media_air_event_e;


typedef struct
{
    void *player;
#ifdef HC_RTOS	
    QueueHandle_t msgid;
#else
    int msgid;
#endif
    int ready;
    hccast_media_status_e status;
    pthread_mutex_t mutex;
    pthread_t tid;
    int64_t duration;
    int64_t position;
    bool media_running;
	
    void *player1;//for audio url
    int ready1;//for audio url
    int is_double_url;
} hccast_media_player_t;

typedef enum
{
    HCCAST_MEDIA_YTB_VIDEO = 0,
    HCCAST_MEDIA_YTB_AUDIO,
}hccast_media_ytb_type_e;


typedef struct
{
    int size;//m3u8 buf size.
    char *data;//yt aircast m3u8 buf.
    hccast_media_ytb_type_e type;//audio or video
}hccast_media_ytb_m3u8_t;


typedef struct  
{
    int readding;	//mean this buf is readding by ffplayer. 
    int offset;	//how many m3u8 buf data has been read.//begin offset is 0.
    hccast_media_ytb_m3u8_t m3u8_info;
    pthread_mutex_t mutex;
} hccast_media_ytb_playlist_t;


typedef struct
{
    char* url;
    char* url1;
    hccast_media_ytb_m3u8_t *ytb_m3u8[2];
    hccast_media_url_mode_e url_mode;
    hccast_media_type_e media_type;
} hccast_media_url_t;

typedef void (*hccast_media_event_callback)(hccast_media_event_e event_type, void* param);
typedef void (*hccast_media_air_event_callback)(int event_type, void* param);


#ifdef __cplusplus
extern "C" {
#endif

void hccast_media_init(hccast_media_event_callback mp_cb);
void hccast_media_destroy(void);
void hccast_media_seturl(hccast_media_url_t* mp_url);
void hccast_media_resume(void);
void hccast_media_pause(void);
void hccast_media_stop(void);
void hccast_media_seek(int64_t position);
long hccast_media_get_duration(void);
void hccast_media_set_duration(void);

long hccast_media_get_position(void);
int hccast_media_get_status(void);
int hccast_media_get_volume(void);
void hccast_media_set_volume(int vol);

void hccast_media_ytb_playlist_init(void);
int hccast_media_ytb_set_m3u8_playlist(int id, hccast_media_ytb_m3u8_t* m3u8_info);
void hccast_media_ytb_update_m3u8_playlist(int id, char* m3u8, int m3u8_size);
int hccast_media_ytb_playlist_read(void * opaque, uint8_t *buf, int bufsize);
int hccast_media_ytb_check_m3u8_status(hccast_media_ytb_playlist_t* ytb_playlist);
void hccast_media_ytb_playlist_buf_reset(void);

void hccast_media_air_event_init(hccast_media_air_event_callback air_cb);
void *hccast_media_player_get(void);
void hccast_media_stop_by_key(void);
void hccast_media_seek_by_key(int64_t position);
void hccast_media_pause_by_key(void);
void hccast_media_resume_by_key(void);



#ifdef __cplusplus
}
#endif

#endif
