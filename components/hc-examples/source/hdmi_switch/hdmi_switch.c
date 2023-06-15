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
#include <malloc.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/hdmi_rx.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>
#include <hcuapi/fb.h>

#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <hcuapi/notifier.h>
#include <hcuapi/kumsgq.h>
#include <sys/epoll.h>
#include <errno.h>

#include <sys/msg.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include "console.h"
#include <sys/mman.h>
#include <ffplayer.h>
#include <linux/fb.h>
#include <unistd.h>

#define MSG_TASK_STACK_DEPTH (0x2000)
#define MX_EVNTS (10)
#define EPL_TOUT (1000)

#define HDMI_SWITCH_HDMI_STATUS_PLUGOUT		0
#define HDMI_SWITCH_HDMI_STATUS_PLUGIN		1
#define HDMI_SWITCH_BOOTLOGO_LAYER			DIS_PIC_LAYER_CURRENT
#define HDMI_SWITCH_HDMI_RX_LAYER           DIS_PIC_LAYER_MAIN

#define HDMI_SWITCH_DEFAULT_DISP_RESOLUTION TVSYS_1080P_60
#define HDMI_SWITCH_GET_EDID_TRY_TIME		500

struct hdmi_info{
    int fd;

    pthread_t pid;
    int thread_run;
    int epoll_fd;
    int kumsg_id;

    char plug_status;
    char enable;
};

struct hdmi_switch{
    struct hdmi_info in;
    struct hdmi_info out;

    int dis_fd;
    int fb_fd;
    struct dis_display_info bootlogo;
    struct dis_display_info hdmi_rx;
    struct dis_tvsys hdmi_out_tvsys;
    enum TVSYS last_tv_sys;
};

typedef struct mediaplayer {
	void *player;
	char *uri;
} mediaplayer;

struct hdmi_switch g_switch;
static sem_t get_edid_thread_wait_sem;
static pthread_t get_edid_thread_t = 0;
static sem_t hdmi_in_thread_wait_sem;
static pthread_t hdmi_in_thread_t = 0;
static int hdmi_in_thread_run = 0;

static int get_edid_task_run = 0;
static int try_get_edid = 0;
static int hdmi_out_force_4k = 0;
static int hdmi_out_force_1080p = 0;
static int hdmi_out_force_720p = 0;
static enum TVSYS tv_sys_4k = TVSYS_4096X2160P_30;
static enum TVSYS tv_sys_1080p = TVSYS_1080P_60;
static enum TVSYS tv_sys_720p = TVSYS_720P_60;

static struct mediaplayer g_mp;

static int tvsys_to_tvtype(enum TVSYS tv_sys, struct dis_tvsys *tvsys)
{
    switch(tv_sys){
        case TVSYS_480I:
            tvsys->tvtype = TV_NTSC;
            tvsys->progressive = 0;
        break;

        case TVSYS_480P:
            tvsys->tvtype = TV_NTSC;
            tvsys->progressive = 1;
        break;

        case TVSYS_576I :
            tvsys->tvtype = TV_PAL;
            tvsys->progressive = 0;
        break;

        case TVSYS_576P:
            tvsys->tvtype = TV_PAL;
            tvsys->progressive = 1;
        break;

        case TVSYS_720P_50:
            tvsys->tvtype = TV_LINE_720_30;
            tvsys->progressive = 1;
        break;

        case TVSYS_720P_60:
            tvsys->tvtype = TV_LINE_720_30;
            tvsys->progressive = 1;
        break;

        case TVSYS_1080I_25:
            tvsys->tvtype = TV_LINE_1080_25;
            tvsys->progressive = 0;
        break;

        case TVSYS_1080I_30:
            tvsys->tvtype = TV_LINE_1080_30;
            tvsys->progressive = 0;
        break;

        case TVSYS_1080P_24:
            tvsys->tvtype = TV_LINE_1080_24;
            tvsys->progressive = 1;
        break;

        case TVSYS_1080P_25:
            tvsys->tvtype = TV_LINE_1080_25;
            tvsys->progressive = 1;
        break;

        case TVSYS_1080P_30:
            tvsys->tvtype = TV_LINE_1080_30;
            tvsys->progressive = 1;
        break;

        case TVSYS_1080P_50:
            tvsys->tvtype = TV_LINE_1080_50;
            tvsys->progressive = 1;
        break;

        case TVSYS_1080P_60:
            tvsys->tvtype = TV_LINE_1080_60;
            tvsys->progressive = 1;
        break;

        case TVSYS_3840X2160P_30:
            tvsys->tvtype = TV_LINE_3840X2160_30;
            tvsys->progressive = 1;
        break;

        case TVSYS_4096X2160P_30:
            tvsys->tvtype = TV_LINE_4096X2160_30;
            tvsys->progressive = 1;
        break;

        default:
            tvsys->tvtype = TV_LINE_720_30;
            tvsys->progressive = 1;
    }

    printf("%s:%d: tvtype=%d\n", __func__, __LINE__, tvsys->tvtype);
    //printf("%s:%d: progressive=%d\n", __func__, __LINE__, tvsys->progressive);

    return 0;
}

static int __set_hdmi_out_tvsys(struct dis_tvsys *tvsys)
{
    int ret = 0;

    if( g_switch.dis_fd < 0){
        printf("g_switch.dis_fd is invalue(%d)\n", g_switch.dis_fd);
        return -1;
    }

    ret = ioctl(g_switch.dis_fd , DIS_SET_TVSYS , tvsys);
    if(ret < 0){
        printf("DIS_SET_TVSYS failed, ret=%d\n", ret);
        return -1;
    }

    return 0;
}

static int set_hdmi_out_tvsys(enum TVSYS tv_sys)
{
    struct dis_tvsys out_tvsys = {0};

    printf("%s:%d: tv_sys= %d\n", __func__, __LINE__, tv_sys);

    if(hdmi_out_force_4k){
        tv_sys = tv_sys_4k;
    }else if(hdmi_out_force_1080p){
        tv_sys = tv_sys_1080p;
    }else if(hdmi_out_force_720p){
        tv_sys = tv_sys_720p;
    }

    tvsys_to_tvtype(tv_sys, &out_tvsys);

    if(g_switch.hdmi_out_tvsys.tvtype !=out_tvsys.tvtype
        ||g_switch.hdmi_out_tvsys.progressive !=out_tvsys.progressive){

        printf("%s:%d: set new display resolution, old(%d, %d), new(%d, %d)\n",
                __func__, __LINE__, g_switch.hdmi_out_tvsys.tvtype, g_switch.hdmi_out_tvsys.progressive,
                out_tvsys.tvtype, out_tvsys.progressive);

        g_switch.hdmi_out_tvsys.tvtype =out_tvsys.tvtype;
        g_switch.hdmi_out_tvsys.progressive =out_tvsys.progressive;
        g_switch.last_tv_sys = tv_sys;
        __set_hdmi_out_tvsys(&g_switch.hdmi_out_tvsys);
    }

    return 0;
}

static int get_hdmi_out_tvsys(struct dis_tvsys *tvsys)
{
    int ret = 0;

    if(tvsys->distype != DIS_TYPE_HD){
        printf("warning: tvsys->distype is DIS_TYPE_HD\n");
        tvsys->distype = DIS_TYPE_HD;
    }

    if( g_switch.dis_fd < 0){
        printf("g_switch.dis_fd is invalue(%d)\n", g_switch.dis_fd);
        return -1;
    }

    ret = ioctl(g_switch.dis_fd , DIS_GET_TVSYS , tvsys);
    if(ret < 0){
        printf("DIS_SET_TVSYS failed, ret=%d\n", ret);
        return -1;
    }

    printf("%s:%d: distype=%d\n", __func__, __LINE__, tvsys->distype);
    printf("%s:%d: layer=%d\n", __func__, __LINE__, tvsys->layer);
    printf("%s:%d: tvtype=%d\n", __func__, __LINE__, tvsys->tvtype);
    printf("%s:%d: progressive=%d\n", __func__, __LINE__, tvsys->progressive);

    return 0;
}

static int play_uri(char *uri)
{
    HCPlayerInitArgs init_args = {0};
    int fd = 0;

    fd = access(uri, F_OK);
    if(fd < 0) {
        printf("err: play_uri, %s is not exist\n", uri);
        return -1;
    }

    init_args.uri = uri;
    g_mp.player = hcplayer_create(&init_args);
    if (!g_mp.player) {
        printf("err: hcplayer_create failed\n");
        return -1;
    }

    g_mp.uri = strdup(uri);
    hcplayer_play(g_mp.player);

    return 0;
}

static int mp_init(void)
{
    memset(&g_mp, 0, sizeof(struct mediaplayer));

    play_uri("/usr/share/hdmi_switch/no_signal.jpg");

    return 0;
}

static int mp_exit(void)
{
    hcplayer_stop2(g_mp.player, true, false);
    return 0;
}

#if 1
static int hdmi_rx_on_off(int on)
{
    int blank_mode = 0;

    if(on){
        blank_mode = HDMI_RX_VIDEO_BLANK_UNBLANK;
    }else{
        blank_mode = HDMI_RX_VIDEO_BLANK_NORMAL;
    }

    if (ioctl(g_switch.in.fd, HDMI_RX_VIDEO_BLANK, blank_mode) != 0) {
        printf("err: HDMI_RX_VIDEO_BLANK ioctrl failed\n");
        return -1;
    }

    return 0;
}

static int show_no_signal(int show)
{
    int on = 0;

    if(show){
        mp_init();
        on = 0;
    }else{
        mp_exit();
        on = 1;
    }

    return hdmi_rx_on_off(on);
}

#else

static int change_layer_order(int show)
{
    struct dis_layer_blend_order vhance = {0};
    int ret = 0;

    vhance.distype = DIS_TYPE_HD;
    if(show){
        vhance.main_layer = 3;
        vhance.auxp_layer = 2;
        vhance.gmas_layer = 1;
        vhance.gmaf_layer = 0;
    }else{
        vhance.main_layer = 0;
        vhance.auxp_layer = 2;
        vhance.gmas_layer = 1;
        vhance.gmaf_layer = 3;
    }

    return ioctl(g_switch.dis_fd, DIS_SET_LAYER_ORDER, &vhance);
}

static int show_no_signal(int show)
{
    if(show){
        mp_init();
    }else{
        mp_exit();
    }

    return change_layer_order(show);
}
#endif

/* rgb: black(0,0,0) */
/* yuv444: black(0,128,128) */

#define ARGB_BLACK_RANGE 40
struct argb{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
};

#define AYUV_BLACK_RANGE 17
struct ayuv{
    uint8_t v;
    uint8_t u;
    uint8_t y;
    uint8_t a;
};

static int is_pixel_black(enum HDMI_RX_VIDEO_COLOR_FORMAT color_format, uint32_t data)
{
    int black = 0;

    switch(color_format){
        case HDMI_RX_VIDEO_FORMAT_RGB:
        {
            struct argb *rgb = (struct argb *)&data;

            if(rgb->r < ARGB_BLACK_RANGE && rgb->g < ARGB_BLACK_RANGE && rgb->b < ARGB_BLACK_RANGE){
                black = 1;
            }
        }
        break;

        case HDMI_RX_VIDEO_FORMAT_444:
        {
            struct ayuv *yuv = (struct ayuv *)&data;

            if(yuv->y < AYUV_BLACK_RANGE){
                if((yuv->u < (128 + AYUV_BLACK_RANGE)) && (yuv->u > (128 - AYUV_BLACK_RANGE))){
                    if((yuv->v < (128 + AYUV_BLACK_RANGE)) && (yuv->v > (128 - AYUV_BLACK_RANGE))){
                        black = 1;
                    }
                }
            }
        }
        break;

        default:
            printf("err: is_black, unkown color_format(%d)\n", color_format);
    }

#if 0
    {
        char *temp = (char *)&data;

        printf("%d %d %d %d\n", temp[3], temp[2], temp[1], temp[0]);
    }
#endif
    return black;
}

static int is_black(enum HDMI_RX_VIDEO_COLOR_FORMAT color_format, void *buffer, int size)
{
    int *data = (int *)buffer;
    int black = 0, i = 0;
    int pixel_num = size/4;

    for(i = 0; i < pixel_num; i++){
        if(is_pixel_black(color_format, data[i])){
            black = 1;
        }else{
            //printf("%s:%d: is not all black\n", __func__, __LINE__);
            return 0;
        }
    }

    //printf("%s:%d: black=%d\n", __func__, __LINE__, black);

    return black;
}

/* Analysis hdmi rx data */
#if 0

static void *hdmi_in_thread(void *args)
{
    int ret = 0;
    struct hdmi_rx_video_info rx_info = {0};
    int buffer_size = 0;
    void *video_buffer = NULL;
    char get_buffer_retry = 1;
    char debounce_time = 0;
    char show_picture = 0;

    while(hdmi_in_thread_run){
        sem_wait(&hdmi_in_thread_wait_sem);

        if(hdmi_in_thread_run == 0){
            goto end;
        }

        printf("%s:%d: \n", __func__, __LINE__);

        //get hdmi rx buffer
        get_buffer_retry = 1;
        while(get_buffer_retry){
            usleep(500000);

            if(hdmi_in_thread_run == 0){
                goto end;
            }

            if(g_switch.in.plug_status == HDMI_SWITCH_HDMI_STATUS_PLUGOUT){
                break;
            }

            ret = ioctl(g_switch.in.fd, HDMI_RX_GET_VIDEO_INFO , &rx_info);
            if(ret != 0){
                printf("HDMI_RX_SET_VIDEO_STOP_MODE failed, need retry, ret=%d\n", ret);
                continue;
            }

            printf("color_format =%d\n", rx_info.color_format);
            printf("width        =%d\n", rx_info.width);
            printf("height       =%d\n", rx_info.height);

            buffer_size = rx_info.width * rx_info.height * 4;
            if(buffer_size == 0){
                printf("buffer_size is zero, need retry\n");
                continue;
            }
            printf("%s:%d: buffer_size=%d\n", __func__, __LINE__, buffer_size);

            video_buffer = (void *)mmap(0, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_switch.in.fd, 0);
            if(video_buffer == MAP_FAILED){
                printf("video_buffer is invalid, need retry\n");
                continue;
            }

            printf("%s:%d: 0x%x\n", __func__, __LINE__,  (unsigned int)video_buffer);
            get_buffer_retry = 0;
        }

        printf("%s:%d: begin to analysis data\n", __func__, __LINE__);

        //Analysis data
        //If five consecutive detections are black, switch to the no signal picture
        while(g_switch.in.plug_status == HDMI_SWITCH_HDMI_STATUS_PLUGIN){
            sleep(1);

            if(hdmi_in_thread_run == 0){
                goto end;
            }

            if(is_black(rx_info.color_format, video_buffer, buffer_size)){
                if(show_picture){
                    continue;
                }else{
                    printf("%s:%d: is black\n", __func__, __LINE__);
                    debounce_time++;
                    if(debounce_time == 5){
                        debounce_time = 0;
                        show_picture = 1;

                        printf("show picture\n");
                        show_no_signal(1);
                    }
                }
            }else{
                if(show_picture){
                    printf("switch to hdmi rx\n");
                    show_no_signal(0);
                }
                show_picture = 0;
                continue;
            }
        }
    }

end:

    return NULL;
}

#else

static void *hdmi_in_thread(void *args)
{
    int ret = 0;
    struct hdmi_rx_video_info rx_info = {0};
    int buffer_size = 0;
    void *video_buffer = NULL;
    char get_buffer_retry = 1;
    char debounce_time = 0;
    char show_picture = 0;

    while(hdmi_in_thread_run){
        /* waiting for rx plugin */
        sem_wait(&hdmi_in_thread_wait_sem);

        if(hdmi_in_thread_run == 0){
            goto end;
        }

        get_buffer_retry = 1;
        while(g_switch.in.plug_status == HDMI_SWITCH_HDMI_STATUS_PLUGIN){
            if(hdmi_in_thread_run == 0){
                goto end;
            }

            /* waiting for buffer ready */
            if(get_buffer_retry){
                usleep(500000);
            }

            /* get hdmi rx data */
            ret = ioctl(g_switch.in.fd, HDMI_RX_GET_VIDEO_INFO , &rx_info);
            if(ret != 0){
                printf("%s:%d: get rx info failed, need retry, ret=%d\n", __func__, __LINE__, ret);
                get_buffer_retry = 1;
                continue;
            }else{
                /* is data vaild? */
                buffer_size = rx_info.width * rx_info.height * 4;
                if(buffer_size == 0){
                    printf("%s:%d: buffer_size is zero, need retry\n", __func__, __LINE__);
                    get_buffer_retry = 1;
                    continue;
                }

                get_buffer_retry = 0;
            }

            video_buffer = (void *)mmap(0, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_switch.in.fd, 0);
            if(video_buffer == MAP_FAILED){
                printf("%s:%d: video_buffer is invalid, need retry\n", __func__, __LINE__);
                get_buffer_retry = 1;
                continue;
            }

            /* Analysis data */
            if(is_black(rx_info.color_format, video_buffer, buffer_size)){
                if(show_picture){
                    /* nothing to do */
                }else{
                    printf("%s:%d: is black\n", __func__, __LINE__);
                    debounce_time++;
                    if(debounce_time == 5){
                        debounce_time = 0;
                        show_picture = 1;

                        printf("show picture\n");
                        show_no_signal(1);
                    }
                }
            }else{
                if(show_picture){
                    printf("switch to hdmi rx\n");
                    show_no_signal(0);
                }
                show_picture = 0;
            }

            /* munmap buffer */
            if(video_buffer){
                munmap(video_buffer, buffer_size);
                video_buffer = NULL;
            }

            /* waiting for next frame */
            usleep(16000);
        }
    }

end:

    return NULL;
}

#endif


static int hdmi_rx_start(void)
{
    int ret = 0;

    ret = ioctl(g_switch.in.fd , HDMI_RX_SET_VIDEO_DATA_PATH , HDMI_RX_VIDEO_TO_DE);
    if(ret != 0){
        printf("HDMI_RX_SET_VIDEO_DATA_PATH failed\n");
        return -1;
    }

    ret = ioctl(g_switch.in.fd , HDMI_RX_SET_AUDIO_DATA_PATH , HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX);
    if(ret != 0){
        printf("HDMI_RX_SET_AUDIO_DATA_PATH failed\n");
        return -1;
    }

    ret = ioctl(g_switch.in.fd , HDMI_RX_SET_VIDEO_STOP_MODE , 1);
    if(ret != 0){
        printf("HDMI_RX_SET_VIDEO_STOP_MODE failed\n");
        return -1;
    }

    ret = ioctl(g_switch.in.fd, HDMI_RX_SET_VIDEO_ROTATE_MODE , 0);
    if(ret != 0){
        printf("HDMI_RX_SET_VIDEO_STOP_MODE failed\n");
        return -1;
    }

    ret = ioctl(g_switch.in.fd, HDMI_RX_START);
    if(ret != 0){
        printf("HDMI_RX_START failed\n");
        return -1;
    }

    g_switch.in.enable = 1;

    ret = sem_init(&hdmi_in_thread_wait_sem, 0, 0);
    if (ret != 0) {
        printf("err: sem_init failed!\n");
        //return -1;
    }

    hdmi_in_thread_run = 1;
    ret = pthread_create(&hdmi_in_thread_t, NULL, hdmi_in_thread, NULL);
    if (ret != 0) {
        printf("pthread_create failed\n");
        //return -1;
    }

    return 0;
}

static int hdmi_rx_stop(void)
{
    if(g_switch.in.fd >= 0){
        ioctl(g_switch.in.fd , HDMI_RX_STOP);
    }

    return 0;
}

/* stop show bootlogo, end show hdmi in picture */
static void notifier_hdmi_in_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.in.plug_status = HDMI_SWITCH_HDMI_STATUS_PLUGIN;

    sem_post(&hdmi_in_thread_wait_sem);

    return ;
}

/* start show bootlogo */
static void notifier_hdmi_in_plugout(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.in.plug_status = HDMI_SWITCH_HDMI_STATUS_PLUGOUT;

    return ;
}

/* nothing todo */
static void notifier_hdmi_out_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.out.plug_status = HDMI_SWITCH_HDMI_STATUS_PLUGIN;

    /* create thread to get edid info, if not recieve edid notify */
    try_get_edid = 1;

    sem_post(&get_edid_thread_wait_sem);

    return ;
}

/* nothing todo */
static void notifier_hdmi_out_plugout(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch.out.plug_status = HDMI_SWITCH_HDMI_STATUS_PLUGOUT;
    try_get_edid = 0;

    return ;
}

/* detect the HDMI format and adapt it */
static void notifier_hdmi_out_edidready(void *arg, unsigned long param)
{
    struct hdmi_switch *h_switch = (struct hdmi_switch *)arg;
    struct hdmi_edidinfo *edidinfo = (struct hdmi_edidinfo *)param;
    //struct dis_tvsys out_tvsys = {0};

    printf("%s:%d: \n", __func__, __LINE__);

    if(h_switch == NULL || edidinfo == NULL){
        printf("%s:%d: h_switch or edidinfo is null\n", __func__, __LINE__);
        return;
    }

    /* print edid info */
    if(0){
        int i = 0;

        for(i = 0; i < HDMI_TX_MAX_EDID_TVSYS_NUM; i++){
            printf("tvsys[%d]=%d\n", i, edidinfo->tvsys[i]);
        }
        printf("num_tvsys=%d\n", edidinfo->num_tvsys);
        printf("audfmt=%d\n", edidinfo->audfmt);
        printf("best_tvsys=%d\n", edidinfo->best_tvsys);
    }

    try_get_edid = 0;

    if(hdmi_out_force_4k){
        int i = 0;
        int tvsys_4096 = 0;
        int tvsys_3840 = 0;

        for(i = 0; i < HDMI_TX_MAX_EDID_TVSYS_NUM; i++){
            printf("%s:%d: %d, %d\n", __func__, __LINE__, i, edidinfo->tvsys[i]);

            if(edidinfo->tvsys[i] == TVSYS_3840X2160P_30){
                tvsys_3840 = 1;
            }

            if(edidinfo->tvsys[i] == TVSYS_4096X2160P_30){
                tvsys_4096 = 1;
            }
        }

        if (tvsys_4096) {
            tv_sys_4k = TVSYS_4096X2160P_30;
        } else if(tvsys_3840){
            tv_sys_4k = TVSYS_3840X2160P_30;
        }else{
            tv_sys_4k = TVSYS_4096X2160P_30;
        }
    }

    /* update hdmi out info */
    if(g_switch.last_tv_sys != edidinfo->best_tvsys){
        set_hdmi_out_tvsys(edidinfo->best_tvsys);
    }else{
        printf("%s:%d: last_tv_sys(%d) == tv_sys(%d), not set resolution\n",
                __func__, __LINE__, g_switch.last_tv_sys, edidinfo->best_tvsys);
    }

    return ;
}

static void *receive_hdmi_rx_event_func(void *arg)
{
    struct epoll_event events[MX_EVNTS];
    int n = -1;
    int i;

    while(g_switch.in.thread_run) {
        n = epoll_wait(g_switch.in.epoll_fd, events, MX_EVNTS, EPL_TOUT);
        if(n == -1) {
            if(EINTR == errno) {
                continue;
            }
            usleep(100 * 1000);
            continue;
        } else if(n == 0) {
            continue;
        }

        for(i = 0; i < n; i++) {
            unsigned char msg[MAX_KUMSG_SIZE] = {0};
            int len = 0;
            int kumsg_id = (int)events[i].data.ptr;
            len = read(kumsg_id, msg, MAX_KUMSG_SIZE);
            if(len > 0) {
                KuMsgDH *kmsg = (KuMsgDH *)msg;
                switch (kmsg->type) {
                    case HDMI_RX_NOTIFY_CONNECT:
                        printf("hdmi rx connect\n");
                        notifier_hdmi_in_plugin(NULL, 0);
                        break;
                    case HDMI_RX_NOTIFY_DISCONNECT:
                        printf("hdmi rx disconnect\n");
                        notifier_hdmi_in_plugout(NULL, 0);
                        break;
                    default:
                        printf("%s:%d: unkown message type(%d)\n", __func__, __LINE__, kmsg->type);
                    break;
                }
            }
        }
    }

    return NULL;
}

static void *receive_hdmi_tx_event_func(void *arg)
{
    struct epoll_event events[MX_EVNTS];
    int n = -1;
    int i;

    while(g_switch.out.thread_run) {
        n = epoll_wait(g_switch.out.epoll_fd, events, MX_EVNTS, EPL_TOUT);
        if(n == -1) {
            if(EINTR == errno) {
                continue;
            }
            usleep(100 * 1000);
            continue;
        } else if(n == 0) {
            continue;
        }

        for(i = 0; i < n; i++) {
            unsigned char msg[MAX_KUMSG_SIZE] = {0};
            int len = 0;
            int kumsg_id = (int)events[i].data.ptr;
            len = read(kumsg_id, msg, MAX_KUMSG_SIZE);
            if(len > 0) {
                KuMsgDH *kmsg = (KuMsgDH *)msg;
                switch (kmsg->type) {
                    case HDMI_TX_NOTIFY_CONNECT:
                        printf("hdmi tx connect\n");
                        notifier_hdmi_out_plugin(NULL, 0);
                        break;
                    case HDMI_TX_NOTIFY_DISCONNECT:
                        printf("hdmi tx disconnect\n");
                        notifier_hdmi_out_plugout(NULL, 0);
                        break;
                    case HDMI_TX_NOTIFY_EDIDREADY:
                        printf("hdmi tx edidready\n");
                        notifier_hdmi_out_edidready((void *)&g_switch, (unsigned long)&kmsg->params);
                        break;
                    default:
                        printf("%s:%d: unkown message type(%d)\n", __func__, __LINE__, kmsg->type);
                        break;
                }
            }
        }
    }

    return NULL;
}

static int hdmi_hotplug_disable(void)
{
    g_switch.in.thread_run = 0;
    if (pthread_join(g_switch.in.pid, NULL)) {
        printf("thread is not exit...\n");
    }

    if(g_switch.in.epoll_fd >= 0){
        close(g_switch.in.epoll_fd);
        g_switch.in.epoll_fd = -1;
    }

    if(g_switch.in.kumsg_id >= 0){
        close(g_switch.in.kumsg_id);
        g_switch.in.kumsg_id = -1;
    }

    g_switch.out.thread_run = 0;
    if (pthread_join(g_switch.out.pid, NULL)) {
        printf("thread is not exit...\n");
    }

    if(g_switch.out.epoll_fd >= 0){
        close(g_switch.in.epoll_fd);
        g_switch.in.epoll_fd = -1;
    }

    if(g_switch.out.kumsg_id >= 0){
        close(g_switch.in.kumsg_id);
        g_switch.in.kumsg_id = -1;
    }

    return 0;
}

static int hdmi_hotplug_enable(void)
{
    int ret = 0;
    struct epoll_event ev = {0};
    struct kumsg_event event = {0};
    pthread_attr_t attr;

    /* hdmi in hotplug event */
    g_switch.in.thread_run = 1;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, MSG_TASK_STACK_DEPTH);
    if (pthread_create(&g_switch.in.pid, &attr, receive_hdmi_rx_event_func, (void *)NULL)) {
        printf("pthread_create receive_hdmi_rx_event_func fail\n");
        goto err;
    }

    g_switch.in.epoll_fd = epoll_create1(0);
    if(g_switch.in.epoll_fd < 0){
        printf("%s:%d:err: epoll_create1 failed\n", __func__, __LINE__);
        goto err;
    }

    g_switch.in.kumsg_id = ioctl(g_switch.in.fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
    if(g_switch.in.kumsg_id < 0){
        printf("%s:%d:err: KUMSGQ_FD_ACCESS failed\n", __func__, __LINE__);
        goto err;
    }

    ev.events = EPOLLIN;
    ev.data.ptr = (void *)g_switch.in.kumsg_id;
    ret = epoll_ctl(g_switch.in.epoll_fd, EPOLL_CTL_ADD, g_switch.in.kumsg_id, &ev);
    if(ret != 0){
        printf("%s:%d:err: EPOLL_CTL_ADD failed\n", __func__, __LINE__);
        goto err;
    }

    event.evtype = HDMI_RX_NOTIFY_CONNECT;
    event.arg = 0;
    ret = ioctl(g_switch.in.fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret) {
        printf("KUMSGQ_NOTIFIER_SETUP HDMI_RX_NOTIFY_CONNECT fail\n");
        goto err;
    }

    event.evtype = HDMI_RX_NOTIFY_DISCONNECT;
    event.arg = 0;
    ret = ioctl(g_switch.in.fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret) {
        printf("KUMSGQ_NOTIFIER_SETUP HDMI_RX_NOTIFY_DISCONNECT fail\n");
        goto err;
    }

    /* hdmi out hotplug event */
    g_switch.out.thread_run = 1;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, MSG_TASK_STACK_DEPTH);
    if (pthread_create(&g_switch.out.pid, &attr, receive_hdmi_tx_event_func, (void *)NULL)) {
        printf("pthread_create receive_hdmi_tx_event_func fail\n");
        goto err;
    }

    g_switch.out.epoll_fd = epoll_create1(0);
    if(g_switch.out.epoll_fd < 0){
        printf("%s:%d:err: epoll_create1 failed\n", __func__, __LINE__);
        goto err;
    }

    g_switch.out.kumsg_id = ioctl(g_switch.out.fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
    if(g_switch.out.kumsg_id < 0){
        printf("%s:%d:err: KUMSGQ_FD_ACCESS failed\n", __func__, __LINE__);
        goto err;
    }

    ev.events = EPOLLIN;
    ev.data.ptr = (void *)g_switch.out.kumsg_id;
    ret = epoll_ctl(g_switch.out.epoll_fd, EPOLL_CTL_ADD, g_switch.out.kumsg_id, &ev);
    if(ret != 0){
        printf("%s:%d:err: EPOLL_CTL_ADD failed\n", __func__, __LINE__);
        goto err;
    }

    event.evtype = HDMI_TX_NOTIFY_CONNECT;
    event.arg = 0;
    ret = ioctl(g_switch.out.fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret) {
        printf("KUMSGQ_NOTIFIER_SETUP HDMI_TX_NOTIFY_CONNECT fail\n");
        goto err;
    }

    event.evtype = HDMI_TX_NOTIFY_DISCONNECT;
    event.arg = 0;
    ret = ioctl(g_switch.out.fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret) {
        printf("KUMSGQ_NOTIFIER_SETUP HDMI_TX_NOTIFY_DISCONNECT fail\n");
        goto err;
    }

    event.evtype = HDMI_TX_NOTIFY_EDIDREADY;
    event.arg = 0;
    ret = ioctl(g_switch.out.fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret) {
        printf("KUMSGQ_NOTIFIER_SETUP HDMI_TX_NOTIFY_EDIDREADY fail\n");
        goto err;
    }

    return 0;

err:
    hdmi_hotplug_disable();

    return -1;
}

static void *get_edid_thread(void *args)
{
    //struct dis_tvsys out_tvsys = {0};
    enum TVSYS tv_sys = 0;
    int ret = 0;
    int try_time = HDMI_SWITCH_GET_EDID_TRY_TIME;
    int i = 0;

    while(get_edid_task_run){
        sem_wait(&get_edid_thread_wait_sem);

        if(get_edid_task_run == 0){
            break;
        }

        /* check display resolution */
        i = 0;
        try_get_edid = 0;
        while(i < try_time && try_get_edid){
            ret = ioctl(g_switch.out.fd, HDMI_TX_GET_EDID_TVSYS, &tv_sys);
            if(ret == 0){
                if(g_switch.last_tv_sys != tv_sys){
                    set_hdmi_out_tvsys(tv_sys);
                }else{
                    printf("%s:%d: last_tv_sys(%d) == tv_sys(%d), not set resolution\n",
                        __func__, __LINE__, g_switch.last_tv_sys, tv_sys);
                }
                break;
            }

            printf("%s:%d: try_get_edid time=%d\n", __func__, __LINE__, i);

            i++;
            usleep(10000);

            if(get_edid_task_run == 0){
                break;
            }
        }

        try_get_edid = 0;
        if(i == try_time){
            printf("%s:%d: try_get_edid timeout, set display resolution to default \n", __func__, __LINE__);
            if(g_switch.last_tv_sys == TVSYS_INVALID){
                /* set display resolution to default value */
                set_hdmi_out_tvsys(HDMI_SWITCH_DEFAULT_DISP_RESOLUTION);
            }else{
                /* set display resolution to last value */
                set_hdmi_out_tvsys(g_switch.last_tv_sys);
            }
        }
    }

    return NULL;
}

static int hdmi_init(void)
{
    int ret = 0;
    enum TVSYS tv_sys = 0;
    //struct dis_tvsys out_tvsys = {0};

    memset(&g_switch, 0, sizeof(struct hdmi_switch));
    g_switch.in.fd = -1;
    g_switch.out.fd = -1;
    g_switch.dis_fd = -1;

    g_switch.in.fd = open("/dev/hdmi_rx" , O_RDWR);
    if( g_switch.in.fd < 0){
        printf("open /dev/hdmi_rx failed, ret=%d\n", g_switch.in.fd);
        return -1;
    }

    g_switch.out.fd = open("/dev/hdmi" , O_RDONLY);
    if( g_switch.out.fd < 0){
        printf("open /dev/hdmi_tx failed, ret=%d\n", g_switch.out.fd);
        goto err;
    }

    g_switch.dis_fd = open("/dev/dis" , O_RDWR);
    if( g_switch.dis_fd < 0){
        printf("open /dev/dis failed, ret=%d\n", g_switch.dis_fd);
        goto err;
    }

    g_switch.hdmi_out_tvsys.distype = DIS_TYPE_HD;
    //g_switch.hdmi_out_tvsys.distype = HDMI_SWITCH_BOOTLOGO_LAYER;

    g_switch.bootlogo.distype = g_switch.hdmi_out_tvsys.distype;
    g_switch.bootlogo.info.layer = HDMI_SWITCH_BOOTLOGO_LAYER;

    g_switch.hdmi_rx.distype = g_switch.hdmi_out_tvsys.distype;
    g_switch.hdmi_rx.info.layer = HDMI_SWITCH_HDMI_RX_LAYER;

    ret = get_hdmi_out_tvsys(&g_switch.hdmi_out_tvsys);
    if(ret != 0){
        printf("get_hdmi_out_tvsys failed, ret=%d\n", ret);
        goto err;
    }

    if(0){
        struct dis_hdmi_output_pix_fmt pix_fmt = {0};

        pix_fmt.distype = DIS_TYPE_HD;
        pix_fmt.pixfmt = HC_PIX_FMT_RGB_MODE1;

        ret = ioctl(g_switch.dis_fd, DIS_SET_HDMI_OUTPUT_PIXFMT, &pix_fmt);
        if(ret != 0){
            printf("DIS_SET_HDMI_OUTPUT_PIXFMT failed, ret=%d\n", ret);
            goto end;
        }
    }

    get_edid_task_run = 1;

    ret = sem_init(&get_edid_thread_wait_sem, 0, 0);
    if (ret != 0) {
        printf("err: sem_init failed!\n");
        goto end;
    }

    ret = pthread_create(&get_edid_thread_t, NULL, get_edid_thread, NULL);
    if (ret != 0) {
        printf("pthread_create failed\n");
        goto end;
    }

    /* check display resolution */
    ret = ioctl(g_switch.out.fd, HDMI_TX_GET_EDID_TVSYS, &tv_sys);
    if(ret != 0){
        printf("HDMI_TX_GET_EDID_TVSYS failed, ret=%d\n", ret);
        goto end;
    }

    printf("%s:%d: tv_sys= %d\n", __func__, __LINE__, tv_sys);
    if(g_switch.last_tv_sys != tv_sys){
        set_hdmi_out_tvsys(tv_sys);
    }else{
        printf("%s:%d: last_tv_sys(%d) == tv_sys(%d), not set resolution\n",
            __func__, __LINE__, g_switch.last_tv_sys, tv_sys);
    }
end:
    return 0;

err:

    if( g_switch.in.fd >= 0){
        close( g_switch.in.fd);
        g_switch.in.fd = -1;
    }

    if( g_switch.out.fd >= 0){
        close( g_switch.out.fd);
        g_switch.out.fd = -1;
    }

    if( g_switch.dis_fd >= 0){
        close( g_switch.dis_fd);
        g_switch.dis_fd = -1;
    }

    return -1;
}

static int hdmi_exit(int argc, char *argv[])
{
    hdmi_hotplug_disable();

	if (hdmi_in_thread_t) {
	    hdmi_in_thread_run = 0;
	    sem_post(&hdmi_in_thread_wait_sem);
	    if (pthread_join(hdmi_in_thread_t, NULL)) {
	        printf("hdmi_in_thread is not exit...\n");
	    }
		hdmi_in_thread_t = 0;
	}

	hdmi_rx_stop();

	if (get_edid_thread_t) {
		get_edid_task_run = 0;
	    sem_post(&get_edid_thread_wait_sem);
	    if (pthread_join(get_edid_thread_t, NULL)) {
	        printf("get_edid_thread is not exit...\n");
	    }
		get_edid_thread_t = 0;
	}

    if(g_switch.in.fd >= 0){
        close(g_switch.in.fd);
        g_switch.in.fd = -1;
    }

    if(g_switch.out.fd >= 0){
        close(g_switch.out.fd);
        g_switch.out.fd = -1;
    }

    if( g_switch.dis_fd >= 0){
        close(g_switch.dis_fd);
        g_switch.dis_fd = -1;
    }

    return 0;
}

static int hdmi_switch (int argc, char *argv[])
{
    int ret = 0;

    ret = hdmi_init();
    if(ret != 0){
        printf("err: hdmi_init failed\n");
        return -1;
    }

    /* enable hdmi hotplug */
    hdmi_hotplug_enable();

    hdmi_rx_start();

    return 0;
}

static struct termios stored_settings;
static void exit_console(int signo)
{
	(void)signo;
	hdmi_exit(0, 0);
	tcsetattr (0, TCSANOW, &stored_settings);
	exit(0);
}

int main(int argc, char *argv[])
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
	console_init("hdmi_switch:");

	g_switch.dis_fd = -1;
	g_switch.in.fd = -1;

	console_register_cmd(NULL, "start", hdmi_switch, CONSOLE_CMD_MODE_SELF, "start");
	console_register_cmd(NULL, "stop", hdmi_exit, CONSOLE_CMD_MODE_SELF, "stop");

	console_start();
	exit_console(0);
	(void)argc;
	(void)argv;
	return 0;
}

