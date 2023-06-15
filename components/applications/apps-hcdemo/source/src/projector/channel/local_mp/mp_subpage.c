#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key.h"

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_subpage.h"
#include "mp_mainpage.h"

#include "../../screen.h"
#include "../../setup/setup.h"
#include "../../factory_setting.h"

int usb_state=USB_STAT_INVALID;
// lv_timer_t * usb_check_timer;


void sub_page_keyinput_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * parent_target = lv_event_get_target(event);
    lv_group_t * parent_group = lv_event_get_user_data(event);
    printf("%s %d\n",__FUNCTION__,__LINE__);
    if(code == LV_EVENT_KEY)
    {
        int keypad_value = lv_indev_get_key(lv_indev_get_act());
        printf("key code: %d\n", keypad_value);
        if(keypad_value==LV_KEY_ESC)
        {
            _ui_screen_change(ui_mainpage, 0, 0);
        }
        keypad_handler(keypad_value,parent_group);
        if(parent_target==ui_btnback)
        {
            if(keypad_value==LV_KEY_ENTER)
                _ui_screen_change(ui_mainpage, 0, 0);    
        }
        else if(parent_target==ui_btnrootdir)
        {
            if(keypad_value==LV_KEY_ENTER)
            {
                if(mmp_get_usb_stat()== USB_STAT_MOUNT)
                    _ui_screen_change(ui_fspage, 0, 0);  
            }

        }

    }
}


void subpage_open(void)
{
    int id ;
    sub_group= lv_group_create();
    set_key_group(sub_group);
    lv_group_add_obj(sub_group,ui_btnback);
    lv_group_add_obj(sub_group,ui_btnrootdir);
    //test
    lv_obj_add_event_cb(ui_btnback,sub_page_keyinput_event_cb,LV_EVENT_KEY, sub_group);
    lv_obj_add_event_cb(ui_btnrootdir,sub_page_keyinput_event_cb,LV_EVENT_KEY, sub_group);

    switch (get_media_type())
    {
        case MEDIA_TYPE_VIDEO: 
            // lv_img_set_src(ui_Image3,&ui_img_idb_usb_attached_png);
            language_choose_add_label(ui_titie_,movie_k,0);//add user data
            id = projector_get_some_sys_param(P_OSD_LANGUAGE);
            set_label_text_with_font(ui_titie_, id,0,&select_font_media[id]);
            break;
        case MEDIA_TYPE_MUSIC: 
            // lv_img_set_src(ui_Image3,&ui_img_idb_usb_attached_png);
            language_choose_add_label(ui_titie_,music_k,0);
            id = projector_get_some_sys_param(P_OSD_LANGUAGE);
            set_label_text_with_font(ui_titie_, id,0,&select_font_media[id]);
            break;
        case MEDIA_TYPE_PHOTO: 
            // lv_img_set_src(ui_Image3,&ui_img_idb_usb_attached_png);
            language_choose_add_label(ui_titie_,photo_k,0);
            id = projector_get_some_sys_param(P_OSD_LANGUAGE);
            set_label_text_with_font(ui_titie_, id,0,&select_font_media[id]);
            break;
        case MEDIA_TYPE_TXT: 
            // lv_img_set_src(ui_Image3,&ui_img_idb_usb_attached_png);
            language_choose_add_label(ui_titie_,text_k,0);
            id = projector_get_some_sys_param(P_OSD_LANGUAGE);
            set_label_text_with_font(ui_titie_, id,0,&select_font_media[id]);
            break;
        default: 
            break;

    }
    lv_group_focus_obj(ui_btnrootdir);
}

void subpage_close(void)
{
    lv_group_remove_all_objs(sub_group);
    lv_group_del(sub_group);
    lv_obj_remove_event_cb(ui_btnback,sub_page_keyinput_event_cb);
    lv_obj_remove_event_cb(ui_btnrootdir,sub_page_keyinput_event_cb);

}