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
#include <hcuapi/vidmp.h>

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
// #include "local_mp_ui_lld.h"
#include "mp_mainpage.h"
#include "mp_subpage.h"
#include "mp_fspage.h"
#include "mp_ctrlbarpage.h"
#include "mp_ebook.h"
#include "src/font/lv_font.h"
#include "setup.h"
#include "factory_setting.h"

#include "mul_lang_text.h"

#include "mp_bsplayer_list.h"
#include "backstage_player.h"
#include "mp_playerinfo.h"




//for player list subwin
lv_obj_t * mplist_item[MAX_MPLIST_ITEM] ={NULL};
file_list_t bs_play_filelist={0};   //only use for file list 
obj_list_ctrl_t bs_listctrl={0};    //use for key ctrl to focus lvgl obj like dts sys
glist *player_url_glist=NULL;
// return checkbox ->label
static lv_obj_t* create_list_subbtn_with_checkbox(lv_obj_t *parent)
{
    lv_obj_t *list_btn;
    list_btn = lv_list_add_btn(parent, NULL,NULL);
    lv_obj_align(list_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_group_remove_obj(list_btn);

    lv_obj_set_size(list_btn,LV_PCT(100),LV_SIZE_CONTENT);
    // lv_obj_set_style_pad_top(list_btn, 2, 0);
    // lv_obj_set_style_pad_left(list_btn, 5, 0);
    // lv_obj_set_style_pad_right(list_btn, 5, 0);
    lv_obj_set_style_pad_ver(list_btn,2,0);
    lv_obj_set_style_pad_hor(list_btn,0,0);


    // lv_obj_set_style_border_width(list_btn, 2, 0);
    // lv_obj_set_style_border_side(list_btn, LV_BORDER_SIDE_FULL, 0);
    // lv_obj_set_style_border_color(list_btn, lv_color_white(), 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_0, 0);
    // lv_obj_set_style_border_opa(list_btn, LV_OPA_100,  LV_STATE_FOCUS_KEY);

    lv_obj_set_style_bg_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(list_btn, LV_OPA_100,  LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_color(list_btn, lv_palette_main(LV_PALETTE_BLUE),  LV_STATE_FOCUS_KEY);

    lv_obj_set_style_text_color(list_btn, lv_color_white(), 0);
    lv_obj_set_style_text_color(list_btn, lv_color_black(),  LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_font(list_btn, &LISTFONT_3000, 0);
    

    //add checkbox child id 0
    lv_obj_t * checkbox=lv_checkbox_create(list_btn);
    lv_group_remove_obj(list_btn);
    lv_obj_set_style_text_color(checkbox, lv_color_white(), 0);
    // lv_obj_set_style_text_font(checkbox, &lv_font_montserrat_14, 0);
    lv_checkbox_set_text(checkbox, ""); 
    lv_obj_align(checkbox, LV_ALIGN_LEFT_MID, 0, 0);


    //add label child id 1
    lv_obj_t * sublabel=lv_label_create(checkbox);
    lv_obj_set_width(sublabel,CHECKBOX_LAB_W);
    lv_obj_set_style_bg_opa(sublabel, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(sublabel, LV_OPA_100,  LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_color(sublabel, lv_palette_main(LV_PALETTE_BLUE),  LV_STATE_FOCUS_KEY);

    lv_obj_set_style_text_color(sublabel, lv_color_white(), 0);
    lv_obj_set_style_text_color(sublabel, lv_color_black(),  LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_font(sublabel, &LISTFONT_3000, 0);
    lv_obj_align(sublabel,LV_ALIGN_LEFT_MID,CHECKBOX_LAB_X_OFS,0); //off set 

    lv_label_set_long_mode(sublabel,  LV_LABEL_LONG_CLIP);
    return sublabel;

}

//create a subobj with style 
lv_obj_t* create_list_sub_text_obj2(lv_obj_t *parent,int w, int h)
{
    static lv_obj_t *list_label;
    list_label = lv_list_add_text(parent, " ");
    lv_obj_set_style_text_align(list_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_size(list_label,LV_PCT(w),LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(list_label,MPPLAYLIST_LAB_PANHOR,0);
    lv_obj_set_style_pad_ver(list_label,MPPLAYLIST_LAB_PADVER,0);

    lv_obj_set_style_border_width(list_label, 2, 0);
    lv_obj_set_style_border_color(list_label, lv_color_white(), 0);
    lv_obj_set_style_border_opa(list_label, LV_OPA_0, 0);
    // lv_obj_set_style_border_opa(list_label, LV_OPA_100, LV_STATE_CHECKED);

    lv_obj_set_style_bg_opa(list_label, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(list_label, LV_OPA_100, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(list_label, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);

    lv_obj_set_style_text_color(list_label, lv_color_white(), 0);
    lv_obj_set_style_text_color(list_label, lv_color_black(), LV_STATE_CHECKED);

    lv_obj_set_style_text_font(list_label, &LISTFONT_3000, 0);
    lv_label_set_long_mode(list_label,  LV_LABEL_LONG_CLIP);

    return list_label;
}

static lv_obj_t* musiclist_obj=NULL; 
void create_musiclist_subwin(lv_obj_t* p,int w ,int h,lv_obj_t* sub_btn)
{
    musiclist_obj=create_list_obj2(p,w,h);
#ifdef LVGL_RESOLUTION_240P_SUPPORT
    lv_obj_set_height(musiclist_obj,LV_PCT(80));
    lv_obj_add_flag(musiclist_obj,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(musiclist_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
#endif
    for(int i=0;i<MAX_MPLIST_ITEM;i++)
        mplist_item[i]=create_list_subbtn_with_checkbox(musiclist_obj);
    lv_obj_add_event_cb(musiclist_obj, musiclist_win_event_cb, LV_EVENT_ALL, sub_btn);
    lv_group_focus_obj(musiclist_obj);
}

bool find_checkbox_form_glist(char * fslist_string,glist* player_glist)
{
    glist* temp_glist=player_glist->next; //从第一项开始，第0项无数据
    while(temp_glist!=NULL&&temp_glist->data!=NULL){
        if(strcmp(fslist_string,temp_glist->data)==0)
            return true;
        else{
            temp_glist=temp_glist->next;
        }
    }
    return false;
}
int add_checkbox2_musiclist(file_list_t *file_list)
{
    int node_count=file_list->file_count+file_list->dir_count;
    for(int i=0;i<node_count;i++){
        file_node_t *file_node=file_mgr_get_file_node(&bs_play_filelist,i);
        char  path_fullname[1024]={0};
        file_mgr_get_fullname(path_fullname, bs_play_filelist.dir_path, file_node->name);
        if(find_checkbox_form_glist(path_fullname,player_url_glist)==true){
            file_node->user_data=1;
        }
        else{
            file_node->user_data=0;
        }
    }
    return 0;
}

int clear_musiclist(void)
{
    for(int i =0;i<MAX_MPLIST_ITEM;i++){
        lv_label_set_text(mplist_item[i]," ");
        // lv_obj_clear_state(mplist_item[i],LV_STATE_CHECKED);
        lv_obj_clear_state(lv_obj_get_parent(lv_obj_get_parent(mplist_item[i])),LV_STATE_FOCUS_KEY);
        lv_obj_clear_state(lv_obj_get_parent(mplist_item[i]),LV_STATE_CHECKED);
        lv_label_set_long_mode(mplist_item[i], LV_LABEL_LONG_CLIP);
    }
    return 0;
}
int draw_musiclist(void)
{
    file_node_t *file_node = NULL;
    int foucus_idx,node_start;
    node_start = bs_listctrl.top;
    foucus_idx = bs_play_filelist.item_index-node_start;
    clear_musiclist();
    for(int i =0;i<MAX_MPLIST_ITEM;i++){
        file_node=file_mgr_get_file_node(&bs_play_filelist,node_start);
        if(file_node==NULL)
           break;
        lv_label_set_text_fmt(mplist_item[i],"%s",file_node->name);
        // //redraw checkbox 
        // char  path_fullname[1024];
        // file_mgr_get_fullname(path_fullname, bs_play_filelist.dir_path, file_node->name);
        // if(find_checkbox_form_glist(path_fullname,player_url_glist)==true){
        //     lv_obj_add_state(lv_obj_get_parent(mplist_item[i]),LV_STATE_CHECKED);
        //     file_node->user_data=1;
        // }
        // else{
        //     lv_obj_clear_state(lv_obj_get_parent(mplist_item[i]),LV_STATE_CHECKED);
        //     file_node->user_data=0;
        // }
        if(file_node->user_data==1){
            lv_obj_add_state(lv_obj_get_parent(mplist_item[i]),LV_STATE_CHECKED);
        }else{
            lv_obj_clear_state(lv_obj_get_parent(mplist_item[i]),LV_STATE_CHECKED);            
        }
        node_start++;
    }
    lv_obj_add_state(lv_obj_get_parent(lv_obj_get_parent(mplist_item[foucus_idx])),LV_STATE_FOCUS_KEY);
    // lv_obj_add_state(mplist_item[foucus_idx],LV_STATE_CHECKED);
    lv_label_set_long_mode(mplist_item[foucus_idx], LV_LABEL_LONG_SCROLL_CIRCULAR);
}

int bs_player_add_in_glist(file_list_t *m_cur_file_list,glist* player_glist)
{
    char *m_play_path_name=(char *)malloc(1024);    //need to free all glist_t
    // char m_play_path_name[1024];    //函数名是地址是常量
    file_node_t *file_node = NULL;
    file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
    file_mgr_get_fullname(m_play_path_name, m_cur_file_list->dir_path, file_node->name);
    player_glist=glist_append(player_glist,m_play_path_name);
    return 0;
}
//find same name in glist and del them
static int bs_player_delnode_form_glist(glist* player_glist,char * string_to_find)
{
    glist* temp_glist=player_glist->next;//第0 项无数据，从第1项开始
    glist* glist_del=NULL;
    while(temp_glist){
        if(temp_glist!=NULL&&temp_glist->data!=NULL){
            if(strcmp(string_to_find,temp_glist->data)==0){
                glist_del=temp_glist;
                break;
            }
            else{
                temp_glist=temp_glist->next;
            }
                
        }
    }
    if(glist_del){
        if(glist_del->data){
            free(glist_del->data);
            glist_del->data=NULL;//free data 
        }
        player_glist=glist_delete_link(player_glist,glist_del);
        glist_del=NULL;
    }
    return 0;
} 

int musiclist_enter(int index)
{
    bool is_root = false;
    uint16_t file_idx;   
    char *cur_file_name = NULL;
    file_node_t *file_node = NULL;
    cur_file_name = calloc(1, MAX_FILE_NAME + 1);
    char * m_cur_fullname=(char *)malloc(1024);
    if(!cur_file_name){
	    printf("Not enough memory.\n");
    }
    is_root = win_media_is_root_dir(&bs_play_filelist);
    

    //handle fslist index
    if (index == 0){

        if (is_root)
        {

        } else {
        // go to upper dir.
            char *parent_dir;
            parent_dir = calloc(1, MAX_FILE_NAME);
            if(!parent_dir){
                if(cur_file_name)
                {
                    free(cur_file_name);
                    cur_file_name=NULL;
                }
                    
            }

            strcpy(cur_file_name, bs_play_filelist.dir_path);
            win_get_parent_dirname(parent_dir, bs_play_filelist.dir_path);
            file_mgr_create_list(&bs_play_filelist, parent_dir);

            file_idx = win_get_file_idx_fullname(&bs_play_filelist, cur_file_name);
            //  printf("%s(), line:%d, upper dir:%s, index:%d\n", __FUNCTION__, __LINE__, 
            //     parent_dir, file_idx);        
            if (INVALID_VALUE_16 == file_idx)
                file_idx = 0;
            //keep the upper directory's foucs 
            bs_play_filelist.item_index = file_idx;
            osd_list_ctrl_reset(&bs_listctrl, 12, 
                bs_play_filelist.dir_count + bs_play_filelist.file_count, file_idx);
            draw_musiclist();
	        free(parent_dir);
        }
        free(cur_file_name);
        cur_file_name=NULL;

    }
    else
    {
        file_node = file_mgr_get_file_node(&bs_play_filelist, index);
        if (NULL == file_node){
            printf("file_mgr_get_file_node() fail!\n");
            if(cur_file_name!=NULL){
                free(cur_file_name);
                cur_file_name=NULL;
            }else if (m_cur_fullname!=NULL)
            {
                free(m_cur_fullname);
                m_cur_fullname=NULL;
            }
            return -1;
        }   
        file_mgr_get_fullname(m_cur_fullname, bs_play_filelist.dir_path, file_node->name);
        if (file_node->type == FILE_DIR){
            //enter next dir
            // printf("%s(), line:%d, enter next dir: %s\n", __FUNCTION__, __LINE__, m_cur_fullname);        
            file_mgr_create_list(&bs_play_filelist, m_cur_fullname);
            //draw lvgl obj
            bs_play_filelist.item_index = 0;
            osd_list_ctrl_reset(&bs_listctrl, 12, 
            bs_play_filelist.dir_count + bs_play_filelist.file_count, 0);
            //had to check file node if has checkbox 
            add_checkbox2_musiclist(&bs_play_filelist);
            draw_musiclist();
        }
        else {
            //for backstage player
            // for test 
            // backstage_player_task_start(0,NULL);
            // int obj_index=index-bs_listctrl.top;
            // if(file_node->user_data==0)
            // lv_state_t sel_file_sta=lv_obj_get_state(lv_obj_get_parent(mplist_item[index]));
            // if(sel_file_sta!=LV_STATE_CHECKED){
            //     lv_obj_add_state(lv_obj_get_parent(mplist_item[index]),LV_STATE_CHECKED);
            //     bs_player_add_in_glist(&bs_play_filelist,player_url_glist);//glist 如果传参的是空指针，无法创建glist
            //     file_node->user_data=1; //mean has checkbox state  
            // }
            // else{
            //     lv_obj_clear_state(lv_obj_get_parent(mplist_item[index]),LV_STATE_CHECKED);
            //     char * string_to_del=m_cur_fullname;
            //     //remove  list node 
            //     // bs_player_delnode_form_glist(player_url_glist,);
            //     bs_player_delnode_form_glist(player_url_glist,string_to_del);
            //     file_node->user_data=0; //mean has not checkbox state  
            // }
            int obj_index=index-bs_listctrl.top;//offset
            if(file_node->user_data==0)
            {
                lv_obj_add_state(lv_obj_get_parent(mplist_item[obj_index]),LV_STATE_CHECKED);
                bs_player_add_in_glist(&bs_play_filelist,player_url_glist);//glist 如果传参的是空指针，无法创建glist
                file_node->user_data=1; //mean has checkbox state  
            }
            else{
                lv_obj_clear_state(lv_obj_get_parent(mplist_item[obj_index]),LV_STATE_CHECKED);
                char * string_to_del=m_cur_fullname;
                bs_player_delnode_form_glist(player_url_glist,string_to_del);
                file_node->user_data=0; //mean has not checkbox state  
            }
        }
    }
    if (m_cur_fullname!=NULL)
    {
        free(m_cur_fullname);
        m_cur_fullname=NULL;
    }
    if(cur_file_name!=NULL)
    {
        free(cur_file_name);
        cur_file_name=NULL;
    }
       
    return 0;

}

int musiclist_win_add_data(void)
{
    // init once
    if(bs_play_filelist.list==NULL) {
        memset(&bs_play_filelist,0,sizeof(file_list_t));
        bs_play_filelist.media_type= MEDIA_TYPE_MUSIC;
        if ('\0' == bs_play_filelist.dir_path[0])
            strncpy(bs_play_filelist.dir_path, MOUNT_ROOT_DIR, MAX_FILE_NAME-1);
            // do something when insert sd& usb
        file_mgr_create_list(&bs_play_filelist,bs_play_filelist.dir_path);

    }
    if(player_url_glist==NULL)
    {
        player_url_glist=(glist*)malloc(sizeof(glist));
        memset(player_url_glist,0,sizeof(glist));
    }
    osd_list_ctrl_reset(&bs_listctrl, MAX_MPLIST_ITEM, 
    bs_play_filelist.dir_count + bs_play_filelist.file_count, bs_play_filelist.item_index); 
    draw_musiclist();
    return 0;
}
static int musiclist_ctrl_proc(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t pos)
{
    int i; 
    uint16_t count;
    int node_start;
    file_node_t *file_node;
    int focus_idx;
    uint16_t item_new_pos = 0;
    if (osd_list_ctrl_update2(list_ctrl, vkey, pos, &item_new_pos))
    {
        /* mean ctrl.top has change so had to redraw the lvgl info 
        */
        //redraw the list label according the new postion infromation
        count = list_ctrl->depth;
        node_start = list_ctrl->top;
        for (i = 0; i < count; i ++){
            printf("update: node_start:%d\n", node_start);
            if (node_start < list_ctrl->count){
                file_node = file_mgr_get_file_node(&bs_play_filelist, node_start);
                lv_label_set_text_fmt(mplist_item[i], "%s" ,file_node->name);
                if(file_node->user_data==1)//pending
                    lv_obj_add_state(lv_obj_get_parent(mplist_item[i]),LV_STATE_CHECKED);
                else if(file_node->user_data==0)
                    lv_obj_clear_state(lv_obj_get_parent(mplist_item[i]),LV_STATE_CHECKED);
            }else{
                lv_label_set_text(mplist_item[i], "");
                lv_obj_clear_state(lv_obj_get_parent(mplist_item[i]),LV_STATE_CHECKED);
            }
            node_start ++;
        }
    }
    //  or draw again
    // to handle focus_idx in list when key input 
    // lv_obj_clear_state(mplist_item[focus_idx],LV_STATE_CHECKED);
    for(int j=0;j<list_ctrl->depth;j++){
        // lv_obj_clear_state(mplist_item[j],LV_STATE_CHECKED);    //clear all state
        lv_obj_clear_state(lv_obj_get_parent(lv_obj_get_parent(mplist_item[j])),LV_STATE_FOCUS_KEY);    //clear all state
        lv_label_set_long_mode(mplist_item[j], LV_LABEL_LONG_CLIP);
    }
    focus_idx = item_new_pos - list_ctrl->top; 
    // lv_obj_add_state(mplist_item[focus_idx],LV_STATE_CHECKED);
    lv_obj_add_state(lv_obj_get_parent(lv_obj_get_parent(mplist_item[focus_idx])),LV_STATE_FOCUS_KEY);
    lv_label_set_long_mode(mplist_item[focus_idx], LV_LABEL_LONG_SCROLL_CIRCULAR);
    return item_new_pos; 
}

void musiclist_win_event_cb(lv_event_t * e )
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t * user_data = lv_event_get_user_data(e);
    // static int sel_id=0;
    // int sel_count = lv_obj_get_child_cnt(target);

    if(code ==LV_EVENT_KEY)
    {
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        int V_key=0;
        uint16_t new_item_idx = 0;
        uint16_t old_item_idx = bs_play_filelist.item_index;
        switch (key)
        {
            case LV_KEY_UP:
                V_key=V_KEY_UP;
                break;
            case LV_KEY_DOWN:
                V_key=V_KEY_DOWN;
                break;
            case LV_KEY_RIGHT:
                musiclist_enter(bs_play_filelist.item_index);
                break;
            case LV_KEY_LEFT:
                musiclist_enter(0);//index 0 mean back
                break;
            case LV_KEY_ENTER:
                musiclist_enter(bs_play_filelist.item_index);
                break;
            case LV_KEY_ESC:
            // if(lv_obj_is_valid(target))
            //     lv_obj_del(target);
                if(lv_obj_has_flag(target,LV_OBJ_FLAG_HIDDEN)==false)
                    lv_obj_add_flag(target,LV_OBJ_FLAG_HIDDEN);
                lv_group_focus_obj(user_data);
                break;
            default :
                break;
        }
        if(V_key==V_KEY_DOWN||V_key==V_KEY_UP){
            new_item_idx = musiclist_ctrl_proc(&bs_listctrl, V_key, old_item_idx);
            bs_play_filelist.item_index = new_item_idx;
            // printf("old_item_idx:%d, new_item_idx:%d, top:%d, cur_pos:%d, file_count:%d\n", 
            // old_item_idx, new_item_idx,bs_listctrl.top, bs_listctrl.cur_pos, bs_listctrl.count);
            // #ifdef  LVGL_RESOLUTION_240P_SUPPORT
            // lv_obj_scroll_to_view(lv_obj_get_child(target,bs_listctrl.cur_pos), LV_ANIM_OFF);
            //GDB bug here ,whit NULL obj -> scr,and had to adjust checkbox size in this list
            // #endif 
        }

    }

}


int create_musiclist_win(lv_obj_t* p,lv_obj_t* sub_btn)
{
    if(lv_obj_is_valid(musiclist_obj)){
        if(lv_obj_has_flag(musiclist_obj, LV_OBJ_FLAG_HIDDEN)){
            lv_obj_clear_flag(musiclist_obj, LV_OBJ_FLAG_HIDDEN);
            lv_group_focus_obj(musiclist_obj);
        }
    }else{
        create_musiclist_subwin(p,BSLIST_WIN_W_PCT,69,sub_btn);
        musiclist_win_add_data();
    }
    return 0;
}
void * get_bs_musiclist_t(void)
{
    return &bs_play_filelist;
} 
void * get_bs_osdlist_t(void)
{
    return &bs_listctrl;
} 
void* app_get_bsplayer_glist(void)
{
    return player_url_glist;
}

void* app_get_bsplayer_glist_addr(void)
{
    return &player_url_glist;
}
//for bs player stop bs music player and free glist$ file list 
int clear_all_bsplayer_mem(void)
{
    //for bs player stop bs music player and free glist$ file list 
    file_mgr_glist_free(app_get_bsplayer_glist());
    player_url_glist=NULL;
    file_mgr_free_list(get_bs_musiclist_t());
    printf("%s,%d\n",__func__,__LINE__);  
    return 0;     
}