#include "sound_test.h"
#include <hcuapi/pinmux.h>

static bool stop_i2si012 = 0;
int i2si012_test(int argc, char *argv[])
{
	struct snd_pcm_params params;
	int period_size = 1536;
	int channels = 2;
	int rate = 44100;
	int periods = 16;
	int bitdepth = 16;

	int total_read_times = 600;
	int read_size = period_size * channels;
	int record_size = period_size * total_read_times;

	int read_times = 0;
	char *buf = NULL;
	int ret = 0;
	snd_pcm_uframes_t poll_size = period_size;

	struct pollfd pfd;
	int snd_in_fd = 0;
	struct snd_xfer xfer = {0};

	char *ch0 = NULL;
	char *ch1 = NULL;


	buf = malloc (read_size);
	memset(buf, 0, read_size);

	ch0 = malloc (record_size);
	ch1 = malloc (record_size);


	memset(ch0, 0, record_size);
	memset(ch1, 0, record_size);

#if 0
	pinmux_configure(PINPAD_B21, 4);  //i2si2   C300
	pinmux_configure(PINPAD_B22, 4);
	pinmux_configure(PINPAD_B23, 4);
	pinmux_configure(PINPAD_B24, 4);

	pinmux_configure(PINPAD_R00, 7);  //i2si1   A3200
	pinmux_configure(PINPAD_R02, 7);
	pinmux_configure(PINPAD_R03, 7);
	pinmux_configure(PINPAD_R04, 7);
#endif
	pinmux_configure(PINPAD_R07, 7);   //fs    i2si0   A3200
	pinmux_configure(PINPAD_R08, 7);   //bck
	pinmux_configure(PINPAD_R09, 7);   //datai
	pinmux_configure(PINPAD_R10, 7);   //mclk

	printf ("ch0 %p, ch1 %p\n", ch0, ch1);

	snd_in_fd = open("/dev/sndC0i2si0", O_WRONLY);
	if(snd_in_fd < 0){
		printf("open dev fs error \n");
	}

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = snd_in_fd;
	pfd.events = POLLIN | POLLRDNORM;

	params.access = SND_PCM_ACCESS_RW_NONINTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = SND_PCM_ALIGN_RIGHT;
	params.rate = rate;
	params.channels = channels;
	params.period_size = period_size;
	params.periods = periods;
	params.bitdepth = bitdepth;
	params.start_threshold = 1;
	ioctl(snd_in_fd, SND_IOCTL_HW_PARAMS, &params);
	ioctl(snd_in_fd, SND_IOCTL_START, 0);
	printf ("start record\n");
	usleep(50*1000);
	xfer.data = buf;
	xfer.frames = period_size / (params.bitdepth / 8);
	do {
		ret = ioctl(snd_in_fd, SND_IOCTL_XFER, &xfer);
		if (ret < 0) {
			poll(&pfd, 1, 100);
			continue;
		}
		memcpy(ch0 + period_size * read_times, xfer.data, period_size);
		memcpy(ch1 + period_size * read_times, xfer.data + period_size, period_size);
		read_times++;
	} while (read_times < total_read_times);

	printf ("record done, dump data to check\n");

	while(!stop_i2si012) {
		usleep(1*1000*1000);
	}
	printf ("usleep done!!!\n");

	ioctl(snd_in_fd, SND_IOCTL_DROP, 0);
	ioctl(snd_in_fd, SND_IOCTL_HW_FREE, 0);
	close(snd_in_fd);

	free(ch0);
	free(ch1);

	free(buf);
	return 0;
}

int stop_i2si012_test(int argc, char *argv[])
{
	stop_i2si012 = 1;
	return 0;
}
