#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef __linux__
#include <sys/msg.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include "console.h"
#include "ge_draw_subtitle.h"
#include <linux/fb.h>
#include <hcge/ge_api.h>

#else
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>
#include "showlogo.h"
#endif

#include <libavutil/common.h>
#include <libavutil/avstring.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <hcuapi/common.h>
#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>
#include <hcuapi/dumpstack.h>
#include <hcuapi/dis.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/vidsink.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <ffplayer.h>
#include <ffplayer_manager.h>
#include <glist.h>
#include <getopt.h>
#include "dis_test.h"
#include "vin_dvp_test.h"

#define MAX_SCAN_FILE_LIST_LEN (500)
#define SCAN_SUB_DIR 0

typedef struct mediaplayer {
	void *player;
	char *uri;
} mediaplayer;

static mediaplayer *g_mp = 0;
#ifdef __linux__
static int g_msgid = -1;
static struct HCGeDrawSubtitle ge_draw_subtitle = {0};
#else
static QueueHandle_t g_msgid = NULL;
#endif

static pthread_t msg_rcv_thread_id = 0;
static pthread_t get_info_thread_id = 0;
static glist *g_plist = NULL;//for recode multi play
static bool g_mpabort = false;
static bool g_mp_info = false;
static int g_mp_info_interval = 200;//ms
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_multi_ins = false;
static glist *url_list = NULL;
static int g_cur_ply_idx = 0;

static pthread_t auto_switch_thread_id = 0;
static int g_stop_auto_switch = 0;
static pthread_t auto_seek_thread_id = 0;
static int g_stop_auto_seek = 0;
static int g_sync_mode = 2;
static int g_loop_play = 1;
/* i2so: 1 << 0, spo:  1 << 2,  i2so |spo: 1 << 0 | 1 <<2 */
static AudioOutDev g_snd_devs = 0;
static double g_time_ms = 0;
static bool g_buffering_enable = 0;
static bool g_bypass = 0;
static img_dis_mode_e g_img_dis_mode = 0;
static rotate_type_e g_rotate_type = 0;
static mirror_type_e g_mirror_type = 0;
static int g_audio_flush_thres = 0;
static int g_en_subtitle = 0;
static int g_pic_show_duration = 3000;//all pic play with 3000ms delay
static int g_gif_interval = 200;//gif frame 200ms interval.
static image_effect_t g_img_effect = {0};
static HCAlphaBlendMode g_pic_bg = ALPHA_BLEND_CHECKERBOARD;
static bool g_preview_enable = 0;
static vdec_dis_rect_t g_dis_rect = {{0,0,1920,1080},{0,0,1920,1080}};
static bool g_closevp = true,  g_fillblack = false;
static bool g_smooth_mode = false;
static uint8_t g_volume = 100;
static FILE *g_mem_play_file = NULL;
static struct video_transcode_config g_vtranscode = {0};
void *g_vtranscode_path = NULL;
static void *g_mp2 = NULL;
static bool g_disable_video = false;
static bool g_disable_audio = false;
static char *decryption_key = NULL;

extern int play_h264_es(int argc , char *argv[]);
extern int h264_es_play(int argc , char *argv[]);
extern int h264_es_stop(int argc , char *argv[]);
static int enter_es_play(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	return 0;
}

static int pic_backup(void)
{
	int fd;
	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	if (g_smooth_mode) {
		ioctl(fd ,DIS_BACKUP_MP , DIS_TYPE_HD);
	}

	close(fd);
	return 0;
}
static int set_i2so_volume(uint8_t volume)
{
	int snd_fd = -1;

	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
	volume = 0;
	ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &g_volume);
	printf("current volume is %d\n", g_volume);

	close(snd_fd);
	return 0;
}

static int set_i2so_mute(bool mute)
{
	int snd_fd = -1;

	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	if (mute) {
		uint8_t volume = 0;
		ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
	}

	if (!mute) {
		ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &g_volume);
		printf("current volume is %d\n", g_volume);
	}

	close(snd_fd);

	printf("mute is %d\n", mute);
	return 0;
}

static int set_i2so_gpio_mute(bool mute)
{
	int snd_fd = -1;

	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	ioctl(snd_fd, SND_IOCTL_SET_MUTE, mute);

	close(snd_fd);

	printf("mute is %d\n", mute);
	return 0;
}

static inline int find_player_from_list_by_mediaplayer_handle (
	void *a, void *b, void *user_data)
{
	(void)user_data;
	return ((mediaplayer *)(a) != b);
}

static inline int find_player_from_list_by_uri (
	void *a, void *b, void *user_data)
{
	(void)user_data;
	return (strcmp(((mediaplayer *)a)->uri, b));
}

#ifdef __linux__
 static int draw_subtitle(struct vframe_info *tar_fr, struct HCGeDrawSubtitle *ge_info)
{
	char *buf_next;
	uint8_t *tmp;

	if(init_fb_device(ge_info) != 0) {
		av_log(NULL, AV_LOG_ERROR, "Init framebuffer error.\n");
		return -1;
	}

	tmp = (uint8_t *)tar_fr->pixels[0];
	memcpy((void *)ge_info->bg_picture, (void *)tmp, ge_info->w*ge_info->h*4);
	buf_next = (char *)ge_info->screen_buffer[0];
	draw_background((uint8_t *)buf_next, ge_info);

	return 0;
}

 static void subtitle_callback(HCPlayerCallbackType type,
	void *user_data, void *val)
{
	AVSubtitle *sub;
	unsigned i = 0;
	(void)user_data;

	sub = (AVSubtitle *)val;

	if (type == HCPLAYER_CALLBACK_SUBTITLE_OFF) {
		if(ge_draw_subtitle.format == 0) {
			ge_stop_draw_subtitle(&ge_draw_subtitle);
		} else {
			av_log(NULL, AV_LOG_INFO, "HCPLAYER_CALLBACK_SUBTITLE_OFF: sub format %d\n",
				ge_draw_subtitle.format);
		}
	}
	if (type == HCPLAYER_CALLBACK_SUBTITLE_ON) {
		ge_draw_subtitle.format = sub->format;
		if (sub->format == 0) {
			/*AV_PIX_FMT_PAL8 ----> AV_PIX_FMT_ARGB*/
			for(i = 0;i < sub->num_rects; i++) {
				struct SwsContext *img_convert_ctx = sws_getContext_with_alphablend(
					sub->rects[i]->w, sub->rects[i]->h, AV_PIX_FMT_PAL8,
					sub->rects[i]->w, sub->rects[i]->h, AV_PIX_FMT_BGRA,
					SWS_BICUBIC, NULL, NULL, NULL, 0);
				if (img_convert_ctx) {
					struct vframe_info tar_fr = {0};

					tar_fr.pixfmt = AV_PIX_FMT_BGRA;
					tar_fr.width = sub->rects[i]->w;
					tar_fr.height = sub->rects[i]->h;
					ge_draw_subtitle.x = sub->rects[i]->x;
					ge_draw_subtitle.y = sub->rects[i]->y;
					ge_draw_subtitle.w = sub->rects[i]->w;
					ge_draw_subtitle.h = sub->rects[i]->h;

					tar_fr.pitch[0] = tar_fr.width;
					tar_fr.pitch[1] = tar_fr.width;
					tar_fr.pitch[2] = tar_fr.width;
					tar_fr.pitch[3] = tar_fr.width;
					uint32_t buf_size = tar_fr.height * tar_fr.width * 4;
					if(ge_draw_subtitle.tar_size < buf_size) {
						ge_draw_subtitle.tar_size = buf_size;
						if (!ge_draw_subtitle.tar_buf) {
							ge_draw_subtitle.tar_buf = malloc(buf_size);
						} else {
							ge_draw_subtitle.tar_buf = realloc(ge_draw_subtitle.tar_buf, buf_size);
						}
						if (!ge_draw_subtitle.tar_buf) {
							av_log(NULL, AV_LOG_ERROR, "no memory %s:%d\n", __func__ ,__LINE__);
						}
					}
					memset(ge_draw_subtitle.tar_buf, 0,buf_size);
					tar_fr.pixels[0] = ge_draw_subtitle.tar_buf;
					tar_fr.pixels[1] = tar_fr.pixels[0] + tar_fr.height * tar_fr.pitch[0];
					tar_fr.pixels[2] = tar_fr.pixels[1] + tar_fr.height * tar_fr.pitch[1];
					tar_fr.pixels[3] = tar_fr.pixels[2] + tar_fr.height * tar_fr.pitch[2];
					tar_fr.pitch[0] = 4 * tar_fr.width;
					int ret = sws_scale(img_convert_ctx,(const uint8_t * const *)sub->rects[i]->data,
						sub->rects[i]->linesize, /* addr & linesize of src image */
						0, sub->rects[i]->h, /* the first line to end line of src image */
						tar_fr.pixels, tar_fr.pitch);/* addr & linesize of dst image */
					if (ret != tar_fr.height) {
						av_log(NULL, AV_LOG_ERROR, "ret %d != tar_fr.height %d\n",
						ret, tar_fr.height);
					}

					draw_subtitle(&tar_fr, &ge_draw_subtitle);

					sws_freeContext(img_convert_ctx);
				} else {
					sws_freeContext(img_convert_ctx);
					av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
				}
			}
		}else {
			av_log(NULL, AV_LOG_INFO, "HCPLAYER_CALLBACK_SUBTITLE_ON: sub format %d\n", sub->format);
		}
	}
}
#endif

static int play_uri(char *uri)
{
	HCPlayerInitArgs init_args = {0};
	if (!g_mp) {
		return -1;
	}

	init_args.uri = uri;
	init_args.snd_devs = g_snd_devs;
	init_args.sync_type = g_sync_mode;
	init_args.user_data = g_mp;
	init_args.start_time = g_time_ms;
	init_args.buffering_enable = g_buffering_enable;
	init_args.play_attached_file = 1;
	init_args.msg_id = (int)g_msgid;
	init_args.img_dis_mode = g_img_dis_mode;
	init_args.mirror_type = g_mirror_type;
	init_args.rotate_type = g_rotate_type % 4;
	init_args.callback = NULL;
	init_args.disable_audio = g_disable_audio;
	init_args.disable_video = g_disable_video;
	if (g_en_subtitle) {
#ifdef __linux__
		init_args.callback = subtitle_callback;
#endif
	}
	if (g_rotate_type || g_mirror_type) {
		init_args.rotate_enable = 1;
	}
	init_args.audio_flush_thres = g_audio_flush_thres;
	init_args.bypass = g_bypass;

	init_args.img_dis_hold_time = g_pic_show_duration;
	init_args.gif_dis_interval = g_gif_interval;
	init_args.img_alpha_mode = g_pic_bg;

	init_args.decryption_key = decryption_key;

	if (g_img_effect.mode != IMG_SHOW_NULL) {
		memcpy(&init_args.img_effect, &g_img_effect, sizeof(image_effect_t));
	}

	if (g_vtranscode.b_enable) {
		memcpy(&init_args.transcode_config, &g_vtranscode, sizeof(struct video_transcode_config));
	}

	if (g_preview_enable) {
		init_args.preview_enable = 1;
		memcpy(&init_args.src_area, &g_dis_rect.src_rect, sizeof(struct av_area));
		memcpy(&init_args.dst_area, &g_dis_rect.dst_rect, sizeof(struct av_area));
	}

	g_mp->player = hcplayer_create(&init_args);
	if (!g_mp->player) {
		return -1;
	}

	g_mp->uri = strdup(uri);

	g_plist = glist_append(g_plist, g_mp);
	hcplayer_play(g_mp->player);

	g_mp = malloc(sizeof(mediaplayer));
	if (!g_mp) {
		printf("malloc g_mp err\n");
	}
	memset(g_mp, 0, sizeof(mediaplayer));

	return 0;
}

static void play_next_uri(void)
{
	printf("play_next_uri\n");
	if (url_list) {
		char *uri;

		g_cur_ply_idx++;
		if (g_cur_ply_idx >= (int)glist_length(url_list)) {
			g_cur_ply_idx = 0;
		}

		uri = (char *)glist_nth_data(url_list, g_cur_ply_idx);
		if (uri) {
			printf("uri %s sync_type %d\n", uri,g_sync_mode);
			play_uri(uri);
		}
	}
}

static void play_prev_uri(void)
{
	printf("play_prev_uri\n");

	if (url_list) {
		char *uri;

		g_cur_ply_idx--;
		if (g_cur_ply_idx < 0) {
			g_cur_ply_idx = glist_length(url_list) - 1;
		}

		uri = (char *)glist_nth_data(url_list, g_cur_ply_idx);
		if (uri) {
			printf("uri %s sync_type %d\n", uri, g_sync_mode);
			play_uri(uri);
		}
	}
}

static void *msg_recv_thread(void *arg)
{
	HCPlayerMsg msg;
	glist *list = NULL;
	mediaplayer *mp = NULL;
	(void)arg;

	while(!g_mpabort) {
#ifdef __linux__
		if (msgrcv(g_msgid, (void *)&msg, sizeof(HCPlayerMsg) - sizeof(msg.type), 0, 0) == -1)
#else
		if (xQueueReceive((QueueHandle_t)g_msgid, (void *)&msg, -1) != pdPASS)
#endif
		{
			if (!g_mpabort) {
				printf("msg_recv_thread err\n");
				usleep(5000);
			}
			continue;
		}

		if (g_mpabort) {
			break;
		}

		if (g_mp2 && msg.type == HCPLAYER_MSG_STATE_EOS) {
			hcplayer_multi_destroy(g_mp2);
			g_mp2 = NULL;
			continue;
		}

		pthread_mutex_lock(&g_mutex);

		mp = msg.user_data;
		list = glist_find_custom(g_plist, mp,
			find_player_from_list_by_mediaplayer_handle);
		if (!list) {
			usleep(1000);
			pthread_mutex_unlock(&g_mutex);
			continue;
		}

		if (msg.type == HCPLAYER_MSG_STATE_EOS)
		{
			printf ("app get eos\n");
			if (mp->player) {
				hcplayer_stop2 (mp->player, g_closevp, g_fillblack);
				pic_backup();
				mp->player = NULL;
			}
			if (mp->uri) {
				free(mp->uri);
				mp->uri = NULL;
			}
			free(mp);
			g_plist = glist_delete_link(g_plist, list);

			play_next_uri();
		} else if (msg.type == HCPLAYER_MSG_STATE_TRICK_BOS) {
			printf ("app get trick bos\n");
			if (mp->player) {
				hcplayer_resume(mp->player);
			}
		} else if (msg.type == HCPLAYER_MSG_STATE_TRICK_EOS) {
			printf ("app get trick eos\n");
			if (mp->player) {
				hcplayer_stop2 (mp->player, g_closevp, g_fillblack);
				mp->player = NULL;
				pic_backup();
			}
			if (mp->uri) {
				free(mp->uri);
				mp->uri = NULL;
			}
			free(mp);
			g_plist = glist_delete_link(g_plist, list);

			play_next_uri();
		} else if (msg.type == HCPLAYER_MSG_OPEN_FILE_FAILED
			|| msg.type == HCPLAYER_MSG_UNSUPPORT_FORMAT
			|| msg.type == HCPLAYER_MSG_ERR_UNDEFINED) {
			printf ("err happend, stop it\n");
			if (mp->player) {
				hcplayer_stop2 (mp->player, g_closevp, g_fillblack);
				mp->player = NULL;
				pic_backup();
			}
			if (mp->uri) {
				free(mp->uri);
				mp->uri = NULL;
			}
			free(mp);
			g_plist = glist_delete_link(g_plist, list);

			list = glist_nth(url_list, g_cur_ply_idx);
			url_list = glist_delete_link(url_list, list);
			g_cur_ply_idx--;

			play_next_uri();
		} else if (msg.type == HCPLAYER_MSG_BUFFERING) {
			printf("buffering %d\n", msg.val);
		} else if (msg.type == HCPLAYER_MSG_STATE_PLAYING) {
			printf("player playing\n");
		} else if (msg.type == HCPLAYER_MSG_STATE_PAUSED) {
			printf("player paused\n");
		} else if (msg.type == HCPLAYER_MSG_STATE_READY) {
			printf("player ready\n");
		} else if (msg.type == HCPLAYER_MSG_READ_TIMEOUT) {
			printf("player read timeout\n");
		} else if (msg.type == HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO) {
			printf("no audio track or no supported audio track\n");
		} else if (msg.type == HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO) {
			printf("no video track or no supported video track\n");
		} else if (msg.type == HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE) {
			HCPlayerVideoInfo video_info;
			char *video_type = "unknow";
			if (!hcplayer_get_nth_video_stream_info (mp->player, msg.val, &video_info)) {
				/* only a simple sample, app developers use a static struct to mapping them. */
				if (video_info.codec_id == HC_AVCODEC_ID_HEVC) {
					video_type = "h265";
				} else if (video_info.codec_id == HC_AVCODEC_ID_VP9) {
					video_type = "vp9";
				} else if (video_info.codec_id == HC_AVCODEC_ID_AMV) {
					video_type = "amv";
				}
			}
			printf("unsupport video type %s, codec id %d\n", video_type, video_info.codec_id);
		} else if (msg.type == HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE) {
			HCPlayerAudioInfo audio_info;
			char *audio_type = "unknow";
			if (!hcplayer_get_nth_audio_stream_info (mp->player, msg.val, &audio_info)) {
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
			printf("unsupport audio type %s\n", audio_type);
		} else if (msg.type == HCPLAYER_MSG_AUDIO_DECODE_ERR) {
			printf("audio dec err, audio idx %d\n", msg.val);
			/* check if it is the last audio track, if not, then change to next one. */
			if (mp->player) {
				int total_audio_num = -1;
				total_audio_num = hcplayer_get_audio_streams_count(mp->player);
				if (msg.val >= 0 && total_audio_num > (msg.val + 1)) {
					HCPlayerAudioInfo audio_info;
					if (!hcplayer_get_cur_audio_stream_info(mp->player, &audio_info)) {
						if (audio_info.index == msg.val) {
							int idx = audio_info.index + 1;
							while (hcplayer_change_audio_track(mp->player, idx)) {
								idx++;
								if (idx >= total_audio_num) {
									break;
								}
							}
						}
					}
				} else {
					hcplayer_change_audio_track(mp->player, -1);
				}
			}
		} else if (msg.type == HCPLAYER_MSG_VIDEO_DECODE_ERR) {
			printf("video dec err, video idx %d\n", msg.val);
			/* check if it is the last video track, if not, then change to next one. */
			if (mp->player) {
				int total_video_num = -1;
				total_video_num = hcplayer_get_video_streams_count(mp->player);
				if (msg.val >= 0 && total_video_num > (msg.val + 1)) {
					HCPlayerVideoInfo video_info;
					if (!hcplayer_get_cur_video_stream_info(mp->player, &video_info)) {
						if (video_info.index == msg.val) {
							int idx = video_info.index + 1;
							while (hcplayer_change_video_track(mp->player, idx)) {
								idx++;
								if (idx >= total_video_num) {
									break;
								}
							}
						}
					}
				} else {
					hcplayer_change_video_track(mp->player, -1);
				}
			}
		}  else if (msg.type == HCPLAYER_MSG_FIRST_VIDEO_FRAME_TRANSCODED) {
			printf("first video frame transcoded!\n");
			AvPktHd hdr = { 0 };
			if (hcplayer_read_transcoded_picture(mp->player, &hdr, sizeof(AvPktHd))
				!= sizeof(AvPktHd)) {
				printf("read header err\n");
			} else {
				void *data = malloc(hdr.size);
				if (data) {
					if (hcplayer_read_transcoded_picture(mp->player , data , hdr.size) != hdr.size) {
						printf ("read data err\n");
					} else {
						printf("get a picture, size %d\n", hdr.size);
						if (g_vtranscode_path) {
							FILE *rec = fopen(g_vtranscode_path, "wb");
							if (rec) {
								fwrite(data, hdr.size, 1, rec);
								fclose(rec);
								printf("write pic success\n");
							} else {
								printf("write pic failed\n");
							}
						}
					}
					free(data);
				} else {
					printf("no memory");
				}
			}
		} else {
			printf("unknow msg %d\n", (int)msg.type);
		}

		pthread_mutex_unlock(&g_mutex);
	}

	return NULL;
}

#ifndef __linux__
static FILE *avfile = NULL;
static int hcread(void *buf, int size, void *file)
{
	FILE *file_to_read = (FILE *)file;
	fread(buf, 1, size, file_to_read);
}

static int showlogo(int argc, char *argv[])
{
	int i = 0;

	if (argc < 2) {
		return -1;
	}

	avfile = fopen(argv[1], "rb");

	if (!avfile) {
		return -1;
	}

	return start_show_logo((void *)avfile, hcread);
}

static int stop_showlogo(int argc, char *argv[])
{
	if (avfile) {
		stop_show_logo();
		fclose(avfile);
		avfile = NULL;
	}

	return 0;
}

static int wait_showlogo(int argc, char *argv[])
{
	if (avfile) {
		wait_show_logo();
		fclose(avfile);
		avfile = NULL;
	}

	return 0;
}
#endif

static int mp_stop(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

#ifndef __linux__
	stop_showlogo(0, NULL);
#endif

	pthread_mutex_lock(&g_mutex);

	if (argc <= 1) {
		while (g_plist) {
			list = glist_last(g_plist);
			if (list) {
				mp = (mediaplayer *)list->data;
				if (mp->player) {
					hcplayer_stop2 (mp->player, g_closevp, g_fillblack);
					mp->player = NULL;
					pic_backup();
				}
				if (mp->uri) {
					free(mp->uri);
					mp->uri = NULL;
				}
				free(mp);
				g_plist = glist_delete_link(g_plist, list);
			}
		}

		if (argc != 0) {
			while (url_list) {
				if (url_list->data)
					free(url_list->data);
				url_list = glist_delete_link(url_list, url_list);
			}
		}
	} else if (argc > 1){
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			if (mp->player) {
				hcplayer_stop2 (mp->player, g_closevp, g_fillblack);
				mp->player = NULL;
				pic_backup();
			}
			if (mp->uri) {
				free(mp->uri);
				mp->uri = NULL;
			}
			free(mp);
			g_plist = glist_delete_link(g_plist, list);
		}
	}

	if (g_mem_play_file) {
		fclose(g_mem_play_file);
		g_mem_play_file = NULL;
	}

#ifdef __linux__
	if (ge_draw_subtitle.fbdev > 0) {
		if (ge_draw_subtitle.ctx != NULL) {
			free(ge_draw_subtitle.ctx);
			ge_draw_subtitle.ctx = NULL;
		}
		if (ge_draw_subtitle.tar_buf != NULL) {
			free(ge_draw_subtitle.tar_buf);
			ge_draw_subtitle.tar_buf = NULL;
		}
		deinit_fb_device(&ge_draw_subtitle);
		memset(&ge_draw_subtitle, 0, sizeof(struct HCGeDrawSubtitle));
	}
#endif
	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_smooth(int argc, char *argv[])
{
	if (argc < 2)
		return -1;
	int distype = DIS_TYPE_HD;
	int fd;
	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	g_smooth_mode = atoi(argv[1]);
	if (g_smooth_mode == 0) {
		g_closevp = 1;
		g_fillblack = 0;
		ioctl(fd ,DIS_FREE_BACKUP_MP , distype);
	} else {
		g_closevp = 0;
		g_fillblack = 0;
	}

	close(fd);
	return 0;

}
static int mp_play(int argc, char *argv[])
{
	int opt;
	char *uri = NULL;
	opterr = 0;
	optind = 0;

	if (argc < 2) {
		return -1;
	}

	uri = argv[1];
	if (!uri)
		return -1;

	g_time_ms = 0;
	g_sync_mode = 2;

	while ((opt = getopt(argc-1, &argv[1], "b:t:s:d:p:r:m:a:o:e:")) != EOF) {
		switch (opt) {
		case 'b':
			g_buffering_enable = atoi(optarg);
			printf("buffering_enable %d\n", g_buffering_enable);
			break;
		case 't':
			g_time_ms = atof(optarg);
			printf("time_ms %f\n", g_time_ms);
			break;
		case 's':
			g_sync_mode = atoi(optarg);
			printf("uri_info.sync_type %d\n", g_sync_mode);
			break;
		case 'd':
			g_img_dis_mode = atoi(optarg);
			printf("img_dis_mode %d\n", g_img_dis_mode);
			break;
		case 'p':
			g_bypass = atoi(optarg);
			printf("bypass %d\n", g_bypass);
			break;
		case 'r':
			g_rotate_type = atoi(optarg);
			if (g_rotate_type > 4)
				g_rotate_type = 4;
			printf("rotate_type %d\n", g_rotate_type);
			break;
		case 'm':
			g_mirror_type = atoi(optarg);
			printf("mirror_type %d\n", g_mirror_type);
			break;
		case 'a':
			g_audio_flush_thres = atoi(optarg);
			printf("audio_flush_thres %d\n", g_audio_flush_thres);
			break;
		case 'e':
			g_en_subtitle = atoi(optarg);
			printf("en_subtitle %d\n", g_en_subtitle);
			break;
		case 'o':
			g_snd_devs = atoi(optarg);
			printf("g_snd_devs %ld\n", g_snd_devs);
			break;
		default:
			break;
		}
	}

	pthread_mutex_lock(&g_mutex);
	while (url_list) {
		if (url_list->data)
			free(url_list->data);
		url_list = glist_delete_link(url_list, url_list);
	}
	pthread_mutex_unlock(&g_mutex);

	if (!g_multi_ins) {
		mp_stop(0, NULL);
	}

	pthread_mutex_lock(&g_mutex);

	if (g_time_ms >= 1) {
		g_time_ms *= 1000;
	}
	play_uri(uri);
	if (g_loop_play) {
		url_list = glist_append(url_list, strdup(uri));
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int memory_read(void * opaque, uint8_t *buf, int bufsize)
{
	int fd, ret;
	fd = fileno((FILE *)opaque);
	ret = read(fd, buf, bufsize);
	if (ret == 0) {
		return AVERROR_EOF;
	}

	return (ret == -1) ? AVERROR(errno) : ret;
}

static int64_t memory_seek(void *opaque, int64_t offset, int whence)
{
	int64_t ret;
	int fd = fileno((FILE *)opaque);

#ifndef AVSEEK_SIZE
#define AVSEEK_SIZE 0x10000
#endif

	if (whence == AVSEEK_SIZE) {
		/**
		* ORing this as the "whence" parameter to a seek function causes it to
		* return the filesize without seeking anywhere. Supporting this is optional.
		* If it is not supported then the seek function will return 0.
		*/
		struct stat st;
		if(fstat(fd, &st))
			return 0;
		else
			return st.st_size;
	}

	ret = lseek(fd, offset, whence);

	return ret < 0 ? AVERROR(errno) : ret;
}

static int play_uri_memory(char *uri)
{
	HCPlayerInitArgs init_args = {0};

	if (!g_mp) {
		return -1;
	}

	g_mem_play_file = fopen(uri, "r");
	if (!g_mem_play_file) {
		return -1;
	}

	//printf("g_mem_play_file %p\n", g_mem_play_file);
	init_args.readdata_opaque = g_mem_play_file;
	init_args.readdata_callback = memory_read;
	init_args.seekdata_callback = memory_seek;
	init_args.user_data = g_mp;
	init_args.play_attached_file = 1;
	init_args.msg_id = (int)g_msgid;
	init_args.sync_type = g_sync_mode;

	g_mp->player = hcplayer_create(&init_args);
	if (!g_mp->player) {
		return -1;
	}

	g_plist = glist_append(g_plist, g_mp);
	hcplayer_play(g_mp->player);

	g_mp = malloc(sizeof(mediaplayer));
	if (!g_mp) {
		printf("malloc g_mp err\n");
	}
	memset(g_mp, 0, sizeof(mediaplayer));

	return 0;
}

static int mp_memory_play(int argc, char *argv[])
{
	char *uri;
	int ret = 0;

	if (argc < 2)
		return -1;

	uri = argv[1];
	if (!uri)
		return -1;

	pthread_mutex_lock(&g_mutex);
	while (url_list) {
		if (url_list->data)
			free(url_list->data);
		url_list = glist_delete_link(url_list, url_list);
	}
	pthread_mutex_unlock(&g_mutex);

	if (!g_multi_ins) {
		mp_stop(0, NULL);
	}

	pthread_mutex_lock(&g_mutex);
	ret = play_uri_memory(uri);
	pthread_mutex_unlock(&g_mutex);

	return ret;
}

static int mp_pause(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc == 1) {
		list = g_plist;
		while (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_pause(mp->player);
			list = glist_next(list);
		}
	} else {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_pause(mp->player);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_resume(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc == 1) {
		list = g_plist;
		while (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_resume(mp->player);
			list = glist_next(list);
		}
	} else {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_resume(mp->player);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}


static int mp_seek(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc >= 3) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_seek(mp->player, atoi(argv[2]) * 1000);
		}
	} else if (glist_length(g_plist) == 1 && argc == 2) {
		mp = glist_first(g_plist)->data;
		hcplayer_seek(mp->player, atoi(argv[1]) * 1000);
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static void print_media_info(mediaplayer *mp)
{
	int64_t filesize;
	HCPlayerAudioInfo audio_info = {0};
	HCPlayerVideoInfo video_info = {0};
	HCPlayerSubtitleInfo subtitle_info = {0};
	HCPlayerMediaInfo media_info = {0};

	if (mp->uri) {
		printf("uri: %s:\n", mp->uri);
	}

	if (mp->player) {
		filesize = hcplayer_get_filesize(mp->player);
		printf ("filesize "LONG_INT_FORMAT"\n", filesize);

		printf ("number of audio tracks %d\n",
			hcplayer_get_audio_streams_count(mp->player));
		printf ("number of video tracks %d\n",
			hcplayer_get_video_streams_count(mp->player));
		printf ("number of subtitle tracks %d\n",
			hcplayer_get_subtitle_streams_count(mp->player));
		printf ("\n");

		hcplayer_get_cur_audio_stream_info(mp->player, &audio_info);
		printf ("audio info:\n");
		printf ("index:		 %d\n", audio_info.index);
		printf ("codec_id:		%d\n", audio_info.codec_id);
		printf ("lang_code:	 %s\n", audio_info.lang_code);
		printf ("channels:		%d\n", audio_info.channels);
		printf ("samplerate:	%d\n", audio_info.sample_rate);
		printf ("depth:		 %d\n", audio_info.depth);
		printf ("bitrate:		"LONG_INT_FORMAT"\n", audio_info.bit_rate);
		printf ("\n");

		hcplayer_get_cur_video_stream_info(mp->player, &video_info);
		printf ("video info:\n");
		printf ("index:		 %d\n", video_info.index);
		printf ("codec_id:		%d\n", video_info.codec_id);
		printf ("lang_code:	 %s\n", video_info.lang_code);
		printf ("width:		 %d\n", video_info.width);
		printf ("height:		%d\n", video_info.height);
		printf ("frame_rate:	%f\n", video_info.frame_rate);
		printf ("bitrate:		"LONG_INT_FORMAT"\n", video_info.bit_rate);
		printf ("\n");

		hcplayer_get_cur_subtitle_stream_info(mp->player, &subtitle_info);
		printf ("subtitle info:\n");
		printf ("index:		 %d\n", subtitle_info.index);
		printf ("codec_id:		%d\n", subtitle_info.codec_id);
		printf ("lang_code:	 %s\n", subtitle_info.lang_code);
		printf ("\n");

		hcplayer_get_media_info(mp->player, &media_info);
		printf ("media info:\n");
		if (media_info.artist)
			printf ("artist:		%s\n", media_info.artist);
		if (media_info.album)
			printf ("album:		%s\n", media_info.album);
		if (media_info.title)
			printf ("title:		%s\n", media_info.title);
		if (media_info.TYER)
			printf ("TYER:		%s\n", media_info.TYER);
		if (media_info.datetime)
			printf ("datetime:	%s\n", media_info.datetime);
		if (media_info.orientation)
			printf ("orientation:	%s\n", media_info.orientation);
		if (media_info.gpslatitude)
			printf ("GPS:	%s\n", media_info.gpslatitude);
		if (media_info.make)
			printf ("make:	%s\n", media_info.make);

		printf ("\nbitrate:		"LONG_INT_FORMAT"\n", media_info.bit_rate);
	}
}

static int mp_info(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc > 1) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			print_media_info(mp);
		}
	} else {
		list = g_plist;
		while (list) {
			print_media_info((mediaplayer *)list->data);
			list = glist_next(list);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_time(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;
	int64_t position = 0;
	int64_t duration = 0;

	pthread_mutex_lock(&g_mutex);

	if (argc > 1) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			position = hcplayer_get_position(mp->player);
			duration = hcplayer_get_duration(mp->player);
			printf("uri: %s:\n", mp->uri);
			printf("curtime/duration %lld ms/%lld ms\n",
				position, duration);
		}
	} else {
		list = g_plist;
		while (list) {
			mp = (mediaplayer *)list->data;
			position = hcplayer_get_position(mp->player);
			duration = hcplayer_get_duration(mp->player);
			//printf("uri: %s:\n", mp->uri);
			printf("\033[1A");
			fflush(stdout);
			printf("\033[K");
			fflush(stdout);
			printf("pos/dur %8lld.%03llds/%8lld.%03llds\n",
				position/1000, position%1000, duration/1000, duration%1000);
			fflush(stdout);
			list = glist_next(list);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_rate(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc >= 3) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_set_speed_rate(mp->player, atof(argv[2]));
		}
	} else if (glist_length(g_plist) == 1 && argc == 2) {
		mp = glist_first(g_plist)->data;
		hcplayer_set_speed_rate(mp->player, atof(argv[1]));
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_log(int argc, char *argv[])
{
	if (argc < 2) {
		return -1;
	}

	hcplayer_change_log_level(atoi(argv[1]));

	return 0;
}

static int mp_deinit(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	g_mpabort = 1;
	g_mp_info = 0;

	mp_stop(0, NULL);

	hcplayer_deinit();

	if (msg_rcv_thread_id){
		HCPlayerMsg msg;
		msg.type = HCPLAYER_MSG_UNDEFINED;
#ifdef __linux__
		if (g_msgid >= 0) {
			msgsnd(g_msgid, (void *)&msg, sizeof(HCPlayerMsg) - sizeof(msg.type), 0);
		}
#else
		if (g_msgid) {
			xQueueSendToBack((QueueHandle_t)g_msgid, &msg, 0);
		}
#endif
		pthread_join(msg_rcv_thread_id, NULL);
		msg_rcv_thread_id = 0;
	}

#ifdef __linux__
	if (g_msgid >= 0) {
		msgctl(g_msgid,IPC_RMID,NULL);
		g_msgid = -1;
	}
#else
	if (g_msgid) {
		vQueueDelete(g_msgid);
		g_msgid = NULL;
	}
#endif

	if (get_info_thread_id){
		pthread_join(get_info_thread_id, NULL);
		get_info_thread_id = 0;
	}

	if (g_mp) {
		free(g_mp);
		g_mp = NULL;
	}

	if (g_vtranscode_path) {
		free(g_vtranscode_path);
	}

#ifndef __linux__
	*((uint32_t *)0xb8808300) |= 0x1;
#endif

	return 0;
}

int mp_set_dis_onoff(bool on_off)
{
    int fd = -1;
    struct dis_win_onoff winon = { 0 };
    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0)
    {
        return -1;
    }
    winon.distype = DIS_TYPE_HD;
    winon.layer = DIS_LAYER_MAIN;
    winon.on = on_off ? 1 : 0;
    ioctl(fd , DIS_SET_WIN_ONOFF , &winon);
    close(fd);
    return 0;
}

static int mp_debug(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	ffplayer_debug();
	return 0;
}

static int mp_init(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (!g_mp) {
		g_mp = malloc(sizeof(mediaplayer));
		if (!g_mp) {
			return -1;
		}
		memset(g_mp, 0, sizeof(mediaplayer));
		set_i2so_volume(10);
		mp_set_dis_onoff(false);
	}

#ifdef __linux__
	if (g_msgid < 0) {
		g_msgid = msgget(MKTAG('h','c','p','l'), 0666 | IPC_CREAT);
		if (g_msgid < 0) {
			printf ("create msg queue failed\n");
			mp_deinit(0, NULL);
			return -1;
		}
	}
#else
	if (!g_msgid) {
		g_msgid = xQueueCreate(( UBaseType_t )configPLAYER_QUEUE_LENGTH,
			sizeof(HCPlayerMsg));
		if (!g_msgid) {
			printf ("create msg queue failed\n");
			mp_deinit(0, NULL);
			return -1;
		}
	}
#endif

	g_multi_ins = 0;

	g_mpabort = 0;

	if (!msg_rcv_thread_id)
	{
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 0x2000);
		if(pthread_create(&msg_rcv_thread_id, &attr, msg_recv_thread, NULL)) {
			mp_deinit(0, NULL);
			return -1;
		}
	}

	hcplayer_init(LOG_WARNING);

#ifndef __linux__
	*((uint32_t *)0xb8808300) &= 0xfffffffe;
	//console_run_cmd("nsh mw 0xb8808300=0x00040000 4");
#endif
	return 0;
}

static int mp_multi(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	g_loop_play =0;
	g_multi_ins = true;
	printf("enter multi-instance mode\n");
	return 0;
}

static int mp_single(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	g_loop_play = 1;
	g_multi_ins = false;
	printf("exit multi-instance mode\n");
	return 0;
}

static int mp_pic_mode(int argc, char *argv[])
{
	int opt;
	opterr = 0;
	optind = 0;

	while ((opt = getopt(argc, &argv[0], "e:i:d:b:")) != EOF) {
		switch (opt) {
		case 'i':
			g_gif_interval = atoi(optarg);
			printf("g_gif_interval %d\n", g_gif_interval);
			break;
		case 'd':
			g_pic_show_duration = atoi(optarg);
			printf("g_pic_show_duration %d\n", g_pic_show_duration);
			break;
		case 'b':
			g_pic_bg = atoi(optarg);
			printf("g_pic_bg %d\n", g_pic_bg);
			break;

		default:
			printf("pic_mode -i 100 -d 3000 -b 1\n"
				"-i int,ms,gif frame interval\n"
				"-d int,ms,pic stay time\n"
				"-b alpha blend mode\n");
			break;
		}
	}

	return 0;
}

static int mp_pic_effect(int argc, char *argv[])
{
	if (argc < 5) {
		printf("need 4 args\n");
		return -1;
	}

	g_img_effect.mode = atoi(argv[1]);
	if (g_img_effect.mode >= IMG_SHOW_MODE_MAX) {
		return -1;
	}
	g_img_effect.mode_param.brush_param.direction = atoi(argv[2]);
	g_img_effect.mode_param.brush_param.type = atoi(argv[3]);
	g_img_effect.mode_param.brush_param.time = atoi(argv[4]);

	return 0;
}

static int mp_pic_effect_enable(int argc, char *argv[])
{
	int vidsink_fd;
	int enable;

	if (argc >= 2) {
		vidsink_fd = open("/dev/vidsink", O_WRONLY);
		if (vidsink_fd < 0) {
			return -1;
		}

		enable = atoi(argv[1]);
		if (enable) {
			ioctl(vidsink_fd, VIDSINK_ENABLE_IMG_EFFECT, 0);
		} else {
			ioctl(vidsink_fd, VIDSINK_DISABLE_IMG_EFFECT, 0);
		}

		close(vidsink_fd);

		return 0;
	}

	return -1;
}

static int scan_dir(char *path)
{
	int ret = 0;
	DIR *dirp;
	struct dirent *entry;
	char item_path[512];
	char *uri;
	int uri_len;
	int len = strlen(path);

	//printf("scan %s\n", path);
	if ((dirp = opendir(path)) == NULL) {
		ret = -1;
		return ret;
	}

	while (1) {
		entry = readdir(dirp);
		if (!entry)
			break;
		//printf("entry->d_name %s, entry->d_type %d\n", entry->d_name, entry->d_type);

		if(entry->d_name[0] == '.'){
			continue;
		}

		if (SCAN_SUB_DIR && entry->d_type == 4){//S_ISDIR(entry->d_type)){
			memset(item_path, 0, 512);
			strcpy(item_path, path);
			if (item_path[len - 1] != '/') {
				item_path[len] = '/';
			}
			strcpy(item_path + strlen(item_path), entry->d_name);
			scan_dir(item_path);
		} else if (entry->d_type != 4) {
			uri_len = len + strlen(entry->d_name) + 1;
			if(path[len - 1] != '/'){
				uri_len++;
			}

			uri = malloc(uri_len);
			if(!uri){
				ret = 0;
				goto end;
			}
			memset(uri, 0, uri_len);
			strcpy(uri, path);
			if (uri[len - 1] != '/') {
				uri[len] = '/';
			}
			strcpy(uri + strlen(uri), entry->d_name);
			printf("add %s\n", uri);
			url_list = glist_append(url_list, (void *)uri);
			if (glist_length(url_list) >= MAX_SCAN_FILE_LIST_LEN) {
				break;
			}
		}
	}

end:
	closedir(dirp);

	return ret;
}

static int mp_scan(int argc, char *argv[])
{
	DIR *dirp = NULL;

	if (argc < 2) {
		return -1;
	}

	printf("try open dir %s\n", argv[1]);
	dirp = opendir(argv[1]);
	if (argc >= 3) {
		g_sync_mode = atoi(argv[2]);
	}

	if (dirp) {
		g_multi_ins = false;
		mp_stop(0, NULL);
		closedir(dirp);

		pthread_mutex_lock(&g_mutex);

		while (url_list) {
			if (url_list->data)
				free(url_list->data);
			url_list = glist_delete_link(url_list, url_list);
		}

		scan_dir(argv[1]);
		if (url_list) {
			char *uri = NULL;

			g_cur_ply_idx = 0;
			uri = (char *)glist_nth_data(url_list, g_cur_ply_idx);
			if (uri) {
				printf("uri %s sync_type %d\n",uri, g_sync_mode);
				play_uri(uri);
			}
		}

		printf("scan done, url_list len %d\n", (int)glist_length(url_list));
		pthread_mutex_unlock(&g_mutex);
	} else {
		printf("open dir %s failed\n", argv[1]);
	}

	return 0;
}

static int mp_next(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	mp_stop(0, NULL);

	pthread_mutex_lock(&g_mutex);
	play_next_uri();
	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_prev(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	mp_stop(0, NULL);

	pthread_mutex_lock(&g_mutex);
	play_prev_uri();
	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_volume(int argc, char *argv[])
{
	uint8_t volume = 0;

	if (argc < 2) {
		return -1;
	}

	volume = atoi(argv[1]);

	return set_i2so_volume(volume);
}

static int mp_i2so_mute(int argc, char *argv[])
{
	bool mute = 0;

	if (argc < 2) {
		return -1;
	}

	mute = atoi(argv[1]);

	return set_i2so_mute(mute);
}

static int mp_i2so_gpio_mute(int argc, char *argv[])
{
	bool mute = 0;

	if (argc < 2) {
		return -1;
	}

	mute = atoi(argv[1]);

	return set_i2so_gpio_mute(mute);
}

static int mp_set_twotone(int argc, char *argv[])
{
	int opt;
	int snd_fd = -1;
	opterr = 0;
	optind = 0;
	struct snd_twotone tt = {0};
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("twotone open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	while ((opt = getopt(argc, &argv[0], "b:t:o:m:")) != EOF) {
		switch (opt) {
		case 'b':
			tt.bass_index = atoi(optarg);
			printf("twotone bass_index %d\n", tt.bass_index);
			break;
		case 't':
			tt.treble_index = atoi(optarg);
			printf("twotone treble_index %d\n", tt.treble_index);
			break;
		case 'o':
			tt.onoff = atoi(optarg);
			printf("twotone onoff %d\n", tt.onoff);
			break;
		case 'm':
			tt.tt_mode = atoi(optarg);
			printf("twotone mode %d\n", tt.tt_mode);
			break;

		default:
			break;
		}
	}

	ioctl(snd_fd, SND_IOCTL_SET_TWOTONE, &tt);
	close(snd_fd);
	return 0;
}

static int mp_set_lr_balance(int argc, char *argv[])
{
	int opt;
	int snd_fd = -1;
	opterr = 0;
	optind = 0;
	struct snd_lr_balance lr = {0};
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("lr_balance open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	while ((opt = getopt(argc, &argv[0], "i:o:")) != EOF) {
		switch (opt) {
		case 'i':
			lr.lr_balance_index = atoi(optarg);
			printf("lr balance index %d\n", lr.lr_balance_index);
			break;
		case 'o':
			lr.onoff = atoi(optarg);
			printf("lr balance onoff %d\n", lr.onoff);
			break;
		default:
			break;
		}
	}
	ioctl(snd_fd, SND_IOCTL_SET_LR_BALANCE, &lr);
	close(snd_fd);
	return 0;
}

static int mp_set_audio_eq6(int argc, char *argv[])
{
	int opt;
	int snd_fd = -1;
	opterr = 0;
	optind = 0;
	struct snd_audio_eq6 eq6 = {0};
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("audio eq6 open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	while ((opt = getopt(argc, &argv[0], "o:m:")) != EOF) {
		switch (opt) {
		case 'o':
			eq6.onoff = atoi(optarg);
			printf("audio eq6 onoff %d\n", eq6.onoff);
			break;
		case 'm':
			eq6.mode = atoi(optarg);
			printf("audio eq6 mode %d\n", eq6.mode);
			break;
		default:
			break;
		}
	}
	ioctl(snd_fd, SND_IOCTL_SET_EQ6, &eq6);
	close(snd_fd);
	return 0;
}

static void *get_info_thread(void *arg)
{
	(void)arg;

	while(g_mp_info) {
		mp_time(0, 0);
		usleep(g_mp_info_interval*1000);
		//mp_info(0, 0);
		//usleep(g_mp_info_interval*1000);
	}
	return NULL;
}

static int mp_loop_info(int argc, char *argv[])
{
	if (argc > 1) {
		g_mp_info_interval = atoi(argv[1]);//ms
	}
	if (!get_info_thread_id) {
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 0x2000);
		g_mp_info = true;
		if(pthread_create(&get_info_thread_id, &attr, get_info_thread, NULL)) {
			mp_deinit(0, NULL);
			return -1;
		}
	}

	return 0;
}

static int mp_remove_loop_info(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	g_mp_info = false;
	pthread_join(get_info_thread_id, NULL);
	get_info_thread_id = 0;
	return 0;
}

static void *auto_test_thread(void *arg)
{
	struct timeval tv;
	(void)arg;

	while(!g_stop_auto_switch) {
		gettimeofday(&tv, NULL);
		int ms = ((uint32_t)tv.tv_usec/1000) % 40;
		usleep(ms * 100 * 1000);
		mp_next(0, NULL);
	}

	//auto_switch_thread_id = 0;
	//pthread_detach(pthread_self ());
	//pthread_exit(NULL);
	return NULL;
}

static int mp_auto_switch(int argc, char *argv[])
{
	if (argc < 2)
		return -1;

	if (atoi(argv[1]) == 1) {
		if (!auto_switch_thread_id) {
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setstacksize(&attr, 0x2000);
			g_stop_auto_switch = 0;
			if(pthread_create(&auto_switch_thread_id, &attr, auto_test_thread, NULL)) {
				mp_deinit(0, NULL);
				return -1;
			}
		}
	} else {
		g_stop_auto_switch = 1;
		if (auto_switch_thread_id) {
			pthread_join(auto_switch_thread_id, NULL);
		}
		auto_switch_thread_id = 0;
	}

	return 0;
}

static void *auto_seek_thread(void *arg)
{
	struct timeval tv;
	mediaplayer *mp = NULL;
	(void)arg;

	while(!g_stop_auto_seek) {
		gettimeofday(&tv, NULL);
		int ms = ((uint32_t)tv.tv_usec/1000) % 40;
		usleep(ms * 100 * 1000);
		pthread_mutex_lock(&g_mutex);
		if (glist_first(g_plist) && glist_first(g_plist)->data) {
			int64_t duration;
			mp = glist_first(g_plist)->data;
			duration = hcplayer_get_duration(mp->player);
			gettimeofday(&tv, NULL);
			ms = ((uint32_t)tv.tv_usec/1000);
			hcplayer_seek(mp->player, ms * (duration / 1000));
		} else {
			//printf("mp %p\n", mp);
		}
		pthread_mutex_unlock(&g_mutex);
	}

	//auto_seek_thread_id = 0;
	//pthread_detach(pthread_self ());
	//pthread_exit(NULL);
	return NULL;
}

static int mp_auto_seek(int argc, char *argv[])
{
	if (argc < 2) {
		return -1;
	}

	if (atoi(argv[1]) == 1) {
			if (!auto_seek_thread_id) {
				pthread_attr_t attr;
				pthread_attr_init(&attr);
				pthread_attr_setstacksize(&attr, 0x2000);
				g_stop_auto_seek = 0;
				if(pthread_create(&auto_seek_thread_id, &attr, auto_seek_thread, NULL)) {
					mp_deinit(0, NULL);
					return -1;
				}
			}
	} else {
			g_stop_auto_seek = 1;
			if (auto_seek_thread_id) {
				pthread_join(auto_seek_thread_id, NULL);
			}
			auto_seek_thread_id = 0;
	}

	return 0;
}

static void change_display_rect(void * player,
	struct vdec_dis_rect *old_rect, struct vdec_dis_rect *new_rect)
{
	int n;
	int times = 50;
	struct vdec_dis_rect rect;
	struct av_area *ns = &new_rect->src_rect;
	struct av_area *nd = &new_rect->dst_rect;
	struct av_area *os = &old_rect->src_rect;
	struct av_area *od = &old_rect->dst_rect;

	for(n = 0 ; n < times ; n++) {
		rect.src_rect.x = (int)os->x + ((int)ns->x - (int)os->x) * n / times;
		rect.src_rect.y = (int)os->y + ((int)ns->y - (int)os->y) * n / times;
		rect.src_rect.w = (int)os->w + ((int)ns->w - (int)os->w) * n / times;
		rect.src_rect.h = (int)os->h + ((int)ns->h - (int)os->h) * n / times;
		rect.dst_rect.x = (int)od->x + ((int)nd->x - (int)od->x) * n / times;
		rect.dst_rect.y = (int)od->y + ((int)nd->y - (int)od->y) * n / times;
		rect.dst_rect.w = (int)od->w + ((int)nd->w - (int)od->w) * n / times;
		rect.dst_rect.h = (int)od->h + ((int)nd->h - (int)od->h) * n / times;
		hcplayer_set_display_rect(player, &rect);
		usleep(20 * 1000);
	}

	hcplayer_set_display_rect(player, new_rect);
}

static int mp_preview(int argc, char *argv[])
{
	struct vdec_dis_rect old_rect;
	struct vdec_dis_rect new_rect;
	if (argc < 9) {
		printf("too few args\n");
		return -1;
	}

	if (atoi(argv[1]) < 0 || atoi(argv[1]) > 1920 ||
		atoi(argv[2]) < 0 || atoi(argv[2]) > 1080 ||
		atoi(argv[3]) <= 0 || atoi(argv[3]) + atoi(argv[1]) > 1920 ||
		atoi(argv[4]) <= 0 || atoi(argv[4]) + atoi(argv[2]) > 1080 ||
		atoi(argv[5]) < 0 || atoi(argv[5]) > 1920 ||
		atoi(argv[6]) < 0 || atoi(argv[6]) > 1080 ||
		atoi(argv[7]) <= 0 || atoi(argv[7]) + atoi(argv[5]) > 1920 ||
		atoi(argv[8]) <= 0 || atoi(argv[8]) + atoi(argv[6]) > 1080) {
		printf("invalid args\n");
		printf("0 <= x <= 1920, 0 <= y <= 1080, 0 < x + w <= 1920, 0 < y + h <= 1080\n");
		return -1;
	}

	pthread_mutex_lock(&g_mutex);

	g_preview_enable = 1;

	memcpy(&old_rect, &g_dis_rect, sizeof(struct vdec_dis_rect));

	g_dis_rect.src_rect.x = atoi(argv[1]);
	g_dis_rect.src_rect.y = atoi(argv[2]);
	g_dis_rect.src_rect.w = atoi(argv[3]);
	g_dis_rect.src_rect.h = atoi(argv[4]);
	g_dis_rect.dst_rect.x = atoi(argv[5]);
	g_dis_rect.dst_rect.y = atoi(argv[6]);
	g_dis_rect.dst_rect.w = atoi(argv[7]);
	g_dis_rect.dst_rect.h = atoi(argv[8]);
	if (g_plist && g_plist->data) {
		int dis_fd = -1;
		dis_fd = open("/dev/dis" , O_WRONLY);
		memcpy(&new_rect, &g_dis_rect, sizeof(struct vdec_dis_rect));
		change_display_rect(((mediaplayer *)g_plist->data)->player, &old_rect, &new_rect);
		if (dis_fd >= 0) {
			struct dis_zoom dz;
			dz.distype = DIS_TYPE_HD;
			dz.layer = DIS_LAYER_MAIN;
			memcpy(&dz.src_area, &g_dis_rect.src_rect, sizeof(struct av_area));
			memcpy(&dz.dst_area, &g_dis_rect.dst_rect, sizeof(struct av_area));
			ioctl(dis_fd, DIS_SET_ZOOM, &dz);
			close(dis_fd);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_rm_preview(int argc, char *argv[])
{
	int dis_fd = -1;
	vdec_dis_rect_t dis_rect = {{0, 0, 1920, 1080},{0, 0, 1920, 1080}};
	(void)argc;
	(void)argv;

	pthread_mutex_lock(&g_mutex);
	memcpy(&g_dis_rect, &dis_rect, sizeof(vdec_dis_rect_t));
	g_preview_enable = 0;
	dis_fd = open("/dev/dis" , O_WRONLY);
	if (dis_fd >= 0) {
		struct dis_zoom dz;
		dz.distype = DIS_TYPE_HD;
		dz.layer = DIS_LAYER_MAIN;
		memcpy(&dz.src_area, &g_dis_rect.src_rect, sizeof(struct av_area));
		memcpy(&dz.dst_area, &g_dis_rect.dst_rect, sizeof(struct av_area));
		ioctl(dis_fd, DIS_SET_ZOOM, &dz);
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_full_screen(int argc, char *argv[])
{
	vdec_dis_rect_t dis_rect = {{0, 0, 1920, 1080}, {0, 0, 1920, 1080}};
	mediaplayer *mp = NULL;
	(void)argc;
	(void)argv;

	pthread_mutex_lock(&g_mutex);

	memcpy(&g_dis_rect, &dis_rect, sizeof(vdec_dis_rect_t));
	if (g_plist && g_plist->data) {
		mp = (mediaplayer *)g_plist->data;
		change_display_rect(mp->player, &g_dis_rect, &dis_rect);
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_disable_av(int argc, char *argv[])
{
	int opt;
	opterr = 0;
	optind = 0;

	while ((opt = getopt(argc, &argv[0], "a:v:")) != EOF) {
		switch (opt) {
		case 'a':
			g_disable_audio = atoi(optarg);
			break;
		case 'v':
			g_disable_video = atoi(optarg);
			break;

		default:
			break;
		}
	}

	return 0;
}

static int mp_change_rotate(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc >= 3) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_change_rotate_type(mp->player, atoi(argv[2]) % 4);
		}
	} else if (glist_length(g_plist) == 1 && argc == 2) {
		mp = glist_first(g_plist)->data;
		hcplayer_change_rotate_type(mp->player, atoi(argv[1]) % 4);
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_stop_mode(int argc, char *argv[])
{
	if (argc < 3) {
		printf("please enter: stop_mode closevp(bool) fillblack(bool);\nfor example stop_mode 0 0\n");
		return -1;
	}

	g_closevp = atoi(argv[1]);
	g_fillblack = atoi(argv[2]);

	return 0;
}

static int mp_set_decryption_key(int argc, char *argv[])
{

	if (decryption_key)
		free(decryption_key);
	decryption_key = NULL;

	if (argc < 2) {
		printf("current decryption_key removed, if need set, please enter: decryption_key c7e16c4403654b85847037383f0c2db3\n");
		return 0;
	}

	decryption_key = strdup(argv[1]);

	return 0;
}

static int mp_set_audio_track(int argc, char *argv[])
{
	int index = 0;
	mediaplayer *mp = NULL;
	int ret = 0;
	if (argc < 2) {
		printf("please enter: atrack index\n");
		return -1;
	}

	index = atoi(argv[1]);

	mp = (mediaplayer *)g_plist->data;
	int total_audio_num = -1;
	total_audio_num = hcplayer_get_audio_streams_count(mp->player);
	if (index >= 0 && total_audio_num > index) {
		HCPlayerAudioInfo audio_info = {0};
		hcplayer_get_cur_audio_stream_info(mp->player, &audio_info);
		if(index != audio_info.index) {
			int count = 0;
			while ((ret = hcplayer_change_audio_track(mp->player, index)) != 0) {
				index++;
				count++;
				if (index >= total_audio_num) {
					index = 0;
				}
				if (count > total_audio_num) {
					break;
				}
			}
			if (ret != 0) {
				hcplayer_change_audio_track(mp->player, -1);
			}
		} else {
			return 0;
		}
	} else {
		hcplayer_change_audio_track(mp->player, -1);
	}


	return 0;
}

static int mp_set_video_track(int argc, char *argv[])
{
	int index = 0;
	int ret = 0;
	mediaplayer *mp = NULL;

	if (argc < 2) {
		printf("please enter: vtrack index\n");
		return -1;
	}

	index = atoi(argv[1]);
	mp = (mediaplayer *)g_plist->data;
	int total_video_num = -1;
	total_video_num = hcplayer_get_video_streams_count(mp->player);
	if (index >= 0 && total_video_num > index) {
		HCPlayerVideoInfo video_info = {0};
		hcplayer_get_cur_video_stream_info(mp->player, &video_info);
		if(index != video_info.index) {
			int count = 0;
			while ((ret = hcplayer_change_video_track(mp->player, index)) != 0) {
				index++;
				count++;
				if (index >= total_video_num) {
					index = 0;
				}
				if (count > total_video_num) {
					break;
				}
			}
			if (ret != 0) {
				hcplayer_change_video_track(mp->player, -1);
			}
		} else {
			return 0;
		}
	} else {
		hcplayer_change_video_track(mp->player, -1);
	}

	return 0;
}

static int mp_set_subtitle_track(int argc, char *argv[])
{
	int index = 0;
	mediaplayer *mp = NULL;
	int ret = 0;
	if (argc < 2) {
		printf("please enter: strack index\n");
		return -1;
	}

	index = atoi(argv[1]);

	mp = (mediaplayer *)g_plist->data;
	int total_subtitle_num = -1;
	total_subtitle_num = hcplayer_get_subtitle_streams_count(mp->player);
	if (index >= 0 && total_subtitle_num > index) {
		HCPlayerSubtitleInfo subtitle_info = {0};
		hcplayer_get_cur_subtitle_stream_info(mp->player, &subtitle_info);
		if(index != subtitle_info.index) {
			int count = 0;
			while ((ret = hcplayer_change_subtitle_track(mp->player, index)) != 0) {
				index++;
				count++;
				if (index >= total_subtitle_num) {
					index = 0;
				}
				if (count > total_subtitle_num) {
					break;
				}
			}
			if (ret != 0) {
				hcplayer_change_subtitle_track(mp->player, -1);
			}
		} else {
			return 0;
		}
	} else {
		hcplayer_change_subtitle_track(mp->player, -1);
	}

	return 0;
}

static int mp_set_video_transcode(int argc, char *argv[])
{
	if (argc < 2) {
		printf("transcode_set b_enable b_show b_capture_one b_scale scale_factor store_path\n"
			"for_example: transcode_set 1 0 0 0 0 /media/hdd/v.mjpg\n"
			"or 'transcode_set 0'\n");
		return -1;
	}

	g_vtranscode.b_enable = atoi(argv[1]);

	if (!g_vtranscode.b_enable) {
		printf ("transcode disabled\n");
		return 0;
	} else if (g_vtranscode.b_enable && argc < 7) {
		printf("transcode_set b_enable b_show b_capture_one b_scale scale_factor store_path\n"
			"for_example: transcode_set 1 0 0 0 0 /media/hdd/v.mjpg\n"
			"or 'transcode_set 0'\n");
		g_vtranscode.b_enable = 0;
		return -1;
	}

	g_vtranscode.b_show = atoi(argv[2]);
	g_vtranscode.b_capture_one = atoi(argv[3]);
	g_vtranscode.b_scale = atoi(argv[4]);
	g_vtranscode.scale_factor = atoi(argv[5]);
	if (g_vtranscode_path)
		free(g_vtranscode_path);
	g_vtranscode_path = strdup(argv[6]);

	return 0;
}

static int mp_play_two_uri(int argc, char *argv[])
{
	HCPlayerInitArgs audio_initargs = {0}, video_initargs = {0};

	if (argc < 3) {
		return -1;
	}

	if (g_mp2) {
		hcplayer_multi_destroy(g_mp2);
	}

	audio_initargs.uri = strdup(argv[1]);
	video_initargs.uri = strdup(argv[2]);

	audio_initargs.sync_type = video_initargs.sync_type = HCPLAYER_AUDIO_MASTER;
	audio_initargs.msg_id = video_initargs.msg_id = (int)g_msgid;

	g_mp2 = hcplayer_multi_create(&audio_initargs, &video_initargs);
	if (!g_mp2) {
		return -1;
	}

	hcplayer_multi_play(g_mp2);

	return 0;
}

static int mp_stop_two_uri(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (g_mp2) {
		hcplayer_multi_destroy(g_mp2);
		g_mp2 = NULL;
		return 0;
	}

	return -1;
}

static int mp_time_two_uri(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (g_mp2) {
		printf("pos/dur: %d, %d\n", hcplayer_multi_position(g_mp2),
			hcplayer_multi_duration(g_mp2));
		return 0;
	}

	return -1;
}

static int mp_pause_two_uri(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (g_mp2) {
		return hcplayer_multi_pause(g_mp2);
	}

	return -1;
}

static int mp_resume_two_uri(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (g_mp2) {
		return hcplayer_multi_play(g_mp2);
	}

	return -1;
}

static int mp_seek_two_uri(int argc, char *argv[])
{
	if (g_mp2 && argc > 1) {
		return hcplayer_multi_seek(g_mp2, atoi(argv[1]) * 1000);
	}

	return -1;
}

#ifdef __linux__
static struct termios stored_settings;

static int dumpstack_cmd(int argc, char *argv[])
{
	long long val;
	unsigned long xHandle = 0;
	int fd;

	if (argc != 2)
		return 0;

	val = strtoll(argv[1], NULL, 16);
	xHandle = (unsigned long)val;

	fd = open("/dev/dumpstack", O_RDWR);
	if (fd < 0)
		return 0;

	ioctl(fd, DUMPSTACK_DUMP, xHandle);

	close(fd);

	return 0;
}

static int avp_cmd(int argc, char *argv[])
{
	char buf[512];
	int fd, ret, i;

	fd = open("/dev/virtuart", O_RDWR);
	if (fd < 0) {
		printf("open /dev/virtuart fail!\n");
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	for (i = 1; i < argc; i++) {
		strcat(buf, " ");
		strcat(buf, argv[i]);
	}

	strcat(buf, "\n");

	write(fd, buf, strlen(buf));
	usleep(5000);

	while ((ret = read(fd, buf, sizeof(buf))) > 0) {
		printf("%s", buf);
		memset(buf, 0, sizeof(buf));
	}
	printf("\n");

	close(fd);

	return 0;
}

static int mp_system(int argc, char *argv[])
{
	if (argc >= 2) {
		system(argv[1]);
	}
	return 0;
}

static void exit_console(int signo)
{
	(void)signo;
	mp_deinit(0, NULL);
	tcsetattr (0, TCSANOW, &stored_settings);
	exit(0);
}

int main (int argc, char *argv[])
{
	struct termios new_settings;
	FILE *fp = NULL;
	char buf[CONSOLE_MAX_CMD_BUFFER_LEN];
	struct console_cmd *cmd;

	mp_init(0, NULL);

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

	console_init("mp:");

	system("cat /dev/zero > /dev/fb0");

	console_register_cmd(NULL, "dbg", mp_debug, CONSOLE_CMD_MODE_SELF,
		"init: debug mp");
	console_register_cmd(NULL, "init", mp_init, CONSOLE_CMD_MODE_SELF,
		"init: init mp");
	console_register_cmd(NULL, "deinit", mp_deinit, CONSOLE_CMD_MODE_SELF,
		"deinit: deinit mp");
	console_register_cmd(NULL, "multi", mp_multi, CONSOLE_CMD_MODE_SELF,
		"enter multi-instance mode");
	console_register_cmd(NULL, "single", mp_single, CONSOLE_CMD_MODE_SELF,
		"exit multi-instance mode");

	console_register_cmd(NULL, "play", mp_play, CONSOLE_CMD_MODE_SELF,
		"play file_path -s sync -b buffering -d img_dis_mode -t start_time -r rotate -a athresh -e play_subtitle\n\t\t\t"
		"-s sync: 0, free run; 1, sync to stc(not support yet); 2, audio master, 3, video_master(not support yet)\n\t\t\t"
		"-b buffering: 0/1\n\t\t\t"
		"-d img_dis_mode: 1, full screen; 2, realsize; 4, auto(pillbox or letterbox)\n\t\t\t"
		"-t time: 0< time < 1, start_time = total_duration * time; time >= 1, start_time = time(seconds)\n\t\t\t"
		"-r rotate: 1, 90; 2, 180; 3, 270; 4, only enable rotate\n\t\t\t"
		"-m mirror:(not support yet)\n\t\t\t"
		"-e play_subtitle: 1, show subtitle; 0, do not show subtitle\n\t\t\t"
		"-a athresh: i2so will only keep max athresh data, unit is ms\n\n");
	console_register_cmd(NULL, "memory_play", mp_memory_play, CONSOLE_CMD_MODE_SELF,
		"memory_play local_file_path\n\t\t\t"
		"simulate memory play, will load file to memory, and then play memory with ffplayer");
	console_register_cmd(NULL, "stop_mode", mp_stop_mode, CONSOLE_CMD_MODE_SELF,
		"stop_mode closevp(bool) fillblack(bool, not support yet);\n\t\t\t"
		"for example stop_mode 0 0\n");
	console_register_cmd(NULL, "stop", mp_stop, CONSOLE_CMD_MODE_SELF,
		"stop: stop all player. \n\t\t\t"
		"If in multi mode, use 'stop /mnt/1.mkv'");
	console_register_cmd(NULL, "pause", mp_pause, CONSOLE_CMD_MODE_SELF,
		"pause: pause all player. \n\t\t\t"
		"If in multi mode, use 'pause /mnt/1.mkv'");
	console_register_cmd(NULL, "resume", mp_resume, CONSOLE_CMD_MODE_SELF,
		"resume: resume all player. \n\t\t\t"
		"If in multi mode, use 'resume /mnt/1.mkv'");
	console_register_cmd(NULL, "seek", mp_seek, CONSOLE_CMD_MODE_SELF,
		"seek time: seek to time, unit is second. 0 < time < duration \n\t\t\t"
		"If in multi mode, use 'seek /mnt/1.mkv time'");
	console_register_cmd(NULL, "rate", mp_rate, CONSOLE_CMD_MODE_SELF,
		"rate n: change play rate of the player, unit is float, n > 0 or n < 0 is both ok.\n\t\t\t"
		"If in multi mode, we can use 'rate /mnt/1.mkv n'");
	console_register_cmd(NULL, "info", mp_info, CONSOLE_CMD_MODE_SELF,
		"info: print media info of all player. \n\t\t\t"
		"If in multi mode, use 'info /mnt/1.mkv'");
	console_register_cmd(NULL, "time", mp_time, CONSOLE_CMD_MODE_SELF,
		"time: print position & duration of all player. \n\t\t\t"
		"If in multi mode, use 'time /mnt/1.mkv'");
	console_register_cmd(NULL, "rotate", mp_change_rotate, CONSOLE_CMD_MODE_SELF,
		"rotate n: 1, 90; 2, 180; 3, 270");
	console_register_cmd(NULL, "preview", mp_preview, CONSOLE_CMD_MODE_SELF,
		"preview src dst, base on 1920*1080\n\t\t\t"
		"for example: preview 0 0 1920 1080  800 450 320 180");
	console_register_cmd(NULL, "rm_preview", mp_rm_preview, CONSOLE_CMD_MODE_SELF,
		"remove preview flag");
	console_register_cmd(NULL, "full_screen", mp_full_screen, CONSOLE_CMD_MODE_SELF,
		"resume to full screen play from preview");
	console_register_cmd(NULL, "disav", mp_disable_av, CONSOLE_CMD_MODE_SELF,
		"disav -v 0 -a 1");
	console_register_cmd(NULL, "smooth", mp_smooth, CONSOLE_CMD_MODE_SELF,
		"smooth 1/0 1: smooth mode; 0:normal mode\n\t\t\t");

	console_register_cmd(NULL, "volume", mp_volume, CONSOLE_CMD_MODE_SELF,
		"volume 100 -> set volume 100\n\t\t\t");
	console_register_cmd(NULL, "mute", mp_i2so_mute, CONSOLE_CMD_MODE_SELF,
		"mute 1/0 -> 1/0 to mute i2so /unmute i2so\n\t\t\t");
	console_register_cmd(NULL, "gpio_mute", mp_i2so_gpio_mute, CONSOLE_CMD_MODE_SELF,
		"gpio mute 1/0 -> 1/0 to gpio mute i2so /unmute i2so\n\t\t\t");

	console_register_cmd(NULL, "twotone", mp_set_twotone, CONSOLE_CMD_MODE_SELF,
			"twotone -o (1/0) -m 1 -> set twodone on/off & music mode\n\t\t\t");
	console_register_cmd(NULL, "lr_balance", mp_set_lr_balance, CONSOLE_CMD_MODE_SELF,
			"lr_balance -o (1/0) -i 1 -> set lr_balance on/off & index 1 \n\t\t\t");
	console_register_cmd(NULL, "eq6", mp_set_audio_eq6, CONSOLE_CMD_MODE_SELF,
			"eq6 -o (1/0) -m 1 -> set audio eq6 on/off & mode  1 \n\t\t\t");

	console_register_cmd(NULL, "pic_mode", mp_pic_mode, CONSOLE_CMD_MODE_SELF,
		"picture_mode -i 50 -> set gif interval to 50ms\n\t\t\t"
		"picture_mode -d 3000 -> set picture show duration to 3000 ms\n\t\t\t"
		"picture_mode -b 1 -> set png backgroud, 1: white, 2: black, 3: chess\n\n");
	console_register_cmd(NULL, "pic_effect", mp_pic_effect, CONSOLE_CMD_MODE_SELF,
		"pic_effect  mode  direction  type  time\n\t\t\t"
		"mode: 0, normal; 1, shuttle; 2, brush; 3, slide; 4, ramdom; 5, fade\n\t\t\t"
		"direction: 0, up; 1, left; 2 down; 3, right\n\t\t\t"
		"time: step interval, unit is ms"
		"for example: pic_effect 1 0 0 50");
	console_register_cmd(NULL, "pic_effect_enable", mp_pic_effect_enable, CONSOLE_CMD_MODE_SELF,
		"pic_effect_en 1/pic_effect_en 0 to alloc/free pic_effect buffer");


	console_register_cmd(NULL, "scan", mp_scan, CONSOLE_CMD_MODE_SELF,
		"scan directory_path"
		"for example: 'scan /mnt/usb' will exit multi-instance mode, \n\t\t\tand scan /mnt/usb to create a list and then play list one by one\n\n");
	console_register_cmd(NULL, "n", mp_next, CONSOLE_CMD_MODE_SELF,
		"play next url in list");
	console_register_cmd(NULL, "p", mp_prev, CONSOLE_CMD_MODE_SELF,
		"play previous url in list");

	console_register_cmd(NULL, "log", mp_log, CONSOLE_CMD_MODE_SELF,
		"log level: 0, panic; 1, fatal; 2, err, 3, warn; 4, info; 5, verbose(print main flow); 6, debug(print pkt info); 7, trace(print all)\n\n");
	console_register_cmd(NULL, "loopinfo", mp_loop_info, CONSOLE_CMD_MODE_SELF,
		"loopinfo 200 -> will loop get info per 200ms");
	console_register_cmd(NULL, "rm_loopinfo", mp_remove_loop_info, CONSOLE_CMD_MODE_SELF,
		"rm_loopinfo -> will stop get info loop");
	console_register_cmd(NULL, "auto_switch", mp_auto_switch, CONSOLE_CMD_MODE_SELF,
		"auto_switch 1/0 -> 1/0 to start/stop auto change stream test");
	console_register_cmd(NULL, "auto_seek", mp_auto_seek, CONSOLE_CMD_MODE_SELF,
		"auto_seek 1/0 -> 1/0 to start/stop auto seek test");

	console_register_cmd(NULL, "atrack", mp_set_audio_track, CONSOLE_CMD_MODE_SELF,
		"atrack index;\n\t\t\t"
		"set audio play track, index range(0 ~ (hcplayer_get_audio_streams_count() - 1) \n\t\t\t"
		"info cmd can get number of audio tracks\n\n");
	console_register_cmd(NULL, "vtrack", mp_set_video_track, CONSOLE_CMD_MODE_SELF,
		"vtrack index");
	console_register_cmd(NULL, "strack", mp_set_subtitle_track, CONSOLE_CMD_MODE_SELF,
		"strack index");

	console_register_cmd(NULL, "transcode_set", mp_set_video_transcode, CONSOLE_CMD_MODE_SELF,
		"set video transcode config");

	console_register_cmd(NULL, "two_play", mp_play_two_uri, CONSOLE_CMD_MODE_SELF,
		"two_play a_uri v_uri\n");
	console_register_cmd(NULL, "two_stop", mp_stop_two_uri, CONSOLE_CMD_MODE_SELF,
		"two_stop");
	console_register_cmd(NULL, "two_time", mp_time_two_uri, CONSOLE_CMD_MODE_SELF,
		"two_time");
	console_register_cmd(NULL, "two_seek", mp_seek_two_uri, CONSOLE_CMD_MODE_SELF,
		"two_seek");
	console_register_cmd(NULL, "two_pause", mp_pause_two_uri, CONSOLE_CMD_MODE_SELF,
		"two_pause");
	console_register_cmd(NULL, "two_resume", mp_resume_two_uri, CONSOLE_CMD_MODE_SELF,
		"two_resume");
	console_register_cmd(NULL, "decryption_key", mp_set_decryption_key, CONSOLE_CMD_MODE_SELF,
		"set decryption_key, for example: decryption_key c7e16c4403654b85847037383f0c2db3");

	/* dis test */
	cmd = console_register_cmd(NULL, "dis_test", enter_dis_test, CONSOLE_CMD_MODE_SELF,
		"enter dis test");
	console_register_cmd(cmd, "aspect_ratio", aspect_test, CONSOLE_CMD_MODE_SELF,
		"aspect_ratio -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -t TV_MODE -m DISPLAY_MODE");
	console_register_cmd(cmd, "disrotate", rotate_test, CONSOLE_CMD_MODE_SELF,
		"disrotate -r rotate(1 90, 2 180,3 270) -m mirror -d DIS_TYPE");
	console_register_cmd(cmd, "tvsys" , tvsys_test , CONSOLE_CMD_MODE_SELF ,
		"tvsys -c cmd(0:DIS_SET_TVSYS  1:DIS_GET_TVSYS) -l DIS_LAYER -d DIS_TYPE -t TVTYPE -p progressive -s 1:dual out 0:single out -a dac type");
	console_register_cmd(cmd, "zoom" , zoom_test , CONSOLE_CMD_MODE_SELF ,
		"tvsys -c cmd -l DIS_LAYER -d DIS_TYPE source area:(-x -y -w -h )  dst area:(-o x-offset of the area -k y-offset of the area -j Width of the area -g Height of the area");
	console_register_cmd(cmd, "winon", winon_test , CONSOLE_CMD_MODE_SELF ,
		"winon -l DIS_LAYER(0x1 main, 0x10 auxp) -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -o ON(1 on, 0 off)");
	console_register_cmd(cmd, "venhance", venhance_test , CONSOLE_CMD_MODE_SELF ,
		"venhance -c Picture-enhancement-type(0x01 BRIGHTNESS,0x02 CONTRAST,0x04 SATURATION,0x08 SHARPNESS,0x10 HUE)-d DIS_TYPE(0 sd, 1 hd, 2 uhd) -g picture enhancement value(0-100,default 50)");
	console_register_cmd(cmd, "hanen", enhance_enable_test , CONSOLE_CMD_MODE_SELF ,
		"hanen -e ENHANCE EN(0 disable, 1 enable) -d DIS_TYPE(0 sd, 1 hd, 2 uhd)");
	console_register_cmd(cmd, "outfmt", hdmi_out_fmt_test , CONSOLE_CMD_MODE_SELF ,
		"outfmt -f HDMI_OUT_FORMT(1 YCBCR_420,2 YCBCR_422,3 YCBCR_444,4 RGB_MODE1,5 RGB_MODE2) -d DIS_TYPE(0 sd, 1 hd, 2 uhd)");
	console_register_cmd(cmd, "layerorder", layerorder_test , CONSOLE_CMD_MODE_SELF ,
		"layerorder -d 1 -o 0 -l 1 -r 2 -g 3 ->set layer order: main_layer 0, auxp_layer 1, gmas_layer 2, gmaf_layer 3");
	console_register_cmd(cmd, "bppic", backup_pic_test , CONSOLE_CMD_MODE_SELF ,"bppic  -d DIS_TYPE(0 sd, 1 hd, 2 uhd)");
	console_register_cmd(cmd, "cutoff", cutoff_test , CONSOLE_CMD_MODE_SELF ,
		"cutoff -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -c cutoff");
	console_register_cmd(cmd, "awonoff", auto_win_onoff_test , CONSOLE_CMD_MODE_SELF ,
		"awonoff -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -w WinOnOff");
	console_register_cmd(cmd, "singleout", single_output_test , CONSOLE_CMD_MODE_SELF ,
		"singleout -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -o single output");
	console_register_cmd(cmd, "keystone", keystone_param_test , CONSOLE_CMD_MODE_SELF ,
		"keystone -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -c bg color enable(0 disable 1 enable) -e keystone(0 disable 1 enable) -w up width -i down width -y bg y -b bg cb -r bg cr");
	console_register_cmd(cmd, "get_keystone", get_keystone_param_test , CONSOLE_CMD_MODE_SELF ,
		"get keystone -d DIS_TYPE(0 sd, 1 hd, 2 uhd)");
	console_register_cmd(cmd, "bgcolor", bg_color_test , CONSOLE_CMD_MODE_SELF ,
		"bgcolor -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -y color uY -b color uCb -r color uCr");
	console_register_cmd(cmd, "regdac", reg_dac_test , CONSOLE_CMD_MODE_SELF ,
		"regdac -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -t eDacType -e enable -f uDacFirst -s uDacSecond -h uDacThird -p bpregressive(1 DVI_SCAN_PROGRESSIVE,2 DVI_SCAN_INTERLACE) -v tvtype");
	console_register_cmd(cmd, "underdac", under_dac_test , CONSOLE_CMD_MODE_SELF ,
		"underdac -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -u type");

	cmd = console_register_cmd(NULL , "es_play" , enter_es_play, CONSOLE_CMD_MODE_SELF , "enter H264_es_test");
	console_register_cmd(cmd, "h264_es_test" , play_h264_es , CONSOLE_CMD_MODE_SELF , "enter H264_es_test");
	console_register_cmd(cmd, "h264_es_play" , h264_es_play , CONSOLE_CMD_MODE_SELF , "play h264 format raw data");
	console_register_cmd(cmd, "h264_es_stop" , h264_es_stop , CONSOLE_CMD_MODE_SELF , "stop h264 format raw data");

	/* vin dvp test */
	cmd = console_register_cmd(NULL, "vin_dvp", vindvp_enter, CONSOLE_CMD_MODE_SELF,
		"enter vin dvp test");
	console_register_cmd(cmd, "start", vindvp_start, CONSOLE_CMD_MODE_SELF, "vin dvp_start");
	console_register_cmd(cmd, "stop", vindvp_stop, CONSOLE_CMD_MODE_SELF, "vin dvp stop");
	console_register_cmd(cmd, "enable", vindvp_enable, CONSOLE_CMD_MODE_SELF, "vin_dvp_enable");
	console_register_cmd(cmd, "disable", vindvp_disable, CONSOLE_CMD_MODE_SELF, "vin_dvp_disable");
	console_register_cmd(cmd, "set_region", vindvp_set_combine_regin_status, CONSOLE_CMD_MODE_SELF, "vin_set_set_region");
	console_register_cmd(cmd, "capture", vindvp_capture_pictrue, CONSOLE_CMD_MODE_SELF, "vin_dvp_capture_pictrue");

	/* linux only */
	console_register_cmd(NULL, "avp", avp_cmd, CONSOLE_CMD_MODE_SELF,
		"send avp command to avp. e.g: avp | avp help | avp os top");
	console_register_cmd(NULL, "dumpstack", dumpstack_cmd, CONSOLE_CMD_MODE_SELF,
		"send avp command to avp. e.g: avp | avp help | avp os top");
	console_register_cmd(NULL, "system", mp_system, CONSOLE_CMD_MODE_SELF , "system argv[1]");

	if (argc == 2) {
		fp = fopen(argv[1], "r");
	}

	if (fp) {
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			printf("[console_run_cmd] %s", buf);
			console_run_cmd(buf);
		}
		fclose(fp);
	}

	console_start();

	exit_console(0);
	return 0;
}
#else
CONSOLE_CMD(mp, NULL, mp_init, CONSOLE_CMD_MODE_SELF,
	"enter & init mplayer")
CONSOLE_CMD(init,  "mp", mp_init, CONSOLE_CMD_MODE_SELF,
	"init mplayer")
CONSOLE_CMD(deinit, "mp", mp_deinit, CONSOLE_CMD_MODE_SELF,
	"deinit mplayer")
CONSOLE_CMD(multi, "mp", mp_multi, CONSOLE_CMD_MODE_SELF,
	"enter multi-instance mode")
CONSOLE_CMD(single, "mp", mp_single, CONSOLE_CMD_MODE_SELF,
	"exit multi-instance mode")
CONSOLE_CMD(smooth, "mp", mp_smooth, CONSOLE_CMD_MODE_SELF,
	"smooth 1/0 1: smooth mode; 0:normal mode")

CONSOLE_CMD(play, "mp", mp_play, CONSOLE_CMD_MODE_SELF,
	"play file_path -s sync -b buffering -d img_dis_mode -t start_time -r rotate -a athresh -e play_subtitle\n\t\t\t"
	"-s sync: 0, free run; 1, sync to stc(not support yet); 2, audio master, 3, video_master(not support yet)\n\t\t\t"
	"-b buffering: 0/1\n\t\t\t"
	"-d img_dis_mode: 1, full screen; 2, realsize; 4, auto(pillbox or letterbox)\n\t\t\t"
	"-t time: 0< time < 1, start_time = total_duration * time; time >= 1, start_time = time(seconds)\n\t\t\t"
	"-r rotate: 1, 90; 2, 180; 3, 270; 4, only enable rotate\n\t\t\t"
	"-m mirror:(not support yet)\n\t\t\t"
	"-e play_subtitle: 1, show subtitle; 0, do not show subtitle\n\t\t\t"
	"-a athresh: i2so will only keep max athresh data, unit is ms\n\n")
CONSOLE_CMD(memory_play, "mp", mp_memory_play, CONSOLE_CMD_MODE_SELF,
	"memory_play local_file_path\n\t\t\t"
	"simulate memory play, will load file to memory, and then play memory with ffplayer")
CONSOLE_CMD(decryption_key, "mp", mp_set_decryption_key, CONSOLE_CMD_MODE_SELF,
	"set decryption_key;\n\t\t\t"
	"for example: decryption_key c7e16c4403654b85847037383f0c2db3\n")
CONSOLE_CMD(stop_mode, "mp", mp_stop_mode, CONSOLE_CMD_MODE_SELF,
	"stop_mode closevp(bool) fillblack(bool, not support yet);\n\t\t\t"
	"for example stop_mode 0 0\n")
CONSOLE_CMD(stop, "mp", mp_stop, CONSOLE_CMD_MODE_SELF,
	"stop: stop all player. \n\t\t\t"
	"If in multi mode, we can use 'stop /mnt/1.mkv'")
CONSOLE_CMD(pause, "mp", mp_pause, CONSOLE_CMD_MODE_SELF,
	"pause: pause all player. \n\t\t\t"
	"If in multi mode, we can use 'pause /mnt/1.mkv'")
CONSOLE_CMD(resume, "mp", mp_resume, CONSOLE_CMD_MODE_SELF,
	"resume: resume all player. \n\t\t\t"
	"If in multi mode, we can use 'resume /mnt/1.mkv'")
CONSOLE_CMD(seek, "mp", mp_seek, CONSOLE_CMD_MODE_SELF,
	"seek time: seek to time, unit is second. 0 < time < duration \n\t\t\t"
	"If in multi mode, we can use 'seek /mnt/1.mkv time'")
CONSOLE_CMD(rate, "mp", mp_rate, CONSOLE_CMD_MODE_SELF,
	"rate n: change play rate of the player, unit is float, n > 0 or n < 0 is both ok.\n\t\t\t"
	"If in multi mode, we can use 'rate /mnt/1.mkv n'")
CONSOLE_CMD(info, "mp", mp_info, CONSOLE_CMD_MODE_SELF,
	"info: print media info of all player. \n\t\t\t"
	"If in multi mode, we can use 'info /mnt/1.mkv'")
CONSOLE_CMD(time, "mp", mp_time, CONSOLE_CMD_MODE_SELF,
	"time: print position & duration of all player. \n\t\t\t"
	"If in multi mode, we can use 'time /mnt/1.mkv'")
CONSOLE_CMD(rotate, "mp", mp_change_rotate, CONSOLE_CMD_MODE_SELF,
	"rotate n: 1, 90; 2, 180; 3, 270")
CONSOLE_CMD(preview, "mp", mp_preview, CONSOLE_CMD_MODE_SELF,
	"preview src dst, base on 1920*1080\n\t\t\t"
	"for example: preview 0 0 1920 1080  800 450 320 180")
CONSOLE_CMD(rm_preview, "mp", mp_rm_preview, CONSOLE_CMD_MODE_SELF,
	"remove preview flag")
CONSOLE_CMD(full_screen, "mp", mp_full_screen, CONSOLE_CMD_MODE_SELF,
	"resume to full screen play from preview")


CONSOLE_CMD(volume, "mp", mp_volume, CONSOLE_CMD_MODE_SELF,
	"volume 100 -> set volume to 100\n\n")
CONSOLE_CMD(mute, "mp", mp_i2so_mute, CONSOLE_CMD_MODE_SELF,
	"mute 1/0 -> enable/disable snd out")
CONSOLE_CMD(gpio_mute, "mp", mp_i2so_gpio_mute, CONSOLE_CMD_MODE_SELF,
	"gpio_mute 1/0 -> gpio mute enable/disable snd out")

CONSOLE_CMD(twotone, "mp", mp_set_twotone, CONSOLE_CMD_MODE_SELF,
	"twotone -o (1/0) -m 1 -> set twodone on/off & music mode\n")
CONSOLE_CMD(lr_balance, "mp", mp_set_lr_balance, CONSOLE_CMD_MODE_SELF,
	"lr_balance -o (1/0) -i 1 -> set lr_balance on/off & index 1\n")
CONSOLE_CMD(eq6, "mp", mp_set_audio_eq6, CONSOLE_CMD_MODE_SELF,
	"eq6 -o (1/0) -m 1 -> set audio eq6 on/off & mode 1\n")

CONSOLE_CMD(pic_mode, "mp", mp_pic_mode, CONSOLE_CMD_MODE_SELF,
	"pic_mode -i 50 -> set gif interval to 50ms\n\t\t\t"
	"pic_mode -d 3000 -> set picture show duration to 3000 ms\n\t\t\t"
	"pic_mode -b 1 -> set png backgroud, 1: white, 2: black, 3: chess\n\n")
CONSOLE_CMD(pic_effect, "mp", mp_pic_effect, CONSOLE_CMD_MODE_SELF,
	"pic_effect mode direction type time\n\t\t\t"
	"mode: 0, normal; 1, shuttle; 2, brush; 3, slide; 4, ramdom; 5, fade\n\t\t\t"
	"direction: 0, up; 1, left; 2 down; 3, right\n\t\t\t"
	"time: step interval, unit is ms")
CONSOLE_CMD(pic_effect_enable, "mp", mp_pic_effect_enable, CONSOLE_CMD_MODE_SELF,
	"pic_effect_en 1/pic_effect_en 0 to alloc/free buffer for pic_effect")


CONSOLE_CMD(scan, "mp", mp_scan, CONSOLE_CMD_MODE_SELF,
	"scan directory_path"
	"for example: 'scan /mnt/usb' will exit multi-instance mode, scan /mnt/usb to create a list and then play list one by one\n\n")
CONSOLE_CMD(n, "mp", mp_next, CONSOLE_CMD_MODE_SELF,
	"play next stream in scaned list")
CONSOLE_CMD(p, "mp", mp_prev, CONSOLE_CMD_MODE_SELF,
	"play previous stream in scaned list")


CONSOLE_CMD(log, "mp", mp_log, CONSOLE_CMD_MODE_SELF,
	"log level: 0, panic; 1, fatal; 2, err, 3, warn; 4, info; 5, verbose(print main flow); 6, debug(print pkt info); 7, trace(print all)\n\n")
CONSOLE_CMD(loopinfo, "mp", mp_loop_info, CONSOLE_CMD_MODE_SELF,
	"loopinfo 200 -> will loop get info per 200ms")
CONSOLE_CMD(rm_loopinfo, "mp", mp_remove_loop_info, CONSOLE_CMD_MODE_SELF,
	"rm_loopinfo -> will stop get info loop")
CONSOLE_CMD(auto_switch, "mp", mp_auto_switch, CONSOLE_CMD_MODE_SELF,
	"auto_switch 1/0 -> 1/0 to start/stop auto change stream test")
CONSOLE_CMD(auto_seek, "mp", mp_auto_seek, CONSOLE_CMD_MODE_SELF,
	"auto_seek  1/0 -> 1/0 to start/stop auto seek test")

CONSOLE_CMD(atrack, "mp", mp_set_audio_track, CONSOLE_CMD_MODE_SELF,
	"atrack index;\n\t\t\t"
	"set audio play track, index range(0 ~ (hcplayer_get_audio_streams_count() - 1) \n\t\t\t"
	"info cmd can get number of audio tracks\n\n")
CONSOLE_CMD(vtrack, "mp", mp_set_video_track, CONSOLE_CMD_MODE_SELF,
	"vtrack index")
CONSOLE_CMD(strack, "mp", mp_set_subtitle_track, CONSOLE_CMD_MODE_SELF,
	"strack index")

CONSOLE_CMD(transcode_set, "mp", mp_set_video_transcode, CONSOLE_CMD_MODE_SELF,
	"set video transcode config")

CONSOLE_CMD(two_play, "mp", mp_play_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_play a_uri v_uri\n")
CONSOLE_CMD(two_stop, "mp", mp_stop_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_stop")
CONSOLE_CMD(two_time, "mp", mp_time_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_time")
CONSOLE_CMD(two_seek, "mp", mp_seek_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_seek")
CONSOLE_CMD(two_pause, "mp", mp_pause_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_pause")
CONSOLE_CMD(two_resume, "mp", mp_resume_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_resume")

CONSOLE_CMD(disav, "mp", mp_disable_av, CONSOLE_CMD_MODE_SELF,
	"disav -v 0 -a 1")

CONSOLE_CMD(showav, "mp", showlogo, CONSOLE_CMD_MODE_SELF,
	"start show av\n\n")
CONSOLE_CMD(stop_showav, "mp", stop_showlogo, CONSOLE_CMD_MODE_SELF,
	"stop show av")
CONSOLE_CMD(wait_showav, "mp", wait_showlogo, CONSOLE_CMD_MODE_SELF,
	"wait show av done and then destroy it")
CONSOLE_CMD(h264_es_test , "mp" , play_h264_es , CONSOLE_CMD_MODE_SELF , "enter H264_es_test");


CONSOLE_CMD(dis , NULL , enter_dis_test , CONSOLE_CMD_MODE_SELF , "enter dis test")
CONSOLE_CMD(tvsys , "dis" , tvsys_test , CONSOLE_CMD_MODE_SELF ,
		"tvsys -c cmd(0:DIS_SET_TVSYS  1:DIS_GET_TVSYS) -l DIS_LAYER -d DIS_TYPE -t TVTYPE -p progressive")
CONSOLE_CMD(zoom , "dis" , zoom_test , CONSOLE_CMD_MODE_SELF ,
		"tvsys -l DIS_LAYER -d DIS_TYPE source area:(-x -y -w -h )  dst area:(-o x-offset of the area -k y-offset of the area -j Width of the area -g Height of the area)")
CONSOLE_CMD(aspect , "dis" , aspect_test , CONSOLE_CMD_MODE_SELF ,
		 "aspect -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -t TV_MODE -m DISPLAY_MODE")
CONSOLE_CMD(disrotate , "dis" , rotate_test , CONSOLE_CMD_MODE_SELF ,
		 "disrotate -r rotate(1 90, 2 180,3 270)  -m mirror -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(winon , "dis" , winon_test , CONSOLE_CMD_MODE_SELF ,
		 "winon -l DIS_LAYER(0x1 main, 0x10 auxp) -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -o ON(1 on, 0 off)")
CONSOLE_CMD(venhance , "dis" , venhance_test , CONSOLE_CMD_MODE_SELF ,
	"venhance -c Picture enhancement type(0x01 BRIGHTNESS,0x02 CONTRAST,0x04 SATURATION,0x08 SHARPNESS,0x10 HUE)-d DIS_TYPE(0 sd, 1 hd, 2 uhd) -g picture enhancement value(0-100,default 50)")
CONSOLE_CMD(hanen , "dis" , enhance_enable_test , CONSOLE_CMD_MODE_SELF ,
	"hanen -e ENHANCE EN(0 disable, 1 enable) -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(outfmt , "dis" , hdmi_out_fmt_test , CONSOLE_CMD_MODE_SELF ,
	"outfmt -f HDMI OUT FORMT(1 YCBCR_420,2 YCBCR_422,3 YCBCR_444,4 RGB_MODE1,5 RGB_MODE2) -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(layerorder , "dis" , layerorder_test , CONSOLE_CMD_MODE_SELF ,
	"layerorder -d 1 -o 0 -l 1 -r 2 -g 3 ->set layer order: main_layer 0, auxp_layer 1, gmas_layer 2, gmaf_layer 3")
CONSOLE_CMD(bppic , "dis" , backup_pic_test , CONSOLE_CMD_MODE_SELF ,"bppic  -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(cutoff , "dis" , cutoff_test , CONSOLE_CMD_MODE_SELF ,
	"cutoff -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -c cutoff")
CONSOLE_CMD(awonoff , "dis" , auto_win_onoff_test , CONSOLE_CMD_MODE_SELF ,
	"awonoff -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -w WinOnOff")
CONSOLE_CMD(singleout , "dis" , single_output_test , CONSOLE_CMD_MODE_SELF ,
	"singleout -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -o single output")
CONSOLE_CMD(keystone, "dis" , keystone_param_test , CONSOLE_CMD_MODE_SELF ,
	"keystone -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -c bg color enable(0 disable 1 enable) -e keystone(0 disable 1 enable) -w up width -i down width -y bg y -b bg cb -r bg cr")
CONSOLE_CMD(get_keystone, "dis" , get_keystone_param_test , CONSOLE_CMD_MODE_SELF, "get keystone -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(bgcolor , "dis" , bg_color_test , CONSOLE_CMD_MODE_SELF ,
	"bgcolor -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -y color uY -b color uCb -r color uCr")
CONSOLE_CMD(regdac , "dis" , reg_dac_test , CONSOLE_CMD_MODE_SELF ,
	"regdac -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -t eDacType -e enable -f uDacFirst -s uDacSecond -h uDacThird -p bpregressive(1 DVI_SCAN_PROGRESSIVE,2 DVI_SCAN_INTERLACE) -v tvtype")
CONSOLE_CMD(underdac , "dis" , under_dac_test , CONSOLE_CMD_MODE_SELF ,
	"underdac -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -u type")
CONSOLE_CMD(cb , "dis" , miracast_vscreen_cb_test , CONSOLE_CMD_MODE_SELF ,
	"cb (miracast vscreen cb) -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -o 1:on 0:off")
CONSOLE_CMD(getdisplayinfo , "dis" , dis_get_display_info_test , CONSOLE_CMD_MODE_SELF ,
                "getdisplayinfo -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(getbuf , "dis" , dis_get_video_buf_test , CONSOLE_CMD_MODE_SELF ,
                "get display buf in rgb")
CONSOLE_CMD(dump , "dis" , dis_dump , CONSOLE_CMD_MODE_SELF ,
                "y:buf_y c:buf_c s:size)")
CONSOLE_CMD(dbg , "mp" , mp_debug , CONSOLE_CMD_MODE_SELF ,
	"show mp info")
#endif
