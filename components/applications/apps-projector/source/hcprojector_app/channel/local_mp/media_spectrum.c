
#include "app_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <hcuapi/dumpstack.h>
#include <hcuapi/kshm.h>
#include <hcuapi/common.h>
#include "media_spectrum.h"
#include "mp_spectdis.h"
#include "app_config.h"

#ifdef AUDIO_SPECTRUM_SUPPORT
extern int *snd_spectrum_run(int *data, int ch, int bitdepth, int rate, int size, int *collumn_num);
extern void snd_spectrum_stop(void);
#endif
static bool stop_i2so_spectrum = 0;
static pthread_t i2so_spectrum_thread_id = 0;
int *spectrum = NULL;

void * get_music_spect_data(void)
{
    return spectrum;
}
static void *i2so_spectrum_thread(void *args)
{
	int snd_in_fd = -1;
	int collumn_num = 10;
	int ch = 2;
	struct kshm_info audio_read_hdl = {0};

	struct snd_hw_info hw_info = {0};
	uint32_t rec_buf_size = 0;
	void *tmp_buf = NULL;
	int tmp_buf_size = 0;
	int ret = 0;
	int time_elapsed = 0;
	int startup = 1;
	// int *spectrum = NULL;
	// int spect_count=0;
	snd_in_fd = open("/dev/sndC0i2so", O_WRONLY);
	if(snd_in_fd < 0) {
		printf("error: can not open i2so dev \n");
		goto err;
	}

	rec_buf_size = 300 * 1024;
	ret = ioctl(snd_in_fd, SND_IOCTL_SET_RECORD, rec_buf_size);
	printf("ret0 %d\n", ret);
	//ret = ioctl(snd_in_fd, SND_IOCTL_GET_HW_INFO, &hw_info);
	//printf("ret1 %d\n", ret);
	ret |= ioctl(snd_in_fd, KSHM_HDL_ACCESS, &audio_read_hdl);

#ifdef __linux__
	int akshm_fd = open("/dev/kshmdev" , O_RDONLY);	
	if (akshm_fd < 0) {
		goto err;
	}

	ret |= ioctl(akshm_fd, KSHM_HDL_SET, &audio_read_hdl);

#endif	

	printf("ret2 %d\n", ret);
	if (ret) {
		printf("set i2so rec failed %d\n", ret);
		ioctl(snd_in_fd, SND_IOCTL_SET_FREE_RECORD, 0);
		goto err;
	}
	printf("i2so rec start\n");

	while (!stop_i2so_spectrum) {
		AvPktHd hdr = {0};
	#ifdef __HCRTOS__
		while (kshm_read((kshm_handle_t)&audio_read_hdl, &hdr, sizeof(AvPktHd))
			!= sizeof(AvPktHd) && !stop_i2so_spectrum) 
	#else
		while (read(akshm_fd, &hdr, sizeof(AvPktHd))
			!= sizeof(AvPktHd) && !stop_i2so_spectrum) 
	#endif
		{
			usleep(10*1000);
		}

		if (tmp_buf_size < hdr.size) {
			tmp_buf = realloc(tmp_buf, hdr.size);
			if (!tmp_buf) {
				printf("no memory\n");
				break;
			}
			tmp_buf_size = hdr.size;
		}
	#ifdef __HCRTOS__
		while (kshm_read((kshm_handle_t)&audio_read_hdl, tmp_buf, hdr.size)
			!= hdr.size && !stop_i2so_spectrum) 
	#else
		while (read(akshm_fd, tmp_buf, hdr.size)
			!= hdr.size && !stop_i2so_spectrum) 
	#endif
		{
			usleep(20*1000);
		}

		ret = ioctl(snd_in_fd, SND_IOCTL_GET_HW_INFO, &hw_info);
		if (ret || hw_info.pcm_params.channels == 0 || hw_info.pcm_params.bitdepth == 0 || hw_info.pcm_params.rate == 0) {
			continue;
		}
#ifdef AUDIO_SPECTRUM_SUPPORT
		spectrum = snd_spectrum_run(tmp_buf, hw_info.pcm_params.channels, 
			(hw_info.pcm_params.bitdepth == 16) ? 16 : 24, hw_info.pcm_params.rate, hdr.size, &collumn_num);
#endif
		if (hw_info.pcm_params.bitdepth == 16)
			time_elapsed += ((hdr.size / hw_info.pcm_params.channels) >> 1) * 1000 / hw_info.pcm_params.rate;
		else
			time_elapsed += ((hdr.size / hw_info.pcm_params.channels) >> 2) * 1000 / hw_info.pcm_params.rate;

		if (startup || time_elapsed >= 100) {
			/* Refresh UI spectrum for every 100ms */
			music_spect_refresh_dis();
			time_elapsed = 0;
			startup = 0;
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
#ifdef __linux__
	if (akshm_fd > 0)
		close(akshm_fd);
#endif	
	(void)args;
	return NULL;
}

 int music_spectrum_stop(void)
{
	stop_i2so_spectrum = 1;
	if (i2so_spectrum_thread_id)
		pthread_join(i2so_spectrum_thread_id, NULL);
	i2so_spectrum_thread_id = 0;
        printf("\n>>> %s\n\n", __func__);

	return 0;
}

 int music_spectrum_start()
{
        pthread_attr_t attr;
        if (i2so_spectrum_thread_id) {
               music_spectrum_stop();
        }

        stop_i2so_spectrum = 0;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x1000);
        if (pthread_create(&i2so_spectrum_thread_id, &attr,
                                i2so_spectrum_thread, NULL)) {
                printf("create i2so spectrum thread failed\n");
        }
	printf("\n>>> %s\n\n", __func__);
	pthread_attr_destroy(&attr);

        return 0;
}


