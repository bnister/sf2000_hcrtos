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
#include <sys/stat.h>
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
#include "raw_proto.h"

#define NETWORK_MODE_ETHERNET

#ifdef NETWORK_MODE_ETHERNET
#define HDMI_WIRELESS_MAX_NET_SPEED (10*1024*1024)
#else
#define HDMI_WIRELESS_MAX_NET_SPEED (4*1024*1024)
#endif

#define LENGTH_OF_LISTEN_QUEUE 20

#define DATA_BUFFER_SIZE (1024*1024*4)
#define CTRL_BUFFER_SIZE (64*1024)

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
static FILE *arecfile = NULL;
static FILE *vrecfile = NULL;
static unsigned int rotate_mode = 0;

static bool video_send_running =false;
static bool audio_send_running =false;

static int quality_type_arr[4]={
	JPEG_ENC_QUALITY_TYPE_LOW_BITRATE,
	JPEG_ENC_QUALITY_TYPE_NORMAL,
	JPEG_ENC_QUALITY_TYPE_HIGH_QUALITY,
	JPEG_ENC_QUALITY_TYPE_ULTRA_HIGH_QUALITY,
};
static int g_quality = 0; //middle bit rate

struct data_element{
	data_header_t header;
	uint8_t *data;
	uint8_t *data_align;
};

static Queue *video_queue;
static Queue *audio_queue;

static sem_t video_sema;
static sem_t audio_sema;

static int sndinfo_fd;

#ifdef __HCRTOS__
struct kshm_info rx_audio_read_hdl = {0};
struct kshm_info rx_video_read_hdl = {0};
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

struct data_cache{
	int start;
	int last;
	int count;
	int max;
	struct data_element *data[0];
};

struct data_cache *data_cache_init(int max)
{
	struct data_cache *p;	
	p = calloc(1, sizeof(struct data_cache) + max * sizeof(struct data_element *));
	if(!p){
		printf("%s:not enough memory.\n", __func__);
		return NULL;
	}
	p->max = max;
	return p;
}

static void data_cache_insert(struct data_cache *p, struct data_element *data)
{
	if(p->count < p->max){
		p->data[p->last] = data;
		p->last = (p->last + 1)%p->max;
		p->count++;
	}else{
		queue_element_free(p->data[p->start]);
		p->data[p->start] = NULL;
		p->start = (p->start + 1)%p->max;
		p->data[p->last] = data;
		p->last = (p->last + 1)%p->max;
		p->count++;

	}
	/*printf("p->count: %d, p->last: %d, p->start: %d\n", p->count, p->last, p->start);*/

}


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


static int create_socket(void)
{

	int fd = -1;
	struct sockaddr_in my_addr;
	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htons(INADDR_ANY);
	my_addr.sin_port = htons(0);
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0) {
		printf("socket create error. error: %s\n", strerror(errno));
		return -1;
	}

	{
		int opt = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	}

	if(bind(fd, (struct sockaddr *)&my_addr, sizeof(my_addr))) {
		printf("bind error. error: %s", strerror(errno));
		goto fail;
	}


	return fd;
fail:
	if(fd > 0)
		close(fd);
	return -1;
}
#ifndef __HCRTOS__
static int read_data(int fd, uint8_t *buf, int size)
{
	int remain = size;
	uint8_t *p = buf;
	while(read(fd, p, remain) != size && !stop_read) {
		usleep(20*1000);
	}
	return 0;
}
#else
static int read_data(struct kshm_info *hdl, uint8_t *buf, int size)
{
	int remain = size;
	uint8_t *p = buf;
	while(kshm_read(hdl, p, remain) != size && !stop_read) {
		usleep(20*1000);
	}
	return 0;
}
#endif

static void *audio_data_thread(void *args)
{
	AvPktHd hdr = {0};
	struct data_element *e;
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

#ifndef __HCRTOS__
		if (read_data(akshm_fd, (uint8_t *)&hdr, sizeof(AvPktHd)) != 0) {
#else
		if (read_data(&rx_audio_read_hdl, (uint8_t *)&hdr, sizeof(AvPktHd)) != 0) {
#endif
			printf("read audio hdr from kshm err\n");
			break;
		}

		if(hdr.size <=0)
			goto again1;
		e->data = malloc(hdr.size + 2*32 + RAW_SEGEMENT_UNIT);
		if(!e->data){
			printf("%s:%d,Not enough memory\n",__func__, __LINE__);
			goto again1;
		}

		if((uint32_t)e->data & 0x1f)
			e->data_align = (uint8_t *)((uint32_t)(e->data + 32)&(~(1<<5 - 1)));
		else
			e->data_align = e->data;

#ifndef __HCRTOS__
		if(read_data(akshm_fd, e->data_align, hdr.size) != 0) {
#else
		if(read_data(&rx_audio_read_hdl, e->data_align, hdr.size) != 0) {
#endif
			printf("read audio data from kshm err\n");
			usleep(30 * 1000);
			free(e->data);
			goto again1;
		}

		e->header.size = hdr.size;
		e->header.pts = hdr.pts;
		e->header.sample_rate = get_sample_rate();

		strncpy(e->header.magic, DATA_HEADER_MAGIC, sizeof(e->header.magic));
		/*printf("%s, e->header.size: %d\n", __func__, e->header.size);*/

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
		sem_post(&audio_sema);
	}
	return NULL;
}

struct quality_stats {
	int threshold_high;
	int threshold_low;
	int max_delay_high;
	int max_delay_low;
	int delay_high;
	int delay_low;
};

#define FRAME_SIZE_MAX (11*1024*1024/30)
static struct jenc_rdo_t g_rdo;
static int update_enc_table(struct jenc_rdo_t *rdo)
{
	struct jpeg_enc_quant enc_table;

	/*printf("(y, c) = (%d, %d)\n", jenc_rdo_y_q_index(rdo), jenc_rdo_c_q_index(rdo));*/
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
	if (size > FRAME_SIZE_MAX){
		jenc_rdo_rate_control(&g_rdo, JENC_RDO_OP_REDUCE_BITRATE);
		update_enc_table(&g_rdo);
	}else if ( size < (FRAME_SIZE_MAX - 0.1 * FRAME_SIZE_MAX)){
		jenc_rdo_rate_control(&g_rdo, JENC_RDO_OP_INCREASE_BITRATE);
		update_enc_table(&g_rdo);
	}
}

static void *video_data_thread(void *args)
{
	AvPktHd hdr = {0};
	struct data_element *e;
	int quality = 1;

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
		if (read_data(&rx_video_read_hdl, (uint8_t *)&hdr, sizeof(AvPktHd)) != 0) {
#endif
			printf("read video hdr from kshm err\n");
			break;
		}

		if(hdr.size <=0){
			goto again1;
		}
		e->data = malloc(hdr.size + 2*32 + RAW_SEGEMENT_UNIT);
		if(!e->data){
			printf("%s:%d,Not enough memory\n",__func__, __LINE__);
			goto again1;
		}
		if((uint32_t)e->data & 0x1f)
			e->data_align = (uint8_t *)((uint32_t)(e->data + 32)&(~(1<<5 - 1)));
		else
			e->data_align = e->data;
		/*printf("e->data: %p, e->data_align: %p\n", e->data, e->data_align);*/

#ifndef __HCRTOS__
		if(read_data(vkshm_fd, e->data_align, hdr.size) != 0) {
#else
		if(read_data(&rx_video_read_hdl, e->data_align, hdr.size) != 0) {
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

		if(!video_send_running){
			free(e->data);
			goto again1;
		}

#ifdef AIRPLAY_FRAME_RATE
		/*print_frame_rate(&video_frame_rate, 1, e->header.size);*/
#endif

		jdo_change_quality(e->header.size);

		if(enqueue(video_queue, e)){
			free(e->data);
			e->data = NULL;
			goto again1;
		}
		sem_post(&video_sema);
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

	struct data_element *e = NULL;
	struct data_cache *cache = NULL;;
	printf("rx_audio_read_thread run\n");
	cache = data_cache_init(5);
	if(!cache){
		printf("%s:%d, init data cache error.\n", __func__, __LINE__);
		return NULL;
	}

	while (!stop_read) {
		queue_clear(audio_queue, queue_element_free);
		audio_send_running = true;
		while(true) {
			e = front(audio_queue);
			if(!e){
				sem_wait(&audio_sema);
				continue;
			}

			/*printf("%s, e->header.size: %d\n", __func__, e->header.size);*/
			raw_send_func(&e->header, e->data_align, e->header.size, STREAM_AUDIO, STREAM_AUDIO_DATA);

#ifdef AIRPLAY_FRAME_RATE
			print_frame_rate(&audio_frame_rate, 1, e->header.size);
#endif
			data_cache_insert(cache, e);
		}
		audio_send_running = false;
	}

	return NULL;
fail:
	return NULL;
}




static void *rx_video_read_thread(void *args)
{
	struct data_element *e = NULL;
	struct data_cache *cache = NULL;;
	printf("rx_video_read_thread run\n");
	cache = data_cache_init(5);
	if(!cache){
		printf("%s:%d, init data cache error.\n", __func__, __LINE__);
		return NULL;
	}

	while (!stop_read) {
		queue_clear(video_queue, queue_element_free);
		video_send_running = true;
		while(true) {
			e = front(video_queue);
			if(!e){
				sem_wait(&video_sema);
				continue;
			}
			raw_send_func(&e->header, e->data_align, e->header.size, STREAM_VIDEO, STREAM_VIDEO_DATA);
			/*printf("%s, e->header.size: %ld\n", __func__, e->header.size);*/
#ifdef AIRPLAY_FRAME_RATE
			print_frame_rate(&video_frame_rate, 1, e->header.size);
#endif
			
			data_cache_insert(cache, e);

		}
		video_send_running = false;
	}

	free(cache);

	return NULL;
}

static void usage(void)
{
	printf("-v video path\n");
	printf("-a audio path\n");
	printf("-q encode quality \
			0 middle bit rate\n \
			1 low bit rate\n \
			2 high bit rate\n");
	printf("-s server ip\n");
}

static int hdmi_rx_set_enc_table(void)
{
    int opt;
    opterr = 0;
    optind = 0;

    struct jpeg_enc_quant enc_table;

    if(rx_fd >= 0)
    {
	    printf("%s:%d\n", __func__, __LINE__);
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

#ifdef __HCRTOS__
int display_source_main (int argc, char *argv[])
#else
int main (int argc, char *argv[])
#endif
{
	enum HDMI_RX_VIDEO_DATA_PATH vpath = HDMI_RX_VIDEO_TO_OSD;
	enum HDMI_RX_AUDIO_DATA_PATH apath = HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX;
	char server_ip[32];

	signal(SIGPIPE, SIG_IGN);

	memset(server_ip, 0, sizeof(server_ip));
	pthread_attr_t attr;
	int opt;

	opterr = 0;
	optind = 0;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 0x1000);
	sem_init(&video_sema, 0, 1);
	sem_init(&audio_sema, 0, 1);

	audio_queue = initQueue(5);
	if(!audio_queue){
		printf("init audio queue error.\n");
		return -1;
	}

	video_queue = initQueue(2);
	if(!video_queue){
		printf("init video queue error.\n");
		return -1;
	}
	printf("jenc_rd_init\n");
	jenc_rdo_init(&g_rdo, JPEG_QUANT_TABLES_SIZE, 9, 2,
			JPEG_QUANT_TABLES_SIZE, 17, 2);
	raw_init("eth0", recv_calback, NULL);

#ifndef __HCRTOS__
	akshm_fd = open("/dev/kshmdev", O_RDONLY);
	vkshm_fd = open("/dev/kshmdev", O_RDONLY);
	if (akshm_fd < 0 || vkshm_fd < 0) {
		printf("%s:%d\n", __func__, __LINE__);
		goto err;
	}
#endif
	
	while ((opt = getopt(argc, argv, "a:v:r:s:q:h")) != EOF) {
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
		case 'q':
			g_quality = atoi(optarg);
			break;
		case 'h':
			usage();
			return 0;
		default:
			break;
		}
	}

	rx_fd = open("/dev/hdmi_rx", O_WRONLY);
	if (rx_fd < 0) {
		printf("%s:%d\n", __func__, __LINE__);
		goto err;
	}

	printf("apath %d, vpath %d\n", apath, vpath);
	ioctl(rx_fd, HDMI_RX_SET_VIDEO_DATA_PATH, vpath);
	ioctl(rx_fd, HDMI_RX_SET_AUDIO_DATA_PATH, apath);
	
	/*ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , JPEG_ENC_QUALITY_TYPE_NORMAL);*/
	printf("g_quality: %d\n", g_quality);
#if 0
	ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , JPEG_ENC_QUALITY_TYPE_ULTRA_HIGH_QUALITY);
	/*ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY, JPEG_ENC_QUALITY_TYPE_LOW_BITRATE);*/
#else
	hdmi_rx_set_enc_table();
	ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY, JPEG_ENC_QUALITY_TYPE_USER_DEFINE);
#endif
    /*set_hdmi_rx_encoder_quality(g_quality);*/
	if(true){
		ioctl(rx_fd, HDMI_RX_AUDIO_KSHM_ACCESS, &rx_audio_read_hdl);
		printf("get audio hdl, kshm desc %p\n", rx_audio_read_hdl.desc);
#ifndef __HCRTOS__
		ioctl(akshm_fd, KSHM_HDL_SET, &rx_audio_read_hdl);
#endif

		stop_read = 0;
		if (pthread_create(&audio_data_thread_id, &attr,
		                   audio_data_thread, NULL)) {
			printf("audio kshm recv thread create failed\n");
			goto err;
		}

		if (pthread_create(&rx_audio_read_thread_id, &attr,
		                   rx_audio_read_thread, server_ip)) {
			printf("audio kshm recv thread create failed\n");
			goto err;
		}

	}

	if (vpath == HDMI_RX_VIDEO_TO_KSHM || vpath==HDMI_RX_VIDEO_TO_DE_AND_KSHM || vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM ) {

		printf("%s:%d\n", __func__, __LINE__);
		ioctl(rx_fd, HDMI_RX_VIDEO_KSHM_ACCESS, &rx_video_read_hdl);
		printf("get video hdl, kshm desc %p\n", rx_video_read_hdl.desc);
#ifndef __HCRTOS__
		ioctl(vkshm_fd, KSHM_HDL_SET, &rx_video_read_hdl);
#endif

		stop_read = 0;
		if (pthread_create(&video_data_thread_id, &attr,
		                   video_data_thread, NULL)) {
			printf("video kshm recv thread create failed\n");
			goto err;
		}
#if 1

		if (pthread_create(&rx_video_read_thread_id, &attr,
		                   rx_video_read_thread, server_ip)) {
			printf("video kshm recv thread create failed\n");
			goto err;
		}
#endif
	}

	printf("%s:%d\n", __func__, __LINE__);
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

	if (arecfile)
		fclose(arecfile);
	if (vrecfile)
		fclose(vrecfile);
	arecfile = vrecfile = NULL;

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
CONSOLE_CMD(display_source, NULL, display_source_main, CONSOLE_CMD_MODE_SELF, "display_source")
#endif
