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
#include <errno.h>

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
#include <hcuapi/vindvp.h>
#include <hcuapi/tvtype.h>
#include "vin_dvp_test.h"

static int vin_dvp_fd = -1;
static int vkshm_fd = -1;
static bool video_stop_read = 0;
static struct kshm_info video_read_hdl = { 0 };
static enum VINDVP_VIDEO_DATA_PATH vpath = VINDVP_VIDEO_TO_DE;
static unsigned int rotate_mode = ROTATE_TYPE_0;
static enum TVTYPE tv_sys = TV_NTSC;
static bool vin_dvp_started = false;
static unsigned int combine_mode = VINDVP_COMBINED_MODE_DISABLE;
static bool creat_read_task = false;
static pthread_t vin_dvp_video_read_thread_id = 0;
static pthread_attr_t attr;
static int value = 0;
static void *vin_dvp_video_read_thread(void *args)
{
	uint32_t  data_size = 0;
	void *data = NULL;
	printf("vkshm_fd %d\n", vkshm_fd);
	printf("video_read_thread run*****************************************```\n");
	while(!video_stop_read && vkshm_fd >= 0)
	{
		AvPktHd hdr = { 0 };
		while(read(vkshm_fd , &hdr , sizeof(AvPktHd)) != sizeof(AvPktHd))
		{
			//printf("read audio hdr from kshm err %d\n", vkshm_fd);
			//printf("error: %s\n", strerror(errno));
			usleep(20 * 1000);
			if (video_stop_read) {
				goto end;
			}
		}
		printf("pkt size 0x%x, flag %d\n" , (int)hdr.size , (int)hdr.flag);

		if (data_size < hdr.size) {
			data_size = hdr.size;
			if(data) {
				data = realloc(data, data_size);
			} else {
				data = malloc(data_size);
			}
			if(!data)
			{
				printf("vin_dvp_video_read_thread：no memory\n");
				return NULL;
			}
		}

		while(read(vkshm_fd , data , hdr.size) != hdr.size)
		{
			printf("read audio data from kshm err\n");
			usleep(20 * 1000);
			if (video_stop_read) {
				goto end;
			}

		}
		printf("data: 0x%x\n" , (unsigned int)data);
	}
	usleep(1000);

end:
	if(data)
	free(data);

	(void)args;
	printf("out video_read_thread run*****************************************```\n");

	return NULL;
}

int vindvp_start(int argc , char *argv[])
{
	opterr = 0;
	optind = 0;

	int opt;
	int ret;

	if(vin_dvp_started == true)
	{
		return 0;
	}

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr , 0x10000);

	vpath = VINDVP_VIDEO_TO_DE;
	rotate_mode = ROTATE_TYPE_0;
	combine_mode = VINDVP_COMBINED_MODE_DISABLE;

	vin_dvp_fd = open("/dev/vindvp" , O_WRONLY);
	if(vin_dvp_fd < 0)
	{
	printf("open vindvp dev failed\n");
	return -1;
	}

	vkshm_fd = open("/dev/kshmdev" , O_RDWR);
	if(vkshm_fd < 0)
	{
		printf("open kshm dev failed\n");
		return -1;
	}
	printf("vkshm fd %d\n", vkshm_fd);
	while((opt = getopt(argc , argv , "v:r:t:c:")) != EOF)
	{
		switch(opt)
		{
		case 'v':
			vpath = atoi(optarg);
		break;
		case 'r':
			rotate_mode = atoi(optarg);
			break;
		case 't':
			value = atoi(optarg);
			break;
		case 'c':
			combine_mode = atoi(optarg);
			break;
		default:
			break;
		}
	}

	printf("vpath %d\n" , vpath);
	printf("combine_mode %d\n" , combine_mode);
	ret = ioctl(vin_dvp_fd , VINDVP_SET_VIDEO_DATA_PATH , vpath);
	if (ret < 0) {
		printf("VINDVP_SET_VIDEO_DATA_PATH error\n");
	}
	ret = ioctl(vin_dvp_fd , VINDVP_SET_COMBINED_MODE , combine_mode);
	if (ret < 0) {
		printf("VINDVP_SET_COMBINED_MODE error\n");
	}

	if (vpath == VINDVP_VIDEO_TO_KSHM ||
	vpath == VINDVP_VIDEO_TO_DE_AND_KSHM ||
	vpath == VINDVP_VIDEO_TO_DE_ROTATE_AND_KSHM) {

	ret = ioctl(vin_dvp_fd , VINDVP_VIDEO_KSHM_ACCESS , &video_read_hdl);
	if (ret < 0) {
		printf("VINDVP_VIDEO_KSHM_ACCESS error\n");
	}
	//printf("get video hdl, kshm desc 0x%x\n" , (int)video_read_hdl.desc);

	ret = ioctl(vkshm_fd, KSHM_HDL_SET, &video_read_hdl);
	if (ret < 0) {
		printf("KSHM_HDL_SET error\n");
	}

	if(pthread_create(&vin_dvp_video_read_thread_id , &attr ,
	vin_dvp_video_read_thread , NULL)) {
		printf("audio kshm recv thread create failed\n");
		goto err;
	}

	video_stop_read = 0;
	creat_read_task = true;
	}

	if(vpath == VINDVP_VIDEO_TO_DE_ROTATE ||
	vpath == VINDVP_VIDEO_TO_DE_ROTATE_AND_KSHM)
	{
		printf("rotate_mode = 0x%x\n" , rotate_mode);
		ioctl(vin_dvp_fd , VINDVP_SET_VIDEO_ROTATE_MODE , rotate_mode);
	}

	ioctl(vin_dvp_fd , VINDVP_START , tv_sys);
	printf("vin dvp start ok\n");
	vin_dvp_started = true;
	return 0;

err:
	close(vin_dvp_fd);
	vin_dvp_fd = -1;
	close(vkshm_fd);
	vkshm_fd = -1;
	return -1;

}

int vindvp_stop(int argc , char *argv[])
{
	(void)argc;
	(void)argv;
	if(vin_dvp_fd >= 0)
	{
		video_stop_read = 1;
		vin_dvp_started = false;

		if(vpath == VINDVP_VIDEO_TO_KSHM ||
		vpath == VINDVP_VIDEO_TO_DE_AND_KSHM ||
		vpath == VINDVP_VIDEO_TO_DE_ROTATE_AND_KSHM)
		{
			pthread_attr_destroy(&attr);
			if(vin_dvp_video_read_thread_id)
			pthread_join(vin_dvp_video_read_thread_id , NULL);
		}

		if(vkshm_fd >= 0) {
			close(vkshm_fd);
			vkshm_fd = -1;
		}

		ioctl(vin_dvp_fd , VINDVP_STOP);
		close(vin_dvp_fd);
		vin_dvp_fd = -1;
		return 0;
	}
	else
	{
		return -1;
	}
}

int vindvp_enter(int argc , char *argv[])
{
	(void)argc;
	(void)argv;
	return 0;
}


int vindvp_enable(int argc , char *argv[])
{
	(void)argc;
	(void)argv;
	if(vin_dvp_fd >= 0)
	{
		ioctl(vin_dvp_fd , VINDVP_ENABLE);
	}
	return 0;
}

int vindvp_disable(int argc , char *argv[])
{
	(void)argc;
	(void)argv;

	if(vin_dvp_fd >= 0)
	{
		ioctl(vin_dvp_fd , VINDVP_DISABLE);
	}
	return 0;
}


int vindvp_set_combine_regin_status(int argc , char *argv[])
{
	int opt;
	opterr = 0;
	optind = 0;

	combine_region_freeze_cfg_t region_cfg = {0};

	while((opt = getopt(argc , argv , "i:s:")) != EOF)
	{
		switch(opt)
		{
		case 'i':
			region_cfg.region = atoi(optarg);
			break;
		case 's':
			region_cfg.status = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if(vin_dvp_fd >= 0)
	{
		ioctl(vin_dvp_fd , VINDVP_SET_COMBINED_REGION_FREEZE ,&region_cfg);
	}

	return 0;
}

int vindvp_capture_pictrue(int argc , char *argv[])
{
	int opt;
	opterr = 0;
	optind = 0;
	pthread_attr_t attr;

	vindvp_combined_capture_mode_e capture_mode = VINDVP_COMBINED_CAPTRUE_ORIGINAL;

	while((opt = getopt(argc , argv , "m:")) != EOF)
	{
		switch(opt)
		{
		case 'm':
			capture_mode = (vindvp_combined_capture_mode_e)atoi(optarg);
			break;

		default:
			break;
		}
	}

	if(vin_dvp_fd >= 0)
	{
		ioctl(vin_dvp_fd , VINDVP_CAPTURE_ONE_PICTURE , capture_mode);

		int ret;

		if(false == creat_read_task)
		{
			ioctl(vin_dvp_fd , VINDVP_VIDEO_KSHM_ACCESS , &video_read_hdl);

			ret = ioctl(vkshm_fd, KSHM_HDL_SET, &video_read_hdl);
			printf("ret1 %d\n", ret);

			video_stop_read = 0;
			if(pthread_create(&vin_dvp_video_read_thread_id , &attr ,
			vin_dvp_video_read_thread , NULL)) {
				printf("audio kshm recv thread create failed\n");
				goto err;
			}

			creat_read_task = true;
		}

	}

	return 0;

err:
	close(vin_dvp_fd);
	vin_dvp_fd = -1;
	close(vkshm_fd);
	vkshm_fd = -1;
	return -1;
}
