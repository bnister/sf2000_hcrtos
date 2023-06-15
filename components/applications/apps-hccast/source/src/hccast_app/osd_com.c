#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "../lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include "com_api.h"
#include "osd_com.h"
#include "key.h"
#include "ui_rsc/lan_str/lan_str_eng.h"

#include "ui_rsc/lan_str/lan_str_eng.h"
//#include "ui_rsc/lan_str/lan_str_chn.h"

lv_style_t m_text_style_mid_normal;
lv_style_t m_text_style_mid_high;
lv_style_t m_text_style_mid_disable;
lv_style_t m_text_style_left_normal;
lv_style_t m_text_style_left_high;
lv_style_t m_text_style_left_disable;
lv_style_t m_text_style_right_normal;
lv_style_t m_text_style_right_high;
lv_style_t m_text_style_right_disable;

static language_type_t m_language = LANGUAGE_ENGLISH;
static lv_obj_t *m_obj_msg = NULL;
static lv_timer_t *msgbox_timer = NULL;
static msg_timeout_func m_msg_timeout_func = NULL;
static void *m_timeout_func_data = NULL;

static char **m_language_list[] = {
    eng_string,
    // chn_string,
    // fre_string,
    // span_string,
};

lv_obj_t *obj_img_open(void *parent, void *bitmap, int x, int y)
{
    lv_obj_t * img = lv_img_create(parent);
    lv_img_set_src(img, (const char*)bitmap);
    lv_obj_set_pos(img, x, y);
    //lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, 0);
    return img;
}

lv_obj_t *obj_label_open(void *parent, int x, int y, int w, char *str)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, w);
    lv_obj_set_height(label, 60);
    if (str)
        lv_label_set_text(label, str);

    return label;
}

static void osd_set_language(language_type_t language)
{
    m_language = language;
}

language_type_t osd_get_language(void)
{
    return m_language;
}


char *osd_get_string(int string_id)
{
    language_type_t language_idx = LANGUAGE_ENGLISH;
    language_idx = osd_get_language();

    char **lan_str;
    lan_str = m_language_list[language_idx];
    if (!lan_str)
        return NULL;

    //printf("string_id: %d, string: %s\n", string_id, lan_str[string_id]);
    return lan_str[string_id];
}

void osd_draw_background(void *parent, bool if_transp)
{
    if (if_transp) {//set the backgroud to transparency, may only valid 32bit colors??
        lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, LV_PART_MAIN);
        //lv_obj_set_style_bg_color(parent, LV_COLOR_CHROMA_KEY, LV_PART_MAIN);
    }
    else {
         lv_obj_set_style_bg_color(parent, COLOR_FRENCH_GREY, LV_PART_MAIN);
    }
    lv_obj_set_pos(parent, 0, 0);
    lv_obj_set_size(parent, OSD_MAX_WIDTH, OSD_MAX_HEIGHT);
    lv_obj_set_style_border_opa(parent, LV_OPA_TRANSP, LV_PART_MAIN);
}


void osd_obj_com_set(void)
{
    //common font setting
    lv_style_init(&m_text_style_mid_normal);
    lv_style_set_text_font(&m_text_style_mid_normal, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_mid_normal, COLOR_WHITE);
    lv_style_set_text_align(&m_text_style_mid_normal, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&m_text_style_mid_high);
    lv_style_set_text_font(&m_text_style_mid_high, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_mid_high, FONT_COLOR_HIGH);
    lv_style_set_text_align(&m_text_style_mid_high, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&m_text_style_mid_disable);
    lv_style_set_text_font(&m_text_style_mid_disable, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_mid_disable, FONT_COLOR_GRAY);
    lv_style_set_text_align(&m_text_style_mid_disable, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&m_text_style_left_normal);
    lv_style_set_text_font(&m_text_style_left_normal, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_left_normal, COLOR_WHITE);
    lv_style_set_text_align(&m_text_style_left_normal, LV_TEXT_ALIGN_LEFT);

    lv_style_init(&m_text_style_left_high);
    lv_style_set_text_font(&m_text_style_left_high, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_left_high, FONT_COLOR_HIGH);
    lv_style_set_text_align(&m_text_style_left_high, LV_TEXT_ALIGN_LEFT);

    lv_style_init(&m_text_style_left_disable);
    lv_style_set_text_font(&m_text_style_left_disable, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_left_disable, FONT_COLOR_GRAY);
    lv_style_set_text_align(&m_text_style_left_disable, LV_TEXT_ALIGN_LEFT);

    lv_style_init(&m_text_style_right_normal);
    lv_style_set_text_font(&m_text_style_right_normal, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_right_normal, COLOR_WHITE);
    lv_style_set_text_align(&m_text_style_right_normal, LV_TEXT_ALIGN_RIGHT);

    lv_style_init(&m_text_style_right_high);
    lv_style_set_text_font(&m_text_style_right_high, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_right_high, FONT_COLOR_HIGH);
    lv_style_set_text_align(&m_text_style_right_high, LV_TEXT_ALIGN_RIGHT);

    lv_style_init(&m_text_style_right_disable);
    lv_style_set_text_font(&m_text_style_right_disable, FONT_SIZE_MID);
    lv_style_set_text_color(&m_text_style_right_disable, FONT_COLOR_GRAY);
    lv_style_set_text_align(&m_text_style_right_disable, LV_TEXT_ALIGN_RIGHT);

}

#if 1
void osd_list_ctrl_reset(obj_list_ctrl_t *list_ctrl, uint16_t depth, uint16_t item_count, uint16_t new_pos)
{
printf("%s(), line:%d, depth:%d, item_count:%d, new_pos:%d\n", __FUNCTION__, __LINE__, \
    depth, item_count, new_pos);    

    if (new_pos > depth-1){
        list_ctrl->top = new_pos-(depth-1);
        list_ctrl->cur_pos = new_pos;
        list_ctrl->new_pos = new_pos;
    }else{
        list_ctrl->top = 0;
        list_ctrl->cur_pos = new_pos;
        list_ctrl->new_pos = new_pos;
    }
    list_ctrl->depth = depth;
    list_ctrl->count = item_count;

printf("%s(), line:%d, list: top:%d, cur_pos:%d, new_pos:%d\n", __FUNCTION__, __LINE__, \
    list_ctrl->top, list_ctrl->cur_pos, list_ctrl->new_pos);    

}


bool osd_list_ctrl_shift(obj_list_ctrl_t *list_ctrl, int16_t shift, uint16_t *new_top, uint16_t *new_pos)
{
    int16_t point, top;
    uint16_t page_point;
    uint32_t page_moving;
    uint16_t check_cnt = 0;

    uint16_t shift_top;

    if(list_ctrl->count == 0 || shift == 0)
        return false;

    if(list_ctrl->new_pos < list_ctrl->top)
        list_ctrl->new_pos = list_ctrl->cur_pos = list_ctrl->top;    
    if(list_ctrl->new_pos > list_ctrl->count)
        list_ctrl->top = list_ctrl->new_pos = list_ctrl->cur_pos = 0;
        
    page_point = list_ctrl->new_pos - list_ctrl->top;

    point = list_ctrl->new_pos;
    top   = list_ctrl->top;
    shift_top = list_ctrl->top;

    printf("%d: shift = %d,top=%d,point=%d\n", __LINE__, shift,top,point);
    do{
        page_moving = (shift == list_ctrl->depth || shift == -list_ctrl->depth)? 1 : 0;
            
        point += shift;

        /* If move out of current page, the top point also need to move.*/
        if( (point < top) || (point >= (top + list_ctrl->depth) ) )
        {
            /*
            if((list_ctrl->bListStyle & LIST_FIX_PAGE)&&(page_moving==0))
            {
                if(shift>0)
                    top += list_ctrl->depth;
                else
                    top -= list_ctrl->depth;
            }
            else
            */
            {
                top += shift;
            }
        }
        printf("%d: shift = %d,top=%d,point=%d\n", __LINE__, shift,top,point);

        /* Moving in current page only.*/
        if(top == shift_top && point < list_ctrl->count)
        {
            printf("%d: shift = %d,top=%d,point=%d\n", __LINE__, shift,top,point);
        }
        else if(top < 0)   /*Moving to unexist "up" page*/
        {
            printf("%d: top < 0\n", __LINE__);
            if(shift_top > 0) // Must be page moving
            {
                printf("%d: shift_top > 0\n", __LINE__);
                /* Need move to first page */
                //page_moving 
                top = 0;

                if(page_moving)
                    point = page_point;
                else
                    top = point;                    
                /*
                if(page_moving && (list_ctrl->bListStyle & LIST_PAGE_KEEP_CURITEM))
                    point = page_point;
                else
                {
                    top = point;
                    //point = 0;
                }
                */
                
                printf("%d: page moving=%d: top=%d, point = %d\n", __LINE__, page_moving, top, point);
            }
            else//shift_top == 0
            {   
                /* Need move to last page */

                printf("%d: shift_top == 0\n", __LINE__);

                /* Don't allow to scroll */
               // if (!(list_ctrl->bListStyle & LIST_SCROLL))
                //    return FALSE;
                
            #if 1
                top = list_ctrl->count - list_ctrl->depth;
                if(top < 0)
                    top = 0;

                if(page_moving)
                    point = top + page_point;
                else
                    point = list_ctrl->count - 1;

            #else
                if(list_ctrl->bListStyle & LIST_FULL_PAGE)
                {
                    top = list_ctrl->count - list_ctrl->depth;
                    if(top < 0)
                        top = 0;
                }
                else
                    top = (list_ctrl->count-1) / list_ctrl->depth * list_ctrl->depth;

                if(page_moving && (list_ctrl->bListStyle & LIST_PAGE_KEEP_CURITEM))
                    point = top + page_point;
                else
                    point = list_ctrl->count - 1;
            #endif

                if(point >= list_ctrl->count)
                    point = list_ctrl->count - 1;
            }
        }       
        else if(point >= list_ctrl->count)   /*Moving to unexist "down" page*/
        {
            printf("%d: point >= list_ctrl->count\n", __LINE__);
            
            if(shift_top + list_ctrl->depth < list_ctrl->count) // Must be page moving
            {
                //page_moving 

                printf("%d: shift_top + list_ctrl->depth < list_ctrl->count\n", __LINE__);
                
                /* Need move to last page */            
                //if(list_ctrl->bListStyle & LIST_FULL_PAGE)
                if(1)
                {
                    top = list_ctrl->count - list_ctrl->depth;
                    if(top < 0)
                        top = 0;
                }
                else
                    top = (list_ctrl->count-1) / list_ctrl->depth * list_ctrl->depth;
                
            #if 1
                if(page_moving)
                    point = top + page_point;
                else
                    point = list_ctrl->count - 1;
            #else
                if(page_moving && (list_ctrl->bListStyle & LIST_PAGE_KEEP_CURITEM))
                    point = top + page_point;
                else
                    point = list_ctrl->count - 1;
            #endif
                if(point >= list_ctrl->count)
                    point = list_ctrl->count - 1;
            }
            else
            {
               /* Need move to first page */

               printf("%d: Need move to first page\n", __LINE__);
               /* Don't allow to scroll */
                // if (!(list_ctrl->bListStyle & LIST_SCROLL))
                //     return FALSE;

                //page_moving 
                top = 0;
                
                //if(page_moving && (list_ctrl->bListStyle & LIST_PAGE_KEEP_CURITEM))
                if(page_moving)
                    point = page_point;
                else
                    point = 0;   
            }
        }
        else
        {
            printf("?\n");
        }

    #if 0
        if(list_ctrl->bListStyle & LIST_ITEMS_COMPLETE)
            pItem = list_ctrl->pListField[point];
        else
            pItem = list_ctrl->pListField[point - top];

        if(shift_top != top)
        {               
            //libc_printf("objestlist page changed from %d to %d\n", shift_top, top);           
            OSD_SIGNAL((POBJECT_HEAD)list_ctrl, EVN_LIST_PAGE_PRE_CHANGE, top, list_ctrl->depth);
        }
        if(OSD_CheckAttr(pItem, C_ATTR_ACTIVE))
            break;
    #endif
        shift_top = top;
        check_cnt++;
    }while(0);

    list_ctrl->cur_pos = list_ctrl->new_pos = point;
    list_ctrl->top = top;

    printf("%d: shift = %d,top=%d,point=%d\n", __LINE__, shift,top,point);


    *new_pos  = point;
    *new_top    = top;


    return true;

}

bool osd_list_ctrl_update(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos)
{
    uint16_t old_top = 0;
    uint16_t new_position = 0;
    uint16_t new_top = 0;
    int16_t step = 0;
    bool ret;

    old_top = list_ctrl->top;

    switch (vkey)
    {
    case V_KEY_UP:
        step = -1;
        break;
    case V_KEY_DOWN:
        step = 1;
        break;
    case V_KEY_P_UP:
        step = -list_ctrl->depth;
        break;
    case V_KEY_P_DOWN:
        step = list_ctrl->depth;
        break;
    }

    ret = osd_list_ctrl_shift(list_ctrl, step, &new_top, &new_position); 
    *new_pos = new_position;
    if (!ret)
    {
        return false;
    }
    if (old_top != new_top)
    {
        return true;
    }
    return false;
}

#else
/**
 * Reset the list_ctrl control. When enter a new dir, should be reset to update current items position.
 * @param list_ctrl  [list control]
 * @param count      [the item count in a page]
 * @param item_count [all the count(dirs and files) of file list]
 * @param new_pos    [to set a positon of the file list, start and end may 
 *                    be changes according to the position]
 */
void osd_list_ctrl_reset(obj_list_ctrl_t *list_ctrl, uint16_t count, uint16_t item_count, uint16_t new_pos)
{

printf("%s(), line:%d, item_count:%d, start:%d, end:%d, pos:%d\n", __FUNCTION__, __LINE__, \
    item_count, list_ctrl->start, list_ctrl->end, new_pos);    

    if (new_pos > count-1){
        list_ctrl->pos = new_pos;
        list_ctrl->start = new_pos-count+1;
        list_ctrl->end = new_pos;
    }else{
        list_ctrl->start = 0;
        list_ctrl->pos = 0;
        list_ctrl->end = count-1;
    }
printf("%s(), line:%d, item_count:%d, start:%d, end:%d, pos:%d\n", __FUNCTION__, __LINE__, \
    item_count, list_ctrl->start, list_ctrl->end, new_pos);    

    list_ctrl->list_count = item_count;
}

bool osd_list_ctrl_update(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos)
{
    int count;
    bool update_start_end = true;
    uint16_t jump_cnt;

    list_ctrl->pos = old_pos;

    count = list_ctrl->end - list_ctrl->start + 1;
    //1st case: NO need to go to next page, only update the list's position information.
    if (count >= list_ctrl->list_count){
        if (V_KEY_UP == vkey){
            if (list_ctrl->pos == list_ctrl->start){
                list_ctrl->pos = list_ctrl->list_count-1;
            }else{
                list_ctrl->pos --;
            }
        }else if (V_KEY_DOWN == vkey){
            if (list_ctrl->pos == list_ctrl->list_count-1){
                list_ctrl->pos = 0;
            }else{
                list_ctrl->pos ++;
            }
        }
        *new_pos = list_ctrl->pos;
        update_start_end = false;
        return update_start_end;
    }


    //2nd case: may got to next page, should upate the list's start, end, position information.
    if (0 == list_ctrl->start){
    //2.1 case: the current list page is the first page
        if (V_KEY_UP == vkey){
            if (0 == list_ctrl->pos){
                list_ctrl->pos = list_ctrl->list_count -1;
                list_ctrl->end = list_ctrl->list_count -1;
                list_ctrl->start = list_ctrl->end - count +1;
            } else {
                list_ctrl->pos --;
                update_start_end = false;
            }
        }else if (V_KEY_DOWN == vkey){
            if (list_ctrl->end == list_ctrl->pos){
                list_ctrl->pos ++;
                list_ctrl->start ++;
                list_ctrl->end ++;
            } else {
                list_ctrl->pos ++;
                update_start_end = false;
            }
        }else if (V_KEY_P_UP == vkey){
        }else if (V_KEY_P_DOWN == vkey){
            if (list_ctrl->pos+count < list_ctrl->list_count)
                jump_cnt = count;
            else
                jump_cnt = list_ctrl->list_count-list_ctrl->pos;

            printf("pos:%d, start:%d, end:%d, count:%d, list_count:%d, jump_cnt:%d\n", list_ctrl->pos, 
                list_ctrl->start, list_ctrl->end, count, list_ctrl->list_count, jump_cnt);

            list_ctrl->pos += jump_cnt;
            list_ctrl->start += jump_cnt;
            list_ctrl->end += jump_cnt;
            if (list_ctrl->end > (list_ctrl->list_count-1))
                list_ctrl->end =  list_ctrl->list_count-1;
            
            printf("2 pos:%d, start:%d, end:%d, count:%d, list_count:%d, jump_cnt:%d\n", list_ctrl->pos, 
                list_ctrl->start, list_ctrl->end, count, list_ctrl->list_count, jump_cnt);
        }
    } else if ((list_ctrl->list_count-1) == list_ctrl->end){
    //2.2 case: the current list page is the last page
        if (V_KEY_UP == vkey){

            if (list_ctrl->pos == list_ctrl->start){
                list_ctrl->pos --;
                list_ctrl->end --;
                list_ctrl->start --;
            } else {
                list_ctrl->pos --;
                update_start_end = false;
            }
        }else if (V_KEY_DOWN == vkey){
            if (list_ctrl->end == list_ctrl->pos){
                list_ctrl->pos = 0;
                list_ctrl->start = 0;
                list_ctrl->end = count-1;
            } else {
                list_ctrl->pos ++;
                update_start_end = false;
            }
            
        }else if (V_KEY_P_UP == vkey){
            if (count-1 > list_ctrl->pos){
                list_ctrl->pos = 0;
                list_ctrl->start = 0;
                list_ctrl->end = count -1;
            } else {
                list_ctrl->pos -= count;
                list_ctrl->start -= count;
                list_ctrl->end -= count;
            }
            
        }else if (V_KEY_P_DOWN == vkey){
        }



    }else {
    //2.3 case: the current list page is the middle page
        if (V_KEY_UP == vkey){
            if (list_ctrl->pos == list_ctrl->start){
                list_ctrl->pos --;
                list_ctrl->end --;
                list_ctrl->start --;
            } else {
                list_ctrl->pos --;
                update_start_end = false;
            }
        }else if (V_KEY_DOWN == vkey){
            if (list_ctrl->pos == list_ctrl->end){
                list_ctrl->pos ++;
                list_ctrl->start ++;
                list_ctrl->end ++;
            } else {
                list_ctrl->pos ++;
                update_start_end = false;
            }
            
        }else if (V_KEY_P_UP == vkey){
            if (count-1 > list_ctrl->pos){
                list_ctrl->pos = 0;
                list_ctrl->start = 0;
                list_ctrl->end = count -1;
            } else {
                list_ctrl->pos -= count;
                list_ctrl->start -= count;
                list_ctrl->end -= count;
            }
            
        }else if (V_KEY_P_DOWN == vkey){
            if (list_ctrl->pos+count < list_ctrl->list_count)
                jump_cnt = count;
            else
                jump_cnt = list_ctrl->list_count-list_ctrl->pos;

            list_ctrl->pos += jump_cnt;
            list_ctrl->start += jump_cnt;
            list_ctrl->end += jump_cnt;
        }
    }

    *new_pos = list_ctrl->pos;
    return update_start_end;
}
#endif

int win_msgbox_btn_open(const char *str_msg, uint32_t timeout)
{
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Message box function for message
static pthread_mutex_t m_mutex_msgbox = PTHREAD_MUTEX_INITIALIZER;

static void msgbox_msg_clear()
{
    if (msgbox_timer){
        lv_timer_pause(msgbox_timer);
        lv_timer_del(msgbox_timer);
    }
    msgbox_timer = NULL;

    if (m_obj_msg)
        lv_obj_del(m_obj_msg);

    m_obj_msg = NULL;
}

static void msgbox_timer_cb(lv_timer_t * t)
{
    //msgbox_msg_clear();
    pthread_mutex_lock(&m_mutex_msgbox);
	msgbox_msg_clear();
	
    if (m_msg_timeout_func){
        m_msg_timeout_func(m_timeout_func_data);
    }
    pthread_mutex_unlock(&m_mutex_msgbox);
}

void win_msgbox_msg_open(const char *str_msg, uint32_t timeout, msg_timeout_func timeout_func, void *user_data)
{
    int w = OSD_MAX_WIDTH/3;
    int h = OSD_MAX_HEIGHT/3;

    win_msgbox_msg_close();

    m_obj_msg = lv_obj_create(lv_scr_act());

    lv_obj_set_style_bg_color(m_obj_msg, COLOR_DEEP_GREY, LV_PART_MAIN); //grey
    lv_obj_set_style_border_opa(m_obj_msg, LV_OPA_TRANSP,LV_PART_MAIN);
    lv_obj_set_style_radius(m_obj_msg, 10, 0);
    lv_obj_clear_flag(m_obj_msg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(m_obj_msg, w, h);
    lv_obj_center(m_obj_msg);

    lv_obj_t *txt_msg = lv_label_create(m_obj_msg);
    lv_obj_add_style(txt_msg, TEXT_STY_MID_NORMAL, 0);
    lv_obj_center(txt_msg);

    lv_label_set_text(txt_msg, str_msg);
    
    if (0 != timeout && INFINITE_VALUE != timeout){
        msgbox_timer = lv_timer_create(msgbox_timer_cb, timeout, NULL);
    }

    if (timeout_func){
        pthread_mutex_lock(&m_mutex_msgbox);
        m_msg_timeout_func = timeout_func;
        m_timeout_func_data = user_data;
        pthread_mutex_unlock(&m_mutex_msgbox);
    }
}

void win_msgbox_msg_close(void)
{
    pthread_mutex_lock(&m_mutex_msgbox);
	msgbox_msg_clear();
    m_msg_timeout_func = NULL;
    m_timeout_func_data = NULL;
    pthread_mutex_unlock(&m_mutex_msgbox);
}

/////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////////////
// Data buffering circle, mainly for media playing to download data
// 

//define this means use grey background with the circle buffering, to 
//improve the smooth of th circle buffering.
//#define WIN_BUFFERING_UI_WITH_BG

static lv_obj_t *m_arc_buffering = NULL;
static lv_obj_t *m_txt_buffering = NULL;
#ifdef WIN_BUFFERING_UI_WITH_BG
static lv_obj_t *m_obj_buffering = NULL;
#endif
static lv_anim_t m_anim_buff;


static void set_angle(void * obj, int32_t v)
{
#ifdef WIN_BUFFERING_UI_WITH_BG    
    if (!m_obj_buffering)
        return;
#endif    
    if (m_arc_buffering && obj)
        lv_arc_set_value(obj, v);
}

void win_data_buffing_open(void)
{

#ifdef WIN_BUFFERING_UI_WITH_BG
    if (m_obj_buffering)
        lv_obj_del(m_obj_buffering);
    m_obj_buffering = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_obj_buffering, 200, 200);
    lv_obj_clear_flag(m_obj_buffering, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(m_obj_buffering);
    lv_obj_set_style_bg_color(m_obj_buffering, COLOR_DEEP_GREY, LV_PART_MAIN); //grey
    lv_obj_set_style_border_opa(m_obj_buffering, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(m_obj_buffering, 10, 0);
    m_arc_buffering = lv_arc_create(m_obj_buffering);

#else
    if (m_arc_buffering)
         lv_obj_del(m_arc_buffering);
     m_arc_buffering = lv_arc_create(lv_scr_act());
#endif     

    lv_obj_set_size(m_arc_buffering, 150, 150);
    lv_arc_set_rotation(m_arc_buffering, 270);
    lv_arc_set_bg_angles(m_arc_buffering, 0, 360);
    lv_obj_remove_style(m_arc_buffering, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    //lv_obj_clear_flag(m_arc_buffering, LV_OBJ_FLAG_CLICKABLE);  /*To not allow adjusting by click*/
    lv_obj_set_style_arc_color(m_arc_buffering, lv_color_hex(0xD0D0D0), LV_PART_MAIN);

#if 1
    lv_anim_init(&m_anim_buff);
    lv_anim_set_var(&m_anim_buff, m_arc_buffering);
    lv_anim_set_exec_cb(&m_anim_buff, set_angle);
    lv_anim_set_time(&m_anim_buff, 1000);
    lv_anim_set_repeat_count(&m_anim_buff, LV_ANIM_REPEAT_INFINITE);    /*Just for the demo*/
    lv_anim_set_repeat_delay(&m_anim_buff, 200);
    lv_anim_set_values(&m_anim_buff, 0, 100);
    lv_anim_start(&m_anim_buff);

#else
    lv_arc_set_value(m_arc_buffering, 0);
#endif    
    lv_obj_center(m_arc_buffering);

    m_txt_buffering = lv_label_create(m_arc_buffering);
    lv_obj_center(m_txt_buffering);
    lv_obj_set_style_text_color(m_txt_buffering, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(m_txt_buffering, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_label_set_text(m_txt_buffering, "Buffering...");
}

void win_data_buffing_update(int percent)
{
    int value;
    char str_msg[64] = {0};

    if (!m_arc_buffering)
        return;
    if (percent <= 0){
        value = 0;
        sprintf(str_msg, "Buffering...");
    } else if (percent < 100 ){
        value = percent;
        sprintf(str_msg, "%d%%", value);
    } else {
        value = 100;
        sprintf(str_msg, "%d%%", value);
    }

#ifdef WIN_BUFFERING_UI_WITH_BG
    if (100 == value){
        lv_obj_add_flag(m_obj_buffering, LV_OBJ_FLAG_HIDDEN);
    } else {
        //lv_arc_set_value(m_arc_buffering, value);
        lv_label_set_text(m_txt_buffering, str_msg);
        lv_obj_clear_flag(m_obj_buffering, LV_OBJ_FLAG_HIDDEN);
    }
#else
    if (100 == value){
        lv_obj_add_flag(m_arc_buffering, LV_OBJ_FLAG_HIDDEN);
    } else {
        //lv_arc_set_value(m_arc_buffering, value);
        lv_label_set_text(m_txt_buffering, str_msg);
        lv_obj_clear_flag(m_arc_buffering, LV_OBJ_FLAG_HIDDEN);
    }

#endif    
}

void win_data_buffing_close(void)
{
#ifdef WIN_BUFFERING_UI_WITH_BG    
    if (m_obj_buffering)
        lv_obj_del(m_obj_buffering);

    m_obj_buffering = NULL;
#else
    if (m_arc_buffering)
        lv_obj_del(m_arc_buffering);

    m_arc_buffering = NULL;
#endif    
}
/////////////////////////////////////////////////////////////////////////////////////
