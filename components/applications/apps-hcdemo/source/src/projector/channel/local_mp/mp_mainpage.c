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
#include "mp_mainpage.h"

#include "../../screen.h"


media_type_t media_type;
extern file_list_t *m_cur_file_list;
lv_obj_t * obj[4]={NULL};
void set_key_group(lv_group_t * group)
{
	lv_indev_set_group(indev_keypad, group);
	lv_group_set_default(group);
}

void keypad_handler(uint32_t value,lv_group_t * parent_group)
{
    // lv_obj_t * focus_obj=lv_group_get_focused(m_group);
    if(value==LV_KEY_UP||value==LV_KEY_RIGHT) 
        lv_group_focus_next(parent_group);
    if(value==LV_KEY_DOWN||value==LV_KEY_LEFT)
        lv_group_focus_prev(parent_group);
    if(value==LV_KEY_ENTER)
    {
        printf("click ok\r\n");
    }
}

void main_page_keyinput_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * parent_target = lv_event_get_target(event);
    lv_group_t * parent_group = lv_event_get_user_data(event);
    if(code == LV_EVENT_KEY)
    {
        int keypad_value = lv_indev_get_key(lv_indev_get_act());
        printf("key code: %d\n", keypad_value);
        keypad_handler(keypad_value,parent_group);
        if(mmp_get_usb_stat()==USB_STAT_MOUNT)
        {
            if(parent_target==ui_btnmovie)
            {
                if(keypad_value==LV_KEY_ENTER)
                {
                    media_type=MEDIA_TYPE_VIDEO;
                    _ui_screen_change(ui_subpage, 0, 0); 
                }   
            }
            else if(parent_target==ui_btnmusic)
            {
                if(keypad_value==LV_KEY_ENTER)
                {
                    media_type=MEDIA_TYPE_MUSIC;
                    _ui_screen_change(ui_subpage, 0, 0);  
                }
            }
            else if(parent_target==ui_btnphoto)
            {
                if(keypad_value==LV_KEY_ENTER)
                {
                    media_type=MEDIA_TYPE_PHOTO;
                    _ui_screen_change(ui_subpage, 0, 0);  
                }
                    

            }
            else if(parent_target==ui_btntext)
            {
                if(keypad_value==LV_KEY_ENTER)
                {
                    media_type=MEDIA_TYPE_TXT;
                    _ui_screen_change(ui_subpage, 0, 0);  
                }
            }
        }
    }
}

void usb_img_refresh_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if(code == LV_EVENT_REFRESH)
    {
        int  usb_connectstate=mmp_get_usb_stat();
        if(usb_connectstate==USB_STAT_MOUNT)
        {
            lv_obj_clear_flag(ui_usb_img,LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ui_usb_lab, "");
        }
        else
        {
            lv_obj_add_flag(ui_usb_img,LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ui_usb_lab,"NO Device");
        }
    }

}




int mainpage_open(void)
{
    main_group= lv_group_create();
    set_key_group(main_group);
    int focus_idx=0;
    int i;
    lv_obj_t * temp_obj[4]={ui_btnmovie,ui_btnmusic,ui_btnphoto,ui_btntext};
    for(i=0;i<4; i++)
    {
        obj[i]=temp_obj[i];
        lv_group_add_obj(main_group, obj[i]);
        lv_obj_add_event_cb(obj[i],main_page_keyinput_event_cb, LV_EVENT_KEY, main_group);
    }
    //handle usb icon
    lv_obj_add_event_cb(ui_usb_img,usb_img_refresh_cb, LV_EVENT_REFRESH, NULL);
    //handle focus on
    if(m_cur_file_list!=NULL)
    {
        focus_idx= media_type;
        lv_group_focus_obj(obj[focus_idx]);
    }
    return 0;
}

int mainpage_close(void)
{
    int i;
    lv_group_remove_all_objs(main_group);
    lv_group_del(main_group);
    for(i=0;i<4;i++)
    {
        lv_obj_remove_event_cb(obj[i],main_page_keyinput_event_cb);
    }
    return 0;
}
media_type_t get_media_type(void)
{
    return media_type;
} 

void set_all_label_with_font(lv_obj_t * parent, lv_font_t* font)
{
    for(int i=0; i< lv_obj_get_child_cnt(parent); i++){
        lv_obj_t *temp = lv_obj_get_child(parent, i);
        if (temp->class_p == &lv_label_class){
                lv_obj_set_style_text_font(temp, font, 0);        
        } else if(temp->class_p == &lv_btnmatrix_class ){
                lv_obj_set_style_text_font(temp, font, 0);        
        }
    }
}