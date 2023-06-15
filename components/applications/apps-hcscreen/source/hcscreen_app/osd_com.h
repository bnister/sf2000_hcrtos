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
#include "lan_str_id.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    LANGUAGE_ENGLISH,
    LANGUAGE_CHINESE,    
    LANGUAGE_FRENCH,
    LANGUAGE_SPANISH,
}language_type_t;

#define FONT_SIZE_LARGE (&lv_font_montserrat_28) //large font size
#define FONT_SIZE_MID (&lv_font_montserrat_22) //middle font size
//#define FONT_SIZE_MID (&SiYuanHeiTi_Light_22_1b) //middle font size

#ifdef __linux__
#define FONT_SIZE_SMALL (&lv_font_montserrat_18) //small font size
#else
#define FONT_SIZE_SMALL (&lv_font_montserrat_14) //small font size
#endif

#define COLOR_WHITE lv_color_hex(0xFFFFFF) //white
#define COLOR_RED lv_color_hex(0xC00000) //red
#define COLOR_GREEN lv_color_hex(0x00FF00) //deep green
#define COLOR_YELLOW lv_color_hex(0xE0E000) //yellow

#define COLOR_BLUE lv_color_hex(0x4B50B0) //0x4B50B0

#define FONT_COLOR_HIGH lv_color_hex(0xE0E000) //highlight color, yelow

#define FONT_COLOR_GRAY lv_color_hex(0x7F7F7F) //grey

#define COLOR_DEEP_GREY lv_color_hex(0x303030) 
//#define COLOR_FRENCH_GREY lv_color_hex(0x505050) 
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


/**
 * The structure is to adjust the position information of the list(LV widgets)
 */
#if 1
typedef struct{
    uint16_t top;        //In a page, the start index of file list
    uint16_t depth;      //In a page, the max count of list items 
    uint16_t cur_pos;        //the current index of file list
    uint16_t new_pos;        //the new index of file list
    uint16_t select;        //the select index of file list
    uint16_t count; // all the count(dirs and files) of file list
}obj_list_ctrl_t;

#else
typedef struct{
    uint16_t start;      //In a page, the start index of file list
    uint16_t end;        //In a page, the end index of file list
    uint16_t pos;        //In a page, the current position of file list. start <= pos <= end.
    uint16_t list_count; // all the count(dirs and files) of file list
}obj_list_ctrl_t;
#endif

typedef void (*msg_timeout_func)(void *user_data);


lv_obj_t *obj_img_open(void *parent, void *bitmap, int x, int y);
lv_obj_t *obj_label_open(void *parent, int x, int y, int w, char *str);
void osd_draw_background(void *parent, bool if_transp);
char *osd_get_string(int string_id);

language_type_t obj_get_language_type(void);

void osd_list_ctrl_reset(obj_list_ctrl_t *list_ctrl, uint16_t count, uint16_t item_count, uint16_t new_pos);
bool osd_list_ctrl_update(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos);
language_type_t osd_get_language(void);
void osd_obj_com_set(void);
bool osd_list_ctrl_shift(obj_list_ctrl_t *list_ctrl, int16_t shift, uint16_t *new_top, uint16_t *new_pos);

void win_msgbox_msg_open(const char *str_msg, uint32_t timeout, msg_timeout_func timeout_func, void *user_data);
void win_msgbox_msg_close(void);
int win_msgbox_btn_open(const char *str_msg, uint32_t timeout);

void win_data_buffing_open(void);
void win_data_buffing_update(int percent);
void win_data_buffing_close(void);

bool win_cast_root_wait_open(uint32_t timeout);
bool win_dlna_wait_open(uint32_t timeout);
bool win_cast_play_wait_open(uint32_t timeout);

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
