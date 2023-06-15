#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef __linux__
#include <sys/msg.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include "console.h"
#else
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>
#include "showlogo.h"
#endif

#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/common.h>
#include <hcuapi/auddec.h>
#include <hcuapi/snd.h>
struct aac_eld_file;
typedef struct aac_eld_file aac_eld_file_t;

struct pcm_render {
	struct audio_config cfg;
	int fd;
};

extern aac_eld_file_t *create_aac_eld(void);
extern void aac_eld_decode_frame(aac_eld_file_t *aac_eld, unsigned char *inbuffer, int inputsize, void *outbuffer, int *outputsize);
extern void destroy_aac_eld(aac_eld_file_t *aac);

static FILE *datafile = NULL;
static aac_eld_file_t *aacdec = NULL;
static void *out_buf = NULL;
static void *pcmrender = NULL;
static pthread_t fdkaac_test_thread_id = 0;
static int g_stop_fdkaac_test = 1;
static void *pcm_render_init(enum HCAVCodecID codec_id, int bits , int channels , int samplerate)
{
	struct pcm_render *p =
		(struct pcm_render *)malloc(sizeof(struct pcm_render));
	memset(&p->cfg , 0 , sizeof(struct audio_config));
	p->cfg.codec_id = codec_id;
	p->cfg.sync_mode = 0;
	p->cfg.bits_per_coded_sample = bits;
	p->cfg.channels = channels;
	p->cfg.sample_rate = samplerate;

	p->fd = open("/dev/auddec" , O_RDWR);
	if( p->fd < 0 ) {
		printf("Open /dev/auddec error.");
		free(p);
		return NULL;
	}

	if( ioctl(p->fd , AUDDEC_INIT , &p->cfg) != 0 ) {
		printf("Init auddec error.");
		close(p->fd);
		free(p);
		return NULL;
	}

	ioctl(p->fd , AUDDEC_START , 0);
	return p;
}

static void pcm_render_destroy(void *phandle)
{
	struct pcm_render *p = (struct pcm_render *)phandle;
	if( !p )
		return;

	if( p->fd > 0 ) {
		close(p->fd);
	}
	free(p);
}

static int pcm_render_rend(void *phandle, uint8_t *audio_frame, int packet_size)
{
	struct pcm_render *p = (struct pcm_render *)phandle;
	AvPktHd pkthd = { 0 };
	pkthd.dur = 0;
	pkthd.size = packet_size;
	pkthd.flag = AV_PACKET_ES_DATA;
	pkthd.pts = -1;
	while(write(p->fd , (uint8_t *)&pkthd , sizeof(AvPktHd)) != sizeof(AvPktHd) ) {
		//printf("Write AvPktHd fail\n");
		usleep(100*1000);
	}

	while(write(p->fd , audio_frame , packet_size) != packet_size ) {
		//printf("Write audio_frame error fail\n");
		usleep(100*1000);
	}

	return 0;
}

static void pcm_render_rend_eos_packet(void *phandle)
{
	struct pcm_render *p = (struct pcm_render *)phandle;
	AvPktHd pkthd = {0};

	pkthd.pts = 0;
	pkthd.size = 0;
	pkthd.flag = AV_PACKET_EOS;
	while(write(p->fd , (uint8_t *)&pkthd , sizeof(AvPktHd)) != sizeof(AvPktHd) ) {
		//printf("Write AvPktHd fail\n");
		usleep(100*1000);
	}
}

static int pcm_render_get_eos(void *phandle)
{
	struct pcm_render *p = (struct pcm_render *)phandle;
	int eos = 1;
	ioctl(p->fd, AUDDEC_CHECK_EOS, &eos);
	printf("eos %d\n", eos);
	return eos;
}

static void aac_play(void)
{
	char bufin[2048];
	char *ret;
	int size, read_size;
	int out_size;

	do {
		size = fread(bufin, 1, 4, datafile);
		if (size != 4) {
			printf("can not get size %d, %d\n", 4, size);
			break;
		}

		size = *(unsigned int *)bufin;

		read_size = fread(bufin, 1, size, datafile);
		if (read_size != size) {
			printf("can not get data %d, %d\n", size, read_size);
			break;
		}

		aac_eld_decode_frame(aacdec, bufin, read_size, out_buf, &out_size);
		pcm_render_rend(pcmrender, out_buf, out_size);
	} while (!g_stop_fdkaac_test);

	if (!g_stop_fdkaac_test) {
		pcm_render_rend_eos_packet(pcmrender);
	}

	while (!g_stop_fdkaac_test && !pcm_render_get_eos(pcmrender)) {
		printf("wait eos...\n");
		usleep(100*1000);
	}
}

static void *fdkaac_test_thread(void *arg)
{
	aacdec = create_aac_eld();

	out_buf = (void *)malloc(1024*1024);

	pcmrender = pcm_render_init(HC_AVCODEC_ID_PCM_S16LE, 16, 2, 44100);

	if (!datafile || !aacdec ||!out_buf || !pcmrender) {
		printf("fdkaac_test_thread preready err```\n");
		goto err;
	}

	aac_play();

err:

	if (datafile) {
		fclose(datafile);
		datafile = NULL;
	}

	if (out_buf) {
		free(out_buf);
		out_buf = NULL;
	}

	if (pcmrender) {
		pcm_render_destroy(pcmrender);
	}

	if (aacdec) {
		destroy_aac_eld(aacdec);
	}

	printf("fdkaac_test_thread exit\n");
	fdkaac_test_thread_id = 0;
	pthread_detach(pthread_self ());
	pthread_exit(NULL);
}

static int fdkaac_test(int argc, char *argv[])
{
	pthread_attr_t attr;

	if (argc < 2) {
		printf ("fdkaac datafile_path lenfile_path\n");
		return -1;
	}

	datafile = fopen (argv[1], "r");
	printf ("datafile %p, %s\n", datafile, argv[1]);

	if (!datafile) {
		goto err;
	}

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 0x4000);
	g_stop_fdkaac_test = 0;
	if(pthread_create(&fdkaac_test_thread_id, &attr, fdkaac_test_thread, NULL)) {
		goto err;
	}

	return 0;

err:
	if (datafile) {
		fclose(datafile);
		datafile = NULL;
	}
}

static int fdkaac_test_stop(int argc, char *argv[])
{
	g_stop_fdkaac_test = 1;
	return 0;
}

CONSOLE_CMD(fdkaac, NULL, fdkaac_test, CONSOLE_CMD_MODE_SELF,
		"do fdkaac test")
CONSOLE_CMD(fdkaac_stop, NULL, fdkaac_test_stop, CONSOLE_CMD_MODE_SELF,
		"fdkaac test stop")
