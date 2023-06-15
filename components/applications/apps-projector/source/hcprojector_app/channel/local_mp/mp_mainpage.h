/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-12-16 17:17:04
 */
#ifndef __MP_MAINPAGE_H_
#define __MP_MAINPAGE_H_

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"
#include "media_player.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum SCREEN_SUBMP{
	SCREEN_SUBMP0,
	SCREEN_SUBMP1,
	SCREEN_SUBMP2,
	SCREEN_SUBMP3,
}SCREEN_SUBMP_E;

void set_key_group(lv_group_t * group);
void main_page_keyinput_event_cb(lv_event_t *event);
void storage_icon_refresh_cb(lv_event_t *event);

int mainpage_open(void);
int mainpage_close(void);
extern lv_indev_t * indev_keypad;
media_type_t get_media_type(void);
SCREEN_SUBMP_E get_screen_submp(void);

int app_set_screen_submp(SCREEN_SUBMP_E subpage);








#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif

