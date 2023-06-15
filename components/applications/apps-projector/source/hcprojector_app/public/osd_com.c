#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "lvgl/lvgl.h"
#include "lvgl/src/font/lv_font.h"
#include "com_api.h"
#include "osd_com.h"

#ifdef __linux__
#include <linux/input.h>
#else
#include <hcuapi/input.h>
#endif
#include "setup.h"

#include "key_event.h"
//#include "local_mp_ui.h"
//#include "setup.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "app_config.h"
#include "../channel/local_mp/mp_preview.h"

//#include "ui_rsc/lan_str/lan_str_chn.h"
extern void set_label_text2(lv_obj_t* obj, uint16_t label_id, uint16_t font_id);

lv_style_t m_text_style_mid_normal;
lv_style_t m_text_style_mid_high;
lv_style_t m_text_style_mid_disable;
lv_style_t m_text_style_left_normal;
lv_style_t m_text_style_left_high;
lv_style_t m_text_style_left_disable;
lv_style_t m_text_style_right_normal;
lv_style_t m_text_style_right_high;
lv_style_t m_text_style_right_disable;

LV_IMG_DECLARE(IDB_Icon_unsupported);

static language_type_t m_language = LANGUAGE_ENGLISH;
static lv_obj_t *m_obj_msg = NULL;
static lv_timer_t *msgbox_timer = NULL;
static msg_timeout_func m_msg_timeout_func = NULL;
static void *m_timeout_func_data = NULL;
char *eng_string[] = {
	"IPTV",
	"Miracast",
	"Airplay",
	"DLNA",
	"Media player",
	"Wirecast",
	"Configuration",

	"Configuration",
	"Language setting",
	"Wifi Setting",
	"OSD setting",
	"Software upgrade",
	"Factory Setting",
	"Software information",

	"Video",
	"Music",
	"Photo",
};
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
        lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, 0);
        //lv_obj_set_style_bg_color(parent, LV_COLOR_CHROMA_KEY, 0);
    }
    else {
         lv_obj_set_style_bg_color(parent, COLOR_FRENCH_GREY, 0);
    }
    lv_obj_set_pos(parent, 0, 0);
    lv_obj_set_size(parent,LV_PCT(100),LV_PCT(100));
    lv_obj_set_style_border_opa(parent, LV_OPA_TRANSP, 0);
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
// printf("%s(), line:%d, depth:%d, item_count:%d, new_pos:%d\n", __FUNCTION__, __LINE__, \
//     depth, item_count, new_pos);    

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

// printf("%s(), line:%d, list: top:%d, cur_pos:%d, new_pos:%d\n", __FUNCTION__, __LINE__, \
//     list_ctrl->top, list_ctrl->cur_pos, list_ctrl->new_pos);    

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

    // printf("%d: shift = %d,top=%d,point=%d\n", __LINE__, shift,top,point);
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
        // printf("%d: shift = %d,top=%d,point=%d\n", __LINE__, shift,top,point);

        /* Moving in current page only.*/
        if(top == shift_top && point < list_ctrl->count)
        {
            // printf("%d: shift = %d,top=%d,point=%d\n", __LINE__, shift,top,point);
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
                
                printf("%d: page moving=%ld: top=%d, point = %d\n", __LINE__, page_moving, top, point);
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

    // printf("%d: shift = %d,top=%d,point=%d\n", __LINE__, shift,top,point);


    *new_pos  = point;
    *new_top    = top;


    return true;

}

bool osd_list_ctrl_update(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos)
{
    uint16_t old_top;
    uint16_t new_position = 0;
    uint16_t new_top;
    int16_t step = VKEY_NULL;
    bool ret;

    old_top = list_ctrl->top;

    switch (vkey)
    {
    case V_KEY_UP:
        step = -4;
        break;
    case V_KEY_DOWN:
        step = 4;
        break;
    case V_KEY_P_UP:
        step = -list_ctrl->depth;
        break;
    case V_KEY_P_DOWN:
        step = list_ctrl->depth;
        break;
    case V_KEY_LEFT:
        step = -1;
        break;
    case V_KEY_RIGHT:
        step = 1;
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


bool osd_list_ctrl_update2(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos)
{
    uint16_t old_top;
    uint16_t new_position = 0;
    uint16_t new_top;
    int16_t step = VKEY_NULL;
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
			printf("3333\n");
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

        printf("3 pos:%d, start:%d, end:%d, count:%d, list_count:%d, jump_cnt:%d\n", list_ctrl->pos, 
            list_ctrl->start, list_ctrl->end, count, list_ctrl->list_count, jump_cnt);

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
// Message box function for message
static pthread_mutex_t m_mutex_msgbox = PTHREAD_MUTEX_INITIALIZER;

static void msgbox_msg_clear()
{
    if (msgbox_timer){
        lv_timer_pause(msgbox_timer);
        lv_timer_del(msgbox_timer);
    }
    msgbox_timer = NULL;

    if (m_obj_msg && lv_obj_is_valid(m_obj_msg))
    {
        lv_obj_del(m_obj_msg);
        printf("obj_add :%p",m_obj_msg);
    }
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

static lv_obj_t * ui_tip_img;
static lv_obj_t * ui_tip_lab;
extern void language_choose_add_label1(lv_obj_t* obj, uint32_t label_id);
extern void set_label_text_with_font1(lv_obj_t * label,int  id,char* color,  lv_font_t* font);
extern lv_font_t select_font_mplist[3];
void win_msgbox_msg_open(int str_msg_id, uint32_t timeout, msg_timeout_func timeout_func, void *user_data)
{
    if(lv_obj_is_valid(m_obj_msg))
        return;    
    if (!m_obj_msg || !lv_obj_is_valid(m_obj_msg)){
        m_obj_msg = lv_obj_create(lv_scr_act());
        lv_obj_set_size(m_obj_msg,LV_PCT(40),LV_PCT(35));
        lv_obj_set_x(m_obj_msg, 0);
        lv_obj_set_y(m_obj_msg, MSGBOX_Y_OFS);
        lv_obj_set_align(m_obj_msg, LV_ALIGN_CENTER);
        lv_obj_clear_flag(m_obj_msg, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(m_obj_msg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(m_obj_msg, lv_color_hex(0x4D72E0), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(m_obj_msg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    
        // ui_tip_img
        ui_tip_img = lv_img_create(m_obj_msg);
        lv_obj_set_width(ui_tip_img, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_tip_img, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_tip_img, 0);
        lv_obj_set_y(ui_tip_img, 0);
        lv_img_set_src(ui_tip_img,&IDB_Icon_unsupported);
        lv_obj_set_align(ui_tip_img, LV_ALIGN_CENTER);
        lv_obj_add_flag(ui_tip_img, LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_clear_flag(ui_tip_img, LV_OBJ_FLAG_SCROLLABLE);

        // ui_tip_lab
        ui_tip_lab = lv_label_create(m_obj_msg);
        lv_obj_set_width(ui_tip_lab, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_tip_lab, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_tip_lab, 0);
        lv_obj_set_y(ui_tip_lab, 0);
        lv_obj_set_align(ui_tip_lab, LV_ALIGN_BOTTOM_MID);
        lv_label_set_long_mode(ui_tip_lab,LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_color(ui_tip_lab, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(ui_tip_lab, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(ui_tip_lab, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (0 != timeout && INVALID_VALUE_32 != timeout){
            msgbox_timer = lv_timer_create(msgbox_timer_cb, timeout, NULL);
        }

    }
	if (msgbox_timer)
    {
		lv_timer_reset(msgbox_timer);
	    lv_timer_set_period(msgbox_timer, timeout);
	}

    lv_label_set_text(ui_tip_lab,(char *)api_rsc_string_get(str_msg_id));
    lv_obj_set_style_text_font(ui_tip_lab, osd_font_get(FONT_MID), 0);        


    if (timeout_func){
        pthread_mutex_lock(&m_mutex_msgbox);
        m_msg_timeout_func = timeout_func;
        m_timeout_func_data = user_data;
        pthread_mutex_unlock(&m_mutex_msgbox);
    }else{
        m_msg_timeout_func = NULL;
        m_timeout_func_data = NULL;

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

void win_data_buffing_open(void *parent)
{

#ifdef WIN_BUFFERING_UI_WITH_BG
    if (m_obj_buffering)
        lv_obj_del(m_obj_buffering);
    if (NULL == parent)
        m_obj_buffering = lv_obj_create(lv_scr_act());
    else
        m_obj_buffering = lv_obj_create((lv_obj_t*)parent);

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
    if (NULL == parent)
        m_arc_buffering = lv_arc_create(lv_scr_act());
    else
        m_arc_buffering = lv_arc_create((lv_obj_t*)parent);
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


static lv_obj_t *m_msgbox_obj = NULL;
static user_msgbox_cb msgbox_func = NULL;
static void *msgbox_user_data = NULL;
static void win_msgbox_msg_btn_event_cb(lv_event_t *e){

    control_msg_t ctl_msg = {0};  
    int btn_sel = -1;  

    lv_obj_t * obj = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if(code == LV_EVENT_KEY){

        printf("obj:0x%x, target:0x%x\n", (unsigned int)obj, (unsigned int)target);     
        
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            if(lv_msgbox_get_active_btn(obj) == 0){
                btn_sel = MSGBOX_OK;
            } else{
                btn_sel = MSGBOX_CANCEL;
            }
        }
        else if(key == LV_KEY_ESC){
            btn_sel = MSGBOX_CANCEL;
        }

        if (btn_sel != -1){
            lv_obj_del(target->parent);
            if (msgbox_func)
                msgbox_func(btn_sel, msgbox_user_data);
            m_msgbox_obj = NULL;
        }
    }
    
}

/*
extern lv_font_t* select_font[3];
extern const char* get_some_language_str(const char *str, int index);
void win_msgbox_btn_open(lv_obj_t* parent, char *str_msg, user_msgbox_cb cb, void *user_data)
{
    lv_obj_t *obj_root;

    if (m_msgbox_obj)
        lv_obj_del(m_msgbox_obj);

    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    static const char * btns[3];
    btns[0] = get_some_language_str("Ok\0è·?¨\0Ok", id);
    btns[1] = get_some_language_str("Close\01?±?\0Close", id);
    btns[2] = "";
    //static const char* mbox1_str = "Ok?\0è·?¨￡?\0Ok?";
    
    obj_root = parent ? parent : lv_scr_act();
    m_msgbox_obj = lv_msgbox_create(obj_root, "", get_some_language_str(str_msg, id), btns, false);

    lv_obj_center(m_msgbox_obj);
    lv_obj_set_size(m_msgbox_obj, LV_PCT(30),LV_PCT(30));

    lv_obj_t *label = lv_msgbox_get_content(m_msgbox_obj);
    lv_obj_set_style_text_font(label,select_font[id], 0);

    lv_obj_t *btns_obj = lv_msgbox_get_btns(m_msgbox_obj);
     lv_obj_set_style_text_font(btns_obj, select_font[id], 0);
    
    if (cb){
        msgbox_func = cb;
        if (user_data)
            msgbox_user_data = user_data;
        else
            msgbox_user_data = NULL;
    }else{
        msgbox_func = NULL;
        msgbox_user_data = NULL;
    }

    lv_obj_set_style_bg_color(m_msgbox_obj, lv_palette_darken(LV_PALETTE_GREY, 1), 0);

    lv_obj_add_event_cb(m_msgbox_obj, win_msgbox_msg_btn_event_cb, LV_EVENT_ALL,NULL);

    lv_group_focus_obj(lv_msgbox_get_btns(m_msgbox_obj));
}
*/

/**
 * @description: transform x pixel in diff coood
 * @param {int} x : x pixel in LV_DSIP_W coood
 * @return {*}return piexl in cur lv_disp_coood
 * @author: Yanisin 
 */
int lv_xpixel_transform(int x)
{
    int temp_width=lv_disp_get_hor_res(lv_disp_get_default());
    int x_pixel=x*temp_width/DEFAULT_LV_DISP_W;
    return x_pixel;
}
int lv_ypixel_transform(int y)
{
    int temp_hight=lv_disp_get_ver_res(lv_disp_get_default());
    int y_pixel=y*temp_hight/DEFAULT_LV_DISP_H;
    return y_pixel;
}

#ifdef LVGL_MBOX_STANDBY_SUPPORT
static lv_obj_t* standby_mbox1 = NULL;
static lv_group_t* pre_group=NULL;
static lv_obj_t* prev_obj = NULL;
static int standby_count = 0;

#if PROJECTER_C1_VERSION
static lv_obj_t *btnmatrix_standby = NULL;
void win_del_lvmbox_standby(void)
{
	standby_count = 0;
    // lv_event_send(standby_mbox1, LV_EVENT_KEY, 0);
	printf("%s %d\n",__func__,__LINE__);
    if(btnmatrix_standby != NULL)
    {
        lv_obj_del(btnmatrix_standby);
        btnmatrix_standby = NULL;
    }

    if(standby_mbox1!=NULL)
	{
		lv_obj_del(standby_mbox1);
        standby_mbox1 = NULL;
	}
}

static void win_open_lvmbox_standby_cb(lv_event_t *e){
	lv_obj_t * obj = lv_event_get_current_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *target = lv_event_get_target(e);
	if(code == LV_EVENT_KEY){
		uint16_t key = lv_indev_get_key(lv_indev_get_act());
		if(key == LV_KEY_ENTER){
			if(lv_btnmatrix_get_selected_btn(obj) == 0){
				enter_standby();
			}
			else{
                win_del_lvmbox_standby();
				if(lv_obj_is_valid(prev_obj)){
					lv_group_set_default(pre_group);
					lv_indev_set_group(indev_keypad, pre_group);
					lv_group_focus_obj(prev_obj);
				}
			}
		}
		else if(key == V_KEY_POWER)
		{
			standby_count++;
			if(standby_count > 1)
				enter_standby();
		}
		else if(key == LV_KEY_UP || key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_LEFT){
		}
		else
		{
            win_del_lvmbox_standby();
			if(lv_obj_is_valid(prev_obj)){
				lv_group_set_default(pre_group);
				lv_indev_set_group(indev_keypad, pre_group);
				lv_group_focus_obj(prev_obj);
			}
		}
	}
}

void win_open_lvmbox_standby(void)
{
    if(standby_mbox1 == NULL&&btnmatrix_standby == NULL)
    {
		preview_reset();
		static const char * btn_txts[3];
		btn_txts[0] = api_rsc_string_get(STR_RESTORE_OK_1);
		btn_txts[1] = api_rsc_string_get(STR_RESTORE_CLOSE);
		btn_txts[2] = "";
		standby_mbox1 = lv_msgbox_create(lv_layer_top(), api_rsc_string_get(STR_ENTER_STANDBY), NULL, NULL, false);
		lv_obj_t *mbox_txt = lv_msgbox_get_title(standby_mbox1);
		lv_obj_set_style_text_font(mbox_txt, osd_font_get(FONT_MID), 0);
		lv_obj_align(standby_mbox1,LV_ALIGN_CENTER,0,-20);
		lv_obj_set_size(standby_mbox1,250,130);
		lv_obj_set_style_bg_color(standby_mbox1, lv_palette_darken(LV_PALETTE_GREY, 2) , 0);
        // lv_obj_add_event_cb(standby_mbox1, win_open_lvmbox_standby_cb, LV_EVENT_ALL,NULL);
        if(btn_txts) {
			btnmatrix_standby = lv_btnmatrix_create(lv_layer_top());
			lv_btnmatrix_set_map(btnmatrix_standby, btn_txts);
			lv_btnmatrix_set_btn_ctrl_all(btnmatrix_standby, LV_BTNMATRIX_CTRL_CLICK_TRIG | LV_BTNMATRIX_CTRL_NO_REPEAT);
			// lv_obj_remove_style_all(btnmatrix_standby);
			lv_obj_set_style_bg_opa(btnmatrix_standby, LV_OPA_COVER,0);
			uint32_t btn_cnt = 0;
			while(btn_txts[btn_cnt] && btn_txts[btn_cnt][0] != '\0') {
			    btn_cnt++;
			}
			lv_obj_set_style_border_width(btnmatrix_standby, 0, 0 | LV_STATE_FOCUS_KEY);
			lv_obj_set_style_outline_width(btnmatrix_standby, 0, 0 | LV_STATE_FOCUS_KEY);
			lv_obj_set_style_outline_color(btnmatrix_standby,lv_color_make(28, 119, 209),0);
			lv_obj_set_style_bg_color(btnmatrix_standby, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
			lv_obj_set_style_bg_color(btnmatrix_standby, lv_palette_darken(LV_PALETTE_GREY, 2), LV_STATE_FOCUS_KEY);
			lv_obj_set_style_bg_color(btnmatrix_standby, lv_color_make(224, 224, 224) ,LV_STATE_FOCUS_KEY | LV_PART_ITEMS);
			lv_obj_set_style_text_color(btnmatrix_standby, lv_color_make(0, 0, 0),LV_STATE_FOCUS_KEY | LV_PART_ITEMS);
			lv_obj_set_style_bg_color(btnmatrix_standby, lv_color_make(99, 119, 135) , LV_STATE_DEFAULT | LV_PART_ITEMS);
			lv_obj_set_style_text_color(btnmatrix_standby, lv_color_make(28, 119, 209) , LV_STATE_DEFAULT | LV_PART_ITEMS);
			lv_obj_set_style_text_font(btnmatrix_standby, osd_font_get(FONT_MID), LV_PART_ITEMS);
			const lv_font_t * font = lv_obj_get_style_text_font(btnmatrix_standby, LV_PART_ITEMS);
			lv_coord_t btn_h = lv_font_get_line_height(font) + LV_DPI_DEF / 10;
			printf("font = %d\n",btn_h);
			lv_obj_set_size(btnmatrix_standby, btn_cnt * (2 * LV_DPI_DEF / 3) + 10, btn_h+ 25);
			lv_obj_set_style_max_width(btnmatrix_standby, lv_pct(100), LV_PART_ITEMS);
			// lv_obj_add_flag(btnmatrix_standby, LV_OBJ_FLAG_EVENT_BUBBLE);    /*To see the event directly on the message box*/
			lv_obj_center(btnmatrix_standby);
			lv_obj_add_event_cb(btnmatrix_standby, win_open_lvmbox_standby_cb, LV_EVENT_ALL,NULL);
			lv_group_focus_obj(btnmatrix_standby);
			// lv_obj_set_style_bg_color(btnmatrix_standby, lv_color_hex(0x666666) , 0);
        }
    }
}
#else
void win_del_lvmbox_standby(void)
{
	if(standby_mbox1!=NULL)
	{
		standby_count = 0;
		printf("%s %d\n",__func__,__LINE__);
		lv_msgbox_close(standby_mbox1);
		standby_mbox1 = NULL;
	}
}
static void win_open_lvmbox_standby_cb(lv_event_t *e){
	lv_obj_t * obj = lv_event_get_current_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *target = lv_event_get_target(e);
	if(code == LV_EVENT_KEY){
		uint16_t key = lv_indev_get_key(lv_indev_get_act());
		if(key == LV_KEY_ENTER){
			if(lv_msgbox_get_active_btn(obj) == 0){
				enter_standby();
			}
			else{
				// lv_obj_del(target->parent);
				// turn_to_setup_scr();
				lv_msgbox_close(obj);
				if(lv_obj_is_valid(prev_obj)){
					lv_group_set_default(pre_group);
					lv_indev_set_group(indev_keypad, pre_group);
					lv_group_focus_obj(prev_obj);
				}
				standby_mbox1 = NULL;
				standby_count = 0;
			}
		}
		else if(key == V_KEY_POWER)
		{
			standby_count++;
			if(standby_count > 1)
				enter_standby();
		}
		else if(key == LV_KEY_UP || key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_LEFT){
		}
		else
		{
			// lv_obj_del(target->parent);
			// turn_to_setup_scr();
			lv_msgbox_close(obj);
			if(lv_obj_is_valid(prev_obj)){
				lv_group_set_default(pre_group);
				lv_indev_set_group(indev_keypad, pre_group);
				lv_group_focus_obj(prev_obj);
			}
			standby_mbox1 = NULL;
			standby_count = 0;
		}
	}
}

void win_open_lvmbox_standby(void){
    if(standby_mbox1 == NULL)
    {
		preview_reset();
        int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
        static const char * btns[3];
        btns[0] = api_rsc_string_get(STR_RESTORE_OK_1);
        btns[1] = api_rsc_string_get(STR_RESTORE_CLOSE);
        btns[2] = "";

        pre_group = lv_group_get_default();
        prev_obj = lv_group_get_focused(pre_group);

        // lv_obj_t *standby_mbox1
        standby_mbox1 = lv_msgbox_create(lv_layer_top(), "", api_rsc_string_get(STR_ENTER_STANDBY), btns, false);
        // slave_scr_obj = standby_mbox1;
        // standby_obj = standby_mbox1;

        // lv_msgbox_start_auto_close(standby_mbox1, 1000);
        lv_obj_t *label = lv_msgbox_get_content(standby_mbox1);
        lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);

        lv_obj_t *btns_obj = lv_msgbox_get_btns(standby_mbox1);
        lv_obj_set_style_text_font(btns_obj, osd_font_get(FONT_MID), 0);

#ifndef LVGL_RESOLUTION_240P_SUPPORT
		const lv_font_t * font = lv_obj_get_style_text_font(btns_obj, LV_PART_ITEMS);
		lv_coord_t btn_h = lv_font_get_line_height(font) + LV_DPI_DEF / 10;
		lv_obj_set_height(btns_obj, btn_h);
#endif

        lv_obj_set_style_bg_color(standby_mbox1, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
        lv_obj_add_event_cb(standby_mbox1,win_open_lvmbox_standby_cb,LV_EVENT_ALL,NULL);
        lv_obj_center(standby_mbox1);
        lv_group_focus_obj(lv_msgbox_get_btns(standby_mbox1));
    }
}
#endif
#endif

#if 1
/**
 * @description: unit the font in  all screen 
 * @return {*}
 * @author: Yanisin
 */
static lv_font_t *osd_font_english[] = 
{
    &ENG_BIG,
    &ENG_MID,
    &ENG_NORMAL,
    &ENG_SMALL
};

static lv_font_t *osd_font_chn[] = 
{
    &CHN_LARGE,  
    &CHN_MID,    
    &CHN_NORMAL,   
    &CHN_SMALL 
};

static lv_font_t *osd_font_french[]=
{
	// &french_18,
    // &french_14,
    // &french_12,
    // &french_10
};
static lv_font_t **osd_font_array[] = 
{
    osd_font_english,
    osd_font_chn,
    osd_font_french,
};

/**
 * @description: get diff font size in the same language
 * @param {Font_size_e} font_idx    input a font size  as FOTN_LARGE,FONT_NORMAL ....
 * @return {*} diff size of fonts
 * @author: Yanisin
 */
lv_font_t *osd_font_get(int font_idx)
{
    lv_font_t **font;
    int lang_id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    font = osd_font_array[lang_id];
    return  font[font_idx];
}

lv_font_t *osd_font_get_by_langid(int lang_id, int font_idx)
{
    lv_font_t **font;
    //int lang_id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    font = osd_font_array[lang_id];
    return  font[font_idx];
}

static void label_text_set_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t i = (uint32_t)obj->user_data;
    uint32_t label_id = GET_LABEL_ID(i);
    uint32_t font_id = GET_FONT_ID(i);
   
    // lv_label_set_text(obj,mul_langs_text[label_id][projector_get_some_sys_param(P_OSD_LANGUAGE)]);
    if(label_id == STR_NONE){
        lv_label_set_text(obj, " ");
    }else{
        lv_label_set_text(obj,(char *)api_rsc_string_get(label_id));
        lv_obj_set_style_text_font(obj, osd_font_get(font_id), 0);        
    }

}

void set_label_text2(lv_obj_t* obj, uint16_t label_id, uint16_t font_id){
    obj->user_data = (void*) STORE_LABEL_AND_FONT_ID(label_id, font_id);
    lv_obj_add_event_cb(obj, label_text_set_event_handle, LV_EVENT_REFRESH,NULL);
    lv_event_send(obj, LV_EVENT_REFRESH, NULL);
}

#endif