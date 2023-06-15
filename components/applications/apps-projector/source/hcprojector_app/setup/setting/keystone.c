#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <hcuapi/dis.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/input-event-codes.h>
#include <stdlib.h>
#include <unistd.h>
#include "setup.h"
#include "screen.h"
#include "factory_setting.h"
#include "../../mul_lang_text.h"
#include "../../app_config.h"
#include "osd_com.h"

#ifdef PROJECTOR_VMOTOR_SUPPORT
 #define KEYSTONE_ADJUST_BAR_LEN 33 
 #else 
  #define KEYSTONE_ADJUST_BAR_LEN 11
 #endif

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define KEYSTONE_FONT lv_font_montserrat_10
    #define KEYSTONE_LEFT_WIDTH_PCT LV_SIZE_CONTENT
#else
    #define KEYSTONE_FONT lv_font_montserrat_40
    #define KEYSTONE_LEFT_WIDTH_PCT 23
#endif

#define KEYSTONE_TOP          0
#define KEYSTONE_BOTTOM  1
#define KEYSTONE_STEP    8 // adjust step of display area_width
#define KEYSTONE_MIN_WIDTH   400



lv_obj_t* keystone_scr;
//lv_group_t * keysone_g;
lv_obj_t* keystone_list;

static int fd = -1;
static int16_t  width_top = 0, width_bottom = 0,max_width = 0;
static bool in_keystone = false;
static lv_obj_t* cur_obj;

extern lv_timer_t *timer_setting;
extern lv_obj_t *slave_scr_obj;
extern lv_obj_t *tab_btns;
extern uint16_t tabv_act_id;
extern SCREEN_TYPE_E cur_scr;
static bool keystone_is_forbid = false;

static int keystone_set(int dir, uint16_t step);
static void keystone_stop(void);
void set_keystone(int top_w, int bottom_w);
void change_keystone();

void keystone_init(void)
{	
	struct dis_screen_info dis_info;
	struct dis_keystone_param vhance;
	memset(&vhance, 0, sizeof(vhance));
	if(fd < 0)
    {
    	fd = open("/dev/dis" , O_RDWR);
    	if(fd < 0) {
    		return ;
    	}
       	dis_info.distype = DIS_TYPE_HD;
    	if(ioctl(fd, DIS_GET_SCREEN_INFO, &dis_info)!=0){
    	    perror("ioctl(get screeninfo)");
    	}
    	max_width = dis_info.area.w;
    	width_bottom = width_top = dis_info.area.w;  
		if(0== projector_get_some_sys_param(P_KEYSTONE_TOP) || 0xff == projector_get_some_sys_param(P_KEYSTONE_TOP)){
			projector_set_some_sys_param(P_KEYSTONE_TOP, width_top);
		    projector_set_some_sys_param(P_KEYSTOME_BOTTOM, width_bottom);
		    projector_sys_param_save();
		}
    }
}
void set_keystone(int top_w, int bottom_w){
    if(top_w <=0 || bottom_w <= 0){
        return;
    }
	struct dis_keystone_param vhance;
	memset(&vhance, 0, sizeof(vhance));

	vhance.distype = DIS_TYPE_HD;

    if(fd == -1)
    {
    	fd = open("/dev/dis" , O_RDWR);
    	if(fd < 0) {
    		return;
    	}
	}
	
    vhance.info.enable = 1;
    vhance.info.bg_enable = 0;
    vhance.info.width_up = top_w;
    vhance.info.width_down = bottom_w;
    projector_set_some_sys_param(P_KEYSTONE_TOP, top_w);
    projector_set_some_sys_param(P_KEYSTOME_BOTTOM, bottom_w);
    projector_sys_param_save();
    ioctl(fd , DIS_SET_KEYSTONE_PARAM , &vhance);
}

int keystone_set(int dir, uint16_t step)
{
	struct dis_screen_info dis_info;
	struct dis_keystone_param vhance;
	memset(&vhance, 0, sizeof(vhance));

	vhance.distype = DIS_TYPE_HD;

 	{
        width_top = projector_get_some_sys_param(P_KEYSTONE_TOP);
        width_bottom = projector_get_some_sys_param(P_KEYSTOME_BOTTOM);
        max_width = width_top > width_bottom ? width_top : width_bottom;
    }
	
    vhance.info.enable = 1;
    vhance.info.bg_enable = 0;
    if(dir == KEYSTONE_TOP)
    {
        if(width_bottom  < max_width)
        {
            width_bottom  += step;
            width_bottom = width_bottom>max_width?max_width:width_bottom;
            width_top = max_width;
        }
        else
        {
            width_top -= step;  
            if(width_top < KEYSTONE_MIN_WIDTH)
                width_top  = KEYSTONE_MIN_WIDTH;
            width_bottom = max_width;
        }
    }
    else if(dir == KEYSTONE_BOTTOM)
    {
        if(width_top  < max_width)
        {
            width_top  += step;
            width_top = width_top>max_width?max_width:width_top;
            width_bottom = max_width;
        }
        else
        {
            width_bottom -= step;                  
            if(width_bottom < KEYSTONE_MIN_WIDTH)
                width_bottom = KEYSTONE_MIN_WIDTH;
            width_top = max_width;
        }
    }
    vhance.info.width_up = width_top;
    vhance.info.width_down = width_bottom;
    projector_set_some_sys_param(P_KEYSTONE_TOP, width_top);
    projector_set_some_sys_param(P_KEYSTOME_BOTTOM, width_bottom);
    projector_sys_param_save();
    printf("top width:  %d, bottom width: %d\n",(int)vhance.info.width_up, (int)vhance.info.width_down);
	ioctl(fd , DIS_SET_KEYSTONE_PARAM , &vhance);

	return 0;
}
static void keystone_stop(void)
{
    if(fd >0)
    {
        max_width = 0;
        width_top = 0;
        width_bottom = 0;
        close(fd);
        fd = -1;
    }
}

static int sel_id = 0;

static void event_handle(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    //lv_obj_t *btn = lv_event_get_user_data(e);
    if(code == LV_EVENT_PRESSED) {
        if(sel_id == 0) //up
        {
            keystone_set(KEYSTONE_TOP, KEYSTONE_STEP);
        }
        else if(sel_id==1){// reset keystone
            set_keystone(max_width, max_width);
            width_top = width_bottom = max_width;
        }
        else if(sel_id==2)
        {
            keystone_set(KEYSTONE_BOTTOM, KEYSTONE_STEP);
        }
		lv_group_focus_obj(lv_group_get_focused(lv_group_get_default()));
    }
    // if(code == LV_EVENT_DRAW_PART_BEGIN)
    // {
    //         lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    //         lv_btnmatrix_t* btnm = (lv_btnmatrix_t*)keystone_list;
    //         if(dsc->class_p == &lv_btnmatrix_class && dsc->type == LV_BTNMATRIX_DRAW_PART_BTN) {
    //             if(dsc->id == btnm->btn_id_sel) {
    //                    if(lv_btnmatrix_get_selected_btn(obj) == dsc->id) 
    //                         dsc->rect_dsc->bg_color = lv_palette_darken(LV_PALETTE_BLUE, 3);
    //                     else 
    //                         dsc->rect_dsc->bg_color = lv_palette_main(LV_PALETTE_BLUE);
    //             }
    //         }

    // }
    else if(code == LV_EVENT_KEY){
        if(timer_setting)
            lv_timer_pause(timer_setting);
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_HOME){
           
            // lv_obj_del(obj);
            //  slave_scr_obj = NULL;
            //  if(cur_scr == SCREEN_SETUP){
            //     turn_to_setup_scr();
            //  }else{
            //     lv_group_focus_obj(cur_obj);
            //     cur_obj = NULL;
            // }
            turn_to_main_scr();
            return;
            
        }else if(key == LV_KEY_ESC){
          
            turn_to_main_scr();
            // lv_obj_del(obj);
            // slave_scr_obj = NULL;
            // if(cur_scr == SCREEN_SETUP){
            //     turn_to_setup_scr();
            // }else{
            //     lv_group_focus_obj(cur_obj);
            //     cur_obj = NULL;
            // }
            
            return;
        }else if(key == LV_KEY_UP || key == LV_KEY_LEFT){
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            sel_id -= 1;
            if(sel_id<0){
                lv_group_focus_obj(tab_btns);
                return;
            }
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
        }else if(key == LV_KEY_DOWN || key == LV_KEY_RIGHT){
             lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            sel_id += 1;
            if(sel_id>=lv_obj_get_child_cnt(target)){
                lv_group_focus_next(lv_group_get_default());
                return;
            }
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);           
        }
    }else if(code == LV_EVENT_FOCUSED){
         if(act_key_code == KEY_UP){
            sel_id = lv_obj_get_child_cnt(target)-1;
        }else if(act_key_code == KEY_DOWN){
            sel_id = 0;
        }
        if(sel_id>-1)
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
    }else if(code == LV_EVENT_DEFOCUSED){
        
        if(sel_id<lv_obj_get_child_cnt(target))
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
        sel_id = 0;
    }
}


// static const char* btnm_map[] = { "+", "\n",
//                                   "-", "",
// };



void set_keystone_disable(bool en){
    keystone_is_forbid = en;
}

void keystone_screen_init(lv_obj_t *parent)
{
    //keystone_scr = lv_obj_create(NULL);
    //keysone_g = lv_group_create();
    // lv_group_set_default(keysone_g );
    // lv_indev_set_group(indev_keypad, keysone_g);
    //lv_obj_set_style_bg_opa(setup_slave_root, LV_OPA_TRANSP, 0);
    // if(slave_scr_obj || keystone_is_forbid){
    //     return;
    // }
    // lv_obj_clear_flag(setup_scr, LV_OBJ_FLAG_HIDDEN);
    // cur_obj = lv_group_get_focused(lv_group_get_default());
    // lv_obj_set_style_border_opa(setup_slave_root, LV_OPA_TRANSP, 0);
    // keystone_list = lv_btnmatrix_create(parent);
    // //slave_scr_obj = keystone_list;
    // lv_btnmatrix_set_map(keystone_list, btnm_map);
    // lv_obj_set_style_text_font(keystone_list, &lv_font_montserrat_26, 0);
    // lv_obj_set_style_radius( keystone_list,0,LV_PART_MAIN);
    // lv_group_focus_obj(keystone_list);
    // lv_obj_set_style_bg_color(keystone_list, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    // lv_obj_set_style_outline_width(keystone_list, 0, 0);

    // lv_obj_align(keystone_list, LV_ALIGN_CENTER, 0, 0);
    // lv_obj_set_size(keystone_list, LV_PCT(100), LV_PCT(22));
    // lv_obj_set_style_bg_color(keystone_list, lv_palette_main(LV_PALETTE_GREY), LV_PART_ITEMS);

    // 
    #ifdef PROJECTOR_VMOTOR_SUPPORT
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_set_size(obj, LV_PCT(85), LV_PCT(40));
    lv_obj_set_style_pad_left(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);

    lv_obj_t *label = lv_label_create(obj);
    lv_obj_set_size(label, LV_PCT(KEYSTONE_LEFT_WIDTH_PCT), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(label, 0, 0);
    // language_choose_add_label1(label,);
    // set_label_text1(label, projector_get_some_sys_param(P_OSD_LANGUAGE), NULL);
    set_label_text2(label,  STR_KEYSTONE, FONT_NORMAL);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);


    keystone_list = create_list_obj1(obj, 77, 100);
    #else
    keystone_list = create_list_obj1(parent, 77, 100);
    lv_obj_t *label;
    #endif

    create_list_sub_text_obj3(keystone_list,100, KEYSTONE_ADJUST_BAR_LEN, "+");
    lv_obj_set_style_text_font(lv_obj_get_child(keystone_list, 0), &KEYSTONE_FONT,0);
    label = lv_label_create(lv_obj_get_child(keystone_list, 0));
    
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //lv_obj_set_style_border_width(label, 2, 0);
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, "<");
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    label = lv_label_create(lv_obj_get_child(keystone_list, 0));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, ">");
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);



    create_list_sub_text_obj4(keystone_list, 100,  KEYSTONE_ADJUST_BAR_LEN, STR_RESET);
    //lv_obj_set_style_text_font(lv_obj_get_child(keystone_list, 1), &lv_font_montserrat_40,0);

    label = lv_label_create(lv_obj_get_child(keystone_list, 1));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, "<");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    label = lv_label_create(lv_obj_get_child(keystone_list, 1));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, ">");
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);



    create_list_sub_text_obj3(keystone_list,100,KEYSTONE_ADJUST_BAR_LEN , "-");
    lv_obj_set_style_text_font(lv_obj_get_child(keystone_list, 2), &KEYSTONE_FONT,0);

    label = lv_label_create(lv_obj_get_child(keystone_list, 2));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, "<");
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    label = lv_label_create(lv_obj_get_child(keystone_list, 2));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, ">");
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(lv_obj_get_child(keystone_list, 2), &KEYSTONE_FONT,0);
    lv_obj_add_event_cb(keystone_list, event_handle, LV_EVENT_ALL, NULL);
	keystone_init();
}



void change_keystone(){
#ifdef KEYSTONE_SUPPORT
     tabv_act_id = TAB_KEYSTONE;
#endif
    change_screen(SCREEN_SETUP);
}
