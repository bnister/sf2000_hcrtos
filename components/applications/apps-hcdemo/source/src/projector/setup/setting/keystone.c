#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <hcuapi/dis.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include "../setup.h"
#include "../../screen.h"
#include "../../factory_setting.h"

lv_obj_t* keystone_scr;
//lv_group_t * keysone_g;
static  lv_obj_t* btnm1;

#define KEYSTONE_TOP          0
#define KEYSTONE_BOTTOM  1
#define KEYSTONE_STEP    8 // adjust step of display area_width
#define KEYSTONE_MIN_WIDTH   400

static int fd = -1;
static int16_t  width_top = 0, width_bottom = 0,max_width = 0;
static bool in_keystone = false;
extern lv_timer_t *timer_setting;
extern lv_obj_t *salve_scr_obj;
static int keystone_set(int dir, uint16_t step);
static void keystone_stop(void);
void set_keystone(int top_w, int bottom_w);

void set_keystone(int top_w, int bottom_w){
    if(top_w <=0 || bottom_w <= 0){
        return;
    }
	struct dis_keystone_param vhance;
	memset(&vhance, 0, sizeof(vhance));

	vhance.distype = DIS_TYPE_HD;

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type or config no distype para %d\n" , vhance.distype);
		return;
	}

    if(fd == -1)
    {
    	fd = open("/dev/dis" , O_RDWR);
    	if(fd < 0) {
    		return;
    	}
    	//vhance.info.width_down = vhance.info.width_up = dis_info.area.w;
	}
	
    vhance.info.enable = 1;
    vhance.info.bg_enable = 0;
     vhance.info.width_up = top_w;
    vhance.info.width_down = bottom_w;
    ioctl(fd , DIS_SET_KEYSTONE_PARAM , &vhance);
}

int keystone_set(int dir, uint16_t step)
{
	struct dis_screen_info dis_info;
	struct dis_keystone_param vhance;
	memset(&vhance, 0, sizeof(vhance));

	vhance.distype = DIS_TYPE_HD;

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type or config no distype para %d\n" , vhance.distype);
		return -1;
	}

    if(fd == -1)
    {
    	fd = open("/dev/dis" , O_RDWR);
    	if(fd < 0) {
    		return -1;
    	}
       dis_info.distype = DIS_TYPE_HD;
    	if(ioctl(fd, DIS_GET_SCREEN_INFO, &dis_info)!=0){
    	    perror("ioctl(get screeninfo)");
    	}
    	max_width = dis_info.area.w;
    	width_bottom = width_top = dis_info.area.w;
    	//vhance.info.width_down = vhance.info.width_up = dis_info.area.w;
    }else{
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
    if(fd >=0)
    {
        max_width = 0;
        width_top = 0;
        width_bottom = 0;
        close(fd);
        fd = -1;
    }
}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    //lv_obj_t *btn = lv_event_get_user_data(e);
    if(code == LV_EVENT_PRESSED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(obj);

        if(id == 0) //up
        {
            keystone_set(KEYSTONE_TOP, KEYSTONE_STEP);
        }
        else
        {
            keystone_set(KEYSTONE_BOTTOM, KEYSTONE_STEP);
        }
    }
    if(code == LV_EVENT_DRAW_PART_BEGIN)
    {
            lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
            lv_btnmatrix_t* btnm = (lv_btnmatrix_t*)btnm1;
            if(dsc->class_p == &lv_btnmatrix_class && dsc->type == LV_BTNMATRIX_DRAW_PART_BTN) {
                if(dsc->id == btnm->btn_id_sel) {
                       if(lv_btnmatrix_get_selected_btn(obj) == dsc->id) 
                            dsc->rect_dsc->bg_color = lv_palette_darken(LV_PALETTE_BLUE, 3);
                        else 
                            dsc->rect_dsc->bg_color = lv_palette_main(LV_PALETTE_BLUE);
                }
            }

    }
    if(code == LV_EVENT_KEY){
        lv_timer_pause(timer_setting);
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_HOME){
            in_keystone = false;
            lv_obj_del(obj);
            turn_to_setup_scr();
        }
        if(key == LV_KEY_ESC){
            in_keystone = false;
            //turn_to_main_scr();
            lv_obj_del(obj);
            turn_to_setup_scr();
            return;
        }
        lv_timer_reset(timer_setting);
        lv_timer_resume(timer_setting);
    }
}


static const char* btnm_map[] = { "+", "\n",
                                  "-", "",
};

void keystone_screen_init(lv_obj_t *btn)
{
    //keystone_scr = lv_obj_create(NULL);
    //keysone_g = lv_group_create();
    // lv_group_set_default(keysone_g );
    // lv_indev_set_group(indev_keypad, keysone_g);
    //lv_obj_set_style_bg_opa(slave_scr, LV_OPA_TRANSP, 0);
    in_keystone = true;
    lv_obj_set_style_border_opa(slave_scr, LV_OPA_TRANSP, 0);
    btnm1 = lv_btnmatrix_create(slave_scr);
    salve_scr_obj = btnm1;
    lv_btnmatrix_set_map(btnm1, btnm_map);
    lv_obj_set_style_text_font(btnm1, &lv_font_montserrat_26, 0);
    lv_obj_set_style_radius( btnm1,0,LV_PART_MAIN);
    lv_group_focus_obj(btnm1);
    lv_obj_set_style_bg_color(btnm1, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_outline_width(btnm1, 0, 0);

    lv_obj_align(btnm1, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btnm1, 200, 160);
    lv_obj_set_style_bg_color(btnm1, lv_palette_main(LV_PALETTE_GREY), LV_PART_ITEMS);

    lv_obj_add_event_cb(btnm1, event_handler, LV_EVENT_ALL, btn);
}
