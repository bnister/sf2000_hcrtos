#ifndef __HC_AUDIO_COMMON_H__
#define __HC_AUDIO_COMMON_H__
#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>

#ifdef __cplusplus
extern "C" {
#endif

int hccast_audio_set_i2so_mute(int mute);
void hccast_set_aspect_mode(dis_tv_mode_e ratio ,
                            dis_mode_e dis_mode ,
                            dis_scale_avtive_mode_e active_mode);
void hccast_set_volume(int vol);
int hccast_get_volume(void);
void hccast_set_dis_zoom(av_area_t *src_rect ,
                         av_area_t *dst_rect ,
                         dis_scale_avtive_mode_e active_mode);
void hccast_get_miracast_picture_area(av_area_t *src_rect);

int hccast_snd_dev_open(int channels, int bitdepth,int rate, int format);
void hccast_snd_dev_close(void);
int hccast_snd_dev_feed(unsigned char*buf,int size,unsigned int pts);
void hccast_snd_dev_flush(void);

int hccast_get_audio_sync_thresh();
int hccast_set_audio_sync_thresh(int ms);


#ifdef __cplusplus
}
#endif

#endif
