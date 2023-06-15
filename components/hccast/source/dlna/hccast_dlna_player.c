/*
 * created by Weiwen.liu
 *
 * */

#include <signal.h>
#include <sys/types.h>
#ifndef HC_RTOS
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/prctl.h>
#endif
#include <sys/ioctl.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <hcuapi/snd.h>

//for net
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <hccast_av.h>
#ifdef HC_RTOS
#include <dlna/dlna_api.h>
#else
#include <hccast/dlna_api.h>
#endif
#include <hccast_media.h>
#include "hccast_dlna_player.h"
#include <hccast_log.h>
#include <hccast_wifi_mgr.h>
#include <hccast_dlna.h>

#define LOG_TAG "ffplayer"

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#if 0
#define player_func_wrapper(func) do{Log_info(LOG_TAG, "enter:"#func);func();Log_info(LOG_TAG, "leave:"#func);}while(0)
#define player_func_wrapper_1(func, p1) do{Log_info(LOG_TAG, "enter:"#func);func(p1);Log_info(LOG_TAG, "leave:"#func);}while(0)
#define player_func_wrapper_2(func, p1, p2) do{Log_info(LOG_TAG, "enter:"#func);func(p1, p2);Log_info(LOG_TAG, "leave:"#func);}while(0)
#define player_func_wrapper_3(func, p1, p2, p3) do{Log_info(LOG_TAG, "enter:"#func);func(p1, p2, p3);Log_info(LOG_TAG, "leave:"#func);}while(0)
#define player_func_wrapper_1_ret(func, p1, r) do{Log_info(LOG_TAG, "enter:"#func);r = func(p1);Log_info(LOG_TAG, "leave:"#func);}while(0)
#else
#define player_func_wrapper(func) do{func();}while(0)
#define player_func_wrapper_1(func, p1) do{func(p1);}while(0)
#define player_func_wrapper_2(func, p1, p2) do{func(p1, p2);}while(0)
#define player_func_wrapper_3(func, p1, p2, p3) do{func(p1, p2, p3);}while(0)
#define player_func_wrapper_1_ret(func, p1, r) do{r = func(p1);}while(0)
#endif

#define DLNA_DEBUG_ENABLE
#ifdef DLNA_DEBUG_ENABLE
#define DLNA_AP_DEBUG printf

#else
#define DLNA_AP_DEBUG
#endif


static uint8_t m_volume = 50;
static bool m_mute = false;
static int64_t last_duration = 0;
static int64_t last_position = 0;
static int PlayStatus = MPLAYER_STOP;
static void *m_player;
static int m_player_idx = -1;
static int m_msgid  = -1;//non block msg
static pthread_t m_thread = 0;
static dlna_renderer_observer_st *m_observer = NULL;
static pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;

#define LOCK() do{pthread_mutex_lock(&m_mutex);}while(0)
#define UNLOCK() do{pthread_mutex_unlock(&m_mutex);}while(0)


static double buffer_duration = 0.0; /* Buffer disbled by default, see #182 */
extern hccast_dlna_event_callback dlna_callback;

static void set_play_status_lock(int status);
static int __play_media(const char *uri, const char *meta);

const static char mime_support_list[][64] =
{
    //"audio/mkv",
    "audio/quicktime",
    "audio/mpegurl",
    "audio/mpeg",
    "audio/mpeg3",
    "audio/mp3",
    //"audio/mp4",
    //"audio/basic",
    //"audio/midi",
    //"audio/ulaw",
    "audio/ogg",
    //"audio/DVI4",
    //"audio/G722",
    //"audio/G723",
    //"audio/G726-16",
    //"audio/G726-24",
    //"audio/G726-32",
    //"audio/G726-40",
    //"audio/G728",
    //"audio/G729",
    //"audio/G729D",
    //"audio/G729E",
    //"audio/GSM",
    //"audio/GSM-EFR",
    //"audio/L8",
    //"audio/L16",
    //"audio/LPC",
    //"audio/MPA",
    //"audio/PCMA",
    //"audio/PCMU",
    //"audio/QCELP",
    //"audio/RED",
    //"audio/VDVI",
    //"audio/ac3",
    //"audio/vorbis",
    //"audio/speex",
    //"audio/flac",
    //"audio/ape",
    //"audio/x-ape",
    "audio/x-flac",
    //"audio/x-aiff",
    //"audio/x-pn-realaudio",
    //"audio/x-realaudio",
    //"audio/x-wav",
    //"audio/x-matroska",
    //"audio/x-ms-wma",
    "audio/x-mpegurl",
    "audio/x-aac",
    //"application/x-shockwave-flash",
    "application/ogg",
    //"application/sdp",
    //"image/gif",
    "image/jpeg",
    //"image/ief",
    "image/png",
    //"image/tiff",
    "video/avi",
    //"video/divx",
    "video/mpeg",
    //"video/fli",
    "video/flv",
    "video/quicktime",
    //"video/vnd.vivo",
    //"video/vc1",
    //"video/ogg",
    "video/mp4",
    "video/mkv",
    //"video/BT656",
    //"video/CelB",
    //"video/JPEG",
    //"video/H261",
    //"video/H263",
    //"video/H263-1998",
    //"video/H263-2000",
    //"video/MPV",
    "video/MP2T",
    //"video/MP1S",
    //"video/MP2P",
    //"video/BMPEG",
    //"video/xvid",
    //"video/x-divx",
    //"video/x-matroska",
    "video/x-mkv",
    "video/x-ms-wmv",
    "video/x-ms-avi",
    "video/x-flv",
    //"video/x-fli",
    "video/x-ms-asf",
    //"video/x-ms-asx",
    //"video/x-ms-wmx",
    //"video/x-ms-wvx",
    //"video/x-msvideo",
    //"video/x-xvid",
    "video/3gpp",
    "video/x-vnd.on2.vp8",
    "video/webm",
};

struct MetaType_t
{
    char ext[16];
    char mimeType[32];
    hccast_media_type_e type;
}
MetaContentType[] =
{
    ".mpeg",    "video/mpeg",               HCCAST_MEDIA_MOVIE,
    ".mkv",     "video/x-mkv",              HCCAST_MEDIA_MOVIE,
    ".mkv",     "video/webm",               HCCAST_MEDIA_MOVIE,
    ".ts",      "video/vnd.dlna.mpeg-tts",  HCCAST_MEDIA_MOVIE,
    ".mp4",     "video/x-ms-bye3",          HCCAST_MEDIA_MOVIE,
    ".3gpp",    "video/3gpp",               HCCAST_MEDIA_MOVIE,
    ".avi",     "video/avi",                HCCAST_MEDIA_MOVIE,
    ".flv",     "video/flv",                HCCAST_MEDIA_MOVIE,
    ".wmv",     "video/x-ms-wmv",           HCCAST_MEDIA_MOVIE,
    ".mov",     "video/quicktime",          HCCAST_MEDIA_MOVIE,
    ".m4v",     "video/x-m4v",              HCCAST_MEDIA_MOVIE,
    ".asf",     "video/x-ms-asf",           HCCAST_MEDIA_MOVIE,
    ".asf",     "video/x-ms-asf-plugin",    HCCAST_MEDIA_MOVIE,
    ".wvx",     "video/x-ms-wvx",           HCCAST_MEDIA_MOVIE,
    ".dat",     "video/mp2t",               HCCAST_MEDIA_MOVIE,
    ".mp4",     "video/mp2p",               HCCAST_MEDIA_MOVIE,
    ".mkv",     "video/x-matroska",         HCCAST_MEDIA_MOVIE,
    ".mp4",     "video/mp4",                HCCAST_MEDIA_MOVIE,
    ".mkv",     "video/mkv",                HCCAST_MEDIA_MOVIE,
    ".ts",      "video/MP2T",               HCCAST_MEDIA_MOVIE,
    ".avi",     "video/x-ms-avi",           HCCAST_MEDIA_MOVIE,
    ".flv",     "video/x-flv",              HCCAST_MEDIA_MOVIE,
    ".vp8",     "video/x-vnd.on2.vp8",      HCCAST_MEDIA_MOVIE,
    ".mpeg",    "application/vnd.apple.mpeg", HCCAST_MEDIA_MOVIE,
    ".flv",     "video/*",                  HCCAST_MEDIA_MOVIE, //for huya app
    ".mp4",     "video/*",                  HCCAST_MEDIA_MOVIE, //for renrenshipin app
    ".mp4",     "application/vnd.app",      HCCAST_MEDIA_MOVIE, //for web video caster

    ".mp3",     "audio/mpeg",               HCCAST_MEDIA_MUSIC,
    ".mp3",     "audio/mp3",                HCCAST_MEDIA_MUSIC,
    ".ogg",     "audio/x-ogg",              HCCAST_MEDIA_MUSIC,
    ".ogg",     "audio/ogg",                HCCAST_MEDIA_MUSIC,
    ".aac",     "audio/x-aac",              HCCAST_MEDIA_MUSIC,
    ".aac",     "audio/aac",                HCCAST_MEDIA_MUSIC,
    ".aac",     "audio/quicktime",          HCCAST_MEDIA_MUSIC,
    ".wav",     "audio/wav",                HCCAST_MEDIA_MUSIC,
    ".flac",    "audio/x-flac",             HCCAST_MEDIA_MUSIC,
    ".flac",    "audio/flac",               HCCAST_MEDIA_MUSIC,

    ".jpg",     "image/jpeg",               HCCAST_MEDIA_PHOTO,
    ".jpeg",    "image/jpeg",               HCCAST_MEDIA_PHOTO,
    ".png",     "image/png",                HCCAST_MEDIA_PHOTO,
    ".photo",     "image/*",                HCCAST_MEDIA_PHOTO,

};


static hccast_media_type_e get_media_type(const char *uri, const char *meta)
{
    hccast_media_type_e type = HCCAST_MEDIA_INVALID;
    int i = 0;
    char meta_str[32] = {0};


    if (!meta)
    {
        return type;
    }

    if (strstr(meta, "protocolInfo=\"http-get:") != NULL)
    {
        char *p = strstr(meta, "protocolInfo=\"");
        memset(meta_str, '\0', 32);
        if (sscanf(p, "protocolInfo=\"%30[^\"]\"", meta_str) == 1)//get the first protocol type
        {
            for (i = 0; i < sizeof(MetaContentType) / sizeof(struct MetaType_t); i++)
            {
                if (strcasestr(meta_str, MetaContentType[i].mimeType))
                {
                    type = MetaContentType[i].type;
                    break;
                }
            }

            if (type !=HCCAST_MEDIA_INVALID)
            {
                // find media type
            }
            // special for jinritoutiao
            else if((strstr(meta_str, "*:*/*:*") != NULL))
            {
                type = HCCAST_MEDIA_MOVIE;
            }
            //special for huaweishipin
            else if(strstr(meta_str, "*:*:*") != NULL)
            {
                if (strstr(uri, ".mp3") != NULL)
                    type = HCCAST_MEDIA_MUSIC;
                else
                    type = HCCAST_MEDIA_MOVIE;
            }
            // rtsp protocol default media type : HCCAST_MEDIA_MOVIE
            else if (strstr(uri, "rtsp://") != NULL || strstr(uri, "RTSP://") != NULL)
            {
                type = HCCAST_MEDIA_MOVIE;
            }
            else
            {
                type = HCCAST_MEDIA_MOVIE;
            }
        }
    }
    else
    {
        type = HCCAST_MEDIA_MOVIE;
        for (i = 0; i < sizeof(MetaContentType) / sizeof(struct MetaType_t); i++)
        {
            if (strcasestr(uri, MetaContentType[i].ext))
            {
                type = MetaContentType[i].type;
                break;
            }
        }
    }

    return type;
}



static int get_current_player_state(void)
{
    /*Log_info(LOG_TAG, "PlayStatus: %d", PlayStatus);*/
    return hccast_media_get_status();
}

#if 0
static void init_mime_list(void)
{
    int mime_num = sizeof(mime_support_list) / sizeof(mime_support_list[0]);
    int i = 0;
    for (i = 0; i < mime_num; i ++)
    {
        register_mime_type(mime_support_list[i]);
    }
    register_mime_type("audio/*");
}
#endif

static void output_ffplayer_set_next_uri(const char *uri, const char *meta)
{
    hccast_log(LL_INFO,"Set next uri to '%s'\n", uri);
}

//URL format: scheme://host:port/path
int output_ffplayer_check_url_legal(char* url)
{
	
	int ret = 0;
	int port_num = 80;
	char server_ip[128] = {0};
	char host_name[128] = {0};
	struct hostent *host = NULL;
	
	char* phost = NULL;
	int i = 0;
	
	phost = strstr(url,"://");
	if(phost)
	{	
		//step1: parse host:port.
		phost+=strlen("://");
		
		for(i = 0;((i < sizeof(host_name) - 1) && (phost[i] != '/' && phost[i] != '\0')); i++)
		{
			host_name[i] = phost[i];
		}

		//step2: parse if it have port num.
		char *pos = strstr(host_name, ":");
	    if (pos)
	    {
	        sscanf(pos, ":%d", &port_num);
			memcpy(server_ip,host_name,sizeof(host_name));
			char* tmp = strstr(server_ip, ":");
			if(tmp)
			{
				*tmp = '\0';
			}
	    }
		else
		{
			memcpy(server_ip,host_name,sizeof(host_name));
		}

		//step3: parse ip.
		host = gethostbyname(server_ip);
		if(host)
		{
			if(host->h_addr_list[0])
			{
				strcpy(server_ip, inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
			}	
		}
		else
		{
			return -1;
		}
		hccast_log(LL_NOTICE,"Phone Host name:%s, server_ip:%s\n", host_name,server_ip);

		//step4: whether network segment or not
		if((inet_addr(server_ip)&inet_addr(HCCAST_HOSTAP_MASK)) == (inet_addr(HCCAST_HOSTAP_IP)&inet_addr(HCCAST_HOSTAP_MASK)))
		{
			return 0;
		}
		else
		{
			return -1;
		}

	}
	else
	{
		return -1;
	}

	return ret;
}

static void output_ffplayer_set_uri(const char *uri,
                                    void* meta_cb, const char *meta)
{
	int hostap_en = 0;
    hccast_log(LL_NOTICE,"[DLNA][Set uri]to '%s', meta_cb: 0x%08x\n", uri, (uint32_t)meta_cb);

    if(strlen(uri) == 0)
    {
        return ;
    }

	if(dlna_callback)
	{
		dlna_callback (HCCAST_DLNA_GET_HOSTAP_STATE, (void*)&hostap_en, NULL);
	}

	if(hostap_en)
	{
		if(output_ffplayer_check_url_legal(uri) != 0)
		{
			if(dlna_callback)
				dlna_callback (HCCAST_DLNA_HOSTAP_MODE_SKIP_URL, NULL, NULL);
			hccast_log(LL_NOTICE,"[DLNA]:skip bad url for hostap mode\n");
			return ;
		}
	}


    hccast_media_url_t mp_url;
    memset(&mp_url, 0, sizeof(hccast_media_url_t));
    mp_url.url = uri;
    mp_url.url1 = NULL;
    mp_url.url_mode = HCCAST_MEDIA_URL_DLNA;
    mp_url.media_type = get_media_type(uri,meta);
    hccast_log(LL_NOTICE,"%s() media_type: %d\n",__func__,mp_url.media_type);

    hccast_media_seturl(&mp_url);

}


static int output_ffplayer_play(void* callback)
{
    int ret = 0;
    hccast_log(LL_NOTICE,"[DLNA][PLAY]\n");
    hccast_media_resume();
    return ret;
}

static int output_ffplayer_stop(void)
{
    hccast_log(LL_NOTICE,"[DLNA][STOP]\n");
    hccast_media_stop();
    return 0;
}

static int output_ffplayer_pause(void)
{
	hccast_log(LL_NOTICE,"[DLNA][PAUSE]\n");
    hccast_media_pause();
    return 0;
}

//milliseconds
static int output_ffplayer_seek(uint64_t position_ms)
{
    hccast_log(LL_NOTICE, "[DLNA][SEEK]position_ms: %lld\n", position_ms);
    hccast_media_seek(position_ms / 1000000);
    return 0;
}

#if 0
static const char *gststate_get_name(GstState state)
{
    switch (state)
    {
        case GST_STATE_VOID_PENDING:
            return "VOID_PENDING";
        case GST_STATE_NULL:
            return "NULL";
        case GST_STATE_READY:
            return "READY";
        case GST_STATE_PAUSED:
            return "PAUSED";
        case GST_STATE_PLAYING:
            return "PLAYING";
        default:
            return "Unknown";
    }
}
#endif

static int output_ffplayer_add_options(char *ctx)
{

    return 0;
}

//unit is milliseconds
static int output_ffplayer_get_position(uint64_t *track_duration,
                                        uint64_t *track_pos)
{
    int rc = 0;

    last_duration = hccast_media_get_duration();
    last_position = hccast_media_get_position();

    last_duration *= 1000000;
    last_position *= 1000000;

    *track_duration = last_duration;
    *track_pos = last_position;
    /*Log_info(LOG_TAG, "*track_duration: %lld, *track_pos: %lld", *track_duration, *track_pos);*/
    return rc;
}

static int output_ffplayer_get_volume(int *v)
{
    m_volume = hccast_media_get_volume();
    //DLNA_AP_DEBUG("[DLNA]Query volume fraction: %d\n", m_volume);
    *v = (int)m_volume;
    return 0;
}

static int output_ffplayer_set_volume(int value)
{
    hccast_log(LL_INFO,"[DLNA]Pre volume fraction to %d\n", m_volume);
    hccast_log(LL_INFO,"[DLNA]Set volume fraction to %d\n", value);

    //if (m_volume != value)
    {
        m_volume = (uint8_t)value;
        hccast_media_set_volume((int)value);
    }

    return 0;
}

static int output_ffplayer_get_mute(int *m)
{
    hccast_log(LL_INFO,"[DLNA]m_mute: %d\n", m_mute);
    *m = m_mute;
    return 0;
}

static int output_ffplayer_set_mute(int m)
{
    hccast_log(LL_NOTICE,"[DLNA]Set mute to %s\n", m ? "on" : "off");

    //if (m_mute != m)
    {
        m_mute = m;

        if (m)
        {
            output_ffplayer_set_volume(0);
        }
        else
        {
            output_ffplayer_set_volume(m_volume);
        }
    }

    return 0;
}

#if 0
static void prepare_next_stream(GstElement *obj, gpointer userdata)
{
    (void)obj;
    (void)userdata;

    Log_info(LOG_TAG, "about-to-finish cb: setting uri %s",
             gs_next_uri_);
    free(gsuri_);
    gsuri_ = gs_next_uri_;
    gs_next_uri_ = NULL;
    if (gsuri_ != NULL)
    {
        g_object_set(G_OBJECT(player_), "uri", gsuri_, NULL);
        if (play_trans_callback_)
        {
            // TODO(hzeller): can we figure out when we _actually_
            // start playing this ? there are probably a couple
            // of seconds between now and actual start.
            play_trans_callback_(PLAY_STARTED_NEXT_STREAM);
        }
    }
}
#endif


//extern void init_mime_list(int mime_num, char  mime_support_list[][64]);

static int output_ffplayer_init(void)
{
    //SongMetaData_init(&song_meta_);
    int minenum = sizeof(mime_support_list) / sizeof(mime_support_list[0]);

    dlna_init_mime_list(minenum, mime_support_list);

    m_volume = hccast_get_volume();//get system volume.
    hccast_log(LL_INFO,"[DLNA]m_volume %d\n", m_volume);

    if(m_volume <= 0)
    {
        m_mute = true;
    }
    else
    {
        m_mute = false;
    }

    return 0;
}

void output_ffplayer_deinit(void)
{


}


static int output_ffplayer_get_status(DmaStatus_E *status)
{
    if (!status)
        return 0;
    switch (get_current_player_state())
    {
        case HCCAST_MEDIA_STATUS_PAUSED:
            *status = DMA_STATUS_PAUSED;
            break;
        case HCCAST_MEDIA_STATUS_BUFFERING:
            *status = DMA_STATUS_BUFFERING;
            break;
        case HCCAST_MEDIA_STATUS_PLAYING:
            *status = DMA_STATUS_PLAYING ;
            break;
        case HCCAST_MEDIA_STATUS_STOP:
        default:
            *status = DMA_STATUS_STOPPED ;
    }
    return 0;
}


struct output_module output_ffplayer =
{
    .shortname = "ffplayer",
    .description = "hchip multimedia framework",
    /*.add_options = output_ffplayer_add_options,*/

    .init        = output_ffplayer_init,
    .set_uri     = output_ffplayer_set_uri,
    .set_next_uri = output_ffplayer_set_next_uri,
    .play        = output_ffplayer_play,
    .stop        = output_ffplayer_stop,
    .pause       = output_ffplayer_pause,
    .seek        = output_ffplayer_seek,

    .get_position = output_ffplayer_get_position,
    .get_volume  = output_ffplayer_get_volume,
    .set_volume  = output_ffplayer_set_volume,
    .get_mute  = output_ffplayer_get_mute,
    .set_mute  = output_ffplayer_set_mute,
    .get_status = output_ffplayer_get_status,
};
