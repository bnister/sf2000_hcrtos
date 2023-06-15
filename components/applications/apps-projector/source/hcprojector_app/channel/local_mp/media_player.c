/**
 *
 * 
 * media_player.c. The file is for media player, the play action include
 * stop/play/pause/seek/fast forward/fast backward/slow forward/slow backward/step/
 */
#include "app_config.h"
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
#include "factory_setting.h"
#include "screen.h"
#include "mp_fspage.h"
#include "local_mp_ui.h"
#include "mp_ctrlbarpage.h" 
#include "setup.h"
#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif
#include <hcuapi/audsink.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/lvds.h>
#ifdef RTOS_SUBTITLE_SUPPORT
#include <hcuapi/vidsink.h>
#include <libswscale/swscale.h>
#endif
#include "mp_thumbnail.h"
#include "mp_playlist.h"
#define Music_cover_zoom_w 600
#define Music_cover_zoom_h 600
#define Music_cover_zoom_x 1170
#define Music_cover_zoom_y 150
#define Dis_source_w 1920
#define Dis_source_h 1080
#define Dis_source_x 0
#define Dis_source_y 0

static int m_ff_speed[] = {1, 2, 4, 8, 16, 24, 32};
static int m_fb_speed[] = {1, -2, -4, -8, -16, -24, -32};
static float m_sf_speed[] = {1.0, 1.0/2.0, 1.0/4.0, 1.0/8.0, 1.0/16.0, 1.0/24.0};
static float m_sb_speed[] = {1, -1/2, -1/4, -1/8, -1/16, -1/24};
static bool m_closevp = false,  m_fillblack = false;
static media_play_mode_t preview_enable = MEDIA_PLAY_NORMAL;
static vdec_dis_rect_t m_dis_rect;// used for preview/normal

void media_set_play_mode(media_play_mode_t mode, vdec_dis_rect_t *rect)
{
	preview_enable = mode;
	memcpy(&m_dis_rect,rect, sizeof(vdec_dis_rect_t));
}
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
		media_hld->state=MEDIA_PLAY_END;
		api_control_send_media_message(msg->type);
		break;
    case HCPLAYER_MSG_STATE_TRICK_EOS:
        printf (">> app get trick eos, fast play to end\n");
		media_hld->state= MEDIA_PLAY_END;
		api_control_send_media_message(msg->type);
        break;
    case HCPLAYER_MSG_STATE_TRICK_BOS:
        printf (">> app get trick bos, fast back play to begining!\n");
		api_control_send_media_message(msg->type);
        break;
    case HCPLAYER_MSG_OPEN_FILE_FAILED:
        printf (">> open file fail\n");
		if(m_closevp == false&&media_hld->type!=MEDIA_TYPE_MUSIC){
			api_dis_show_onoff(false);
			//Beacause of video/pic frame backup,therefore close last frame if next frame can not play
		}
		api_control_send_media_message(msg->type);
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
		api_control_send_media_message(msg->type);
        break;
    case HCPLAYER_MSG_STATE_PAUSED:
        printf(">> player paused\n");
        break;
    case HCPLAYER_MSG_STATE_READY:
        printf(">> player ready\n");
		api_control_send_media_message(msg->type);
        break;
    case HCPLAYER_MSG_READ_TIMEOUT:
        printf(">> player read timeout\n");
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO:
        printf(">>Audio Track Unsupport");
		api_control_send_media_message(msg->type);
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO:
        printf(">>Video Track Unsupport");
		if(m_closevp == false&&media_hld->type!=MEDIA_TYPE_MUSIC){
			api_dis_show_onoff(false);
			//Beacause of video frame backup,therefore close last frame if next frame can not play
		}
        api_control_send_media_message(msg->type);
		break;
    case HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE:
        {
            //this case mean player has not this type's decode and player will
			//change to next one.if player's all type decode not support send unspport all video 
			HCPlayerVideoInfo video_info;
            char *video_type = "unknow";
            if (!hcplayer_get_nth_video_stream_info (media_hld->player, msg->val, &video_info)) {
                /* only a simple sample, app developers use a static struct to mapping them. */
                if (video_info.codec_id == HC_AVCODEC_ID_HEVC) {
                    video_type = "h265";
                } 
            }
            printf(">>Unsupport Video Type:%s", video_type);
        }
        break;
    case HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE:
        {
            HCPlayerAudioInfo audio_info;
            char *audio_type = "unknow";
			//msg->val mean audio track here  
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
            printf(">>Unsupport Audio Type:%s", audio_type);
        }
        break;
    case HCPLAYER_MSG_AUDIO_DECODE_ERR:
        {
            //this case mean player has this decode but decode err
			printf(">>audio dec err, audio idx %d\n", msg->val);
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
									api_control_send_media_message(msg->type);
                                    break;
                                }
                            }
                        }
                    }
                }else{
					api_control_send_media_message(msg->type);
					hcplayer_change_audio_track(media_hld->player, -1);
				}
            }
        }
        break;
    case HCPLAYER_MSG_VIDEO_DECODE_ERR:
        {
			//this case mean player has this decode but decode err
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
									if(m_closevp == false&&media_hld->type!=MEDIA_TYPE_MUSIC){
										api_dis_show_onoff(false);
										/*Beacause of video frame backup,therefore close last frame 
										  if next frame can not play*/
									}
                                	api_control_send_media_message(msg->type);
									break;
                                }
                            }
                        }
                    }
                }else{
					if(m_closevp == false&&media_hld->type!=MEDIA_TYPE_MUSIC){
						api_dis_show_onoff(false);
						/*Beacause of video frame backup,therefore close last frame 
							if next frame can not play*/
					}
					api_control_send_media_message(msg->type);
					hcplayer_change_video_track(media_hld->player, -1);
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
        	//IPC_NOWAIT, no block but when when hcplayer send message and hcplayer send messaga
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

#ifdef RTOS_SUBTITLE_SUPPORT
static lv_subtitle_t lv_subtitle;
static uint16_t last_buf_size = 0;

static void subtitle_callback(HCPlayerCallbackType type,
	void *user_data, void *val)
{
	AVSubtitle *sub;
	unsigned i = 0;
	uint32_t buf_size = 0;
	
	(void)user_data;

	sub = (AVSubtitle *)val;

	if (type == HCPLAYER_CALLBACK_SUBTITLE_OFF) {
	    printf("HCPLAYER_CALLBACK_SUBTITLE_OFF\n");
		if(lv_subtitle.type == 0) {		
			//stop subtitle( pic) show
                   // img_subtitle_fill(0,0,0,0, false)
			subtitles_event_send(SUBTITLES_EVENT_HIDDEN, NULL);
		} else {
			//stop subtitle(text/ass) show
			//lv_subtitle.data = realloc(lv_subtitle.data, 0);
			//show_subtitles("");
			subtitles_event_send(SUBTITLES_EVENT_HIDDEN, NULL);
			printf("HCPLAYER_CALLBACK_SUBTITLE_OFF: sub format %d\n", lv_subtitle.type);
		}
	}
	if (type == HCPLAYER_CALLBACK_SUBTITLE_ON) {
		lv_subtitle.type = sub->format; //ge_draw_subtitle.format = sub->format;
		if (sub->format == 0) { // subtitle pic show     
			/*AV_PIX_FMT_PAL8 ----> AV_PIX_FMT_ARGB*/
			for(i = 0;i < sub->num_rects; i++) {//
			    struct SwsContext *img_convert_ctx = sws_getContext_with_alphablend(
					sub->rects[i]->w, sub->rects[i]->h, AV_PIX_FMT_PAL8,
					sub->rects[i]->w, sub->rects[i]->h, AV_PIX_FMT_BGRA,
					SWS_BICUBIC, NULL, NULL, NULL, 0);
				if (img_convert_ctx) {
        			struct vframe_info tar_fr = {0};
        			tar_fr.pixfmt = AV_PIX_FMT_BGRA;
					tar_fr.width = sub->rects[i]->w;
					tar_fr.height = sub->rects[i]->h;
					tar_fr.pitch[0] = tar_fr.width;
					tar_fr.pitch[1] = tar_fr.width;
					tar_fr.pitch[2] = tar_fr.width;
					tar_fr.pitch[3] = tar_fr.width;
					buf_size = tar_fr.height * tar_fr.width * 4;
				
					if(last_buf_size < buf_size) {
						last_buf_size = buf_size;
						if (!lv_subtitle.data) {
							lv_subtitle.data = malloc(buf_size);
						} else {
							lv_subtitle.data = realloc(lv_subtitle.data, buf_size);
						}
						if (!lv_subtitle.data) {
							printf( "no memory %s:%d\n", __func__ ,__LINE__);
						}
						memset(lv_subtitle.data, 0,buf_size);
					}else{
						memset(lv_subtitle.data, 0, last_buf_size);
					}
					
					
					tar_fr.pixels[0] = lv_subtitle.data;
					tar_fr.pixels[1] = tar_fr.pixels[0] + tar_fr.height * tar_fr.pitch[0];
					tar_fr.pixels[2] = tar_fr.pixels[1] + tar_fr.height * tar_fr.pitch[1];
					tar_fr.pixels[3] = tar_fr.pixels[2] + tar_fr.height * tar_fr.pitch[2];
					tar_fr.pitch[0] = 4 * tar_fr.width;
					
					int ret = sws_scale(img_convert_ctx,(const uint8_t * const *)sub->rects[i]->data,
						sub->rects[i]->linesize, /* addr & linesize of src image */
						0, sub->rects[i]->h, /* the first line to end line of src image */
						tar_fr.pixels, tar_fr.pitch);/* addr & linesize of dst image */

					if (ret !=tar_fr.height) {
						printf( "ret %d != tar_fr.height %d\n",	ret, sub->rects[i]->h);
					}
					//show subtitle, pass subtitle data to lvgl to display, inform lvgl to display subtitle
					// to do here...

					lv_subtitle.w = sub->rects[i]->w;
					lv_subtitle.h = sub->rects[i]->h;
					lv_subtitle.pitch = 4 * sub->rects[i]->w;// argb8888
					subtitles_event_send(SUBTITLES_EVENT_SHOW, &lv_subtitle);

					printf("picture w: %d, h: %d\n", lv_subtitle.w, lv_subtitle.h);					
					sws_freeContext(img_convert_ctx);
				}else {
					printf( "Cannot initialize the conversion context\n");
				}
			}
		 }
		else if(sub->format == 1){
        	    AVSubtitleRect  *subRects = *sub->rects;
		    if(subRects->type ==SUBTITLE_TEXT ){
		      if(subRects->text != NULL){
				buf_size = strlen(subRects->text);
				if(last_buf_size < buf_size) {
					last_buf_size = buf_size;
					if (!lv_subtitle.data) {
						lv_subtitle.data = malloc(buf_size);
					} else {
						lv_subtitle.data = realloc(lv_subtitle.data, buf_size);
					}
					if (!lv_subtitle.data) {
						printf( "Error: no memory %s:%d\n", __func__ ,__LINE__);
						return ;
					}
				}else{
					memset(lv_subtitle.data, 0, last_buf_size);
				}
				memcpy(lv_subtitle.data, subRects->text, buf_size);
				subtitles_event_send(SUBTITLES_EVENT_SHOW, &lv_subtitle);
				//show_subtitles(lv_subtitle.data);
				printf("text[%ld]: %s\n", buf_size, subRects->text );
		      }
		    }
		    if(subRects->type ==SUBTITLE_ASS){
    		    if(subRects->ass != NULL){
		            buf_size = strlen(subRects->ass);
				if(last_buf_size < buf_size) {
					last_buf_size = buf_size;
					if (!lv_subtitle.data) {
						lv_subtitle.data = malloc(buf_size);
					} else {
						lv_subtitle.data = realloc(lv_subtitle.data, buf_size);
					}
					if (!lv_subtitle.data) {
						printf( "Error: no memory %s:%d\n", __func__ ,__LINE__);
						return ;
					}
				}else{
					memset(lv_subtitle.data, 0, last_buf_size);
				}
				
				memcpy(lv_subtitle.data, subRects->ass, buf_size);
				subtitles_event_send(SUBTITLES_EVENT_SHOW, &lv_subtitle);
				//show_subtitles(lv_subtitle.data);
				printf("ass[%ld]: %s\n", buf_size, subRects->ass);
		        #if 0    
		            uint8_t icnt = 1;
		            i = 0;
		            while(icnt==9&& subRects->ass[i] ==',')//text : the content after 9th ',' 
		                icnt++;
		            printf("ass:  %s\n", &subRects->ass[i]);    
		        #endif    
		       }
		    }
		}
		else {
			printf("HCPLAYER_CALLBACK_SUBTITLE_ON: sub format %d\n", sub->format);
		}
	}
}

#endif

media_handle_t *media_open(media_type_t type)
{
	media_handle_t *media_hld = (media_handle_t*)malloc(sizeof(media_handle_t));

	memset(media_hld, 0, sizeof(media_handle_t));
	media_hld->type = type;
	media_hld->state = MEDIA_STOP;
	media_hld->msg_id = INVALID_ID;

	if (MEDIA_TYPE_PHOTO == type){
		media_hld->time_gap = 3000; //3 seconds interval for next slide show
	}
	media_hld->loop_type = PlAY_LIST_SEQUENCE;

	media_monitor_init(media_hld);
	pthread_mutex_init(&media_hld->api_lock, NULL);
	return media_hld;
}

void media_close(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	if(m_closevp==false){
		if(media_hld->type==MEDIA_TYPE_VIDEO){
			api_dis_show_onoff(false);
			api_media_pic_backup_free();			
		}else if(media_hld->type==MEDIA_TYPE_PHOTO){
			api_dis_show_onoff(false);
			image_effect_t* pic_effect=get_img_effect_mode(); 
			if(pic_effect->mode == IMG_SHOW_NULL){
				api_media_pic_backup_free();
			}
		}
	}
	media_monitor_deinit(media_hld);
	pthread_mutex_destroy(&media_hld->api_lock);
	free((void*)media_hld);
}

int media_play(media_handle_t *media_hld, const char *media_src)
{
	ASSERT_API(media_hld && media_src);
    sys_param_t * psys_param;
	image_effect_t* g_img_effect;
	dis_zoom_t musiccover_args={0};
    HCPlayerInitArgs player_args;
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    rotate_type_e rotate_type = ROTATE_TYPE_0;
    mirror_type_e mirror_type = MIRROR_TYPE_NONE;
    printf("%s:%d: \n" , __func__ , __LINE__);

    api_get_flip_info(&rotate , &h_flip);
    rotate_type = rotate;
    mirror_type = h_flip;

	pthread_mutex_lock(&media_hld->api_lock);
	strncpy(media_hld->play_name, media_src, MAX_FILE_NAME-1);
	printf("%s(), line:%d. play: %s.\n", __FUNCTION__, __LINE__, media_src);
	memset(&player_args, 0, sizeof(player_args));
    player_args.uri = media_src;
	player_args.msg_id = media_hld->msg_id;
	player_args.user_data = media_hld;
	player_args.sync_type = 2;
#ifdef RTOS_SUBTITLE_SUPPORT
	memset(&lv_subtitle, 0, sizeof(lv_subtitle));
	player_args.callback = subtitle_callback;
	last_buf_size = 0;
    subtitles_event_send(SUBTITLES_EVENT_HIDDEN, NULL);
	ext_subtitles_init(app_get_playlist_t());	//scan subtitle file form filelist
	ext_subtitle_t *m_subtitle =ext_subtitle_data_get();
	if(m_subtitle->ext_subs_count!=0){
		player_args.ext_subtitle_stream_num=m_subtitle->ext_subs_count;
		player_args.ext_sub_uris=m_subtitle->uris;
	}
#endif    

#if CONFIG_APPS_PROJECTOR_SPDIF_OUT
	player_args.snd_devs = AUDSINK_SND_DEVBIT_SPO | AUDSINK_SND_DEVBIT_I2SO;
#endif

    psys_param = projector_get_sys_param();
    player_args.rotate_enable = 1;
    player_args.rotate_type = rotate_type;
    player_args.mirror_type = mirror_type;
    musiccover_args = app_reset_mainlayer_param(rotate_type, mirror_type);
	
    printf(">>>>rotate_enable: %d, rotate_type: %d, mirror_type:%d\n",player_args.rotate_enable, player_args.rotate_type, player_args.mirror_type);

   if(preview_enable == MEDIA_PLAY_PREVIEW){
		if(media_hld->type==MEDIA_TYPE_PHOTO){
			player_args.img_dis_mode = IMG_DIS_THUMBNAIL;
		}else if(media_hld->type==MEDIA_TYPE_MUSIC){
			player_args.play_attached_file = 1;
		}
		player_args.preview_enable = 1;
   		memcpy(&player_args.src_area, &m_dis_rect.src_rect, sizeof(struct av_area));
		memcpy(&player_args.dst_area, &m_dis_rect.dst_rect, sizeof(struct av_area));
		media_switch_blink(true);
   	}else {
		switch(media_hld->type){
			case MEDIA_TYPE_VIDEO:
				media_switch_blink(false);
				break;
			case MEDIA_TYPE_MUSIC:
				player_args.play_attached_file = 1;
				player_args.preview_enable=1;
				memcpy(&player_args.src_area,&musiccover_args.src_area,sizeof(av_area_t));
				memcpy(&player_args.dst_area,&musiccover_args.dst_area,sizeof(av_area_t));
				media_switch_blink(true);
				break;
			case MEDIA_TYPE_PHOTO:
				media_switch_blink(false);
				player_args.img_dis_mode = IMG_DIS_AUTO;
				//#ifdef SYS_ZOOM_SUPPORT
					player_args.preview_enable=1;
					player_args.dst_area.x = get_display_x();
					player_args.dst_area.y = get_display_y();
					player_args.dst_area.h = get_display_v();
					player_args.dst_area.w = get_display_h();
					memcpy(&player_args.src_area,&musiccover_args.src_area,sizeof(av_area_t));		
				//#endif
				player_args.img_dis_hold_time = media_hld->time_gap;
				player_args.gif_dis_interval = 50;
				player_args.img_alpha_mode = 0;	
				
				g_img_effect=get_img_effect_mode(); 
				if (g_img_effect->mode != IMG_SHOW_NULL) {
					memcpy(&player_args.img_effect, g_img_effect, sizeof(image_effect_t));
				}
				
				break;
			default :
				break;
		} 
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
	return API_SUCCESS;
}

int media_play_for_thumbnail(media_handle_t *media_hld, const char *media_src)
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

	if (MEDIA_TYPE_MUSIC == media_hld->type){
		player_args.play_attached_file = 0;

		//set dis dev scale to right  min
	}else if (MEDIA_TYPE_PHOTO == media_hld->type){
		player_args.img_dis_mode = IMG_DIS_AUTO;
		player_args.img_dis_hold_time = 3000;
		player_args.gif_dis_interval = 50;
		player_args.img_alpha_mode = 0;		
   } 
   //only use for thumbnail param
	player_args.transcode_config.b_enable=1;
	player_args.transcode_config.b_show=0;
	player_args.transcode_config.b_scale=1;
	player_args.transcode_config.b_capture_one=1;
	player_args.transcode_config.scale_factor=VIDDEC_SCACLE_1_8;
	player_args.disable_audio=1;
	player_args.start_time=0.2;

#if CONFIG_APPS_PROJECTOR_SPDIF_OUT
	player_args.snd_devs = AUDSINK_SND_DEVBIT_I2SO | AUDSINK_SND_DEVBIT_SPO;
#endif


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
	return API_SUCCESS;

}



int media_stop(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);
	hcplayer_stop2(media_hld->player, m_closevp, m_fillblack);
	if(m_closevp==false){
		if(media_hld->type==MEDIA_TYPE_VIDEO){
			api_media_pic_backup();
		}else if(media_hld->type==MEDIA_TYPE_PHOTO){
			image_effect_t* pic_effect=get_img_effect_mode(); 
			if(pic_effect->mode == IMG_SHOW_NULL){
				api_pic_effect_enable(false);
				api_media_pic_backup();
			}
		}
	}
	media_hld->state = MEDIA_STOP;
	media_hld->speed = 0;
#ifdef RTOS_SUBTITLE_SUPPORT	
	if(lv_subtitle.data){
	    free(lv_subtitle.data);
	    lv_subtitle.data = NULL;
    }
	printf("media stop111\n");
	subtitles_event_send(SUBTITLES_EVENT_CLOSE, NULL);
	ext_subtitle_deinit();
#endif    
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
	#ifdef RTOS_SUBTITLE_SUPPORT
		subtitles_event_send(SUBTITLES_EVENT_PAUSE, NULL);
	#endif
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
	#ifdef RTOS_SUBTITLE_SUPPORT
		subtitles_event_send(SUBTITLES_EVENT_RESUME, NULL);
	#endif
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

	play_time = (uint32_t)hcplayer_get_position(media_hld->player);
	if ((int)play_time < 0){
		play_time = media_hld->play_time;
		pthread_mutex_unlock(&media_hld->api_lock);
		return play_time;
	}

	play_time = play_time/1000;
	media_hld->play_time = play_time;
	// printf("play time %ld\n", play_time);
	pthread_mutex_unlock(&media_hld->api_lock);
	return play_time;
}

uint32_t media_get_totaltime(media_handle_t *media_hld)
{
	uint32_t total_time;
	ASSERT_API(media_hld);
	total_time = (uint32_t)(hcplayer_get_duration(media_hld->player)/1000);
	media_hld->total_time = total_time;
	return total_time;
}


media_state_t media_get_state(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	media_state_t temp_state;
	pthread_mutex_lock(&media_hld->api_lock);
	temp_state=media_hld->state;
	pthread_mutex_unlock(&media_hld->api_lock);
	return temp_state;
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

int media_get_info(media_handle_t *media_hld,mp_info_t* mp_info)
{
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);
	if (media_hld->player) 
	{
		mp_info->filesize = (float)hcplayer_get_filesize(media_hld->player)*1.0/1024.0/1024.0; // size unit MB 
		hcplayer_get_cur_audio_stream_info(media_hld->player, &mp_info->audio_info);
		hcplayer_get_cur_video_stream_info(media_hld->player, &mp_info->video_info);
		hcplayer_get_cur_subtitle_stream_info(media_hld->player, &mp_info->subtitle_info);
		hcplayer_get_media_info(media_hld->player, &mp_info->media_info);
		mp_info->audio_tracks_count=hcplayer_get_audio_streams_count(media_hld->player);
		mp_info->subtitles_count=hcplayer_get_subtitle_streams_count(media_hld->player);
		stat(media_hld->play_name,&mp_info->stat_buf);//get date form vfs, "stat" is LINUX C FUNC 
	}
	pthread_mutex_unlock(&media_hld->api_lock);
	return 0;
}


//for media-pic effect func  
int set_img_dis_modeparam(image_effect_t *  g_img_effect)
{

    switch (g_img_effect->mode)
    {
        case IMG_SHOW_SHUTTERS:
            g_img_effect->mode=IMG_SHOW_SHUTTERS;
			g_img_effect->mode_param.shuttles_param.time=50;   
			g_img_effect->mode_param.shuttles_param.direction= 0; //up
			g_img_effect->mode_param.shuttles_param.type=0;
            break;
        case IMG_SHOW_BRUSH :
            g_img_effect->mode=IMG_SHOW_BRUSH;
			g_img_effect->mode_param.brush_param.time=2; 
			g_img_effect->mode_param.brush_param.direction= 0; //up
			g_img_effect->mode_param.brush_param.type=0;            
			break;
        case IMG_SHOW_SLIDE:
            g_img_effect->mode=IMG_SHOW_SLIDE;
			g_img_effect->mode_param.slide_param.time=5; 
			g_img_effect->mode_param.slide_param.direction= 0; //up
			g_img_effect->mode_param.slide_param.type=0;            
			break;
        case IMG_SHOW_RANDOM:
            g_img_effect->mode=IMG_SHOW_RANDOM;
			g_img_effect->mode_param.random_param.time=10;  
			// g_img_effect.mode_param.random_param.direction= 0; //up
			g_img_effect->mode_param.random_param.type=0;            
			break;
        case IMG_SHOW_FADE :
            g_img_effect->mode=IMG_SHOW_FADE;
			g_img_effect->mode_param.fade_param.time=1;  
			// g_img_effect.mode_param.fade_param.direction= 0; //up
			g_img_effect->mode_param.fade_param.type=0;            
			break;
        default :
            break;
    }

    return 0;
}
//for media-pic zoom in or zoom out func 
/**
 * @description: set bcaklight 
 * @param {bool} val 1 -> backlight is on  ,0 -> backlight is off 
 * @return {*}
 */
int app_set_blacklight(bool val)
{
	int ret = 0;
#ifdef BACKLIGHT_MONITOR_SUPPORT
	if(val)
		api_pwm_backlight_monitor_update_status();
	else
		ret = api_set_backlight_brightness(0);
#else
	if(val == 1)
		ret = api_set_backlight_brightness(100);
	else
		ret = api_set_backlight_brightness(0);
#endif
	return ret;
}
/**
 * @description: use to blink or not when media_player to switch pic or video 
 * @param {bool} blink-> true  not blink ->false
 * @return {*}
 * @author: Yanisin
 */
int media_switch_blink(bool blink){
	m_closevp=	blink;
	return 0;
}

int media_pic_change_rotate(media_handle_t* media_hld,rotate_type_e type)
{
	ASSERT_API(media_hld);
	api_media_pic_backup();
	// do backup when rotate can avoid mosaic
	pthread_mutex_lock(&media_hld->api_lock);
	hcplayer_change_rotate_type(media_hld->player,type);
	media_hld->pic_rotate=type;
	pthread_mutex_unlock(&media_hld->api_lock);
	return 0;
}

// play next content in media_handle_t -> urls
int media_play_next_uri(media_handle_t* media_hld)
{
}