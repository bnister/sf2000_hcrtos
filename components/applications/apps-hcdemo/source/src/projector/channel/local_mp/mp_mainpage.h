#ifndef __MP_MAINPAGE_H_
#define __MP_MAINPAGE_H_

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    exit_dir, 
    exit_page,
}exit_code_t ;
void set_key_group(lv_group_t * group);
void keypad_handler(uint32_t value,lv_group_t * parent_group);
void main_page_keyinput_event_cb(lv_event_t *event);
int mainpage_open(void);
int mainpage_close(void);
extern lv_indev_t * indev_keypad;
media_type_t get_media_type(void);








#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif

