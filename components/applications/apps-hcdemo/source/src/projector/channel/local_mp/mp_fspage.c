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
#include "mp_fspage.h"
#include "mp_mainpage.h"


#include "../../screen.h"
#include "../../setup/setup.h"
#include "../../factory_setting.h"
#define PREVIWE_W  640
#define PREVIWE_H  360
#define PREVIWE_X  640
#define PREVIWE_Y  360
#define DIS_SOURCE_W  1920
#define DIS_SOURCE_H  1080
#define DIS_SOURCE_X  0
#define DIS_SOURCE_Y  0

extern file_list_t *m_cur_file_list;
extern file_list_t  m_file_list[MEDIA_TYPE_COUNT]; 
extern obj_list_ctrl_t m_list_ctrl;
extern media_type_t media_type;

char * fslist_name[100];
char * m_cur_fullname= NULL;
media_handle_t *m_cur_media_hld = NULL;
// exit_code_t fsobj0_exit_code;
extern lv_obj_t * ui_ebook_txt;
static lv_obj_t * fs_lab[12]={NULL};
lv_obj_t * fs_obj[12];
lv_obj_t * fs_img[12];

extern int usb_state;
extern SCREEN_TYPE_E cur_scr;
int last_page,cur_page,page_num;
static lv_timer_t * previwe_timer_handle=NULL;
void *mp_get_cur_player_hdl(void)
{
    return m_cur_media_hld;
}
void * mp_get_cur_fullpath(void)
{
    return m_cur_fullname;
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
	file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
    printf("%s %d\n",__FUNCTION__,__LINE__);
    if(code == LV_EVENT_KEY)
    {
        keypad_value = lv_indev_get_key(lv_indev_get_act());
        printf("key code: %d\n", keypad_value);
        switch (keypad_value)
        {
            case LV_KEY_RIGHT :
                if(m_cur_file_list->item_index==m_list_ctrl.count-1)
                    break;
                else
                {
                    lv_group_focus_next(fs_group);
                    v_key=V_KEY_RIGHT;
                    break;
                }
            case LV_KEY_LEFT :
                if(m_cur_file_list->item_index==0)
                    break;
                else
                {
                    lv_group_focus_prev(fs_group);
                    v_key=V_KEY_LEFT;
                    break; 
                }
            case LV_KEY_UP :
                if(cur_page==1 &&m_list_ctrl.cur_pos<4)
                    break;    
                else{
                    for(i=0;i<4;i++)
                        lv_group_focus_prev(fs_group);
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
                        lv_group_focus_next(fs_group);
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
                _ui_screen_change(ui_subpage, 0, 0);
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
        //for previwe& 如果有窗口就要clear obj，没有timer就去clear会error,怎么判断判断页面是否有open pre，这个判断可以改动
        //或判断obj是否有child，就知道是否需要关闭
        if(NULL!=ui_win_zoom)
        {
            printf("open->close %s %d\n",__FUNCTION__,__LINE__);
            previwe_close();
        }
        printf("%s %d\n",__FUNCTION__,__LINE__);
    }

}

//for media list 
//get fslist all name in num array 
void get_fsnode_name(char * path)
{
    file_node_t * file_node;
	// file_node_t * first_file_node;
    // file_node_t * second_file_node;
    m_cur_file_list = &m_file_list[media_type]; //
    if ('\0' == m_cur_file_list->dir_path[0])
        strncpy(m_cur_file_list->dir_path,path, MAX_FILE_NAME-1);
    file_mgr_create_list(m_cur_file_list, m_cur_file_list->dir_path);
    int i;
    for(i=0;i<m_cur_file_list->dir_count+m_cur_file_list->file_count;i++){
		file_node= file_mgr_get_file_node(m_cur_file_list,i);
        // printf("%s \r\n",file_node->name);
        fslist_name[i]=(char *)malloc(50);
        sprintf(fslist_name[i],"%s",file_node->name);
        printf("%s\r\n",fslist_name[i]);
    }
    printf("%s %d\n",__FUNCTION__,__LINE__);
  
}
void clear_fslist_obj(void)
{   
    int i ; 
    for(i=0;i<12;i++)
    {
        if(fs_lab[i]!= NULL)
        {
            lv_label_set_text(fs_lab[i],"");
            //need to set img with balck bg 
            //recreate obj will set img scr null 
            lv_img_set_src(fs_img[i],&black_bg);
        }   
    }
    lv_group_remove_all_objs(fs_group);

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
            lv_img_set_src(fs_img[i],&ui_img_folder_png);
        } else if (FILE_VIDEO == file_node->type){
            lv_img_set_src(fs_img[i],&IDB_FileSelect_movie);
        } else if (FILE_MUSIC == file_node->type){
            lv_img_set_src(fs_img[i],&IDB_FileSelect_music);
        } else if(FILE_IMAGE == file_node->type){
            lv_img_set_src(fs_img[i],&IDB_FileSelect_photo);
        }
        else {
            lv_img_set_src(fs_img[i],&IDB_FileSelect_text);
        }
        //add lab  
        if(file_node==NULL) 
            break;
        lv_label_set_text_fmt(fs_lab[i],"%s",file_node->name);
        // printf("%s\n",file_node->name);
        lv_group_add_obj(fs_group,fs_obj[i]);
        node_start++;
        list_it = list_it->next;
    }
    //handle ui foucus_idx
    lv_group_focus_obj(fs_obj[foucus_idx]);
    label_set_long_mode_with_state(foucus_idx);
    //updata title  
    if((m_cur_file_list->file_count+m_cur_file_list->dir_count)>= 12)
    {
        page_num=(m_cur_file_list->file_count+m_cur_file_list->dir_count)/12 +1;
        cur_page= m_cur_file_list->item_index/12 +1;
    }  
    else if((m_cur_file_list->file_count+m_cur_file_list->dir_count)<12)
    {
        page_num=1;
        cur_page=1;
    }
    //handle the first icon
    if(cur_page==1)
    {
        lv_img_set_src(fs_img[0],&ui_img_thumbnail_upfolder_png);
        lv_label_set_text_fmt(fs_lab[0],"%s","Back");
    }
    lv_label_set_text_fmt(ui_fscount,"%d / %d",cur_page,page_num);
    // language_choose_add_label(ui_fspath,m_cur_file_list->dir_path,0);
    if(m_cur_file_list->media_type==MEDIA_TYPE_VIDEO)
        language_choose_add_label(ui_fstitle,movie_k,0);
    else if(m_cur_file_list->media_type==MEDIA_TYPE_MUSIC)
        language_choose_add_label(ui_fstitle,music_k,0);
    else if (m_cur_file_list->media_type==MEDIA_TYPE_PHOTO)
        language_choose_add_label(ui_fstitle,photo_k,0);
	else if (m_cur_file_list->media_type==MEDIA_TYPE_TXT)
        language_choose_add_label(ui_fstitle,text_k,0);
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(ui_fstitle, id,0,&select_font_mplist[id]);

    lv_label_set_text_fmt(ui_fspath,"%s",m_cur_file_list->dir_path);
    set_label_text_with_font(ui_fspath, id,0,&select_font_mplist[id]);
}

int media_fslist_enter(int index)
{
    bool is_root = false;
    uint16_t file_idx;   
    char *cur_file_name = NULL;
    file_node_t *file_node = NULL;
    cur_file_name = calloc(1, MAX_FILE_NAME + 1);
    m_cur_fullname=(char *)malloc(1024);
    if(!cur_file_name){
	    printf("Not enough memory.\n");
    }
    is_root = win_media_is_root_dir(m_cur_file_list);
    

    //handle fslist index
    if (index == 0){

        if (is_root)
        {
            // exit the menu list window
            // fsobj0_exit_code=exit_page;
            _ui_screen_change(ui_subpage, 0, 0);
        } else {
        // go to upper dir.
            char *parent_dir;
            parent_dir = calloc(1, MAX_FILE_NAME);
            if(!parent_dir){
                if(cur_file_name)
                    free(cur_file_name);
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

    }
    else
    {
        file_node = file_mgr_get_file_node(m_cur_file_list, index);
        if (NULL == file_node){
            printf("file_mgr_get_file_node() fail!\n");
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
    // free(m_cur_fullname);
    return 0;
    

}

void media_fslist_open(void)
{
    int i,id ;
    lv_obj_t * temp_img[12]={ui_fsimg0,ui_fsimg1,ui_fsimg2,ui_fsimg3,ui_fsimg4,ui_fsimg5,
    ui_fsimg6,ui_fsimg7,ui_fsimg8,ui_fsimg9,ui_fsimg10,ui_fsimg11};
    lv_obj_t * temp_obj[12]={ui_fsobj0,ui_fsobj1,ui_fsobj2,ui_fsobj3,ui_fsobj4,ui_fsobj5,
    ui_fsobj6,ui_fsobj7,ui_fsobj8,ui_fsobj9,ui_fsobj10,ui_fsobj11};
    for(i=0;i< 12;i++)
    {
        fs_img[i]=temp_img[i];
        fs_obj[i]=temp_obj[i];
        lv_obj_add_event_cb(fs_obj[i],fs_page_keyinput_event_cb, LV_EVENT_KEY, NULL);
        fs_lab[i]=lv_label_create(ui_fspage);
        lv_label_set_text(fs_lab[i],"");
        lv_obj_set_style_text_color(fs_lab[i],lv_color_hex(0xffffff),0);
        // lv_obj_set_style_text_font(fs_lab[i],&select_font_mplist[1],0);
        id = projector_get_some_sys_param(P_OSD_LANGUAGE);
        set_label_text_with_font(fs_lab[i], id,0,&select_font_mplist[id]);
        lv_obj_set_style_text_align(fs_lab[i], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_width(fs_lab[i], 150);
        lv_obj_set_height(fs_lab[i], 35);       
        lv_label_set_long_mode(fs_lab[i], LV_LABEL_LONG_DOT);     
        lv_obj_align_to(fs_lab[i],fs_obj[i],LV_ALIGN_OUT_BOTTOM_MID,0,8);
    }
    // set_label_text_with_font(ui_fstitle, id,0,&select_font_mplist[id]);


    fs_group= lv_group_create();
    set_key_group(fs_group);
    // get_fsnode_name("/media"); 
    //init m_cur_file_list
    m_cur_file_list = &m_file_list[media_type]; 
    if ('\0' == m_cur_file_list->dir_path[0])
        strncpy(m_cur_file_list->dir_path,"/media", MAX_FILE_NAME-1);
    file_mgr_create_list(m_cur_file_list, m_cur_file_list->dir_path);

    osd_list_ctrl_reset(&m_list_ctrl, 12, m_cur_file_list->dir_count + m_cur_file_list->file_count, m_cur_file_list->item_index); 
    osd_list_update_top(&m_list_ctrl);  //list->top diff in this case so handle here
    draw_media_fslist(ui_fspage,"/media");  
    printf("%s %d\n",__FUNCTION__,__LINE__);
    //create tim 只能创建一个定时器，
    // if(!previwe_timer_handle)
    //     previwe_timer_handle=lv_timer_create(previwe_open_timer_cb,5000,NULL);
}

void media_fslist_close(void)
{
    int i=0;
    lv_group_remove_all_objs(fs_group);
    lv_group_del(fs_group);
    for(i=0;i< 12;i++)
    {
        lv_obj_remove_event_cb(fs_obj[i],fs_page_keyinput_event_cb);
        lv_obj_del(fs_lab[i]); 
        fs_lab[i]=NULL;
    }
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
    if((list_ctrl->count)>12)
    {
        cur_page=item_new_pos/12 +1;
    }
    else if((list_ctrl->count)<=12)
    {
        last_page=cur_page=1;   //handle last page& cur page
    }
    lv_label_set_text_fmt(ui_fscount,"%d / %d",cur_page,page_num);
    //draw page num
    node_start=12*(cur_page-1);
    list_ctrl->top=12*(cur_page-1);
    printf("updata list top : %d\r\n", list_ctrl->top);
    //turn  page operation last page != cur page 
    //last page =cur page 
    if (last_page!=cur_page){ 
        lv_group_remove_all_objs(fs_group);
        //redraw the list label according the new postion infromation
        for (i = 0; i < count; i ++){
            // file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
            if(node_start < list_ctrl->count)
            {
                //add image 
                file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
                if (FILE_DIR == file_node->type){
                    lv_img_set_src(fs_img[i],&ui_img_folder_png);     
                } else if (FILE_VIDEO == file_node->type){
                    lv_img_set_src(fs_img[i],&IDB_FileSelect_movie);
                } else if (FILE_MUSIC == file_node->type){
                    lv_img_set_src(fs_img[i],&IDB_FileSelect_music);
                } else if(FILE_IMAGE == file_node->type){
                    lv_img_set_src(fs_img[i],&IDB_FileSelect_photo);
                }
                else {
                    lv_img_set_src(fs_img[i],&IDB_FileSelect_text);
                }
                lv_label_set_text_fmt(fs_lab[i],"%s",file_node->name);
                //group
                lv_group_add_obj(fs_group,fs_obj[i]);
            }
            else
            {
                lv_img_set_src(fs_img[i],&black_bg);
                lv_label_set_text(fs_lab[i],"");
            }   
            node_start ++; 
        }
        if(cur_page==1)
        {
            lv_img_set_src(fs_img[0],&ui_img_thumbnail_upfolder_png);
            lv_label_set_text(fs_lab[0],"Back");
        }
        last_page=cur_page;
    }
    //handle ui foucus 
    focus_idx = item_new_pos - list_ctrl->top;
    lv_group_focus_obj(fs_obj[focus_idx]);
    // printf("focus_idx : %d\r\n", focus_idx);
    label_set_long_mode_with_state(focus_idx);
    return item_new_pos; 
} 

void check_usb_hotplug() //do it in while(1)
{
    lv_disp_t * dispp = lv_disp_get_default();
    usb_state=mmp_get_usb_stat();
    if(usb_state!=USB_STAT_MOUNT)
    {
        if(dispp->act_scr==ui_fspage||dispp->act_scr==ui_ctrl_bar||dispp->act_scr==ui_subpage)
        {
            _ui_screen_change(ui_mainpage,0, 0);
            cur_scr=SCREEN_CHANNEL_MP;
        }
        else if(dispp->act_scr==ui_ebook_txt)
        {
            ebook_free_buff();
            _ui_screen_change(ui_mainpage,0, 0);
            cur_scr=SCREEN_CHANNEL_MP;

        }
        else if((dispp->act_scr==ui_mainpage)&&m_cur_file_list->dir_path!=NULL)
        {
            clear_fslist_path(m_cur_file_list->dir_path);
        }

    }
    if(dispp->act_scr==ui_mainpage)
    {
        //send event &usb state here
        lv_event_send(ui_usb_img,LV_EVENT_REFRESH,0);
    }
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
    //按下后需要设置样式为滚动+hignlight
    for(int i=0;i<12;i++)
    {
        if(i==foucus_idx)
        {
            lv_label_set_long_mode(fs_lab[i],LV_LABEL_LONG_SCROLL_CIRCULAR);
            lv_obj_set_style_text_color(fs_lab[i],lv_color_hex(0xFFFF00),0);
        }
        else
        {
            lv_label_set_long_mode(fs_lab[i],LV_LABEL_LONG_DOT);
            lv_obj_set_style_text_color(fs_lab[i],lv_color_hex(0xFFFFFF),0);
        } 

    }
    return 0;
}
/////////////////////////////////////////////////////////
// timer to previwe or msg to previwe 
#if 1
//open
void previwe_open_timer_cb(lv_timer_t * t)
{
    //timer pause 
    lv_timer_pause(previwe_timer_handle);
    //create ui 
    win_previwe_create(ui_fspage);
    //media play
    // preview_mp_play(m_cur_file_list->item_index);
    // //set zoom 
    // set_zoom(DIS_SOURCE_W,DIS_SOURCE_H,0,0,PREVIWE_X,PREVIWE_Y,PREVIWE_W,PREVIWE_H);
    // //调整图层（osd 在底层,主图层在顶层）
    // ioctl(de);
    
}
float format_filesize(uint64_t size_Byte)
{
    float size_Mb=size_Byte/1024/1024;
    return size_Mb;
}
int previwe_close(void)
{
    //clear ui 
    win_previwe_clear();
    // win_previwe_clear(ui_win_name);
    // win_previwe_clear(ui_file_info);
    //meida_stop
    //de ctrl
    //set zoom 
    //timer reset &恢复
    lv_timer_resume(previwe_timer_handle);
    lv_timer_reset(previwe_timer_handle);
    return 0;
}
void win_previwe_create(lv_obj_t* parent)
{
    // win_previwe_clear(ui_win_zoom);
    // win_previwe_clear(ui_win_name);
    // win_previwe_clear(ui_file_info);
#if 1   //ui draw
    ui_win_zoom = lv_obj_create(parent);
    lv_obj_set_width(ui_win_zoom, 425);
    lv_obj_set_height(ui_win_zoom, 240);
    lv_obj_set_align(ui_win_zoom, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_win_zoom, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_win_zoom, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_win_zoom, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_win_zoom, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_win_zoom, lv_color_hex(0x0478F7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_win_zoom, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_win_zoom, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_win_zoom, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_win_name = lv_obj_create(parent);
    lv_obj_set_width(ui_win_name, 425);
    lv_obj_set_height(ui_win_name, 50);
    lv_obj_set_x(ui_win_name, 0);
    lv_obj_set_y(ui_win_name, 152);
    lv_obj_set_align(ui_win_name, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_win_name, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_win_name, lv_color_hex(0xAD31F9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_win_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_win_name, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_win_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_name = lv_label_create(ui_win_name);
    lv_obj_set_width(ui_name, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_name, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_name, LV_ALIGN_CENTER);
    lv_label_set_text(ui_name, "for Previwe to do");
    lv_obj_set_style_text_font(ui_name, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_file_info = lv_obj_create(parent);
    lv_obj_set_width(ui_file_info, 267);
    lv_obj_set_height(ui_file_info, 297);
    lv_obj_set_x(ui_file_info, 354);
    lv_obj_set_y(ui_file_info, 27);
    lv_obj_set_align(ui_file_info, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_file_info, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_file_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_file_info, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_file_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_file_info, lv_color_hex(0x0478F7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_file_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_file_info, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_file_info, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_file_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_file_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_x(ui_file_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_y(ui_file_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_file_info, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_file_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_file_info, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_info_s = lv_label_create(ui_file_info);
    lv_obj_set_width(ui_info_s, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_info_s, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_info_s, -71);
    lv_obj_set_y(ui_info_s, -104);
    lv_obj_set_align(ui_info_s, LV_ALIGN_CENTER);
    lv_label_set_text(ui_info_s, "W&H:");

    ui_info_s1 = lv_label_create(ui_file_info);
    lv_obj_set_width(ui_info_s1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_info_s1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_info_s1, -75);
    lv_obj_set_y(ui_info_s1, -53);
    lv_obj_set_align(ui_info_s1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_info_s1, "Size:");

    ui_info_t_ = lv_label_create(ui_file_info);
    lv_obj_set_width(ui_info_t_, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_info_t_, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_info_t_, 50);
    lv_obj_set_y(ui_info_t_, -104);
    lv_obj_set_align(ui_info_t_, LV_ALIGN_CENTER);
    lv_label_set_text(ui_info_t_, "1024");

    ui_info_t1 = lv_label_create(ui_file_info);
    lv_obj_set_width(ui_info_t1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_info_t1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_info_t1, 50);
    lv_obj_set_y(ui_info_t1, -53);
    lv_obj_set_align(ui_info_t1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_info_t1, "512");
    lv_obj_set_style_text_align(ui_info_t1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
#endif
}
void win_previwe_clear(void)
{
//传参调用有问题
# if 0
    if(parent)
    {
        lv_obj_del(parent); 
        parent = NULL;
        printf("%p\n",parent);
        printf("%s ,%d\n",__FUNCTION__, __LINE__);
    }
#endif
    lv_obj_del(ui_win_zoom);
    lv_obj_del(ui_file_info);
    lv_obj_del(ui_win_name);
    ui_win_zoom=NULL;
    ui_file_info= NULL;
    ui_win_name=NULL;
    printf("%s ,%d\n",__FUNCTION__, __LINE__);

}

#endif 