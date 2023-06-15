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

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_fspage.h"
#include "mp_mainpage.h"

#include "com_api.h"
#include "screen.h"
#include "setup.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "mp_thumbnail.h"
#include "mp_preview.h"
#include "mp_bsplayer_list.h"
#include "mp_ebook.h"

file_list_t *m_cur_file_list=NULL;
file_list_t  m_file_list[MEDIA_TYPE_COUNT]={0}; 
obj_list_ctrl_t m_list_ctrl;
extern media_type_t media_type;

media_handle_t *m_cur_media_hld = NULL;

extern SCREEN_TYPE_E cur_scr;
extern SCREEN_SUBMP_E screen_submp;
int last_page=1; //start form 1st page 
int cur_page,page_num;

extern void media_player_close(void);
void *mp_get_cur_player(void)
{
    if(m_cur_media_hld)
        return m_cur_media_hld->player;
    else
        return NULL;
}
void *mp_get_cur_player_hdl(void)
{
    return m_cur_media_hld;
}
void* app_get_file_list()
{
    return m_cur_file_list;
}

void fs_page_keyinput_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    // lv_obj_t * parent_target=lv_event_get_target(event);
    int keypad_value,i;
    int v_key=0;
    uint16_t new_item_idx;
    uint16_t old_item_idx;
	file_node_t *file_node = NULL;
    if(code == LV_EVENT_KEY)
    {
        file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
        keypad_value = lv_indev_get_key(lv_indev_get_act());
        // printf("key code: %d\n", keypad_value);
        uint32_t conv_key = key_convert_vkey(keypad_value);
#ifndef LVGL_RESOLUTION_240P_SUPPORT
        /*for preview ,if preview init had to code here*/  
        // on air in small win  条件互斥
        if(lv_obj_is_valid(ui_win_zoom)){ 
            preview_key_ctrl(keypad_value);
        }else{
            if(preview_timer_handle!=NULL)
                lv_timer_reset(preview_timer_handle);
        }
        /*for preview */
#endif
        switch (keypad_value)
        {
            case LV_KEY_RIGHT :
                if(m_cur_file_list->item_index==m_list_ctrl.count-1)
                    break;
                else
                {
                    lv_group_focus_next(lv_group_get_default());
                    v_key=V_KEY_RIGHT;
                    break;
                }
            case LV_KEY_LEFT :
                if(m_cur_file_list->item_index==0)
                    break;
                else
                {
                    lv_group_focus_prev(lv_group_get_default());
                    v_key=V_KEY_LEFT;
                    break; 
                }
            case LV_KEY_UP :
                if(cur_page==1 &&m_list_ctrl.cur_pos<4)
                    break;    
                else{
                    for(i=0;i<4;i++)
                        lv_group_focus_prev(lv_group_get_default());
                    v_key=V_KEY_UP;
                    break;
                }
            case LV_KEY_DOWN:
                if(m_list_ctrl.cur_pos>(m_list_ctrl.count-(m_list_ctrl.count%4)))
                    break;
                else if(m_list_ctrl.cur_pos+4>m_list_ctrl.count-1)
                    break;
                else
                {
                    for(i=0;i<4;i++)
                        lv_group_focus_next(lv_group_get_default());
                    v_key=V_KEY_DOWN;
                    break;
                }
            case LV_KEY_ENTER:
				if(MEDIA_TYPE_TXT == m_cur_file_list->media_type && FILE_TXT == file_node->type)
				{
					printf("%s,%d\n",__func__,__LINE__);
					_ui_screen_change(ui_ebook_txt,0,0); 
					printf("%s,%d\n",__func__,__LINE__);
				}
				else
                	media_fslist_enter(m_cur_file_list->item_index);
                break;
            case LV_KEY_ESC : //back btn value in lvgl mmap
                if(lv_obj_is_valid(ui_win_zoom)){ 
                    preview_reset();
                }else{
                    // _ui_screen_change(ui_subpage, 0, 0);
                    media_fslist_enter(0);//0 mean back to upper
                }
                break;
        }
        //when left or right key updata the cursor and list item  
        if((keypad_value==LV_KEY_RIGHT||keypad_value==LV_KEY_LEFT||keypad_value==LV_KEY_DOWN||keypad_value==LV_KEY_UP)&&v_key!=0)
        {
            old_item_idx = m_cur_file_list->item_index;
            new_item_idx = fslist_ctl_proc(&m_list_ctrl, v_key, old_item_idx);
            m_cur_file_list->item_index = new_item_idx;
            printf("cursor on :%d\r\n",new_item_idx);
        }
        else if(conv_key==V_KEY_YELLOW)
        {
            static bool test_flag = false;
            //test for get thumpnail
            if(test_flag==false){
                thumbnail_pthread_start();
                test_flag=true;
            }else{
                thumbnail_pthread_stop();
                test_flag=false;
            }
        }
    }
}


void clear_fslist_obj(void)
{   
    int i ; 
    for(i=0;i<12;i++)
    {
        if(obj_labelitem[i]!= NULL)
        {
            lv_label_set_text(obj_labelitem[i],"");
            //need to set img with balck bg 
            //recreate obj will set img scr null 
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&black_bg);
        }   
    }
    lv_group_remove_all_objs(lv_group_get_default());

}   

/*fspage handle ,add lab & img in fs obj  */
void draw_media_fslist(lv_obj_t * parent,char * dir_path)
{
    //get_fsnode_name(dir_path);  //free memory  to do 
    int i;
    glist *list_it;
    file_node_t *file_node;
    int foucus_idx,count,node_start;
    node_start = m_list_ctrl.top;
    count = m_list_ctrl.depth;
    list_it = (glist*)m_cur_file_list->list;
    foucus_idx = m_cur_file_list->item_index-node_start;
    m_cur_file_list->media_type=media_type;
    //clear all lab & image
    clear_fslist_obj();
    //this loop for add lab and img
    for(i=0;i<count;i++)
    {
        if (NULL == list_it)
            break;
        if(node_start==m_list_ctrl.count)
            break;
        //add img 
        file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
        if (FILE_DIR == file_node->type){
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&cultraview_folder);
        } else if (FILE_VIDEO == file_node->type){
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_movie);
        } else if (FILE_MUSIC == file_node->type){
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_music);
        } else if(FILE_IMAGE == file_node->type){
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_photo);
        }
        else {
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_text);
        }
        //add lab  
        if(file_node==NULL) 
            break;
        lv_label_set_text_fmt(obj_labelitem[i],"%s",file_node->name);
        // printf("%s\n",file_node->name);
        lv_group_add_obj(lv_group_get_default(),obj_item[i]);
        node_start++;
        list_it = list_it->next;
    }
    //handle ui foucus_idx
    lv_group_focus_obj(obj_item[foucus_idx]);
    label_set_long_mode_with_state(foucus_idx);
    //updata title

    if((m_cur_file_list->file_count+m_cur_file_list->dir_count)%12==0)
        page_num=(m_cur_file_list->file_count+m_cur_file_list->dir_count)/12; 
    else 
        page_num=(m_cur_file_list->file_count+m_cur_file_list->dir_count)/12 +1;

    if((m_cur_file_list->item_index+1)%12==0)//item_index from 0 start,so offset 1 
        last_page=cur_page= (m_cur_file_list->item_index+1)/12;
    else 
        last_page=cur_page= (m_cur_file_list->item_index+1)/12+1;

    //handle the first icon
    if(cur_page==1)
    {
        lv_img_set_src(lv_obj_get_child(obj_item[0],0),&Thumbnail_Upfolder);
        // lv_label_set_text_fmt(obj_labelitem[0],"%s","Back");
        set_label_text2(obj_labelitem[0],STR_BACK,FONT_MID);
    }
    lv_label_set_text_fmt(lv_obj_get_child(lv_obj_get_child(ui_fspage,1),1),"%d / %d",cur_page,page_num);
    lv_label_set_text_fmt(lv_obj_get_child(lv_obj_get_child(ui_fspage,1),0),"%s",m_cur_file_list->dir_path);    
}

int media_fslist_enter(int index)
{
    bool is_root = false;
    uint16_t file_idx;   
    char *cur_file_name = NULL;
    file_node_t *file_node = NULL;
    cur_file_name = calloc(1, MAX_FILE_NAME + 1);
    char *m_cur_fullname=(char *)malloc(1024);
    if(!cur_file_name){
	    printf("Not enough memory.\n");
    }
    is_root = win_media_is_user_rootdir(m_cur_file_list);
    

    //handle fslist index
    if (index == 0){

        if (is_root)
        {
            _ui_screen_change(ui_subpage, 0, 0);
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

            strcpy(cur_file_name, m_cur_file_list->dir_path);
            win_get_parent_dirname(parent_dir, m_cur_file_list->dir_path);
            file_mgr_create_list(m_cur_file_list, parent_dir);

            file_idx = win_get_file_idx_fullname(m_cur_file_list, cur_file_name);
            //  printf("%s(), line:%d, upper dir:%s, index:%d\n", __FUNCTION__, __LINE__, 
            //     parent_dir, file_idx);        
            if (INVALID_VALUE_16 == file_idx)
                file_idx = 0;
            //keep the upper directory's foucs 
            m_cur_file_list->item_index = file_idx;
            osd_list_ctrl_reset(&m_list_ctrl, 12, 
                m_cur_file_list->dir_count + m_cur_file_list->file_count, file_idx);
            m_list_ctrl.top =12*(m_list_ctrl.cur_pos/12);
            draw_media_fslist(ui_fspage,parent_dir);
	        free(parent_dir);
        }
        free(cur_file_name);
        cur_file_name=NULL;

    }
    else
    {
        file_node = file_mgr_get_file_node(m_cur_file_list, index);
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
        file_mgr_get_fullname(m_cur_fullname, m_cur_file_list->dir_path, file_node->name);
        if (file_node->type == FILE_DIR){
            //enter next dir
            // printf("%s(), line:%d, enter next dir: %s\n", __FUNCTION__, __LINE__, m_cur_fullname);        
            file_mgr_create_list(m_cur_file_list, m_cur_fullname);
            //draw lvgl obj
            m_cur_file_list->item_index = 0;
            osd_list_ctrl_reset(&m_list_ctrl, 12, 
            m_cur_file_list->dir_count + m_cur_file_list->file_count, 0);
            draw_media_fslist(ui_fspage,file_node->name);
        }
        else {
            //play media file
            // lv_obj_add_flag(ui_fspage,LV_OBJ_FLAG_HIDDEN);
            _ui_screen_change(ui_ctrl_bar,0,0);
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

void media_fslist_open(void)
{
    fs_group= lv_group_create();
    set_key_group(fs_group);
    fs_group->auto_focus_dis=1;
    //create fspage ui 
    create_fspage_scr(); 
    //init m_cur_file_list
    static media_type_t last_media_type=MEDIA_TYPE_COUNT;
    // if(last_media_type!=media_type||m_cur_file_list->list==NULL)//need to do 
    // {
    //     m_cur_file_list = &m_file_list[media_type]; 
    //     if ('\0' == m_cur_file_list->dir_path[0])
    //         strncpy(m_cur_file_list->dir_path, MOUNT_ROOT_DIR, MAX_FILE_NAME-1);
    //     file_mgr_create_list(m_cur_file_list, m_cur_file_list->dir_path);
    //     last_media_type=media_type;
    // }
    //hotplug need to free media_list 
    if(last_media_type!=media_type||m_cur_file_list->list==NULL)
    {
        m_cur_file_list = &m_file_list[media_type];
        m_cur_file_list->media_type=media_type;
        partition_info_t * m_cur_partinfo=mmp_get_partition_info();
        char root_path[MAX_FILE_NAME]={0};
    #ifdef __HCRTOS__    
        file_mgr_get_fullname(root_path,MOUNT_ROOT_DIR,m_cur_partinfo->used_dev);
    #else
        strcpy(root_path, m_cur_partinfo->used_dev);
    #endif
        if ('\0' == m_cur_file_list->dir_path[0])
            strncpy(m_cur_file_list->dir_path,root_path, MAX_FILE_NAME-1);
        file_mgr_create_list(m_cur_file_list, m_cur_file_list->dir_path);
        last_media_type=media_type;
    }

    osd_list_ctrl_reset(&m_list_ctrl, MAX_FILELIST_ITEMS, m_cur_file_list->dir_count + m_cur_file_list->file_count, m_cur_file_list->item_index); 
    osd_list_update_top(&m_list_ctrl);  //list->top diff in this case so handle here
    draw_media_fslist(ui_fspage, MOUNT_ROOT_DIR);  
    screen_submp=SCREEN_SUBMP2;
    printf("%s %d\n",__FUNCTION__,__LINE__);
#ifndef LVGL_RESOLUTION_240P_SUPPORT
    if(m_cur_file_list->media_type!=MEDIA_TYPE_TXT)
        preview_init();
#endif
    api_ffmpeg_player_get_regist(mp_get_cur_player);

}

void media_fslist_close(void)
{
    int i=0;
    lv_group_remove_all_objs(fs_group);
    lv_group_del(fs_group);
    //for preview
    preview_deinit();
    clear_fapage_scr();
}

uint16_t fslist_ctl_proc(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t pos)
{
    int i; 
    uint16_t count;
    int node_start;
    uint16_t item_new_pos = 0;
    count = list_ctrl->depth;
    file_node_t*  file_node;
    int focus_idx=0;
    //update the list ctrl postion information
    osd_list_ctrl_update(list_ctrl, vkey, pos, &item_new_pos);
    //updata pagenum 
    if((item_new_pos+1)% 12==0) //item offset 1
        cur_page=(item_new_pos+1)/12;
    else 
        cur_page=(item_new_pos+1)/12+1;


    lv_label_set_text_fmt(lv_obj_get_child(lv_obj_get_child(ui_fspage,1),1),"%d / %d",cur_page,page_num);
    //draw page num
    node_start=12*(cur_page-1);
    list_ctrl->top=12*(cur_page-1);
    printf("updata list top : %d\r\n", list_ctrl->top);
    //turn  page operation last page != cur page 
    //last page =cur page 
    if (last_page!=cur_page){ 
        lv_group_remove_all_objs(lv_group_get_default());
        //redraw the list label according the new postion infromation
        for (i = 0; i < count; i ++){
            // file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
            if(node_start < list_ctrl->count)
            {
                //add image 
                file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
                if (FILE_DIR == file_node->type){
                    lv_img_set_src(lv_obj_get_child(obj_item[i],0),&cultraview_folder);     
                } else if (FILE_VIDEO == file_node->type){
                    lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_movie);
                } else if (FILE_MUSIC == file_node->type){
                    lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_music);
                } else if(FILE_IMAGE == file_node->type){
                    lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_photo);
                }
                else {
                    lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_text);
                }
                lv_label_set_text_fmt(obj_labelitem[i],"%s",file_node->name);
                //group
                lv_group_add_obj(lv_group_get_default(),obj_item[i]);
            }
            else
            {
                lv_img_set_src(lv_obj_get_child(obj_item[i],0),&black_bg);
                lv_label_set_text(obj_labelitem[i],"");
            }   
            node_start ++; 
        }
        if(cur_page==1)
        {
            lv_img_set_src(lv_obj_get_child(obj_item[0],0),&Thumbnail_Upfolder);
            set_label_text2(obj_labelitem[0],STR_BACK,FONT_MID);
        }
        last_page=cur_page;
    }
    //handle ui foucus 
    focus_idx = item_new_pos - list_ctrl->top;
    lv_group_focus_obj(obj_item[focus_idx]);
    // printf("focus_idx : %d\r\n", focus_idx);
    label_set_long_mode_with_state(focus_idx);
    return item_new_pos; 
} 


// top should ctrl 
void osd_list_update_top(obj_list_ctrl_t *list_ctrl)
{
    int lastwin_page_count=0;
    if(list_ctrl->count>list_ctrl->depth)
    {
        lastwin_page_count=list_ctrl->cur_pos/12+1;
    }
    else if(list_ctrl->count<=list_ctrl->depth)
    {
        lastwin_page_count=1;
    }
    list_ctrl->top=12*(lastwin_page_count-1);
    printf("set top: %d\n",list_ctrl->top);
}
void clear_fslist_path(void *path)
{
    memset(path,0,1024);
}

int label_set_long_mode_with_state(int foucus_idx)
{
    //press set style scroll+hignlight
    for(int i=0;i<12;i++)
    {
        if(i==foucus_idx)
        {
            lv_label_set_long_mode(obj_labelitem[i],LV_LABEL_LONG_SCROLL_CIRCULAR);
            lv_obj_set_style_text_color(obj_labelitem[i],lv_color_hex(0xFFFF00),0);
        }
        else
        {
            lv_label_set_long_mode(obj_labelitem[i],LV_LABEL_LONG_DOT);
            lv_obj_set_style_text_color(obj_labelitem[i],lv_color_hex(0xFFFFFF),0);
        } 

    }
    return 0;
}
/////////////////////////////////////////////////////////

int app_media_list_all_free()
{
    int media_type_cnt=4;
    for(int i=0;i<media_type_cnt;i++){
        file_list_t * single_fslist=&m_file_list[i];
        file_mgr_free_list(single_fslist);
    }
#ifdef RTOS_SUBTITLE_SUPPORT
    file_mgr_subtitle_list_free();
#endif
    return 0;
}

