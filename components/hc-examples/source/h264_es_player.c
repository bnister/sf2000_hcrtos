#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <pthread.h>

#include "es_decoder.h"

#define QUICK_MODE_ENABLE 0
#define ROTATE_ENABLE
#ifdef ROTATE_ENABLE
#define ROTATE_MODE 1
#else
#define ROTATE_MODE 0
#endif //  ROTATE_ENABLE

static char *file_name[] = {
#if 1


    "r_video_1080x1920_1.h264",
    "r_video_1080x1920_2.h264",
    "r_video_1080x1920_3.h264",
    "r_video_1080x1920_4.h264",
    "r_video_1080x1920_5.h264",
    "r_video_1080x1920_6.h264",
    "r_video_1080x1920_7.h264",
    "r_video_1080x1920_8.h264",
    "r_video_1080x1920_9.h264",
    "r_video_1080x1920_10.h264",
    "r_video_640x1136_1.h264",
    "r_video_752x1344_1.h264",
    "r_video_752x1344_2.h264",
    "r_video_886x1920_1.h264",
    "r_video_886x1920_2.h264.h264",
    "r_video_896x1920_1.h264",


    //"4k.ts_video_es.h264",
    //"video_2436_2436_1.h264",
    //"video_2688_2688_1.h264",
    //"video_1920x1920_1.h264",
    //"video_1920_1080_1.h264",


    "video_1126_2436_6.h264",
    "video_1242_2688_1.h264",
    "video_2688_1242_1.h264",

    "video_2436_1126_1.h264",
    "video_2436_1126_2.h264",
    "video_2436_1126_3.h264",
    "video_2436_1126_4.h264",
    "video_2436_1126_5.h264",

    "video_1126_2436_1.h264",
    "video_1126_2436_2.h264",
    "video_1126_2436_3.h264",
    "video_1126_2436_4.h264",
    "video_1126_2436_5.h264",

    "video_1920_1080_1.h264",
    "video_1920_1080_2.h264",
    "video_1920_1080_3.h264",
    "video_1920_1080_4.h264",
    "video_1920_1080_5.h264",

    "video_1080_1920_1.h264",
    "video_1080_1920_2.h264",

    "video_1024_768_1.h264",
    "video_1136_640_1.h264",
    "video_1136_640_2.h264",
    "video_1136_640_3.h264",
    "video_1136_750_5.h264",
    "video_1136_752_4.h264",
    "video_1280_722_1.h264",
    "video_1280_722_2.h264",
    "video_640_1136_1.h264",
    "video_640_1136_2.h264",
    "video_640_1136_3.h264",
    "video_640_1136_4.h264",
    "video_752_1134_1.h264",


#endif
};

void *g_p_handle = NULL;
static int dec_pkt_num = 0;

static void init_video_config(void)
{
    int width = 1920;
    int height = 1080;
    uint8_t *extradata = NULL;
    int extradata_size = 0;
    uint8_t rotate_enable = 0;
    int8_t quick_mode_enable = QUICK_MODE_ENABLE;

#ifdef ROTATE_ENABLE
    rotate_enable = 1;
#endif
    g_p_handle = h264_decode_init(width , height , 
                                  extradata , extradata_size ,
                                  rotate_enable, quick_mode_enable);
}

static int decode_packet(void *g_p_handle , char *video_frame , int packet_size)
{
    uint32_t rotate_mode = ROTATE_MODE;

    if(dec_pkt_num <200)
    {
        rotate_mode = 0;
    }
    else
    {
        rotate_mode = ROTATE_MODE;
    }
    h264_decode(g_p_handle , (uint8_t *)video_frame , packet_size , rotate_mode);
    dec_pkt_num++;
	
	return 0;
}

int play_h264_es_file(char *filename)
{
    printf("enter play_h264_es\n");
    //char *devname = NULL;
    //int dec_fd = 0;
    FILE *h264_fd = 0;
    //int ret = 0;

    int need_read = 1;
    int read_size = 0;
    int one_frame_size = 0;
    int packet_size = 0;
    char *video_frame = NULL;

    char m_video_file_name[256] = { 0 };

    memcpy(m_video_file_name , "/media/sda2/" , 256);
    strcat(m_video_file_name , filename);

    h264_fd = fopen(m_video_file_name , "r");
    if( h264_fd == NULL ) {
        printf("%s fail\n" , m_video_file_name);
        return 0;
    }

    dec_pkt_num = 0;

    init_video_config();
    video_frame = malloc(0x100000);

    while( 1 ) {
        if( need_read ) {
            read_size = fread(&one_frame_size , 1 , 4 , h264_fd);
            if( one_frame_size > 0x100000 ) {
                printf("size error\n");
            }
            if( read_size < 4 ) {
                break;
            }
            read_size = fread(video_frame , 1 , one_frame_size , h264_fd);
            //printf("one_frame_size = 0x%x\n" , one_frame_size);
            if( read_size < one_frame_size ) {
                break;
            }
            packet_size = one_frame_size;
        }

        if( read_size == packet_size ) {
            decode_packet(g_p_handle , video_frame , one_frame_size);
        }
        else {
            printf("file end\n");
            break;
        }
//sleep:
        usleep(33 * 1000);
    }

    if( video_frame != NULL ) {
        free(video_frame);
        video_frame = NULL;
    }

    h264_decoder_destroy(g_p_handle);
    fclose(h264_fd);
    printf("file end\n");
    return 0;
}

int play_h264_es(int argc , char *argv[])
{
    int i;
	(void)argc;
	(void)argv;

    while( 1 ) {
        for( i = 0; i < (int)sizeof(file_name) / 4; i++ ) {
            play_h264_es_file(file_name[i]);
        }
    }
}

static bool h264_player_running = 0;
static char h264_path[512];
static pthread_t tid = 0;
static void *es_play_thread(void *arg)
{
    printf("enter play_h264_es\n");
    char *filename = (char *)arg;
    FILE *h264_fd = 0;
    int need_read = 1;
    int read_size = 0;
    int one_frame_size = 0;
    int packet_size = 0;
    char *video_frame = NULL;
    h264_fd = fopen(filename , "r");
    if( h264_fd == NULL ) {
        printf("%s fail\n" , filename);
        return 0;
    }
    dec_pkt_num = 0;
    init_video_config();
    video_frame = malloc(0x100000);
    while(h264_player_running) {
        if( need_read ) {
            read_size = fread(&one_frame_size , 1 , 4 , h264_fd);
            if( one_frame_size > 0x100000 ) {
                printf("size error\n");
            }
            if( read_size < 4 ) {
                break;
            }
            read_size = fread(video_frame , 1 , one_frame_size , h264_fd);
            if( read_size < one_frame_size ) {
                break;
            }
            packet_size = one_frame_size;
        }
        if( read_size == packet_size ) {
            decode_packet(g_p_handle , video_frame , one_frame_size);
        }
        else {
            printf("file end\n");
            break;
        }
        usleep(33 * 1000);
    }
    if( video_frame != NULL ) {
        free(video_frame);
        video_frame = NULL;
    }
    h264_decoder_destroy(g_p_handle);
    fclose(h264_fd);
    printf("file end\n");
    g_p_handle = NULL;
    return 0;
}

int h264_es_play(int argc , char *argv[])
{
	if(argc != 2){
		printf("Usage: %s <h264 path>\n", argv[0]);
		return -1;
	}
	if(g_p_handle && h264_player_running){
		h264_player_running = 0;
		pthread_join(tid, NULL);
	}
	strncpy(h264_path, argv[1], sizeof(h264_path));
	printf("h264_path: %s\n", h264_path);
	h264_player_running = 1;
	if(pthread_create(&tid, NULL, es_play_thread, h264_path)){
		printf("Create es thread error\n");
		h264_player_running = 0;
		return -1;
	}
	return 0;
}

int h264_es_stop(int argc , char *argv[])
{
	(void)argc;
	(void)argv;
	if(g_p_handle && h264_player_running){
		h264_player_running = 0;
		pthread_join(tid, NULL);
	}
	return 0;
}
