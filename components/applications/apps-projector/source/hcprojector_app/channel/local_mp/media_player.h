/**
 * media_player.h
 */

#ifndef __MEDIA_PLAYER_H__
#define __MEDIA_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "app_config.h"

#include <pthread.h>
//#include "com_api.h"	
#include <ffplayer.h>
#include <hcuapi/dis.h>
#include <sys/stat.h>
#include <hcuapi/vidmp.h>
#include "mp_zoom.h"
//#define  RTOS_SUBTITLE_SUPPORT

typedef enum{
	MEDIA_PLAY_NORMAL,
	MEDIA_PLAY_PREVIEW,
}media_play_mode_t;

typedef enum{
	MEDIA_TYPE_VIDEO,
	MEDIA_TYPE_MUSIC,
	MEDIA_TYPE_PHOTO,	
	MEDIA_TYPE_TXT,
	MEDIA_TYPE_COUNT,
}media_type_t;

typedef enum    
{
	MEDIA_STOP		= 0, 			//stop
	MEDIA_PLAY  		= 1, 		//normal playback
	MEDIA_PAUSE 		= 2, 		//pause
	MEDIA_FB	 		= 3,		//fast backward	
	MEDIA_FF	 		= 4,		//fast forword
	MEDIA_SB			= 5,		//slow backward
	MEDIA_SF			= 6,		//slow forward
	MEDIA_STEP			= 7,		//step video
	MEDIA_RESUME_STOP	= 8,		//
	MEDIA_PLAY_END		= 9,		//play in the end
	MEDIA_ZOOM_IN,
	MEDIA_ZOOM_OUT,
	MEDIA_ROTATE_LEFT,
	MEDIA_ROATE_RIGHT,
} media_state_t;


//used for play list to manage the play mode
typedef enum
{
	PlAY_LIST_SEQUENCE, //seqeunce play media
	PLAY_LIST_REPEAT,  // Juset like sequnce
	PLAY_LIST_RAND,   //rand play media list
	PLAY_LIST_ONE,   // only play current media.
	PLAY_LIST_NONE,  //Just like sequnce
}PlayListLoopType;


typedef struct{
	media_type_t		type;
	media_state_t 		state;	
	uint8_t 			speed;	//only used for video
	rotate_type_e 		pic_rotate;
	Zoom_size_e			zoom_size;
	uint32_t 			play_time; //not used for photo
	uint32_t 			total_time; //not used for photo
	int64_t				last_seek_op_time;
	uint32_t			jump_interval;
	uint32_t 			time_gap; //only used for photo, the slide show play interval, unit is ms.
	PlayListLoopType	loop_type;
	char 				play_name[1024];
	void 				*player;
	int                 msg_id;
	void* 				playlist; //use for prev or next
	uint8_t             exit;
	pthread_mutex_t 	api_lock;
	pthread_mutex_t 	msg_task_mutex;
	pthread_cond_t 		msg_task_cond;
}media_handle_t;

typedef struct{
	float filesize;
	int audio_tracks_count; 
	int subtitles_count;
	struct stat stat_buf ;
	HCPlayerAudioInfo audio_info;
	HCPlayerVideoInfo video_info;
	HCPlayerSubtitleInfo subtitle_info;
	HCPlayerMediaInfo media_info;
}mp_info_t ;



#ifdef RTOS_SUBTITLE_SUPPORT
typedef struct {
	uint16_t type; // 0: pic ;  others: text or ass or ....
	int w;
	int h;
	int pitch;
	uint8_t *data;
}lv_subtitle_t;

#endif

media_handle_t *media_open(media_type_t type);
void media_close(media_handle_t *media_hld);
int media_play(media_handle_t *media_hld, const char *media_src);
int media_play_for_thumbnail(media_handle_t *media_hld, const char *media_src);
int media_stop(media_handle_t *media_hld);
int media_pause(media_handle_t *media_hld);
int media_resume(media_handle_t *media_hld);
int media_seek(media_handle_t *media_hld, int time_sec);
media_state_t media_get_state(media_handle_t *media_hld);
int media_fastforward(media_handle_t *media_hld);
int media_fastbackward(media_handle_t *media_hld);
int media_slowforward(media_handle_t *media_hld);
int media_slowbackward(media_handle_t *media_hld);
uint32_t media_get_playtime(media_handle_t *media_hld);
uint32_t media_get_totaltime(media_handle_t *media_hld);
uint8_t media_get_speed(media_handle_t *media_hld);
char *media_get_cur_play_file(media_handle_t *media_hld);
int media_get_info(media_handle_t *media_hld,mp_info_t* mp_info);
// int media_set_previwe(media_handle_t *media_hld);
int set_img_dis_modeparam(image_effect_t *  g_img_effect);
int app_set_blacklight(bool val);
void media_set_play_mode(media_play_mode_t mode, vdec_dis_rect_t *rect);
int media_switch_blink(bool blink);
int media_pic_change_rotate(media_handle_t* media_hld,rotate_type_e type);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
