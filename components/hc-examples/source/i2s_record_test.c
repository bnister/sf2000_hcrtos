#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/msg.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include "console.h"
#include <hcuapi/common.h>
#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <hcuapi/dumpstack.h>
#include <hcuapi/dis.h>

#include <ffplayer.h>
#include <glist.h>
#include <getopt.h>
#include <hcuapi/pinmux.h>
#include <hcuapi/kshm.h>
#include <sys/mman.h>

#include "wav.h"

static void *player = NULL;
static int play(int argc, char *argv[])
{
	if (argc > 1) {
		HCPlayerInitArgs init_args = {0};

		if (player) {
			hcplayer_stop(player);
			player = NULL;
		}
		init_args.uri = argv[1];
		hcplayer_init(LOG_WARNING);
		player = hcplayer_create(&init_args);
		if (player) {
			hcplayer_play(player);
		}

		return player?0:(-1);
	}

	return -1;
}

static int stop(int argc, char *argv[])
{
	if (player) {
		hcplayer_stop(player);
		player = NULL;
	}

	(void)argc;
	(void)argv;
	return 0;
}

/* below is i2si rec test code */
static int i2si_source = SND_PCM_SOURCE_AUDPAD;
static int i2si_rate = 44100;
static int i2si_format = 16;
static int i2si_channles = 2;
static bool stop_i2si = 0;
static FILE *arecfile = NULL;
static pthread_t i2si_rec_thread_id = 0;

static void *i2si_rec_thread(void *args)
{
	struct snd_pcm_params params;
	int read_size = 1536;
	char *buf = NULL;
	int ret = 0;
	struct pollfd pfd = {0};
	int snd_in_fd = 0;
	struct snd_xfer xfer = {0};
	struct wave_header header = {0};

	snd_in_fd = open("/dev/sndC0i2si", O_WRONLY);
	if (snd_in_fd < 0) {
		printf("open i2si dev failed\n");
		goto err;
	}

	buf = malloc (read_size);
	if (!buf) {
		printf("malloc rec buf failed\n");
		goto err;
	}
	memset(buf, 0, read_size);

	switch(i2si_format) {
		case 16:
			params.format = SND_PCM_FORMAT_S16_LE;
			params.bitdepth = 16;
			break;
		case 24:
			params.format = SND_PCM_FORMAT_S24_LE;
			params.bitdepth = 32;
			break;
		default:
			printf("unsupport i2si_format %d\n", i2si_format);
			goto err;
	}
	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = SND_PCM_ALIGN_RIGHT;
	params.rate = i2si_rate;
	params.channels = i2si_channles;
	params.period_size = read_size;
	params.periods = 16;
	params.start_threshold = 1;
	params.pcm_source = i2si_source;
	ret = ioctl(snd_in_fd, SND_IOCTL_HW_PARAMS, &params);
	if (ret) {
		printf("i2si hw config failed, ret %d\n", ret);
		goto err;
	}
	ret = ioctl(snd_in_fd, SND_IOCTL_START, 0);
	if (ret) {
		printf("i2si start failed, ret %d\n", ret);
		ioctl(snd_in_fd, SND_IOCTL_HW_FREE, 0);
		goto err;
	}

	generate_wave_header(&header, i2si_rate, params.bitdepth, i2si_channles);
	fwrite (&header, sizeof(struct wave_header), 1, arecfile);
	if (params.format == SND_PCM_FORMAT_S24_LE) {
		int offset[1] = {0};
		fwrite (offset, 1, 1, arecfile);
	}

	xfer.data = buf;
	xfer.frames = read_size / (params.channels * params.bitdepth / 8);
	do {
		ret = ioctl(snd_in_fd, SND_IOCTL_XFER, &xfer);
		if (ret < 0) {
			pfd.fd = snd_in_fd;
			pfd.events = POLLIN | POLLRDNORM;
			poll(&pfd, 1, 100);
			printf("ret SND_IOCTL_XFER %d\n",ret);
			continue;
		}

		fwrite (xfer.data, read_size, 1, arecfile);
	} while (!stop_i2si);
	printf ("record exit\n");

	ioctl(snd_in_fd, SND_IOCTL_DROP, 0);
	ioctl(snd_in_fd, SND_IOCTL_HW_FREE, 0);

err:
	if (snd_in_fd)
		close(snd_in_fd);
	if (buf)
		free(buf);

	(void)args;
	return NULL;
}

static int i2si_rec_stop(int argc, char *argv[])
{
	stop_i2si = 1;
	if (i2si_rec_thread_id)
		pthread_join(i2si_rec_thread_id, NULL);
	i2si_rec_thread_id = 0;

	if (arecfile) {
		fclose(arecfile);
		arecfile = NULL;
	}

	(void)argc;
	(void)argv;
	return 0;
}

static int i2si_rec_start(int argc, char *argv[])
{
	int opt;
	char *uri = "/media/hdd/astream";

	opterr = 0;
	optind = 0;
	i2si_source = SND_PCM_SOURCE_AUDPAD;
	i2si_format = 16;
	i2si_rate = 44100;
	i2si_channles = 2;

	if (arecfile) {
		i2si_rec_stop(0, 0);
	}

	while ((opt = getopt(argc, argv, "p:s:r::f:c")) != EOF) {
		switch (opt) {
		case 'p':
			uri = optarg;
			break;
		case 's':
			i2si_source = atoi(optarg);
			break;
        case 'r':
			i2si_rate = atoi(optarg);
			break;
        case 'f':
			i2si_format = atoi(optarg);
			break;
        case 'c':
			i2si_channles = atoi(optarg);
			break;
		default:
			break;
		}
	}

	arecfile = fopen(uri, "wb");
	if (!arecfile) {
		return -1;
	}

	stop_i2si = 0;
	if (pthread_create(&i2si_rec_thread_id, NULL,
		i2si_rec_thread, NULL)) {
		printf("create i2si rec thread failed\n");
		if (arecfile) {
			fclose(arecfile);
			arecfile = NULL;
		}
	}

	return 0;
}

/* below is i2so rec test code */
static FILE *i2so_recfile = NULL;
static bool stop_i2so_rec = 0;
static pthread_t i2so_rec_thread_id = 0;
static void *i2so_rec_thread(void *args)
{
	int snd_in_fd = -1;
	int kshm_fd = -1;
	struct kshm_info audio_read_hdl = {0};
	struct snd_hw_info hw_info = {0};
	uint32_t rec_buf_size = 0;
	void *tmp_buf = NULL;
	int tmp_buf_size = 0;
	int ret = 0;
	struct wave_header header = {0};
	int offset[1] = {0};

	snd_in_fd = open("/dev/sndC0i2so", O_WRONLY);
	if(snd_in_fd < 0) {
		printf("error: can not open i2so dev \n");
		goto err;
	}

	kshm_fd = open("/dev/kshmdev", O_RDONLY);
	if (kshm_fd < 0) {
		goto err;
	}

	rec_buf_size = 300 * 1024;
	ret = ioctl(snd_in_fd, SND_IOCTL_SET_RECORD, rec_buf_size);
	printf("ret0 %d\n", ret);
	ret = ioctl(snd_in_fd, SND_IOCTL_GET_HW_INFO, &hw_info);
	printf("ret1 %d\n", ret);
	ret |= ioctl(snd_in_fd, KSHM_HDL_ACCESS, &audio_read_hdl);
	printf("ret2 %d\n", ret);
	ret |= ioctl(kshm_fd, KSHM_HDL_SET, &audio_read_hdl);
	printf("ret3 %d\n", ret);
	if (ret) {
		printf("set i2so rec failed %d\n", ret);
		ioctl(snd_in_fd, SND_IOCTL_SET_FREE_RECORD, 0);
		goto err;
	}
	printf("i2so rec start\n");

	generate_wave_header(&header, 44100, 32, 2);
	fwrite (&header, sizeof(struct wave_header), 1, i2so_recfile);
	fwrite (offset, 1, 1, i2so_recfile);//SND_PCM_FORMAT_S24_LE need 3 bytes

	while (!stop_i2so_rec) {
		AvPktHd hdr = {0};
		while (read(kshm_fd, &hdr, sizeof(AvPktHd)) != sizeof(AvPktHd)
			&& !stop_i2so_rec) {
			usleep(10*1000);
		}

		if (tmp_buf_size < hdr.size) {
			tmp_buf_size = hdr.size;
			if(tmp_buf) {
				tmp_buf = realloc(tmp_buf, tmp_buf_size);
			} else {
				tmp_buf = malloc(tmp_buf_size);
			}
		}
		while (read(kshm_fd, tmp_buf, hdr.size) != hdr.size
			&& !stop_i2so_rec) {
			usleep(20*1000);
		}

		if (i2so_recfile) {
			fwrite (tmp_buf, hdr.size, 1, i2so_recfile);
		}
	}

	ioctl(snd_in_fd, SND_IOCTL_SET_FREE_RECORD, 0);

err:
	printf("i2so rec exit\n");
	if (snd_in_fd >= 0) {
		close(snd_in_fd);
	}

	if (tmp_buf) {
		free(tmp_buf);
	}

	if (kshm_fd >= 0) {
		close(kshm_fd);
	}

	(void)args;
	return NULL;
}

static int i2so_rec_stop(int argc, char *argv[])
{
	stop_i2so_rec = 1;
	if (i2so_rec_thread_id)
		pthread_join(i2so_rec_thread_id, NULL);
	i2so_rec_thread_id = 0;

	if (i2so_recfile) {
		fclose(i2so_recfile);
		i2so_recfile = NULL;
	}

	(void)argc;
	(void)argv;
	return 0;
}

static int i2so_rec_start(int argc, char *argv[])
{
	int opt;
	char *uri = "/media/hdd/astream";

	opterr = 0;
	optind = 0;

	if (i2so_recfile || i2so_rec_thread_id) {
		i2so_rec_stop(0, 0);
	}

	while ((opt = getopt(argc, argv, "p:")) != EOF) {
		switch (opt) {
		case 'p':
			uri = optarg;
			break;
		default:
			break;
		}
	}

	i2so_recfile = fopen(uri, "wb");
	if (!i2so_recfile) {
		return -1;
	}

	stop_i2so_rec = 0;
	if (pthread_create(&i2so_rec_thread_id, NULL,
		i2so_rec_thread, NULL)) {
		printf("create i2so rec thread failed\n");
		if (i2so_recfile) {
			fclose(i2so_recfile);
			i2so_recfile = NULL;
		}
	}

	return 0;
}

/* below is i2so spectrum test code */
extern int *snd_spectrum_run(int *data, int ch, int bitdepth, int rate, int size, int *collumn_num);
extern void snd_spectrum_stop(void);
static bool stop_i2so_spectrum = 0;
static pthread_t i2so_spectrum_thread_id = 0;
static void *i2so_spectrum_thread(void *args)
{
	int snd_in_fd = -1;
	int collumn_num = 9;
	int kshm_fd = -1;
	struct kshm_info audio_read_hdl = {0};
	struct snd_hw_info hw_info = {0};
	uint32_t rec_buf_size = 0;
	void *tmp_buf = NULL;
	int tmp_buf_size = 0;
	int ret = 0;
	int *spectrum = NULL;

	snd_in_fd = open("/dev/sndC0i2so", O_WRONLY);
	if(snd_in_fd < 0) {
		printf("error: can not open i2so dev \n");
		goto err;
	}

	kshm_fd = open("/dev/kshmdev", O_RDONLY);
	if (kshm_fd < 0) {
		goto err;
	}

	rec_buf_size = 300 * 1024;
	ret = ioctl(snd_in_fd, SND_IOCTL_SET_RECORD, rec_buf_size);
	printf("ret0 %d\n", ret);
	ret = ioctl(snd_in_fd, SND_IOCTL_GET_HW_INFO, &hw_info);
	printf("ret1 %d\n", ret);
	ret |= ioctl(snd_in_fd, KSHM_HDL_ACCESS, &audio_read_hdl);
	printf("ret2 %d\n", ret);
	ret |= ioctl(kshm_fd, KSHM_HDL_SET, &audio_read_hdl);
	printf("ret3 %d\n", ret);
	if (ret) {
		printf("set i2so rec failed %d\n", ret);
		ioctl(snd_in_fd, SND_IOCTL_SET_FREE_RECORD, 0);
		goto err;
	}
	printf("i2so rec start\n");

	while (!stop_i2so_rec) {
		AvPktHd hdr = {0};
		while (read(kshm_fd, &hdr, sizeof(AvPktHd)) != sizeof(AvPktHd)
			&& !stop_i2so_rec) {
			usleep(10*1000);
		}

		if (tmp_buf_size < hdr.size) {
			tmp_buf_size = hdr.size;
			if(tmp_buf) {
				tmp_buf = realloc(tmp_buf, tmp_buf_size);
			} else {
				tmp_buf = malloc(tmp_buf_size);
			}
		}
		while (read(kshm_fd, tmp_buf, hdr.size) != hdr.size
			&& !stop_i2so_rec) {
			usleep(20*1000);
		}

		spectrum = snd_spectrum_run(tmp_buf, hw_info.pcm_params.channels, (hw_info.pcm_params.bitdepth == 16) ? 16 : 24, hw_info.pcm_params.rate, hdr.size, &collumn_num);
		if (spectrum && collumn_num > 0) {
			printf("spectrum: ");
			for (int i = 0; i < collumn_num; i++) {
				printf(" %d", spectrum[i]);
			}
			printf("\n");
		}
		usleep(8*1000);
	}

	ioctl(snd_in_fd, SND_IOCTL_SET_FREE_RECORD, 0);
	snd_spectrum_stop();

err:
	printf("i2so rec exit\n");
	if (snd_in_fd >= 0) {
		close(snd_in_fd);
	}

	if (tmp_buf) {
		free(tmp_buf);
	}

	if (kshm_fd >= 0) {
		close(kshm_fd);
	}

	(void)args;
	return NULL;
}

static int i2so_spectrum_stop(int argc, char *argv[])
{
	stop_i2so_spectrum = 1;
	if (i2so_spectrum_thread_id)
		pthread_join(i2so_spectrum_thread_id, NULL);
	i2so_spectrum_thread_id = 0;

	(void)argc;
	(void)argv;
	return 0;
}

static int i2so_spectrum_start(int argc, char *argv[])
{
	if (i2so_spectrum_thread_id) {
		i2so_spectrum_stop(0, 0);
	}

	stop_i2so_spectrum = 0;
	if (pthread_create(&i2so_spectrum_thread_id, NULL,
		i2so_spectrum_thread, NULL)) {
		printf("create i2so spectrum thread failed\n");
	}

	(void)argc;
	(void)argv;
	return 0;
}

static struct termios stored_settings;
static void exit_console(int signo)
{
	(void)signo;
	i2si_rec_stop(0, 0);
	i2so_rec_stop(0, 0);
	stop(0, 0);
	tcsetattr (0, TCSANOW, &stored_settings);
	exit(0);
}

int main (int argc, char *argv[])
{
	struct termios new_settings;

	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;
	new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);

	signal(SIGTERM, exit_console);
	signal(SIGINT, exit_console);
	signal(SIGSEGV, exit_console);
	signal(SIGBUS, exit_console);
	console_init("i2s_test:");

	console_register_cmd(NULL, "play", play, CONSOLE_CMD_MODE_SELF, "play uri");
	console_register_cmd(NULL, "stop", stop, CONSOLE_CMD_MODE_SELF, "stop");
	console_register_cmd(NULL, "i2si_rec_start", i2si_rec_start,
		CONSOLE_CMD_MODE_SELF,
		"i2si_rec_start -p dst_path(default /mnt/hdd/astream if not set) -s source(0:i2si pin pad, 1:hdmi_rx) -r rate -f format(16LE/24LE) -c ch_num");
	console_register_cmd(NULL, "i2si_rec_stop", i2si_rec_stop,
		CONSOLE_CMD_MODE_SELF, "i2si_rec_stop");
	console_register_cmd(NULL, "i2so_rec_start", i2so_rec_start,
		CONSOLE_CMD_MODE_SELF, "i2so_rec_start -p dst_path(default /mnt/hdd/astream if not set)");
	console_register_cmd(NULL, "i2so_rec_stop", i2so_rec_stop,
		CONSOLE_CMD_MODE_SELF, "i2so_rec_stop");
	console_register_cmd(NULL, "i2so_spectrum_start", i2so_spectrum_start,
		CONSOLE_CMD_MODE_SELF, "i2so_spectrum_start");
	console_register_cmd(NULL, "i2so_spectrum_stop", i2so_spectrum_stop,
		CONSOLE_CMD_MODE_SELF, "i2so_spectrum_stop");

	console_start();
	exit_console(0);
	(void)argc;
	(void)argv;
	return 0;
}

