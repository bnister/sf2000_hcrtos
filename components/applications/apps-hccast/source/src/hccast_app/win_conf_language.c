/**
 * win_conf_language.c
 */
#include <stdio.h>
#include <unistd.h>
#include "../lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include "obj_mgr.h"
#include "com_api.h"
#include "menu_mgr.h"
#include "osd_com.h"
#include "key.h"

static lv_obj_t *m_language_root;
static lv_obj_t *m_label_language_name;
static lv_group_t *m_lanugage_group;


static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t vkey = VKEY_NULL;

    if(code == LV_EVENT_KEY){
        uint32_t value = lv_indev_get_key(lv_indev_get_act());
        vkey = key_convert_vkey(value);

        printf("sub vkey: %lu...\n", vkey);

        if (V_KEY_UP == vkey){
            lv_group_focus_prev(m_lanugage_group);
            printf("LV_KEY_UP...\n");
        } else if (V_KEY_DOWN == vkey){
            lv_group_focus_next(m_lanugage_group);
            printf("LV_KEY_DOWN...\n");
        }else {
            api_control_send_key(vkey);
        }
    }
}


static void list_show_test(lv_obj_t *parent)
{
	static lv_obj_t * list1;
    list1 = lv_list_create(parent);
    lv_obj_set_size(list1, 180, 420);
    lv_obj_center(list1);

    /*Add buttons to the list*/
    lv_obj_t * btn;

    lv_list_add_text(list1, "File");
    btn = lv_list_add_btn(list1, LV_SYMBOL_FILE, "New");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_DIRECTORY, "Open");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_SAVE, "Save");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_CLOSE, "Delete");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_EDIT, "Edit");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);

    lv_list_add_text(list1, "Connectivity");
    btn = lv_list_add_btn(list1, LV_SYMBOL_BLUETOOTH, "Bluetooth");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_GPS, "Navigation");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_USB, "USB");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_BATTERY_FULL, "Battery");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);

    lv_list_add_text(list1, "Exit");
    btn = lv_list_add_btn(list1, LV_SYMBOL_OK, "Apply");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
    btn = lv_list_add_btn(list1, LV_SYMBOL_CLOSE, "Close");
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);	
}

static int win_language_open(void *arg)
{

	(void)arg;
	static lv_style_t sty_font_large;

    //regist key device to group, so that the object in the group can 
    //get the key event.    
    m_lanugage_group = lv_group_create();
    key_regist(m_lanugage_group);


    m_language_root = lv_obj_create(lv_scr_act());
	osd_draw_background(m_language_root, false);

    m_label_language_name = obj_label_open(m_language_root, SUB_NAME_X, SUB_NAME_Y, SUB_NAME_W, osd_get_string(STR_LANGUAGE));
#if 1
    lv_style_init(&sty_font_large);
    lv_style_set_text_font(&sty_font_large, FONT_SIZE_LARGE);
    lv_style_set_text_color(&sty_font_large, COLOR_WHITE);
    lv_obj_add_style(m_label_language_name, &sty_font_large, 0); 

#endif


//just for test
    list_show_test(m_language_root);
    return 0;

}

static int win_language_close(void *arg)
{
    lv_group_remove_all_objs(m_lanugage_group);
    lv_group_del(m_lanugage_group);

    lv_obj_del(m_language_root);

    return API_SUCCESS;

}


static win_ctl_result_t win_lanugages_control(void *arg1, void *arg2)
{
    (void)arg2;
    uint32_t key;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;

	if (MSG_TYPE_KEY == ctl_msg->msg_type){    

		key = ctl_msg->msg_code;
	    if (key == V_KEY_ENTER){

	    }else if (key == V_KEY_EXIT){
	        ret = WIN_CTL_POPUP_CLOSE;
	    }
	    else if (key == V_KEY_MENU){
	        ret = WIN_CTL_POPUP_CLOSE;   
	    }
	} else {

	}


    return ret;
}


win_des_t g_win_conf_language =
{
    .open = win_language_open,
    .close = win_language_close,
    .control = win_lanugages_control,
};



