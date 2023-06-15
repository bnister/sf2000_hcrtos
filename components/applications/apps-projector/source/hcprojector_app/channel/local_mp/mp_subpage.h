/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-12-20 13:53:54
 */
#ifndef __MP_SUBPAGE_H_
#define __MP_SUBPAGE_H_

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"

#ifdef __cplusplus
extern "C" {
#endif

void sub_page_keyinput_event_cb(lv_event_t *event);
void subpage_open(void);
void subpage_close(void);






#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif

