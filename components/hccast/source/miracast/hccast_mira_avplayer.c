#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <hccast_com.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/dsc.h>
#include <hcuapi/snd.h>
#include <hcuapi/avsync.h>
#include <hcuapi/dis.h>
#ifdef HC_RTOS
#include <nuttx/wqueue.h>
#include <miracast/miracast_api.h>
#include <hcuapi/avevent.h>
#else
#include <sys/epoll.h>
#include <hcuapi/kumsgq.h>
#include <hccast/miracast_api.h>
#endif
#include <hccast_av.h>
#include <hccast_log.h>
#include "hccast_mira_avplayer.h"
#include "hccast_dsc.h"
#include <hccast_mira.h>

#define MIRA_EPOLL_EVENT_TYPE_KUMSG 0x1000

#define MX_EVNTS (10)
#define EPL_TOUT (1000)

struct mira_epoll_event_data
{
    int fd;
    int type;
};

static uint8_t g_vol = 50;
static unsigned int g_vfeed_cnt = 0;
static unsigned int g_vfeed_len = 0;
static unsigned int g_afeed_cnt = 0;
static unsigned char g_afeed_err_cnt = 0;
static unsigned int g_afeed_len = 0;
static unsigned int g_vfeed_pts = 0;
static int mvfd = -1;
static int mafd = -1;
static int m_snd_dev_open = -1;
static int g_mira_vdec_started = 0;
static int g_mira_kumsg_id  = -1;
static int g_mira_eplfd = -1;
static int g_mira_dis_fd = -1;
static int g_mira_event_init = 0;
static int g_mira_enable_vrotation = 0;//v_screen enable
static int g_mira_v_screen = 0;
static int m_flip_rotate = 0;
static int m_flip_mirror = 0;
static int g_audio_sync_thresh = 0;

static wfd_resolution_t g_mira_resolution = WFD_1080P30;
static pthread_t g_mira_pid = -1;
static pthread_mutex_t g_mira_mutex = PTHREAD_MUTEX_INITIALIZER;

static uint8_t g_es_dump_en = 0;
static char g_es_dump_folder[64] = {0};
static unsigned int g_es_dump_cnt = 0;
static FILE *g_es_vfp = NULL;

static struct mira_epoll_event_data g_mira_kumsg_data = { 0 };
extern hccast_mira_event_callback mira_callback;
static int g_video_first_showed = 0;

#ifdef HC_RTOS
static unsigned char *g_mira_aac_decode_buf = NULL;
static void *g_mira_audio_render = NULL;
extern void *create_aac_adts(void);
extern void aac_eld_decode_frame(void *aac_eld, unsigned char *inbuffer, int inputsize, void *outbuffer, int *outputsize);
extern void destroy_aac_eld(void *aac);
#endif

int hccast_mira_set_default_resolution(int res)
{
    g_mira_resolution = res;
    miracast_set_resolution(g_mira_resolution);
    return 0;
}

static void hccast_mira_player_state_timer(int sig)
{
    while (g_mira_vdec_started)
    {
        miracast_player_show_state();

        sleep(2);
    }
}

#define _READ(__data, __idx, __size, __shift) \
    (((__uint##__size##_t) (((const unsigned char *) (__data))[__idx])) << (__shift))

#define S16BETOS16(pin) (_READ(pin, 0, 16, 8) | _READ(pin, 1, 16, 0))

void hccast_mira_set_audio_sync_thresh()
{
    g_audio_sync_thresh = hccast_get_audio_sync_thresh();
    hccast_set_audio_sync_thresh(300);
}

void hccast_mira_restore_audio_sync_thresh()
{
    hccast_set_audio_sync_thresh(g_audio_sync_thresh);
    g_audio_sync_thresh = 0;
}

void hccast_mira_set_volume_mute(int flag)
{
#ifdef HC_RTOS
    if(flag)
    {
        hccast_set_volume(0);
    }
    else
    {
        hccast_set_volume(100);
    }
#endif    
}

int hccast_mira_get_mirror_rotation()
{
    int rotate = 0;
    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_MIRROR_ROTATION, (void *)&rotate, NULL);
    }

    return rotate;
}

int hccast_mira_get_mirror_vscreen_auto_rotation()
{
    int auto_rotate = 0;
    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_MIRROR_VSCREEN_AUTO_ROTATION, (void *)&auto_rotate, NULL);
    }

    return auto_rotate;
}

int hccast_mira_get_flip_mode()
{
    int flip_mode = 0;
    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_FLIP_MODE, (void *)&flip_mode, NULL);
    }

    return flip_mode;
}

int hccast_mira_get_mirror_full_vscreen()
{
    int full_screen = 1;
    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_MIRROR_FULL_VSCREEN, (void *)&full_screen, NULL);
    }

    return full_screen;
}

void hccast_mira_set_dis_zoom_info(hccast_mira_zoom_info_t* zoom_info)
{
    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_SET_DIS_ZOOM_INFO, (void*)zoom_info, NULL);
    }
}

void hccast_mira_reset_aspect_mode()
{
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};
    
    hccast_mira_set_dis_zoom_info(&mira_zoom_info);
    hccast_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
}

int hccast_mira_get_video_info(int *width, int *heigth)
{
#if 0    
    struct vdec_decore_status status = {0};


    if (ioctl(mvfd, VIDDEC_GET_STATUS, &status) != 0)
    {
        hccast_log(LL_ERROR, "%s %d error.\n", __func__, __LINE__);
        return -1;
    }

    if (/*status.first_pic_decoded && */status.pic_width && status.pic_height)
    {
        *width  = status.pic_width;
        *heigth = status.pic_height;
        hccast_log(LL_NOTICE, "%s width: %d, heigth: %d.\n", __func__, *width, *heigth);
    }
#else

    struct dis_display_info mpinfo = {0};
    
    hccast_get_current_pic_info(&mpinfo);
    printf("video info: rotate=%d, w:%d, h:%d\n", 
        mpinfo.info.rotate_mode, mpinfo.info.pic_dis_area.w, mpinfo.info.pic_dis_area.h);    

    if (mpinfo.info.rotate_mode == ROTATE_TYPE_90 ||
        mpinfo.info.rotate_mode == ROTATE_TYPE_270){
        *width = mpinfo.info.pic_dis_area.h;
        *heigth = mpinfo.info.pic_dis_area.w;
    } else {
        *width = mpinfo.info.pic_dis_area.w;
        *heigth = mpinfo.info.pic_dis_area.h;
    }

#endif    
    return 0;
}

#ifndef HC_RTOS
static void hccast_mira_handle_event_msg()
{
    unsigned char msg[MAX_KUMSG_SIZE] = {0};
    int len = 0;
    static unsigned long b_v_screen = 0;
    int width_ori = 1920;       //MIRACAST CAST SEND
    int width_height = 1080;    //MIRACAST CAST SEND
    int vdec_w;
    int vdec_h;
    //av_area_t src_rect = { 0, 0, 1920, 1080 };
    //av_area_t dst_rect = { 0, 0, 1920, 1080 };
    av_area_t picture_info = { 0, 0, 1920, 1080 };
    int flip_rotate = 0;
    int flip_mirror = 0;
    int full_screen = hccast_mira_get_mirror_full_vscreen();
    int flip_mode = hccast_mira_get_flip_mode();
    int vscreen_auto_rotation = hccast_mira_get_mirror_vscreen_auto_rotation();
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};

    flip_rotate = (flip_mode & 0xffff0000) >> 16;
    flip_mirror = flip_mode & 0xffff;

    len = read(g_mira_kumsg_id, msg, MAX_KUMSG_SIZE);
    if (len > 0)
    {
        KuMsgDH *msgd = (KuMsgDH *)msg;
        if (msgd->type == DIS_NOTIFY_MIRACAST_VSRCEEN)
        {
            b_v_screen = (unsigned long)msgd->params;

            if (b_v_screen)
            {
                pthread_mutex_lock(&g_mira_mutex);

                if (g_mira_vdec_started && g_mira_enable_vrotation)
                {
                    hccast_log(LL_NOTICE, "V_SCR\n");
                    g_mira_v_screen = 1;

                    if(!full_screen)
                    {
                        hccast_set_aspect_mode(DIS_TV_16_9 , DIS_VERTICALCUT , DIS_SCALE_ACTIVE_IMMEDIATELY);
                    }
                    else
                    {
                        hccast_get_miracast_picture_area(&picture_info);

                        if (hccast_mira_get_video_info(&vdec_w,  &vdec_h) < 0)
                        {
                            wfd_resolution_t res = miracast_get_resolution();
                            if ((picture_info.h <= 720) || (res == WFD_720P30) || (res == WFD_720P60))
                            {
                                width_ori = 1280;
                            }
                            else
                            {
                                width_ori = 1920;
                            }
                        }
                        else
                        {
                            width_ori = vdec_w;
                        }

                        if(vscreen_auto_rotation)
                        {
                            if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                            {
                                mira_zoom_info.src_rect.x = 0;
                                mira_zoom_info.src_rect.y = (picture_info.x * 1080) / width_ori;
                                mira_zoom_info.src_rect.w = 1920;
                                mira_zoom_info.src_rect.h = 1080 - 2 * mira_zoom_info.src_rect.y;
                                mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                                hccast_mira_set_dis_zoom_info(&mira_zoom_info);
                            }
                            else
                            {
                                mira_zoom_info.src_rect.x = (picture_info.x * 1920) / width_ori;
                                mira_zoom_info.src_rect.y = 0;
                                mira_zoom_info.src_rect.w = 1920 - 2 * mira_zoom_info.src_rect.x;
                                mira_zoom_info.src_rect.h = 1080;
                                mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                                hccast_mira_set_dis_zoom_info(&mira_zoom_info);
                            }
                        }
                        else
                        {
                            hccast_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                        
                            if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                            {
                                mira_zoom_info.src_rect.x = 0;
                                mira_zoom_info.src_rect.y = (picture_info.x * 1080) / width_ori;
                                mira_zoom_info.src_rect.w = 1920;
                                mira_zoom_info.src_rect.h = 1080 - 2 * mira_zoom_info.src_rect.y;
                                mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                                hccast_mira_set_dis_zoom_info(&mira_zoom_info);
                            }
                            else
                            {
                                mira_zoom_info.src_rect.x = (picture_info.x * 1920) / width_ori;
                                mira_zoom_info.src_rect.y = 0;
                                mira_zoom_info.src_rect.w = 1920 - 2 * mira_zoom_info.src_rect.x;
                                mira_zoom_info.src_rect.h = 1080;
                                mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                                hccast_mira_set_dis_zoom_info(&mira_zoom_info);
                            }
                        }
                    }  
                }

                pthread_mutex_unlock(&g_mira_mutex);
            }
            else
            {
                pthread_mutex_lock(&g_mira_mutex);

                if (g_mira_vdec_started && g_mira_enable_vrotation)
                {
                    hccast_log(LL_NOTICE, "H_SCR\n");
                    g_mira_v_screen = 0;
                    
                    if(!full_screen)
                    {
                        hccast_set_aspect_mode(DIS_TV_16_9 , DIS_PILLBOX , DIS_SCALE_ACTIVE_NEXTFRAME);
                    }
                    else
                    {
                        if(vscreen_auto_rotation)
                        {
                            mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                            hccast_mira_set_dis_zoom_info(&mira_zoom_info);
                        }
                        else
                        {
                            hccast_mira_reset_aspect_mode();
                        }
                    }
                }

                pthread_mutex_unlock(&g_mira_mutex);
            }
        }
        else
        {
            hccast_log(LL_WARNING, "%s other msg\n", __func__);
        }
    }
    else
    {
        hccast_log(LL_ERROR, "%s %d read error\n", __func__, __LINE__);
    }
}

static void *hccast_mira_receive_event_func(void *arg)
{
#if __linux__
    struct epoll_event events[MX_EVNTS];
    int n = -1;
    int i;
    int len;

    while (1)
    {
        n = epoll_wait(g_mira_eplfd, events, MX_EVNTS, EPL_TOUT);
        if (n == -1)
        {
            if (EINTR == errno)
            {
                continue;
            }
            usleep(100 * 1000);
            continue;
        }
        else if (n == 0)
        {
            continue;
        }

        for (i = 0; i < n; i++)
        {
            struct mira_epoll_event_data *d = (struct mira_epoll_event_data *)events[i].data.ptr;
            if (d->type == MIRA_EPOLL_EVENT_TYPE_KUMSG)
            {
                hccast_mira_handle_event_msg();
            }
        }
    }
#endif

    return NULL;
}

void hccast_mira_add_video_listen_event(int mira_fd)
{

}

void hccast_mira_delete_video_listen_event(void)
{

}

#else

static int video_key = -1;

void hccast_mira_rtos_event_video_first_showed(void *arg, unsigned long param)
{
    int mira_vdec_started = 0;
    
    pthread_mutex_lock(&g_mira_mutex);
    mira_vdec_started = g_mira_vdec_started;
    pthread_mutex_unlock(&g_mira_mutex);
    
    if(mira_vdec_started)
    {
        if (mira_callback && (g_video_first_showed == 0))
        {
            mira_callback(HCCAST_MIRA_START_FIRST_FRAME_DISP, NULL, NULL);
            hccast_mira_set_volume_mute(0);
            g_video_first_showed = 1;
        }
    }    
}

void hccast_mira_add_video_listen_event(int mira_fd)
{
    if(video_key < 0)
    {
        struct work_notifier_s de_notify = { 0 };
        
        de_notify.evtype = AVEVENT_VIDDEC_FIRST_FRAME_DECODED;
        de_notify.qid = HPWORK;
        de_notify.remote = 0;
        de_notify.oneshot = 0;
        de_notify.qualifier = mira_fd;
        de_notify.arg = NULL;
        de_notify.worker2 = hccast_mira_rtos_event_video_first_showed;

        video_key = work_notifier_setup(&de_notify);
        if (video_key < 0)
        {
            hccast_log(LL_ERROR, "%s hccast_mira_add_video_listen_event fail.\n", __func__);
        }
    }
}

void hccast_mira_delete_video_listen_event(void)
{
    if(video_key > 0)
    {
        work_notifier_teardown(video_key);
        video_key = -1;
    }
}

void hccast_mira_rtos_event_handle(void *arg, unsigned long param)
{
    static unsigned long b_v_screen = 0;
    b_v_screen = (unsigned long)param;
    int width_ori = 1920;       //MIRACAST CAST SEND
    int width_height = 1080;    //MIRACAST CAST SEND
    int vdec_w;
    int vdec_h;
    //av_area_t src_rect = { 0, 0, 1920, 1080 };
    //av_area_t dst_rect = { 0, 0, 1920, 1080 };
    av_area_t picture_info = { 0, 0, 1920, 1080 };
    int flip_rotate = 0;
    int flip_mirror = 0;
    int full_screen = hccast_mira_get_mirror_full_vscreen();
    int flip_mode = hccast_mira_get_flip_mode();
    int vscreen_auto_rotation = hccast_mira_get_mirror_vscreen_auto_rotation();
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};
    
    flip_rotate = (flip_mode & 0xffff0000) >> 16;
    flip_mirror = flip_mode & 0xffff;

    if (b_v_screen)
    {
        pthread_mutex_lock(&g_mira_mutex);

        if (g_mira_vdec_started && g_mira_enable_vrotation)
        {
            hccast_log(LL_NOTICE, "V_SCR\n");
            g_mira_v_screen = 1;
            
            if(!full_screen)
            {
                hccast_set_aspect_mode(DIS_TV_16_9 , DIS_VERTICALCUT , DIS_SCALE_ACTIVE_IMMEDIATELY);
            }
            else
            {
                hccast_get_miracast_picture_area(&picture_info);

                if (hccast_mira_get_video_info(&vdec_w,  &vdec_h) < 0)
                {
                    wfd_resolution_t res = miracast_get_resolution();
                    if ((picture_info.h <= 720) || (res == WFD_720P30) || (res == WFD_720P60))
                    {
                        width_ori = 1280;
                    }
                    else
                    {
                        width_ori = 1920;
                    }
                }
                else
                {
                    width_ori = vdec_w;
                }

                if(vscreen_auto_rotation)
                {
                    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                    {
                        mira_zoom_info.src_rect.x = 0;
                        mira_zoom_info.src_rect.y = (picture_info.x * 1080) / width_ori;
                        mira_zoom_info.src_rect.w = 1920;
                        mira_zoom_info.src_rect.h = 1080 - 2 * mira_zoom_info.src_rect.y;
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                        hccast_mira_set_dis_zoom_info(&mira_zoom_info);
                    }
                    else
                    {
                        mira_zoom_info.src_rect.x = (picture_info.x * 1920) / width_ori;
                        mira_zoom_info.src_rect.y = 0;
                        mira_zoom_info.src_rect.w = 1920 - 2 * mira_zoom_info.src_rect.x;
                        mira_zoom_info.src_rect.h = 1080;
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                        hccast_mira_set_dis_zoom_info(&mira_zoom_info);
                    }
                }
                else
                {
                    hccast_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                
                    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                    {
                        mira_zoom_info.src_rect.x = 0;
                        mira_zoom_info.src_rect.y = (picture_info.x * 1080) / width_ori;
                        mira_zoom_info.src_rect.w = 1920;
                        mira_zoom_info.src_rect.h = 1080 - 2 * mira_zoom_info.src_rect.y;
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                        hccast_mira_set_dis_zoom_info(&mira_zoom_info);
                    }
                    else
                    {
                        mira_zoom_info.src_rect.x = (picture_info.x * 1920) / width_ori;
                        mira_zoom_info.src_rect.y = 0;
                        mira_zoom_info.src_rect.w = 1920 - 2 * mira_zoom_info.src_rect.x;
                        mira_zoom_info.src_rect.h = 1080;
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                        hccast_mira_set_dis_zoom_info(&mira_zoom_info);
                    }
                }
            }          
        }

        pthread_mutex_unlock(&g_mira_mutex);
    }
    else
    {
        pthread_mutex_lock(&g_mira_mutex);

        if (g_mira_vdec_started && g_mira_enable_vrotation)
        {
            hccast_log(LL_NOTICE, "H_SCR\n");
            g_mira_v_screen = 0;

            if(!full_screen)
            {
                hccast_set_aspect_mode(DIS_TV_16_9 , DIS_PILLBOX , DIS_SCALE_ACTIVE_NEXTFRAME);
            }
            else
            {
                if(vscreen_auto_rotation)
                {
                    mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                    hccast_mira_set_dis_zoom_info(&mira_zoom_info);
                }
                else
                {
                    hccast_mira_reset_aspect_mode();
                }
            }
        }

        pthread_mutex_unlock(&g_mira_mutex);
    }
}
#endif

static void hccast_mira_set_kumsg_ctrl()
{
#ifdef __linux__
    struct epoll_event ev;
    struct kumsg_event event = { 0 };

    if (g_mira_event_init)
    {
        return ;
    }

    g_mira_eplfd = epoll_create1(0);
    if (g_mira_eplfd < 0)
    {
        hccast_log(LL_ERROR, "%s create g_mira_eplfd fail.\n", __func__);
        goto fail;
    }

    g_mira_dis_fd = open("/dev/dis", O_RDWR);
    if (g_mira_dis_fd < 0)
    {
        hccast_log(LL_ERROR, "%s create dis_fd fail.\n", __func__);
        goto fail;
    }

    g_mira_kumsg_id = ioctl(g_mira_dis_fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
    if (g_mira_kumsg_id < 0)
    {
        hccast_log(LL_ERROR, "%s can not get kumsgq\n", __func__);
        goto fail;
    }

    g_mira_kumsg_data.fd = g_mira_kumsg_id;
    g_mira_kumsg_data.type = MIRA_EPOLL_EVENT_TYPE_KUMSG;

    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.ptr = (void *)&g_mira_kumsg_data;

    if (epoll_ctl(g_mira_eplfd, EPOLL_CTL_ADD, g_mira_kumsg_id, &ev) != 0)
    {
        hccast_log(LL_ERROR, "%s epoll_ctl EPOLL_CTL_ADD error\n", __func__);
        goto fail;
    }

    event.evtype = DIS_NOTIFY_MIRACAST_VSRCEEN;
    event.arg = 0;
    if (ioctl(g_mira_dis_fd, KUMSGQ_NOTIFIER_SETUP, &event) != 0)
    {
        hccast_log(LL_ERROR, "%s KUMSGQ_NOTIFIER_SETUP error\n", __func__);
        goto fail;
    }

    if (pthread_create(&g_mira_pid, NULL, hccast_mira_receive_event_func, (void *)NULL) < 0)
    {
        hccast_log(LL_ERROR, "%s create hccast_mira_receive_event_func fail.\n", __func__);
        goto fail;
    }

    //close(dis_fd);
    g_mira_event_init = 1;
    hccast_log(LL_NOTICE, "%s hccast_mira_set_kumsg_ctrl done.\n", __func__);
    return ;

fail:
    if (g_mira_eplfd >= 0)
    {
        close(g_mira_eplfd);
        g_mira_eplfd = -1;
    }

    if (g_mira_dis_fd >= 0)
    {
        close(g_mira_dis_fd);
        g_mira_dis_fd = -1;
    }

    if (g_mira_kumsg_id >= 0)
    {
        close(g_mira_kumsg_id);
        g_mira_kumsg_id = -1;
    }

    g_mira_event_init = 0;
#else
    struct work_notifier_s vscreen_notify = { 0 };

    if (g_mira_event_init)
    {
        return ;
    }

    vscreen_notify.evtype = DIS_NOTIFY_MIRACAST_VSRCEEN;
    vscreen_notify.qid = HPWORK;
    vscreen_notify.remote = 0;
    vscreen_notify.oneshot = 0;
    vscreen_notify.qualifier = NULL;
    vscreen_notify.arg = NULL;
    vscreen_notify.worker2 = hccast_mira_rtos_event_handle;

    if (work_notifier_setup(&vscreen_notify) < 0)
    {
        hccast_log(LL_ERROR, "%s work_notifier_setup fail.\n", __func__);
    }
    else
    {
        g_mira_event_init = 1;
    }
    
#endif
}

void hccast_mira_vscreen_detect_enable(int enable)
{
    struct dis_miracast_vscreen_detect_param mpara = { 0 };
    int fd = -1;

    fd = open("/dev/dis", O_RDWR);
    if (fd < 0)
    {
        return ;
    }

    mpara.distype = DIS_TYPE_HD;
    if (enable)
    {
        mpara.on = 1;
    }
    else
    {
        mpara.on = 0;
    }

    ioctl(fd, DIS_SET_MIRACAST_VSRCEEN_DETECT, &mpara);

    close(fd);
}

int hccast_mira_process_rotation_change()
{
    int temp = 0;
    av_area_t rect = {0, 0, 1920, 1080};
    int rotate = 0;
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_NEXTFRAME};
    
    int flip_mode = hccast_mira_get_flip_mode();
    m_flip_rotate = (flip_mode & 0xffff0000) >> 16;
    m_flip_mirror = flip_mode & 0xffff;

    rotate = hccast_mira_get_mirror_rotation();
    if((rotate == HCCAST_MIRA_SCREEN_ROTATE_90) || (rotate == HCCAST_MIRA_SCREEN_ROTATE_270))
    {
        temp = 1;
    }
    else
    {
        temp = 0;
    }

    //for avoid every time will call back the hccast_mira_vscreen_detect_enable.
    if (temp != g_mira_enable_vrotation)
    {
        if (temp)
        {
            hccast_mira_vscreen_detect_enable(1);
            g_mira_enable_vrotation = 1;
        }
        else
        {
            hccast_mira_vscreen_detect_enable(0);
            hccast_mira_set_dis_zoom_info(&mira_zoom_info);
            g_mira_enable_vrotation = 0;
            g_mira_v_screen = 0;
        }
    }

    return rotate;
}

int hccast_mira_es_dump_start(char *folder)
{
    memset(g_es_dump_folder, 0, 64);
    strcat(g_es_dump_folder, folder);
    g_es_dump_en = 1;

    return 0;
}

int hccast_mira_es_dump_stop()
{
    g_es_dump_en = 0;

    return 0;
}

static int hccast_mira_video_open(void)
{
    pthread_t tid;
    struct video_config mvcfg;
    char path[128] = {0};
    int rotate = 0;

    wfd_resolution_t res = miracast_get_resolution();

    hccast_log(LL_NOTICE, "miracast resolution type: %d\n", res);
    if (mvfd >= 0)
    {
        close(mvfd);
        mvfd = -1;
    }

    memset(&mvcfg, 0, sizeof(struct video_config));

    mvcfg.codec_id = HC_AVCODEC_ID_H264;
    mvcfg.sync_mode = AVSYNC_TYPE_UPDATESTC;
    mvcfg.quick_mode = 3;
    hccast_log(LL_INFO, "video sync_mode: %d\n", mvcfg.sync_mode);
    mvcfg.decode_mode = VDEC_WORK_MODE_KSHM;

    if (WFD_1080P60 == res)
    {
        mvcfg.pic_width = 1920;
        mvcfg.pic_height = 1080;
        mvcfg.frame_rate = 60 * 1000;
    }
    else if (WFD_1080P30 == res)
    {
        mvcfg.pic_width = 1920;
        mvcfg.pic_height = 1080;
        mvcfg.frame_rate = 60 * 1000;
    }
    else if (WFD_720P60 == res)
    {
        mvcfg.pic_width = 1280;
        mvcfg.pic_height = 720;
        mvcfg.frame_rate = 60 * 1000;
    }
    else if (WFD_720P30 == res)
    {
        mvcfg.pic_width = 1280;
        mvcfg.pic_height = 720;
        mvcfg.frame_rate = 30 * 1000;
    }

    mvcfg.pixel_aspect_x = 1;
    mvcfg.pixel_aspect_y = 1;
    mvcfg.preview = 0;
    mvcfg.extradata_size = 0;

    mvcfg.rotate_enable = 1;//default enable.

    int flip_mode = hccast_mira_get_flip_mode();
    m_flip_rotate = (flip_mode & 0xffff0000) >> 16;
    m_flip_mirror = flip_mode & 0xffff;

    rotate = hccast_mira_get_mirror_rotation();
    if ((rotate== HCCAST_MIRA_SCREEN_ROTATE_90) || (rotate== HCCAST_MIRA_SCREEN_ROTATE_270))
    {
        hccast_mira_vscreen_detect_enable(1);
        g_mira_enable_vrotation = 1;
    }
    else
    {
        hccast_mira_vscreen_detect_enable(0);
        g_mira_enable_vrotation = 0;
    }

    mvfd = open("/dev/viddec", O_RDWR);
    if (mvfd < 0)
    {
        hccast_log(LL_ERROR, "Open /dev/viddec error!\n");

        pthread_mutex_lock(&g_mira_mutex);
        g_mira_vdec_started = 0;
        pthread_mutex_unlock(&g_mira_mutex);

        return -1;
    }

    if (ioctl(mvfd, VIDDEC_INIT, &mvcfg) != 0)
    {
        hccast_log(LL_ERROR, "Init viddec error!\n");
        close(mvfd);
        mvfd = -1;

        pthread_mutex_lock(&g_mira_mutex);
        g_mira_vdec_started = 0;
        pthread_mutex_unlock(&g_mira_mutex);

        return -1;
    }

    ioctl(mvfd, VIDDEC_START, 0);
    hccast_log(LL_DEBUG,  "mvfd: %d\n", mvfd);
    hccast_mira_set_audio_sync_thresh();

    pthread_mutex_lock(&g_mira_mutex);
    g_mira_vdec_started = 1;
    g_mira_v_screen = 0;
    g_video_first_showed = 0;
    pthread_mutex_unlock(&g_mira_mutex);

    hccast_mira_set_kumsg_ctrl();
    hccast_mira_add_video_listen_event(mvfd);
    if(hccast_mira_get_mirror_vscreen_auto_rotation())
    {
        hccast_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
    }    
    pthread_create(&tid, NULL, hccast_mira_player_state_timer, NULL);

    if (g_es_dump_en)
    {
        sprintf(path, "%s/miracast-%d.h264", g_es_dump_folder, g_es_dump_cnt);
        g_es_dump_cnt ++;

        g_es_vfp = fopen(path, "w+");
    }

    g_vfeed_pts = 0;

    return 0;
}

static void hccast_mira_video_close(void)
{
    hccast_log(LL_INFO, "func: %s\n", __func__);

    if (g_es_vfp)
    {
        fflush(g_es_vfp);
        fclose(g_es_vfp);
        g_es_vfp = NULL;
    }

    if (mvfd > 0)
    {
        close(mvfd);
        mvfd = -1;
    }

    hccast_mira_restore_audio_sync_thresh();
    pthread_mutex_lock(&g_mira_mutex);
    hccast_mira_vscreen_detect_enable(0);
    hccast_mira_delete_video_listen_event();
    g_mira_vdec_started = 0;
    g_mira_v_screen = 0;
    g_mira_enable_vrotation = 0;
    hccast_mira_reset_aspect_mode();
    //hccast_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
    pthread_mutex_unlock(&g_mira_mutex);
}

static int hccast_mira_get_rotate_mode(int flip_mode, int rotation)
{
    int flip_mode_0[4] = {ROTATE_TYPE_0, ROTATE_TYPE_270, ROTATE_TYPE_90, ROTATE_TYPE_180};
    int flip_mode_90[4] = {ROTATE_TYPE_90, ROTATE_TYPE_0, ROTATE_TYPE_180, ROTATE_TYPE_270};
    int flip_mode_180[4] = {ROTATE_TYPE_180, ROTATE_TYPE_90, ROTATE_TYPE_270, ROTATE_TYPE_0};
    int flip_mode_270[4] = {ROTATE_TYPE_270, ROTATE_TYPE_180, ROTATE_TYPE_0, ROTATE_TYPE_90};
    
    if (ROTATE_TYPE_0 == flip_mode)
    {
        return flip_mode_0[rotation];
    }
    else if (ROTATE_TYPE_90 == flip_mode)
    {
        return flip_mode_90[rotation];
    }
    else if (ROTATE_TYPE_180 == flip_mode)
    {
        return flip_mode_180[rotation];
    }
    else if (ROTATE_TYPE_270 == flip_mode)
    {
        return flip_mode_270[rotation];
    }
    
    return 0;
}

static int hccast_mira_video_feed(unsigned char *data, unsigned int len, unsigned int pts,
                                  int last_slice, unsigned int width, unsigned int height)
{
    float rate = 1;
    int temp = 0;
    int rotate = 0;

    g_vfeed_cnt ++;
    g_vfeed_len += len;

    if (g_es_vfp)
        fwrite(data, 1, len, g_es_vfp);

    if (mvfd < 0)
    {
        hccast_log(LL_ERROR, "open video error!\n");
        return -1;
    }

    //ADD AUD NALU to decode last frame quickly
    data[len] = 0x0;
    data[len + 1] = 0x0;
    data[len + 2] = 0x0;
    data[len + 3] = 0x1;
    data[len + 4] = 0x9;
    len = len + 5;

    AvPktHd pkthd = { 0 };
    pkthd.pts = pts;
    pkthd.dur = 0;
    pkthd.size = len;
    pkthd.flag = AV_PACKET_ES_DATA;

    pthread_mutex_lock(&g_mira_mutex);
    rotate = hccast_mira_process_rotation_change();

    if (g_mira_enable_vrotation)
    {
        if (g_mira_v_screen)
        {
            pkthd.video_rotate_mode = hccast_mira_get_rotate_mode(m_flip_rotate, rotate);
        }
        else
        {
            if(hccast_mira_get_mirror_vscreen_auto_rotation())
            {
                pkthd.video_rotate_mode = m_flip_rotate;
            }
            else
            {
                pkthd.video_rotate_mode = hccast_mira_get_rotate_mode(m_flip_rotate, rotate);
            }    
        }
    }
    else
    {
        pkthd.video_rotate_mode = hccast_mira_get_rotate_mode(m_flip_rotate, rotate);
    }
    pkthd.video_mirror_mode = m_flip_mirror;

    pthread_mutex_unlock(&g_mira_mutex);

    if (pts)
    {
        g_vfeed_pts = pts;
    }

    if (write(mvfd, (uint8_t *)&pkthd, sizeof(AvPktHd)) != sizeof(AvPktHd))
    {
        hccast_log(LL_ERROR,  "Write AvPktHd fail!\n");
        goto fail;
    }
    if (write(mvfd, data, len) != len)
    {
        hccast_log(LL_ERROR, "Write video_frame error fail!\n");
        goto fail;
    }
    return 0;

fail:
    ioctl(mvfd, VIDDEC_FLUSH, &rate);
    return 0;
}

static int ___codec_tag;

static int _hccast_mira_audio_open_auddec()
{
    struct audio_config macfg;

    if (mafd >= 0)
    {
        close(mafd);
        mafd = -1;
    }

    memset(&macfg, 0, sizeof(struct audio_config));
    macfg.codec_id = HC_AVCODEC_ID_AAC;
    macfg.sync_mode = AVSYNC_TYPE_SYNCSTC;
    macfg.bits_per_coded_sample = 16;
    macfg.channels = 2;
    macfg.bit_rate = 0;
    macfg.sample_rate = 48000;
    macfg.block_align = 0;
    macfg.extradata_size = 0;
    macfg.snd_devs = hccast_com_media_control(HCCAST_CMD_SND_DEVS_GET, 0);

    mafd = open("/dev/auddec", O_RDWR);
    if (mafd < 0)
    {
        hccast_log(LL_ERROR, "Open /dev/auddec error!\n");
        return -1;
    }
    if (ioctl(mafd, AUDDEC_INIT, &macfg) != 0)
    {
        hccast_log(LL_ERROR, "Init auddec error!\n");
        close(mafd);
        mafd = -1;
        return -1;
    }
    ioctl(mafd, AUDDEC_START, 0);

    return 0;
}

static int _hccast_mira_audio_close_auddec()
{
    if (mafd >= 0)
    {
        close(mafd);
        mafd = -1;
    }

    return 0;
}

static int _hccast_mira_audio_feed_auddec(unsigned char *buf, int length, unsigned int pts)
{
    AvPktHd pkthd = { 0 };

    if (mafd < 0)
    {
        hccast_log(LL_ERROR, "audio not open!\n");
        return -1;
    }

    pkthd.pts = pts;
    pkthd.dur = 0;
    pkthd.size = length;
    pkthd.flag = AV_PACKET_ES_DATA;
    if (write(mafd, (uint8_t *)&pkthd, sizeof(AvPktHd)) != sizeof(AvPktHd))
    {
        hccast_log(LL_ERROR, "Write AvPktHd fail!\n");
        goto fail;
    }

    if (write(mafd, buf, length) != length)
    {
        hccast_log(LL_ERROR, "Write audio_frame error fail!\n");
        goto fail;
    }

    return length;

fail:
    ioctl(mafd, AUDDEC_FLUSH, 0);
    return -1;
}

static int _hccast_mira_audio_open_i2so()
{
    hccast_snd_dev_close();

    m_snd_dev_open = hccast_snd_dev_open(2, 16, 48000, SND_PCM_FORMAT_S16_LE);
    if (m_snd_dev_open < 0)
    {
        hccast_log(LL_ERROR, "hccast_snd_dev_open error!\n");
        return -1;
    }

    return 0;
}

static int _hccast_mira_audio_close_i2so()
{
    hccast_snd_dev_close();
    m_snd_dev_open = -1;

    return 0;
}

static int _hccast_mira_audio_feed_i2so(unsigned char *buf, int length, unsigned int pts)
{
    if (m_snd_dev_open < 0)
    {
        return -1;
    }

    if (length != 1920)
    {
        hccast_log(LL_WARNING, "skip pcm %d\n", length);
        return -1;
    }

    unsigned short *in, *out ;
    out = (unsigned short *)buf;
    in = (unsigned short *)buf;
    for (int i = 0; i < length / 2; i++)
    {
        *out = S16BETOS16(in);
        out++;
        in++;
    }

    hccast_snd_dev_feed(buf, length, pts);

    return 0;
}

static int hccast_mira_audio_open(int codec_tag)
{
    hccast_log(LL_INFO,  "func: %s, codec_tag: %d.\n", __func__, codec_tag);

    ___codec_tag = codec_tag;

    if (___codec_tag == CODEC_ID_PCM_S16BE)
    {
        _hccast_mira_audio_open_i2so();
    }
    else
    {
        _hccast_mira_audio_open_auddec();
    }

    g_vol = hccast_get_volume();
    hccast_log(LL_NOTICE, "set vol %d->100\n", g_vol);
    hccast_set_volume(100);
    hccast_mira_set_volume_mute(1);

    hccast_log(LL_DEBUG, "mafd: %d.\n", mafd);
    return 0;
}

static void hccast_mira_audio_close()
{
    hccast_set_volume(g_vol);
    
    if (___codec_tag == CODEC_ID_PCM_S16BE)
    {
        _hccast_mira_audio_close_i2so();
    }
    else
    {
        _hccast_mira_audio_close_auddec();
    }
}

int hccast_mira_audio_feed(int type, unsigned char *buf, int length, unsigned int pts)
{
    int ret = -1;

    g_afeed_cnt ++;
    g_afeed_len += length;

    if (___codec_tag == CODEC_ID_PCM_S16BE)
    {
        ret = _hccast_mira_audio_feed_i2so(buf, length, pts);
    }
    else
    {
        ret = _hccast_mira_audio_feed_auddec(buf, length, pts);
    }

    return ret;
}

void hccast_mira_av_state(char *s)
{
    int64_t vpts = 0;

    if (mvfd >= 0)
    {
        ioctl(mvfd, VIDDEC_GET_CUR_TIME, &vpts);
    }

    hccast_log(LL_NOTICE, "[FEED] V(%d:%d) A(%d:%d) P(%.8x:%.8x) %s\n",
               g_vfeed_cnt, g_vfeed_len, g_afeed_cnt, g_afeed_len, g_vfeed_pts, (unsigned int)vpts, s);
    g_vfeed_cnt = 0;
    g_vfeed_len = 0;
    g_afeed_cnt = 0;
    g_afeed_len = 0;
}

void hccast_mira_av_reset()
{
}

static miracast_av_func_t hccast_mira_av_driver =
{
    ._video_open = hccast_mira_video_open,
    ._video_close = hccast_mira_video_close,
    ._video_feed = hccast_mira_video_feed,
    ._audio_open = hccast_mira_audio_open,
    ._audio_close = hccast_mira_audio_close,
    ._audio_feed = hccast_mira_audio_feed,
    ._av_state = hccast_mira_av_state,
    ._av_reset = hccast_mira_av_reset,
};

static miracast_dsc_func_t hccast_mira_dsc_driver =
{
    .dsc_aes_ctr_open = hccast_dsc_aes_ctr_open,
    .dsc_aes_cbc_open = hccast_dsc_aes_cbc_open,
    .dsc_ctx_destroy  = hccast_dsc_ctx_destroy,
    .dsc_aes_decrypt  = hccast_dsc_decrypt,
    .dsc_aes_encrypt  = hccast_dsc_encrypt,
};

int mira_av_player_init(void)
{
    miracast_set_resolution(g_mira_resolution);

    miracast_ioctl(WFD_CMD_SET_DSC_FUNC, (unsigned long)&hccast_mira_dsc_driver, (unsigned long)0);
    miracast_ioctl(WFD_CMD_SET_AV_FUNC, (unsigned long)&hccast_mira_av_driver, (unsigned long)0);

#ifdef HC_RTOS
    miracast_ioctl(WFD_CMD_ENABLE_AAC, 0, 0);
#endif
    //miracast_ioctl(WFD_CMD_DISABLE_AUDIO, 1, 0);

    return 0;
}
