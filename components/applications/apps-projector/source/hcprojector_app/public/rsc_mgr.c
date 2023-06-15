/*
rsc_mgr.c: use to manage the UI resource: string, font, etc
 */
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "lvgl/lvgl.h"
#include "lvgl/src/font/lv_font.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"
//#include "local_mp_ui.h"
//#include "setup.h"
#include "factory_setting.h"

#include "hcstring_id.h"
#include "../ui_rsc/string/str_english.h"
#include "../ui_rsc/string/str_chinese.h"
//#include "../ui_rsc/string/str_french.h"


typedef struct{
    uint16_t lang_id;
    uint8_t *string_array;
}string_table_t;

string_table_t multi_string_array[] = {
    {LANGUAGE_ENGLISH, strs_array_english},
    {LANGUAGE_CHINESE, strs_array_chinese},
#ifdef ARABIC_SUPPORT
//    {LANGUAGE_ARABIC, str_arabic_array},
#endif
};

uint8_t *api_rsc_string_get_by_langid(uint16_t lang_id, uint32_t string_id){
    uint32_t str_num = 0;
    uint8_t *str_array = NULL;
    uint8_t *str_data = NULL;
    uint32_t  str_count = 0;
    uint32_t offset = 0;
    uint32_t string_idx = string_id-1;
    int i;
    uint16_t language_id;

    int lang_count = sizeof(multi_string_array)/sizeof(multi_string_array[0]);
    for (i = 0; i<lang_count; i++)
    {
        if (lang_id == multi_string_array[i].lang_id)
        {
            str_array = multi_string_array[i].string_array;
            break;
        }
    }

    if (NULL == str_array)
        return NULL;

    language_id = (str_array[0] << 8) + str_array[1];
    str_num = (str_array[2] << 8) + str_array[3];
    if (string_idx >= str_num)
        return NULL;

	offset = (str_array[4 + 3 * string_idx] << 16) + (str_array[4 + 3 * string_idx + 1] << 8) + str_array[4 + 3 * string_idx + 2];
	str_data = (uint8_t*)&str_array[offset];

	return str_data; //utf-8, end by '\0'  
}

uint8_t *api_rsc_string_get(uint32_t string_id)
{
    uint16_t lang_id = projector_get_some_sys_param(P_OSD_LANGUAGE);
//    uint16_t lang_id = LANG_CHINESE;
    return api_rsc_string_get_by_langid(lang_id, string_id);

}

