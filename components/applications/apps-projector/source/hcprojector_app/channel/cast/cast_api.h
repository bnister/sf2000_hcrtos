/**
 * @file cast_api.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __CAST_API_H__
#define __CAST_API_H__

#ifdef DLNA_SUPPORT
#include <hccast/hccast_dlna.h>
#endif

#ifdef AIRCAST_SUPPORT
#include <hccast/hccast_air.h>
#endif

#ifdef MIRACAST_SUPPORT
#include <hccast/hccast_mira.h>
#endif

#ifdef USBMIRROR_SUPPORT
#include <hccast/hccast_um.h>
#endif

#ifdef CAST_SUPPORT
#include <hccast/hccast_scene.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    CAST_TYPE_AIRCAST = 0,
    CAST_TYPE_DLNA,
    CAST_TYPE_MIRACAST,

    CAST_TYPE_NONE,
}cast_type_t;



typedef bool (*cast_ui_wait_ready_func)(uint32_t timeout);

int cast_get_service_name(cast_type_t cast_type, char *service_name, int length);
void cast_restart_services(void);
int cast_init(void);
int cast_deinit(void);
bool cast_is_demo(void);

void cast_dlna_ui_wait_init(cast_ui_wait_ready_func ready_func);
void cast_mira_ui_wait_init(cast_ui_wait_ready_func ready_func);
void cast_air_ui_wait_init(cast_ui_wait_ready_func ready_func);
void cast_main_ui_wait_init(cast_ui_wait_ready_func ready_func);

#ifdef DLNA_SUPPORT
int hccast_dlna_callback_func(hccast_dlna_event_e event, void* in, void* out);
#else
int hccast_dlna_service_uninit(void);
int hccast_dlna_service_start(void);
int hccast_dlna_service_stop(void);
#endif

#ifdef MIRACAST_SUPPORT
int hccast_mira_callback_func(hccast_mira_event_e event, void* in, void* out);
#else
int hccast_mira_service_start(void);
int hccast_mira_service_stop(void);
int hccast_mira_player_init(void);
int hccast_mira_get_stat(void);
int hccast_mira_service_uninit(void);
#endif

#ifdef AIRCAST_SUPPORT
int hccast_air_callback_event(hccast_air_event_e event, void* in, void* out);
#else
int hccast_air_ap_mirror_stat(void);
int hccast_air_ap_audio_stat(void);
int hccast_air_service_start(void);
int hccast_air_service_stop(void);
void hccast_air_mdnssd_start(void);
void hccast_air_mdnssd_stop(void);
void hccast_air_mediaplayer_2_aircast_event(int type, void *param);
int hccast_air_ap_get_mirror_frame_num(void);
int hccast_air_service_is_start(void);
#endif


#ifdef USBMIRROR_SUPPORT
void ui_um_play_init(void);
int cast_usb_mirror_init(void);
int cast_usb_mirror_deinit(void);
int cast_usb_mirror_start(void);
int cast_usb_mirror_stop(void);
void cast_usb_mirror_rotate_init(void);
#else
/*
int hccast_um_init(void);
int hccast_um_deinit(void);
int hccast_um_param_set(hccast_um_param_t *param);
int hccast_ium_start(char *uuid, hccast_um_cb event_cb);
int hccast_ium_stop(void);
int hccast_aum_start(hccast_aum_param_t *param, hccast_um_cb event_cb);
int hccast_aum_stop(void);
*/
#endif

void restart_air_service_by_hdmi_change(void);


extern cast_ui_wait_ready_func dlna_ui_wait_ready;
extern cast_ui_wait_ready_func mira_ui_wait_ready;
extern cast_ui_wait_ready_func air_ui_wait_ready;
extern cast_ui_wait_ready_func cast_main_ui_wait_ready;


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif



