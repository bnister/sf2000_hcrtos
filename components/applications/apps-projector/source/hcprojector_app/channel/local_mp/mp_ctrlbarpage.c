//This file is used to handle lvgl ui related logic and operations
//all most ui draw in local mp ui.c 
#include "app_config.h"
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
#include "win_media_list.h"
#include <hcuapi/input-event-codes.h>
#include <hcuapi/dis.h>
#include <ffplayer.h>
#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_mainpage.h"
#include "mp_ctrlbarpage.h"
#include "mp_fspage.h"
#include "mp_ebook.h"
#include "screen.h"
#include "../../volume/volume.h"
#include "setup.h"
#include "factory_setting.h"

#include "mul_lang_text.h"

#include "media_spectrum.h"
#include "mp_bsplayer_list.h"
#include "backstage_player.h"
#include "mp_playlist.h"
#include "mp_thumbnail.h"
#include "mp_preview.h"
#include "mp_playerinfo.h"
#include "mp_spectdis.h"
#include "com_api.h"
#include "mp_zoom.h"


lv_timer_t * sec_counter_timer;
lv_timer_t * bar_show_timer;
bool m_play_bar_show = true;
media_state_t play_state;
extern media_handle_t *m_cur_media_hld; 

static media_handle_t *m_player_hld[MEDIA_TYPE_COUNT] = {NULL,};
lv_obj_t * focus_obj=NULL;
image_effect_t img_effect={0};
extern SCREEN_TYPE_E prev_scr;
extern SCREEN_TYPE_E last_scr;
extern SCREEN_TYPE_E cur_scr;
extern SCREEN_SUBMP_E screen_submp;
static bool blacklight_val=false;
static bool is_bg_music=false;

static int user_vkey_action_ctrl(uint32_t user_vkey,lv_obj_t* target)
{
    media_state_t P_STA;
    char* full_filename=NULL;
    char *m_play_file_name=NULL;

    switch (user_vkey){
                case V_KEY_RIGHT :
                    if(lv_group_get_focused(lv_group_get_default())==ui_playbar){
                        __media_seek_proc(LV_KEY_RIGHT);
                    }
                    else{
                        if(lv_obj_get_index(target)==RIGHT_SCROOLL_IDX){
                            lv_obj_set_scroll_snap_x(lv_obj_get_parent(target),LV_SCROLL_SNAP_START);
                        }else{
                             lv_obj_set_scroll_snap_x(lv_obj_get_parent(target),LV_SCROLL_SNAP_NONE);
                        }
                        lv_group_focus_next(lv_group_get_default());
                    }
                    show_play_bar(true);
                    lv_timer_reset(bar_show_timer);
                    break;
                case V_KEY_LEFT :
                    if(lv_group_get_focused(lv_group_get_default())==ui_playbar){
                        __media_seek_proc(LV_KEY_LEFT);
                    }else{
                        if(lv_obj_get_index(target)==LEFT_SCROOLL_IDX){
                            lv_obj_set_scroll_snap_x(lv_obj_get_parent(target),LV_SCROLL_SNAP_END);
                        }else{
                             lv_obj_set_scroll_snap_x(lv_obj_get_parent(target),LV_SCROLL_SNAP_NONE);
                        }
                        lv_group_focus_prev(lv_group_get_default());
                    }
                    show_play_bar(true);
                    lv_timer_reset(bar_show_timer);
                    break; 
                case V_KEY_UP :
                    if(m_cur_file_list->media_type==MEDIA_TYPE_VIDEO||m_cur_file_list->media_type==MEDIA_TYPE_MUSIC){
                        //used for bar if 
                        if(lv_group_get_focused(lv_group_get_default())!=ui_playbar)
                        {
                            focus_obj=lv_group_get_focused(lv_group_get_default());
                            lv_group_add_obj(lv_group_get_default(),ui_playbar);
                            lv_group_focus_obj(ui_playbar);
                        }
                    }
                    show_play_bar(true);
                    lv_timer_reset(bar_show_timer);
                    break;
                case V_KEY_DOWN:
                    if(m_cur_file_list->media_type==MEDIA_TYPE_VIDEO||m_cur_file_list->media_type==MEDIA_TYPE_MUSIC){
                        //used for bar 
                        if(lv_group_get_focused(lv_group_get_default())==ui_playbar)
                        {
                            lv_obj_clear_state(ui_playbar,LV_STATE_ANY);
                            lv_group_remove_obj(ui_playbar);
                            lv_group_focus_obj(focus_obj);
                        }
                    }
                    show_play_bar(true);
                    lv_timer_reset(bar_show_timer);
                    break;
                case V_KEY_ENTER:
                    ctrl_bar_enter(target);
                    if(lv_obj_is_valid(ui_play_bar)){  
                        //按键stop 时，会跳转页面，不要操作本页面的obj
                        show_play_bar(true);
                        lv_timer_reset(bar_show_timer);
                    }
                    break;
                case V_KEY_EXIT : 
                    if (m_play_bar_show){
                        show_play_bar(false);
                    }
                    break;
                case V_KEY_PLAY:
                    if(m_cur_media_hld!=NULL){
                            P_STA= media_get_state(m_cur_media_hld);
                        if (MEDIA_PLAY == P_STA){
                            media_pause(m_cur_media_hld);
                            lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                            set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                            set_label_text2(ui_playstate,STR_PAUSE,FONT_MID);
                        }else if (MEDIA_STOP == P_STA){
                            media_play(m_cur_media_hld, m_cur_media_hld->play_name);
                            lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                            set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                            set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
                        }
                        else{
                            media_resume(m_cur_media_hld);
                            lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                            set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                            set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
                        }
                    }//ebook has not this func
                    break;
                case V_KEY_PAUSE:
                   if(m_cur_media_hld!=NULL){
                            P_STA= media_get_state(m_cur_media_hld);
                        if (MEDIA_PLAY == P_STA){
                            media_pause(m_cur_media_hld);
                            lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                            set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                            set_label_text2(ui_playstate,STR_PAUSE,FONT_MID);
                        }else if (MEDIA_STOP == P_STA){
                            media_play(m_cur_media_hld, m_cur_media_hld->play_name);
                            lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                            set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                            set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
                        }
                        else{
                            media_resume(m_cur_media_hld);
                            lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                            set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                            set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
                        }
                    }//ebook has not this func
                    break;
                case V_KEY_STOP:
                    _ui_screen_change(ui_fspage,0,0);
                    break;                
                case V_KEY_FB:
                    if(m_cur_media_hld!=NULL&&m_cur_media_hld->type==MEDIA_TYPE_VIDEO){
                        P_STA= media_get_state(m_cur_media_hld);
                        if (P_STA!=MEDIA_STOP ){
                            media_fastbackward(m_cur_media_hld);
                        }
                        set_label_text2(ui_playstate,STR_FB,FONT_MID);
                        ctrlbar_reflesh_speed();
                    }
                    break;
                case V_KEY_FF:
                    if(m_cur_media_hld!=NULL&&m_cur_media_hld->type==MEDIA_TYPE_VIDEO){
                        P_STA= media_get_state(m_cur_media_hld);
                        if (P_STA!=MEDIA_STOP ){
                            media_fastforward(m_cur_media_hld);
                        }    
                        set_label_text2(ui_playstate,STR_FF,FONT_MID);
                        ctrlbar_reflesh_speed();
                    }
                    break;
                case V_KEY_PREV:
                    app_reset_diszoom_param();  //before play reset dis area if zoom+/-
                    full_filename = win_media_get_pre_file(app_get_playlist_t()); 
                    if (full_filename){
                        if(m_cur_media_hld!=NULL){
                            P_STA= media_get_state(m_cur_media_hld);
                            if (MEDIA_STOP != P_STA)
                                media_stop(m_cur_media_hld);
                            media_play(m_cur_media_hld, full_filename);
                        }else{
                            ebook_read_file(full_filename);
                        }
                        m_play_file_name = win_media_get_cur_file_name(app_get_playlist_t());
                        lv_label_set_text(ui_playname, m_play_file_name);
                    }
                    break;                
                case V_KEY_NEXT:
                    app_reset_diszoom_param();  //before play reset dis area if zoom+/-
                    full_filename = win_media_get_next_file(app_get_playlist_t()); 
                    if (full_filename){
                        if(m_cur_media_hld!=NULL){
                            P_STA= media_get_state(m_cur_media_hld);
                            if (MEDIA_STOP != P_STA)
                                media_stop(m_cur_media_hld);
                            media_play(m_cur_media_hld, full_filename);
                        }else{
                            ebook_read_file(full_filename);
                        }
                        m_play_file_name = win_media_get_cur_file_name(app_get_playlist_t());
                        lv_label_set_text(ui_playname, m_play_file_name);
                    }
                    break;
                default:   
                    break;
            }

    return 0;
}

void ctrl_bar_keyinput_event_cb(lv_event_t *event){
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target =lv_event_get_target(event);
    if(code == LV_EVENT_PRESSED){
        lv_obj_clear_state(target,LV_STATE_PRESSED);
    }else if(code == LV_EVENT_KEY){
        uint32_t lv_key = lv_indev_get_key(lv_indev_get_act());
        uint32_t user_vkey=key_convert_vkey(lv_key);   // other key convert2 user_vkey

        // for backlight func press any btn to set backlight  on when backlight is off 
        if(blacklight_val==true){
            app_set_blacklight(blacklight_val);
            printf("backlight:%d\n",blacklight_val);
            blacklight_val=!blacklight_val;
            return;
        }
        if(lv_obj_has_flag(ui_play_bar,LV_OBJ_FLAG_HIDDEN)){
            //mean ctrlbar is hidden
            if(lv_key>LV_KEY_ESC){
                //mean press user_vkey
               user_vkey_action_ctrl(user_vkey,target);
            }else{
                if(lv_key==LV_KEY_ESC){
                    _ui_screen_change(ui_fspage,0,0);
                }else{
                    show_play_bar(true);
                    lv_timer_reset(bar_show_timer);
                }
            } 
        }else{
            //mean ctrlbar is on show 
            user_vkey_action_ctrl(user_vkey,target);    
        } 
    }
}

int Ctrlbar_mediastate_refr(void);

int ctrl_bar_enter(lv_obj_t * parent_btn)
{
    char *play_name = NULL;
    char *m_play_file_name=NULL;
    media_handle_t* m_hdl=mp_get_cur_player_hdl();
    if(m_cur_file_list->media_type!=MEDIA_TYPE_TXT){
        play_state= media_get_state(m_cur_media_hld);
    }
    if(parent_btn==ctrlbarbtn[0])  //PAUSE/PLAY
    {
        if(m_cur_file_list->media_type!=MEDIA_TYPE_TXT){
            if (MEDIA_PLAY == play_state){
                media_pause(m_cur_media_hld);
                lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
            }else if (MEDIA_STOP == play_state){
                media_play(m_cur_media_hld, m_cur_media_hld->play_name);
                lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
            }
            else{
                media_resume(m_cur_media_hld);
                lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
            }
        }else if(m_cur_file_list->media_type==MEDIA_TYPE_TXT){
            change_ebook_txt_info(LV_KEY_UP);
        }
    }
    else if(parent_btn==ctrlbarbtn[1]) //ROTATE
    {
        switch(m_cur_file_list->media_type){
            case MEDIA_TYPE_VIDEO:
                if (MEDIA_STOP != play_state){
                    media_fastbackward(m_cur_media_hld);
                }
                break;
            case MEDIA_TYPE_MUSIC:
                break;
            case MEDIA_TYPE_PHOTO:
                switch(m_hdl->pic_rotate)
                {
                    case ROTATE_TYPE_0:
                        m_hdl->pic_rotate=ROTATE_TYPE_90;
                        break;
                    case ROTATE_TYPE_90:
                        m_hdl->pic_rotate=ROTATE_TYPE_180;
                        break;
                    case ROTATE_TYPE_180:
                        m_hdl->pic_rotate=ROTATE_TYPE_270;
                        break;
                    case ROTATE_TYPE_270:
                        m_hdl->pic_rotate=ROTATE_TYPE_0;
                        break;
                }
                if(m_hdl!=NULL){
                    if (MEDIA_PLAY == play_state){
                        media_pause(m_cur_media_hld);
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                    }
                    media_pic_change_rotate(m_hdl, m_hdl->pic_rotate);
                }
                break;
            case MEDIA_TYPE_TXT:
                change_ebook_txt_info(LV_KEY_DOWN);
                break;
            default :
                break;
        }
    }
    else if(parent_btn==ctrlbarbtn[2]) //ROTATE
    {
        switch(m_cur_file_list->media_type){
            case MEDIA_TYPE_VIDEO:
                if (MEDIA_STOP != play_state)//ff
                    media_fastforward(m_cur_media_hld);
                break;
            case MEDIA_TYPE_MUSIC:
                break;
            case MEDIA_TYPE_PHOTO:
                switch(m_hdl->pic_rotate)//rotate
                {
                    case ROTATE_TYPE_0:
                        m_hdl->pic_rotate=ROTATE_TYPE_270;
                        break;
                    case ROTATE_TYPE_270:
                        m_hdl->pic_rotate=ROTATE_TYPE_180;
                        break;
                    case ROTATE_TYPE_180:
                        m_hdl->pic_rotate=ROTATE_TYPE_90;
                        break;
                    case ROTATE_TYPE_90:
                        m_hdl->pic_rotate=ROTATE_TYPE_0;
                        break;
                }
                if(m_hdl!=NULL){
                    if (MEDIA_PLAY == play_state){
                        media_pause(m_cur_media_hld);
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                    }
                    media_pic_change_rotate(m_cur_media_hld, m_hdl->pic_rotate);
                }
                break;
            case MEDIA_TYPE_TXT://prev
                play_name = win_media_get_pre_file(app_get_playlist_t()); 
                if(play_name){
                    ebook_read_file(play_name);
                    m_play_file_name = win_media_get_cur_file_name(app_get_playlist_t());
                    lv_label_set_text(ui_playname, m_play_file_name);
                }
                break;
            default :
                break;
        }
    }
    else if(parent_btn==ctrlbarbtn[3]) //PREV
    {
        if(m_cur_file_list->media_type!=MEDIA_TYPE_TXT){//prev
            app_reset_diszoom_param();  //for zoom func to reset zoom_t param
            play_name = win_media_get_pre_file(app_get_playlist_t()); 
            if (play_name){
                if (MEDIA_STOP != play_state)
                    media_stop(m_cur_media_hld);
                media_play(m_cur_media_hld, play_name);
                lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                m_play_file_name = win_media_get_cur_file_name(app_get_playlist_t());
                lv_label_set_text(ui_playname, m_play_file_name);
                if(m_cur_file_list->media_type==MEDIA_TYPE_PHOTO){
                    m_hdl->pic_rotate=ROTATE_TYPE_0;
                }
            }
        }else{
            play_name = win_media_get_next_file(app_get_playlist_t()); //next
            if(play_name){
                ebook_read_file(play_name);
                m_play_file_name = win_media_get_cur_file_name(app_get_playlist_t());
                lv_label_set_text(ui_playname, m_play_file_name);
            }
        }

    }
    else if(parent_btn==ctrlbarbtn[4]) //NEXT
    {
        if(m_cur_file_list->media_type!=MEDIA_TYPE_TXT){        //next
            app_reset_diszoom_param();  //before play reset dis area
            play_name = win_media_get_next_file(app_get_playlist_t()); 
            if (play_name){
                if (MEDIA_STOP != play_state)
                    media_stop(m_cur_media_hld);
                media_play(m_cur_media_hld, play_name);
                lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                m_play_file_name = win_media_get_cur_file_name(app_get_playlist_t());
                lv_label_set_text(ui_playname, m_play_file_name);
                if(m_cur_file_list->media_type==MEDIA_TYPE_PHOTO){
                    m_hdl->pic_rotate=ROTATE_TYPE_0;
                }
            }
        }else{
            _ui_screen_change(ui_fspage,0,0);               //stop
        }

    }
    else if(parent_btn==ctrlbarbtn[5]) //STOP
    {
        if(m_cur_file_list->media_type!=MEDIA_TYPE_TXT){
            //stop
            _ui_screen_change(ui_fspage,0,0);
        }else{
            //for bg music
            if(is_ebook_bgmusic==false){
                //start play music if has songs on bslist
                glist* bsplaylist=app_get_bsplayer_glist();
                if(bsplaylist!=NULL&&bsplaylist->next!=NULL){
                    lv_obj_set_style_bg_img_src(parent_btn,&IDB_Hint_Music_On,0);
                    set_label_text2(lv_obj_get_child(parent_btn,0),STR_MUSIC_OFF,FONT_MID);
                    backstage_player_task_start(0,NULL);
                    is_ebook_bgmusic=true;
                }
            }else{
                lv_obj_set_style_bg_img_src(parent_btn,&IDB_Hint_Music_Off,0);
                set_label_text2(lv_obj_get_child(parent_btn,0),STR_MUSIC_ON,FONT_MID);
                backstage_player_task_stop(0,NULL);
                is_ebook_bgmusic=false;
            }    
        }
    }
    else if(parent_btn==ctrlbarbtn[6]) 
    {
        if(m_cur_file_list->media_type!=MEDIA_TYPE_TXT){
            if(m_hdl->loop_type==PlAY_LIST_SEQUENCE){
                set_label_text2(lv_obj_get_child(ctrlbarbtn[6],0),STR_SINGLEROUND,FONT_MID);
                m_hdl->loop_type=PLAY_LIST_ONE;
            }else if(m_hdl->loop_type==PLAY_LIST_ONE){
                set_label_text2(lv_obj_get_child(ctrlbarbtn[6],0),STR_LISTROUND,FONT_MID);
                m_hdl->loop_type=PlAY_LIST_SEQUENCE;
            }
        }else{
            create_playlist_win(ui_ebook_txt,parent_btn);
        }
    }
    else if(parent_btn==ctrlbarbtn[7]) // commom info 
    {
        if(m_cur_file_list->media_type!=MEDIA_TYPE_TXT){
            create_mpinfo_win(lv_scr_act(),parent_btn);    
        }else{
            create_mpinfo_win(ui_ebook_txt,parent_btn);
        }
    }
    else if(parent_btn==ctrlbarbtn[8]) //plyer list 
    {
        //create a list of player ready
        if(m_cur_file_list->media_type!=MEDIA_TYPE_TXT){
            create_playlist_win(lv_scr_act(),parent_btn);
        }else{
            //for music source 
            create_musiclist_win(ui_ebook_txt,parent_btn);
        }
    }
    else if(parent_btn==ctrlbarbtn[9]) 
    {
        //zoom + or close black light 
        if(m_cur_file_list->media_type!=MEDIA_TYPE_MUSIC)
        {
            //player pause when zoom
            if (MEDIA_PLAY == media_get_state(m_cur_media_hld)&&m_cur_file_list->media_type==MEDIA_TYPE_PHOTO){
                media_pause(m_cur_media_hld);
                lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
            }else{
                api_set_display_aspect(DIS_TV_AUTO,DIS_NORMAL_SCALE);
            }
            app_set_diszoom(MPZOOM_IN); 
            Zoom_Param_t * zoom_args=app_get_zoom_param();
            if(zoom_args->zoom_state>0){
                m_cur_media_hld->state=MEDIA_ZOOM_IN;
                m_cur_media_hld->zoom_size=zoom_args->zoom_size;
            }else if(zoom_args->zoom_state<0){
                m_cur_media_hld->state=MEDIA_ZOOM_OUT;
                m_cur_media_hld->zoom_size=zoom_args->zoom_size;
            }else if(zoom_args->zoom_state==0){
                if(m_cur_media_hld->type==MEDIA_TYPE_PHOTO){
                    m_cur_media_hld->state=MEDIA_PAUSE;
                }else
                    m_cur_media_hld->state=MEDIA_PLAY;
            }

        }else {
            app_set_blacklight(blacklight_val); //off backlight
            printf("backlight:%d\n",blacklight_val);
            blacklight_val=!blacklight_val;
        }
    }
    else if(parent_btn==ctrlbarbtn[10]) //zoom - or mute in music
    {
        if(m_cur_file_list->media_type!=MEDIA_TYPE_MUSIC)
        {
            if (MEDIA_PLAY == media_get_state(m_cur_media_hld)&&m_cur_file_list->media_type==MEDIA_TYPE_PHOTO)
                media_pause(m_cur_media_hld);
            app_set_diszoom(MPZOOM_OUT);
            Zoom_Param_t * zoom_args=app_get_zoom_param();
            if(zoom_args->zoom_state>0){
                m_cur_media_hld->state=MEDIA_ZOOM_IN;
                m_cur_media_hld->zoom_size=zoom_args->zoom_size;
            }else if(zoom_args->zoom_state<0){
                m_cur_media_hld->state=MEDIA_ZOOM_OUT;
                m_cur_media_hld->zoom_size=zoom_args->zoom_size;
            }else if(zoom_args->zoom_state==0){
                if(m_cur_media_hld->type==MEDIA_TYPE_PHOTO){
                    m_cur_media_hld->state=MEDIA_PAUSE;
                }else
                    m_cur_media_hld->state=MEDIA_PLAY;
            }
        }
        else
        {
            // static bool is_mute = false;
            static uint8_t vol_back = 0;
            is_mute = !is_mute;
            
            if(is_mute){
                vol_back = projector_get_some_sys_param(P_VOLUME);
            }
            api_media_mute(is_mute);
            if (is_mute){
                projector_set_some_sys_param(P_VOLUME, 0);
                
            }else{
                projector_set_some_sys_param(P_VOLUME, vol_back);
            }
            create_mute_icon();
        }
    }
    else if(parent_btn==ctrlbarbtn[11]) 
    {

        if(m_cur_media_hld->type==MEDIA_TYPE_PHOTO){
            Zoom_Param_t* cur_zoomparam=app_get_zoom_param();
            if(cur_zoomparam->zoom_size>ZOOM_NORMAL)
                //zoom move must after zoom +
                create_zoommoove_win(ui_play_bar,parent_btn);
        }
        else if(m_cur_media_hld->type==MEDIA_TYPE_VIDEO)
        {
            if (MEDIA_STOP != play_state)
                media_slowforward(m_cur_media_hld);
        }
    
    }
    else if(parent_btn==ctrlbarbtn[12]) //on off bgmusic
    {
        /*for media step*/
        if(m_cur_media_hld->type==MEDIA_TYPE_VIDEO){ 
            /*500 ms after to do sonthing */
            /*create& start  a timer,after timer goto timer cb*/  
            /* or lvgl task sleep 200s then reset*/
            if(MEDIA_PAUSE == media_get_state(m_cur_media_hld))
                media_resume(m_cur_media_hld);
                api_sleep_ms(200);
            if (MEDIA_PLAY == media_get_state(m_cur_media_hld))
                media_pause(m_cur_media_hld);
        }
        else if(m_cur_media_hld->type==MEDIA_TYPE_PHOTO)
        {
            if(is_bg_music==false)
            {
                //start play music if has songs on bslist
                glist* bsplaylist=app_get_bsplayer_glist();
                if(bsplaylist!=NULL&&bsplaylist->next!=NULL){
                    lv_obj_set_style_bg_img_src(ctrlbarbtn[12],&IDB_Hint_Music_On,0);
                    set_label_text2(lv_obj_get_child(ctrlbarbtn[12],0),STR_MUSIC_OFF,FONT_MID);
                    backstage_player_task_start(0,NULL);
                    is_bg_music=true;
                }
            }else{
                lv_obj_set_style_bg_img_src(ctrlbarbtn[12],&IDB_Hint_Music_Off,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[12],0),STR_MUSIC_ON,FONT_MID);
                backstage_player_task_stop(0,NULL);
                is_bg_music=false;
            }    
        }
    }
    else if(parent_btn==ctrlbarbtn[13]) //music source or ratio
    {
        if(m_cur_media_hld->type==MEDIA_TYPE_VIDEO)
        {
            //add a state machine
             switch (projector_get_some_sys_param(P_ASPECT_RATIO))
            {
            case DIS_TV_AUTO:
                projector_set_some_sys_param(P_ASPECT_RATIO, DIS_TV_4_3);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[13],0),STR_ASPECT_4_3,FONT_MID);
                break;
            case DIS_TV_4_3:
                projector_set_some_sys_param(P_ASPECT_RATIO, DIS_TV_16_9);  
                set_label_text2(lv_obj_get_child(ctrlbarbtn[13],0),STR_ASPECT_16_9,FONT_MID);        
                break;
            case DIS_TV_16_9:
                projector_set_some_sys_param(P_ASPECT_RATIO, DIS_TV_AUTO);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[13],0),STR_ASPECT_AUTO,FONT_MID);        
                break;
            default:
                break;
            }
            projector_sys_param_save();
            api_set_display_area(projector_get_some_sys_param(P_ASPECT_RATIO));
        }
        else if(m_cur_media_hld->type==MEDIA_TYPE_PHOTO)
        {
            create_musiclist_win(lv_scr_act(),parent_btn);
        }
    }
    else if(parent_btn==ctrlbarbtn[14])
    {
        if(m_cur_media_hld->type==MEDIA_TYPE_PHOTO)
        {
            switch(img_effect.mode)
            {
                case IMG_SHOW_NULL:
                    api_dis_show_onoff(false); 
                    img_effect.mode=IMG_SHOW_SHUTTERS;
                    set_img_dis_modeparam(&img_effect);
                    set_label_text2(lv_obj_get_child(parent_btn,0),STR_SHUTTER_EFFECT,FONT_MID);        
                    break;
                case IMG_SHOW_NORMAL: 
                    img_effect.mode=IMG_SHOW_SHUTTERS;
                    set_img_dis_modeparam(&img_effect);
                    set_label_text2(lv_obj_get_child(parent_btn,0),STR_SHUTTER_EFFECT,FONT_MID);        
                    break;
                case IMG_SHOW_SHUTTERS: 
                    img_effect.mode=IMG_SHOW_BRUSH;
                    set_img_dis_modeparam(&img_effect);
                    set_label_text2(lv_obj_get_child(parent_btn,0),STR_BRUSH_EFFECT,FONT_MID);        
                    break;
                case IMG_SHOW_BRUSH: 
                    img_effect.mode=IMG_SHOW_SLIDE;
                    set_img_dis_modeparam(&img_effect);
                    set_label_text2(lv_obj_get_child(parent_btn,0),STR_SLIDE_EFFECT,FONT_MID);        
                    break;
                case IMG_SHOW_SLIDE: 
                    img_effect.mode=IMG_SHOW_RANDOM;
                    set_img_dis_modeparam(&img_effect);
                    set_label_text2(lv_obj_get_child(parent_btn,0),STR_RAND_EFFECT,FONT_MID);        
                    break;
                case IMG_SHOW_RANDOM: 
                    img_effect.mode=IMG_SHOW_FADE;
                    set_img_dis_modeparam(&img_effect);
                    set_label_text2(lv_obj_get_child(parent_btn,0),STR_FADE_EFFECT,FONT_MID);        
                    break;
                case IMG_SHOW_FADE: 
                    img_effect.mode=IMG_SHOW_NULL;
                    api_pic_effect_enable(false);
                    set_img_dis_modeparam(&img_effect);
                    set_label_text2(lv_obj_get_child(parent_btn,0),STR_NO_EFFECT,FONT_MID);        
                    break; 
                default :
                    break;
            }    
        }
    }
    Ctrlbar_mediastate_refr();
    return 0;
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
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);

    play_time = media_get_playtime(m_cur_media_hld);
    format_time(play_time, time_fmt);
    lv_label_set_text(lv_obj_get_child(ui_play_bar,2), time_fmt);
    lv_slider_set_value(ui_playbar, play_time, LV_ANIM_ON);

    total_time = media_get_totaltime(m_cur_media_hld);
    format_time(total_time, time_fmt);
    lv_label_set_text(lv_obj_get_child(ui_play_bar,4), time_fmt);
    if (total_time > 0)
        lv_slider_set_range(ui_playbar, 0, total_time);
    
    play_state= media_get_state(m_cur_media_hld);
    Ctrlbar_mediastate_refr();
}

void show_play_bar(bool show)
{
    if (show){
        lv_obj_clear_flag(ui_play_bar, LV_OBJ_FLAG_HIDDEN);
    }else{
        lv_obj_add_flag(ui_play_bar, LV_OBJ_FLAG_HIDDEN);
    }
    m_play_bar_show = show;

}

void bar_show_timer_cb(lv_timer_t * t)
{
    show_play_bar(false);

}


#ifdef RTOS_SUBTITLE_SUPPORT
subtitles_event_t subtitles_e = -1;
char *subtitles_str = NULL;
static lv_img_dsc_t subtitles_img_dsc = {0};
lv_timer_t *subtitles_timer = NULL;
uint16_t subtitles_type = -1;

static void subtitles_event_cb(){
    switch (subtitles_e)
    {
    case SUBTITLES_EVENT_SHOW:
        if(subtitles_type == 1){
            show_subtitles(subtitles_str);             
        }else{
            show_subtitles_pic(&subtitles_img_dsc);
        }
        
        break;
    case SUBTITLES_EVENT_HIDDEN:
        if(subtitles_type == 1){
            show_subtitles("");            
        }else{
            show_subtitles_pic(NULL);
        }

        break;
    case SUBTITLES_EVENT_PAUSE:
        printf("subtitles pause\n");
        if(subtitles_timer){
            lv_timer_pause(subtitles_timer);
        }
        break;
    case SUBTITLES_EVENT_CLOSE:
        
        if(subtitles_timer){
            lv_timer_pause(subtitles_timer);
            lv_timer_del(subtitles_timer);
            subtitles_timer = NULL;
            printf("subtitles close\n");
        }

        break;
    default:
        break;
    }
    subtitles_e = -1;
}

void subtitles_event_send(int e, lv_subtitle_t *subtitle){

    
    if(e == SUBTITLES_EVENT_RESUME){
        if (subtitles_timer){
            lv_timer_resume(subtitles_timer);
            lv_timer_reset(subtitles_timer);
        }        
    }else{
        subtitles_e = e;
        if(subtitle == NULL){
            return;
        }
        if(subtitle->type == 1){
            subtitles_str = subtitle->data;     
        }else if(subtitle->type == 0){
            subtitles_img_dsc.header.w = subtitle->w;
            subtitles_img_dsc.header.h = subtitle->h;
            subtitles_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
            subtitles_img_dsc.data_size = subtitle->w*subtitle->h*LV_IMG_PX_SIZE_ALPHA_BYTE;
            subtitles_img_dsc.data = subtitle->data;
        }
       subtitles_type = subtitle->type;
    }

}

void subtitles_timer_handle(lv_timer_t *e){
    subtitles_event_cb();
}

void create_subtitles_rect(void){

    if(m_cur_media_hld->type == MEDIA_TYPE_VIDEO){
        lv_obj_t *obj = lv_obj_create(ui_ctrl_bar);
        lv_obj_move_background(obj);
        lv_obj_set_size(obj, LV_PCT(80), LV_PCT(18));
        lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
        lv_obj_set_style_pad_all(obj, 0, 0);
        lv_obj_set_style_border_width(obj, 0, 0);
        lv_obj_set_style_outline_width(obj, 0, 0);
        lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

        subtitles_obj = lv_label_create(obj);
        lv_obj_set_style_border_width(subtitles_obj, 0, 0);
        lv_obj_set_style_pad_bottom(subtitles_obj, 0, 0);
        lv_obj_set_size(subtitles_obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_label_set_text(subtitles_obj, "");
        lv_obj_set_style_text_font(subtitles_obj, &LISTFONT_3000, 0);
        lv_obj_set_style_text_color(subtitles_obj, lv_color_white(), 0);
        lv_obj_set_style_text_align(subtitles_obj, LV_TEXT_ALIGN_CENTER, 0);
        //lv_obj_center(subtitles_obj);
        lv_obj_align(subtitles_obj, LV_ALIGN_BOTTOM_MID, 0, 0);

        subtitles_obj_pic = lv_img_create(obj);
        lv_img_set_src(subtitles_obj_pic, NULL);
        //lv_obj_center(subtitles_obj_pic);
        lv_obj_align(subtitles_obj_pic, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_size(subtitles_obj_pic, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

        if(!subtitles_timer){
            subtitles_e = -1;
            subtitles_timer = lv_timer_create(subtitles_timer_handle, 500, 0);
            //lv_timer_set_repeat_count(subtitles_timer, -1);
            printf("subtitles timer create\n");
            lv_timer_reset(subtitles_timer);
        }



    }

}

void del_subtitles_rect(void){
    subtitles_event_send(SUBTITLES_EVENT_CLOSE, NULL);
}

void show_subtitles( char *str){
    if(str==""){
        lv_label_set_text(subtitles_obj, "");
        return;
    }
    str = subtitles_str_remove_prefix(str);
    subtitles_str_get_text(str);   
    lv_obj_move_foreground(subtitles_obj);
    //lv_label_set_text(subtitles_obj, str);
}

void show_subtitles_pic(lv_img_dsc_t *dsc){
    if(dsc == NULL){
        lv_obj_add_flag(subtitles_obj_pic, LV_OBJ_FLAG_HIDDEN);
    }else{
        lv_obj_clear_flag(subtitles_obj_pic, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(subtitles_obj_pic, dsc);
        lv_obj_move_foreground(subtitles_obj_pic);        
    }

}

char* subtitles_str_remove_prefix(char* str){
    char *str_p = NULL;
    int dot_count = 0;
    for(int i=0; i<strlen(str);i++){
        if(str[i] == ','){
            dot_count++;
            if(dot_count == 9){
                if(i+1 < strlen(str)){
                    return str+i+1;
                }else{
                    return "";
                }
                
            }
        }
    }
    return "";
}
/*
* 
*/
void subtitles_str_get_text(char* str){
    int a=-1, first = -1, second = -1;
    int size = strlen(str);
    printf("str size: %d\n", size);
    for(int i=0; i<size;i++){
        //printf("%c\n", str[i]);
        if(str[i] == 0x5c && i+1<size && str[i+1] == 'N'){
            str[i] = '\0';
            i+=2;
            a=i;
        }
        while(str[i] == '{'){
            str[i] = '\0';
            a = 1+i;
            while (i < size && str[i] != '}'){
                i++;
            }
            if(i<size && str[i] == '}'){
                str[i] = '\0';
                subtitles_str_set_style(str+a);
            }
            i++;
        }
        if(str[i] == '\n' || str[i] == '\r'){
            str[i] = '\0';
        }

        if(first == -1){
            first = i;
        }else if(a > 0 && second == -1){
            second = i;
        }
        a=-1;
    }
    if(first>=0){
        if(second>first){
            lv_label_set_text_fmt(subtitles_obj,"%s\n%s", str+first, str+second);             
        }else{
            lv_label_set_text(subtitles_obj, str+first);
        }
    }else{
        lv_label_set_text(subtitles_obj, "");
    }

}

void subtitles_str_set_style(char *str){
    // if(strncmp(str, "\c&H", 4) == 0){

    // }
}
/**
 * @description: sepecial handle for ext subtitle format .idx , if has this format(xxx.idx)
 * do not add xxx.sub in subtitle list or del it in subs list 
 * @return {*}
 * @author: Yanisin
 */
glist* ext_subtitle_format_handle(glist* subs_glist)
{
    glist* dst_glist=NULL;
    // scan subglist ,if subsglist has a .idx 
    if(subs_glist!=NULL){
        dst_glist=subs_glist;
        for(int i=0;i<glist_length(subs_glist);i++){
            char *file_str=(char *)glist_nth_data(subs_glist,i);
            if(strstr(file_str,".idx")){
                char file_without_ext[1024]={0};
                file_mgr_rm_extension(file_without_ext,file_str);
                for(int j=0;j<glist_length(subs_glist);j++){
                    if(!strncmp(file_without_ext,(char *)glist_nth_data(subs_glist,j),strlen(file_without_ext))){
                        // finded samename in glist
                        if(file_mgr_optional_filter((char *)glist_nth_data(subs_glist,j),"sub")){
                            dst_glist=glist_delete_link(subs_glist,glist_nth(subs_glist,j));
                        }
                    }
                }
            }
        }
    }
    return dst_glist;
}
static ext_subtitle_t ext_subtitle;
#define MAX_EXT_SUBTITLE_NUM 100
char *m_uris[MAX_EXT_SUBTITLE_NUM]={NULL};
int ext_subtitles_init(file_list_t* src_list)
{
    if(src_list==NULL||src_list->list==NULL){
        return 0;
    }
    if(src_list->media_type==MEDIA_TYPE_VIDEO||src_list->media_type==MEDIA_TYPE_MUSIC){
        glist* subs_list=file_mgr_subtitile_list_get();
        subs_list=ext_subtitle_format_handle(subs_list);
        // idx file had to specal handle 
        if(subs_list!=NULL){
            int len=glist_length(subs_list);
            int j=0;
            file_node_t * file_node = file_mgr_get_file_node(src_list, src_list->item_index);
            if(file_node->name!=NULL){
                char file_without_ext[1024]={0};
                file_mgr_rm_extension(file_without_ext,file_node->name);
                for(int i=0;i<len;i++){
                    if(!strncmp((char *)glist_nth_data(subs_list,i),file_without_ext,strlen(file_without_ext))){
                        // printf("--------> %s ,add in ext_subtitle struct\n",file_without_ext);
                        ext_subtitle.ext_subs_count++;
                        char url_single[1024]={0};
                        file_mgr_get_fullname(url_single,src_list->dir_path,glist_nth_data(subs_list,i));
                        m_uris[j]=strdup(url_single);
                        // put it in a char * cont,strdup will malloc mem ,need to free mem when 
                        j++;
                        ext_subtitle.uris=m_uris;
                    }
                }            
            }

        }
    }
    return 0;
}
int ext_subtitle_deinit()
{
    glist* subs_list=file_mgr_subtitile_list_get();
    if(subs_list!=NULL&&ext_subtitle.ext_subs_count!=0){
            for(int i=0;i<ext_subtitle.ext_subs_count;i++){
                free(m_uris[i]);
                m_uris[i]=NULL;
            }
        memset(&ext_subtitle,0,sizeof(ext_subtitle_t));
    }
    return 0;
}
ext_subtitle_t * ext_subtitle_data_get()
{
    return &ext_subtitle;
}
#endif

//for media play bar 
void ctrlbarpage_open(void)
{
    file_node_t *file_node = NULL;
    char m_play_path_name[1024];
    //add group 
    play_bar_group= lv_group_create();
    play_bar_group->auto_focus_dis=1;
    set_key_group(play_bar_group);
    create_ctrlbarpage_scr(ui_ctrl_bar,ctrl_bar_keyinput_event_cb);
    file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
    lv_label_set_text(ui_playname,file_node->name);
    set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
    //play media file
    if(last_scr==SCREEN_SETUP||last_scr==SCREEN_CHANNEL)
    {
        //do nothing 
        set_keystone_disable(false);
        is_previwe_open=false;
    }
    else    
    {
        api_set_display_area(projector_get_some_sys_param(P_ASPECT_RATIO));
        playlist_init();    
        m_cur_media_hld = m_player_hld[m_cur_file_list->media_type];
        if (NULL == m_cur_media_hld){
            m_cur_media_hld = media_open(m_cur_file_list->media_type);
            m_player_hld[m_cur_file_list->media_type] = m_cur_media_hld;
        }
        file_mgr_get_fullname(m_play_path_name, m_cur_file_list->dir_path, file_node->name);
        media_play(m_cur_media_hld, m_play_path_name);
        //set_display_zoom_when_sys_scale();
    }

    switch(m_cur_media_hld->type){
        case MEDIA_TYPE_VIDEO:
            create_ctrlbar_in_video(ui_winbar);
            break;
        case MEDIA_TYPE_MUSIC:
            create_ctrlbar_in_music(ui_winbar);
            music_spectrum_start();
            api_set_display_aspect(DIS_TV_AUTO,DIS_NORMAL_SCALE);
            break;
        case MEDIA_TYPE_PHOTO:
            create_ctrlbar_in_photo(ui_winbar);
            api_set_display_aspect(DIS_TV_AUTO,DIS_NORMAL_SCALE);
            break;
        default :
            break;
    }
    show_play_bar(true);
    sec_counter_timer = lv_timer_create(sec_timer_cb, 1000, NULL); 
    bar_show_timer = lv_timer_create(bar_show_timer_cb, 5000, NULL);
    screen_submp=SCREEN_SUBMP3;
} 

void media_player_close(void)
{
    
    if(m_cur_media_hld){
        media_stop(m_cur_media_hld);
        media_close(m_cur_media_hld);
        m_player_hld[m_cur_file_list->media_type] = NULL;
        m_cur_media_hld = NULL;

    }
    api_pic_effect_enable(false);
}

void media_player_open(void)
{
    char m_play_path_name[1024];
    file_node_t *file_node = NULL;
    file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
    m_cur_media_hld = m_player_hld[m_cur_file_list->media_type];
    if (NULL == m_cur_media_hld){
        m_cur_media_hld = media_open(m_cur_file_list->media_type);
        m_player_hld[m_cur_file_list->media_type] = m_cur_media_hld;
    }
    file_mgr_get_fullname(m_play_path_name, m_cur_file_list->dir_path, file_node->name);
    media_play(m_cur_media_hld, m_play_path_name);
}
void media_player_open2(file_list_t*m_filelist)
{
    char m_play_path_name[1024];
    file_node_t *file_node = NULL;
    file_node = file_mgr_get_file_node(m_filelist, m_filelist->item_index);
    m_cur_media_hld = m_player_hld[m_filelist->media_type];
    if (NULL == m_cur_media_hld){
        m_cur_media_hld = media_open(m_filelist->media_type);
        m_player_hld[m_filelist->media_type] = m_cur_media_hld;
    }
    file_mgr_get_fullname(m_play_path_name, m_filelist->dir_path, file_node->name);
    media_play(m_cur_media_hld, m_play_path_name);
}
int ctrlbarpage_close(void)
{
    win_msgbox_msg_close();
    lv_group_remove_all_objs(play_bar_group);
    lv_group_del(play_bar_group);
    play_bar_group=NULL;
    clear_ctrlbarpage_scr();

    lv_timer_pause(sec_counter_timer);
    lv_timer_del(sec_counter_timer);

    lv_timer_pause(bar_show_timer);
    lv_timer_del(bar_show_timer); 

    switch(m_cur_media_hld->type){
        case MEDIA_TYPE_VIDEO:
            app_reset_diszoom_param();
            break;
        case MEDIA_TYPE_MUSIC:
            music_spectrum_stop();
            //reset backlight 
            if(blacklight_val==true){
                app_set_blacklight(blacklight_val);
                printf("backlight:%d\n",blacklight_val);
                blacklight_val=!blacklight_val;
            }
            break;
        case MEDIA_TYPE_PHOTO:
            if(is_bg_music==true){
                is_bg_music=false;
                backstage_player_task_stop(0,NULL);
            }
            //for zoom func
            app_reset_diszoom_param();
            break;
        default :
            break;
    }   
    if(cur_scr==SCREEN_CHANNEL||cur_scr==SCREEN_SETUP)//perss EPG /MENU
        return 0;
    playlist_deinit();    
    media_player_close();
    return 0;
} 

static char *m_str_ff[] = {"", ">> x2", " >> x4", ">> x8", ">> x16", ">> x24", ">> x32"};
static char *m_str_fb[] = {"", "<< x2", " << x4", "<< x8", "<< x16", "<< x24", "<< x32"};
static char *m_str_sf[] = {"", ">> 1/2", " >> 1/4", ">> 1/8", ">> 1/16", ">> 1/24"};
static char *m_str_sb[] = {"", "<< 1/2", " << 1/4", "<< 1/8", "<< 1/16", "<< 1/24"};
static char *m_str_zi[] = {"", "x2", "x4", "x8"};
static char *m_str_zo[] = {"", "x1/2", "x1/4", "x1/8"};

void ctrlbar_reflesh_speed(void)
{
    uint8_t speed = 0;
    char **str_speed = NULL;

    if (m_cur_media_hld==NULL)
        return ;
    play_state = media_get_state(m_cur_media_hld);
    speed = media_get_speed(m_cur_media_hld);
    Zoom_Param_t* zoom_mode=app_get_zoom_param();
    Zoom_size_e z_size =zoom_mode->zoom_size;
    if (speed==0&&z_size==0){
        lv_label_set_text(ui_speed, "");
        return;
    }
    if (MEDIA_FF == play_state){
        str_speed = m_str_ff;
        lv_label_set_text(ui_speed, str_speed[speed]); 
    }else if (MEDIA_FB == play_state){
        str_speed = m_str_fb;
        lv_label_set_text(ui_speed, str_speed[speed]); 
    }else if (MEDIA_SF == play_state){
        str_speed = m_str_sf;
        lv_label_set_text(ui_speed, str_speed[speed]); 
    }else if (MEDIA_SB == play_state){
        str_speed = m_str_sb;
        lv_label_set_text(ui_speed, str_speed[speed]);
    }else if (MEDIA_ZOOM_IN==play_state||MEDIA_ZOOM_OUT==play_state){
        if(zoom_mode->zoom_state>0){
            lv_label_set_text(ui_speed, m_str_zi[abs(zoom_mode->zoom_state)]);
        }else{
            lv_label_set_text(ui_speed, m_str_zo[abs(zoom_mode->zoom_state)]);
        }
    }
}


static int64_t get_sys_clock_time (void) //get ms
{
        struct timeval time_t;
        gettimeofday(&time_t, NULL);
        return time_t.tv_sec * 1000 + time_t.tv_usec / 1000;
}

static void __media_seek_proc(uint32_t key)
{
    uint32_t total_time = media_get_totaltime(m_cur_media_hld);
    uint32_t play_time_initial = media_get_playtime(m_cur_media_hld);
    uint32_t play_time = play_time_initial;
    uint32_t jump_interval = 0;
    uint32_t seek_time = 0;

    if (total_time < jump_interval)
        return ;

    int64_t cur_op_time = get_sys_clock_time();
    if ((cur_op_time - m_cur_media_hld->last_seek_op_time) < 1000) {
        m_cur_media_hld->jump_interval += 10;
    } else {
        m_cur_media_hld->jump_interval = 10;
    }

    m_cur_media_hld->last_seek_op_time = cur_op_time;
    
    if (m_cur_media_hld->jump_interval < 300)
        jump_interval = m_cur_media_hld->jump_interval;
    else
        jump_interval = 300;

    if (LV_KEY_LEFT == key){//seek backward
        if (play_time > jump_interval)
            seek_time = play_time - jump_interval;
        else
            seek_time = 0;
    }else if(LV_KEY_RIGHT==key)
    { //seek forward
        if ((play_time + jump_interval) > total_time)
            seek_time = total_time;
        else
            seek_time = play_time + jump_interval;
    }
    media_seek(m_cur_media_hld, seek_time);
}

static int media_msg_handle(uint32_t msg_type)
{
    int  ret=0;
    switch (msg_type)
    {
    case HCPLAYER_MSG_OPEN_FILE_FAILED:

		win_msgbox_msg_open(STR_FILE_FAIL, 3000, NULL, NULL);

        printf("%s  %d\n",__FUNCTION__,__LINE__);
        break;
    case HCPLAYER_MSG_STATE_READY:
        break;   
    case HCPLAYER_MSG_STATE_PLAYING:
        if(lv_obj_is_valid(subwin_mpinfo)){
            lv_event_send(subwin_mpinfo,LV_EVENT_REFRESH,NULL);
        }
        break;   
    case HCPLAYER_MSG_STATE_EOS :
        if(m_cur_media_hld->loop_type==PlAY_LIST_SEQUENCE){
            ctrl_bar_enter(ctrlbarbtn[4]);
        }else if(m_cur_media_hld->loop_type==PLAY_LIST_ONE){
            char * play_name = win_media_get_cur_file_name(app_get_playlist_t()); 
            if (play_name){
                if (MEDIA_STOP != play_state)
                    media_stop(m_cur_media_hld);
                media_play(m_cur_media_hld, m_cur_media_hld->play_name);
                set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
                set_label_text2(ui_speed,STR_NONE,FONT_MID);
            }
        }
        break;
    case HCPLAYER_MSG_STATE_TRICK_EOS: 
        if(m_cur_media_hld->loop_type==PlAY_LIST_SEQUENCE){
            ctrl_bar_enter(ctrlbarbtn[4]);
        }else if(m_cur_media_hld->loop_type==PLAY_LIST_ONE){
            char * play_name = win_media_get_cur_file_name(app_get_playlist_t());
            if (play_name){
                if (MEDIA_STOP != play_state)
                    media_stop(m_cur_media_hld);
                media_play(m_cur_media_hld, m_cur_media_hld->play_name);
                set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
                set_label_text2(ui_speed,STR_NONE,FONT_MID);
            }
        }
        break;
    case HCPLAYER_MSG_STATE_TRICK_BOS:
        media_resume(m_cur_media_hld);
        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
        set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
        set_label_text2(ui_speed,STR_NONE,FONT_MID);
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO:
        if(m_cur_media_hld->type==MEDIA_TYPE_VIDEO||m_cur_media_hld->type==MEDIA_TYPE_MUSIC){
            win_msgbox_msg_open(STR_AUDIO_USPT, 2000, NULL, NULL); 
        }
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO:
        if(m_cur_media_hld->type==MEDIA_TYPE_VIDEO){
            win_msgbox_msg_open(STR_VIDEO_USPT, 2000, NULL, NULL);
        }
        break;
    case HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE:
        if(m_cur_media_hld->type==MEDIA_TYPE_VIDEO){
            win_msgbox_msg_open(STR_USPT_VIDEO_TYPE, 2000, NULL, NULL);
        }
        break;
    case HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE:
        if(m_cur_media_hld->type==MEDIA_TYPE_VIDEO||m_cur_media_hld->type==MEDIA_TYPE_MUSIC){
	        win_msgbox_msg_open(STR_USPT_AUDIO_TYPE, 2000, NULL, NULL);
        }
        break;
    case HCPLAYER_MSG_AUDIO_DECODE_ERR:
        switch (m_cur_media_hld->type)
        {
            case MEDIA_TYPE_VIDEO:
            	win_msgbox_msg_open(STR_AUDIO_ERROR, 2000, NULL, NULL);
                break;
            case MEDIA_TYPE_MUSIC:
            	win_msgbox_msg_open(STR_AUDIO_ERROR, 2000, NULL, NULL);
                break;
            default :
                break;
        }
        break;
    case HCPLAYER_MSG_VIDEO_DECODE_ERR:
        switch (m_cur_media_hld->type)
        {
            case MEDIA_TYPE_VIDEO:
            	win_msgbox_msg_open(STR_VIDEO_ERROR, 2000, NULL, NULL);
                break;
            case MEDIA_TYPE_MUSIC:
                break;
            case MEDIA_TYPE_PHOTO:
                win_msgbox_msg_open(STR_PIC_ERROR, 2000, NULL, NULL);
            default :
                break;
        }
        break;
    default:
        break;
    }
    return ret;
}


image_effect_t*  get_img_effect_mode(void)
{
    return &img_effect;
}



void media_playbar_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

    if(MSG_TYPE_MSG == ctl_msg->msg_type)
    {
        media_msg_handle(ctl_msg->msg_code);
    }else if(ctl_msg->msg_type== MSG_TYPE_CMD){
        spectrum_uimsg_handle(ctl_msg->msg_code);
    }

}


// for anyother key reset the backlight
int ctrlbar_reset_mpbacklight(void)
{
    if(m_cur_media_hld==NULL){
        return -1;
    }else if(m_cur_media_hld->type==MEDIA_TYPE_MUSIC){
        if(blacklight_val==true){
            app_set_blacklight(blacklight_val);
            printf("backlight:%d\n",blacklight_val);
            blacklight_val=!blacklight_val;
        }
    }
    return 0;
}
/*
 for ui ctlbar show media_player state ,just refresh obj "ui_playstate" "ui_speed"
*/ 
int Ctrlbar_mediastate_refr(void)
{
    if(m_cur_media_hld!=NULL){
        switch(m_cur_media_hld->state){
            case MEDIA_PLAY:
                set_label_text2(ui_playstate,STR_PLAY,FONT_MID);        
                break;
            case MEDIA_PAUSE:
                set_label_text2(ui_playstate,STR_PAUSE,FONT_MID);        
                break;
            case MEDIA_FB:
                set_label_text2(ui_playstate,STR_FB,FONT_MID);        
                break;
            case MEDIA_FF:
                set_label_text2(ui_playstate,STR_FF,FONT_MID);        
                break;
            case MEDIA_SB:
                break;
            case MEDIA_SF:
                set_label_text2(ui_playstate,STR_SF,FONT_MID);                  
                break;
            case MEDIA_ZOOM_IN:
                set_label_text2(ui_playstate,STR_ZOOMIN,FONT_MID);                  
                break;
            case MEDIA_ZOOM_OUT:
                set_label_text2(ui_playstate,STR_ZOOMOUT,FONT_MID);                  
                break;
        }
    } 
    ctrlbar_reflesh_speed();       
    return 0;
}