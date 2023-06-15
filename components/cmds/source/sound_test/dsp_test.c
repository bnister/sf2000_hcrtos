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
#include <hcuapi/codec_id.h>
#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include "sound_test.h"

struct audio_decoder {
	struct audio_config cfg;
	int fd;
};

static int i2si_source = SND_PCM_SOURCE_AUDPAD;
static int stop_i2si = 0;

static void *audio_decoder_init(int bits, int channels, int samplerate)
{
	struct audio_decoder *p =
		(struct audio_decoder *)malloc(sizeof(struct audio_decoder));
	memset(&p->cfg , 0 , sizeof(struct audio_config));
	p->cfg.bits_per_coded_sample = bits;
	p->cfg.channels = channels;
	p->cfg.sample_rate = samplerate;
	p->cfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;

	p->fd = open("/dev/auddec" , O_RDWR);
	if( p->fd < 0 ) {
		printf("Open /dev/auddec error.");
		free(p);
		return NULL;
	}

	if( ioctl(p->fd , AUDDEC_INIT , &p->cfg) != 0 ) {
		printf("Init auddec error\n");
		close(p->fd);
		free(p);
		return NULL;
	}

	ioctl(p->fd, AUDDEC_START, 0);
	return p;
}

static int audio_decode(void *phandle, uint8_t *audio_frame, size_t packet_size)
{
	if (!phandle) {
		printf("audio_decode param:phandle is NULL\n");
		return -1;
	}
	struct audio_decoder *p = (struct audio_decoder *)phandle;
	AvPktHd pkthd = { 0 };
	pkthd.dur = 0;
	pkthd.size = packet_size;
	pkthd.flag = AV_PACKET_ES_DATA;
	pkthd.pts = -1;
	while(1) {
	    if(write(p->fd , (uint8_t *)&pkthd , sizeof(AvPktHd)) != sizeof(AvPktHd)) {
			usleep(20*1000);
			continue;
		}
		break;
	}
	while(1) {
		if(write(p->fd , audio_frame , packet_size) != (int)packet_size) {
			usleep(20*1000);
			continue;
	    }
		break;
	}
	return 0;
}

static void audio_decoder_destroy(void *phandle)
{
	struct audio_decoder *p = (struct audio_decoder *)phandle;
	if( !p )
		return;

	if( p->fd >= 0 ) {
		close(p->fd);
	}
	free(p);
	return;
}

static void audio_test_thread(void *arg)
{
	struct pollfd pfd;
	void *hdl= NULL;
	char *data = NULL;
	char *buf = NULL;

	int ret = 0;
	int channels = 2;
	int rate = 44100;
	int periods = 16;
	int bitdepth = 16;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	int align = SND_PCM_ALIGN_RIGHT;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	int period_size = 1536;   //period_size must be a multiple of 32
	int read_size = period_size;
	int snd_in_fd = 0;
	struct snd_xfer xfer = {0};
	struct snd_pcm_params params = {0};

	buf = malloc (read_size);
	if(!buf) {
		printf("no menery alloc\n");
		return;
	}
	memset(buf, 0, read_size);

	data = malloc (read_size);
	if(!data) {
		printf("no menery alloc\n");
		return;
	}
	memset(data, 0, read_size);

	snd_in_fd = open("/dev/sndC0i2si", O_WRONLY);
	if (snd_in_fd < 0) {
		printf ("open i2si failed: fd = %d\n", snd_in_fd);
		return;
	}

	hdl = audio_decoder_init(bitdepth, channels, rate);
	if (!hdl) {
		printf ("audio_decoder_init error\n");
		return;
	}
	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = snd_in_fd;
	pfd.events = POLLIN | POLLRDNORM;

	params.access = access;
	params.format = format;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = align;
	params.rate = rate;
	params.channels = channels;
	params.period_size = period_size;
	params.periods = periods;
	params.bitdepth = bitdepth;
	params.start_threshold = 1;
	params.pcm_source = i2si_source;

	ret = ioctl(snd_in_fd, SND_IOCTL_HW_PARAMS, &params);
	if (ret < 0) {
		printf("SND_IOCTL_HW_PARAMS error \n");
	}

	ret = ioctl(snd_in_fd, SND_IOCTL_START, 0);
	if (ret < 0) {
		printf("SND_IOCTL_START error \n");
	}

	xfer.data = buf;
	xfer.frames = period_size / (channels * params.bitdepth / 8);
	do {
		ret = ioctl(snd_in_fd, SND_IOCTL_XFER, &xfer);
		if (ret < 0) {
			poll(&pfd, 1, 100);
			continue;
		}
		/* fetch record data */
		memcpy(data, xfer.data, period_size);

		/* play recorded data */
		ret = audio_decode(hdl, data, period_size);
		if (ret) {
			printf("audio_decode error\n");
			return;
		}
		memset(data, 0, period_size);

	} while (!stop_i2si);

	ioctl(snd_in_fd, SND_IOCTL_DROP, 0);
	ioctl(snd_in_fd, SND_IOCTL_HW_FREE, 0);
	close(snd_in_fd);

	audio_decoder_destroy(hdl);

	if (buf)
		free(buf);
	if (data)
		free(data);

	vTaskDelete(NULL);
	return;
}


int dsp_test(int argc, char *argv[])
{
	int ret;

	if (argc > 1) {
		i2si_source = atoi(argv[1]);
	} else {
		i2si_source = SND_PCM_SOURCE_AUDPAD;
	}

	ret = xTaskCreate(audio_test_thread, "audio_test_thread",
		0x2000, NULL, portPRI_TASK_HIGH, NULL);
	if(ret != pdTRUE) {
		return -1;
	}

	return 0;
}

int stop_dsp_test(int argc, char *argv[])
{
	stop_i2si = 1;
	return 0;
}
