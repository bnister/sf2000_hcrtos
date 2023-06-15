/**
 *
 * 
 * media_player.c. The file is for media player, the play action include
 * stop/play/pause/seek/fast forward/fast backward/slow forward/slow backward/step/
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ffplayer.h>
#include <hcuapi/dis.h>
#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>
#include "com_api.h"
#include "media_player.h"
#include <sys/ioctl.h>
#include "os_api.h"
#include "key.h"
#include "../../factory_setting.h"
#include "../../screen.h"
#include "mp_fspage.h"
#include "../../setup/setup.h"
#include <bluetooth.h>

static int m_ff_speed[] = {1, 2, 4, 8, 16, 24, 32};
static int m_fb_speed[] = {1, -2, -4, -8, -16, -24, -32};
static float m_sf_speed[] = {1, 1/2, 1/4, 1/8, 1/16, 1/24};
static float m_sb_speed[] = {1, -1/2, -1/4, -1/8, -1/16, -1/24};
static bool m_closevp = true,  m_fillblack = false;

static void *media_monitor_task(void *arg);
static int media_monitor_deinit(media_handle_t *media_hld);
static int media_monitor_init(media_handle_t *media_hld);
static void media_msg_proc(media_handle_t *media_hld, HCPlayerMsg *msg);

static void media_msg_proc(media_handle_t *media_hld, HCPlayerMsg *msg)
{
    char  msg_info_s[50];
	if (!media_hld || !msg)
        return;
printf("%s(), msg->type:%d\n", __FUNCTION__, (int)(msg->type));
    switch (msg->type)
    {
    case HCPLAYER_MSG_STATE_EOS:
        printf (">> app get eos, normal play end!\n");
        api_control_send_key(msg->type);
        break;
    case HCPLAYER_MSG_STATE_TRICK_EOS:
        printf (">> app get trick eos, fast play to end\n");
        api_control_send_key(msg->type);
        break;
    case HCPLAYER_MSG_STATE_TRICK_BOS:
        printf (">> app get trick bos, fast back play to begining!\n");
        api_control_send_key(msg->type);
        break;
    case HCPLAYER_MSG_OPEN_FILE_FAILED:
        printf (">> open file fail\n");
		sprintf(msg_info_s,"Open file fail");
		win_msgbox_msg_open(msg_info_s, 3000, NULL, NULL);
		// api_control_send_media_message(msg->type);
        break;
    case HCPLAYER_MSG_ERR_UNDEFINED:
        printf (">> error unknow\n");
        break;
    case HCPLAYER_MSG_UNSUPPORT_FORMAT:
        printf (">> unsupport format\n");
        break;
    case HCPLAYER_MSG_BUFFERING:
        printf(">> buffering %d\n", msg->val);
        break;
    case HCPLAYER_MSG_STATE_PLAYING:
        printf(">> player playing\n");
        break;
    case HCPLAYER_MSG_STATE_PAUSED:
        printf(">> player paused\n");
        break;
    case HCPLAYER_MSG_STATE_READY:
        printf(">> player ready\n");
        break;
    case HCPLAYER_MSG_READ_TIMEOUT:
        printf(">> player read timeout\n");
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO:
        sprintf(msg_info_s,"Audio track unsupport");
		win_msgbox_msg_open(msg_info_s, 3000, NULL, NULL);
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO:
        sprintf(msg_info_s,"Video track unsupport");
		win_msgbox_msg_open(msg_info_s, 3000, NULL, NULL);
        break;
    case HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE:
        {
            HCPlayerVideoInfo video_info;
            char *video_type = "unknow";
            if (!hcplayer_get_nth_video_stream_info (media_hld->player, msg->val, &video_info)) {
                /* only a simple sample, app developers use a static struct to mapping them. */
                if (video_info.codec_id == HC_AVCODEC_ID_HEVC) {
                    video_type = "h265";
                } 
            }
            sprintf(msg_info_s,"Unsupport Video Type %s\n", video_type);
			win_msgbox_msg_open(msg_info_s, 3000, NULL, NULL);
			// api_control_send_media_message(msg->type);
        }
        break;
    case HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE:
        {
            HCPlayerAudioInfo audio_info;
            char *audio_type = "unknow";
            if (!hcplayer_get_nth_audio_stream_info (media_hld->player, msg->val, &audio_info)) {
                /* only a simple sample, app developers use a static struct to mapping them. */
                if (audio_info.codec_id < 0x11000) {
                    audio_type = "pcm";
                } else if (audio_info.codec_id < 0x12000) {
                    audio_type = "adpcm";
                } else if (audio_info.codec_id == HC_AVCODEC_ID_DTS) {
                    audio_type = "dts";
                } else if (audio_info.codec_id == HC_AVCODEC_ID_EAC3) {
                    audio_type = "eac3";
                } else if (audio_info.codec_id == HC_AVCODEC_ID_APE) {
                    audio_type = "ape";
                } 
            }
            sprintf(msg_info_s,"Unsupport Audio Type %s\n", audio_type);
			// api_control_send_media_message(msg->type);
			win_msgbox_msg_open(msg_info_s, 3000, NULL, NULL);
        }
        break;
    case HCPLAYER_MSG_AUDIO_DECODE_ERR:
        {
            printf("audio dec err, audio idx %d\n", msg->val);
            /* check if it is the last audio track, if not, then change to next one. */
            if (media_hld->player) {
                int total_audio_num = -1;
                total_audio_num = hcplayer_get_audio_streams_count(media_hld->player);
                if (msg->val >= 0 && total_audio_num > (msg->val + 1)) {
                    HCPlayerAudioInfo audio_info;
                    if (!hcplayer_get_cur_audio_stream_info(media_hld->player, &audio_info)) {
                        if (audio_info.index == msg->val) {
                            int idx = audio_info.index + 1;
                            while (hcplayer_change_audio_track(media_hld->player, idx)) {
                                idx++;
                                if (idx >= total_audio_num) {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
			// api_control_send_media_message(msg->type);
			sprintf(msg_info_s,"AUDIO_DECODE_ERR");
			win_msgbox_msg_open(msg_info_s, 3000, NULL, NULL);
        }
        break;
    case HCPLAYER_MSG_VIDEO_DECODE_ERR:
        {
            printf("video dec err, video idx %d\n", msg->val);
            /* check if it is the last video track, if not, then change to next one. */
            if (media_hld->player) {
                int total_video_num = -1;
                total_video_num = hcplayer_get_video_streams_count(media_hld->player);
                if (msg->val >= 0 && total_video_num > (msg->val + 1)) {
                    HCPlayerVideoInfo video_info;
                    if (!hcplayer_get_cur_video_stream_info(media_hld->player, &video_info)) {
                        if (video_info.index == msg->val) {
                            int idx = video_info.index + 1;
                            while (hcplayer_change_video_track(media_hld->player, idx)) {
                                idx++;
                                if (idx >= total_video_num) {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
			sprintf(msg_info_s,"VIDEO_DECODE_ERR");
			win_msgbox_msg_open(msg_info_s, 3000, NULL, NULL);
        }
        break;
    default:
        break;
    }
}

static void *media_monitor_task(void *arg)
{
    HCPlayerMsg msg;
    media_handle_t *media_hld = (media_handle_t *)arg;
    while(!media_hld->exit) {
        do{
            // if (0 != api_message_receive(media_hld->msg_id, &msg, msg_length))
            // {
            //     break;
            // }

        #ifdef __linux__
            if (msgrcv(media_hld->msg_id, (void *)&msg, sizeof(HCPlayerMsg) - sizeof(msg.type), 0, 0) == -1)
        #else
            if (xQueueReceive((QueueHandle_t)media_hld->msg_id, (void *)&msg, -1) != pdPASS)
        #endif
            {
                break;
            }

            //media_hld = (media_handle_t*)(msg.user_data);
            media_msg_proc(media_hld, &msg);

        }while(0);
        api_sleep_ms(10);
    }
    	pthread_mutex_lock(&media_hld->msg_task_mutex);
	pthread_cond_signal(&media_hld->msg_task_cond);
	pthread_mutex_unlock(&media_hld->msg_task_mutex);

    printf("exit media_monitor_task()\n");
    return NULL;
}

static int media_monitor_init(media_handle_t *media_hld)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    if (INVALID_ID != media_hld->msg_id)
        return API_SUCCESS;

    media_hld->msg_id = api_message_create(CTL_MSG_COUNT, sizeof(HCPlayerMsg));
    pthread_mutex_init(&media_hld->msg_task_mutex, NULL);
    pthread_cond_init(&media_hld->msg_task_cond, NULL);

    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, media_monitor_task, (void*)media_hld)) 
        return API_FAILURE;

    return API_SUCCESS;

}


static int media_monitor_deinit(media_handle_t *media_hld)
{
    if (INVALID_ID == media_hld->msg_id)
        return API_SUCCESS;


#ifndef __linux__
    HCPlayerMsg msg;
    msg.type = HCPLAYER_MSG_ERR_UNDEFINED;
    xQueueSend((QueueHandle_t)media_hld->msg_id, ( void * ) &msg, ( TickType_t ) 0 );
#endif
    media_hld->exit = 1; 
    pthread_mutex_lock(&media_hld->msg_task_mutex);
    pthread_cond_wait(&media_hld->msg_task_cond, &media_hld->msg_task_mutex);
    pthread_mutex_unlock(&media_hld->msg_task_mutex);
    api_message_delete(media_hld->msg_id);
    media_hld->msg_id = INVALID_ID;

    pthread_mutex_destroy(&media_hld->msg_task_mutex);
    pthread_cond_destroy(&media_hld->msg_task_cond);
	// media_hld->msg_task_cond = 0xffff;
    return API_SUCCESS;
}



media_handle_t *media_open(media_type_t type)
{
	media_handle_t *media_hld = (media_handle_t*)malloc(sizeof(media_handle_t));

	memset(media_hld, 0, sizeof(media_handle_t));
	media_hld->type = type;
	media_hld->state = MEDIA_STOP;
	media_hld->msg_id = INVALID_ID;

	if (MEDIA_TYPE_PHOTO == type){
		media_hld->time_gap = 2000; //2 seconds interval for next slide show
	}
	media_hld->loop_type = PLAY_LIST_NONE;

	media_monitor_init(media_hld);
	pthread_mutex_init(&media_hld->api_lock, NULL);

	return media_hld;
}

void media_close(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);

	media_monitor_deinit(media_hld);
	pthread_mutex_destroy(&media_hld->api_lock);
	free((void*)media_hld);
}

int media_play(media_handle_t *media_hld, const char *media_src)
{
	int rotate = projector_get_some_sys_param(P_INIT_ROTATE);
    int h_flip = projector_get_some_sys_param(P_INIT_H_FLIP);
    int v_flip = projector_get_some_sys_param(P_INIT_V_FLIP);
	ASSERT_API(media_hld && media_src);
    sys_param_t * psys_param;

    HCPlayerInitArgs player_args;

	pthread_mutex_lock(&media_hld->api_lock);
	strncpy(media_hld->play_name, media_src, MAX_FILE_NAME-1);

	printf("%s(), line:%d. play: %s.\n", __FUNCTION__, __LINE__, media_src);
	memset(&player_args, 0, sizeof(player_args));
    player_args.uri = media_src;
	player_args.msg_id = media_hld->msg_id;
	player_args.user_data = media_hld;
	player_args.sync_type = 2;
	player_args.img_dis_mode = IMG_DIS_FULLSCREEN;
	//player_args.buffering_enable = 1;
	// printf("code here\n");

    psys_param = projector_get_sys_param();
    player_args.rotate_enable = 1;
    if(psys_param->sysdata.flip_mode == FLIP_MODE_ROTATE_180)
    {
		if(rotate==0)
			player_args.rotate_type = ROTATE_TYPE_180;
		else if(rotate==90)
			player_args.rotate_type = ROTATE_TYPE_90;
		else if(rotate==180)
			player_args.rotate_type = ROTATE_TYPE_0;
		else if(rotate==270)
			player_args.rotate_type = ROTATE_TYPE_270;
		else
			player_args.rotate_type = ROTATE_TYPE_180;
    }
    else if(psys_param->sysdata.flip_mode == FLIP_MODE_H_MIRROR)
    {
		if(h_flip)
			player_args.mirror_type = MIRROR_TYPE_UPDOWN;
		else
			player_args.mirror_type = MIRROR_TYPE_LEFTRIGHT;
    }
    else if(psys_param->sysdata.flip_mode == FLIP_MODE_V_MIRROR)
    {
		if(v_flip)
			player_args.mirror_type = MIRROR_TYPE_LEFTRIGHT;
		else
			player_args.mirror_type = MIRROR_TYPE_UPDOWN;
    }
	else
	{
		if(rotate==0)
			player_args.rotate_type = ROTATE_TYPE_0;
		else if(rotate==90)
			player_args.rotate_type = ROTATE_TYPE_270;
		else if(rotate==180)
			player_args.rotate_type = ROTATE_TYPE_180;
		else if(rotate==270)
			player_args.rotate_type = ROTATE_TYPE_90;
		else
			player_args.rotate_type = ROTATE_TYPE_0;
	}
    printf(">>>>rotate_enable: %d, rotate_type: %d, mirror_type: %d\n",player_args.rotate_enable, player_args.rotate_type, player_args.mirror_type);
      
	// printf("picture here\n");
	// player_args.img_dis_hold_time = 3000;
	// player_args.gif_dis_interval = 50;
	// player_args.img_alpha_mode = 1;	

	if (MEDIA_TYPE_MUSIC == media_hld->type){
		player_args.play_attached_file = 1;
	}else if (MEDIA_TYPE_PHOTO == media_hld->type){
		player_args.img_dis_mode = IMG_DIS_REALSIZE;
		player_args.img_dis_hold_time = 3000;
		player_args.gif_dis_interval = 50;
		player_args.img_alpha_mode = 0;	
	}

    media_hld->player = hcplayer_create(&player_args);
    if (!media_hld->player){
        printf("hcplayer_create() fail!\n");
        pthread_mutex_unlock(&media_hld->api_lock);
        return API_FAILURE;
    }
    hcplayer_play(media_hld->player);

    media_hld->state = MEDIA_PLAY;
    media_hld->speed = 0;

	pthread_mutex_unlock(&media_hld->api_lock);

	if(bt_get_connet_state()>=BT_CONNECT_STATUS_CONNECTED)
	{
		bluetooth_set_gpio_mutu(1);
		app_set_i2so_gpio_mute(1);
	}
	else
	{
		bluetooth_set_gpio_mutu(0);
		app_set_i2so_gpio_mute(0);
	}
	printf("------------->>>>>>>> bt_get_connet_state = %d \n",bt_get_connet_state());
	return API_SUCCESS;
}

#define SOUND_DEV	"/dev/sndC0i2so"
#define DEFAULT_VOL	50
static uint8_t m_vol_back = DEFAULT_VOL;
int media_mute(bool mute)
{
	int snd_fd = -1;

	snd_fd = open(SOUND_DEV, O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return API_FAILURE;
	}

	uint8_t volume = 0;
	if (mute){
		ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &m_vol_back);
		if (0 == m_vol_back)
			m_vol_back = DEFAULT_VOL;
		if(bt_get_connet_state()<BT_CONNECT_STATUS_CONNECTED)
		ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
		ioctl(snd_fd, SND_IOCTL_SET_MUTE, mute);
	} else {
		volume = m_vol_back;
		if(bt_get_connet_state()<BT_CONNECT_STATUS_CONNECTED)
		ioctl(snd_fd, SND_IOCTL_SET_MUTE, mute);
		ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
	}

	close(snd_fd);

	printf("mute is %d, vol: %d\n", mute, volume);
	return API_SUCCESS;
}

int media_set_vol(uint8_t volume)
{
	int snd_fd = -1;

	snd_fd = open(SOUND_DEV, O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return API_FAILURE;
	}

	ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
	volume = 0;
	ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &volume);
	printf("current volume is %d\n", volume);

	close(snd_fd);
	return API_SUCCESS;
}

int media_vol_up(void)
{
	/*
	uint8_t volume;
	int snd_fd = -1;

	snd_fd = open(SOUND_DEV, O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return API_FAILURE;
	}

	ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &volume);
	volume += 

	ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
	volume = 0;
	printf("current volume is %d\n", volume);

	close(snd_fd);
	*/
	return API_SUCCESS;
}



int media_stop(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);

	hcplayer_stop2(media_hld->player, m_closevp, m_fillblack);
	media_hld->state = MEDIA_STOP;
	media_hld->speed = 0;
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}

int media_pause(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);

	pthread_mutex_lock(&media_hld->api_lock);
	hcplayer_pause(media_hld->player);
	media_hld->state = MEDIA_PAUSE;
	media_hld->speed = 0;
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}

int media_resume(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);
	hcplayer_resume(media_hld->player);
	media_hld->state = MEDIA_PLAY;
	media_hld->speed = 0;
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}

int media_seek(media_handle_t *media_hld, int time_sec)
{
	ASSERT_API(media_hld);

	pthread_mutex_lock(&media_hld->api_lock);
	hcplayer_seek(media_hld->player, time_sec * 1000);
	media_hld->state = MEDIA_PLAY;
	media_hld->speed = 0;
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}

//1x, 2x, 4x, 8x, 16x, 24x, 32x
int media_fastforward(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	int speed;
	int speed_cnt;

	pthread_mutex_lock(&media_hld->api_lock);
	speed_cnt = sizeof(m_ff_speed)/sizeof(m_ff_speed[0]);

	if (media_hld->state != MEDIA_FF){
		speed = 1;
	}else {
		speed = media_hld->speed;
		speed += 1;
		speed = speed % speed_cnt;
	}
	printf("%s(), line:%d. speed: %d\n", __FUNCTION__, __LINE__, m_ff_speed[speed]);
	hcplayer_set_speed_rate(media_hld->player, m_ff_speed[speed]);
	media_hld->speed = speed;
	if (0 == speed) //normal play
		media_hld->state = MEDIA_PLAY;
	else
		media_hld->state = MEDIA_FF;

	pthread_mutex_unlock(&media_hld->api_lock);

	return API_SUCCESS;
}

//1x, -2x, -4x, -8x, -16x, -24x, -32x
int media_fastbackward(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	int speed;
	int speed_cnt;

	pthread_mutex_lock(&media_hld->api_lock);
	speed_cnt = sizeof(m_fb_speed)/sizeof(m_fb_speed[0]);

	if (media_hld->state != MEDIA_FB){
		speed = 1;	
	}else{
		speed = media_hld->speed;
		speed += 1;
		speed = speed % speed_cnt;
	}
	printf("%s(), line:%d. speed: %d\n", __FUNCTION__, __LINE__, m_fb_speed[speed]);
	hcplayer_set_speed_rate(media_hld->player, m_fb_speed[speed]);
	media_hld->speed = speed;
	if (0 == speed) //normal play
		media_hld->state = MEDIA_PLAY;
	else
		media_hld->state = MEDIA_FB;

	pthread_mutex_unlock(&media_hld->api_lock);

	return API_SUCCESS;
}

//1x, 1/2, 1/4, 1/8, 1/16, 1/24
int media_slowforward(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	int speed;
	int speed_cnt;

	pthread_mutex_lock(&media_hld->api_lock);
	speed_cnt = sizeof(m_sf_speed)/sizeof(m_sf_speed[0]);

	if (media_hld->state != MEDIA_SF){
		speed = 1;
	} else {
		speed = media_hld->speed;
		speed += 1;
		speed = speed % speed_cnt;
	}
	printf("%s(), line:%d. speed: %f\n", __FUNCTION__, __LINE__, (double)m_sf_speed[speed]);
	hcplayer_set_speed_rate(media_hld->player, m_sf_speed[speed]);
	media_hld->speed = speed;
	if (0 == speed) //normal play
		media_hld->state = MEDIA_PLAY;
	else
		media_hld->state = MEDIA_SF;


	pthread_mutex_unlock(&media_hld->api_lock);

	return API_SUCCESS;
}

//1x, -1/2, -1/4, -1/8, -1/16, -1/24
int media_slowbackward(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	int speed;
	int speed_cnt;

	pthread_mutex_lock(&media_hld->api_lock);
	speed_cnt = sizeof(m_sb_speed)/sizeof(m_sb_speed[0]);

	if (media_hld->state != MEDIA_SB){
		speed = 1;	
	}else{
		speed = media_hld->speed;
		speed += 1;
		speed = speed % speed_cnt;
	}
	printf("%s(), line:%d. speed: %f\n", __FUNCTION__, __LINE__, (double)m_sb_speed[speed]);
	hcplayer_set_speed_rate(media_hld->player, m_sb_speed[speed]);
	media_hld->speed = speed;
	if (0 == speed) //normal play
		media_hld->state = MEDIA_PLAY;
	else
		media_hld->state = MEDIA_SB;


	pthread_mutex_unlock(&media_hld->api_lock);

	return API_SUCCESS;
}

uint32_t media_get_playtime(media_handle_t *media_hld)
{
	uint32_t play_time;
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);
	play_time = (uint32_t)(hcplayer_get_position(media_hld->player)/1000);
	media_hld->play_time = play_time;
	//printf("play time %ld\n", play_time);
	pthread_mutex_unlock(&media_hld->api_lock);
	return play_time;
}

uint32_t media_get_totaltime(media_handle_t *media_hld)
{
	uint32_t total_time;
	ASSERT_API(media_hld);
	total_time = (uint32_t)(hcplayer_get_duration(media_hld->player)/1000);
	media_hld->play_time = total_time;
	return total_time;
}


media_state_t media_get_state(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	return media_hld->state;
}

uint8_t media_get_speed(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	return media_hld->speed;
}

char *media_get_cur_play_file(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	return media_hld->play_name;
}

mp_info_t media_get_info(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	mp_info_t mp_info;
	char * uri=NULL;
	uri=mp_get_cur_fullpath();
	pthread_mutex_lock(&media_hld->api_lock);
	if (uri) {
		printf("full path : %s:\n", uri);
		if (media_hld->player) 
		{
			mp_info.filesize = hcplayer_get_filesize(media_hld->player);
			hcplayer_get_cur_audio_stream_info(media_hld->player, &mp_info.audio_info);
			hcplayer_get_cur_video_stream_info(media_hld->player, &mp_info.video_info);
			hcplayer_get_media_info(media_hld->player, &mp_info.media_info);
		}
	}
	else
	{
		printf("%s  %d \r\n",__FUNCTION__, __LINE__);
		printf("Error Path \r\n");
	} 
	pthread_mutex_unlock(&media_hld->api_lock);
	return mp_info;
}

void app_set_i2so_gpio_mute(bool val)//0 mute 1 no mute
{
	int snd_fd = -1;
	snd_fd = open(SOUND_DEV, O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return;
	}
	ioctl(snd_fd, SND_IOCTL_SET_MUTE, val);
	close(snd_fd);
}
// int media_set_previwe(media_handle_t *media_hld)
// {
// 	ASSERT_API(media_hld);
// 	pthread_mutex_lock(&media_hld->api_lock);
// 	set_zoom(DIS_SOURCE_W,DIS_SOURCE_H,0,0,PREVIWE_X,PREVIWE_Y,PREVIWE_W,PREVIWE_H);
// 	pthread_mutex_unlock(&media_hld->api_lock);
// 	return 0;
// }
