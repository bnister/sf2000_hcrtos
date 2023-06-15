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
#include <hcuapi/codec_id.h>
#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include "alac_dec.h"

#define ALOGI printf
#define ALOGE printf
#define ALOGD printf

struct alac_decoder {
    struct audio_config cfg;
    int fd;
};

void *alac_decoder_init(int bits, int channels, int samplerate)
{
    struct alac_decoder *p =
        (struct alac_decoder *)malloc(sizeof(struct alac_decoder));
    memset(&p->cfg , 0 , sizeof(struct audio_config));
    p->cfg.bits_per_coded_sample = bits;
    p->cfg.channels = channels;
    p->cfg.sample_rate = samplerate;
	p->cfg.codec_id = HC_AVCODEC_ID_ALAC;

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

int alac_decode(void *phandle, uint8_t *audio_frame, size_t packet_size)
{
    struct alac_decoder *p = (struct alac_decoder *)phandle;
    AvPktHd pkthd = { 0 };
    pkthd.dur = 0;
    pkthd.size = packet_size;
    pkthd.flag = AV_PACKET_ES_DATA;
    pkthd.pts = -1;
	while(1) {
	    if(write(p->fd , (uint8_t *)&pkthd , sizeof(AvPktHd)) !=
	       sizeof(AvPktHd)) {
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

void alac_decoder_flush(void *phandle)
{
    struct alac_decoder *p = (struct alac_decoder *)phandle;
    ioctl(p->fd, AUDDEC_FLUSH, 0);
}

void alac_decoder_destroy(void *phandle)
{
    struct alac_decoder *p = (struct alac_decoder *)phandle;
    if( !p )
        return;

    if( p->fd > 0 ) {
        close(p->fd);
    }
    free(p);
}

int alac_test (int argc, char *argv[])
{
	(void)argc;
	(void *)argv;
	int bits, channels, samplerate, process_units, ret;
	void *hdl= NULL;
	int buf_size = 0;
	uint8_t *data = NULL;
	bits = 16;
	channels = 2;
	samplerate = 44100;
	FILE *alac_file	= NULL;
	char *alac_url = "/media/sda2/aircast.alac";

	hdl = alac_decoder_init(bits, channels, samplerate);

	if (alac_file == NULL) {
		alac_file = fopen(alac_url, "r");
		if (!alac_file) {
			printf("can not open alac url\n");
			return -1;
		}
	}

	while (1) {
		ret = fread((void *)&buf_size, 1, 4, alac_file);
		if (ret != 4) {
			break;
		}

		printf("buf_size = %d\n", buf_size);

		if (data == NULL) {
			data = malloc(buf_size);
		} else {
			data = realloc(data, buf_size);
		}

		ret = fread(data, 1, buf_size, alac_file);
		if (ret != buf_size) {
			break;
		}

		ret = alac_decode(hdl, data, buf_size);
		if (ret) {
			printf("alac_decode error\n");
			return -1;
		}
		memset(data, 0, buf_size);
	}

	alac_decoder_destroy(hdl);
	fclose(alac_file);

	return 0;
}
