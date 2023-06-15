#include <stdio.h>
#include <kernel/delay.h>
#include "sound_test.h"
#include <hcuapi/common.h>

#define RECODER_TIME  1200
static bool stop_i2si = 0;
static void *audio_data = NULL;
static void *buf = NULL;
int i2si_i2so_recoder_test(int argc, char *argv[])
{
	int snd_in_fd;
	uint32_t buf_size;
	int recoder_size;
	int ds = 0;
	int ret = 0;
	int recoder_time = 0;

	struct kshm_info audio_read_hdl = {0};
	uint32_t aread_size = 0;

	snd_in_fd = open("/dev/sndC0i2so", O_WRONLY);
	if(snd_in_fd < 0) {
		printf("error: can not open i2so dev \n");
		return -1;
	}

	buf_size = 300 * 1024;
	recoder_size = 0;

	if (!audio_data) {
		audio_data = malloc(1536 * RECODER_TIME);
		printf("audio_data %p\n",audio_data);
	}

	ioctl(snd_in_fd, SND_IOCTL_SET_RECORD, buf_size);

	ioctl(snd_in_fd, KSHM_HDL_ACCESS, &audio_read_hdl);

	printf("start recorde\n");

	while (1) {
		AvPktHd hdr = {0};
		while (kshm_read((void *)&audio_read_hdl, &hdr, sizeof(AvPktHd)) != sizeof(AvPktHd)) {
			msleep(5);
		}

		if (recoder_size < hdr.size) {
			recoder_size = hdr.size;
			if(buf) {
				buf = realloc(buf, recoder_size);
			} else {
				buf = malloc(recoder_size);
			}
		}
		while (kshm_read((void *)&audio_read_hdl, buf, hdr.size) != hdr.size) {
			msleep(20);
		}
		aread_size += hdr.size;
		if(aread_size > 1536 * RECODER_TIME) {
			break;
		}
		memcpy(audio_data + aread_size - hdr.size, buf, hdr.size);
		msleep(1);
	}

	printf("recorde done !\n");
	while(!stop_i2si) {
		msleep(1000);
	}

	ioctl(snd_in_fd, SND_IOCTL_SET_FREE_RECORD, 0);
	close(snd_in_fd);

	free(audio_data);
	audio_data = NULL;
	return 0;
}

