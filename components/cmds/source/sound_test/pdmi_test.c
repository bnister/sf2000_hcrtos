#include "sound_test.h"

int pdmi_test(int argc, char *argv[])
{
	struct snd_pcm_params params;
	int channels = 2;
	int rate = 48000;
	int periods = 16;
	int bitdepth = 16;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	int align = SND_PCM_ALIGN_RIGHT;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_NONINTERLEAVED;
	int period_size = 1536;

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
	printf ("ch0 %p, ch1 %p\n", ch0, ch1);

	snd_in_fd = open("/dev/sndC0pdmi", O_WRONLY);
	printf ("pdmi fd %d\n", snd_in_fd);

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = snd_in_fd;
	pfd.events = POLLOUT | POLLWRNORM;

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
	ioctl(snd_in_fd, SND_IOCTL_HW_PARAMS, &params);
	ioctl(snd_in_fd, SND_IOCTL_START, 0);
	printf ("start record\n");

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
	asm volatile ("nop;.word 0x1000ffff;nop;");

	ioctl(snd_in_fd, SND_IOCTL_DROP, 0);
	ioctl(snd_in_fd, SND_IOCTL_HW_FREE, 0);
	close(snd_in_fd);

	free(ch0);
	free(ch1);

	free(buf);
	return 0;
}
