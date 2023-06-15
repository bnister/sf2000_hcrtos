#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#ifdef HC_RTOS
#include <aircast/aircast_api.h>
#include <aircast/aircast_mdns.h>
#include <aircast/aircast_urlplay.h>
#else
#include <hccast/aircast_api.h>
#include <hccast/aircast_mdns.h>
#include <hccast/aircast_urlplay.h>
#endif
#include <hccast_air.h>
#include <stdint.h>
#include <hccast_dsc.h>
#include <hccast_av.h>
#include <hccast_media.h>
#include <hccast_scene.h>
#include <hccast_dsc.h>
#include <hccast_log.h>
#include "hccast_air_api.h"


static int aircast_mirror_conn = 0;
static int aircast_audio_conn = 0;
static int aircast_url_cid = -1;
static int aircast_url_totaltime = 0;
static int aircast_url_type = 0;//1-yt advert url, 2-yt live url.
static int g_is_air_audio = 0;
static int current_aircast_mode = 0;
static int g_frame_num = 60;//default
static int g_url_enable_def_vol = 1;

static int total_time = 0;
static int playing_url = 0;
static int seek_pos = 0;
static int seek_old_pos = 0;
static int cur_vol_cid = 0;
static int advert_total_time = 0;
static int is_advert_url = 0;
static int g_mirror_skip_url = 0;

hccast_air_res_e g_air_res = HCCAST_AIR_RES_1080P60;
static int g_air_user_res_w = 0;
static int g_air_user_res_h = 0;
static int g_air_user_frame_num = 0;

hccast_air_event_callback aircast_cb_fun = NULL;
extern aircast_av_func_t g_aircast_av_func;

static int g_dnssd_started = 0;
static int g_air_started = 0;
static pthread_mutex_t g_air_svr_mutex = PTHREAD_MUTEX_INITIALIZER;

#define M3U8_INFO_NUM 2
hccast_media_ytb_m3u8_t ytb_m3u8[M3U8_INFO_NUM];

static aircast_dsc_func_t g_aircast_dsc_aes_func = {hccast_dsc_aes_ctr_open,
                                       hccast_dsc_aes_cbc_open,
                                       hccast_dsc_ctx_destroy,
                                       hccast_dsc_decrypt,
                                       hccast_dsc_encrypt
                                      };


unsigned int hccast_air_get_tick(void)
{
    unsigned int cur_tick;
    struct timespec time;

    clock_gettime(CLOCK_REALTIME, &time);

    cur_tick = (time.tv_sec * 1000) + (time.tv_nsec / 1000000);

    return (cur_tick);
}

int hccast_air_ap_mirror_stat(void)
{
    return aircast_mirror_conn;
}

int hccast_air_ap_audio_stat(void)
{
    return aircast_audio_conn;
}

int hccast_air_mirror_get_skip_url(void)
{
    return g_mirror_skip_url;
}

void hccast_air_mirror_set_skip_url(int flag)
{
    g_mirror_skip_url = flag;
}

int hccast_air_is_playing_music(void)
{
    return g_is_air_audio && aircast_audio_conn;
}

int hccast_air_get_mirror_mode(void)
{
#ifndef AIRCAST_SUPPORT_MIRROR_ONLY
    int data_mode = 1;
    int hostap_en = 0;
    int mirror_mode = AIRCAST_MIRROR_ONLY;

    if(aircast_cb_fun)
    {
    	aircast_cb_fun(HCCAST_AIR_GET_MIRROR_MODE, (void*)&data_mode, NULL);
    	aircast_cb_fun(HCCAST_AIR_GET_NETWORK_STATUS, (void*)&hostap_en, NULL);
    }

    if(data_mode == 0)//mirror and stream.
    {
    	mirror_mode = AIRCAST_MIRROR_WITH_HTTP_STREAM;
    }
    else if(data_mode == 1) //mirror only.
    {
    	mirror_mode = AIRCAST_MIRROR_ONLY;
    }
    else if(data_mode == 2) //Auto.
    {
    	if(hostap_en)
    	{
            mirror_mode = AIRCAST_MIRROR_ONLY;
    	}
    	else
    	{
            mirror_mode = AIRCAST_MIRROR_WITH_HTTP_STREAM;
    	}
    }
    else
    {	
    	hccast_log(LL_WARNING,"data_mode is a bad value: %d\n",mirror_mode);	
    }

    hccast_log(LL_INFO,"mirror_mode: %d\n",mirror_mode);
    return mirror_mode;
#else
    return AIRCAST_MIRROR_ONLY;
#endif	
}

int hccast_air_ap_get_mirror_frame_num(void)
{
    return g_frame_num;
}

static void hccast_air_ap_set_mirror_frame_num(void)
{
    int is_4k_mdoe = 0;
#ifdef AIRMIRROR_P30	
    g_frame_num = 30;
#else	
    if(aircast_cb_fun)
    {
    	aircast_cb_fun(HCCAST_AIR_CHECK_4K_MODE, (void*)&is_4k_mdoe, NULL);
    	if(is_4k_mdoe)
    	{
            g_frame_num = 30;
    	}
    	else
    	{
            if(g_air_user_frame_num)
            {
                g_frame_num = g_air_user_frame_num;
            }
            else
            {
                if((g_air_res == HCCAST_AIR_RES_1080P60) || (g_air_res == HCCAST_AIR_RES_720P60))
                {
                    g_frame_num = 60;
                }
                else if((g_air_res == HCCAST_AIR_RES_720P30) || (g_air_res == HCCAST_AIR_RES_1080P30))
                {
                    g_frame_num = 30;
                }
                else
                {
                    g_frame_num = 60;
                    hccast_log(LL_NOTICE, "%s use default g_frame_num 60\n",__func__);
                }
            }
    	}
    }
#endif	
}

void hccast_air_ap_config_mirror_resolution(void)
{
    int frame_num = hccast_air_ap_get_mirror_frame_num();
    int width = 0;//1920;//1280;
    int height = 0;//1080;//720;

    if(g_air_user_res_w && g_air_user_res_h)
    {
        width = g_air_user_res_w;           
        height = g_air_user_res_h;
    }
    else
    {
        if((g_air_res == HCCAST_AIR_RES_1080P60) || (g_air_res == HCCAST_AIR_RES_1080P30))
        {
            width = 1920;
            height = 1080;
        }
        else if((g_air_res == HCCAST_AIR_RES_720P30) || (g_air_res == HCCAST_AIR_RES_720P60))
        {
            width = 1280;
            height = 1280;
        } 
    }
    
    hccast_log(LL_NOTICE,"[%s][%d] %dx%dP%d \n", __func__, __LINE__, width, height, frame_num);
    hccast_air_api_set_resolution(width, height, frame_num);
}

void hccast_air_set_resolution(int width, int height, int frame_num)
{
    if(width && height && frame_num)
    {   
        g_air_user_res_w = width;
        g_air_user_res_h = height;
        g_air_user_frame_num = frame_num;
    }
}

static void hccast_air_ap_stop_air_play_url()
{
    if (playing_url)
    {
        //stop mediaplayer
        hccast_media_stop();
        is_advert_url = 0;
        playing_url = 0;
        hccast_log(LL_INFO,"[%s][%d]\n", __func__, __LINE__);
    }
}

static void hccast_air_ap_stop_dlna_play_url()
{
    hccast_media_stop();
    hccast_log(LL_INFO,"[%s][%d]\n", __func__, __LINE__);
}


static void hccast_air_ap_pause_player(unsigned char b_pause)
{
    if (b_pause)
    {
        hccast_media_pause();
    }
    else
    {
        hccast_media_resume();
    }
}

static int hccast_air_ffplayer_get_status(int status)
{
    int aircast_player_state;
    switch (status)
    {
        case HCCAST_MEDIA_STATUS_PAUSED:
            aircast_player_state = AIRCAST_MEDIA_STATUS_PAUSED;
            break;

        case HCCAST_MEDIA_STATUS_PLAYING:
            aircast_player_state = AIRCAST_MEDIA_STATUS_PLAYING ;
            break;
        case HCCAST_MEDIA_STATUS_STOP:
        default:
            aircast_player_state = AIRCAST_MEDIA_STATUS_STOPPED ;
            break;
    }

    return aircast_player_state;
}


static void hccast_air_ap_get_play_state(AircastPlayerState_T *pplayerStat)
{
    int status;

    memset(pplayerStat, 0, sizeof(pplayerStat));
    if(is_advert_url)
    {
    	pplayerStat->totalTime = advert_total_time;
    }
    else
    {
    	pplayerStat->totalTime = ((long)hccast_media_get_duration() / 1000);
    }
    
    pplayerStat->currTime = (hccast_media_get_position() / 1000);
    pplayerStat->status = hccast_air_ffplayer_get_status(hccast_media_get_status());

}

static int aircast_cur_event = 0;
void hccast_air_mediaplayer_2_aircast_event(int type, void *param)
{
    hccast_log(LL_INFO,"%s:%d\n", __FUNCTION__, type);
    aircast_cur_event = type;
    hccast_air_api_event_notify(type, param);
}

void hccast_air_media_event(int type, void *param)
{
    int air_event;

    switch(type)
    {
    	case HCCAST_MEDIA_AIR_VIDEO_END:
            air_event = AIRCAST_VIDEO_ENDED;
            hccast_air_mediaplayer_2_aircast_event(air_event,param);
            break;	
    	case HCCAST_MEDIA_AIR_VIDEO_USEREXIT:
            air_event = AIRCAST_VIDEO_USEREXIT;
            hccast_air_mediaplayer_2_aircast_event(air_event,param);
            break;
        case HCCAST_MEDIA_AIR_VIDEO_PAUSE:
            air_event = AIRCAST_VIDEO_PAUSED;
            hccast_air_mediaplayer_2_aircast_event(air_event,param);
            break;
        case HCCAST_MEDIA_AIR_VIDEO_PLAY:
            air_event = AIRCAST_VIDEO_PLAY;
            hccast_air_mediaplayer_2_aircast_event(air_event,param);
            break;
        case HCCAST_MEDIA_AIR_VIDEO_LOADING:
            air_event = AIRCAST_VIDEO_LOADING;
            hccast_air_mediaplayer_2_aircast_event(air_event,param);
            break;
    	default :
            break;
    }
}

void hccast_air_callback_func(hccast_air_event_e msg_type, void* in, void* out)
{
    if (aircast_cb_fun)
        aircast_cb_fun(msg_type, in, out);
}

int hccast_air_ap_url_parse(struct air_url_info *puri_info, hccast_media_url_t* mp_url_info)
{
    int ret = 0;
    char *video_url = NULL;
    char *audio_url = NULL;


    if((puri_info == NULL) || (mp_url_info == NULL))
    {
    	return -1;
    }

    //advertisement double url.
    if((strstr(puri_info->url, "Advertisement")) && (strstr(puri_info->url,"V://")) && (strstr(puri_info->url,"A://")) )
    {
    	hccast_log(LL_NOTICE,"%s()This is Advertisement url.\n",__func__);

    	if((strstr(puri_info->url,"V://")) && (strstr(puri_info->url,"A://")))
    	{
            //parse audio url
            char* a_url_beging = strstr(puri_info->url,"A://");
            audio_url = a_url_beging +strlen("A://");
            //parse video url
            video_url = strstr(puri_info->url,"V://");
            video_url += strlen("V://");
            *a_url_beging = '\0';

            char* tmp = strstr(video_url,"index.m3u8");
            tmp += strlen("index.m3u8");
            *tmp = '\0';

            tmp = strstr(audio_url,"index.m3u8");
            tmp += strlen("index.m3u8");
            *tmp = '\0';

            mp_url_info->url = video_url;
            mp_url_info->url1 = audio_url;
            mp_url_info->url_mode = HCCAST_MEDIA_URL_AIRCAST;
            mp_url_info->media_type = HCCAST_MEDIA_MOVIE;
            mp_url_info->ytb_m3u8[HCCAST_MEDIA_YTB_VIDEO] = &ytb_m3u8[HCCAST_MEDIA_YTB_VIDEO];
            mp_url_info->ytb_m3u8[HCCAST_MEDIA_YTB_AUDIO] = &ytb_m3u8[HCCAST_MEDIA_YTB_AUDIO];

            is_advert_url = 1;
            ret = 1;
            hccast_log(LL_NOTICE,"video:%s\n",mp_url_info->url);
            hccast_log(LL_NOTICE,"audio:%s\n",mp_url_info->url1);
    	}	
    }
    else if((strstr(puri_info->url,"V://")) && (strstr(puri_info->url,"A://")) )//double url
    {
    	hccast_log(LL_NOTICE,"%s()This is double aircast url.\n",__func__);
        //parse audio url
        char* a_url_beging = strstr(puri_info->url,"A://");
        audio_url = a_url_beging +strlen("A://");
        //parse video url
        video_url = strstr(puri_info->url,"V://");
        video_url += strlen("V://");
        *a_url_beging = '\0';

        mp_url_info->url = video_url;
        mp_url_info->url1 = audio_url;
        mp_url_info->url_mode = HCCAST_MEDIA_URL_AIRCAST;
        mp_url_info->media_type = HCCAST_MEDIA_MOVIE;
        mp_url_info->ytb_m3u8[HCCAST_MEDIA_YTB_VIDEO] = &ytb_m3u8[HCCAST_MEDIA_YTB_VIDEO];
        mp_url_info->ytb_m3u8[HCCAST_MEDIA_YTB_AUDIO] = &ytb_m3u8[HCCAST_MEDIA_YTB_AUDIO];
    	
    	is_advert_url = 0;
    	ret = 1;
    	hccast_log(LL_NOTICE,"video:%s\n",mp_url_info->url);
    	hccast_log(LL_NOTICE,"audio:%s\n",mp_url_info->url1);
    		
    }
    else //normal url
    {
        mp_url_info->url = puri_info->url;
        mp_url_info->url1 = NULL;
        mp_url_info->url_mode = HCCAST_MEDIA_URL_AIRCAST;
        mp_url_info->media_type = HCCAST_MEDIA_MOVIE;
        is_advert_url = 0;
        ret = 0;
        hccast_log(LL_NOTICE,"video+audio:%s\n",mp_url_info->url);
    }

    return ret;
}

void hccast_air_ap_free_ytb_m3u8_buf()
{
    int i = 0;

    for(i = 0; i < M3U8_INFO_NUM; i++)
    {
    	if(ytb_m3u8[i].data)
    	{
            hccast_log(LL_DEBUG,"%s free num:%d\n",__func__,i);
            free(ytb_m3u8[i].data);
            ytb_m3u8[i].data = NULL;
            ytb_m3u8[i].size = 0;
    	}
    }	
}

void hccast_air_ap_seturl(void *param)
{
    struct air_url_info *puri_info = param;
    int ytb_flag = 0;
    int i = 0;
    int hostap_en = 0;

    hccast_log(LL_NOTICE,"[%s][%d]: AIRCAST_SET_URL \n", __func__, __LINE__);

    //This case for aircast mirror callback.
    if (strstr(puri_info->url, "music.tc.qq"))
    {
        hccast_air_mirror_set_skip_url(1);
        hccast_log(LL_NOTICE,"AIRCAST_SET_URL---mark special url, need ignore\n");
        return ;
    }
    else if(strstr(puri_info->url, "https://adsmind.gdtimg.com/"))//skip can not play advertisment.
    {
    	hccast_log(LL_NOTICE,"AIRCAST_SET_URL---mark special url, need ignore\n");
    	return ;
    }

    hccast_air_mirror_set_skip_url(0);
    
    if (g_aircast_av_func._video_close)
    {
        g_aircast_av_func._video_close();
    }

    if (g_aircast_av_func._audio_close)
    {
        g_aircast_av_func._audio_close();
    }
   

    if(hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_MIRROR)
    {
        hccast_air_callback_func(HCCAST_AIR_MIRROR_STOP,NULL,NULL);
    }

    if(current_aircast_mode == AIRCAST_MIRROR_WITH_HTTP_STREAM)
    {
        if(aircast_cb_fun)
        {	
            aircast_cb_fun(HCCAST_AIR_GET_NETWORK_STATUS, (void*)&hostap_en, NULL);
            if(hostap_en)
            {
                aircast_cb_fun(HCCAST_AIR_HOSTAP_MODE_SKIP_URL, NULL, NULL);
                hccast_log(LL_WARNING,"AIRCAST_SET_URL---skip it when is hostap mode.\n");
                return;
            }
        }
    }


    hccast_media_url_t mp_url;
    memset(&mp_url, 0, sizeof(hccast_media_url_t));

    ytb_flag = hccast_air_ap_url_parse(puri_info, &mp_url);

    hccast_media_seturl(&mp_url);


    if(ytb_flag)
    {
    	hccast_air_ap_free_ytb_m3u8_buf();
    }

    aircast_url_cid = puri_info->cid;

    playing_url = 1;
    hccast_air_mediaplayer_2_aircast_event(AIRCAST_VIDEO_LOADING, aircast_url_cid);
    total_time = 0;

    if(g_url_enable_def_vol)
    {
        hccast_set_volume(80);
    }    
}

void hccast_air_ap_stop_play(void *param)
{
    int cid = (int)param;
    hccast_log(LL_NOTICE,"[%s][%d]: AIRCAST_STOP_PLAY %d\n", __FUNCTION__, __LINE__, cid);
    aircast_url_totaltime = 0;
    aircast_url_type = 0;

    if (hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY)
    {
        hccast_air_ap_stop_air_play_url();
    }

    hccast_log(LL_NOTICE,"%s: AIRCAST_STOP_PLAY done\n", __FUNCTION__);

    total_time = 0;
}

void hccast_air_ap_get_play_info(void *param)
{
    AircastPlayerState_T *pplayerStat = param;
    int cid = pplayerStat->ret;
    char *fix_str = "";
    int diff = 0;

    hccast_air_ap_get_play_state(pplayerStat);
    if (pplayerStat->status == AIRCAST_MEDIA_STATUS_PLAYING \
        && !pplayerStat->totalTime\
        && pplayerStat->currTime > pplayerStat->totalTime)
    {
        pplayerStat->totalTime = aircast_url_totaltime;
        fix_str = "f1";
    }
    pplayerStat->volume = aircast_url_cid;
    if (!total_time && pplayerStat->status == AIRCAST_MEDIA_STATUS_PLAYING)
        total_time = pplayerStat->totalTime;

    if (aircast_cur_event == AIRCAST_VIDEO_LOADING)
    {
        diff = pplayerStat->currTime > seek_old_pos ? pplayerStat->currTime - seek_old_pos : seek_old_pos - pplayerStat->currTime;
        if (diff > 1)
        {
            hccast_air_mediaplayer_2_aircast_event(AIRCAST_VIDEO_PLAY, 0);
        }
    }
    if ((pplayerStat->ret == 0) && (pplayerStat->status == AIRCAST_MEDIA_STATUS_STOPPED))
    {
        if (cid > aircast_url_cid)
        {
            pplayerStat->currTime = 0;//total_time;
            pplayerStat->totalTime = 0;//total_time;
            fix_str = "f2";
        }
        else if (total_time)
        {

            pplayerStat->currTime = total_time;
            pplayerStat->totalTime = total_time;
            fix_str = "f3";
        }
    }

    if (aircast_url_type == 2 && pplayerStat->currTime > pplayerStat->totalTime)
    {
        pplayerStat->totalTime = pplayerStat->currTime + 10;
        fix_str = "f4";
    }
#if 0
    if (playing_url)
        printf("pplayerStat%s(%d): currTime = %d , totalTime = %d , mp_status = %d\n", \
               fix_str, cid, pplayerStat->currTime, pplayerStat->totalTime, pplayerStat->status);
#endif	

}

void hccast_air_ap_url_seek(void *param)
{
    int64_t seek_time_ms = 0;

    if (aircast_url_type == 1 || aircast_url_type == 2)
    {
        hccast_log(LL_WARNING,"[%s][%d]:ignore seek, url_type = %d \n", __func__, __LINE__, aircast_url_type);
        return;
    }

    hccast_log(LL_NOTICE,"[%s][%d]:seek position = %d \n", __func__, __LINE__, param);
    seek_time_ms = (int)param * 1000;

    hccast_media_seek(seek_time_ms);//need to change to ms.

    seek_pos = param;

    AircastPlayerState_T playerStat;
    hccast_air_ap_get_play_state(&playerStat);
    if (playerStat.ret == 0)
    {
        seek_old_pos = playerStat.currTime;
    }
    hccast_air_mediaplayer_2_aircast_event(AIRCAST_VIDEO_LOADING, aircast_url_cid);

}

void hccast_air_ap_mirror_start()
{
    aircast_mirror_conn ++;
    hccast_air_mirror_set_skip_url(0);
    hccast_scene_switch(HCCAST_SCENE_AIRCAST_MIRROR);

    hccast_log(LL_NOTICE,"[%s][%d]:AIRCAST_MIRROR_START (%d)\n", __func__, __LINE__, aircast_mirror_conn);
    usleep(100 * 1000);
	
    hccast_air_callback_func(HCCAST_AIR_MIRROR_START, NULL, NULL);
    if (g_aircast_av_func._video_open)
    {
        g_aircast_av_func._video_open();
    }

    if (g_aircast_av_func._audio_open)
    {
        g_aircast_av_func._audio_open();
    }
}

void hccast_air_ap_mirror_stop()
{
    hccast_log(LL_NOTICE,"[%s][%d]:AIRCAST_MIRROR_STOP (%d)\n", __func__, __LINE__, aircast_mirror_conn);
	
    if (g_aircast_av_func._video_close)
    {
        g_aircast_av_func._video_close();
    }

    if(g_is_air_audio == 0)
    {
        hccast_log(LL_NOTICE,"[%s][%d]:audio need close.\n",__func__, __LINE__);
        if (g_aircast_av_func._audio_close)
        {
            g_aircast_av_func._audio_close();
        }
    }
    
    //for air-mirror change for airplay stream.
    if((hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_MIRROR) \
    	|| (hccast_get_current_scene() == HCCAST_SCENE_NONE) )
    {
    	hccast_scene_reset(HCCAST_SCENE_AIRCAST_MIRROR, HCCAST_SCENE_NONE);
    	hccast_air_callback_func(HCCAST_AIR_MIRROR_STOP, NULL, NULL);
    }
    else if((hccast_get_current_scene() == HCCAST_SCENE_DLNA_PLAY) || \
    	(hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR)|| \
    	(hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
    {
    	//just call to leave menu.because the current scene had changed to dlna for set url.
    	hccast_air_callback_func(HCCAST_AIR_MIRROR_STOP, NULL, NULL);
    }

    aircast_mirror_conn--;
}

void hccast_air_ap_set_volume(void *param)
{
    int volume = (int)param;
    int vol_cid = -1;
    hccast_air_api_ioctl(AIRCAST_GET_VOL_ID, &vol_cid, 0);
    int vol = 0;
    hccast_log(LL_NOTICE,"[%s][%d]AIRCAST_SET_VOLUME:vol=%d\n", __func__, __LINE__, volume);

    //range[-30 ~ 0]
    if (volume < -30)
    {
        hccast_log(LL_NOTICE,"[%s][%d]AIRCAST_SET_VOLUME: inval volume\n", __func__, __LINE__);
        return ;
    }
    else if (volume <= -29)
    {
        vol = 0;
    }
    else
    {
        vol = (30 + volume) * 100 / 30;
    }

    cur_vol_cid = vol_cid;
    //set volume
    if((hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY) && (vol == 0))
    {
        hccast_log(LL_NOTICE,"Aircast skip this time setting vol.\n");
    }
    else
    {
    	hccast_set_volume(vol);
    }
    
}

void hccast_air_ap_audio_start()
{
    aircast_audio_conn++;
    hccast_log(LL_NOTICE,"[%s][%d]:AIRCAST_AUDIO_START %d\n", __func__, __LINE__, aircast_audio_conn);

    hccast_scene_mira_stop();
    hccast_scene_set_mira_restart_flag(0);


    if (hccast_get_current_scene() == HCCAST_SCENE_DLNA_PLAY)
    {
        hccast_air_ap_stop_dlna_play_url();
    }

    if((hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY) && (hccast_media_get_status() != HCCAST_MEDIA_STATUS_STOP))
    {
        hccast_log(LL_NOTICE,"[%s][%d] AIRCAST_AUDIO_START: now just skip audio open operation.\n",__func__, __LINE__);
    }
    else
    {
        if (g_aircast_av_func._audio_open)
        {
            g_aircast_av_func._audio_open();
        }
    }

    
    g_is_air_audio = 1;

    if (hccast_get_current_scene() == HCCAST_SCENE_NONE)
    {
        hccast_air_callback_func(HCCAST_AIR_AUDIO_START, NULL, NULL);
    }

}

void hccast_air_ap_audio_stop()
{
    hccast_log(LL_NOTICE,"[%s][%d]:AIRCAST_AUDIO_STOP %d\n", __func__, __LINE__, aircast_audio_conn);
    if (g_aircast_av_func._audio_close)
    {
        g_aircast_av_func._audio_close();
    }
    aircast_audio_conn--;
    g_is_air_audio = 0;
    hccast_scene_set_mira_restart_flag(1);
    usleep(20 * 1000);
    if (hccast_get_current_scene() == HCCAST_SCENE_NONE)
    {
        hccast_air_callback_func(HCCAST_AIR_AUDIO_STOP, NULL, NULL);
    }
}

void hccast_air_ap_store_ytb_m3u8(int idx, struct mlhls_m3u8_info* play_info)
{
    int m3u8_len;
    int m3u8_buf_size;

    //we should free at before.
    if(ytb_m3u8[idx].data)
    {
    	free(ytb_m3u8[idx].data);
    	ytb_m3u8[idx].data = NULL;
    	ytb_m3u8[idx].size = 0;
    }

    m3u8_len = strlen(play_info->m3u8);
    m3u8_buf_size = m3u8_len + 1;
	
    ytb_m3u8[idx].data = malloc(m3u8_buf_size);
    if(ytb_m3u8[idx].data == NULL)
    {
        hccast_log(LL_ERROR,"[%s]: ytb_m3u8[%d].data malloc fail\n",__func__, idx);
    }
    else
    {
    	ytb_m3u8[idx].type = idx;
    	ytb_m3u8[idx].size = m3u8_buf_size;
    	memcpy(ytb_m3u8[idx].data, play_info->m3u8,m3u8_len);
    	ytb_m3u8[idx].data[m3u8_len] = '\0';
    	hccast_log(LL_NOTICE,"[%s]: ytb_m3u8[%d].data size :%d\n",__func__, idx,ytb_m3u8[idx].size);
    }
}

void hccast_air_ap_set_play_info(struct mlhls_m3u8_info* play_info)
{
    static int last_live_cid;
    static int live_set_play_info_cnt = 0;

    int cid = play_info->id >> 8;


    if (aircast_url_type == 2)//this is live stream
    {
    	//check wheather the same client. 
        if (cid != last_live_cid) 
        {
            last_live_cid = cid;
            live_set_play_info_cnt = 1;
        }
        else
        {
            live_set_play_info_cnt++;//it mean the same stream.
        }
    }
    else
    {
        live_set_play_info_cnt = 0;
    }


    if (live_set_play_info_cnt >= 2)
    {
    	//update live stream m3u8 playlist.
        if (play_info->type == 2)//video 
        {
            hccast_media_ytb_update_m3u8_playlist(HCCAST_MEDIA_YTB_VIDEO, play_info->m3u8,strlen(play_info->m3u8));
            //printf("[%s]:YTB:V: id = %d, total_len = %d, total_time = %d \n", __func__,play_info->id, play_info->total_len, play_info->total_time);
            aircast_url_totaltime = play_info->total_time;
            aircast_url_type = play_info->url_type;
        }
        else if (play_info->type == 1)//audio
        {
            hccast_media_ytb_update_m3u8_playlist(HCCAST_MEDIA_YTB_AUDIO, play_info->m3u8,strlen(play_info->m3u8));
            //printf("[%s]:YTB:A: id = %d, total_len = %d, total_time = %d \n",  __func__, play_info->id, play_info->total_len, play_info->total_time);
        }
        else
        {
            hccast_log(LL_WARNING,"%s: unknown type.\n", __FUNCTION__);
        }
    }
    else  //here is the first stream m3u8 store.
    {
        if (play_info->type == 2)//video
        {
            hccast_air_ap_store_ytb_m3u8(HCCAST_MEDIA_YTB_VIDEO, play_info);
            hccast_log(LL_INFO,"[%s]:V: id = %d, total_len = %d, total_time = %d \n", __func__, play_info->id, play_info->total_len, play_info->total_time);
            aircast_url_totaltime = play_info->total_time;
            aircast_url_type = play_info->url_type;
        }
        else if (play_info->type == 1)//audio
        {
            hccast_air_ap_store_ytb_m3u8(HCCAST_MEDIA_YTB_AUDIO, play_info);
            hccast_log(LL_INFO,"[%s]:A: id = %d, total_len = %d, total_time = %d \n", __func__, play_info->id, play_info->total_len, play_info->total_time);
        }
        else
        {
            hccast_log(LL_WARNING,"%s: unknown type.\n", __FUNCTION__);
        }
    }

    advert_total_time = play_info->total_time;
}

static void hccast_air_ap_event_process(int event_type, void *param)
{

    if (event_type == AIRCAST_SET_URL)
    {
        hccast_air_ap_seturl(param);
    }
    else if (event_type == AIRCAST_STOP_PLAY)
    {
        hccast_air_ap_stop_play(param);
    }
    else if (event_type == AIRCAST_GET_PLAY_INFO)
    {
        hccast_air_ap_get_play_info(param);
    }
    else if (event_type == AIRCAST_URL_SEEK)
    {
        hccast_air_ap_url_seek(param);
    }
    else if (event_type == AIRCAST_URL_PAUSE)
    {
        hccast_log(LL_NOTICE,"[%s][%d]:AIRCAST_URL_PAUSE \n", __func__, __LINE__);
        hccast_air_ap_pause_player(1);
    }
    else if (event_type == AIRCAST_URL_RESUME_PLAY)
    {
        hccast_log(LL_NOTICE,"[%s][%d]:AIRCAST_URL_RESUME_PLAY \n", __func__, __LINE__);
        hccast_air_ap_pause_player(0);
    }
    else if (event_type == AIRCAST_MIRROR_START)
    {
        hccast_air_ap_mirror_start();
    }
    else if (event_type == AIRCAST_MIRROR_STOP)
    {
        hccast_air_ap_mirror_stop();
    }
    else if (event_type == AIRCAST_SET_VOLUME)
    {
        hccast_air_ap_set_volume(param);
    }
    else if (event_type == AIRCAST_GOT_AUDIO_MODE)
    {

    }
    else if (event_type == AIRCAST_AUDIO_START)
    {
        hccast_air_ap_audio_start();
    }
    else if (event_type == AIRCAST_AUDIO_STOP)
    {
        hccast_air_ap_audio_stop();
    }
    else if (event_type == AIRCAST_SET_PLAY_INFO)
    {
	struct mlhls_m3u8_info play_info;
	memcpy(&play_info, param, sizeof(play_info));
	hccast_air_ap_set_play_info(&play_info);
    }
    else if(event_type == AIRCAST_INVALID_CERT)
    {
    	hccast_air_callback_func(HCCAST_AIR_INVALID_CERT,NULL,NULL);
    }
    else if(event_type == AIRCAST_CONNRESET)
    {
    	hccast_air_callback_func(HCCAST_AIR_BAD_NETWORK,NULL,NULL);
    }
    else if(event_type == AIRCAST_FAKE_LIB)
    {
        hccast_air_callback_func(HCCAST_AIR_FAKE_LIB,NULL,NULL);
    }
}

void hccast_air_mdnssd_start()
{
    if (!g_dnssd_started)
    {
        hc_mdns_daemon_start();
        g_dnssd_started = 1;
    }
}

int hccast_air_service_is_start()
{
	return g_air_started;
}

void hccast_air_mdnssd_stop()
{
    if (g_dnssd_started)
    {
        hc_mdns_daemon_stop();
        g_dnssd_started = 0;

    }
}

int hccast_air_service_start()
{
    int aircast_mode;
    char service_name[64] = "HCCAST_AIR_TEST";
    char device_name[32] = "wlan0";


    pthread_mutex_lock(&g_air_svr_mutex);
    if (g_air_started)
    {
        hccast_log(LL_WARNING,"[%s]: aircast service has been start\n", __func__);
        pthread_mutex_unlock(&g_air_svr_mutex);
        return 0;
    }

    aircast_mode = hccast_air_get_mirror_mode();

    hccast_air_mdnssd_start();//just need start once time.

    usleep(200 * 1000);
    hccast_air_ap_set_mirror_frame_num();
    hccast_air_ap_config_mirror_resolution();
    hccast_air_api_ioctl(AIRCAST_SET_MIRROR_MODE, aircast_mode, 0);
    hccast_air_api_ioctl(AIRCAST_SET_AV_FUNC, &g_aircast_av_func, 0);
    hccast_air_api_ioctl(AIRCAST_SET_AES_FUNC, &g_aircast_dsc_aes_func, 0);
    hccast_air_api_set_event_callback(hccast_air_ap_event_process);

    if (aircast_cb_fun)
    {
        aircast_cb_fun(HCCAST_AIR_GET_SERVICE_NAME, (void*)service_name, NULL);
        aircast_cb_fun(HCCAST_AIR_GET_NETWORK_DEVICE, (void*)device_name, NULL);
        aircast_cb_fun(HCCAST_AIR_URL_ENABLE_SET_DEFAULT_VOL, (void*)&g_url_enable_def_vol, NULL);
    }

    hccast_air_api_service_start(service_name, device_name);

    current_aircast_mode = aircast_mode;
    g_air_started = 1;
    pthread_mutex_unlock(&g_air_svr_mutex);
    hccast_log(LL_NOTICE,"[Aircast]: hc_aircast_service_start done\n");
}

int hccast_air_service_stop()
{
    pthread_mutex_lock(&g_air_svr_mutex);
    if (g_air_started == 0)
    {
        hccast_log(LL_WARNING,"[%s]: aircast service has been stop\n", __func__);
        pthread_mutex_unlock(&g_air_svr_mutex);
        return 0;
    }

    hccast_air_api_service_stop();
#ifdef HC_RTOS	
    hccast_air_mdnssd_stop();//rtos not support network netlink, so need restart mdns network every time. 
#endif	
    g_air_started = 0;
    pthread_mutex_unlock(&g_air_svr_mutex);

    hccast_log(LL_NOTICE,"[Aircast]: hc_aircast_service_stop done\n");
}

int hccast_air_ioctl(int cmd, void *arg)
{
    if(cmd == HCCAST_AIR_CMD_SET_RESOLUTION)
    {
        if(arg == NULL)
        {
            return -1;
        }
        
        g_air_res = *(unsigned int *)arg;
        g_air_user_res_w = 0;
        g_air_user_res_h = 0;
        g_air_user_frame_num = 0;
        hccast_log(LL_NOTICE, "%s g_air_res:%d\n",__func__,g_air_res);
    }
    
    return 0;
}

int hccast_air_stop_playing()
{
    int wait_time;
    unsigned int tick = hccast_air_get_tick();
    
    if(hccast_air_ap_mirror_stat())
    {
        wait_time = 5000;
        hccast_log(LL_NOTICE, "[%s:%u] begin stop air-mirror.\n",__func__, tick);
        hccast_air_api_event_notify(AIRCAST_USER_MIRROR_STOP, 0);
        while (hccast_air_ap_mirror_stat()&& ((hccast_air_get_tick() - tick) < wait_time))
        {
            hccast_scene_switch_sleep_ms(20);
        }
    }
    else if(hccast_air_ap_audio_stat())
    {
        wait_time = 2000;
        hccast_log(LL_NOTICE, "[%s:%u] begin stop air-music.\n",__func__, tick);
        hccast_air_api_event_notify(AIRCAST_USER_AUDIO_STOP, 0);
        while (hccast_air_ap_audio_stat() && (hccast_air_get_tick() - tick) < wait_time)
        {
            hccast_scene_switch_sleep_ms(20);
        }
    }
    else if(hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY)
    {
        wait_time = 5000;
        hccast_log(LL_NOTICE, "[%s:%u] begin stop air-media.\n",__func__, tick);
        hccast_air_api_event_notify(AIRCAST_VIDEO_USEREXIT, 0);
        while (playing_url && (hccast_air_get_tick() - tick) < wait_time)
        {
            hccast_scene_switch_sleep_ms(20);
        }
    }
    else
    {
        hccast_log(LL_INFO, "%s nothing to do.\n",__func__);
    }

    hccast_log(LL_NOTICE, "[%s:%u] done.\n",__func__, hccast_air_get_tick());
}


int hccast_air_service_init(hccast_air_event_callback aircast_cb)
{
    if (aircast_cb)
        aircast_cb_fun = aircast_cb;
    
    hccast_air_api_init();
    hccast_media_air_event_init(hccast_air_media_event);
    hccast_scene_air_event_init(hccast_air_api_event_notify);
    memset(ytb_m3u8, 0, sizeof(hccast_media_ytb_m3u8_t) * M3U8_INFO_NUM);

    hccast_air_api_set_event_callback(hccast_air_ap_event_process);
    hccast_air_api_service_init();
#ifdef HC_RTOS
    hc_mdns_set_devname("wlan0");
#endif
}


