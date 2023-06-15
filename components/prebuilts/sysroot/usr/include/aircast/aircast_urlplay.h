#ifndef __AIRCAST_URLPLAY_H__
#define __AIRCAST_URLPLAY_H__

typedef enum
{
    AIRCAST_MEDIA_MUSIC = 0,
    AIRCAST_MEDIA_MOVIE = 20,
    AIRCAST_MEDIA_PHOTO = 40,
    AIRCAST_MEDIA_INVALID = -1
} AircastMedia_E;



typedef enum
{
    AIRCAST_MEDIA_STATUS_STOPPED = 0,
    AIRCAST_MEDIA_STATUS_PLAYING = 1,
    AIRCAST_MEDIA_STATUS_PAUSED = 2,
    AIRCAST_MEDIA_STATUS_ERROR = 3,
    AIRCAST_MEDIA_STATUS_BUFFERING = 64,
    AIRCAST_MEDIA_STATUS_INVALID = -1
} AircastMediaStatus_E;


typedef struct
{
    int ret;
    AircastMediaStatus_E status;
    int currTime;
    int totalTime;
	int volume;
} AircastPlayerState_T;


#endif 
