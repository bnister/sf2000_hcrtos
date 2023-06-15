#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h> 
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <termios.h>
#include <signal.h>
#include "console.h"
#else
#include <kernel/lib/console.h>
#endif

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/tvdec.h>
#include <hcuapi/tvtype.h>

static int tv_dec_fd = -1;
static pthread_t tv_dec_video_read_thread_id = 0;
static bool video_stop_read = 1;
static enum TVDEC_VIDEO_DATA_PATH vpath = TVDEC_VIDEO_TO_DE;
static unsigned int rotate_mode = ROTATE_TYPE_0;
static unsigned int mirror_mode = MIRROR_TYPE_NONE;
static enum TVTYPE tv_sys = TV_NTSC;
static int kshm_fd = -1;
static FILE *recfile = NULL;

static void *tv_dec_video_read_thread(void *args)
{
	int data_size = 0;
	uint8_t *data = NULL;

	//printf("rx_audio_read_thread run\n");
	while (!video_stop_read && kshm_fd >= 0) {
		AvPktHd hdr = {0};
		while (read(kshm_fd, &hdr, sizeof(AvPktHd)) != sizeof(AvPktHd)){
			//printf("read audio hdr from kshm err\n");
			usleep(20*1000);
			if (video_stop_read) {
				goto end;
			}
		}
		printf("get pkt size %d\n", (int)hdr.size);

		if (data_size < hdr.size) {
			data_size = hdr.size;
			if (data) {
				data = realloc(data, data_size);
			} else {
				data = malloc(data_size);
			}
			if (!data) {
				printf("no memory\n");
				goto end;
			}
		}

		while (read(kshm_fd, data, hdr.size) != hdr.size){
			//printf("read audio data from kshm err\n");
			usleep(20*1000);
			if (video_stop_read) {
				goto end;
			}
		}

		//printf("adata: 0x%x, 0x%x, 0x%x, 0x%x\n", data[0], data[1], data[2], data[3]);
		if (recfile) {
			fwrite (data, hdr.size, 1, recfile);
		}
		usleep(1000);
	}

end:
	if(data)
		free(data);

	(void)args;
	
	return NULL;
}

static int tv_dec_stop(int argc , char *argv[])
{
    video_stop_read = 1;
	if (tv_dec_video_read_thread_id)
		pthread_join(tv_dec_video_read_thread_id, NULL);
	tv_dec_video_read_thread_id = 0;

	if (recfile)
		fclose(recfile);
	recfile = NULL;

	if (kshm_fd >= 0)
		close (kshm_fd);
	kshm_fd = -1;

	if (tv_dec_fd >= 0) {
		ioctl(tv_dec_fd , TVDEC_STOP);
		close (tv_dec_fd);
	}
	tv_dec_fd = -1;

	(void)argc;
	(void)argv;
	return 0;
}

static int tv_dec_start(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    int value = 0;

    if (tv_dec_fd < 0) {
        tv_dec_stop(0, 0);
    }

    vpath = TVDEC_VIDEO_TO_DE;
    rotate_mode = ROTATE_TYPE_0;

    tv_dec_fd = open("/dev/tv_decoder" , O_WRONLY);
    if (tv_dec_fd < 0) {
        return -1;
    }

    while((opt = getopt(argc , argv , "v:r:t:m:")) != EOF)
    {
        switch(opt)
        {
            case 'v':
                vpath = atoi(optarg);
                break;
            case 'r':
                rotate_mode = atoi(optarg) % 4;
                break;
            case 'm':
                mirror_mode = atoi(optarg);
                break;
            case 't':
                value = atoi(optarg);
                if (value == 0) {
                    tv_sys = TV_PAL;
                } else {
                    tv_sys = TV_NTSC;
                }
                break;
            default:
                break;
        }
    }

    printf("vpath %d\n" , vpath);
	if (vpath > TVDEC_VIDEO_TO_DE_ROTATE_AND_KSHM) {
		goto err;
	}
    ioctl(tv_dec_fd , TVDEC_SET_VIDEO_DATA_PATH , vpath);

    if(vpath == TVDEC_VIDEO_TO_KSHM || 
        vpath == TVDEC_VIDEO_TO_DE_AND_KSHM||
        vpath == TVDEC_VIDEO_TO_DE_ROTATE_AND_KSHM) {
        struct kshm_info kshm_hdl = { 0 };

		recfile = fopen("/media/hdd/vstream", "wb");
		if (!recfile) {
			goto err;
		}

		kshm_fd = open("/dev/kshmdev", O_RDONLY);
		if (kshm_fd < 0) {
			goto err;
		}

        ioctl(tv_dec_fd , TVDEC_VIDEO_KSHM_ACCESS , &kshm_hdl);
		ioctl(kshm_fd, KSHM_HDL_SET, &kshm_hdl);

        video_stop_read = 0;
		if (pthread_create(&tv_dec_video_read_thread_id, NULL, 
			tv_dec_video_read_thread, NULL)) {
			printf("tv dec kshm recv thread create failed\n");
			goto err;
		}
    }

    if(vpath == TVDEC_VIDEO_TO_DE_ROTATE ||
       vpath == TVDEC_VIDEO_TO_DE_ROTATE_AND_KSHM) {
        printf("rotate_mode = 0x%x\n" , rotate_mode);
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_ROTATE_MODE , rotate_mode);
		
        printf("mirror_mode = 0x%x\n" , mirror_mode);
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_MIRROR_MODE , mirror_mode);
    }

    ioctl(tv_dec_fd , TVDEC_START , tv_sys);
    printf("tv_dec start ok\n");
    return 0;

err:
	tv_dec_stop(0, 0);
	return -1;
}

static struct termios stored_settings;
static void exit_console(int signo)
{
	(void)signo;
	tv_dec_stop(0, 0);
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
	console_init("tv_dec:");
	
	console_register_cmd(NULL, "start", tv_dec_start, CONSOLE_CMD_MODE_SELF, "start -v vmode(0:de; 1:de & rotate; 2:rec; 3:de & rec; 4: de & rotate & rec) -r angle(0~3)");
	console_register_cmd(NULL, "stop", tv_dec_stop, CONSOLE_CMD_MODE_SELF, "stop");

	console_start();
	exit_console(0);
	(void)argc;
	(void)argv;
	return 0;
}
