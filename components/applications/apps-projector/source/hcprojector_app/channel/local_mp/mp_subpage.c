/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-12-29 10:53:06
 */
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_subpage.h"
#include "mp_mainpage.h"

#include "screen.h"
#include "setup.h"
#include "factory_setting.h"
#include "mp_fspage.h"

extern SCREEN_SUBMP_E screen_submp;
partition_info_t * cur_temp_partition=NULL;
#define MAX_DEVNAME_LEN 20
static char last_devname[MAX_DEVNAME_LEN]={0};


void sub_page_keyinput_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target = lv_event_get_target(event);
    if(code == LV_EVENT_KEY)
    {
        int keypad_value = lv_indev_get_key(lv_indev_get_act());
        switch(keypad_value){
            case LV_KEY_UP:
                lv_group_focus_next(lv_group_get_default());
                break;
            case LV_KEY_DOWN:
                lv_group_focus_prev(lv_group_get_default());
                break;
            case LV_KEY_LEFT:
                lv_group_focus_prev(lv_group_get_default());
                break;
            case LV_KEY_RIGHT:
                lv_group_focus_next(lv_group_get_default());
                break;
            case LV_KEY_ESC:
                _ui_screen_change(ui_mainpage, 0, 0);
                break;
            case LV_KEY_ENTER:
                if(lv_obj_get_index(lv_obj_get_parent(target))==0){
                    _ui_screen_change(ui_mainpage, 0, 0);
                }else{
                    int obj_cont_index=lv_obj_get_index(lv_obj_get_parent(target));
                    api_set_partition_info(obj_cont_index-1);
                    if(strcmp(last_devname,cur_temp_partition->used_dev)!=0){
                        app_media_list_all_free();
                        memset(last_devname,0,sizeof(last_devname));
                        strncpy(last_devname,cur_temp_partition->used_dev,strlen(cur_temp_partition->used_dev));
                    }
                    _ui_screen_change(ui_fspage, 0, 0);  
                }
                break;
            default :
                break;
        }
    }
}


void subpage_open(void)
{
    sub_group= lv_group_create();
    set_key_group(sub_group);
    create_subpage_scr();
    cur_temp_partition=mmp_get_partition_info();
    screen_submp=SCREEN_SUBMP1;
    lv_group_focus_next(lv_group_get_default());
}

void subpage_close(void)
{
    lv_group_remove_all_objs(sub_group);
    lv_group_del(sub_group);
    clear_subpage_scr();
}