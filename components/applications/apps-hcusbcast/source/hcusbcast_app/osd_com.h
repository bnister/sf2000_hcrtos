#ifndef __OSD_COM_H__
#define __OSD_COM_H__

//#include "list.h"
#ifdef __linux__
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#else
#include <lvgl/lvgl/lvgl.h>
#include <lvgl/lvgl/src/font/lv_font.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FONT_SIZE_LARGE (&lv_font_montserrat_28) //large font size
#define FONT_SIZE_MID (&lv_font_montserrat_22) //middle font size


#define COLOR_WHITE lv_color_hex(0xFFFFFF) //white
#define COLOR_RED lv_color_hex(0xC00000) //red
#define COLOR_GREEN lv_color_hex(0x00FF00) //deep green
#define COLOR_YELLOW lv_color_hex(0xE0E000) //yellow
#define COLOR_BLUE lv_color_hex(0x4B50B0) //0x4B50B0
#define FONT_COLOR_HIGH lv_color_hex(0xE0E000) //highlight color, yelow
#define FONT_COLOR_GRAY lv_color_hex(0x7F7F7F) //grey
#define COLOR_DEEP_GREY lv_color_hex(0x303030) 
#define COLOR_FRENCH_GREY lv_color_hex(0x303841) 

extern lv_style_t m_text_style_mid_normal;
extern lv_style_t m_text_style_mid_high;
extern lv_style_t m_text_style_mid_disable;
extern lv_style_t m_text_style_left_normal;
extern lv_style_t m_text_style_left_high;
extern lv_style_t m_text_style_left_disable;
extern lv_style_t m_text_style_right_normal;
extern lv_style_t m_text_style_right_high;
extern lv_style_t m_text_style_right_disable;

#define TEXT_STY_MID_NORMAL				(&m_text_style_mid_normal)
#define TEXT_STY_MID_HIGH				(&m_text_style_mid_high)
#define TEXT_STY_MID_DISABLE			(&m_text_style_mid_disable)
#define TEXT_STY_LEFT_NORMAL			(&m_text_style_left_normal)
#define TEXT_STY_LEFT_HIGH				(&m_text_style_left_high)
#define TEXT_STY_LEFT_DISABLE			(&m_text_style_left_disable)
#define TEXT_STY_RIGHT_NORMAL			(&m_text_style_right_normal)
#define TEXT_STY_RIGHT_HIGH				(&m_text_style_right_high)
#define TEXT_STY_RIGHT_DISABLE			(&m_text_style_right_disable)


typedef void (*msg_timeout_func)(void *user_data);
void osd_draw_background(void *parent, bool if_transp);
void win_msgbox_msg_open(const char *str_msg, uint32_t timeout, msg_timeout_func timeout_func, void *user_data);
void win_msgbox_msg_close(void);
void osd_obj_com_set(void);
void win_lable_pop_msg_open(char* lable_msg);
void win_lable_pop_msg_close(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
