/*
win_sys_info.c
*/
#include <unistd.h>
#include <pthread.h>
#include "../lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include "menu_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key.h"
#include "data_mgr.h"

#define INFO_W  OSD_MAX_WIDTH/3
#define INFO_H  60

static lv_obj_t *m_sys_info_root = NULL;
static lv_obj_t *m_info_version = NULL;
static lv_obj_t *m_info_product_id = NULL;
static lv_group_t *m_sys_info_group  = NULL;

static uint32_t valid_key_map[] = {
    V_KEY_V_UP,
    V_KEY_V_DOWN,
    V_KEY_LEFT,
    V_KEY_RIGHT,
    V_KEY_EXIT,
    V_KEY_MENU,
};

static bool key_filter(uint32_t vkey)
{
    int i;
    int count = sizeof(valid_key_map)/sizeof(valid_key_map[0]);
    for (i = 0; i < count; i++) {
        if (valid_key_map[i] == vkey)
            return true;
    }
    return false;
}

static void event_handler(lv_event_t * e)
{
    
    lv_event_code_t code = lv_event_get_code(e);
//    lv_obj_t * obj = lv_event_get_target(e);
    uint32_t vkey = VKEY_NULL;
    lv_indev_t *key_indev = lv_indev_get_act();

    if(code == LV_EVENT_KEY && key_indev->proc.state == LV_INDEV_STATE_PRESSED){
        uint32_t value = lv_indev_get_key(key_indev);
        vkey = key_convert_vkey(value);
        if (!key_filter(vkey))
            return;
        api_control_send_key(vkey);
    }
}


static int win_sys_info_open(void *arg)
{
    int obj_cnt;
    int i;
    int start_x;
    int start_y;
    sys_data_t *sys_param = data_mgr_sys_get();

    m_sys_info_group = lv_group_create();

    //user need get the LV_KEY_NEXT and LV_KEY_PREV key, so disable auto focus.
    m_sys_info_group->auto_focus_dis = 1;
    key_regist(m_sys_info_group);
    m_sys_info_root = lv_obj_create(lv_scr_act());
    lv_group_add_obj(m_sys_info_group, m_sys_info_root);
    lv_obj_add_event_cb(m_sys_info_root, event_handler, LV_EVENT_ALL, NULL); 
    osd_draw_background(m_sys_info_root, false);

    m_info_product_id = lv_label_create(m_sys_info_root);
    lv_obj_add_style(m_info_product_id, TEXT_STY_MID_NORMAL, 0);
    lv_label_set_recolor(m_info_product_id, true);
    lv_label_set_text_fmt(m_info_product_id, "#E0E000 Product ID:# %s", sys_param->product_id);

    m_info_version = lv_label_create(m_sys_info_root);
    lv_obj_add_style(m_info_version, TEXT_STY_MID_NORMAL, 0);
    lv_label_set_recolor(m_info_version, true);
    lv_label_set_text_fmt(m_info_version, "#E0E000 Firmware version:# %u", sys_param->firmware_version);

    obj_cnt = lv_obj_get_child_cnt(m_sys_info_root);
    start_x = (OSD_MAX_WIDTH - INFO_W) >> 1;
    start_y = (OSD_MAX_HEIGHT - INFO_H * obj_cnt) >> 1;
    lv_obj_t *obj_child;    
    for(i = 0; i < obj_cnt; i ++){
        obj_child = lv_obj_get_child(m_sys_info_root, i);
        if (obj_child){
            lv_obj_set_pos(obj_child, start_x, start_y+INFO_H*i);
        }
    }

	return API_SUCCESS;
}

static int win_sys_info_close(void *arg)
{
    lv_group_remove_all_objs(m_sys_info_group);
    lv_group_del(m_sys_info_group);
    lv_obj_del(m_sys_info_root);

    return API_SUCCESS;
}

static win_ctl_result_t win_sys_info_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_SKIP;

    if (MSG_TYPE_KEY == ctl_msg->msg_type){
        if (V_KEY_EXIT == ctl_msg->msg_code ||
            V_KEY_MENU == ctl_msg->msg_code)
            ret = WIN_CTL_POPUP_CLOSE;
    }else{
	    
    }
	return ret;
}

win_des_t g_win_sys_info = 
{
    .open = win_sys_info_open,
    .close = win_sys_info_close,
    .control = win_sys_info_control,
};
