#include <kernel/delay.h>
#include "sound_test.h"

int i2so_test(int argc, char *argv[])
{
	struct snd_pcm_params params;
	int read_size = 24000;
	char *buf = NULL;
	int size = 0;
	snd_pcm_uframes_t delay = 0;
	int ret = 0;
	uint32_t pts = 0;
	snd_pcm_uframes_t poll_size = read_size;
	snd_pcm_uframes_t seek_position = 0;

	struct pollfd pfd;
	FILE *file = NULL;
	int snd_fd = 0;

	printf ("argc %d\n", argc);

	buf = av_malloc (read_size);
	printf ("buf %p\n", buf);

	file = fopen ("/romfs/test.wav", "r");
	printf ("file %p\n", file);

	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	printf ("snd_fd %d\n", snd_fd);

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = snd_fd;
	pfd.events = POLLOUT | POLLWRNORM;

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;

	if (argc >= 2)
		params.sync_mode = atoi(argv[1]);
	else
		params.sync_mode = AVSYNC_TYPE_FREERUN;

	if (argc >= 3)
		params.align = atoi(argv[2]);
	else
		params.align = 0;

	if (argc >= 4)
		params.rate = atoi(argv[3]);
	else
		params.rate = 11025;

	params.channels = 2;
	params.period_size = read_size;
	params.periods = 8;
	params.bitdepth = 16;
	params.start_threshold = 2;
	ioctl(snd_fd, SND_IOCTL_HW_PARAMS, &params);
	printf ("SND_IOCTL_HW_PARAMS done\n");

	ioctl(snd_fd, SND_IOCTL_AVAIL_MIN, &poll_size);

	ioctl(snd_fd, SND_IOCTL_START, 0);
	printf ("SND_IOCTL_START done\n");

	do {
		size = fread(buf, 1, read_size, file);
		printf ("size %d\n", size);
		seek_position += size;
		fseek(file,seek_position,SEEK_SET);

		if (size > 0) {
			do {
				struct snd_xfer xfer = {0};
				xfer.data = buf;
				xfer.frames = size/4;
				xfer.tstamp_ms = pts;
				ret = ioctl(snd_fd, SND_IOCTL_XFER, &xfer);
				if (ret < 0) {
					//printf ("poll\n");
					poll(&pfd, 1, 100);
				}
			} while (ret < 0);
			pts +=  read_size * 1000 / (params.rate *
				params.channels * params.bitdepth / 8) ;
		}
	} while (size == read_size);
	printf ("write data done\n");

#if 1
	do {
		ioctl(snd_fd, SND_IOCTL_DELAY, &delay);
		if ((int)delay * 4/* ch*bitdepth/8*/ >
			(int)params.period_size) {
			printf ("delay %ld\n", delay);
			msleep(100);
		} else {
			break;
		}
	} while(1);
#else
	ioctl(snd_fd, SND_IOCTL_DRAIN, 0);
#endif

	printf ("SND_IOCTL_DROP\n");
	ioctl(snd_fd, SND_IOCTL_DROP, 0);

	printf ("SND_IOCTL_HW_FREE\n");
	ioctl(snd_fd, SND_IOCTL_HW_FREE, 0);

	printf ("close\n");
	close(snd_fd);

	av_free(buf);

	fclose(file);
	return 0;
}
