#ifndef __HC_AIRCAST_AV_PLAYER_H__
#define __HC_AIRCAST_AV_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    HCCAST_AIR_AUDIO_NONE,
    HCCAST_AIR_AUDIO_PCM,
    HCCAST_AIR_AUDIO_AAC_ELD,
} hccast_air_audio_type_e;

int hccast_air_video_open();
void hccast_air_video_close();
int hccast_air_video_feed(unsigned char *data, unsigned int len,
                          unsigned int pts, int last_slice, unsigned int width, unsigned int height);
int hccast_air_audio_open();
void hccast_air_audio_close();
int hccast_air_audio_feed(int type, unsigned char *buf, int length, unsigned int pts);

#ifdef __cplusplus
}
#endif
#endif
