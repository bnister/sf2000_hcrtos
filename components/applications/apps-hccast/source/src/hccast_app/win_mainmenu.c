#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
//#include <lvgl/lvgl.h>
#include "../lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include "key.h"
#include "obj_mgr.h"
#include "com_api.h"
#include "menu_mgr.h"
#include "osd_com.h"


//#include "typedef.h"

#define IMG_W  256
#define IMG_H  256
#define IMG_IPTV_H  567

#define IMG_IPTV_X  80
#define IMG_IPTV_Y  (OSD_MAX_HEIGHT-IMG_IPTV_H)/2 //80

#define IMG_GAP_X  (OSD_MAX_WIDTH-IMG_IPTV_X*2)/4 // four bitmaps in one line
#define IMG_GAP_Y  (IMG_IPTV_H - IMG_H)

static lv_obj_t *m_mainmenu_root;
static lv_obj_t *m_img_iptv = NULL;
static lv_obj_t *m_img_miracast = NULL;
static lv_obj_t *m_img_airplay = NULL;
static lv_obj_t *m_img_dlna = NULL;
static lv_obj_t *m_img_wirecast = NULL;
static lv_obj_t *m_img_media = NULL;
static lv_obj_t *m_img_conf = NULL;


static lv_obj_t *m_label_iptv = NULL;
static lv_obj_t *m_label_miracast = NULL;
static lv_obj_t *m_label_airplay = NULL;
static lv_obj_t *m_label_dlna = NULL;
static lv_obj_t *m_label_wirecast = NULL;
static lv_obj_t *m_label_media = NULL;
static lv_obj_t *m_label_conf = NULL;

static char *m_str_iptv = NULL;
static char *m_str_miracast = NULL;
static char *m_str_airplay = NULL;
static char *m_str_dlna = NULL;
static char *m_str_wirecast = NULL;
static char *m_str_media = NULL;
static char *m_str_conf = NULL;

LV_IMG_DECLARE(im_iptv)
LV_IMG_DECLARE(im_iptv_b)
LV_IMG_DECLARE(miracast_256x256)
LV_IMG_DECLARE(miracast_256x256_b)
LV_IMG_DECLARE(airplay_256x256)
LV_IMG_DECLARE(airplay_256x256_b)
LV_IMG_DECLARE(dlna_256x256)
LV_IMG_DECLARE(dlna_256x256_b)
LV_IMG_DECLARE(wirecast_256x256)
LV_IMG_DECLARE(wirecast_256x256_b)
LV_IMG_DECLARE(media_256x256)
LV_IMG_DECLARE(media_256x256_b)
LV_IMG_DECLARE(setting_256x256)
LV_IMG_DECLARE(setting_256x256_b)

enum{
    MANU_ID_IPTV = 1,
    MANU_ID_MIRACAST,
    MANU_ID_AIRCAST,
    MANU_ID_DLNA,
    MANU_ID_WIRECAST,
    MANU_ID_MIDIA_PLAYER,
    MANU_ID_CONFIG,
};

static void win_event_cb(lv_event_t *event);

#define LV_OBJ_LINK_ITEM(obj, id, up_id, down_id, left_id, rigth_id, show, high, grey, select) \
do{ \
    struct list_head head = LIST_HEAD_INIT(head);\
    obj_link_t link = {head, obj, OBJ_LV_TYPE_IMG, id, up_id, down_id, left_id, rigth_id, show, high, grey, select};  \
    obj_mgr_add_link(m_obj_handle, &link); \
    lv_obj_add_event_cb(obj, win_event_cb, LV_EVENT_KEY, NULL); \
    lv_group_add_obj(m_group, obj); \
}while(0)

#define LV_OBJ_MENU_ITEM(item_name, x, y, h, bitmap) \
do{ \
    m_img_##item_name = obj_img_open(m_mainmenu_root, bitmap, x, y); \
    m_label_##item_name = obj_label_open(m_mainmenu_root, x, y+h+6, IMG_W, m_str_##item_name); \
    lv_obj_add_style(m_label_##item_name, TEXT_STY_MID_NORMAL, 0); \
}while(0)

//attach the object to the link object, so that these objects can be in one container
#define  LV_OBJ_LINK_OBJ(link_obj, obj, show, high, grey, select)    \
do{         \
    struct list_head node = LIST_HEAD_INIT(node);           \
    obj_style_t style = {node, obj, OBJ_LV_TYPE_LABEL, show, high, grey, select}; \
    obj_link_t *link = obj_mgr_get_link(m_obj_handle, link_obj); \
    obj_mgr_add_obj(link, &style); \
}while(0)


static lv_group_t *m_group = NULL;
static void *m_obj_handle = NULL;
static char m_obj_cur_id = 1;

static void win_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *target = lv_event_get_target(event);
    uint32_t vkey = VKEY_NULL;

    lv_obj_t *next_obj = NULL;
    if(code == LV_EVENT_KEY){
        uint32_t value = lv_indev_get_key(lv_indev_get_act());
        vkey = key_convert_vkey(value);
        //printf("maninmenu key: %d\n", vkey);
        if (VKEY_NULL == vkey)
            return;

        if (obj_is_valid_key(vkey)){

            next_obj = obj_mgr_draw_obj_by_key(m_obj_handle, target, vkey);
            if (next_obj){
                obj_link_t *link;
                lv_group_focus_obj(next_obj);
                link = obj_mgr_get_link(m_obj_handle, next_obj);
                m_obj_cur_id = link->id;
            }
        }else{
            api_control_send_key(vkey);
        }
    }

}

static void create_main_menu(lv_obj_t *parent)
{

    m_obj_handle = obj_mgr_open();

    m_str_iptv = osd_get_string(STR_IPTV);
    m_str_miracast = osd_get_string(STR_MIRACAST);
    m_str_airplay = osd_get_string(STR_AIRPLAY);
    m_str_dlna = osd_get_string(STR_DLNA);
    m_str_wirecast = osd_get_string(STR_WIRECAST);
    m_str_media = osd_get_string(STR_MEDIA_PLAYER);
    m_str_conf = osd_get_string(STR_CONF);

    LV_OBJ_MENU_ITEM(iptv,      IMG_IPTV_X,             IMG_IPTV_Y,           IMG_IPTV_H, &im_iptv);
    LV_OBJ_MENU_ITEM(miracast,  IMG_IPTV_X+IMG_GAP_X*1, IMG_IPTV_Y,           IMG_H,      &miracast_256x256);
    LV_OBJ_MENU_ITEM(airplay,   IMG_IPTV_X+IMG_GAP_X*2, IMG_IPTV_Y,           IMG_H,      &airplay_256x256);
    LV_OBJ_MENU_ITEM(dlna,      IMG_IPTV_X+IMG_GAP_X*3, IMG_IPTV_Y,           IMG_H,      &dlna_256x256);
    LV_OBJ_MENU_ITEM(wirecast,  IMG_IPTV_X+IMG_GAP_X*1, IMG_IPTV_Y+IMG_GAP_Y, IMG_H,      &wirecast_256x256);
    LV_OBJ_MENU_ITEM(media,     IMG_IPTV_X+IMG_GAP_X*2, IMG_IPTV_Y+IMG_GAP_Y, IMG_H,      &media_256x256);
    LV_OBJ_MENU_ITEM(conf,      IMG_IPTV_X+IMG_GAP_X*3, IMG_IPTV_Y+IMG_GAP_Y, IMG_H,      &setting_256x256);

    LV_OBJ_LINK_ITEM(m_img_iptv,1,1,1,4,2,&im_iptv,&im_iptv_b,NULL,NULL);
    LV_OBJ_LINK_ITEM(m_img_miracast,2,5,5,1,3,&miracast_256x256,&miracast_256x256_b,NULL,NULL);
    LV_OBJ_LINK_ITEM(m_img_airplay,3,6,6,2,4,&airplay_256x256,&airplay_256x256_b,NULL,NULL);
    LV_OBJ_LINK_ITEM(m_img_dlna,4,7,7,3,5,&dlna_256x256,&dlna_256x256_b,NULL,NULL);
    LV_OBJ_LINK_ITEM(m_img_wirecast,5,2,2,4,6,&wirecast_256x256,&wirecast_256x256_b,NULL,NULL);
    LV_OBJ_LINK_ITEM(m_img_media,6,3,3,5,7,&media_256x256,&media_256x256_b,NULL,NULL);
    LV_OBJ_LINK_ITEM(m_img_conf,7,4,4,6,1,&setting_256x256,&setting_256x256_b,NULL,NULL);

    LV_OBJ_LINK_OBJ(m_img_iptv, m_label_iptv, TEXT_STY_MID_NORMAL, TEXT_STY_MID_HIGH, NULL, NULL);
    LV_OBJ_LINK_OBJ(m_img_miracast, m_label_miracast, TEXT_STY_MID_NORMAL, TEXT_STY_MID_HIGH, NULL, NULL);
    LV_OBJ_LINK_OBJ(m_img_airplay, m_label_airplay, TEXT_STY_MID_NORMAL, TEXT_STY_MID_HIGH, NULL, NULL);
    LV_OBJ_LINK_OBJ(m_img_dlna, m_label_dlna, TEXT_STY_MID_NORMAL, TEXT_STY_MID_HIGH, NULL, NULL);
    LV_OBJ_LINK_OBJ(m_img_wirecast, m_label_wirecast, TEXT_STY_MID_NORMAL, TEXT_STY_MID_HIGH, NULL, NULL);
    LV_OBJ_LINK_OBJ(m_img_media, m_label_media, TEXT_STY_MID_NORMAL, TEXT_STY_MID_HIGH, NULL, NULL);
    LV_OBJ_LINK_OBJ(m_img_conf, m_label_conf, TEXT_STY_MID_NORMAL, TEXT_STY_MID_HIGH, NULL, NULL);

    printf("focus menu_id: %d\n", m_obj_cur_id);
    obj_mgr_draw_link_by_id(m_obj_handle, m_obj_cur_id, OBJ_DRAW_HIGH);
    lv_group_focus_obj((lv_obj_t*)obj_mgr_get_obj_by_id(m_obj_handle, m_obj_cur_id));
}


static win_ctl_result_t win_mainmenu_key_act(uint32_t key)
{
    win_ctl_result_t ret = WIN_CTL_NONE;
    win_des_t *win = NULL;
    int id;

    id = m_obj_cur_id;
    submenu_info_t *submenu = NULL;
    if (key == V_KEY_ENTER){
        printf("Get V_KEY_ENTER!\n");
        switch (id) 
        {
        case MANU_ID_IPTV:
            //frank test
            //win = &g_win_conf_language;
            break;
        case MANU_ID_MIRACAST:
            break;
        case MANU_ID_AIRCAST:
            break;
        case MANU_ID_DLNA:
            break;
        case MANU_ID_WIRECAST:
            break;
        case MANU_ID_MIDIA_PLAYER:
            if(mmp_get_usb_stat() !=USB_STAT_MOUNT)
            {
                //popup msg hint.
                win_msgbox_msg_open("Please plugin USB-disk or SD card!", 2000, NULL, NULL);
                printf(" please check udisk if plug-in or not\n");
                return ret;
            }
            win = &g_win_submenu;
            submenu = &g_submenu_media;
            win->param = (void*)(submenu);
            break;
        case MANU_ID_CONFIG:
            win = &g_win_submenu;
            submenu = &g_submenu_conf;
            win->param = (void*)(submenu);
            break;
        default:
            break;            
        }
        if (win){
            menu_mgr_push(win);
            ret = WIN_CTL_PUSH_CLOSE;
        }
    }else if (key == V_KEY_EXIT){
        printf("Get V_KEY_EXIT!\n");
    }
    else if (key == V_KEY_MENU){
        printf("Get V_KEY_MENU!\n");
    }else{
        ret = WIN_CTL_SKIP;
    }

    return ret;
}

static win_ctl_result_t win_mainmenu_msg_act(control_msg_t *ctl_msg)
{
    win_ctl_result_t ret = WIN_CTL_NONE;
    if (1){
    }else{
        ret = WIN_CTL_SKIP;
    }
    return ret;
}

static int win_mainmenu_open(void *arg)
{
    (void)arg;

    printf("Entering %s()\n", __FUNCTION__);
    m_mainmenu_root = lv_obj_create(lv_scr_act());
    
    //regist key device to group, so that the object in the group can 
    //get the key event.    
    m_group = lv_group_create();
    key_regist(m_group);

    osd_draw_background(m_mainmenu_root, false);
    create_main_menu(m_mainmenu_root);
    return API_SUCCESS;
}

static int win_mainmenu_close(void *arg)
{
    printf("Entering %s()\n", __FUNCTION__);
    if (m_obj_handle)
        obj_mgr_close(m_obj_handle);

    lv_group_remove_all_objs(m_group);
    lv_group_del(m_group);
    
    lv_obj_del(m_mainmenu_root);

    return API_SUCCESS;
}

static win_ctl_result_t win_mainmenu_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;

    if (MSG_TYPE_KEY == ctl_msg->msg_type){
        ret = win_mainmenu_key_act(ctl_msg->msg_code);
    }else{
        ret = win_mainmenu_msg_act(ctl_msg);
    }

    return ret;
}

win_des_t g_win_mainmenu =
{
    .open = win_mainmenu_open,
    .close = win_mainmenu_close,
    .control = win_mainmenu_control,
};
