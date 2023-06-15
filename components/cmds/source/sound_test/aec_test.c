#include "sound_test.h"
#include <hcuapi/pinmux.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <hcaec.h>

static bool stop_aec = 0;
static pthread_t aec_thread_id = 0;
static void *buf0 = NULL;
static void *buf1 = NULL;
static void *ch0 = NULL;
static void *ch1 = NULL;
static void *ch2 = NULL;
static void *ch3 = NULL;
static void *ch4 = NULL;
static int16_t *ch6 = NULL;
static int16_t *ch7 = NULL;

static FILE *file0 = NULL;
static char *uri0 = "/media/sda2/micin.wav";
static FILE *file1 = NULL;
static char *uri1 = "/media/sda2/speaker.wav";
static FILE *file2 = NULL;
static char *uri2 = "/media/sda2/stream2";
static FILE *file3 = NULL;
static char *uri3 = "/media/sda2/stream3";

static void print_usage(const char *prog)
{
	printf("Usage: %s [-mrds]\n", prog);
	puts("  -m --micin         micin wav file(single channel)\n"
	     "  -r --reference     reference wav file(single channel)\n"
	     "                       -r ref1.wav -r ref2.wav ...(max 4 reference)\n"
	     "  -o --output        output wav file\n"
	     "  -d --denoise       denoiseEnable, default 0 (off)\n"
	     "                       0 - off\n"
	     "                       1 - on\n");
}

static int duration2bytes(int samplerate, int duration)
{
	return 2 * samplerate * duration / 1000;
}

static int duration2samples(int samplerate, int duration)
{
	return samplerate * duration / 1000;
}

static void* aec_thread0(void *args)
{
	struct snd_pcm_params params = {0};
	struct snd_pcm_params params1 = {0};
	int period_size = 1536;
	int channels = 2;
	int rate = 16000;
	int periods = 16;
	int bitdepth = 16;

	int total_read_times = 1500;
	int read_size = period_size * channels;
	int record_size = period_size * total_read_times;

	int read_times = 0;
	int ret = 0;

	struct pollfd pfd0;
	struct pollfd pfd1;
	int i2si0_fd = 0;
	int i2si1_fd = 0;
	struct snd_xfer xfer0 = {0};
	struct snd_xfer xfer1 = {0};

	buf0 = malloc (read_size);
	memset(buf0, 0, read_size);
	buf1 = malloc (read_size);
	memset(buf1, 0, read_size);

	ch0 = malloc (record_size);
	ch1 = malloc (record_size);
	ch2 = malloc (record_size * 2);
	ch3 = malloc (record_size);
	ch4 = malloc (record_size);
	ch6 = malloc (record_size);
	ch7 = malloc (record_size);

	memset(ch0, 0, record_size);
	memset(ch1, 0, record_size);
	memset(ch2, 0, record_size * 2);
	memset(ch3, 0, record_size);
	memset(ch4, 0, record_size);
	memset(ch6, 0, record_size);
	memset(ch7, 0, record_size);

	pinmux_configure(PINPAD_R00, 7);  //i2si1	A3200
	pinmux_configure(PINPAD_R02, 7);
	pinmux_configure(PINPAD_R03, 7);
	pinmux_configure(PINPAD_R04, 7);

	pinmux_configure(PINPAD_R07, 7);   //fs    i2si0   A3200
	pinmux_configure(PINPAD_R08, 7);   //bck
	pinmux_configure(PINPAD_R09, 7);   //datai
	pinmux_configure(PINPAD_R10, 7);   //mclk

	printf ("ch0 %p, ch1 %p, ch2 %p, ch3 %p\n", ch0, ch1, ch2, ch3);

	i2si0_fd = open("/dev/sndC0i2si0", O_WRONLY);
	if(i2si0_fd < 0){
		printf("open dev i2si0 error \n");
	}
	i2si1_fd = open("/dev/sndC0i2si", O_WRONLY);
	if(i2si1_fd < 0){
		printf("open dev i2si1 error \n");
	}

	memset(&pfd0, 0, sizeof(struct pollfd));
	pfd0.fd = i2si0_fd;
	pfd0.events = POLLIN | POLLRDNORM;
	memset(&pfd1, 0, sizeof(struct pollfd));
	pfd1.fd = i2si1_fd;
	pfd1.events = POLLIN | POLLRDNORM;

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
	params.pcm_source = SND_PCM_SOURCE_AUDPAD;

	memcpy(&params1, &params, sizeof(struct snd_pcm_params));
	params1.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params1.period_size = period_size * 2;

	ioctl(i2si0_fd, SND_IOCTL_HW_PARAMS, &params);
	ioctl(i2si1_fd, SND_IOCTL_HW_PARAMS, &params1);

	ioctl(i2si0_fd, SND_IOCTL_START, 0);
	printf ("start i2si0 record\n");
	ioctl(i2si1_fd, SND_IOCTL_START, 0);
	printf ("start i2si1 record\n");
	usleep(50*1000);
	xfer0.data = buf0;
	xfer0.frames = period_size / (params.bitdepth / 8);
	xfer1.data = buf1;
	xfer1.frames = period_size * 2 / (channels * params1.bitdepth / 8);

	do {
		ret = ioctl(i2si0_fd, SND_IOCTL_XFER, &xfer0);
		if (ret < 0) {
			poll(&pfd0, 1, 100);
			continue;
		}
		memcpy(ch0 + period_size * read_times,
			xfer0.data, period_size);
		//memcpy(ch1 + period_size * read_times ,
			//xfer0.data + period_size, period_size);

		ret = ioctl(i2si1_fd, SND_IOCTL_XFER, &xfer1);
		if (ret < 0) {
			poll(&pfd1, 1, 100);
			continue;
		}
		memcpy(ch2 + period_size * read_times * 2,
			xfer1.data, period_size * 2);
		//memcpy(ch3 + period_size * read_times,
			//xfer1.data + period_size, period_size);

		read_times++;
	} while (read_times < total_read_times);

	printf ("record done, dump data to check\n");

	if (!file0 && !file1) {
		file0 = fopen(uri0, "wb");
		file1 = fopen(uri1, "wb");
		file2 = fopen(uri2, "wb");
		file3 = fopen(uri3, "wb");
	}

	if (file0 && file1) {
		fwrite (ch0, total_read_times * period_size, 1, file0);
		//fwrite (ch1, total_read_times * period_size, 1, file0);
		int16_t *ch_3 = (int16_t *)ch3;
		int16_t *ch_4 = (int16_t *)ch4;
		int16_t *ch_2 = (int16_t *)ch2;

		for (int i = 0;i < total_read_times * period_size / 2;i++) {
			//ch6[i] = ch_2[2 * i] / 2 + ch_2[2 * i + 1] / 2;     //mix channel to one channel
			ch6[i] = ch_2[2 * i];
			ch7[i] = ch_2[2 * i + 1];
		}

		fwrite (ch6, total_read_times * period_size, 1, file1);  //single channel
		fwrite (ch7, total_read_times * period_size, 1, file2);  //single channel
		fwrite (ch2, total_read_times * period_size * 2, 1, file3);//dual channel

		usleep(1000*1000);
		fclose(file0);
		fclose(file1);
		fclose(file2);
		fclose(file3);

	}
	printf("dump data ok\n");

	ioctl(i2si0_fd, SND_IOCTL_DROP, 0);
	ioctl(i2si0_fd, SND_IOCTL_HW_FREE, 0);
	close(i2si0_fd);
	ioctl(i2si1_fd, SND_IOCTL_DROP, 0);
	ioctl(i2si1_fd, SND_IOCTL_HW_FREE, 0);
	close(i2si1_fd);

	free(ch0);
	free(ch1);
	free(ch2);
	free(ch3);
	free(ch4);
	free(ch6);
	free(ch7);

	free(buf0);
	free(buf1);
	return NULL;

}

static int i2si_rec_init(int* i2si0_fd, int* i2si1_fd, int process_units, struct snd_xfer* xfer0,
	struct snd_xfer* xfer1, struct pollfd* pfd0, struct pollfd* pfd1)
{
	struct snd_pcm_params params = {0};
	struct snd_pcm_params params1 = {0};
	int period_size = 0;
	int channels = 2;
	int rate = 16000;
	int periods = 16;
	int bitdepth = 16;
	int read_size = 0;
	int ret = 0;

	period_size = process_units / 2;

	pinmux_configure(PINPAD_R07, 7);   //fs    i2si0   A3200
	pinmux_configure(PINPAD_R08, 7);   //bck
	pinmux_configure(PINPAD_R09, 7);   //datai
	pinmux_configure(PINPAD_R10, 7);   //mclk

	*i2si0_fd = open("/dev/sndC0i2si0", O_WRONLY);
	if(*i2si0_fd < 0){
		printf("open dev i2si0 error \n");
	}
	*i2si1_fd = open("/dev/sndC0i2si", O_WRONLY);
	if(*i2si1_fd < 0){
		printf("open dev i2si1 error \n");
	}

	memset(pfd0, 0, sizeof(struct pollfd));
	pfd0->fd = *i2si0_fd;
	pfd0->events = POLLIN | POLLRDNORM;
	memset(pfd1, 0, sizeof(struct pollfd));
	pfd1->fd = *i2si1_fd;
	pfd1->events = POLLIN | POLLRDNORM;

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
	params.pcm_source = SND_PCM_SOURCE_AUDPAD;

	memcpy(&params1, &params, sizeof(struct snd_pcm_params));
	params1.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params1.period_size = period_size * 2;

	ioctl(*i2si0_fd, SND_IOCTL_HW_PARAMS, &params);
	ioctl(*i2si1_fd, SND_IOCTL_HW_PARAMS, &params1);

	ioctl(*i2si0_fd, SND_IOCTL_START, 0);
	printf ("start i2si0 record\n");
	ioctl(*i2si1_fd, SND_IOCTL_START, 0);
	printf ("start i2si1 record\n");
	usleep(50*1000);
	xfer0->data = buf0;
	xfer0->frames = period_size / (params.bitdepth / 8);
	xfer1->data = buf1;
	xfer1->frames = period_size * 2 / (channels * params1.bitdepth / 8);
	return 0;

}
static void* aec_thread(void *args)
{
#ifdef CMD_BR2_PACKAGE_PREBUILTS_3A
	int ret = 0;
	int i2si0_fd = 0;
	int i2si1_fd = 0;
	struct snd_xfer xfer0 = {0};
	struct snd_xfer xfer1 = {0};
	struct pollfd pfd0;
	struct pollfd pfd1;
	int *ch_2 = NULL;
	void *buf2 = NULL;

	int process_units = 0;
	int process_samples = 0;
	char *micin_file = NULL;
	char *ref_file[MAX_SPEAKER_NUM] = {NULL};
	char *dst_file = NULL;
	FILE *micin_fp = NULL;
	FILE *ref_fp[MAX_SPEAKER_NUM] = {NULL};
	FILE *dst_fp = NULL;
	uint8_t *micin = NULL, *dst = NULL, *ref[MAX_SPEAKER_NUM] = {NULL};
	int i, c;
	void *handle = NULL;
	int samplerate;
	struct hcaec_params aec_params = { 0 };
	struct aec_data data_in = { 0 };
	int speaker = 0;

	static const struct option lopts[] = {
		{ "micin", 1, 0, 'm' },
		{ "reference", 1, 0, 'r' },
		{ "output", 1, 0, 'o' },
		{ "denoise", 1, 0, 'd' },
		{ NULL, 0, 0, 0 },
	};

	/* default params */
	aec_params.filter_length = 2048;
	aec_params.denoise = 0;
	aec_params.ecval_mul = 0;
	samplerate = 16000;

	if (samplerate == 8000 || samplerate == 16000) {
		process_units = duration2bytes(samplerate, 10);
		process_samples = duration2samples(samplerate, 10);
	} else {
		printf("Not support samplerate %d\n", samplerate);
		ret = -1;
		goto err;
	}

	int period_size = process_units / 2;

	buf0 = malloc (process_units);
	if (buf0 == NULL) {
		printf("no memory alloc\n");
		goto err;
	}

	buf1 = malloc (process_units * 2);
	if (buf1 == NULL) {
		printf("no memory alloc\n");
		goto err;
	}

	buf2 = malloc (process_units * 2);
	if (buf2 == NULL) {
		printf("no memory alloc\n");
		goto err;
	}

	micin = malloc(process_units);
	if (micin == NULL) {
		printf("no memory alloc\n");
		goto err;
	}

	for (i = 0; i < speaker; i++) {
		ref[i] = malloc(process_units);
		if (ref[i] == NULL) {
			printf("no memory alloc\n");
			goto err;
		}
	}

	dst = malloc(process_units);
	if (dst == NULL) {
		printf("no memory alloc\n");
		goto err;
	}

	dst_file = "media/sda2/aecfile";
	dst_fp = fopen(dst_file, "wb+");
	if (dst_fp == NULL) {
		printf("open %s failed\n", dst_file);
		goto err;
	}

	aec_params.sample_rate = samplerate;
	aec_params.frame_size = process_samples;
	aec_params.speaker_num = speaker;

	handle = hcaec_create(&aec_params);
	if (handle == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	i2si_rec_init(&i2si0_fd, &i2si1_fd, process_units, &xfer0, &xfer1, &pfd0, &pfd1);
	do {
		ret = ioctl(i2si0_fd, SND_IOCTL_XFER, &xfer0);
		if (ret < 0) {
			poll(&pfd0, 1, 100);
			continue;
		}
		memcpy(micin ,xfer0.data, period_size);

		ret = ioctl(i2si1_fd, SND_IOCTL_XFER, &xfer1);
		if (ret < 0) {
			poll(&pfd1, 1, 100);
			continue;
		}
		memcpy(buf2, xfer1.data, period_size * 2);

		ch_2 = (int *)buf2;
		for (int j = 0;j <	period_size / 2;j++) {
			ref[0][j] = ch_2[2 * j];
		}

		data_in.in = micin;
		for (i = 0; i < speaker; i++)
			data_in.ref[i] = ref[i];
		data_in.out = dst;
		data_in.nsamples = process_samples;
		memset(dst, 0, process_units);

		ret = hcaec_process(handle, &data_in);
		if (ret)
			printf("hcagc process failed %d\n", ret);

		ret = fwrite(dst, 1, process_units, dst_fp);
		if (ret != process_units) {
			printf("write data after denoise failed\n");
			break;
		}

	} while (!stop_aec);

	ioctl(i2si0_fd, SND_IOCTL_DROP, 0);
	ioctl(i2si0_fd, SND_IOCTL_HW_FREE, 0);
	close(i2si0_fd);
	ioctl(i2si1_fd, SND_IOCTL_DROP, 0);
	ioctl(i2si1_fd, SND_IOCTL_HW_FREE, 0);
	close(i2si1_fd);

err:
	if(buf0)
		free(buf0);
	if(buf1)
		free(buf1);
	if(buf2)
		free(buf2);

	if (micin_fp)
		fclose(micin_fp);
	for (i = 0; i < speaker; i++) {
		if (ref_fp[i])
			fclose(ref_fp[i]);
	}
	if (dst_fp)
		fclose(dst_fp);

	if (micin)
		free(micin);
	for (i = 0; i < speaker; i++) {
		if (ref[i])
			free(ref[i]);
	}
	if (dst)
		free(dst);

	if (handle) {
		hcaec_destroy(handle);
	}
#endif
	return NULL;
}

int stop_aec_test(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	stop_aec = 1;
	if (aec_thread_id)
		pthread_join(aec_thread_id, NULL);
	aec_thread_id = 0;

	return 0;
}

int aec_test(int argc, char *argv[])
{
	/*aec test*/
	if (0) {
		if (pthread_create(&aec_thread_id, NULL,
			aec_thread, NULL)) {
			printf("create aec thread failed\n");
		}
	} else {
	/*rec test*/
		if (pthread_create(&aec_thread_id, NULL,
			aec_thread0, NULL)) {
			printf("create aec thread failed\n");
		}
	}
	(void)argc;
	(void)argv;
	return 0;
}
