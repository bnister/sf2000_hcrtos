/**
 *
 * 
 * The file is for another media player, usd to play music and play pic at the same time.
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
#include "key_event.h"
#include "file_mgr.h"
#include <hcuapi/audsink.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/lvds.h>
#include "backstage_player.h"
#include "mp_bsplayer_list.h"
#include "glist.h"
#include "factory_setting.h"

static bool m_closevp = true,  m_fillblack = false;
static void *media_monitor_task(void *arg);
static int media_monitor_deinit(media_handle_t *media_hld);
static int media_monitor_init(media_handle_t *media_hld);
static void media_msg_proc(media_handle_t *media_hld, HCPlayerMsg *msg);
int bs_player_next(media_handle_t* media_hld,glist* player_glist);










static void media_msg_proc(media_handle_t *media_hld, HCPlayerMsg *msg)
{
	if (!media_hld || !msg)
        return;
printf("%s(), msg->type:%d\n", __FUNCTION__, (int)(msg->type));
    switch (msg->type)
    {
    case HCPLAYER_MSG_STATE_EOS:
        printf (">> from player 2, app get eos, normal play end!\n");
		media_hld->state=MEDIA_PLAY_END;
        bs_player_next(media_hld,app_get_bsplayer_glist());
		break;
    case HCPLAYER_MSG_STATE_TRICK_EOS:
        printf (">> from player 2,app get trick eos, fast play to end\n");
		media_hld->state= MEDIA_PLAY_END;
        break;
    case HCPLAYER_MSG_STATE_TRICK_BOS:
        printf (">>from player 2, app get trick bos, fast back play to begining!\n");
        break;
    case HCPLAYER_MSG_OPEN_FILE_FAILED:
        printf (">>from player 2, open file fail\n");

        break;
    case HCPLAYER_MSG_ERR_UNDEFINED:
        printf (">>from player 2, error unknow\n");
        break;
    case HCPLAYER_MSG_UNSUPPORT_FORMAT:
        printf (">>from player 2, unsupport format\n");
        break;
    case HCPLAYER_MSG_BUFFERING:
        printf(">>from player 2, buffering %d\n", msg->val);
        break;
    case HCPLAYER_MSG_STATE_PLAYING:
        printf(">>from player 2, player playing\n");
        break;
    case HCPLAYER_MSG_STATE_PAUSED:
        printf(">>from player 2, player paused\n");
        break;
    case HCPLAYER_MSG_STATE_READY:
        printf(">>from player 2, player ready\n");
        break;
    case HCPLAYER_MSG_READ_TIMEOUT:
        printf(">>from player 2, player read timeout\n");
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO:
        printf("Audio Track Unsupport");
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO:
        printf("Video Track Unsupport");
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
            printf("from player 2,Unsupport Video Type:%s", video_type);
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
            printf("from player 2,Unsupport Audio Type:%s", audio_type);
			// api_control_send_media_message(msg->type);
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
            #include <sys/msg.h>
            //IPC_NOWAIT, no block
            if (msgrcv(media_hld->msg_id, (void *)&msg, sizeof(HCPlayerMsg) - sizeof(msg.type), 0, IPC_NOWAIT) == -1)
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
    pthread_attr_destroy(&attr);
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



media_handle_t *backstage_player_open(media_type_t type)
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

void backstage_player_close(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);

	media_monitor_deinit(media_hld);
	pthread_mutex_destroy(&media_hld->api_lock);
	free((void*)media_hld);
}

int backstage_player_play(media_handle_t *media_hld, const char *media_src)
{
	ASSERT_API(media_hld && media_src);
    HCPlayerInitArgs player_args;

	pthread_mutex_lock(&media_hld->api_lock);
	strncpy(media_hld->play_name, media_src, MAX_FILE_NAME-1);

	printf("%s(), line:%d. play: %s.\n", __FUNCTION__, __LINE__, media_src);
	memset(&player_args, 0, sizeof(player_args));
    player_args.uri = media_src;
	player_args.msg_id = media_hld->msg_id;
	player_args.user_data = media_hld;
	player_args.sync_type = 2;
    
#if CONFIG_APPS_PROJECTOR_SPDIF_OUT
    player_args.snd_devs = AUDSINK_SND_DEVBIT_I2SO | AUDSINK_SND_DEVBIT_SPO;
#endif


	if (MEDIA_TYPE_MUSIC == media_hld->type){
		player_args.play_attached_file = 0;

		//set dis dev scale to right  min
	}else if (MEDIA_TYPE_PHOTO == media_hld->type){
		player_args.img_dis_mode = IMG_DIS_AUTO;
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

    api_set_i2so_gpio_mute_auto();

	return API_SUCCESS;
}
int backstage_player_stop(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);

	hcplayer_stop2(media_hld->player, m_closevp, m_fillblack);
	media_hld->state = MEDIA_STOP;
	media_hld->speed = 0;
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}


//for app use 

// for player 
#define MAX_MEDIA_TYPE  4
media_handle_t *bs_player_hld_arr[MAX_MEDIA_TYPE];
static media_handle_t* bs_player_hld;
int bs_player_close(void)
{
    if(bs_player_hld){
        backstage_player_stop(bs_player_hld);
        backstage_player_close(bs_player_hld);
        for(int i=0;i<MAX_MEDIA_TYPE;i++)
            bs_player_hld_arr[i]=NULL;
        bs_player_hld = NULL;

    }
    return 0;
}
//for test 
int bs_player_open(file_list_t *m_cur_file_list)
{
    char m_play_path_name[1024];
    file_node_t *file_node = NULL;
    file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
    bs_player_hld = bs_player_hld_arr[m_cur_file_list->media_type];
    if (NULL == bs_player_hld){
        bs_player_hld = backstage_player_open(m_cur_file_list->media_type);
        bs_player_hld_arr[m_cur_file_list->media_type] = bs_player_hld;
    }
    file_mgr_get_fullname(m_play_path_name, m_cur_file_list->dir_path, file_node->name);
    // add_filename_append_glist(m_play_path_name,url_list);
    // url_list=glist_append(url_list,m_play_path_name);
    // for(int i=0;i<5;i++)
    //     printf("glist_data %s\n",glist_nth_data(url_list,i));
    backstage_player_play(bs_player_hld, m_play_path_name);
    return 0;
}

//player from list 
int bs_player_open_form_glist(file_list_t *m_cur_file_list,glist* player_glist)
{
    char *m_play_path_name;
    bs_player_hld = bs_player_hld_arr[m_cur_file_list->media_type];
    if (NULL == bs_player_hld){
        bs_player_hld = backstage_player_open(m_cur_file_list->media_type);
        bs_player_hld_arr[m_cur_file_list->media_type] = bs_player_hld;
    }
    m_play_path_name=glist_nth_data(player_glist,1);
    if(m_play_path_name==NULL){
        printf(">> ! backstage player app get playlist fail\n");
        return -1;
    }
    backstage_player_play(bs_player_hld, m_play_path_name);
    return 0;
}
//to do 
int bs_player_next(media_handle_t* media_hld,glist* player_glist)
{
    if(media_hld->state!=MEDIA_STOP)
        backstage_player_stop(media_hld);
    char *m_play_path_name;
    static int list_idx=1;
    list_idx++;
    m_play_path_name=glist_nth_data(player_glist,list_idx);
    if(m_play_path_name==NULL){
        printf(">> ! without next in player glist,run a round\n");
        list_idx=1;
        m_play_path_name=glist_nth_data(player_glist,list_idx);
    }
    backstage_player_play(bs_player_hld, m_play_path_name);
    return 0;

}

void*  app_get_bsplayer_handle(void)
{
    return bs_player_hld;
}


void  backstage_player_task(void *pvParameters)
{
    file_list_t * bs_music_list=get_bs_musiclist_t();
    glist* player_glist=app_get_bsplayer_glist();
    bs_player_open_form_glist(bs_music_list,player_glist);
    while(1){
        api_sleep_ms(100);
    }
   
}

#ifdef __HCRTOS__
static TaskHandle_t bs_player_task_hdl=NULL;
//use a task to start backstage music player
int backstage_player_task_start(int argc, char **argv)
{
	// start projector main task.
	xTaskCreate(backstage_player_task, (const char *)"backstage_player", 0x2000/*configTASK_STACK_DEPTH*/,
		    NULL, portPRI_TASK_NORMAL, &bs_player_task_hdl);
		    return 0;
}

int backstage_player_task_stop(int argc, char **argv)
{
	// start projector main task.
	if (bs_player_task_hdl != NULL){
        bs_player_close();
        vTaskDelete(bs_player_task_hdl);
        bs_player_task_hdl=NULL;
    }
		
    return 0;
}
#else
static pthread_t play_thread_id = 0;
int backstage_player_task_start(int argc, char **argv)
{
    pthread_attr_t attr;
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&play_thread_id, &attr, backstage_player_task, NULL))
    {
        return -1;
    }
    pthread_attr_destroy(&attr);
    return 0;
}

int backstage_player_task_stop(int argc, char **argv)
{
    // start projector main task.
    if (play_thread_id){
        bs_player_close();
        pthread_cancel(play_thread_id);
        play_thread_id=0;
    }
        
    return 0;
}
#endif

