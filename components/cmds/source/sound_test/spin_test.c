#include "sound_test.h"

static char *buf = NULL;
static int stop_spin = 0;
/***************spin test *****************/
static void spin_test_thread(void *arg)
{
	struct snd_pcm_params params;
	int channels = 2;
	int rate = 44100;
	int periods = 16;
	int bitdepth = 16;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	int align = SND_PCM_ALIGN_LEFT;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	int period_size = 1536;

	int total_read_times = 600;
	int read_size = period_size;
	int record_size = read_size * total_read_times;

	int read_times = 0;
	int ret = 0;
	snd_pcm_uframes_t poll_size = period_size;

	struct pollfd pfd;
	int snd_in_fd = 0;
	struct snd_xfer xfer = {0};

	char *ch0 = NULL;

	if(!buf)
		buf = malloc (read_size);//memory leak, only for test.
	memset(buf, 0, read_size);

	ch0 = malloc (record_size);
	memset(ch0, 0, record_size);
	printf ("ch0 %p\n", ch0);

	snd_in_fd = open("/dev/sndC1spin", O_WRONLY);
	printf ("spin fd %d\n", snd_in_fd);

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
	ioctl(snd_in_fd, SND_IOCTL_HW_PARAMS, &params);
	ioctl(snd_in_fd, SND_IOCTL_START, 0);
	printf ("start record\n");

	xfer.data = buf;
	xfer.frames = period_size / (channels * params.bitdepth / 8);
	do {
		ret = ioctl(snd_in_fd, SND_IOCTL_XFER, &xfer);
		if (ret < 0) {
			poll(&pfd, 1, 100);
			continue;
		}
		memcpy(ch0 + period_size * read_times, xfer.data, period_size);
		read_times++;
	} while (read_times < total_read_times);
	printf ("record done, dump data to check\n");
	//asm volatile ("nop;.word 0x1000ffff;nop;");
	while(!stop_spin) {
		usleep(1*1000*1000);
	}

	ioctl(snd_in_fd, SND_IOCTL_DROP, 0);
	ioctl(snd_in_fd, SND_IOCTL_HW_FREE, 0);
	close(snd_in_fd);

	free(ch0);

	vTaskDelete(NULL);
}

int spin_test(int argc, char *argv[])
{
	xTaskCreate(spin_test_thread, "spin_test", 0x2000, NULL,
		    portPRI_TASK_NORMAL, NULL);

	return 0;
}
