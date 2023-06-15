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
#include <sys/time.h>
#include "decoder.h"
#define ALOGD printf
#define ALOGI printf
#define ALOGE printf

// ffplay -pixel_format yuv420p -video_size 768x320 -framerate 25 out.yuv

struct h264_decoder {
	struct video_config cfg;
	int fd;
};

struct aac_decoder {
	struct audio_config cfg;
	int fd;
};
#define AIRPLAY_FRAME_RATE
#ifdef AIRPLAY_FRAME_RATE
struct frame_rate
{
	char *name;
	double frame_cnts;
	int max_frame_cnts;
	double duration;
	double start_time;
	int size;
};
static struct frame_rate video_frame_rate = {
	.name = "video", 
	.max_frame_cnts = 30*2*3,
	.duration = 3.0,
};
static struct frame_rate audio_frame_rate = {
	.name = "audio",
	.max_frame_cnts = 44100*2*3,
	.duration = 3.0,
};
void print_frame_rate(struct frame_rate *rate, double count)
{
	struct timeval tv;
	double duration;
	if(rate->start_time == 0){
		gettimeofday(&tv, NULL);
		rate->start_time = tv.tv_sec + tv.tv_usec/1000000.0;
		rate->frame_cnts = count;
		return;
	}
	rate->frame_cnts += count;
	double cur_time;
	gettimeofday(&tv, NULL);
	cur_time = tv.tv_sec + tv.tv_usec/1000000.0;
	duration =  cur_time - rate->start_time;
	if(rate->frame_cnts > rate->max_frame_cnts || duration > rate->duration){
		double r;
		r = rate->frame_cnts / duration;
		printf("---%-6s frame rate: %-10.2f, frame_cnts: %-10.0f, duration: %-2.2f ,speed: %8d KB/s-----\n", 
				rate->name,r, rate->frame_cnts, duration, (int)(rate->size/(1024*duration)));
		rate->frame_cnts = 0;
		rate->size = 0;
		rate->start_time = cur_time;
	}
}
#endif


#ifndef MKTAG
#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#endif

void *h264_decode_init(int width, int height, uint8_t *extradata, int size)
{
	struct h264_decoder *p = malloc(sizeof(struct h264_decoder));
	memset(&p->cfg, 0, sizeof(struct video_config));

	set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX);

	p->cfg.codec_id = HC_AVCODEC_ID_MJPEG;
	/*p->cfg.sync_mode = 2;*/
	p->cfg.sync_mode = 0;
	p->cfg.quick_mode = 3;
	ALOGI("video sync_mode: %d\n", p->cfg.sync_mode);
	p->cfg.decode_mode = VDEC_WORK_MODE_KSHM;

	p->cfg.pic_width = width;
	p->cfg.pic_height = height;
	p->cfg.frame_rate = 60*1000;
	p->cfg.kshm_size = 0x800000;

	ALOGE("width:%ld, height:%ld, frame_rate:%ld, kshm_size:%u\n",
	        p->cfg.pic_width, p->cfg.pic_height, p->cfg.frame_rate, p->cfg.kshm_size);

	p->cfg.pixel_aspect_x = 1;
	p->cfg.pixel_aspect_y = 1;
	p->cfg.preview = 0;
	p->cfg.extradata_size = size;

	p->cfg.codec_tag = MKTAG('v', 'i', 'd', 'l');

	memcpy(p->cfg.extra_data, extradata, size);

	p->fd = open("/dev/viddec", O_RDWR);
	if (p->fd < 0) {
		ALOGE("Open /dev/viddec error.\n");
		return NULL;
	}

	if (ioctl(p->fd, VIDDEC_INIT, &(p->cfg)) != 0) {
		ALOGE("Init viddec error.\n");
		close(p->fd);
		free(p);
		return NULL;
	}

	ioctl(p->fd, VIDDEC_START, 0);

	ALOGI("fd: %d\n", p->fd);
	return p;
}

int h264_decode(void *phandle, uint8_t *video_frame, size_t packet_size, int32_t pts)
{
	struct h264_decoder *p = (struct h264_decoder *)phandle;
	AvPktHd pkthd = { 0 };
	pkthd.pts = pts;
	pkthd.dur = 0;
	pkthd.size = packet_size;
	pkthd.flag = AV_PACKET_ES_DATA;
	// ALOGI("video_frame: %p, packet_size: %d", video_frame, packet_size);
#ifdef AIRPLAY_FRAME_RATE
	video_frame_rate.size += packet_size;
	print_frame_rate(&video_frame_rate, 1);
#endif
	if ((size_t)write(p->fd, (uint8_t *)&pkthd, sizeof(AvPktHd)) !=
	    sizeof(AvPktHd)) {
		ALOGE("Write AvPktHd fail\n");
		return -1;
	}

	if ((size_t)write(p->fd, video_frame, packet_size) != packet_size) {
		ALOGE("Write video_frame error fail\n");
		ioctl(p->fd, VIDDEC_FLUSH, 0);
		return -1;
	}
	return 0;
}

void h264_decoder_flush(void *phandle)
{
	struct h264_decoder *p = (struct h264_decoder *)phandle;
	ioctl(p->fd, VIDDEC_FLUSH, 0);
	printf("video flush\n");
}

void h264_decoder_destroy(void *phandle)
{
	struct h264_decoder *p = (struct h264_decoder *)phandle;

	struct vdec_rls_param rls_param = {0, 0};
	/*set_aspect_mode(DIS_TV_AUTO, DIS_PILLBOX);*/
	if(!p)
		return;
	ioctl(p->fd , VIDDEC_RLS , &rls_param);

	if(p->fd > 0) {
		close(p->fd);
	}
	free(p);
}

void *aac_decoder_init(int bits, int channels, int samplerate)
{
	struct aac_decoder *p = (struct aac_decoder *)malloc(sizeof(struct aac_decoder));
	memset(&p->cfg, 0, sizeof(struct audio_config));
	p->cfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;
	p->cfg.sync_mode = 0;
	p->cfg.bits_per_coded_sample = bits;
	p->cfg.channels = channels;
	p->cfg.sample_rate = samplerate;
	p->cfg.audio_flush_thres = 100;

	p->fd = open("/dev/auddec", O_RDWR);
	if (p->fd < 0) {
		ALOGE("Open /dev/auddec error.\n");
		free(p);
		return NULL;
	}

	if (ioctl(p->fd, AUDDEC_INIT, &p->cfg) != 0) {
		ALOGE("Init auddec error.\n");
		close(p->fd);
		free(p);
		return NULL;
	}

	ioctl(p->fd, AUDDEC_START, 0);
	printf("samplerate: %d\n", samplerate);
	return p;
}

int aac_decode(void *phandle, uint8_t *audio_frame, size_t packet_size, int32_t pts)
{
	struct aac_decoder *p = (struct aac_decoder *)phandle;
	AvPktHd pkthd = { 0 };
	pkthd.dur = 0;
	pkthd.size = packet_size;
	pkthd.flag = AV_PACKET_ES_DATA;
	pkthd.pts = pts;
#ifdef AIRPLAY_FRAME_RATE
	audio_frame_rate.size += packet_size;
	print_frame_rate(&audio_frame_rate, packet_size>>2);
#endif
	if (write(p->fd, (uint8_t *)&pkthd, sizeof(AvPktHd)) !=
	    sizeof(AvPktHd)) {
		ALOGE("Write AvPktHd fail\n");
		return -1;
	}

	if ((size_t)write(p->fd, audio_frame, packet_size) != packet_size) {
		ALOGE("Write audio_frame error fail\n");
		return -1;
	}

	return 0;
}

void aac_decoder_flush(void *phandle)
{
	struct aac_decoder *p = (struct aac_decoder *)phandle;
	ioctl(p->fd, AUDDEC_FLUSH, 0);
	printf("audio flush\n");
}

void aac_decoder_destroy(void *phandle)
{
	struct aac_decoder *p = (struct aac_decoder *)phandle;
	if(!p)
		return;

	if(p->fd > 0) {
		close(p->fd);
	}
	free(p);
}

void set_volume(uint8_t vol)
{
	int snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if(snd_fd < 0) {
		ALOGE("Open /dev/sndC0i2so fail. %s",strerror(errno));
		return;
	}
	ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &vol);
}

void set_aspect_mode(dis_mode_e ratio, dis_mode_e dis_mode)
{
	int fd = open("/dev/dis", O_RDWR);
	printf("ratio: %d, dis_mode: %d, fd: %d", ratio, dis_mode, fd);
	if ( fd < 0)
		return;
	dis_aspect_mode_t aspect = {0};
	aspect.distype = DIS_TYPE_HD;
	aspect.tv_mode = ratio;
	aspect.dis_mode = dis_mode;
	ioctl(fd, DIS_SET_ASPECT_MODE, &aspect);
	close(fd);
}
