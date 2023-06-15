/**
 * @file menu_mgr.h
 * @author frank.pan (frank.pan@hichiptech.com)
 * @brief Include menu manager header files
 * @version 0.1
 * @date 2022-01-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __MENU_MGR_H__
#define __MENU_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif

//win_ctl function return result
typedef enum{
    WIN_CTL_NONE = 0,  
    WIN_CTL_DONE,  //menu_ctl has proccess the command, not need to proccess again.
    WIN_CTL_SKIP,  //menu_ctl skip the command, may be control task should proccess.
    WIN_CTL_PUSH_CLOSE,  //enter next sub-menu, close current menu.
    WIN_CTL_POPUP_CLOSE,  //back to parent-menu, cloxe current menu.
}win_ctl_result_t;

typedef enum{
    MENU_STATE_ACTIVE,
    MENU_STATE_DEACTIVE,
    MENU_STATE_HIDEN,
}menu_state_t;

typedef enum{
    MENU_TYPE_MENU, //the submenu item would open next submenu
    MENU_TYPE_WIN,  //the submenu item would open function window
}menu_type_t;

typedef struct{
    void *img;  //Option, the submenu image source. But the submenu must has one of img or name at least.
    int str_id; //Option, the submenu name. But the submenu must has one of img or name at least.
    menu_state_t state;  //menu state, active/deactive(grey)/hide
    menu_type_t type;   // menu or win
    void *win;  //the window description *win_des_t
}menu_des_t;

//the submenu information, including the submenu item count.
typedef struct{
    menu_des_t  *menu_des;
    int          menu_count;
    int          menu_str;
    int          focus_id; //the current focus item of the submenu base id is 0
}submenu_info_t;

typedef enum{
    WIN_SUBMENU_IPTV = 1,  
    WIN_SUBMENU_MIRACAST,  
    WIN_SUBMENU_AIRCAST,  
    WIN_SUBMENU_DLNA,  
    WIN_SUBMENU_WIRECAST,  
    WIN_SUBMENU_MEDIA,  
    WIN_SUBMENU_CONF,  
}win_submenu_id_t;

typedef int (*win_open)(void *arg);
typedef int (*win_close)(void *arg);
typedef win_ctl_result_t (*win_ctl)(void *arg1, void *arg2);

typedef struct{
    win_open    open;
    win_close   close;
    win_ctl     control;
    void *param;
}win_des_t;



void menu_mgr_init(void);
void menu_mgr_push(win_des_t *win);
void menu_mgr_pop(void);
void menu_mgr_push_ext(win_des_t *win, int8_t shift);
void menu_mgr_pop_all(void);
win_des_t *menu_mgr_get_top(void);
uint8_t menu_mgr_get_cnt(void);

extern win_des_t g_win_um_play;
extern win_des_t g_win_upgrade;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif