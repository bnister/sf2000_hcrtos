/*
dummy_api.h
 */

#ifndef __COM_API_H__
#define __COM_API_H__

#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef BLUETOOTH_SUPPORT
typedef int (*bluetooth_callback_t)(unsigned long event, unsigned long param);
int bluetooth_init(const char *uart_path, bluetooth_callback_t callback);
int bluetooth_deinit(void);
int bluetooth_poweron(void);
int bluetooth_poweroff(void);
int bluetooth_scan(void);
int bluetooth_stop_scan(void);
int bluetooth_connect(unsigned char *mac);
int bluetooth_is_connected(void);
int bluetooth_disconnect(void);
int bluetooth_set_gpio_backlight(unsigned char value);//0 disable 1 enable
int bluetooth_set_gpio_mutu(unsigned char value);//0 disable 1 enable
int bluetooth_set_cvbs_aux_mode(void);
int bluetooth_set_cvbs_fiber_mode(void);
#endif


#ifndef DLNA_SUPPORT
#endif

#ifndef MIRACAST_SUPPORT
#endif

#ifndef AIRCAST_SUPPORT
#endif

#ifndef USBMIRROR_SUPPORT
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
