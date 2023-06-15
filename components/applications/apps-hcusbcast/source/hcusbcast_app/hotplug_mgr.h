/*
hotplug_mgr.h
 */

#ifndef __HOTPLUG_MGR_H__
#define __HOTPLUG_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif


void hotplug_init(void);
void hotplug_deinit(void);
int hotplug_usb_get(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

