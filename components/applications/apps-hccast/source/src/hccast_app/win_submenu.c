/**
 * win_submenu.c
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
#include "media_player.h"

LV_IMG_DECLARE(language_setting)
LV_IMG_DECLARE(network_set)
LV_IMG_DECLARE(osd_setting)
LV_IMG_DECLARE(sw_update)
LV_IMG_DECLARE(recovery)
LV_IMG_DECLARE(sw_version)

LV_IMG_DECLARE(device_name)
LV_IMG_DECLARE(ip_addr)

LV_IMG_DECLARE(im_small_video)
LV_IMG_DECLARE(im_small_music)
LV_IMG_DECLARE(im_small_photo)


static lv_group_t *m_sub_group = NULL;
static lv_obj_t *m_label_sub_name = NULL;
static lv_obj_t *m_submenu_root;

static lv_obj_t *submenu_img_item[SUB_MENU_MAX];
static lv_obj_t *submenu_label_item[SUB_MENU_MAX];

static submenu_info_t *m_cur_submenu = NULL;

/**
 * The items of configuration submenu
 */
///////////////////////////////////////////
menu_des_t submenu_config[] = {
    {
        .img = &language_setting,
        .str_id = STR_LANGUAGE,
        .state = MENU_STATE_ACTIVE,
        .type = MENU_TYPE_WIN,
        .win = NULL, //to be done.
    },
    {
        .img = &network_set,
        .str_id = STR_WIFI_SET,
        .state = MENU_STATE_DEACTIVE,
        .type = MENU_TYPE_WIN,
        .win = NULL, //to be done.
    },
    {
        .img = &osd_setting,
        .str_id = STR_OSD_SET,
        .state = MENU_STATE_DEACTIVE,
        .type = MENU_TYPE_WIN,
        .win = NULL, //to be done.
    },
    {
        .img = &sw_update,
        .str_id = STR_SW_UPGRADE,
        .state = MENU_STATE_ACTIVE,
        .type = MENU_TYPE_WIN,
        .win = &g_win_upgrade, 
        .param = (void*)MSG_TYPE_USB_UPGRADE,
    },
    {
        .img = &recovery,
        .str_id = STR_FACTORY_SET,
        .state = MENU_STATE_ACTIVE,
        .type = MENU_TYPE_WIN,
        .win = NULL, //to be done.
    },
    {
        .img = &sw_version,
        .str_id = STR_SW_INFOR,
        .state = MENU_STATE_ACTIVE,
        .type = MENU_TYPE_WIN,
        .win = &g_win_sys_info, 
    },

};
#define SUB_MENU_CONF_CNT (sizeof(submenu_config)/sizeof(submenu_config[0]))
///////////////////////////////////////////

menu_des_t submenu_media_player[] = {
    {
        .img = &im_small_video,
        .str_id = STR_VIDEO,
        .state = MENU_STATE_ACTIVE,
        .type = MENU_TYPE_WIN,
        .win = &g_win_media_list, //to be done.
        .param = (void*)MEDIA_TYPE_VIDEO,
    },
    {
        .img = &im_small_music,
        .str_id = STR_MUSIC,
        .state = MENU_STATE_ACTIVE,
        .type = MENU_TYPE_WIN,
        .win = &g_win_media_list,
        .param = (void*)MEDIA_TYPE_MUSIC,
    },
    {
        .img = &im_small_photo,
        .str_id = STR_PHOTO,
        .state = MENU_STATE_ACTIVE,
        .type = MENU_TYPE_WIN,
        .win = &g_win_media_list,
        .param = (void*)MEDIA_TYPE_PHOTO,
    },
};
#define SUB_MENU_MEDIA_CNT (sizeof(submenu_media_player)/sizeof(submenu_media_player[0]))


submenu_info_t  g_submenu_media = {submenu_media_player, SUB_MENU_MEDIA_CNT, STR_MEDIA_PLAYER, 0}; //media player
submenu_info_t  g_submenu_conf = {submenu_config, SUB_MENU_CONF_CNT, STR_CONFIG, 0}; //configuration

/**
 * Get the current submenu item id(base is 0) according the object lable
 * @param  count: the submenu count
 * @param  obj : the object to get the id in the submenu
 * @return      : the submenu item id.
 */
static int win_submenu_get_menu_id(int count, void *obj)
{
    int i;
    printf("%s(), count:%d\n", __FUNCTION__, count);
    for (i = 0; i < count; i ++){
        if ((void*)(submenu_label_item[i]) == obj)
            return i;
    }
    return 0;
}



static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t vkey = VKEY_NULL;
    lv_obj_t * cur_obj = NULL;
    int menu_id;

    if(code == LV_EVENT_KEY){
        uint32_t value = lv_indev_get_key(lv_indev_get_act());
        vkey = key_convert_vkey(value);

        if (V_KEY_UP == vkey){
            lv_group_focus_prev(m_sub_group);
        } else if (V_KEY_DOWN == vkey){
            lv_group_focus_next(m_sub_group);
        }else {
            api_control_send_key(vkey);
        }

        if ((V_KEY_UP == vkey) || (V_KEY_DOWN == vkey)){
            cur_obj = lv_group_get_focused(m_sub_group);
            menu_id = win_submenu_get_menu_id(lv_group_get_obj_count(m_sub_group), cur_obj);
            printf("change to focus id:%d\n", menu_id);
            m_cur_submenu->focus_id = menu_id;
        }
    }
}

static void subwin_active_update(submenu_info_t *submenu)
{
    menu_des_t *menu_des = submenu->menu_des;
    int count = submenu->menu_count;
    int i;
    int idx = 0;

    for (i = 0, idx = 0; i < count; i ++){
        if (MENU_STATE_HIDEN == menu_des[i].state)
            continue;

        if (MENU_STATE_DEACTIVE == menu_des[idx].state)
            lv_obj_add_state(submenu_label_item[idx], LV_STATE_DISABLED);

        //Depend on USB
        if (g_win_submenu.param == (void*)&g_submenu_conf &&
            menu_des[idx].win == (void*)&g_win_upgrade ){
            if (USB_STAT_UNMOUNT == mmp_get_usb_stat()){
                lv_obj_add_state(submenu_label_item[idx], LV_STATE_DISABLED);
            }else{
                lv_obj_clear_state(submenu_label_item[idx], LV_STATE_DISABLED);
            } 
        }
        idx ++;
        //Depend on wifi
    }
}

static void create_sub_menu(lv_obj_t *parent, submenu_info_t *submenu)
{
    int i;
    int start_x;
    int start_y;
    static lv_style_t sty_font_large;

    menu_des_t *menu_des = submenu->menu_des;
    int count = submenu->menu_count;
    int menu_str = submenu->menu_str;

    if (!menu_des)
        return;

    start_x = (OSD_MAX_WIDTH - SUB_MENU_ITEM_W) >> 1;
    start_y = (OSD_MAX_HEIGHT - SUB_MENU_ITEM_H * count) >> 1;

    m_label_sub_name = obj_label_open(parent, SUB_NAME_X, SUB_NAME_Y, SUB_NAME_W, osd_get_string(menu_str));
    lv_style_init(&sty_font_large);
    lv_style_set_text_font(&sty_font_large, FONT_SIZE_LARGE);
    lv_style_set_text_color(&sty_font_large, COLOR_WHITE);
    lv_obj_add_style(m_label_sub_name, &sty_font_large, 0); 


    int idx = 0;
    for (i = 0, idx = 0; i < count; i ++){
        if (MENU_STATE_HIDEN == menu_des[i].state)
            continue;

        submenu_img_item[idx] = obj_img_open(parent, menu_des[idx].img, start_x, start_y+SUB_MENU_ITEM_H*idx);
        submenu_label_item[idx] = obj_label_open(parent, start_x+SUB_MENU_IMG_W+14, start_y+SUB_MENU_ITEM_H*idx+10, \
            SUB_MENU_ITEM_W-80, osd_get_string(menu_des[idx].str_id));

        lv_obj_add_style(submenu_label_item[idx], TEXT_STY_LEFT_NORMAL, 0); 
        lv_obj_add_style(submenu_label_item[idx], TEXT_STY_LEFT_HIGH, LV_STATE_FOCUSED); 
        lv_obj_add_style(submenu_label_item[idx], TEXT_STY_LEFT_DISABLE, LV_STATE_DISABLED); 
        lv_obj_add_event_cb(submenu_label_item[idx], event_handler, LV_EVENT_ALL, NULL);
        lv_group_add_obj(m_sub_group, submenu_label_item[idx]);
        idx ++;
    }
    subwin_active_update(submenu);

    printf("memu focus id:%d\n", submenu->focus_id);
    lv_group_focus_obj(submenu_label_item[submenu->focus_id]);
}


static int win_submenu_open(void *arg)
{
    submenu_info_t *submenu = (submenu_info_t*)arg;
    m_submenu_root = lv_obj_create(lv_scr_act());
    
    //regist key device to group, so that the object in the group can 
    //get the key event.    
    m_sub_group = lv_group_create();
    key_regist(m_sub_group);

    osd_draw_background(m_submenu_root, false);
    m_cur_submenu = submenu;

    create_sub_menu(m_submenu_root, submenu);
#if 0
    if (WIN_SUBMENU_CONF == menu_id){
        //open config submenu
        create_sub_menu(submenu);
    }else if (WIN_SUBMENU_IPTV == menu_id) {

    }
#endif
    return API_SUCCESS;
}

static int win_submenu_close(void *arg)
{

    lv_group_remove_all_objs(m_sub_group);
    lv_group_del(m_sub_group);
    lv_obj_del(m_submenu_root);

    return API_SUCCESS;
}

static win_ctl_result_t win_submenu_key_act(uint32_t key)
{
    win_ctl_result_t ret = WIN_CTL_NONE;
    win_des_t *win = NULL;
    menu_des_t *menu_des;
    int id;

    id = m_cur_submenu->focus_id; 
    if (key == V_KEY_ENTER){
        printf("submenu id: %d\n", id);

        menu_des = &(m_cur_submenu->menu_des[id]);
        if (&recovery == menu_des->img){
            //factory reset
            control_msg_t msg;
            msg.msg_type = MSG_TYPE_KEY_TRIGER_RESET;
            api_control_send_msg(&msg);
            return ret;
        }

        if (!menu_des->win)
            return WIN_CTL_NONE;

        if (MENU_TYPE_MENU == menu_des->type){
            //Next window is also sub menu. so open g_win_submenu again.
            //and win->param is next submenu items
            win = &g_win_submenu;
            win->param = menu_des->win;
            menu_mgr_push(win);

        }else{ //window
            //Next window is real window.
            win = menu_des->win;
            win->param = (void*)(menu_des->param);
        #if 0            
            if (win == &g_win_media_list){ 
                media_type_t media_type = MEDIA_TYPE_VIDEO;
            //video,music,photo use the same window,so we should use id to identify
                if (0 == id)
                    media_type = MEDIA_TYPE_VIDEO;
                else if (1 == id)
                    media_type = MEDIA_TYPE_MUSIC;
                else if (2 == id)
                    media_type = MEDIA_TYPE_PHOTO;

                win->param = (void*)media_type;
            }
        #endif            
            menu_mgr_push(win);
        }
        ret = WIN_CTL_PUSH_CLOSE;

    }else if (key == V_KEY_EXIT){
        ret = WIN_CTL_POPUP_CLOSE;
    }
    else if (key == V_KEY_MENU){
        ret = WIN_CTL_POPUP_CLOSE;   
    }
    return ret;
}


static win_ctl_result_t win_submenu_msg_act(control_msg_t *ctl_msg)
{
    win_ctl_result_t ret = WIN_CTL_NONE;

    if(ctl_msg->msg_type == MSG_TYPE_USB_DISK_UMOUNT)
    {
        if (g_win_submenu.param == &g_submenu_media)
            ret = WIN_CTL_POPUP_CLOSE;
        
        subwin_active_update(m_cur_submenu);
    }
    else if (ctl_msg->msg_type == MSG_TYPE_USB_DISK_MOUNT)
    {
        subwin_active_update(m_cur_submenu);
    }

    return ret;
}

static win_ctl_result_t win_submenu_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;

    if (MSG_TYPE_KEY == ctl_msg->msg_type){
        ret = win_submenu_key_act(ctl_msg->msg_code);
    }else{
        ret = win_submenu_msg_act(ctl_msg);
    }


    return ret;
}


win_des_t g_win_submenu =
{
    .open = win_submenu_open,
    .close = win_submenu_close,
    .control = win_submenu_control,
};
