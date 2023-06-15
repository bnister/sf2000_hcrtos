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
#include <hcuapi/avsync.h>

#include <sys/ioctl.h>
#include <hccast_air.h>
#include <hcuapi/snd.h>
#include <errno.h>
#include <sys/time.h>
#include "hccast_air_decoder.h"
#include "hccast_air_avplayer.h"
#include <hccast_log.h>
#include <hccast_com.h>

// ffplay -pixel_format yuv420p -video_size 768x320 -framerate 25 out.yuv
#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))

struct h264_decoder
{
    struct video_config cfg;
    int fd;
};

struct aac_decoder
{
    struct audio_config cfg;
    int fd;
};

void *hccast_air_h264_decode_init(int width, int height, uint8_t *extradata, int size, int framenum)
{
    struct h264_decoder *p = malloc(sizeof(struct h264_decoder));
    memset(&p->cfg, 0, sizeof(struct video_config));

    //set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX);

    p->cfg.codec_id = HC_AVCODEC_ID_H264;
    p->cfg.sync_mode = AVSYNC_TYPE_UPDATESTC;//video master.
    p->cfg.quick_mode = 3;
    hccast_log(LL_INFO,"video sync_mode: %d\n", p->cfg.sync_mode);
    p->cfg.decode_mode = VDEC_WORK_MODE_KSHM;

    p->cfg.pic_width = width;
    p->cfg.pic_height = height;
    p->cfg.frame_rate = framenum * 1000; //60*1000;
    hccast_log(LL_NOTICE,"frame_rate: %d\n", p->cfg.frame_rate);

    p->cfg.pixel_aspect_x = 1;
    p->cfg.pixel_aspect_y = 1;
    p->cfg.preview = 0;
    p->cfg.extradata_size = size;
    p->cfg.rotate_enable = 1;//default enable.


    //we should check it correctly.
    if (extradata && size > 0)
        memcpy(p->cfg.extra_data, extradata, size);

    p->fd = open("/dev/viddec", O_RDWR);
    if (p->fd < 0)
    {
        hccast_log(LL_ERROR,"Open /dev/viddec error.\n");
        return NULL;
    }

    if (ioctl(p->fd, VIDDEC_INIT, &(p->cfg)) != 0)
    {
        hccast_log(LL_ERROR,"Init viddec error.\n");
        close(p->fd);
        free(p);
        return NULL;
    }

    ioctl(p->fd, VIDDEC_START, 0);

    //hccast_log(LL_WARNING,"fd: %d\n", p->fd);
    return p;
}

int hccast_air_h264_decode(void *phandle, uint8_t *video_frame, size_t packet_size, unsigned int pts, 
                            int rotate_mode, int flip_mirror)
{
    struct h264_decoder *p = (struct h264_decoder *)phandle;
    AvPktHd pkthd = { 0 };
    pkthd.pts = pts;
    pkthd.dur = 0;
    pkthd.size = packet_size;
    pkthd.flag = AV_PACKET_ES_DATA;
    int head_try_times = 100;//1s
    int data_try_times = 100;//1s
    // ALOGI("video_frame: %p, packet_size: %d", video_frame, packet_size);


    pkthd.video_rotate_mode = rotate_mode;
    pkthd.video_mirror_mode = flip_mirror;

    while(head_try_times)
    {
    	if(write(p->fd, (uint8_t *)&pkthd, sizeof(AvPktHd)) !=sizeof(AvPktHd))
    	{
            hccast_log(LL_ERROR,"Write VPktHd fail\n");
            head_try_times--;
            usleep(10*1000);	
    	}
    	else
    	{
            break;
    	}
    }

    while(data_try_times)
    {
    	if(write(p->fd, video_frame, packet_size) !=packet_size)
    	{
            hccast_log(LL_ERROR,"Write video_frame error fail\n");
            data_try_times--;
            usleep(10*1000);	
    	}
    	else
    	{
            break;
    	}
    }
	
    return 0;
}

void hccast_air_h264_decoder_flush(void *phandle)
{
    float rate = 1;
    struct h264_decoder *p = (struct h264_decoder *)phandle;
    ioctl(p->fd, VIDDEC_FLUSH, &rate);
}

void hccast_air_h264_decoder_destroy(void *phandle,int close_vp)
{
    struct h264_decoder *p = (struct h264_decoder *)phandle;
    struct vdec_rls_param rls_param = {0};
    //set_aspect_mode(DIS_TV_AUTO, DIS_PILLBOX);
    if (!p)
        return;
        
    if(close_vp == 0)
    {
        rls_param.closevp = 0;
        rls_param.fillblack = 0;  
        ioctl(p->fd , VIDDEC_RLS , &rls_param);
    }
    
  
    if (p->fd > 0)
    {
        close(p->fd);
    }
    free(p);
}

void *hccast_air_audio_decoder_init(int bits, int channels, int samplerate, int audio_type)
{
    struct aac_decoder *p = (struct aac_decoder *)malloc(sizeof(struct aac_decoder));
	
#ifdef HC_RTOS
    int ret = -1;
    ret = hccast_snd_dev_open(channels,bits,samplerate,SND_PCM_FORMAT_S16_LE);
    if(ret < 0)
    {
    	hccast_log(LL_ERROR,"hccast_snd_dev_open error.\n");
    	return NULL;
    }
    p->fd = -1;
    return p;
#else

    memset(&p->cfg, 0, sizeof(struct audio_config));
    if(audio_type == HCCAST_AIR_AUDIO_AAC_ELD)
    {
    	p->cfg.codec_id = HC_AVCODEC_ID_AAC;
    	p->cfg.sync_mode = AVSYNC_TYPE_SYNCSTC;
    	p->cfg.bits_per_coded_sample = bits;
    	p->cfg.channels = channels;
    	p->cfg.sample_rate = samplerate;
    	p->cfg.codec_tag = MKTAG('e', 'l', 'f', ' ');
    }
    else if(audio_type == HCCAST_AIR_AUDIO_PCM)
    {
    	p->cfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;
    	p->cfg.sync_mode = AVSYNC_TYPE_SYNCSTC;
    	p->cfg.bits_per_coded_sample = bits;
    	p->cfg.channels = channels;
    	p->cfg.sample_rate = samplerate;
    }
    else
    {
    	hccast_log(LL_WARNING,"NOT SUPPORT AUDIO TYPE.\n");
    	return NULL;
    }
    p->cfg.snd_devs = hccast_com_media_control(HCCAST_CMD_SND_DEVS_GET, 0);


    p->fd = open("/dev/auddec", O_RDWR);
    if (p->fd < 0)
    {
        hccast_log(LL_ERROR,"Open /dev/auddec error.\n");
        free(p);
        return NULL;
    }

    if (ioctl(p->fd, AUDDEC_INIT, &p->cfg) != 0)
    {
        hccast_log(LL_ERROR,"Init auddec error.\n");
        close(p->fd);
        free(p);
        return NULL;
    }

    ioctl(p->fd, AUDDEC_START, 0);
    return p;
#endif	
}

int hccast_air_audio_decode(void *phandle, uint8_t *audio_frame, size_t packet_size,unsigned int pts)
{
#ifdef HC_RTOS
    hccast_snd_dev_feed(audio_frame,packet_size,pts);
#else
    struct aac_decoder *p = (struct aac_decoder *)phandle;
    AvPktHd pkthd = { 0 };
    pkthd.dur = 0;
    pkthd.size = packet_size;
    pkthd.flag = AV_PACKET_ES_DATA;
    pkthd.pts = pts;
    if (write(p->fd, (uint8_t *)&pkthd, sizeof(AvPktHd)) !=
        sizeof(AvPktHd))
    {
        hccast_log(LL_ERROR,"Write APktHd fail\n");
        return -1;
    }

    if (write(p->fd, audio_frame, packet_size) != packet_size)
    {
        hccast_log(LL_ERROR,"Write audio_frame error fail\n");
        return -1;
    }
#endif
    return 0;
}

void hccast_air_audio_decoder_flush(void *phandle)
{
    struct aac_decoder *p = (struct aac_decoder *)phandle;
    if (p->fd > 0)
        ioctl(p->fd, AUDDEC_FLUSH, 0);
}

void hccast_air_audio_decoder_destroy(void *phandle)
{
    struct aac_decoder *p = (struct aac_decoder *)phandle;
    if (!p)
        return;
#ifdef HC_RTOS
    hccast_snd_dev_close();
#else
    if (p->fd > 0)
    {
        close(p->fd);
    }
#endif	
    free(p);
}


