#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include "common.h"
#include <signal.h>
#include "decoder.h"
#include <netinet/tcp.h>
#include <sys/select.h>

#ifdef __HCRTOS__
#include <kernel/module.h>
#include <kernel/lib/console.h>
#endif
#include "raw_proto.h"

#define LENGTH_OF_LISTEN_QUEUE 20

#define DATA_BUFFER_SIZE (1024*1024*4)
#define CTRL_BUFFER_SIZE (64*1024)

static int audio_flush_flag = 0;
static int video_flush_flag = 0;
static uint32_t sample_rate = 48000;
#if 0
#define V_PRINT printf
#else
#define V_PRINT(...) do{}while(0)
#endif

void *audio_data_thread(void *argv)
{
	printf("%s\n", __func__);
	uint8_t *buffer = NULL;
	data_header_t header;
	void *adec_handle = NULL;

	adec_handle = aac_decoder_init(16, 2,  sample_rate);
	if(!adec_handle) {
		printf("Init adec error.\n");
		return NULL;
	}

	while(true) {

		buffer = NULL;
		if(raw_get_data(&header, &buffer, STREAM_AUDIO) != 0){
			printf("Get data error.\n");
			if(buffer)
				raw_free_data(buffer);
			continue;
		}

		if(audio_flush_flag) {
			aac_decoder_flush(adec_handle);
			audio_flush_flag = 0;
		}
		if(header.sample_rate != sample_rate){
			aac_decoder_destroy(adec_handle);
			adec_handle = aac_decoder_init(16, 2,  header.sample_rate);
			if(!adec_handle) {
				printf("Init adec error.\n");
				raw_free_data(buffer);
				return NULL;
			}
			sample_rate = header.sample_rate;
			video_flush_flag = 1;
			printf("audio reset\n");
		}
		/*printf("audio:header.size: %d, header.pts:%d\n", header.size, header.pts);*/
		if(aac_decode(adec_handle, buffer, header.size, header.pts))
			aac_decoder_flush(adec_handle);

		raw_free_data(buffer);
	}
	return NULL;
}

#define H264_DECODE_INIT_1080P

static void *video_data_thread(void *argv)
{
	printf("%s\n", __func__);
	uint8_t *buffer = NULL;
	data_header_t header;
	void *vdec_handle = NULL;

#ifdef H264_DECODE_INIT_1080P
	vdec_handle = h264_decode_init(1920, 1080,  NULL, 0);
#else
	vdec_handle = h264_decode_init(1280, 720,  NULL, 0);
#endif
	if(!vdec_handle) {
		printf("Init vdec error.\n");
		return NULL;
	}

	while(true) {
			buffer = NULL;
			if(raw_get_data(&header, &buffer, STREAM_VIDEO) != 0){
				printf("Get data error.\n");
				if(buffer)
					raw_free_data(buffer);
				continue;
			}

			if(video_flush_flag){
				h264_decoder_destroy(vdec_handle);
#ifdef H264_DECODE_INIT_1080P
				vdec_handle = h264_decode_init(1920, 1080,  NULL, 0);
#else
				vdec_handle = h264_decode_init(1280, 720,  NULL, 0);
#endif
				if(!vdec_handle) {
					printf("Init vdec error.\n");
					raw_free_data(buffer);
					return NULL;
				}
				printf("vdec reset\n");
				video_flush_flag  = 0;
			}

			/*printf("video:header.size: %ld, header.pts:%ld\n", header.size, header.pts);*/
			if(h264_decode(vdec_handle, buffer, header.size, header.pts)) {
				/*h264_decoder_flush(vdec_handle);*/
				h264_decoder_destroy(vdec_handle);

#ifdef H264_DECODE_INIT_1080P
				vdec_handle = h264_decode_init(1920, 1080,  NULL, 0);
#else
				vdec_handle = h264_decode_init(1280, 720,  NULL, 0);
#endif
				audio_flush_flag = 1;
			}
			raw_free_data(buffer);

	}
	return NULL;
}

#ifdef __HCRTOS__
int display_sink_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	pthread_t tid;
	raw_init("eth0", raw_recv_callback, NULL);

	raw_recv_init();

	pthread_create(&tid, NULL, video_data_thread, argv[1]);
	pthread_create(&tid, NULL, audio_data_thread, argv[1]);

#ifndef __HCRTOS__
	while (1) {
		usleep(1000*1000);
	}
#endif

	return 0;
}

#ifdef __HCRTOS__
CONSOLE_CMD(display_sink, NULL, display_sink_main, CONSOLE_CMD_MODE_SELF, "display_sink")
#endif
