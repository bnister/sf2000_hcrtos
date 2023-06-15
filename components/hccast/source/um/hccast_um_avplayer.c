#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/snd.h>
#include <hcuapi/avsync.h>
#include <hcuapi/dis.h>
#ifdef HC_RTOS
#include <um/iumirror_api.h>
#include <um/aumirror_api.h>
#else
#include <hccast/iumirror_api.h>
#include <hccast/aumirror_api.h>
#endif

#include <hccast_com.h>

#include <hccast_um.h>
#include "hccast_um_avplayer.h"
#include "hccast_um_api.h"

#define UM_AV_DEBUG printf

static aum_screen_mode_t g_aum_screen_mode;
static unsigned int g_um_type = 0;
static unsigned int g_video_width = 0;
static unsigned int g_video_height = 0;
static unsigned char g_play_mode = 0;
static int g_rotate_mode = 0xFF;
static int g_vdec_started = 0;
static int g_audio_started = 0;
static int g_video_decoder = -1;
static int g_audio_decoder = -1;
static int g_avsync_dev = -1;
static unsigned int g_vfeed_cnt = 0;
static unsigned int g_vfeed_len = 0;
static unsigned int g_afeed_cnt = 0;
static unsigned int g_afeed_len = 0;
static int m_flip_rotate = 0;
static int m_flip_mirror = 0;

static uint8_t g_es_dump_en = 0;
static char g_es_dump_folder[64] = {0};
static unsigned int g_es_dump_cnt = 0;
static FILE *g_es_vfp = NULL;
static int g_ium_vd_dis_mode = DIS_PILLBOX;
static int g_um_enable_rotate = 0;
static pthread_mutex_t g_um_mutex = PTHREAD_MUTEX_INITIALIZER;

extern um_ioctl ium_api_ioctl;
extern hccast_um_cb g_ium_evt_cb;
extern hccast_um_cb g_aum_evt_cb;

static void *hccast_um_player_state_timer(void *args)
{
    while (g_vdec_started)
    {
        UM_AV_DEBUG("[FEED] V(%d:%d) A(%d:%d)\n", g_vfeed_cnt, g_vfeed_len, g_afeed_cnt, g_afeed_len);
        g_vfeed_cnt = 0;
        g_vfeed_len = 0;
        g_afeed_cnt = 0;
        g_afeed_len = 0;
        sleep(2);
    }
}

static void hccast_um_video_dis_mode_change(int expect_vd_dis_mode)
{
    if(g_um_type == UM_TYPE_IUM)
    {
        if (expect_vd_dis_mode !=  g_ium_vd_dis_mode)
        {
            g_ium_vd_dis_mode = expect_vd_dis_mode;
            hccast_set_aspect_mode(DIS_TV_16_9, expect_vd_dis_mode, DIS_SCALE_ACTIVE_IMMEDIATELY);
        }
    }
    else
    {
        hccast_um_zoom_info_t um_zoom_info = {{0, 0, 1920, 1080},{0, 0, 1920, 1080}};
        float ratio;

        pthread_mutex_lock(&g_um_mutex);
        hccast_um_param_t *um_param = hccast_um_param_get();
        
        if(g_um_enable_rotate != um_param->screen_rotate_en)
        {
            if((um_param->full_screen_en) && (um_param->screen_rotate_en == 0))
            {
                g_aum_evt_cb(HCCAST_AUM_EVT_SET_DIS_ZOOM_INFO, (void*)&um_zoom_info, NULL);
            }
            else if((um_param->full_screen_en) && um_param->screen_rotate_en)
            {
                 ratio = (float)g_aum_screen_mode.screen_height / (float)g_aum_screen_mode.screen_width;
                if (!g_aum_screen_mode.mode && (ratio > 1.8) && (g_aum_screen_mode.screen_height > 1920))
                {
                    um_zoom_info.src_rect.h = (1080 * 1920) / g_aum_screen_mode.screen_height;
                    um_zoom_info.src_rect.y = (1080 - um_zoom_info.src_rect.h) / 2;
                    g_aum_evt_cb(HCCAST_AUM_EVT_SET_DIS_ZOOM_INFO, (void*)&um_zoom_info, NULL);
                }
                else
                {
                    g_aum_evt_cb(HCCAST_AUM_EVT_SET_DIS_ZOOM_INFO, (void*)&um_zoom_info, NULL);
                }
            }

            g_um_enable_rotate = um_param->screen_rotate_en;
        }
        pthread_mutex_unlock(&g_um_mutex);
    }
}

static rotate_type_e hccast_um_video_rotate_get()
{
    rotate_type_e rotate_angle = 0;
    int expect_vd_dis_mode = 0;
    hccast_um_param_t *um_param = hccast_um_param_get();

    if (!um_param->screen_rotate_en)
    {
        if (UM_TYPE_AUM == g_um_type)
        {
            rotate_angle = ROTATE_TYPE_0;
        }
        else
        {
            switch (g_rotate_mode)
            {
                case UM_SCREEN_ROTATE_0:
                    rotate_angle = ROTATE_TYPE_0;
                    break;
                case UM_SCREEN_ROTATE_90:
                    rotate_angle = ROTATE_TYPE_90;
                    break;
                case UM_SCREEN_ROTATE_180:
                    rotate_angle = ROTATE_TYPE_180;
                    break;
                case UM_SCREEN_ROTATE_270:
                    rotate_angle = ROTATE_TYPE_270;
                    break;
                default:
                    rotate_angle = ROTATE_TYPE_0;
                    break;
            }
            
            expect_vd_dis_mode = DIS_PILLBOX;
        }
    }
    else
    {
        if (UM_TYPE_AUM == g_um_type)
        {
            if (um_param->screen_rotate_auto && g_aum_screen_mode.mode)
            {
                rotate_angle = ROTATE_TYPE_0;
            }
            else
            {
                rotate_angle = ROTATE_TYPE_270;
            }
        }
        else
        {
            switch (g_rotate_mode)
            {
                case UM_SCREEN_ROTATE_0:
                    if (g_video_width > g_video_height)
                    {
                        rotate_angle = ROTATE_TYPE_0;
                        expect_vd_dis_mode = DIS_PILLBOX;
                    }    
                    else
                    {
                        rotate_angle = ROTATE_TYPE_270;
                        expect_vd_dis_mode = DIS_NORMAL_SCALE;
                    }    
                    break;
                case UM_SCREEN_ROTATE_90:
                    rotate_angle = ROTATE_TYPE_0;
                    expect_vd_dis_mode = DIS_PILLBOX;
                    break;
                case UM_SCREEN_ROTATE_180:
                    rotate_angle = ROTATE_TYPE_90;
                    expect_vd_dis_mode = DIS_NORMAL_SCALE;
                    break;
                case UM_SCREEN_ROTATE_270:
                    rotate_angle = ROTATE_TYPE_180;
                    expect_vd_dis_mode = DIS_PILLBOX;
                    break;
                default:
                    rotate_angle = ROTATE_TYPE_0;
                    expect_vd_dis_mode = DIS_PILLBOX;
                    break;
            }
        }
    }
    
    hccast_um_video_dis_mode_change(expect_vd_dis_mode);
    
    return rotate_angle;
}

static void hccast_um_video_aspect_set()
{
    pthread_mutex_lock(&g_um_mutex);
    
    hccast_um_param_t *um_param = hccast_um_param_get();
    hccast_um_zoom_info_t um_zoom_info = {{0, 0, 1920, 1080},{0, 0, 1920, 1080}};
    float ratio;

    if (um_param->full_screen_en && um_param->screen_rotate_en)
    {
        if (UM_TYPE_AUM == g_um_type)
        {
            hccast_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);

            ratio = (float)g_aum_screen_mode.screen_height / (float)g_aum_screen_mode.screen_width;
            if (!g_aum_screen_mode.mode && (ratio > 1.8) && (g_aum_screen_mode.screen_height > 1920))
            {
                um_zoom_info.src_rect.h = (1080 * 1920) / g_aum_screen_mode.screen_height;
                um_zoom_info.src_rect.y = (1080 - um_zoom_info.src_rect.h) / 2;
                g_aum_evt_cb(HCCAST_AUM_EVT_SET_DIS_ZOOM_INFO, (void*)&um_zoom_info, NULL);
            }
            else
            {
                g_aum_evt_cb(HCCAST_AUM_EVT_SET_DIS_ZOOM_INFO, (void*)&um_zoom_info, NULL);
            }
        }
        else
        {
            g_ium_evt_cb(HCCAST_IUM_EVT_SET_DIS_ZOOM_INFO, (void*)&um_zoom_info, NULL);
            hccast_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
            g_ium_vd_dis_mode = DIS_NORMAL_SCALE;
        }
    }
    else
    {
        if (UM_TYPE_AUM == g_um_type)
        {
            g_aum_evt_cb(HCCAST_AUM_EVT_SET_DIS_ZOOM_INFO, (void*)&um_zoom_info, NULL);
        }
        else
        {
            g_ium_evt_cb(HCCAST_IUM_EVT_SET_DIS_ZOOM_INFO, (void*)&um_zoom_info, NULL);
        }
        hccast_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
        g_ium_vd_dis_mode = DIS_PILLBOX;
    }

    g_um_enable_rotate = um_param->screen_rotate_en;
    pthread_mutex_unlock(&g_um_mutex);
    
}


static void hccast_um_video_restart(int play_mode)
{
    struct video_config mvcfg;

    UM_AV_DEBUG("[%s - %d]\n", __func__, __LINE__);

    if (g_video_decoder >= 0)
    {
        close(g_video_decoder);
        g_video_decoder = -1;
    }

    memset(&mvcfg, 0, sizeof(struct video_config));
    mvcfg.codec_id = HC_AVCODEC_ID_H264;
    mvcfg.decode_mode = VDEC_WORK_MODE_KSHM;
    mvcfg.pic_width = 1920;
    mvcfg.pic_height = 1080;
    mvcfg.frame_rate = 60 * 1000;
    mvcfg.pixel_aspect_x = 1;
    mvcfg.pixel_aspect_y = 1;
    mvcfg.preview = 0;
    mvcfg.extradata_size = 0;
    mvcfg.rotate_enable = 1;
    if (UM_PLAY_MODE_STREAM == play_mode)
    {
        mvcfg.sync_mode = AVSYNC_TYPE_SYNCSTC;
        mvcfg.buffering_start = 0;
        mvcfg.buffering_end = 1000;
    }
    else
    {
        mvcfg.sync_mode = AVSYNC_TYPE_UPDATESTC;
        mvcfg.quick_mode = 3;
    }

    g_video_decoder = open("/dev/viddec", O_RDWR);
    if (g_video_decoder < 0)
    {
        perror("Open viddec");
        return;
    }
    if (ioctl(g_video_decoder, VIDDEC_INIT, &mvcfg) != 0)
    {
        perror("Viddec init");
        close(g_video_decoder);
        g_video_decoder = -1;
        return;
    }
    ioctl(g_video_decoder, VIDDEC_START, 0);
}

static void hccast_um_audio_restart(int play_mode)
{
    struct audio_config macfg;
    uint8_t g_vol = 0;

    UM_AV_DEBUG("[%s - %d]\n", __func__, __LINE__);

    if (g_audio_decoder >= 0)
    {
        close(g_audio_decoder);
        g_audio_decoder = -1;
    }

    memset(&macfg, 0, sizeof(struct audio_config));
    macfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;
    macfg.bits_per_coded_sample = 16;
    macfg.channels = 2;
    macfg.bit_rate = 0;
    macfg.sample_rate = 48000;
    macfg.block_align = 0;
    macfg.extradata_size = 0;
    if (UM_PLAY_MODE_STREAM == play_mode)
    {
        macfg.sync_mode = AVSYNC_TYPE_FREERUN;
    }
    else
    {
        macfg.sync_mode = AVSYNC_TYPE_FREERUN;
        macfg.audio_flush_thres = 300;
    }

    macfg.snd_devs = hccast_com_media_control(HCCAST_CMD_SND_DEVS_GET, 0);

    g_audio_decoder = open("/dev/auddec", O_RDWR);
    if (g_audio_decoder < 0)
    {
        perror("Open auddec");
        return -1;
    }

    if (ioctl(g_audio_decoder, AUDDEC_INIT, &macfg) != 0)
    {
        perror("Init auddec");
        close(g_audio_decoder);
        g_audio_decoder = -1;
        return -1;
    }
    ioctl(g_audio_decoder, AUDDEC_START, 0);

    int snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0)
    {
        perror("Open sndC0i2so");
        g_audio_started = 0;
        return -1;
    }
    g_vol = 100;
    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &g_vol);
    close(snd_fd);
}

extern int hccast_aum_get_flip_mode();
extern int hccast_ium_get_flip_mode();
int hccast_um_get_flip_mode(int um_type)
{
    int flip_mode = 0;

    if (UM_TYPE_AUM == um_type)
    {
        flip_mode = hccast_aum_get_flip_mode();
    }
    else
    {
        flip_mode = hccast_ium_get_flip_mode();
    }

    return flip_mode;
}

int hccast_um_es_dump_start(char *folder)
{
    memset(g_es_dump_folder, 0, 64);
    strcat(g_es_dump_folder, folder);
    g_es_dump_en = 1;

    return 0;
}

int hccast_um_es_dump_stop()
{
    g_es_dump_en = 0;

    return 0;
}

int hccast_um_video_open(int um_type)
{
    struct video_config mvcfg;
    pthread_t tid;
    char path[128] = {0};

    UM_AV_DEBUG("[%s - %d]\n", __func__, __LINE__);

    if (g_vdec_started)
    {
        UM_AV_DEBUG("Warning: aircast video has been started!\n");
    }

    g_um_type = um_type;

    if (g_video_decoder >= 0)
    {
        close(g_video_decoder);
        g_video_decoder = -1;
    }

    int flip_mode = hccast_um_get_flip_mode(g_um_type);
    m_flip_rotate = (flip_mode & 0xffff0000) >> 16;
    m_flip_mirror = flip_mode & 0xffff;

    g_video_width = 0;
    g_video_height = 0;

    memset(&mvcfg, 0, sizeof(struct video_config));
    mvcfg.codec_id = HC_AVCODEC_ID_H264;
    mvcfg.sync_mode = AVSYNC_TYPE_UPDATESTC;
    mvcfg.quick_mode = 3;
    mvcfg.decode_mode = VDEC_WORK_MODE_KSHM;
    mvcfg.pic_width = 1920;
    mvcfg.pic_height = 1080;
    mvcfg.frame_rate = 60 * 1000;
    mvcfg.pixel_aspect_x = 1;
    mvcfg.pixel_aspect_y = 1;
    mvcfg.preview = 0;
    mvcfg.extradata_size = 0;
    mvcfg.rotate_enable = 1;

    g_video_decoder = open("/dev/viddec", O_RDWR);
    if (g_video_decoder < 0)
    {
        perror("Open viddec");
        return -1;
    }
    if (ioctl(g_video_decoder, VIDDEC_INIT, &mvcfg) != 0)
    {
        perror("Viddec init");
        close(g_video_decoder);
        g_video_decoder = -1;
        return -1;
    }
    ioctl(g_video_decoder, VIDDEC_START, 0);

    hccast_um_video_aspect_set();

    g_vdec_started = 1;

    pthread_create(&tid, NULL, hccast_um_player_state_timer, NULL);

    if (g_es_dump_en)
    {
        sprintf(path, "%s/um-%d.h264", g_es_dump_folder, g_es_dump_cnt);
        g_es_dump_cnt ++;

        g_es_vfp = fopen(path, "w+");
    }

    return 0;
}

void hccast_um_video_close()
{
    UM_AV_DEBUG("[%s - %d]\n", __func__, __LINE__);

    if (g_es_vfp)
    {
        fflush(g_es_vfp);
        fclose(g_es_vfp);
        g_es_vfp = NULL;
    }

    if (!g_vdec_started)
    {
        UM_AV_DEBUG("[%s - %d] video has been close.\n", __func__, __LINE__);
        return ;
    }

    if (g_video_decoder > 0)
    {
        close(g_video_decoder);
        g_video_decoder = -1;
    }

    memset(&g_aum_screen_mode, 0, sizeof(g_aum_screen_mode));
    g_vdec_started = 0;
    g_video_width = 0;
    g_video_height = 0;
    g_rotate_mode = 0xFF;
}

static int hccast_um_h264_decode(uint8_t *data, size_t len, unsigned int pts, int rotate)
{
    float rate = 1;
    rotate_type_e rotate_angle = 0;
    AvPktHd pkthd = { 0 };
    int flip_mode = hccast_um_get_flip_mode(g_um_type);

    m_flip_rotate = (flip_mode & 0xffff0000) >> 16;
    m_flip_mirror = flip_mode & 0xffff;

    pkthd.pts = pts;
    pkthd.dur = 0;
    pkthd.size = len;
    pkthd.flag = AV_PACKET_ES_DATA;

    rotate_angle = hccast_um_video_rotate_get();
    pkthd.video_rotate_mode = m_flip_rotate;

    //pkthd.video_rotate_mode = hccast_um_video_rotate_get();
    rotate_angle = (rotate_angle + m_flip_rotate) % 4;
    pkthd.video_rotate_mode = rotate_angle;
    pkthd.video_mirror_mode = m_flip_mirror;

    if (write(g_video_decoder, (uint8_t *)&pkthd, sizeof(AvPktHd)) != sizeof(AvPktHd))
    {
        perror("Write AvPktHd");
        goto fail;
    }
    if (write(g_video_decoder, data, len) != len)
    {
        perror("Write vFrame");
        goto fail;
    }
    return 0;

fail:
    ioctl(g_video_decoder, VIDDEC_FLUSH, &rate);

    return -1;
}

int hccast_um_video_feed(unsigned char *data, unsigned int len,
                         unsigned long long pts, unsigned int rotate, int last_slice,
                         unsigned int width, unsigned int height, unsigned char play_mode)
{
    int need_restart_decoder = 0;
    int resolution_changed = 0;
    float rate = 1;
    rotate_type_e rotate_angle = 0;
    AvPktHd pkthd = {0};
    unsigned char data_aux[5];


    int flip_mode = hccast_um_get_flip_mode(g_um_type);
    m_flip_rotate = (flip_mode & 0xffff0000) >> 16;
    m_flip_mirror = flip_mode & 0xffff;

    if (g_es_vfp)
        fwrite(data, 1, len, g_es_vfp);

    if (!g_vdec_started)
    {
        UM_AV_DEBUG("Video is not started\n");
        return -1;
    }

    g_vfeed_cnt ++;
    g_vfeed_len += len;

    if ((0xFF != play_mode) && (play_mode != g_play_mode) && (g_um_type == UM_TYPE_IUM))
    {
        UM_AV_DEBUG("Play mode change %.2x\n", play_mode);
        g_play_mode = play_mode;
        need_restart_decoder = 1;
        hccast_um_audio_restart(g_play_mode);
    }

    if (width && (g_video_width != width))
    {
        if (g_video_width)
        {
            resolution_changed = 1;
        }
        g_video_width = width;
        
        UM_AV_DEBUG("Video width: %d\n", width);
    }
    if (height && (g_video_height != height))
    {
        if (g_video_height)
        {
            resolution_changed = 1;
        }
        g_video_height = height;
        
        UM_AV_DEBUG("Video height: %d\n", height);
    }

    if (need_restart_decoder || (resolution_changed && g_play_mode != UM_PLAY_MODE_STREAM))
    {
        hccast_um_video_restart(g_play_mode);
    }

    if (resolution_changed || need_restart_decoder)
    {
        //usb air mirror set full screen while video vertical output
        UM_AV_DEBUG("width: %d, height: %d\n", g_video_width, g_video_height);
#if 0        
        if (g_video_height > g_video_width)
        {
            hccast_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
        }
        else
        {
            hccast_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
        }
#endif        
    }

    if (UM_PLAY_MODE_STREAM == g_play_mode)
    {
        rotate = 0;
    }

    //Set rotation mode
    if ((rotate != g_rotate_mode) && (rotate != 0xFF))
    {
        g_rotate_mode = rotate;
        UM_AV_DEBUG("Change rotate %d\n", rotate);
    }

    if (g_video_decoder < 0)
    {
        return -1;
    }

    hccast_um_h264_decode(data, len, pts, rotate);

    //ADD AUD NALU to decode last frame quickly
    if (last_slice == 1)
    {
        data_aux[0] = 0x00;
        data_aux[1] = 0x00;
        data_aux[2] = 0x00;
        data_aux[3] = 0x01;
        data_aux[4] = 0x09;
        hccast_um_h264_decode((uint8_t *)data_aux, 5, pts, rotate);
    }

    return 0;
}

void hccast_um_video_rotate(int rotate_en)
{
    hccast_um_video_aspect_set();
}

void hccast_um_video_mode(int mode)
{
    hccast_um_video_aspect_set();
}

void hccast_aum_screen_mode(aum_screen_mode_t *screen_mode)
{
    hccast_um_video_restart(0);

    memcpy(&g_aum_screen_mode, screen_mode, sizeof(g_aum_screen_mode));

    hccast_um_video_aspect_set();
}

int hccast_um_audio_open()
{
    struct audio_config macfg;
    uint8_t g_vol = 0;

    UM_AV_DEBUG("[%s - %d]\n", __func__, __LINE__);

    if (g_audio_started)
    {
        UM_AV_DEBUG("Warning: Audio has been started!\n");
        return -1;
    }

    if (g_audio_decoder >= 0)
    {
        close(g_audio_decoder);
        g_audio_decoder = -1;
    }

    memset(&macfg, 0, sizeof(struct audio_config));
    macfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;
    macfg.sync_mode = AVSYNC_TYPE_FREERUN;
    macfg.bits_per_coded_sample = 16;
    macfg.channels = 2;
    macfg.bit_rate = 0;
    macfg.sample_rate = 48000;
    macfg.block_align = 0;
    macfg.extradata_size = 0;
    macfg.audio_flush_thres = 300;

    macfg.snd_devs = hccast_com_media_control(HCCAST_CMD_SND_DEVS_GET, 0);

    g_audio_decoder = open("/dev/auddec", O_RDWR);
    if (g_audio_decoder < 0)
    {
        perror("Open auddec");
        return -1;
    }

    if (ioctl(g_audio_decoder, AUDDEC_INIT, &macfg) != 0)
    {
        perror("Init auddec");
        close(g_audio_decoder);
        g_audio_decoder = -1;
        return -1;
    }
    //ioctl(g_audio_decoder, AUDDEC_SET_FLUSH_TIME, 300);
    ioctl(g_audio_decoder, AUDDEC_START, 0);

    g_audio_started = 1;

    int snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0)
    {
        perror("Open sndC0i2so");
        g_audio_started = 0;
        return -1;
    }

    //mute controled is by upper user, don not enable mute here
    //ioctl(snd_fd, SND_IOCTL_SET_MUTE, 0);

    ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &g_vol);
    if (g_vol < 100)
    {
        g_vol = 100;
        ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &g_vol);
    }
    else
    {
        ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &g_vol);
    }
    close(snd_fd);

    g_avsync_dev = open("/dev/avsync0", O_RDWR);

    return 0;
}

void hccast_um_audio_close()
{
    UM_AV_DEBUG("[%s - %d]\n", __func__, __LINE__);

    if (!g_audio_started)
    {
        UM_AV_DEBUG("[%s - %d] audio has been close.\n", __func__, __LINE__);
        return ;
    }

    if (g_audio_decoder >= 0)
    {
        close(g_audio_decoder);
        g_audio_decoder = -1;
    }

    if (g_avsync_dev >= 0)
    {
        close(g_avsync_dev);
        g_avsync_dev = -1;
    }

    g_audio_started = 0;
}

int hccast_um_audio_feed(int type, unsigned char *buf, int length, unsigned long long pts)
{
    AvPktHd pkthd = {0};

    if (!g_audio_started )
    {
        UM_AV_DEBUG("Audio is not started\n");
        return -1;
    }

    g_afeed_cnt ++;
    g_afeed_len += length;

    if (g_audio_decoder < 0)
    {
        return -1;
    }

    pkthd.pts = pts;
    pkthd.dur = 0;
    pkthd.size = length;
    pkthd.flag = AV_PACKET_ES_DATA;
    if (write(g_audio_decoder, (uint8_t *)&pkthd, sizeof(AvPktHd)) != sizeof(AvPktHd))
    {
        perror("Write AvPktHd");
        goto fail;
    }

    if (write(g_audio_decoder, buf, length) != length)
    {
        perror("Write aFrame");
        goto fail;
    }

    return length;

fail:
    ioctl(g_audio_decoder, AUDDEC_FLUSH, 0);

    return 0;
}

void hccast_um_set_timebase(unsigned int time_ms)
{
    int percent = 0;

    if (g_video_decoder >= 0)
    {
        ioctl(g_video_decoder, GET_AV_BUFFERING_PERCENT, &percent);
    }

    if (percent > 50)
    {
        ium_api_ioctl(IUM_CMD_ENABLE_BUFFERING, 0, NULL);
    }
    else if (percent < 20)
    {
        ium_api_ioctl(IUM_CMD_ENABLE_BUFFERING, 1, NULL);
    }

    if (g_avsync_dev >= 0)
    {
        ioctl(g_avsync_dev, AVSYNC_SET_STC_MS, time_ms);
    }
}

void hccast_um_av_reset()
{
    UM_AV_DEBUG("[%s - %d]\n", __func__, __LINE__);
    hccast_um_video_restart(g_play_mode);
    hccast_um_audio_restart(g_play_mode);
}

void hccast_um_video_pause(int pause)
{
    if (g_video_decoder < 0)
    {
        return ;
    }

    UM_AV_DEBUG("[%s - %d]\n", __func__, __LINE__);

    if (pause)
    {
        ioctl(g_video_decoder, VIDDEC_PAUSE, 0);
    }
    else
    {
        ioctl(g_video_decoder, VIDDEC_START, 0);
    }
}

ium_av_func_t ium_av_func =
{
    ._video_open    = hccast_um_video_open,
    ._video_close   = hccast_um_video_close,
    ._video_feed    = hccast_um_video_feed,
    ._video_rotate  = hccast_um_video_rotate,
    ._audio_open    = hccast_um_audio_open,
    ._audio_close   = hccast_um_audio_close,
    ._audio_feed    = hccast_um_audio_feed,
    ._set_timebase  = hccast_um_set_timebase,
    ._av_reset      = hccast_um_av_reset,
    ._video_pause   = hccast_um_video_pause,
};

aum_av_func_t aum_av_func =
{
    ._video_open    = hccast_um_video_open,
    ._video_close   = hccast_um_video_close,
    ._video_feed    = hccast_um_video_feed,
    ._video_rotate  = hccast_um_video_rotate,
    ._video_mode    = hccast_um_video_mode,
    ._screen_mode   = hccast_aum_screen_mode,
    ._audio_open    = hccast_um_audio_open,
    ._audio_close   = hccast_um_audio_close,
    ._audio_feed    = hccast_um_audio_feed,
};
