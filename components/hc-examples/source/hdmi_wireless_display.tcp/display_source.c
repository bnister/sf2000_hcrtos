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
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/hdmi_rx.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>
#include <hcuapi/fb.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/hdmi_rx.h>
#include "queue.h"
#include <hcuapi/snd.h>
#include <semaphore.h>
#include <hcuapi/jpeg_enc.h>

#ifdef __HCRTOS__
#include <kernel/module.h>
#include <kernel/lib/console.h>
#endif

#include "hdmi_rx_quality.h"

#include "jenc_rdo.h"
#include "tables.h"

#define NETWORK_MODE_ETHERNET

#ifdef NETWORK_MODE_ETHERNET
#define HDMI_WIRELESS_MAX_NET_SPEED (10*1024*1024)
#else
#define HDMI_WIRELESS_MAX_NET_SPEED (4*1024*1024)
#endif

#define LENGTH_OF_LISTEN_QUEUE 20

#define DATA_BUFFER_SIZE (1024*1024*4)
#define CTRL_BUFFER_SIZE (64*1024)

static int audio_data_sock;
static int video_data_sock;

static pthread_t rx_audio_read_thread_id = 0;
static pthread_t rx_video_read_thread_id = 0;
static pthread_t video_data_thread_id = 0;
static pthread_t audio_data_thread_id = 0;
static int rx_fd = -1;
static bool stop_read = 0;

#ifndef __HCRTOS__
static int akshm_fd = -1;
static int vkshm_fd = -1;
#endif
static unsigned int rotate_mode = 0;

static bool video_send_running =false;
static bool audio_send_running =false;

struct data_element{
	data_header_t header;
	uint8_t *data;
};

static Queue *video_queue;
static Queue *audio_queue;

static int sndinfo_fd;

#ifdef __HCRTOS__
struct kshm_info rx_audio_read_hdl = {0};
struct kshm_info rx_video_read_hdl = {0};
static int kshm_read_cb(kshm_handle_t hdl, void *request, void *buf, size_t size, void *args);
typedef int (*kshm_cb)(kshm_handle_t hdl, void *request, void *buf, size_t size, void *args);
static int kshm_read_ext(kshm_handle_t hdl, void *buf, size_t size, kshm_cb cb, void *args);
#endif

static void queue_element_free(QueueElement  p);

/*#define AIRPLAY_FRAME_RATE*/
#ifdef AIRPLAY_FRAME_RATE
struct frame_rate {
	char *name;
	double frame_cnts;
	int max_frame_cnts;
	double duration;
	double start_time;
	double size;
};

static struct frame_rate video_frame_rate = {
	.name = "video",
	.max_frame_cnts = 30*2,
	.duration = 1.0,
	.size = 0,
};
static struct frame_rate audio_frame_rate = {
	.name = "audio",
	.max_frame_cnts = 44100*2,
	.duration = 1.0,
	.size = 0,
};


void print_frame_rate(struct frame_rate *rate, double count, int size)
{
	struct timeval tv;
	double duration;
	if(rate->start_time == 0) {
		gettimeofday(&tv, NULL);
		rate->start_time = tv.tv_sec + tv.tv_usec/1000000.0;
		rate->frame_cnts = count;
		return;
	}
	rate->frame_cnts += count;
	rate->size += size;
	double cur_time;
	gettimeofday(&tv, NULL);
	cur_time = tv.tv_sec + tv.tv_usec/1000000.0;
	duration =  cur_time - rate->start_time;
	if(rate->frame_cnts > rate->max_frame_cnts || duration > rate->duration) {
		double r;
		r = rate->frame_cnts / duration;
		printf("---%-06s frame rate: %-10.2f, frame_cnts: %-10.0f, duration: %-2.2f, rate: %-4.2f -----userspace\n",
		       rate->name,r, rate->frame_cnts, duration, rate->size/(1024*1024));
		rate->frame_cnts = 0;
		rate->start_time = cur_time;
		rate->size = 0;
	}
}
#endif

static uint32_t get_sample_rate(void)
{
	if(sndinfo_fd <= 0){
		sndinfo_fd = open("/dev/sndC0i2si", O_RDONLY);
		if(sndinfo_fd < 0){
			perror("open /dev/sndC0i2si error\n");
			return 0;
		}
	}

	struct snd_hw_info info;
	if(ioctl(sndinfo_fd, SND_IOCTL_GET_HW_INFO, &info) < 0){
		perror("get snd info error.\n");
		return 0;
	}
	/*printf("info.pcm_params.rate: %d\n", info.pcm_params.rate);*/
	return info.pcm_params.rate;
}

static int create_socket(int server_port)
{

	int fd = -1;
	struct sockaddr_in my_addr;
	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htons(INADDR_ANY);
#ifndef USE_UDP_PROTOCOL
	my_addr.sin_port = htons(0);
	fd = socket(AF_INET, SOCK_STREAM, 0);
#else
	my_addr.sin_port = htons(server_port);
	fd = socket(AF_INET, SOCK_DGRAM, 0);
#endif

	if(fd < 0) {
		printf("socket create error. error: %s\n", strerror(errno));
		return -1;
	}

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifndef USE_UDP_PROTOCOL
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

#ifndef __HCRTOS__
static int read_data(int fd, uint8_t *buf, int size)
{
	uint8_t *p = buf;
	while(read(fd, p, size) != size && !stop_read) {
		usleep(3*1000);
	}
	return 0;
}
#else

static int kshm_read_ext(kshm_handle_t hdl, void *buf, size_t size, kshm_cb cb, void *args)
{
	struct kshm_info *info = (struct kshm_info *)hdl;
	void *request = NULL;

	if (!info || !size) {
		return KSHM_FAIL;
	}

	request = kshm_request_read(hdl, size);
	if (!request) {
		return KSHM_FAIL;
	}
	if(!cb)
		memcpy(buf, request, size);
	else
		cb(hdl, request, buf, size, args);

	kshm_update_read(hdl, size);

	return size;
}
static int read_data(struct kshm_info *hdl, uint8_t *buf, int size, kshm_cb cb, void *args)
{
	uint8_t *p = buf;
	if(!cb)
		while(kshm_read(hdl, p, size) != size && !stop_read) {
			usleep(3*1000);
		}
	else
		while(kshm_read_ext(hdl, p, size, cb, args) != size && !stop_read) {
			usleep(3*1000);
		}

	return 0;
}
#endif




#define FRAME_SIZE_MAX (11*1024*1024/30)
static struct jenc_rdo_t g_rdo;
static int threshhold = 0;
static int frame_size = FRAME_SIZE_MAX;
static int update_enc_table(struct jenc_rdo_t *rdo)
{
	struct jpeg_enc_quant enc_table;

	printf("(y, c) = (%d, %d)\n", jenc_rdo_y_q_index(rdo), jenc_rdo_c_q_index(rdo));
	memcpy(&enc_table.dec_quant_y , jpeg_quant_tables[jenc_rdo_y_q_index(rdo)].dec_table,64);
	memcpy(&enc_table.enc_quant_y , jpeg_quant_tables[jenc_rdo_y_q_index(rdo)].enc_table, 128);

	memcpy(&enc_table.dec_quant_c , jpeg_quant_tables[jenc_rdo_c_q_index(rdo)].dec_table, 64);
	memcpy(&enc_table.enc_quant_c , jpeg_quant_tables[jenc_rdo_c_q_index(rdo)].enc_table, 128);

	if(ioctl(rx_fd , HDMI_RX_SET_ENC_QUANT , &enc_table) != 0)
		printf("%s:%d, set enc table error.\n", __func__, __LINE__);
	return 0;
}

void jdo_change_quality(int32_t size)
{
	static int32_t prev = 0;
	threshhold++;
	prev = size;
	if (threshhold >= 3 && (size < frame_size) && (frame_size - size) > frame_size * 0.2){
		/*printf("size:%ld,FRAME_SIZE_MAX:%d,diff:%d\n",size, frame_size, abs(frame_size - size));*/
		jenc_rdo_rate_control(&g_rdo, JENC_RDO_OP_INCREASE_BITRATE);
		update_enc_table(&g_rdo);
		threshhold = 0;
	}else if (threshhold >= 3 && (size > frame_size) && (size - frame_size) > 0.3 * frame_size){
		/*printf("size:%ld,FRAME_SIZE_MAX:%d,diff:%d\n",size, frame_size, abs(frame_size - size));*/
		jenc_rdo_rate_control(&g_rdo, JENC_RDO_OP_REDUCE_BITRATE);
		update_enc_table(&g_rdo);
		threshhold = 0;
	}
}

#ifndef USE_UDP_PROTOCOL
static void *audio_data_thread(void *args)
{
	AvPktHd hdr = {0};
	struct data_element *e;
#ifndef USE_UDP_PROTOCOL
	int header_size = 0;
#else
	int header_size = sizeof(data_header_t);
#endif
	printf("%s:%d\n", __func__, __LINE__);
	while(true) {
		
		e = malloc(sizeof(*e));
		/*printf("%s:%d\n", __func__, __LINE__);*/
		if(!e){
			printf("%s:%d,Not enough memory\n",__func__, __LINE__);
			usleep(100 * 1000);
			continue;
		}

again1:
		memset(&hdr, 0, sizeof(AvPktHd));
#ifndef __HCRTOS__
		if (read_data(akshm_fd, (uint8_t *)&hdr, sizeof(AvPktHd)) != 0) {
#else
		if (read_data(&rx_audio_read_hdl, (uint8_t *)&hdr, sizeof(AvPktHd), NULL, NULL) != 0) {
#endif
			printf("read audio hdr from kshm err\n");
			break;
		}

		if(hdr.size <=0)
			goto again1;
		e->data = malloc(hdr.size + header_size);
		if(!e->data){
			printf("%s:%d,Not enough memory\n",__func__, __LINE__);
			goto again1;
		}

#ifndef __HCRTOS__
		if(read_data(akshm_fd, e->data + header_size, hdr.size) != 0) {
#else
		if(read_data(&rx_audio_read_hdl, e->data + header_size, hdr.size, NULL, NULL) != 0) {
#endif
			printf("read audio data from kshm err\n");
			usleep(30 * 1000);
			free(e->data);
			goto again1;
		}

		e->header.size = hdr.size + header_size;
		e->header.pts = hdr.pts;
		e->header.sample_rate = get_sample_rate();

		strncpy(e->header.magic, DATA_HEADER_MAGIC, sizeof(e->header.magic));
		/*printf("%s, e->header.size: %d\n", __func__, e->header.size);*/

		if(header_size > 0){
			memcpy(e->data, &e->header, header_size);
		}

		if(!audio_send_running){
			free(e->data);
			goto again1;
		}

#ifdef AIRPLAY_FRAME_RATE
		/*print_frame_rate(&audio_frame_rate, 1, e->header.size);*/
#endif

		if(enqueue(audio_queue, e)){
			free(e->data);
			e->data = NULL;
			goto again1;
		}
	}
	return NULL;
}

static void *video_data_thread(void *args)
{
	AvPktHd hdr = {0};
	struct data_element *e;
	int quality = 1;
#ifndef USE_UDP_PROTOCOL
	int header_size = 0;
#else
	int header_size = sizeof(data_header_t);
#endif


	printf("%s:%d\n", __func__, __LINE__);

	while(true) {

		e = malloc(sizeof(*e));
		if(!e){
			printf("%s:%d,Not enough memory\n",__func__, __LINE__);
			usleep(100 * 1000);
			continue;
		}

again1:
		
#ifndef __HCRTOS__
		if (read_data(vkshm_fd, (uint8_t *)&hdr, sizeof(AvPktHd)) != 0) {
#else
		if (read_data(&rx_video_read_hdl, (uint8_t *)&hdr, sizeof(AvPktHd), NULL, NULL) != 0) {
#endif
			printf("read video hdr from kshm err\n");
			break;
		}

		/*printf("video:vpkt size %d\n", (int)hdr.size);*/
		/*if (vrecfile) {*/
			//fwrite (&hdr, sizeof(AvPktHd), 1, vrecfile);
		/*}*/

		if(hdr.size <=0){
			goto again1;
		}
		e->data = malloc(hdr.size + header_size);
		if(!e->data){
			printf("%s:%d,Not enough memory\n",__func__, __LINE__);
			goto again1;
		}

#ifndef __HCRTOS__
		if(read_data(vkshm_fd, e->data + header_size, hdr.size) != 0) {
#else
		if(read_data(&rx_video_read_hdl, e->data + header_size, hdr.size, NULL, NULL) != 0) {
#endif
			printf("read audio data from kshm err\n");
			usleep(30 * 1000);
			free(e->data);
			goto again1;
		}

		e->header.size = hdr.size;
		e->header.pts = hdr.pts;

		strncpy(e->header.magic, DATA_HEADER_MAGIC, sizeof(e->header.magic));
		/*printf("%s, e->header.size: %d\n", __func__, e->header.size);*/

		if(header_size > 0){
			memcpy(e->data, &e->header, header_size);
		}

		if(!video_send_running){
			free(e->data);
			goto again1;
		}

#ifdef AIRPLAY_FRAME_RATE
		/*print_frame_rate(&video_frame_rate, 1, e->header.size);*/
#endif

#if 1
        /*quality = change_quality(HDMI_WIRELESS_MAX_NET_SPEED, 30, e->header.size);*/
		jdo_change_quality(e->header.size);
#else

#if 1
        quality = JPEG_ENC_QUALITY_TYPE_ULTRA_HIGH_QUALITY;
        if(video_queue->size == 1){
            quality = JPEG_ENC_QUALITY_TYPE_HIGH_QUALITY;
        }else if(video_queue->size >= 2){
            quality = JPEG_ENC_QUALITY_TYPE_NORMAL;
        }
#else
        quality = JPEG_ENC_QUALITY_TYPE_HIGH_QUALITY;
        if(video_queue->size == 1){
            quality = JPEG_ENC_QUALITY_TYPE_NORMAL;
        }else if(video_queue->size >= 2){
            quality = JPEG_ENC_QUALITY_TYPE_LOW_BITRATE;
        }
#endif

#endif

		if(enqueue(video_queue, e)){
			free(e->data);
			e->data = NULL;
			goto again1;
		}
	}
	return NULL;
}

static void queue_element_free(QueueElement  p)
{
	struct data_element *e = (struct data_element *)p;
	if(e){
		if(e->data)
			free(e->data);
		free(e);
	}
}
static void *rx_audio_read_thread(void *args)
{
	char *server_ip = (char *)args;
	int data_len = 0;
	struct sockaddr_in server_addr;
	if(audio_data_sock <=0) {
		printf("audio_data_sock = %d\n", audio_data_sock);
		return NULL;
	}
	
	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	if(inet_aton(server_ip, &server_addr.sin_addr) == 0) {
		printf("Server IP addr error.\n");
		goto fail;
	}

	server_addr.sin_port = htons(AUDIO_DATA_PORT);


	struct data_element *e = NULL;
	//printf("rx_audio_read_thread run\n");
	while (!stop_read) {

		printf("audio connect server %s......\n",server_ip);
		if(connect(audio_data_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
			printf("Connect %s error. error: %s\n", server_ip, strerror(errno));
			sleep(1);
			continue;
		}
		printf("connected.\n");
		queue_clear(audio_queue, queue_element_free);
		audio_send_running = true;
		while(true) {
			e = front(audio_queue);
			if(!e){
				continue;
			}

			/*printf("%s, e->header.size: %d\n", __func__, e->header.size);*/
			data_len = send(audio_data_sock, &e->header, sizeof(e->header), 0);

			if(data_len != sizeof(e->header)) {
				close(audio_data_sock);
				audio_data_sock = create_socket(AUDIO_DATA_PORT);
				if(audio_data_sock <= 0) {
					goto fail;
				}
				printf("%s,%d,send error, error: %s\n",__func__, __LINE__, strerror(errno));
				free(e->data);
				free(e);
				break;
			}

#ifndef AIRPLAY_FRAME_RATE
			data_len = send(audio_data_sock, e->data, e->header.size, 0);
#else
			print_frame_rate(&audio_frame_rate, 1, e->header.size);
			data_len = send(audio_data_sock, e->data, e->header.size, 0);
#endif

			if(data_len != e->header.size) {
				printf("%s,%d,send error, error: %s\n",__func__, __LINE__, strerror(errno));
				close(audio_data_sock);
				audio_data_sock = create_socket(AUDIO_DATA_PORT);
				if(audio_data_sock <= 0) {
					goto fail;
				}
				free(e->data);
				free(e);
				break;
			}

			free(e->data);
			free(e);

		}
		audio_send_running = false;
	}

	return NULL;
fail:
	return NULL;
}

static void *rx_video_read_thread(void *args)
{
	char *server_ip = (char *)args;
	int data_len = 0;
	struct sockaddr_in server_addr;
	if(video_data_sock <=0) {
		printf("video_data_sock = %d\n", video_data_sock);
		return NULL;
	}
	
	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	if(inet_aton(server_ip, &server_addr.sin_addr) == 0) {
		printf("Server IP addr error.\n");
		goto fail;
	}

	server_addr.sin_port = htons(VIDEO_DATA_PORT);


	struct data_element *e = NULL;
	//printf("rx_video_read_thread run\n");
	while (!stop_read) {

		printf("video connect servier %s......\n",server_ip);
		if(connect(video_data_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
			printf("Connect %s error. error: %s\n", server_ip, strerror(errno));
			sleep(1);
			continue;
		}
		printf("connected.\n");
		queue_clear(video_queue, queue_element_free);
		video_send_running = true;
		while(true) {
			e = front(video_queue);
			if(!e){
				continue;
			}

			data_len = send(video_data_sock, &e->header, sizeof(e->header), 0);

			if(data_len != sizeof(e->header)) {
				close(video_data_sock);
				video_data_sock = create_socket(VIDEO_DATA_PORT);
				if(video_data_sock <= 0) {
					goto fail;
				}
				printf("%s,%d,send error, error: %s\n",__func__, __LINE__, strerror(errno));
				free(e->data);
				free(e);
				break;
			}

#ifndef AIRPLAY_FRAME_RATE
			data_len = send(video_data_sock, e->data, e->header.size, 0);
#else
			print_frame_rate(&video_frame_rate, 1, e->header.size);
			data_len = send(video_data_sock, e->data, e->header.size, 0);
#endif

			if(data_len != e->header.size) {
				printf("%s,%d,data_len: %d, send error, error: %s\n",__func__, __LINE__, data_len, strerror(errno));
				close(video_data_sock);
				video_data_sock = create_socket(VIDEO_DATA_PORT);
				if(video_data_sock <= 0) {
					goto fail;
				}
				free(e->data);
				free(e);
				break;
			}
			free(e->data);
			free(e);
		}
		video_send_running = false;
	}

	return NULL;
fail:
	return NULL;
}
#else

static int udp_send(int sockfd, uint8_t *data, int32_t len, uint32_t flags, struct sockaddr *addr, int32_t addr_len)
{
	uint8_t *ptr = data;
	int32_t ret = 0;
	int payload_len = UDP_MAX_PAYLOAD_LEN;
	int32_t r = 0;
	int segment;
	uint8_t packet[1500];
	udp_header *h;
	uint8_t *payload = &packet[0] + sizeof(udp_header); 

	h = (udp_header *)&packet[0];
	h->magic = 0xdeadbeef;
	h->seg_nums = len / UDP_MAX_PAYLOAD_LEN;
	h->seg_id = 0;
	h->seg_len = UDP_SEG_LENGTH;
	if(len % UDP_MAX_PAYLOAD_LEN != 0)
		h->seg_nums++;

	while(len > 0){
		payload_len = (len < (int32_t)UDP_MAX_PAYLOAD_LEN)? len: (int32_t)UDP_MAX_PAYLOAD_LEN;
		memcpy(payload, ptr, payload_len);
		ret = sendto(sockfd, packet, UDP_SEG_LENGTH, flags, addr, addr_len);
		if(ret != UDP_SEG_LENGTH){
			if(errno == EAGAIN){
				usleep(3*1000);
				continue;
			}
			printf("send error.\n");
			r = -1;
			break;
		}
		len -= payload_len;
		ptr += payload_len;
		h->seg_id++;
	}
	return r;
}

static int udp_send_v1(int sockfd, uint8_t *data, int32_t len, uint32_t flags, struct sockaddr *addr, int32_t addr_len)
{
	uint8_t *ptr = data;
	int32_t ret = 0;
	udp_header *h;
	int32_t seg_nums;

	h = (udp_header *)data;

	if(h->magic != 0xdeadbeef){
		return -1;
	}

	seg_nums = h->seg_nums;

	while(seg_nums > 0){
		ret = sendto(sockfd, ptr, UDP_SEG_LENGTH, flags, addr, addr_len);
		if(ret != UDP_SEG_LENGTH){
			if(errno == EAGAIN){
				usleep(3*1000);
				continue;
			}
			printf("send error.\n");
			return -1;
		}
		ptr += UDP_SEG_LENGTH;
		seg_nums--;
	}

	return 0;
}

static int kshm_read_cb(kshm_handle_t hdl, void *request, void *buf, size_t size, void *args)
{
	uint8_t *src = request;
	uint8_t *dst = buf;
	int32_t ret = 0;
	int payload_len = UDP_MAX_PAYLOAD_LEN;
	int32_t r = 0;
	int segment;
	udp_header h;
	uint8_t *payload;
	data_header_t *header = (data_header_t *)args;

	h.magic = 0xdeadbeef;
	h.seg_nums = size / UDP_MAX_PAYLOAD_LEN;
	h.seg_id = 0;
	h.seg_len = UDP_SEG_LENGTH;
	if(size % UDP_MAX_PAYLOAD_LEN != 0)
		h.seg_nums++;
	

	while(size > 0){
		payload_len = (size < (int32_t)UDP_MAX_PAYLOAD_LEN)? size: (int32_t)UDP_MAX_PAYLOAD_LEN;
		memcpy(dst, &h, sizeof(udp_header));
		if(h.seg_id == 0){
			payload_len -= sizeof(data_header_t);
			memcpy(dst + sizeof(udp_header), header, sizeof(data_header_t));
			memcpy(dst + sizeof(udp_header) + sizeof(data_header_t), src, payload_len);
		}else
			memcpy(dst + sizeof(udp_header), src, payload_len);

		size -= payload_len;
		src += payload_len;
		dst += UDP_SEG_LENGTH;
		h.seg_id++;
	}

	return size;
}

static void *audio_data_thread(void *args)
{
	AvPktHd hdr = {0};
	struct data_element *e;
#ifndef USE_UDP_PROTOCOL
	int header_size = 0;
#else
	int header_size = sizeof(data_header_t);
#endif
	printf("%s:%d\n", __func__, __LINE__);
	while(true) {
		
		e = malloc(sizeof(*e));
		if(!e){
			printf("%s:%d,Not enough memory\n",__func__, __LINE__);
			usleep(100 * 1000);
			continue;
		}

again1:
		memset(&hdr, 0, sizeof(AvPktHd));
#ifndef __HCRTOS__
		if (read_data(akshm_fd, (uint8_t *)&hdr, sizeof(AvPktHd)) != 0) {
#else
		if (read_data(&rx_audio_read_hdl, (uint8_t *)&hdr, sizeof(AvPktHd), NULL, NULL) != 0) {
#endif
			printf("read audio hdr from kshm err\n");
			break;
		}

		if(hdr.size <=0)
			goto again1;
		e->data = malloc(((hdr.size + header_size)/UDP_MAX_PAYLOAD_LEN + 1)*UDP_SEG_LENGTH);
		if(!e->data){
			printf("%s:%d,Not enough memory\n",__func__, __LINE__);
			goto again1;
		}

		e->header.size = hdr.size + header_size;
		e->header.pts = hdr.pts;
		e->header.sample_rate = get_sample_rate();

		strncpy(e->header.magic, DATA_HEADER_MAGIC, sizeof(e->header.magic));

#ifndef __HCRTOS__
		if(read_data(akshm_fd, e->data + header_size, hdr.size) != 0) {
#else
		if(read_data(&rx_audio_read_hdl, e->data/* + header_size*/, hdr.size, kshm_read_cb, &e->header) != 0) {
#endif
			printf("read audio data from kshm err\n");
			usleep(30 * 1000);
			free(e->data);
			goto again1;
		}

		if(!audio_send_running){
			free(e->data);
			goto again1;
		}

#ifdef AIRPLAY_FRAME_RATE
		print_frame_rate(&audio_frame_rate, 1, e->header.size);
#endif

		if(enqueue(audio_queue, e)){
			free(e->data);
			e->data = NULL;
			goto again1;
		}
	}
	return NULL;
}

static void *video_data_thread(void *args)
{
	AvPktHd hdr = {0};
	struct data_element *e;
	int quality = 1;
#ifndef USE_UDP_PROTOCOL
	int header_size = 0;
#else
	int header_size = sizeof(data_header_t);
#endif


	printf("%s:%d\n", __func__, __LINE__);
#if 0
	struct timeval before, cur;
	time_t diff;
	gettimeofday(&before, NULL);
	gettimeofday(&cur, NULL);
#endif

	while(true) {

		e = malloc(sizeof(*e));
		if(!e){
			printf("%s:%d,Not enough memory\n",__func__, __LINE__);
			usleep(100 * 1000);
			continue;
		}

again1:
		
#ifndef __HCRTOS__
		if (read_data(vkshm_fd, (uint8_t *)&hdr, sizeof(AvPktHd)) != 0) {
#else
		if (read_data(&rx_video_read_hdl, (uint8_t *)&hdr, sizeof(AvPktHd), NULL, NULL) != 0) {
#endif
			printf("read video hdr from kshm err\n");
			break;
		}

		if(hdr.size <=0){
			goto again1;
		}
		e->data = malloc(((hdr.size + header_size)/UDP_MAX_PAYLOAD_LEN + 1)*UDP_SEG_LENGTH);
		if(!e->data){
			printf("%s:%d,Not enough memory\n",__func__, __LINE__);
			goto again1;
		}

		e->header.size = hdr.size + header_size;
		e->header.pts = hdr.pts;
		strncpy(e->header.magic, DATA_HEADER_MAGIC, sizeof(e->header.magic));

#ifndef __HCRTOS__
		if(read_data(vkshm_fd, e->data + header_size, hdr.size) != 0) {
#else
		if(read_data(&rx_video_read_hdl, e->data/* + header_size*/, hdr.size, kshm_read_cb, &e->header) != 0) {
#endif
			printf("read audio data from kshm err\n");
			usleep(30 * 1000);
			free(e->data);
			goto again1;
		}

		if(!video_send_running){
			free(e->data);
			goto again1;
		}

#ifdef AIRPLAY_FRAME_RATE
		print_frame_rate(&video_frame_rate, 1, e->header.size);
#endif

		jdo_change_quality(e->header.size);

		if(enqueue(video_queue, e)){
			free(e->data);
			e->data = NULL;
			printf("video queue full\n");
#if 0
			gettimeofday(&cur, NULL);
			diff = ((cur.tv_sec*1000000 + cur.tv_usec) - (before.tv_sec * 1000000 + before.tv_usec))/1000;
			printf("f2:%lld\n", diff);
			before = cur;
#endif
			goto again1;
		}
#if 0
		gettimeofday(&cur, NULL);
		diff = ((cur.tv_sec*1000000 + cur.tv_usec) - (before.tv_sec * 1000000 + before.tv_usec))/1000;
		/*printf("2:%lld\n", diff);*/
		before = cur;
#endif
	}
	return NULL;
}

static void queue_element_free(QueueElement  p)
{
	struct data_element *e = (struct data_element *)p;
	if(e){
		if(e->data)
			free(e->data);
		free(e);
	}
}

static void *rx_audio_read_thread(void *args)
{
	char *server_ip = (char *)args;
	int data_len = 0;
	struct sockaddr_in server_addr;
	if(audio_data_sock <=0) {
		printf("audio_data_sock = %d\n", audio_data_sock);
		exit(-1);
	}
	
	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	if(inet_aton(server_ip, &server_addr.sin_addr) == 0) {
		printf("Server IP addr error.\n");
		goto fail;
	}

	server_addr.sin_port = htons(AUDIO_DATA_PORT);


	struct data_element *e = NULL;
	while (!stop_read) {

		printf("audio connect server %s......\n",server_ip);
		queue_clear(audio_queue, queue_element_free);
		audio_send_running = true;
		while(true) {
			e = front(audio_queue);
			if(!e){
				printf("%s:%d:bugs\n", __func__, __LINE__);
				continue;
			}

			data_len = udp_send_v1(audio_data_sock, e->data, e->header.size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
#ifdef AIRPLAY_FRAME_RATE
			print_frame_rate(&audio_frame_rate, 1, e->header.size);
#endif
			if(data_len != 0) {
				close(audio_data_sock);
				audio_data_sock = create_socket(AUDIO_DATA_PORT);
				if(audio_data_sock <= 0) {
					goto fail;
				}
				printf("%s,%d,send error, error: %s\n",__func__, __LINE__, strerror(errno));
				free(e->data);
				free(e);
				break;
			}
			free(e->data);
			free(e);
		}
		audio_send_running = false;
	}

	return NULL;
fail:
	return NULL;
}

static void *rx_video_read_thread(void *args)
{
	char *server_ip = (char *)args;
	int data_len = 0;
	struct sockaddr_in server_addr;
	if(video_data_sock <=0) {
		printf("video_data_sock = %d\n", video_data_sock);
		exit(-1);
	}
	
	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	if(inet_aton(server_ip, &server_addr.sin_addr) == 0) {
		printf("Server IP addr error.\n");
		goto fail;
	}

	server_addr.sin_port = htons(VIDEO_DATA_PORT);


	struct data_element *e = NULL;

	while (!stop_read) {

		printf("video connect servier %s......\n",server_ip);
		queue_clear(video_queue, queue_element_free);
		video_send_running = true;
		while(true) {
			e = front(video_queue);
			if(!e){
				printf("%s:%d:bugs\n", __func__, __LINE__);
				continue;
			}

			data_len = udp_send_v1(video_data_sock, e->data, e->header.size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
#ifdef AIRPLAY_FRAME_RATE
			print_frame_rate(&audio_frame_rate, 1, e->header.size);
#endif
			if(data_len != 0) {
				close(video_data_sock);
				video_data_sock = create_socket(VIDEO_DATA_PORT);
				if(video_data_sock <= 0) {
					goto fail;
				}
				printf("%s,%d,send error, size:%ld, error: %s\n",__func__, __LINE__, e->header.size, strerror(errno));
				free(e->data);
				free(e);
				break;
			}

			free(e->data);
			free(e);

		}
		video_send_running = false;
	}

	return NULL;
fail:
	return NULL;
}
#endif

static void usage(void)
{
	printf("-v video path\n");
	printf("-a audio path\n");
	printf("-s server ip\n");
	printf("-f limit every frame size of video\n");
}

static int hdmi_rx_set_enc_table(void)
{
	int opt;
	opterr = 0;
	optind = 0;

	struct jpeg_enc_quant enc_table;

	if(rx_fd >= 0)
	{
		memcpy(&enc_table.dec_quant_y , jpeg_quant_tables[0].dec_table,64);
		memcpy(&enc_table.enc_quant_y , jpeg_quant_tables[0].enc_table, 128);

		memcpy(&enc_table.dec_quant_c , jpeg_quant_tables[0].dec_table, 64);
		memcpy(&enc_table.enc_quant_c , jpeg_quant_tables[0].enc_table, 128);
		ioctl(rx_fd , HDMI_RX_SET_ENC_QUANT , &enc_table);
		return 0;
	}
	else
	{
		return -1;
	}
}

static char server_ip[32];
#ifdef __HCRTOS__
int display_source_main (int argc, char *argv[])
#else
int main (int argc, char *argv[])
#endif
{
	enum HDMI_RX_VIDEO_DATA_PATH vpath = HDMI_RX_VIDEO_TO_OSD;
	enum HDMI_RX_AUDIO_DATA_PATH apath = HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX;

	signal(SIGPIPE, SIG_IGN);

	memset(server_ip, 0, sizeof(server_ip));
	pthread_attr_t attr;
	pthread_attr_t audio_attr;
	int opt;

	opterr = 0;
	optind = 0;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 0x1000);

	audio_queue = initQueue(5);
	if(!audio_queue){
		printf("init audio queue error.\n");
		return -1;
	}

	video_queue = initQueue(3);
	if(!video_queue){
		printf("init video queue error.\n");
		return -1;
	}
	printf("jenc_rd_init\n");
	jenc_rdo_init(&g_rdo, JPEG_QUANT_TABLES_SIZE, 9, 2,
			JPEG_QUANT_TABLES_SIZE, 17, 2);

#ifndef __HCRTOS__
	akshm_fd = open("/dev/kshmdev", O_RDONLY);
	vkshm_fd = open("/dev/kshmdev", O_RDONLY);
	if (akshm_fd < 0 || vkshm_fd < 0) {
		printf("%s:%d\n", __func__, __LINE__);
		goto err;
	}
#endif
	
	while ((opt = getopt(argc, argv, "a:v:r:s:hf:")) != EOF) {
		switch (opt) {
		case 'a':
			apath = atoi(optarg);
			break;
		case 'v':
			vpath = atoi(optarg);
			break;
		case 'r':
			rotate_mode = atoi(optarg);
			break;
		case 's':
			strncpy(server_ip, optarg, sizeof(server_ip));
			break;
		case 'h':
			usage();
			return 0;
		case 'f':
			frame_size = atoi(optarg);
			printf("frame_size:%d\n", frame_size);
			break;
		default:
			break;
		}
	}

	rx_fd = open("/dev/hdmi_rx", O_WRONLY);
	if (rx_fd < 0) {
		printf("%s:%d\n", __func__, __LINE__);
		goto err;
	}

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


	printf("apath %d, vpath %d\n", apath, vpath);
	ioctl(rx_fd, HDMI_RX_SET_VIDEO_DATA_PATH, vpath);
	ioctl(rx_fd, HDMI_RX_SET_AUDIO_DATA_PATH, apath);
	
	hdmi_rx_set_enc_table();
	ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY, JPEG_ENC_QUALITY_TYPE_USER_DEFINE);

	if(true){
		ioctl(rx_fd, HDMI_RX_AUDIO_KSHM_ACCESS, &rx_audio_read_hdl);
		printf("get audio hdl, kshm desc %p\n", rx_audio_read_hdl.desc);
#ifndef __HCRTOS__
		ioctl(akshm_fd, KSHM_HDL_SET, &rx_audio_read_hdl);
#endif

		stop_read = 0;
		struct sched_param params = {.sched_priority = 16};;
		pthread_attr_init(&audio_attr);
		pthread_attr_setschedparam(&audio_attr, &params);
		if (pthread_create(&audio_data_thread_id, &audio_attr,
					audio_data_thread, NULL)) {
			printf("audio kshm recv thread create failed\n");
			goto err;
		}

		if (pthread_create(&rx_audio_read_thread_id, &audio_attr,
					rx_audio_read_thread, server_ip)) {
			printf("audio kshm recv thread create failed\n");
			goto err;
		}

	}

	if (vpath == HDMI_RX_VIDEO_TO_KSHM || vpath==HDMI_RX_VIDEO_TO_DE_AND_KSHM || vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM ) {
		ioctl(rx_fd, HDMI_RX_VIDEO_KSHM_ACCESS, &rx_video_read_hdl);
		printf("get video hdl, kshm desc %p\n", rx_video_read_hdl.desc);
#ifndef __HCRTOS__
		ioctl(vkshm_fd, KSHM_HDL_SET, &rx_video_read_hdl);
#endif

		stop_read = 0;
		struct sched_param params = {.sched_priority = 15};;
		pthread_attr_init(&attr);
		pthread_attr_setschedparam(&attr, &params);

		if (pthread_create(&video_data_thread_id, &attr,
		                   video_data_thread, NULL)) {
			printf("video kshm recv thread create failed\n");
			goto err;
		}

		if (pthread_create(&rx_video_read_thread_id, &attr,
		                   rx_video_read_thread, server_ip)) {
			printf("video kshm recv thread create failed\n");
			goto err;
		}
	}

	if(vpath == HDMI_RX_VIDEO_TO_DE_ROTATE  || vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM ){
		printf("rotate_mode = 0x%x\n", rotate_mode);
		ioctl(rx_fd, HDMI_RX_SET_VIDEO_ROTATE_MODE, rotate_mode);
	}

	ioctl(rx_fd, HDMI_RX_START);
	printf("hdmi_rx start ok```\n");

#ifndef __HCRTOS__
	while (1) {
		usleep(1000*1000);
	}
#endif
	return 0;

err:
	stop_read = 1;
	if (rx_audio_read_thread_id)
		pthread_join(rx_audio_read_thread_id, NULL);
	if (rx_video_read_thread_id)
		pthread_join(rx_video_read_thread_id, NULL);
	rx_audio_read_thread_id = rx_video_read_thread_id = 0;


#ifndef __HCRTOS__
	if (akshm_fd >= 0)
		close (akshm_fd);
	akshm_fd = -1;

	if (vkshm_fd >= 0)
		close (vkshm_fd);
	vkshm_fd = -1;
#endif

	if (rx_fd >= 0)
		close (rx_fd);
	rx_fd = -1;

	return -1;
}

#ifdef __HCRTOS__
static void display_thread(void *args)
{
	usleep(1000*1000*3);
	console_run_cmd("net ifconfig eth0 hw ether de:ad:be:ef:82:18");
#if 0
	console_run_cmd("net dhclient eth0");
#else
	console_run_cmd("net ifconfig eth0 192.168.61.2");
	console_run_cmd("net ifconfig eth0 netmask 255.255.255.0 gateway 192.168.61.1");
#endif
	console_run_cmd("display_source -s 192.168.61.1 -a 0 -v 3 -q 3");
	vTaskDelete(NULL);
}
static int display_source(void)
{
	xTaskCreate(display_thread, (const char *)"display_source", configTASK_STACK_DEPTH,
		    NULL, portPRI_TASK_NORMAL, NULL);
	return 0;
}

/*__initcall(display_source)*/

CONSOLE_CMD(display_source, NULL, display_source_main, CONSOLE_CMD_MODE_SELF, "display_source")
#endif
