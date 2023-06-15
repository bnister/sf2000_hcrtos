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
/*#include <linux/tcp.h>*/
#include "decoder.h"
#include <netinet/tcp.h>
#include <sys/select.h>

#ifdef __HCRTOS__
#include <kernel/module.h>
#include <kernel/lib/console.h>
#endif

#define LENGTH_OF_LISTEN_QUEUE 20

#define DATA_BUFFER_SIZE (1024*1024*4)
#define CTRL_BUFFER_SIZE (64*1024)

/*static int audio_ctr_sock;*/
static int audio_data_sock;
/*static int video_ctr_sock;*/
static int video_data_sock;
static int audio_flush_flag = 0;
static int video_flush_flag = 0;
static uint32_t sample_rate = 48000;
#if 0
#define V_PRINT printf
#else
#define V_PRINT(...) do{}while(0)
#endif

static int create_socket(int16_t port)
{

	int fd = -1;
	struct sockaddr_in my_addr;
	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htons(INADDR_ANY);
	my_addr.sin_port = htons(port);
#ifndef USE_UDP_PROTOCOL
	fd = socket(AF_INET, SOCK_STREAM, 0);
#else
	fd = socket(AF_INET, SOCK_DGRAM, 0);
#endif
	if(fd < 0) {
		printf("socket create error. port is %d\n", port);
		return -1;
	}

	{
		int opt = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	}

	int defRcvBufSize = -1;
	socklen_t optlen = DATA_BUFFER_SIZE;
	if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &defRcvBufSize, &optlen) < 0) {
		printf("getsockopt error=%d(%s)!!!\n", errno, strerror(errno));
		goto fail;
	}

#ifndef USE_UDP_PROTOCOL
	int keepAlive = 1;    // 非0值，开启keepalive属性

	int keepIdle = 60;    // 如该连接在60秒内没有任何数据往来,则进行此TCP层的探测

	int keepInterval = 2; // 探测发包间隔为5秒

	int keepCount = 3;        // 尝试探测的最多次数

	// 开启探活
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));

	setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));

	setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));

	setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

	/*printf("defRcvBufSize: %d\n", defRcvBufSize);*/
	if(bind(fd, (struct sockaddr *)&my_addr, sizeof(my_addr))) {
		printf("bind error. error: %s", strerror(errno));
		goto fail;
	}

	if(listen(fd, LENGTH_OF_LISTEN_QUEUE)) {
		printf("listen error. error: %s\n", strerror(errno));
		goto fail;
	}
#else
	if(bind(fd, (struct sockaddr *)&my_addr, sizeof(my_addr))) {
		printf("bind error. error: %s", strerror(errno));
		goto fail;
	}
#endif

	return fd;
fail:
	if(fd > 0)
		close(fd);
	return -1;
}

static int recv_wait(int sockt, void *buf, signed int len)
{
	signed int want_read; // r6@1
	int ret; // r0@2
	size_t read_size; // r2@6

	want_read = len;
	if ( sockt == -1 )
		return -1;

	if ( want_read >= 0x8000 )
		read_size = 0x8000;
	else
		read_size = want_read;

	ret = recv(sockt, (char*)buf, read_size, 0);
	if ( !ret ){
		printf("%s:%d, ret: %d, %s\n", __func__, __LINE__, ret, strerror(errno));
		ret = -1;
	}

	return ret;
}

static int recv_data(int socketfd, uint8_t *buf, int size)
{
	int remain = size;
	uint8_t *p = buf;
	int data_len;
#ifndef USE_UDP_PROTOCOL
	while( remain > 0) {
		data_len = recv_wait(socketfd, p, remain);
		if(data_len < 0) {
			printf("%s,%d,send error, error: %s, data_len: %d\n",__func__, __LINE__, strerror(errno), data_len);
			return -1;
		}
		V_PRINT("remain: %d, data_len : %d\n", remain, data_len);
		remain -= data_len;
		p += data_len;
	}
	V_PRINT("recv playload size: %d\n", size);
#else
	while(remain > 0){
		data_len = recvfrom(socketfd, p, remain, 0, NULL, NULL);
		if(data_len < 0){
			return -1;
		}
		remain -= data_len;
		p += data_len;
	}
#endif
	return 0;
}

#define H264_DECODE_INIT_1080P

#ifndef USE_UDP_PROTOCOL
void *audio_data_thread(void *argv)
{
	printf("%s\n", __func__);
	uint8_t *buffer = NULL;
	int client_sock = -1;
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	data_header_t header;
	void *adec_handle = NULL;
	if(audio_data_sock <=0) {
		printf("audio_data_sock = %d\n", audio_data_sock);
		exit(-1);
	}

	buffer = malloc(DATA_BUFFER_SIZE);
	if(!buffer) {
		printf("%s:%d, Not enough memory\n", __func__, __LINE__);
		exit(-1);
	}
	adec_handle = aac_decoder_init(16, 2,  sample_rate);
	if(!adec_handle) {
		printf("Init adec error.\n");
		exit(-1);
	}

	while(true) {
		printf("wait audio connect.\n");

		client_sock = accept(audio_data_sock, (struct sockaddr *)&client_addr, &len);
		printf("audio connect.\n");
		if(EAGAIN == client_sock) {
			perror("again\n");
			continue;
		} else if(client_sock < 0) {
			printf("%s:%d, client_sock: %d, error: %s\n", __func__, __LINE__, client_sock, strerror(errno));
			close(audio_data_sock);
			if(create_socket(AUDIO_DATA_PORT)) {
				printf("Create socket error\n");
				break;
			}
			continue;
		}

		while(true) {
			//read header
			if(recv_data(client_sock, (uint8_t *)&header, sizeof(header)) != 0) {
				close(client_sock);
				client_sock = -1;
				printf("%s:%d\n", __func__, __LINE__);
				break;
			}
			if(recv_data(client_sock, buffer, header.size) != 0) {
				close(client_sock);
				client_sock = -1;
				printf("%s:%d\n", __func__, __LINE__);
				break;
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
					exit(-1);
				}
				sample_rate = header.sample_rate;
				video_flush_flag = 1;
				printf("audio reset\n");
			}
			if(aac_decode(adec_handle, buffer, header.size, header.pts))
				aac_decoder_flush(adec_handle);

		}
	}
	return NULL;
}

void *video_data_thread(void *argv)
{
	printf("%s\n", __func__);
	uint8_t *buffer = NULL;
	int client_sock = -1;
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	data_header_t header;
	void *vdec_handle = NULL;
	if(video_data_sock <=0) {
		printf("video_data_sock = %d\n", video_data_sock);
		exit(-1);
	}

	buffer = malloc(DATA_BUFFER_SIZE);
	if(!buffer) {
		printf("%s:%d, Not enough memory\n", __func__, __LINE__);
		exit(-1);
	}
#ifdef H264_DECODE_INIT_1080P
	vdec_handle = h264_decode_init(1920, 1080,  NULL, 0);
#else
	vdec_handle = h264_decode_init(1280, 720,  NULL, 0);
#endif
	if(!vdec_handle) {
		printf("Init vdec error.\n");
		exit(-1);
	}

	while(true) {
		printf("wait video connect.\n");

		client_sock = accept(video_data_sock, (struct sockaddr *)&client_addr, &len);
		printf("video connect.\n");
		if(EAGAIN == client_sock) {
			continue;
		} else if(client_sock < 0) {
			printf("%s:%d, client_sock: %d, error: %s\n", __func__, __LINE__, client_sock, strerror(errno));
			close(video_data_sock);
			if(create_socket(VIDEO_DATA_PORT)) {
				printf("Create socket error\n");
				break;
			}
			continue;
		}

		while(true) {
			//read header
			if(recv_data(client_sock, (uint8_t *)&header, sizeof(header)) != 0) {
				close(client_sock);
				client_sock = -1;
				printf("%s:%d\n", __func__, __LINE__);
				break;
			}

			if(recv_data(client_sock, buffer, header.size) != 0) {
				close(client_sock);
				client_sock = -1;
				printf("%s:%d\n", __func__, __LINE__);
				break;
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
					exit(-1);
				}
				printf("vdec reset\n");
				video_flush_flag  = 0;
			}
			if(h264_decode(vdec_handle, buffer, header.size, header.pts)) {
				h264_decoder_destroy(vdec_handle);

#ifdef H264_DECODE_INIT_1080P
				vdec_handle = h264_decode_init(1920, 1080,  NULL, 0);
#else
				vdec_handle = h264_decode_init(1280, 720,  NULL, 0);
#endif
				audio_flush_flag = 1;
			}

		}
	}
	return NULL;
}
#else

static int udp_recv_v1(int sockfd, uint8_t *data, int len)
{
	uint8_t *ptr = data;
	int32_t ret = 0;
	int payload_len = UDP_MAX_PAYLOAD_LEN;
	int32_t r = 0;
	int segment;
	uint8_t packet[1500];
	udp_header *h;
	uint8_t *payload = &packet[0] + sizeof(udp_header); 
	int data_len;
	uint32_t seg_nums = 0;

	h = (udp_header *)&packet[0];

	while(true){
		data_len = recvfrom(sockfd, packet, UDP_SEG_LENGTH, 0, NULL, NULL);
		if(data_len < 0){
			usleep(1*1000);
			continue;
		}
		/*printf("%ld:%lu\n", h->seg_nums, h->seg_id);*/
		if(h->magic == 0xdeadbeef && h->seg_id < h->seg_nums){
			if(h->seg_id == 0){
				if(seg_nums != 0){
					printf("abandon data\n");
				}
				seg_nums = 0;
			}
			memcpy(data + h->seg_id * UDP_MAX_PAYLOAD_LEN, payload, UDP_MAX_PAYLOAD_LEN);
			/*printf("%02x %02x %02x %02x\n", payload[0], payload[1], payload[2],payload[3]);*/
			seg_nums++;
			if(seg_nums == h->seg_nums && (h->seg_id + 1) == h->seg_nums){
				break;
			}
		}
	}

	return 0;
}

static int udp_recv_v2(int sockfd, uint8_t *data, int len)
{
	uint8_t *ptr = data;
	int32_t ret = 0;
	int payload_len = UDP_MAX_PAYLOAD_LEN;
	int32_t r = 0;
	int segment;
	static uint8_t packet[1500];
	udp_header *h;
	uint8_t *payload = &packet[0] + sizeof(udp_header); 
	int data_len;
	uint32_t seg_nums = 0;

	h = (udp_header *)&packet[0];

	while(true){
		data_len = recvfrom(sockfd, h, sizeof(udp_header), 0, NULL, NULL);
		if(data_len < 0){
			usleep(1*1000);
			continue;
		}

		if(data_len != sizeof(udp_header)){
			return -1;
		}

		if(h->magic != 0xdeadbeef){
			return -1;
		}

		data_len = recvfrom(sockfd, data + h->seg_id * UDP_MAX_PAYLOAD_LEN, UDP_MAX_PAYLOAD_LEN, 0, 
				NULL, NULL);

		if(data_len != UDP_MAX_PAYLOAD_LEN){
			return -1;
		}

		printf("%ld:%lu\n", h->seg_nums, h->seg_id);
		if(h->magic == 0xdeadbeef && h->seg_id < h->seg_nums){
			if(h->seg_id == 0){
				seg_nums = 0;
			}
			seg_nums++;
			if(seg_nums == h->seg_nums && (h->seg_id + 1) == h->seg_nums){
				break;
			}
		}
	}

	return 0;
}

#define udp_recv udp_recv_v1

void *audio_data_thread(void *argv)
{
	printf("%s\n", __func__);
	uint8_t *buffer = NULL;
	int client_sock = audio_data_sock;
	data_header_t *header;
	void *adec_handle = NULL;
	int ret = 0;
	if(audio_data_sock <=0) {
		printf("audio_data_sock = %d\n", audio_data_sock);
		exit(-1);
	}

	buffer = malloc(DATA_BUFFER_SIZE);
	if(!buffer) {
		printf("%s:%d, Not enough memory\n", __func__, __LINE__);
		exit(-1);
	}
	adec_handle = aac_decoder_init(16, 2,  sample_rate);
	if(!adec_handle) {
		printf("Init adec error.\n");
		exit(-1);
	}

	while(true) {
		printf("wait audio connect.\n");

		if((client_sock < 0) && !(client_sock = create_socket(AUDIO_DATA_PORT))) {
			printf("Create socket error\n");
			break;
		}

		while(true) {
			if(udp_recv(client_sock, buffer, DATA_BUFFER_SIZE) != 0){
				close(client_sock);
				client_sock = -1;
				printf("%s:%d\n", __func__, __LINE__);
				break;
			}
			header = (data_header_t *)buffer;

			if(audio_flush_flag) {
				aac_decoder_flush(adec_handle);
				audio_flush_flag = 0;
			}

			if(strcmp(header->magic, DATA_HEADER_MAGIC)){
				printf("recv data header\n");
				close(client_sock);
				client_sock = -1;
				printf("%s:%d\n", __func__, __LINE__);
				break;
			}

			if(header->sample_rate != sample_rate){
				aac_decoder_destroy(adec_handle);
				adec_handle = aac_decoder_init(16, 2,  header->sample_rate);
				if(!adec_handle) {
					printf("Init adec error.\n");
					exit(-1);
				}
				sample_rate = header->sample_rate;
				video_flush_flag = 1;
				printf("audio reset\n");
			}
			if(aac_decode(adec_handle, buffer + sizeof(data_header_t), header->size - sizeof(data_header_t), header->pts))
				aac_decoder_flush(adec_handle);
		}
	}
	return NULL;
}

void *video_data_thread(void *argv)
{
	printf("%s\n", __func__);
	uint8_t *buffer = NULL;
	int client_sock = video_data_sock;
	data_header_t *header;
	void *vdec_handle = NULL;
	int ret = 0;
	if(video_data_sock <=0) {
		printf("video_data_sock = %d\n", video_data_sock);
		exit(-1);
	}

	buffer = malloc(DATA_BUFFER_SIZE);
	if(!buffer) {
		printf("%s:%d, Not enough memory\n", __func__, __LINE__);
		exit(-1);
	}
	vdec_handle = h264_decode_init(1920, 1080,  NULL, 0);
	if(!vdec_handle) {
		printf("Init vdec error.\n");
		exit(-1);
	}

	while(true) {
		printf("wait video connect.\n");

		if((client_sock < 0) && !(client_sock = create_socket(VIDEO_DATA_PORT))) {
			printf("Create socket error\n");
			break;
		}

		while(true) {
			//read header
			if(udp_recv(client_sock, buffer, DATA_BUFFER_SIZE) != 0) {
				close(client_sock);
				client_sock = -1;
				printf("%s:%d\n", __func__, __LINE__);
				break;
			}
			header = (data_header_t *)buffer;

			if(strcmp(header->magic, DATA_HEADER_MAGIC)){
				printf("recv data header\n");
				close(client_sock);
				client_sock = -1;
				printf("%s:%d\n", __func__, __LINE__);
				break;
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
					exit(-1);
				}
				printf("vdec reset\n");
				video_flush_flag  = 0;
			}
			if(h264_decode(vdec_handle, buffer + sizeof(data_header_t), header->size - sizeof(data_header_t), header->pts)) {
				h264_decoder_destroy(vdec_handle);
#ifdef H264_DECODE_INIT_1080P
				vdec_handle = h264_decode_init(1920, 1080,  NULL, 0);
#else
				vdec_handle = h264_decode_init(1280, 720,  NULL, 0);
#endif
				audio_flush_flag = 1;
			}


		}
	}
	return NULL;
}
#endif

/*
 * 四个网络接收，两个音频端口，两个视频端口
 * 两个音频线程监听端口为7001,7002,
 * 7003位控制端口端口，7002位音频数据传输端口
 * 两个视频监听端口为7003,7004，
 * 7003位控制端口,7004为视频传输端口
 * 网络传输全部使用tcp协议
 * */
#ifdef __HCRTOS__
int display_sink_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	pthread_t tid;

	/*signal(SIGPIPE, SIG_IGN);*/
	printf("sizeof(data_header_t): %lu\n", sizeof(data_header_t));

	video_data_sock = create_socket(VIDEO_DATA_PORT);
	if(video_data_sock <= 0) {
		printf("Create video_data_sock error\n");
		return -1;
	}
	audio_data_sock = create_socket(AUDIO_DATA_PORT);
	if(audio_data_sock <= 0) {
		printf("Create audio_data_sock error\n");
		return -1;
	}
	printf("video_data_sock: %d\n", video_data_sock);
	printf("audio_data_sock: %d\n", audio_data_sock);
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
static void display_thread(void *args)
{
	usleep(1000*1000*3);
	console_run_cmd("net ifconfig eth0 192.168.61.1 ");
	console_run_cmd("net ifconfig eth0 netmask 255.255.255.0 gateway 192.168.61.1");
	console_run_cmd("net udhcpd start eth0 192.168.61.2 10");
	console_run_cmd("display_sink");
	vTaskDelete(NULL);
}
static int display_sink(void)
{
	xTaskCreate(display_thread, (const char *)"display_source", configTASK_STACK_DEPTH,                    NULL, portPRI_TASK_NORMAL, NULL);
	return 0;        
}

/*__initcall(display_sink)*/

CONSOLE_CMD(display_sink, NULL, display_sink_main, CONSOLE_CMD_MODE_SELF, "display_sink")
#endif
