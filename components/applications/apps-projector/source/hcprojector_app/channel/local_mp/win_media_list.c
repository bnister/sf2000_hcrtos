/**
 * win_media_list.c, use to list media files(USB).
 */
#include <stdio.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "lvgl/src/font/lv_font.h"
#include "obj_mgr.h"
#include "com_api.h"
#include "menu_mgr.h"
#include "osd_com.h"
#include "key_event.h"
#include "media_player.h"
#include "file_mgr.h"
#include "glist.h"
#include "win_media_list.h"

static char m_cur_file_name[MAX_FILE_NAME+1] = {0};

bool win_media_is_root_dir(file_list_t *file_list)
{
    if (strcmp(file_list->dir_path, MOUNT_ROOT_DIR))
        return false;
    else
        return true;
}
bool win_media_is_user_rootdir(file_list_t *file_list)
{
    partition_info_t * m_cur_partinfo=mmp_get_partition_info();
    if(m_cur_partinfo!=NULL){
        char root_path[MAX_FILE_NAME]={0};
    #ifdef __HCRTOS__    
        file_mgr_get_fullname(root_path,MOUNT_ROOT_DIR,m_cur_partinfo->used_dev);
    #else
        strcpy(root_path, m_cur_partinfo->used_dev);
    #endif
        if (strcmp(file_list->dir_path, root_path)){
            return false;
        }
        else{
            return true;
        }
    }
}

void win_get_parent_dirname(char *parentpath, char *path)
{//get parent dir's path, as: /a/b/->/a/
    uint16_t  i;

    strcpy(parentpath, path);
    i = strlen(parentpath);
    while ((parentpath[i] != '/') && (i > 0)) {
        i--;
    }
    if (i != 0){
        parentpath[i] = '\0';
    } else{
        i += 1;
        parentpath[i] = '\0';
    }
}


uint16_t win_get_file_idx_fullname(file_list_t *file_list, char *file_name)
{
    glist *list_tmp;
    file_node_t *file_node;
    uint16_t index = 0;
    bool found = false;

    char full_name[MAX_FILE_NAME+1] = {0};    

    list_tmp = (glist*)file_list->list;
    while (list_tmp){
        file_node = (file_node_t*)(list_tmp->data);
        file_mgr_get_fullname(full_name, file_list->dir_path, file_node->name);
        if (!strcmp(file_name, full_name)){
            found = true;
            break;
        }
        list_tmp = list_tmp->next;
        index ++;
    }
    if (!found)
        index = INVALID_VALUE_16;

    return index;
}

char *win_media_get_cur_file_name(file_list_t *media_file_list)
{
    uint16_t file_index;
    file_node_t *file_node;
    file_list_t *file_list;

    file_list = media_file_list;
    file_index = file_list->item_index;
    file_node = file_mgr_get_file_node(file_list, file_index);
    return file_node->name;
}

char *win_media_get_pre_file(file_list_t *file_list)
{
    file_node_t *file_node = NULL;
    uint16_t item_idx;
    glist *list_tmp;
    bool found = false;

    item_idx = file_list->item_index;
    list_tmp = (glist*)file_list->list;

    if (0 == item_idx){
        printf("%s(), line:%d. index: %d, reversed!\n", __FUNCTION__, __LINE__, item_idx);
        item_idx = file_list->dir_count + file_list->file_count - 1;
    } else {
        item_idx = item_idx - 1;
    }
    list_tmp = glist_nth(list_tmp, item_idx);

    //from current position, get previous file.
    while(list_tmp){
        file_node = (file_node_t*)(list_tmp->data);
        printf("%s(), line:%d. prev file: %s, index:%d\n", __FUNCTION__, __LINE__,
            file_node->name, item_idx);

        if (FILE_DIR != file_node->type){
        printf("%s(), line:%d. found prev file: %s\n", __FUNCTION__, __LINE__,
            file_node->name);
            found = true;
            break;
        }
        list_tmp = list_tmp->prev;
    }

    //if not found, from last position reverse to find file.
    if (!found){
        list_tmp = glist_last((glist*)file_list->list);
        while(list_tmp){
            file_node = (file_node_t*)(list_tmp->data);
            printf("%s(), line:%d. prev file: %s, index:%d\n", __FUNCTION__, __LINE__,
                file_node->name, item_idx);

            if (FILE_DIR != file_node->type){
            printf("%s(), line:%d. found prev file: %s\n", __FUNCTION__, __LINE__,
                file_node->name);
                found = true;
                break;
            }
            list_tmp = list_tmp->prev;
        }
    }

    if (!found){
        printf("%s(), line:%d. not fond pre file!\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    if (NULL == file_node){
        printf("%s(), line:%d. no file node", __FUNCTION__, __LINE__);
        return NULL;
    }

    file_mgr_get_fullname(m_cur_file_name, file_list->dir_path, file_node->name);
    item_idx = glist_index((glist*)(file_list->list), (void*)file_node);
    
    file_list->item_index = item_idx;
    printf("%s(), file:%s, index:%d\n", __FUNCTION__, m_cur_file_name, item_idx);
    return m_cur_file_name;
}

char *win_media_get_next_file(file_list_t *file_list)
{
    file_node_t *file_node = NULL;
    uint16_t item_idx;
    glist *list_tmp;
    bool found = false;

    item_idx = file_list->item_index;
    list_tmp = (glist*)file_list->list;

    if (item_idx == file_list->dir_count + file_list->file_count - 1){
        printf("%s(), line:%d. index: %d, reversed!\n", __FUNCTION__, __LINE__, item_idx);
        item_idx = 0;
    } else {
        item_idx = item_idx + 1;
    }
    list_tmp = glist_nth(list_tmp, item_idx);

    //from current position, get previous file.
    while(list_tmp){
        file_node = (file_node_t*)(list_tmp->data);
        printf("%s(), line:%d. next file: %s, index:%d\n", __FUNCTION__, __LINE__,
            file_node->name, item_idx);

        if (FILE_DIR != file_node->type){
        printf("%s(), line:%d. found next file: %s\n", __FUNCTION__, __LINE__,
            file_node->name);
            found = true;
            break;
        }
        list_tmp = list_tmp->next;
    }

    //if not found, from first position find file.
    if (!found){
        list_tmp = (glist*)file_list->list;
        while(list_tmp){
            file_node = (file_node_t*)(list_tmp->data);
            printf("%s(), line:%d. next file: %s, index:%d\n", __FUNCTION__, __LINE__,
                file_node->name, item_idx);

            if (FILE_DIR != file_node->type){
            printf("%s(), line:%d. found next file: %s\n", __FUNCTION__, __LINE__,
                file_node->name);
                found = true;
                break;
            }
            list_tmp = list_tmp->next;
        }
    }


    if (!found){
        printf("%s(), line:%d. not fond pre file!\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    if (NULL == file_node){
        printf("%s(), line:%d. no file node", __FUNCTION__, __LINE__);
        return NULL;
    }

    file_mgr_get_fullname(m_cur_file_name, file_list->dir_path, file_node->name);
    item_idx = glist_index((glist*)(file_list->list), (void*)file_node);
    
    file_list->item_index = item_idx;
    printf("%s(), file:%s, index:%d\n", __FUNCTION__, m_cur_file_name, item_idx);
    return m_cur_file_name;
}

#if 0

static lv_group_t *m_list_group;
static lv_obj_t *m_list_root;
static lv_obj_t *m_label_media_name;
static lv_obj_t *m_frame_path;
static lv_obj_t *m_label_path;
static lv_obj_t *m_label_file_count;
static lv_obj_t *m_list_media = NULL;

LV_IMG_DECLARE(im_small_video)
LV_IMG_DECLARE(im_small_music)
LV_IMG_DECLARE(im_small_photo)

#define MEDIA_LIST_W    (OSD_MAX_WIDTH >> 1)
#define MEDIA_LIST_H    (OSD_MAX_HEIGHT*3/4)

#define MAX_LIST_COUNT  10
#define LABEL_LIST_X_OFF    10
#define LABEL_LIST_Y_OFF    20
#define LABEL_LIST_H_GAP    50

#define SYMBOL_SPACE    "  " //2 space

#define MEDIA_PATH_W    MEDIA_LIST_W
#define MEDIA_PATH_H    (60)


static lv_obj_t *m_list_label[MAX_LIST_COUNT];

static file_list_t m_file_list[MEDIA_TYPE_COUNT];
// file_list_t m_file_list[MEDIA_TYPE_COUNT];

static file_list_t *m_cur_file_list = NULL;


static void clear_media_list(void);
static void draw_media_list(lv_obj_t *parent);
static uint16_t win_list_ctl_proc(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t pos);

static obj_list_ctrl_t m_list_ctrl;


//transfer vkey to another vkey if necessary,
//For example: V_KEY_PREV used for V_KEY_P_UP
uint32_t vkey_transfer(uint32_t vkey)
{
    uint32_t transfer_key = VKEY_NULL;

    switch (vkey)
    {
    case V_KEY_UP:
        transfer_key = vkey;
        break;
    case V_KEY_DOWN:
        transfer_key = vkey;
        break;
#if 1        
    case V_KEY_P_UP:
    case V_KEY_PREV:
        transfer_key = V_KEY_P_UP;
        break;
    case V_KEY_P_DOWN:
    case V_KEY_NEXT:
        transfer_key = V_KEY_P_DOWN;
        break;
    default:
        break;
    }
#endif    
    return transfer_key;
}


static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
//    lv_obj_t * obj = lv_event_get_target(e);
    uint32_t vkey = VKEY_NULL;
    uint32_t act_key = VKEY_NULL;

    if(code == LV_EVENT_KEY){
        uint32_t value = lv_indev_get_key(lv_indev_get_act());
        vkey = key_convert_vkey(value);
        if (VKEY_NULL == vkey)
            return;

        // char *file_name;
        uint16_t new_item_idx = 0;
        uint16_t old_item_idx = m_cur_file_list->item_index;
        act_key = vkey_transfer(vkey);

        if (VKEY_NULL == act_key){
            api_control_send_key(vkey);
            return;
        }

        new_item_idx = win_list_ctl_proc(&m_list_ctrl, act_key, old_item_idx);
        m_cur_file_list->item_index = new_item_idx;
        printf("old_item_idx:%d, new_item_idx:%d, top:%d, cur_pos:%d, file_count:%d\n", 
            old_item_idx, new_item_idx,m_list_ctrl.top, m_list_ctrl.cur_pos, m_list_ctrl.count);

    }
}



static win_ctl_result_t win_media_file_enter(int index)
{
    bool is_root = false;
    win_ctl_result_t result = WIN_CTL_DONE;
    uint16_t file_idx;
    file_node_t *file_node = NULL;
    char *cur_file_name = NULL;
    cur_file_name = calloc(1, MAX_FILE_NAME + 1);
    if(!cur_file_name){
	    printf("Not enough memory.\n");
	    return result;
    }

    printf("%s(), line:%d, index:%d\n", __FUNCTION__, __LINE__, index);        

    is_root = win_media_is_root_dir(m_cur_file_list);

    if (index == 0){

        if (is_root){
        // exit the menu list window
            api_control_send_key(V_KEY_EXIT);
            return result;

        } else {
        // go to upper dir.
            char *parent_dir;
	    parent_dir = calloc(1, MAX_FILE_NAME);
	    if(!parent_dir){
		    if(cur_file_name)
			    free(cur_file_name);
		    return result;
	    }

            strcpy(cur_file_name, m_cur_file_list->dir_path);
            win_get_parent_dirname(parent_dir, m_cur_file_list->dir_path);
            file_mgr_create_list(m_cur_file_list, parent_dir);

            file_idx = win_get_file_idx_fullname(m_cur_file_list, cur_file_name);
            printf("%s(), line:%d, upper dir:%s, index:%d\n", __FUNCTION__, __LINE__, 
                parent_dir, file_idx);        
            if (INVALID_VALUE_16 == file_idx)
                file_idx = 0;
            //keep the upper directory's foucs 
            m_cur_file_list->item_index = file_idx;
            osd_list_ctrl_reset(&m_list_ctrl, MAX_LIST_COUNT, 
                m_cur_file_list->dir_count + m_cur_file_list->file_count, file_idx);
            draw_media_list(m_list_media);
	    free(parent_dir);

            return result;
        }
    }
    free(cur_file_name);

    // if (!is_root)
    //     index --;

    file_node = file_mgr_get_file_node(m_cur_file_list, index);
    if (NULL == file_node){
        printf("file_mgr_get_file_node() fail!\n");
        return result;
    }

    file_mgr_get_fullname(m_cur_file_name, m_cur_file_list->dir_path, file_node->name);
    if (file_node->type == FILE_DIR){
        //enter next dir
        printf("%s(), line:%d, enter next dir: %s\n", __FUNCTION__, __LINE__, m_cur_file_name);        
        file_mgr_create_list(m_cur_file_list, m_cur_file_name);
        m_cur_file_list->item_index = 0;
        osd_list_ctrl_reset(&m_list_ctrl, MAX_LIST_COUNT, 
            m_cur_file_list->dir_count + m_cur_file_list->file_count, 0);        

        draw_media_list(m_list_media);
    } else {
        win_des_t *cur_win = NULL;
        //play media file
        if (MEDIA_TYPE_VIDEO == m_cur_file_list->media_type) {
            cur_win = &g_win_media_player;
            cur_win->param = (void*)m_cur_file_name;
            menu_mgr_push(cur_win);
            result = WIN_CTL_PUSH_CLOSE;

        } else if (MEDIA_TYPE_MUSIC == m_cur_file_list->media_type) {
            cur_win = &g_win_media_player;
            cur_win->param = (void*)m_cur_file_name;
            menu_mgr_push(cur_win);
            result = WIN_CTL_PUSH_CLOSE;
        } else if (MEDIA_TYPE_PHOTO == m_cur_file_list->media_type) {

            cur_win = &g_win_image_slide;
            cur_win->param = (void*)m_cur_file_name;
            menu_mgr_push(cur_win);
            result = WIN_CTL_PUSH_CLOSE;
        }else if (MEDIA_TYPE_TXT== m_cur_file_list->media_type) {

            cur_win = &g_win_ebook;
            cur_win->param = (void*)m_cur_file_name;
            menu_mgr_push(cur_win);
            result = WIN_CTL_PUSH_CLOSE;
        }
    }

    return result;
}

static void clear_media_list(void)
{
    int i;
//    lv_group_remove_all_objs(m_list_group);
    for (i = 0; i < MAX_LIST_COUNT; i ++){
        if (NULL != m_list_label[i]){
            lv_group_remove_obj(m_list_label[i]);
            lv_obj_del(m_list_label[i]);
            m_list_label[i] = NULL;
        }
    }
}

static void draw_media_list(lv_obj_t *parent)
{
    int x = LABEL_LIST_X_OFF;
    int y = LABEL_LIST_Y_OFF;
    int h = LABEL_LIST_H_GAP;
    glist *list_it;
    file_node_t *file_node;
    int node_start;
    int count = 0;
    char *item_icon;
    int i;
    lv_obj_t *focus_label = NULL;

    lv_label_set_text(m_label_path, m_cur_file_list->dir_path);
    lv_label_set_text_fmt(m_label_file_count, "%d/%d\n", 
        m_cur_file_list->item_index, m_cur_file_list->dir_count+m_cur_file_list->file_count-1);
    
    clear_media_list();

    list_it = (glist*)m_cur_file_list->list;
    count = m_list_ctrl.depth;
    node_start = m_list_ctrl.top;
    int foucus_idx;

    foucus_idx = m_cur_file_list->item_index-node_start;
printf("%s(), line:%d, node_start:%d, item_index:%d, foucus_idx:%d\n", __FUNCTION__, __LINE__, 
    node_start, m_cur_file_list->item_index, foucus_idx);

    for (i = 0; i < count; i ++){
        if (NULL == list_it)
            break;

        file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
printf("%s(), line:%d, file_name[%d]:%s\n", __FUNCTION__, __LINE__, i, file_node->name);
        if (0 == node_start){
            item_icon = LV_SYMBOL_HOME;
        } else if (FILE_DIR == file_node->type){
            item_icon = LV_SYMBOL_DIRECTORY;
        } else if (FILE_VIDEO == file_node->type){
            item_icon = LV_SYMBOL_VIDEO;
        } else if (FILE_MUSIC == file_node->type){
            item_icon = LV_SYMBOL_AUDIO;
        } else {
            item_icon = LV_SYMBOL_IMAGE;
        }    
        m_list_label[i] = lv_label_create(parent);
        lv_obj_add_style(m_list_label[i], TEXT_STY_LEFT_NORMAL, 0); 
        lv_obj_add_style(m_list_label[i], TEXT_STY_LEFT_HIGH, LV_STATE_FOCUSED); 
        lv_label_set_text_fmt(m_list_label[i], "%s"SYMBOL_SPACE"%s", item_icon, file_node->name);
        lv_obj_set_pos(m_list_label[i], x, y+h*i);
        lv_label_set_long_mode(m_list_label[i], LV_LABEL_LONG_SCROLL_CIRCULAR);//LV_LABEL_LONG_DOT, LV_LABEL_LONG_SCROLL
        lv_obj_add_event_cb(m_list_label[i], event_handler, LV_EVENT_KEY, NULL);
        lv_group_add_obj(m_list_group, m_list_label[i]);

        if (i == foucus_idx){
            printf("focus_label index:%d\n", i);
            focus_label = m_list_label[i];
        }else{
            lv_obj_clear_state(m_list_label[i], LV_STATE_FOCUSED);
        }

        list_it = list_it->next;
        node_start ++;
    }
    if (NULL != focus_label)
        lv_group_focus_obj(focus_label);
    else{
		lv_obj_add_event_cb(parent, event_handler, LV_EVENT_KEY, NULL);
		lv_group_add_obj(m_list_group, parent);
    }


}


static int win_media_list_open(void *arg)
{
    int str_id;
    void *img_src;
    media_type_t media_type = (media_type_t)arg;
    m_list_root = lv_obj_create(lv_scr_act());
    int i;
    (void)img_src;

    //regist key device to group, so that the objects in the group can 
    //get the key event.    
    m_list_group = lv_group_create();

    //user need get the LV_KEY_NEXT and LV_KEY_PREV key, so disable auto focus.
    m_list_group->auto_focus_dis = 1;
    key_regist(m_list_group);

    osd_draw_background(m_list_root, false);

    if (media_type == MEDIA_TYPE_VIDEO){
        str_id = STR_VIDEO;
        img_src = &im_small_video;
    } else if (media_type == MEDIA_TYPE_MUSIC){
        str_id = STR_MUSIC;
        img_src = &im_small_music;
    }else{
        str_id = STR_PHOTO;
        img_src = &im_small_photo;
    }

    // obj_img_open(m_list_root, img_src, SUB_NAME_X-60, SUB_NAME_Y -50);
    m_label_media_name = obj_label_open(m_list_root, SUB_NAME_X, SUB_NAME_Y+4 -50, SUB_NAME_W, osd_get_string(str_id));
    lv_style_init(&sty_font_large);
    lv_style_set_text_font(&sty_font_large, FONT_SIZE_LARGE);
    lv_style_set_text_color(&sty_font_large, COLOR_WHITE);
    lv_obj_add_style(m_label_media_name, &sty_font_large, 0); 

    m_frame_path = lv_obj_create(m_list_root);
    lv_obj_clear_flag(m_frame_path, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(m_frame_path, MEDIA_PATH_W, MEDIA_PATH_H);
    lv_obj_align(m_frame_path, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(m_frame_path, COLOR_DEEP_GREY, 0);

    m_label_path = lv_label_create(m_frame_path);
    lv_obj_add_style(m_label_path, TEXT_STY_LEFT_NORMAL, 0); 
    lv_obj_set_size(m_label_path, MEDIA_PATH_W-180, 36);
    lv_obj_align(m_label_path, LV_ALIGN_TOP_LEFT, 0, -6);
    lv_label_set_long_mode(m_label_path, LV_LABEL_LONG_SCROLL_CIRCULAR);//LV_LABEL_LONG_DOT, LV_LABEL_LONG_SCROLL_CIRCULAR

    m_label_file_count = lv_label_create(m_frame_path);
    lv_obj_add_style(m_label_file_count, TEXT_STY_LEFT_NORMAL, 0); 
    //lv_obj_set_pos(m_label_file_count, MEDIA_PATH_W-160, 36);
    lv_obj_align(m_label_file_count, LV_ALIGN_TOP_RIGHT, 0, -6);

    m_cur_file_list = &m_file_list[media_type];
    m_cur_file_list->media_type = media_type;
    if ('\0' == m_cur_file_list->dir_path[0])
        strncpy(m_cur_file_list->dir_path, MOUNT_ROOT_DIR, MAX_FILE_NAME-1);


    m_list_media = lv_obj_create(m_list_root);
    lv_obj_clear_flag(m_list_media, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_set_size(m_list_media, MEDIA_LIST_W, MEDIA_LIST_H);
    lv_obj_center(m_list_media);
    lv_obj_set_style_bg_color(m_list_media, COLOR_DEEP_GREY, 0);
    for (i = 0; i < MAX_LIST_COUNT; i ++)
        m_list_label[i] = NULL;

    file_mgr_create_list(m_cur_file_list, m_cur_file_list->dir_path);

    osd_list_ctrl_reset(&m_list_ctrl, MAX_LIST_COUNT, 
        m_cur_file_list->dir_count + m_cur_file_list->file_count, 
        m_cur_file_list->item_index);

    draw_media_list(m_list_media);

    return API_SUCCESS;
}


static int win_media_list_close(void *arg)
{
    lv_group_remove_all_objs(m_list_group);
    lv_group_del(m_list_group);
    lv_obj_del(m_list_root);
    m_list_media = NULL;
    return API_SUCCESS;
}

static uint16_t win_list_ctl_proc(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t pos)
{
    int i; 
    uint16_t count;
    int node_start;
    file_node_t *file_node;
    int focus_idx;
    uint16_t item_new_pos = 0;

    //update the list ctrl postion information
    if (osd_list_ctrl_update(list_ctrl, vkey, pos, &item_new_pos)){
        //redraw the list label according the new postion infromation
        count = list_ctrl->depth;
        node_start = list_ctrl->top;
        char *item_icon;
        for (i = 0; i < count; i ++){
            printf("update: node_start:%d\n", node_start);
            if (node_start < list_ctrl->count){
                file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
                if (0 == node_start){
                    item_icon = LV_SYMBOL_HOME;
                } else if (FILE_DIR == file_node->type){
                    item_icon = LV_SYMBOL_DIRECTORY;
                } else if (FILE_VIDEO == file_node->type){
                    item_icon = LV_SYMBOL_VIDEO;
                } else if (FILE_MUSIC == file_node->type){
                    item_icon = LV_SYMBOL_AUDIO;
                } else {
                    item_icon = LV_SYMBOL_IMAGE;
                }    
                lv_label_set_text_fmt(m_list_label[i], "%s"SYMBOL_SPACE"%s", item_icon, file_node->name);
            }else{
                lv_label_set_text(m_list_label[i], "");
            }
            node_start ++;
        }
    }
    focus_idx = item_new_pos - list_ctrl->top;
printf("%s(),line:%d. focus_idx:%d, item_new_pos:%d\n", __FUNCTION__, __LINE__, 
    focus_idx, item_new_pos);    
    lv_group_focus_obj(m_list_label[focus_idx]);

    lv_label_set_text_fmt(m_label_file_count, "%d/%d\n", 
        item_new_pos, m_cur_file_list->dir_count+m_cur_file_list->file_count-1);

    return item_new_pos; 
} 

static win_ctl_result_t win_list_key_act(uint32_t key)
{
    win_ctl_result_t ret = WIN_CTL_NONE;
    lv_obj_t *sel_obj = NULL;
    
    if (key == V_KEY_ENTER){
        sel_obj = lv_group_get_focused(m_list_group);
        if (NULL == sel_obj)
            return ret;

        ret = win_media_file_enter(m_cur_file_list->item_index);

    }else if (key == V_KEY_EXIT){
        ret = WIN_CTL_POPUP_CLOSE;
    }
    else if (key == V_KEY_MENU){
        ret = WIN_CTL_POPUP_CLOSE;   
    }

    return ret;
}

static win_ctl_result_t win_list_msg_act(control_msg_t *ctl_msg)
{
    win_ctl_result_t ret = WIN_CTL_NONE;

    return ret;
}

static win_ctl_result_t win_media_list_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;

    if (MSG_TYPE_KEY == ctl_msg->msg_type){
        ret = win_list_key_act(ctl_msg->msg_code);
    }else{
        ret = win_list_msg_act(ctl_msg);
    }

    return ret;

}

file_list_t *win_media_get_cur_list()
{
    return m_cur_file_list;
}
#if 0
char *win_media_get_cur_file_name(void)
{
    uint16_t file_index;
    file_node_t *file_node;
    file_list_t *file_list;

    file_list = win_media_get_cur_list();
    file_index = file_list->item_index;
    file_node = file_mgr_get_file_node(file_list, file_index);
    return file_node->name;
}
#endif 

win_des_t g_win_media_list =
{
    .open = win_media_list_open,
    .close = win_media_list_close,
    .control = win_media_list_control,
};


#endif