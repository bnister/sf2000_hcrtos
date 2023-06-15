//This file is used to handle lvgl ui related logic and operations
//all most ui draw in local mp ui.c 
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
#include "win_media_list.h"

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_mainpage.h"
#include "mp_ctrlbarpage.h"
#include "mp_fspage.h"
#include "../../screen.h"
#include <ffplayer.h>
#include "../../setup/setup.h"
#include "../../factory_setting.h"

lv_timer_t * sec_counter_timer;
lv_timer_t * bar_show_timer;
lv_timer_t * unsupport_win_timer;
bool m_play_bar_show = true;
media_state_t play_state;
lv_obj_t * bar_btn[7];
extern media_handle_t *m_cur_media_hld; 
extern file_list_t *m_cur_file_list;
Media_Round_t media_round=0;

static media_handle_t *m_player_hld[MEDIA_TYPE_COUNT] = {NULL,};
extern char *m_cur_fullname;
char *  m_play_file_name=NULL;

#define MEDIA_IS_VIDEO() (m_cur_media_hld->type == MEDIA_TYPE_VIDEO)
char * uri;

void ctrl_bar_keyinput_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * parent =lv_event_get_target(event);
    int keypad_value;
    uint32_t vkey = VKEY_NULL;
    if(code == LV_EVENT_KEY){
        keypad_value = lv_indev_get_key(lv_indev_get_act());
        vkey = key_convert2_vkey(keypad_value);
        printf("Vkey : %d\n", vkey);
        printf("key code: %d\n", keypad_value);
        switch (keypad_value)
        {
            case LV_KEY_RIGHT :
                lv_group_focus_next(play_bar_group);
                break;
            case LV_KEY_LEFT :
                lv_group_focus_prev(play_bar_group);
                break; 
            case LV_KEY_UP :
                break;
            case LV_KEY_DOWN:
                break;
            case LV_KEY_ENTER:
                crtl_bar_enter(parent);
                break;
            case LV_KEY_ESC : //back btn value in lvgl mmap
                if (m_play_bar_show){
                    show_play_bar(false);
                }else{
                    _ui_screen_change(ui_fspage,0,0);
                }
                break;
        }
        //show bar or not
        vkey_transfer_btn(vkey);    //act ir key not in lvgl
        if (keypad_value!=LV_KEY_ESC)
        {
            show_play_bar(true);
            lv_timer_reset(bar_show_timer);
            ctrlbar_reflesh_speed();
        }
    }

}


void crtl_bar_enter(lv_obj_t * parent_btn)
{
    char *play_name = NULL;
    if(parent_btn==ui_barbtn0)
    {
        if (MEDIA_PLAY == play_state){
            media_pause(m_cur_media_hld);
        }else if (MEDIA_STOP == play_state){
            media_play(m_cur_media_hld, m_cur_media_hld->play_name);
        }
        else{
            media_resume(m_cur_media_hld);
        }
        lv_label_set_text(ui_speed, "");
    }
    else if(parent_btn==ui_barbtn1) //fb
    {
        if (MEDIA_STOP != play_state)
        {
            media_fastbackward(m_cur_media_hld);
            // ctrlbar_reflesh_speed();
        }
            
    }
    else if(parent_btn==ui_barbtn2)
    {
        if (MEDIA_STOP != play_state)
        {
            media_fastforward(m_cur_media_hld);
            // ctrlbar_reflesh_speed();
        }
            
    }
    else if(parent_btn==ui_barbtn3)
    {
        play_name = win_media_get_pre_file(m_cur_file_list); 
        if (play_name){
            if (MEDIA_STOP != play_state)
                media_stop(m_cur_media_hld);
            media_play(m_cur_media_hld, play_name);
            m_play_file_name = win_media_get_cur_file_name();
            lv_label_set_text(ui_playname, m_play_file_name);
        }
    }
    else if(parent_btn==ui_barbtn4)
    {
        play_name = win_media_get_next_file(m_cur_file_list); 
        if (play_name){
            if (MEDIA_STOP != play_state)
                media_stop(m_cur_media_hld);
            media_play(m_cur_media_hld, play_name);
            m_play_file_name = win_media_get_cur_file_name();
            lv_label_set_text(ui_playname, m_play_file_name);
        }
    }
    else if(parent_btn==ui_barbtn5)
    {
        if (MEDIA_PLAY == play_state){
            media_pause(m_cur_media_hld);
        }else if (MEDIA_STOP == play_state){
            media_play(m_cur_media_hld, m_cur_media_hld->play_name);
        }
        else{
            media_resume(m_cur_media_hld);
        }
        lv_label_set_text(ui_speed, "");
    }
    else if(parent_btn==ui_barbtn6) //round
    {
        play_name = win_media_get_cur_file_name(); 
        if (play_name){
            if (MEDIA_STOP != play_state)
                media_stop(m_cur_media_hld);
            media_play(m_cur_media_hld, m_cur_media_hld->play_name);
            m_play_file_name = win_media_get_cur_file_name();
            lv_label_set_text(ui_playname, m_play_file_name);
        }
    }
}

void format_time(uint32_t time, char *time_fmt)
{
    uint32_t hour;
    uint32_t min;
    uint32_t second;

    if (0 == time){
        sprintf(time_fmt, "00:00:00");
        return;
    }

    hour = time / 3600;
    min = (time % 3600) / 60;
    second = time % 60;
    if (hour > 0)
        sprintf(time_fmt, "%02lu:%02lu:%02lu", hour, min, second);
    else
        sprintf(time_fmt, "%02lu:%02lu", min, second);

}

void sec_timer_cb(lv_timer_t * t)
{
    uint32_t play_time = 0;
    uint32_t total_time = 0;
    char time_fmt[16];

    play_time = media_get_playtime(m_cur_media_hld);
    format_time(play_time, time_fmt);
    lv_label_set_text(ui_cur_time, time_fmt);
    lv_bar_set_value(ui_playbar, play_time, LV_ANIM_ON);

    total_time = media_get_totaltime(m_cur_media_hld);
    format_time(total_time, time_fmt);
    //printf("total time: %s\n", time_fmt);
    lv_label_set_text(ui_total_time, time_fmt);
    if (total_time > 0)
        lv_bar_set_range(ui_playbar, 0, total_time);
    
    // get state and set state lab
    play_state= media_get_state(m_cur_media_hld);
    switch(play_state)
    {
        case MEDIA_PLAY:
            lv_label_set_text(ui_playstate,bar_play_k);
            // lv_label_set_text(ui_state,bar_play_k);
            break;
        case MEDIA_PAUSE:
            lv_label_set_text(ui_playstate,bar_pause_k);
            // lv_label_set_text(ui_state,bar_pause_k);
            break;
        case MEDIA_FB:
            lv_label_set_text(ui_playstate,bar_fb_k);
            // lv_label_set_text(ui_state,bar_fb_k);
            break;
        case MEDIA_FF:
            lv_label_set_text(ui_playstate,bar_ff_k);
            // lv_label_set_text(ui_state,bar_ff_k);
            break;
        case MEDIA_SB:
            lv_label_set_text(ui_playstate,"SB");
            // lv_label_set_text(ui_state,"SB");
            break;
        case MEDIA_SF:
            lv_label_set_text(ui_playstate,"SF");
            // lv_label_set_text(ui_state,"SF");
            break;
        default:break;
    }
}

void show_play_bar(bool show)
{
    if (show){
        lv_obj_clear_flag(ui_play_bar, LV_OBJ_FLAG_HIDDEN);
        // lv_obj_clear_flag(ui_state, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_speed, LV_OBJ_FLAG_HIDDEN);
    }else{
        lv_obj_add_flag(ui_play_bar, LV_OBJ_FLAG_HIDDEN);
        // lv_obj_add_flag(ui_state, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_speed, LV_OBJ_FLAG_HIDDEN);
    }
    m_play_bar_show = show;

}

void bar_show_timer_cb(lv_timer_t * t)
{
    show_play_bar(false);
    printf("%s, %d\n", __FUNCTION__, __LINE__);

}

void unsupport_win_timer_cb(lv_timer_t * t)
{
    // _ui_screen_change(ui_fspage,0, 0);
    lv_obj_add_flag(ui_error_tip,LV_OBJ_FLAG_HIDDEN);
}

//for media play bar 
void play_bar_open(void)
{
    file_node_t *file_node = NULL;
    //uiœ‡πÿ
    //add group 
    play_bar_group= lv_group_create();
    set_key_group(play_bar_group);
    lv_obj_t * temp_btn[7]={ui_barbtn0,ui_barbtn1,ui_barbtn2,ui_barbtn3,ui_barbtn4,ui_barbtn5,ui_barbtn6};
    int i;
    for(i=0;i<7;i++)
    {
        bar_btn[i]=temp_btn[i];
        lv_group_add_obj(play_bar_group,bar_btn[i]);
        lv_obj_add_event_cb(bar_btn[i],ctrl_bar_keyinput_event_cb, LV_EVENT_KEY, NULL);
        lv_obj_clear_state(bar_btn[i],LV_STATE_ANY);
    }
    //add the bar win info
    lv_obj_set_style_bg_opa(ui_ctrl_bar, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);//try ok 
    // lv_obj_add_flag(ui_error_tip,LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_state(bar_btn[5],LV_STATE_ANY);
    lv_group_focus_obj(bar_btn[0]);

    lv_obj_set_style_bg_color(ui_barbtn2, lv_color_hex(0x323232),  LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(ui_barbtn2, 255, LV_STATE_DISABLED);
    lv_obj_set_style_text_color(ui_barbtn2,lv_color_hex(0x646464), LV_STATE_DISABLED);

    lv_obj_set_style_bg_color(ui_barbtn1, lv_color_hex(0x323232),  LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(ui_barbtn1, 255, LV_STATE_DISABLED);
    lv_obj_set_style_text_color(ui_barbtn1,lv_color_hex(0x646464), LV_STATE_DISABLED);

    lv_obj_clear_flag(ui_playbar,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_cur_time,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_total_time,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_str,LV_OBJ_FLAG_HIDDEN);
    ////////////////////////////////////////////
    file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_playname, id,0,&select_font_mplist[id]);
    set_label_text_with_font(ui_playstate, id,0,&select_font_mplist[id]);
    // set_label_text_with_font(ui_state, id,0,&select_font_mplist[id]);
    // set_label_text_with_font(ui_tip_lab, id,0,&select_font_mplist[id]);  //here to do

    lv_label_set_text(ui_playname,file_node->name);
    //play media file
    m_cur_media_hld = m_player_hld[m_cur_file_list->media_type];
    if (NULL == m_cur_media_hld){
        m_cur_media_hld = media_open(m_cur_file_list->media_type);
        m_player_hld[m_cur_file_list->media_type] = m_cur_media_hld;
    }
    media_play(m_cur_media_hld, m_cur_fullname);

    show_play_bar(true);
    sec_counter_timer = lv_timer_create(sec_timer_cb, 1000, NULL); 
    bar_show_timer = lv_timer_create(bar_show_timer_cb, 5000, NULL);

    if (m_cur_media_hld->type == MEDIA_TYPE_PHOTO)
    {
        //change the ui 
        lv_obj_add_flag(ui_playbar,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_cur_time,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_total_time,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_str,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(ui_barbtn1,LV_STATE_DISABLED);
        lv_obj_add_state(ui_barbtn2,LV_STATE_DISABLED);
    }
    else if (m_cur_media_hld->type == MEDIA_TYPE_MUSIC)
    {
        lv_obj_add_state(ui_barbtn1,LV_STATE_DISABLED);
        lv_obj_add_state(ui_barbtn2,LV_STATE_DISABLED);
    }

} 
void play_bar_close(void)
{
    lv_group_remove_all_objs(play_bar_group);
    lv_group_del(play_bar_group);
    play_bar_group=NULL;
    int i;
    for(i=0;i<7;i++)
    {
        lv_obj_remove_event_cb(bar_btn[i],ctrl_bar_keyinput_event_cb);
    }

    lv_timer_pause(sec_counter_timer);
    lv_timer_del(sec_counter_timer);

    lv_timer_pause(bar_show_timer);
    lv_timer_del(bar_show_timer); 

    media_stop(m_cur_media_hld);
    media_close(m_cur_media_hld);
	
    if (m_cur_media_hld->type == MEDIA_TYPE_PHOTO)
    {        
        //recover the ratio
        set_aspect_ratio(projector_get_some_sys_param(P_ASPECT_RATIO));
    }
    m_player_hld[m_cur_file_list->media_type] = NULL;
    m_cur_media_hld = NULL;
    free(m_cur_fullname);
    m_cur_fullname=NULL;

    //del timer
    if(unsupport_win_timer!=NULL)
    {
        lv_timer_pause(unsupport_win_timer);
        lv_timer_del(unsupport_win_timer); 
        unsupport_win_timer=NULL;
    }

} 

static char *m_str_ff[] = {"", ">> x2", " >> x4", ">> x8", ">> x16", ">> x24", ">> x32"};
static char *m_str_fb[] = {"", "<< x2", " << x4", "<< x8", "<< x16", "<< x24", "<< x32"};
static char *m_str_sf[] = {"", ">> 1/2", " >> 1/4", ">> 1/8", ">> 1/16", ">> 1/24"};
static char *m_str_sb[] = {"", "<< 1/2", " << 1/4", "<< 1/8", "<< 1/16", "<< 1/24"};
void ctrlbar_reflesh_speed(void)
{
    uint8_t speed = 0;
    char **str_speed = NULL;

    play_state = media_get_state(m_cur_media_hld);
    speed = media_get_speed(m_cur_media_hld);
    if (0 == speed){
        lv_label_set_text(ui_speed, "");
        return;
    }

    if (MEDIA_FF == play_state){
        str_speed = m_str_ff;
    }else if (MEDIA_FB == play_state){
        str_speed = m_str_fb;
    }else if (MEDIA_SF == play_state){
        str_speed = m_str_sf;
    }else if (MEDIA_SB == play_state){
        str_speed = m_str_sb;
    }
    lv_label_set_text(ui_speed, str_speed[speed]); 
}

static int vkey_transfer_btn(uint32_t vkey)
{
    switch(vkey)
    {
        case V_KEY_PLAY :
            crtl_bar_enter(ui_barbtn0);
            break;
        case V_KEY_PAUSE :
            crtl_bar_enter(ui_barbtn0);
            break;
        case V_KEY_STOP :
            crtl_bar_enter(ui_barbtn0);
            break;
        case V_KEY_FF :
            crtl_bar_enter(ui_barbtn2);
            break;
        case V_KEY_FB :
            crtl_bar_enter(ui_barbtn1);
            break;
        default : 
            break;     
    }
    return 0;
}

static int media_key_act(uint32_t key)
{
    int  ret=0;
    // media_state_t play_state;
    file_list_t *file_list = NULL;
    char *play_name = NULL;
    switch (key)
    {
    case V_KEY_NEXT:
        file_list = m_cur_file_list;
        if (V_KEY_PREV == key)
            play_name = win_media_get_pre_file(file_list); 
        else
            play_name = win_media_get_next_file(file_list);
        if (play_name){
            if (MEDIA_STOP != play_state)
                media_stop(m_cur_media_hld);
            media_play(m_cur_media_hld, play_name);
            m_play_file_name = win_media_get_cur_file_name();
            lv_label_set_text(ui_playname, m_play_file_name);
        }
        break;
    default:
        break;
    }

    return ret;
}
static int media_msg_handle(uint32_t msg_type)
{
    int  ret=0;
	char msg_info_s[50];
	printf("%s  %d\n",__FUNCTION__,msg_type);
	#if 0
    switch (msg_type)
    {
    case HCPLAYER_MSG_OPEN_FILE_FAILED:
        if(unsupport_win_timer==NULL)
        {
            lv_obj_clear_flag(ui_error_tip, LV_OBJ_FLAG_HIDDEN); //show the error win 
            unsupport_win_timer = lv_timer_create(unsupport_win_timer_cb, 3000, NULL);
        }
        break;
    case HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE:
        if(unsupport_win_timer==NULL)
        {
            lv_obj_clear_flag(ui_error_tip, LV_OBJ_FLAG_HIDDEN); //show the error win 
            unsupport_win_timer = lv_timer_create(unsupport_win_timer_cb, 3000, NULL);
        }
        break;
    case HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE:
        if(unsupport_win_timer==NULL)
        {
            lv_obj_clear_flag(ui_error_tip, LV_OBJ_FLAG_HIDDEN); //show the error win 
            unsupport_win_timer = lv_timer_create(unsupport_win_timer_cb, 3000, NULL);
        }
        break;
    case HCPLAYER_MSG_AUDIO_DECODE_ERR:
        if(unsupport_win_timer==NULL)
        {
            lv_obj_clear_flag(ui_error_tip, LV_OBJ_FLAG_HIDDEN); //show the error win 
            unsupport_win_timer = lv_timer_create(unsupport_win_timer_cb, 3000, NULL);
        }
        break;
    case HCPLAYER_MSG_VIDEO_DECODE_ERR:
        if(unsupport_win_timer==NULL)
        {
            lv_obj_clear_flag(ui_error_tip, LV_OBJ_FLAG_HIDDEN); //show the error win 
            unsupport_win_timer = lv_timer_create(unsupport_win_timer_cb, 3000, NULL);
        }
        break;
    default:
        break;
    }
	#else
	switch (msg_type)
    {
    case HCPLAYER_MSG_OPEN_FILE_FAILED:
        // if(unsupport_win_timer==NULL)
        // {
        //     lv_obj_clear_flag(ui_error_tip, LV_OBJ_FLAG_HIDDEN); //show the error win 
        //     unsupport_win_timer = lv_timer_create(unsupport_win_timer_cb, 3000, NULL);
        // }
        sprintf(msg_info_s,"Open File Fail");
		win_msgbox_msg_open(msg_info_s, 3000, NULL, NULL);
        printf("%s  %d\n",__FUNCTION__,__LINE__);
        break;
    case HCPLAYER_MSG_STATE_EOS :
        if(media_round==List_Round){
            crtl_bar_enter(ui_barbtn4);
        }else if(media_round==Single_Round){
            char * play_name = win_media_get_cur_file_name(); 
            if (play_name){
                if (MEDIA_STOP != play_state)
                    media_stop(m_cur_media_hld);
                media_play(m_cur_media_hld, m_cur_media_hld->play_name);
            }
        }
        break;
    case HCPLAYER_MSG_STATE_TRICK_EOS: 
        crtl_bar_enter(ui_barbtn4);
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO:
        sprintf(msg_info_s,"Audio Track Unsupport");
		win_msgbox_msg_open(msg_info_s, 2000, NULL, NULL);
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO:
        sprintf(msg_info_s,"Video Track Unsupport");
		win_msgbox_msg_open(msg_info_s, 2000, NULL, NULL);
        break;
    case HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE:
        sprintf(msg_info_s,"Unsupport Video Type");
		win_msgbox_msg_open(msg_info_s, 2000, NULL, NULL);
        break;
    case HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE:
        sprintf(msg_info_s,"Unsupport Audio Type");
	    win_msgbox_msg_open(msg_info_s, 2000, NULL, NULL);
        break;
    case HCPLAYER_MSG_AUDIO_DECODE_ERR:
        sprintf(msg_info_s,"Audio Decode Error");
		win_msgbox_msg_open(msg_info_s, 2000, NULL, NULL);
        break;
    case HCPLAYER_MSG_VIDEO_DECODE_ERR:
        sprintf(msg_info_s,"Video Decode Error");
		win_msgbox_msg_open(msg_info_s, 2000, NULL, NULL);
        break;
    default:
        break;
    }
	#endif
    return ret;
}
//handle ffmepg message
void media_playbar_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
	printf("%s %d %d\n",__FUNCTION__,__LINE__,ctl_msg->msg_type);
    if(MSG_TYPE_MSG == ctl_msg->msg_type)
    {
        media_msg_handle(ctl_msg->msg_code);
        printf("%s %d\n",__FUNCTION__,__LINE__);
    }

}

//meida play for previwe
int preview_mp_play(int fslist_index)
{
    file_node_t *file_node;
    uri=(char *)malloc(1024);
    file_node = file_mgr_get_file_node(m_cur_file_list, fslist_index);
    file_mgr_get_fullname(uri, m_cur_file_list->dir_path, file_node->name);

    m_cur_media_hld = m_player_hld[m_cur_file_list->media_type];
    if (NULL == m_cur_media_hld){
        m_cur_media_hld = media_open(m_cur_file_list->media_type);
        m_player_hld[m_cur_file_list->media_type] = m_cur_media_hld;
    }
    media_play(m_cur_media_hld, uri);
    return 0;
}
int preview_mp_close(void)
{
    media_stop(m_cur_media_hld);
    media_close(m_cur_media_hld);
    m_player_hld[m_cur_file_list->media_type] = NULL;
    m_cur_media_hld = NULL;
    free(uri);
    uri=NULL;
    return 0;
}

//media win for message
