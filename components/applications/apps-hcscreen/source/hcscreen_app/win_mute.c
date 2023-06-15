/**
 * win_mute.c
 */

#include <stdio.h>
#include <unistd.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include "obj_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key.h"
#include "media_player.h"
#include "win_mute.h"

#define WIN_MUTE_W	60
#define WIN_MUTE_H	60

bool m_mute_state = UNMUTE_STATE;
lv_obj_t *m_mute_root = NULL;
lv_obj_t *m_img_mute = NULL;

LV_IMG_DECLARE(im_mute1)

bool win_is_unmute(void)
{
	return m_mute_state;
}

void win_mute_on_off(bool show_osd)
{
	m_mute_state = !!!m_mute_state;
	printf("%s(), line:%d. m_mute_state:%d(%s)\n", __FUNCTION__, __LINE__, 
		m_mute_state, m_mute_state == UNMUTE_STATE ? "UNMUTE!":"MUTE!");

	if (!m_mute_root){
		m_mute_root = lv_obj_create(lv_scr_act());
	    lv_obj_set_size(m_mute_root, WIN_MUTE_W, WIN_MUTE_H);
		lv_obj_set_style_bg_color(m_mute_root, COLOR_DEEP_GREY, LV_PART_MAIN); //grey
		lv_obj_set_style_border_opa(m_mute_root, LV_OPA_TRANSP, LV_PART_MAIN);
	    lv_obj_align(m_mute_root, LV_ALIGN_TOP_RIGHT, -100, 40);
	    lv_obj_clear_flag(m_mute_root, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_style_radius(m_mute_root, 10, 0);

		m_img_mute = lv_img_create(m_mute_root);
		lv_img_set_src(m_img_mute, &im_mute1);
	    lv_obj_align(m_img_mute, LV_ALIGN_CENTER, 0, 0);
	}

	if (MUTE_STATE == m_mute_state)
		media_mute(true);
	else
		media_mute(false);

	if (show_osd){
		if (MUTE_STATE == m_mute_state){
			lv_obj_clear_flag(m_mute_root, LV_OBJ_FLAG_HIDDEN);
		}
		else{
			lv_obj_add_flag(m_mute_root, LV_OBJ_FLAG_HIDDEN);			
		}
	}
}

