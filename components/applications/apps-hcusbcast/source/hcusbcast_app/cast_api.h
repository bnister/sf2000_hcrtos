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

#ifdef USBMIRROR_SUPPORT
#include <hccast/hccast_um.h>
#endif

#include <hccast/hccast_scene.h>


#ifdef __cplusplus
extern "C" {
#endif

int cast_init(void);
int cast_deinit(void);
bool cast_is_demo(void);


#ifdef USBMIRROR_SUPPORT
int cast_usb_mirror_init(void);
int cast_usb_mirror_deinit(void);
int cast_usb_mirror_start(void);
int cast_usb_mirror_stop(void);
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


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif



