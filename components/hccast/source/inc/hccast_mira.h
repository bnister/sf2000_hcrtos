#ifndef __HCCAST_MIRACAST_SERVICE_H__
#define __HCCAST_MIRACAST_SERVICE_H__

#define MIRA_NAME_LEN (64)

typedef enum 
{
	HCCAST_MIRA_CMD_SET_RESOLUTION = 1,
} hccast_mira_cmd_e;

typedef enum 
{
	HCCAST_MIRA_RES_720P30,
	HCCAST_MIRA_RES_1080P30,
	HCCAST_MIRA_RES_480P60,
	HCCAST_MIRA_RES_VESA1400,
} hccast_mira_res_e;

typedef enum
{
    HCCAST_MIRA_SCREEN_ROTATE_0,
    HCCAST_MIRA_SCREEN_ROTATE_270,
    HCCAST_MIRA_SCREEN_ROTATE_90,
    HCCAST_MIRA_SCREEN_ROTATE_180,
} hccast_mira_rotate_type_e;

typedef enum
{
    HCCAST_MIRA_NONE = 0,
    HCCAST_MIRA_GET_DEVICE_NAME,
    HCCAST_MIRA_SSID_DONE,
    HCCAST_MIRA_CONNECT,
    HCCAST_MIRA_CONNECTED,
    HCCAST_MIRA_START_DISP,
    HCCAST_MIRA_DISCONNECT,
    HCCAST_MIRA_STOP_DISP,
    HCCAST_MIRA_GET_CUR_WIFI_INFO,
    HCCAST_MIRA_GET_MIRROR_ROTATION,
    HCCAST_MIRA_GET_MIRROR_VSCREEN_AUTO_ROTATION,
    HCCAST_MIRA_GET_FLIP_MODE,
    HCCAST_MIRA_GET_MIRROR_FULL_VSCREEN,
    HCCAST_MIRA_START_FIRST_FRAME_DISP,
    HCCAST_MIRA_SET_DIS_ZOOM_INFO,
    HCCAST_MIRA_MAX,
} hccast_mira_event_e;
typedef int (*hccast_mira_event_callback)(hccast_mira_event_e event, void* in, void* out);


typedef struct
{
    unsigned short x;		//!< Horizontal start point.
    unsigned short y;		//!< Vertical start point.
    unsigned short w;		//!< Horizontal size.
    unsigned short h;		//!< Vertical size.
} hccast_mira_av_area_t;

typedef struct
{
    hccast_mira_av_area_t src_rect;
    hccast_mira_av_area_t dst_rect;
    int dis_active_mode;
} hccast_mira_zoom_info_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * It starts the Miracast service.
 * 
 * @return 0: success; <0: failed.
 */
int hccast_mira_service_start();

/**
 * It stops the Miracast service.
 * 
 * @return 0
 */
int hccast_mira_service_stop();

/**
 * It disconnects the Miracast connection.
 * 
 * @return 0
 */
int hccast_mira_disconnect();

int hccast_mira_player_init();
int hccast_mira_get_stat(void);
int hccast_mira_service_init(hccast_mira_event_callback func);
int hccast_mira_service_uninit();
int hccast_mira_es_dump_start(char *folder);
int hccast_mira_es_dump_stop();
int hccast_mira_service_set_resolution(hccast_mira_res_e res);

// DEPRECATED. DO NOT USE THIS API PLEASE.
int hccast_mira_service_ioctl(int cmd, void *arg);

#ifdef __cplusplus
}
#endif

#endif
