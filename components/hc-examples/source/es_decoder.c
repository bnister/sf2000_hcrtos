#define LOG_TAG "mediaplayer"
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include <errno.h>
#include "es_decoder.h"

#define ALOGI printf
#define ALOGE printf
#define ALOGD printf

// ffplay -pixel_format yuv420p -video_size 768x320 -framerate 25 out.yuv

struct h264_decoder {
    struct video_config cfg;
    int fd;
};

struct aac_decoder {
    struct audio_config cfg;
    int fd;
};

void *h264_decode_init(int width , int height , 
                       uint8_t *extradata , int size, 
                       int8_t rotate_enable, int8_t quick_mode_enable)
{
    struct h264_decoder *p = malloc(sizeof(struct h264_decoder));
    memset(&p->cfg , 0 , sizeof(struct video_config));

    set_aspect_mode(DIS_TV_16_9 , DIS_PILLBOX);

    p->cfg.codec_id = HC_AVCODEC_ID_H264;
    p->cfg.sync_mode = 0;
    p->cfg.quick_mode = quick_mode_enable;
    ALOGI("video sync_mode: %d\n" , p->cfg.sync_mode);
    p->cfg.decode_mode = VDEC_WORK_MODE_KSHM;

    p->cfg.pic_width = width;
    p->cfg.pic_height = height;
    p->cfg.frame_rate = 60 * 1000;
    ALOGE("frame_rate: %d\n" , (int)p->cfg.frame_rate);

    p->cfg.pixel_aspect_x = 1;
    p->cfg.pixel_aspect_y = 1;
    p->cfg.preview = 0;
    p->cfg.extradata_size = size;
    memcpy(p->cfg.extra_data , extradata , size);

    p->cfg.rotate_enable = rotate_enable;
    ALOGI("video rotate_enable: %d\n" , p->cfg.rotate_enable);



    p->fd = open("/dev/viddec" , O_RDWR);
    if( p->fd < 0 ) {
        ALOGE("Open /dev/viddec error.");
        return NULL;
    }

    if( ioctl(p->fd , VIDDEC_INIT , &( p->cfg )) != 0 ) {
        ALOGE("Init viddec error.");
        close(p->fd);
        free(p);
        return NULL;
    }

    ioctl(p->fd , VIDDEC_START , 0);

    ALOGI("fd: %d" , p->fd);
    return p;
}

int h264_decode(void *phandle , uint8_t *video_frame , size_t packet_size, uint32_t rotate_mode)
{
    struct h264_decoder *p = (struct h264_decoder *)phandle;
    AvPktHd pkthd = { 0 };
    pkthd.pts = -1;
    pkthd.dur = 0;
    pkthd.size = packet_size;
    pkthd.flag = AV_PACKET_ES_DATA;
    pkthd.video_rotate_mode = rotate_mode;

    // ALOGI("video_frame: %p, packet_size: %d", video_frame, packet_size);
    if( write(p->fd , (uint8_t *)&pkthd , sizeof(AvPktHd)) !=
       sizeof(AvPktHd) ) {
        ALOGE("Write AvPktHd fail\n");
        return -1;
    }

    if( write(p->fd , video_frame , packet_size) != (int)packet_size ) {
        ALOGE("Write video_frame error fail\n");
        ioctl(p->fd , VIDDEC_FLUSH , 0);
        return -1;
    }
    return 0;
}

void h264_decoder_flush(void *phandle)
{
    struct h264_decoder *p = (struct h264_decoder *)phandle;
    ioctl(p->fd , VIDDEC_FLUSH , 0);
}

void h264_decoder_destroy(void *phandle)
{
    struct h264_decoder *p = (struct h264_decoder *)phandle;
    set_aspect_mode(DIS_TV_AUTO , DIS_PILLBOX);
    if( !p )
        return;

    if( p->fd > 0 ) {
        close(p->fd);
    }
    free(p);
}

void *aac_decoder_init(int bits , int channels , int samplerate)
{
    struct aac_decoder *p =
        (struct aac_decoder *)malloc(sizeof(struct aac_decoder));
    memset(&p->cfg , 0 , sizeof(struct audio_config));
    p->cfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;
    p->cfg.sync_mode = 0;
    p->cfg.bits_per_coded_sample = bits;
    p->cfg.channels = channels;
    p->cfg.sample_rate = samplerate;

    p->fd = open("/dev/auddec" , O_RDWR);
    if( p->fd < 0 ) {
        ALOGE("Open /dev/auddec error.");
        free(p);
        return NULL;
    }

    if( ioctl(p->fd , AUDDEC_INIT , &p->cfg) != 0 ) {
        ALOGE("Init auddec error.");
        close(p->fd);
        free(p);
        return NULL;
    }

    ioctl(p->fd , AUDDEC_START , 0);
    return p;
}

int aac_decode(void *phandle , uint8_t *audio_frame , size_t packet_size)
{
    struct aac_decoder *p = (struct aac_decoder *)phandle;
    AvPktHd pkthd = { 0 };
    pkthd.dur = 0;
    pkthd.size = packet_size;
    pkthd.flag = AV_PACKET_ES_DATA;
    pkthd.pts = -1;
    if( write(p->fd , (uint8_t *)&pkthd , sizeof(AvPktHd)) !=
       sizeof(AvPktHd) ) {
        ALOGE("Write AvPktHd fail\n");
        return -1;
    }

    if( write(p->fd , audio_frame , packet_size) != (int)packet_size ) {
        ALOGE("Write audio_frame error fail\n");
        return -1;
    }

    return 0;
}

void aac_decoder_flush(void *phandle)
{
    struct aac_decoder *p = (struct aac_decoder *)phandle;
    ioctl(p->fd, AUDDEC_FLUSH, 0);
}

void aac_decoder_destroy(void *phandle)
{
    struct aac_decoder *p = (struct aac_decoder *)phandle;
    if( !p )
        return;

    if( p->fd > 0 ) {
        close(p->fd);
    }
    free(p);
}

void set_volume(uint8_t vol)
{
    int snd_fd = open("/dev/sndC0i2so" , O_WRONLY);
    if( snd_fd < 0 ) {
        ALOGE("Open /dev/sndC0i2so fail. %s" , strerror(errno));
        return;
    }
    ioctl(snd_fd , SND_IOCTL_SET_VOLUME , &vol);
}

__attribute__((weak)) void set_aspect_mode(dis_tv_mode_e ratio , dis_mode_e dis_mode)
{
    int fd = open("/dev/dis" , O_RDWR);
    ALOGD("ratio: %d, dis_mode: %d, fd: %d" , ratio , dis_mode , fd);
    if( fd < 0 )
        return;
    dis_aspect_mode_t aspect = { 0 };
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    ioctl(fd , DIS_SET_ASPECT_MODE , &aspect);
    close(fd);
}
