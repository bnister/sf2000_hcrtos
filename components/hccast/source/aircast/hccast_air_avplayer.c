#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#ifdef HC_RTOS
#include <aircast/aircast_api.h>
#include <freertos/timers.h>
#else
#include <hccast/aircast_api.h>
#endif
#include "hccast_air_avplayer.h"
#include "hccast_air_decoder.h"
#include <hccast_air.h>
#include <signal.h>
#include <hccast_av.h>
#include <time.h>
#include <hcuapi/vidmp.h>
#include <hccast_log.h>

static unsigned int g_video_width = 0;
static unsigned int g_video_height = 0;
static int g_vdec_started = 0;
static int g_audio_started = 0;
static unsigned int g_audio_pts = 0;
static void *g_audio_renderer = NULL;
static unsigned char *g_aac_decode_buf = NULL;
static pthread_mutex_t g_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_audio_mutex = PTHREAD_MUTEX_INITIALIZER;
static void *video_decoder = NULL;
static void *audio_decoder = NULL;
static unsigned int g_vfeed_cnt = 0;
static unsigned int g_vfeed_len = 0;
static unsigned int g_afeed_cnt = 0;
static unsigned int g_afeed_len = 0;
static unsigned int g_afeed_drop_cnt = 0;
static unsigned int g_acontinue_drop_cnt = 0;
static int cur_audio_decoder_type = HCCAST_AIR_AUDIO_NONE;
static int g_air_timer_start = 0;
static int g_video_scale = 0;
static int g_audio_sync_thresh = 0;
static int g_afeed_err_cnt = 0;

static uint8_t g_es_dump_en = 0;
static char g_es_dump_folder[64] = {0};
static unsigned int g_es_dump_cnt = 0;
static FILE *g_es_vfp = NULL;

static int m_flip_rotate = 0;
static int m_flip_mirror = 0;
extern hccast_air_event_callback aircast_cb_fun;

#ifdef HC_RTOS
//struct aac_eld_file;
//typedef struct aac_eld_file aac_eld_file_t;
extern void *create_aac_eld(void);
extern void aac_eld_decode_frame(void *aac_eld, unsigned char *inbuffer, int inputsize, void *outbuffer, int *outputsize);
extern void destroy_aac_eld(void *aac);
#endif

static void *hccast_air_player_state_timer(void *args)
{
    hccast_log(LL_NOTICE, "hccast_air_player_state_timer started!\n");

    while (g_vdec_started)
    {
        hccast_log(LL_NOTICE, "[FEED] V(%d:%d) A(%d:%d:%d)\n", g_vfeed_cnt, g_vfeed_len, g_afeed_cnt, g_afeed_len, g_afeed_drop_cnt);
        g_vfeed_cnt = 0;
        g_vfeed_len = 0;
        g_afeed_cnt = 0;
        g_afeed_len = 0;
        g_afeed_drop_cnt = 0;
        sleep(2);
    }

    pthread_mutex_lock(&g_timer_mutex);
    g_air_timer_start = 0;
    pthread_mutex_unlock(&g_timer_mutex);

    hccast_log(LL_NOTICE, "hccast_air_player_state_timer stopd!\n");
}

void hccast_air_set_audio_sync_thresh()
{
	g_audio_sync_thresh = hccast_get_audio_sync_thresh();
	hccast_set_audio_sync_thresh(300);
}

void hccast_air_restore_audio_sync_thresh()
{
	hccast_set_audio_sync_thresh(g_audio_sync_thresh);
	g_audio_sync_thresh = 0;
}

int hccast_air_get_mirror_rotation()
{
    int rotate = 0;
    if(aircast_cb_fun)
    {
    	aircast_cb_fun(HCCAST_AIR_GET_MIRROR_ROTATION, (void*)&rotate, NULL);
    }

    return rotate;
}

int hccast_air_get_mirror_vscreen_auto_rotation()
{
    int auto_rotate = 0;
    if(aircast_cb_fun)
    {
    	aircast_cb_fun(HCCAST_AIR_GET_MIRROR_VSCREEN_AUTO_ROTATION, (void*)&auto_rotate, NULL);
    }

    return auto_rotate;
}

int hccast_air_get_flip_mode()
{
    int flip_mode = 0;
    if(aircast_cb_fun)
    {
        aircast_cb_fun(HCCAST_AIR_GET_FLIP_MODE, (void*)&flip_mode, NULL);
    }

    return flip_mode;
}

int hccast_air_get_mirror_full_vscreen()
{
    int full_screen = 1;
    if (aircast_cb_fun)
    {
        aircast_cb_fun(HCCAST_AIR_GET_MIRROR_FULL_VSCREEN, (void *)&full_screen, NULL);
    }

    return full_screen;
}

static int hccast_air_get_rotate_mode(int flip_mode, int rotation)
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

static void hccast_air_video_mode_set(int rotate, unsigned int width, unsigned int height)
{
    int video_scale = 0;
    int full_screen = hccast_air_get_mirror_full_vscreen();
    
    if (rotate && (width < height) && (rotate != HCCAST_AIR_SCREEN_ROTATE_180))
    {
        video_scale = 1;
    }

    //Enable it, while air video width > height(ipad is horizontal), output video fullscreen.
    //but the output video may be squashed
#if 0
    if (width > height)
        video_scale = 1;
#endif 

    if (video_scale != g_video_scale)
    {
        if (video_scale)
        {
            if(full_screen)
            {
                hccast_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_NEXTFRAME);
            }
            else
            {
                hccast_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
            }
        }
        else
        {
            hccast_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
        }

        g_video_scale = video_scale;
    }
}

int hccast_air_es_dump_start(char *folder)
{
    memset(g_es_dump_folder, 0, 64);
    strcat(g_es_dump_folder, folder);
    g_es_dump_en = 1;

    return 0;
}

int hccast_air_es_dump_stop()
{
    g_es_dump_en = 0;

    return 0;
}

int hccast_air_video_open()
{
    pthread_t tid;
    char path[128] = {0};
    
    hccast_log(LL_NOTICE, "[%s - %d]\n", __func__, __LINE__);

    if (g_vdec_started)
    {
        hccast_log(LL_WARNING, "Warning: aircast video has been started!\n");
    }

    g_video_width = 0;
    g_video_height = 0;


    if (!video_decoder)
    {
        video_decoder = hccast_air_h264_decode_init(0, 0, NULL, 0, hccast_air_ap_get_mirror_frame_num());
        if (video_decoder == NULL)
        {
            hccast_log(LL_ERROR, "[%s - %d] decode init fail.\n", __func__, __LINE__);
            g_vdec_started = 0;
            return -1;
        }
    }

    hccast_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
    g_video_scale = 0;

    //set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX);
    g_vdec_started = 1;
	
    hccast_air_set_audio_sync_thresh();
	
    pthread_mutex_lock(&g_timer_mutex);
    if (g_air_timer_start == 0)
    {
        g_air_timer_start = 1;

        if (pthread_create(&tid, NULL, hccast_air_player_state_timer, NULL) != 0)
        {
            g_air_timer_start = 0;
        }
    }
    pthread_mutex_unlock(&g_timer_mutex);

    if (g_es_dump_en)
    {
        sprintf(path, "%s/aircast-%d.h264", g_es_dump_folder, g_es_dump_cnt);
        g_es_dump_cnt ++;

        g_es_vfp = fopen(path, "w+");
    }

    return 0;
}

void hccast_air_video_close()
{
    hccast_log(LL_NOTICE, "[%s - %d]\n", __func__, __LINE__);

    if (g_es_vfp)
    {
        fflush(g_es_vfp);
        fclose(g_es_vfp);
        g_es_vfp = NULL;
    }

    if (!g_vdec_started)
    {
        hccast_log(LL_WARNING, "[%s - %d] video has been close.\n", __func__, __LINE__);
        return ;
    }

    hccast_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
    g_video_scale = 0;

    if (video_decoder)
    {
        hccast_air_h264_decoder_destroy(video_decoder,1);
        video_decoder = NULL;
    }

	hccast_air_restore_audio_sync_thresh();

    g_vdec_started = 0;
    g_video_width = 0;
    g_video_height = 0;

}

int hccast_air_video_feed(unsigned char *data, unsigned int len,
                          unsigned int pts, int last_slice, unsigned int width, unsigned int height)
{
    int resolution_changed = 0;
    int rotate = 0;
    int rotate_mode = 0;
    unsigned char data_aux[5];


    if (g_es_vfp)
        fwrite(data, 1, len, g_es_vfp);

    if (!g_vdec_started )
    {
        hccast_log(LL_WARNING, "Video is not started\n");
        return -1;
    }

    g_vfeed_cnt ++;
    g_vfeed_len += len;


    if (width && (g_video_width != width))
    {
        if (g_video_width)
        {
            resolution_changed = 1;
        }
        g_video_width = width;
        //hccast_log(LL_WARNING, "Video width: %d\n", width);
    }

    if (height && (g_video_height != height))
    {
        if (g_video_height)
        {
            resolution_changed = 1;
        }
        g_video_height = height;
        //hccast_log(LL_WARNING, "Video height: %d\n", height);
    }

#if 0
    //we should reconfig video decode.
    if (resolution_changed)
    {
        hccast_log(LL_NOTICE, "resolution_changed\n");
        //step1: stop video decoder.
        if (video_decoder)
        {
            hccast_air_h264_decoder_destroy(video_decoder,0);
            video_decoder = NULL;
        }

        //step2: reopen video decoder and feed data.
        video_decoder = hccast_air_h264_decode_init(0, 0, NULL, 0, hccast_air_ap_get_mirror_frame_num());
        if (!video_decoder)
        {
            hccast_log(LL_ERROR, "h264_decode_init fail: %d\n", __LINE__);
        }

    }
#endif
    rotate = hccast_air_get_mirror_rotation();
    if((rotate == HCCAST_AIR_SCREEN_ROTATE_90) || (rotate == HCCAST_AIR_SCREEN_ROTATE_270))
    {
        if (g_video_width > g_video_height) 
        {
            if(hccast_air_get_mirror_vscreen_auto_rotation())
            {
                rotate = HCCAST_AIR_SCREEN_ROTATE_0;
            }  
        }
    }
    else if((rotate == HCCAST_AIR_SCREEN_ROTATE_0) || (rotate == HCCAST_AIR_SCREEN_ROTATE_180))
    {
        //if(hccast_air_get_mirror_vscreen_auto_rotation())
        //{
        //    rotate = HCCAST_AIR_SCREEN_ROTATE_0;
        //}           
    }
    else
    {
        rotate = HCCAST_AIR_SCREEN_ROTATE_0;
    }
    

    hccast_air_video_mode_set(rotate, g_video_width, g_video_height);

    int flip_mode = hccast_air_get_flip_mode();
    m_flip_rotate = (flip_mode & 0xffff0000) >> 16;
    m_flip_mirror = flip_mode & 0xffff;

    rotate_mode = hccast_air_get_rotate_mode(m_flip_rotate, rotate);

    if (video_decoder)
    {
        if (hccast_air_h264_decode(video_decoder, (uint8_t *)data, len, pts, rotate_mode, m_flip_mirror) < 0)
        {
            hccast_air_h264_decoder_flush(video_decoder);
        }

        //ADD AUD NALU to decode last frame quickly
        if(last_slice == 1)
        {
            data_aux[0] = 0x00;
            data_aux[1] = 0x00;
            data_aux[2] = 0x00;
            data_aux[3] = 0x01;
            data_aux[4] = 0x09;
            hccast_air_h264_decode(video_decoder , (uint8_t *)data_aux , 5 , pts , rotate_mode, m_flip_mirror);
        }
    }

    return 0;
}


int hccast_air_audio_open()
{

    hccast_log(LL_NOTICE, "[%s - %d]\n", __func__, __LINE__);
    pthread_mutex_lock(&g_audio_mutex);

    if (g_audio_started)
    {
        hccast_log(LL_WARNING, "Warning: aircast audio has been started!\n");
        pthread_mutex_unlock(&g_audio_mutex);
        return -1;
    }
#ifdef HC_RTOS
    audio_decoder = hccast_air_audio_decoder_init(16, 2, 44100, HCCAST_AIR_AUDIO_PCM);//default set it to pcm.
#else
    audio_decoder = hccast_air_audio_decoder_init(16, 2, 44100, HCCAST_AIR_AUDIO_AAC_ELD);//default set it to aac eld.
#endif

    if (!audio_decoder)
    {
        hccast_log(LL_ERROR, "Warning: audio_decoder init fail!\n");
        g_audio_started = 0;
        pthread_mutex_unlock(&g_audio_mutex);
        return -1;
    }
#ifdef HC_RTOS
    g_audio_renderer = create_aac_eld();
    g_aac_decode_buf = malloc(8194);
#endif
    g_audio_started = 1;
    cur_audio_decoder_type = HCCAST_AIR_AUDIO_AAC_ELD;
    g_acontinue_drop_cnt = 0;
    g_afeed_err_cnt = 0;
    //hccast_audio_set_i2so_mute(0);
    pthread_mutex_unlock(&g_audio_mutex);
    return 0;
}

void hccast_air_audio_close()
{
    hccast_log(LL_NOTICE, "[%s - %d]\n", __func__, __LINE__);

    //hccast_audio_set_i2so_mute(1);
    pthread_mutex_lock(&g_audio_mutex);
    
    if (!g_audio_started)
    {
        hccast_log(LL_WARNING, "[%s - %d] audio has been close.\n", __func__, __LINE__);
        pthread_mutex_unlock(&g_audio_mutex);
        return ;
    }
#ifdef HC_RTOS
    if (g_audio_renderer)
    {
        destroy_aac_eld(g_audio_renderer);
        g_audio_renderer = NULL;
    }

    if (g_aac_decode_buf)
    {
        free(g_aac_decode_buf);
        g_aac_decode_buf = NULL;
    }
#endif
    if (audio_decoder)
    {
        hccast_air_audio_decoder_destroy(audio_decoder);
        audio_decoder = NULL;
    }

    g_audio_started = 0;
    g_audio_pts = 0;
    g_acontinue_drop_cnt = 0;
    cur_audio_decoder_type = HCCAST_AIR_AUDIO_NONE;
    
    pthread_mutex_unlock(&g_audio_mutex);
}

void hccast_air_audio_reopen(int type)
{
    int feed_audio_type = HCCAST_AIR_AUDIO_NONE;

    if (type == 0x1000000) //aac eld.
    {
        feed_audio_type = HCCAST_AIR_AUDIO_AAC_ELD;
    }
    else//pcm
    {
        feed_audio_type = HCCAST_AIR_AUDIO_PCM;
    }

    if (feed_audio_type != cur_audio_decoder_type)
    {
        //1.destory audio handle.
        if (audio_decoder)
        {
            hccast_air_audio_decoder_destroy(audio_decoder);
            audio_decoder = NULL;
        }

        //2.reopen audio by actual data type.
        audio_decoder = hccast_air_audio_decoder_init(16, 2, 44100, feed_audio_type);
        if (audio_decoder == NULL)
        {
            hccast_log(LL_ERROR, "Warning: audio_decoder reopen fail!\n");
        }
        else
        {
            cur_audio_decoder_type = feed_audio_type;
        }

        hccast_log(LL_NOTICE, "<<<hccast_air_audio_reopen done>>> %d\n", cur_audio_decoder_type);
    }

}

void hccast_air_reset_aaceld()
{
#ifdef HC_RTOS
    if (g_audio_renderer)
    {
        destroy_aac_eld(g_audio_renderer);
        g_audio_renderer = NULL;
    }

    if (g_aac_decode_buf)
    {
        free(g_aac_decode_buf);
        g_aac_decode_buf = NULL;
    }
    
    g_audio_renderer = create_aac_eld();
    g_aac_decode_buf = malloc(8194);
#endif
}

int hccast_air_audio_feed(int type, unsigned char *buf, int length, unsigned int pts)
{
    pthread_mutex_lock(&g_audio_mutex);
    
    if (!g_audio_started )
    {
        hccast_log(LL_WARNING, "Audio is not started\n");
        pthread_mutex_unlock(&g_audio_mutex);
        return -1;
    }
#if 1    
    if(g_audio_pts == 0) //every first open audio backup pts.
    {
        g_audio_pts = pts;
        g_acontinue_drop_cnt = 0;
    }
    else
    {
        if(pts <= g_audio_pts)
        {
            if(g_acontinue_drop_cnt == 3)
            {
                g_audio_pts = pts;
                g_acontinue_drop_cnt = 0;
            }
            else
            {
                //printf("-%ld, %u %u\n",g_audio_pts-pts,g_audio_pts,pts);
                g_acontinue_drop_cnt++;
                g_afeed_drop_cnt++;
                pthread_mutex_unlock(&g_audio_mutex);
                return 0;
            }
        }
        else
        {
            g_audio_pts = pts;
            g_acontinue_drop_cnt = 0;
        }    
    }
#endif    
  

    g_afeed_cnt ++;
    g_afeed_len += length;

#ifdef HC_RTOS
    if (0x1000000 == type)
    {
        int out_len = 0;
        vTaskSuspendAll();
        aac_eld_decode_frame(g_audio_renderer, buf, length, g_aac_decode_buf, &out_len);
        xTaskResumeAll();
        portYIELD();
        if(out_len == 0)
        {
            g_afeed_err_cnt++;
            if(g_afeed_err_cnt == 15)
            {
                hccast_air_reset_aaceld();
                hccast_log(LL_ERROR, "%s %d, aac eld reset.\n",__func__,g_afeed_err_cnt);
                g_afeed_err_cnt = 0;
                pthread_mutex_unlock(&g_audio_mutex);
                return 0;
            }
            hccast_log(LL_ERROR, "%s %d, aac_eld_decode_frame error\n",__func__,__LINE__);
            pthread_mutex_unlock(&g_audio_mutex);
            return 0;
        }
        else
        {
            g_afeed_err_cnt = 0;
        }
        buf = g_aac_decode_buf;
        length = out_len;
    }
#else
	hccast_air_audio_reopen(type);
#endif

    if (audio_decoder)
    {
        if (hccast_air_audio_decode(audio_decoder, (uint8_t *)buf, length, pts) < 0)
        {
            hccast_air_audio_decoder_flush(audio_decoder);
        }
    }
    pthread_mutex_unlock(&g_audio_mutex);
    
    return 0;
}

aircast_av_func_t g_aircast_av_func = {hccast_air_video_open,
                                       hccast_air_video_close,
                                       hccast_air_video_feed,
                                       hccast_air_audio_open,
                                       hccast_air_audio_close,
                                       hccast_air_audio_feed
                                      };
