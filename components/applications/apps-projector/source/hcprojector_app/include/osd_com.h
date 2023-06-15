#ifndef __OSD_COM_H__
#define __OSD_COM_H__

//#include "list.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/font/lv_font.h"
#include "lan_str_id.h"
#include "app_config.h"
#include "hcstring_id.h"
#ifdef __cplusplus
extern "C" {
#endif

#define GET_LABEL_ID(i) (((i)&0XFFFF)>>4)
#define GET_FONT_ID(i) ((i)&0xf)
#define STORE_LABEL_AND_FONT_ID(l_id, f_id) (((l_id)<<4)|(f_id))//左边12存储label_id,4028个label_id，右边4位存储font_id,16种字库大小
#define STR_NONE 0

typedef enum{
    LANGUAGE_ENGLISH,
    LANGUAGE_CHINESE,    
    //LANGUAGE_FRENCH,
 //   LANGUAGE_SPANISH,
	LANGUAGE_MAX  // the max num of language supported
}language_type_t;

#ifdef LVGL_RESOLUTION_240P_SUPPORT
LV_FONT_DECLARE(SiYuanHeiTi_Light_3500_12_1b)
LV_FONT_DECLARE(font_china_18)
LV_FONT_DECLARE(font_china_14)
LV_FONT_DECLARE(font_china_10)
#else 
LV_FONT_DECLARE(SiYuanHeiTi_Nor_7000_28_1b)
LV_FONT_DECLARE(lv_font_montserrat_28);
LV_FONT_DECLARE(lv_font_montserrat_22);
LV_FONT_DECLARE(font_china_26)
LV_FONT_DECLARE(font_china_36)
LV_FONT_DECLARE(font_china_22)
LV_FONT_DECLARE(font_china_14)
#endif


#define FONT_SIZE_LARGE (&lv_font_montserrat_28) //large font size
#define FONT_SIZE_MID (&lv_font_montserrat_22) //middle font size
#define FONT_SIZE_SMALL (&lv_font_montserrat_18) //small font size

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

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define ENG_BIG lv_font_montserrat_18
    #define ENG_MID lv_font_montserrat_14
    #define ENG_NORMAL lv_font_montserrat_12 
    #define ENG_SMALL lv_font_montserrat_10
    #define MSGBOX_Y_OFS   -20
    #define LISTFONT_3000   SiYuanHeiTi_Light_3500_12_1b
    #define CHN_LARGE   font_china_18
    #define CHN_MID     SiYuanHeiTi_Light_3500_12_1b
    #define CHN_NORMAL  SiYuanHeiTi_Light_3500_12_1b
    #define CHN_SMALL   font_china_10
#else
    #define ENG_BIG lv_font_montserrat_36
    #define ENG_MID lv_font_montserrat_28
    #define ENG_NORMAL lv_font_montserrat_26 
    #define ENG_SMALL lv_font_montserrat_14
    #define MSGBOX_Y_OFS   -100
    #define LISTFONT_3000   SiYuanHeiTi_Nor_7000_28_1b
    #define CHN_LARGE   font_china_36
    #define CHN_MID     SiYuanHeiTi_Nor_7000_28_1b
    #define CHN_NORMAL  font_china_26
    #define CHN_SMALL   font_china_14

#endif

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
typedef void (*user_msgbox_cb)(int btn_sel, void *user_data);


lv_obj_t *obj_img_open(void *parent, void *bitmap, int x, int y);
lv_obj_t *obj_label_open(void *parent, int x, int y, int w, char *str);
void osd_draw_background(void *parent, bool if_transp);
char *osd_get_string(int string_id);

language_type_t obj_get_language_type(void);

void osd_list_ctrl_reset(obj_list_ctrl_t *list_ctrl, uint16_t count, uint16_t item_count, uint16_t new_pos);
bool osd_list_ctrl_update(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos);
bool osd_list_ctrl_update2(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos);
language_type_t osd_get_language(void);
void osd_obj_com_set(void);
bool osd_list_ctrl_shift(obj_list_ctrl_t *list_ctrl, int16_t shift, uint16_t *new_top, uint16_t *new_pos);

void win_msgbox_msg_open(int str_msg_id, uint32_t timeout, msg_timeout_func timeout_func, void *user_data);
void win_msgbox_msg_close(void);
void win_msgbox_btn_open(lv_obj_t* parent, char *str_msg, user_msgbox_cb cb, void *user_data);

void win_data_buffing_open(void *parent);
void win_data_buffing_update(int percent);
void win_data_buffing_close(void);

void key_set_group(lv_group_t *key_group);
lv_font_t *osd_font_get(int font_idx);
lv_font_t *osd_font_get_by_langid(int lang_id, int font_idx);
#define DEFAULT_LV_DISP_W 1280
#define DEFAULT_LV_DISP_H 720
int lv_xpixel_transform(int x);
int lv_ypixel_transform(int y);

typedef enum Font_size
{
    FONT_LARGE=0,
    FONT_MID,
    FONT_NORMAL,
    FONT_SMALL,
}Font_size_e;

void set_label_text2(lv_obj_t* obj, uint16_t label_id, uint16_t font_id);

#ifdef LVGL_MBOX_STANDBY_SUPPORT
void win_open_lvmbox_standby(void);
void win_del_lvmbox_standby(void);
#endif

lv_obj_t *api_scr_go_back(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif