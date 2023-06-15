
#include "app_config.h"

#include <stdio.h>
#include "setup.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "hcstring_id.h"
#include "main_page.h"
#include "com_api.h"
#include "osd_com.h"
#include "screen.h"
#include "../../app_config.h"
//#include "network_api.h"

#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif

lv_obj_t *btn_meida = NULL;
lv_obj_t *btn_setting = NULL;
lv_obj_t *btn_help = NULL;

//#define MAIN_PAGE_HELP
#ifdef BLUETOOTH_SUPPORT
    LV_IMG_DECLARE(bt_con);
    LV_IMG_DECLARE(bt_discon);
    lv_obj_t *bt_show;
#endif

#if defined(CAST_SUPPORT) || defined(WIFI_SUPPORT)
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_dhcpd.h>
#include "include/network_api.h"

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define WIFI_SHOW_FONT SiYuanHeiTi_Light_3500_12_1b
    #define WIFI_SHOW_IMG_GROUP 2
    #define WIFI_NAME_SHOW 0
#else
    #define WIFI_SHOW_FONT SiYuanHeiTi_Nor_7000_28_1b
    #define WIFI_SHOW_IMG_GROUP 2
    #define WIFI_NAME_SHOW 1
#endif

lv_obj_t *wifi_show;

lv_obj_t *btn_channel;
lv_obj_t *btn_wired;
lv_obj_t *btn_wireless;
lv_obj_t *btn_wifi;

extern lv_obj_t* wifi_lists;
extern hccast_wifi_ap_info_t cur_conne;
extern hccast_wifi_ap_info_t *cur_conne_p;
#ifdef MAIN_PAGE_HELP

lv_obj_t** btns[] = {&btn_channel, &btn_wired,  &btn_wifi ,&btn_wireless,&btn_meida, &btn_setting, &btn_help};
#define BTNS_SIZE 7
#else
lv_obj_t** btns[] = {&btn_channel, &btn_wired,  &btn_wifi ,&btn_wireless,&btn_meida, &btn_setting};
#define BTNS_SIZE 6
#endif

LV_IMG_DECLARE(wifi4);
LV_IMG_DECLARE(wifi3);
LV_IMG_DECLARE(wifi2);
LV_IMG_DECLARE(wifi1);
LV_IMG_DECLARE(wifi0);
LV_IMG_DECLARE(wifi_no);

// LV_IMG_DECLARE(input_source_icon);
// LV_IMG_DECLARE(internet_icon);
// LV_IMG_DECLARE(media_icon);
// LV_IMG_DECALRE(message_icon);
// LV_IMG_DECALRE(setting_icon);
// LV_IMG_DECLARE(wifi_cast_icon);

//lv_font_t **icons[] = {};
extern void set_wifi_had_con_by_cast(bool b);
extern bool wifi_is_conning_get();
#else
lv_obj_t *btn_hdmi = NULL;
lv_obj_t *btn_av = NULL;
#ifdef MAIN_PAGE_HELP
lv_obj_t** btns[] = {&btn_hdmi, &btn_av, &btn_meida, &btn_setting, &btn_help};
#define BTNS_SIZE 5
#else
lv_obj_t** btns[] = {&btn_hdmi, &btn_av, &btn_meida, &btn_setting};
#define BTNS_SIZE 4
#endif

#endif




//LV_FONT_DECLARE(font40_china);
lv_obj_t *main_page_scr = NULL;
lv_group_t *main_page_g = NULL;
extern lv_font_t *select_font_normal[3];
lv_font_t *main_page_font[3];



static void event_handle(lv_event_t *e);
void main_page_init();

static void event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if(code == LV_EVENT_KEY){
        uint8_t key = lv_indev_get_key(lv_indev_get_act());
        int index = lv_obj_get_index(target);
        if(key == LV_KEY_LEFT){
            #ifdef MAIN_PAGE_HELP
            index = index-2>=0 ? (index==BTNS_SIZE-1 ? index-1 : index-2) : (index==1 ? BTNS_SIZE-3 : BTNS_SIZE-1);
            #else
            index = index == 0 ? BTNS_SIZE-1 : index == 1 ? BTNS_SIZE-2 : index-2;
            #endif
            lv_group_focus_obj(*btns[index]);           
        }else if(key == LV_KEY_RIGHT){
            #ifdef MAIN_PAGE_HELP
            index = index+2<BTNS_SIZE ? (index==BTNS_SIZE-3 ?1 : index+2) : (index+1<BTNS_SIZE ? index+1 : 0);
            #else
            index = index == BTNS_SIZE-2 ? 1 : BTNS_SIZE-1 == index ? 0 : index+2;
            #endif
            lv_group_focus_obj(*btns[index]);            
        }else if(key == LV_KEY_ENTER){
           if(target == btn_meida){
                change_screen(SCREEN_CHANNEL_MP);
                projector_set_some_sys_param(P_CUR_CHANNEL, SCREEN_CHANNEL_MP);
            }else if(target == btn_setting){
                change_screen(SCREEN_SETUP);
            }
        #ifdef CAST_SUPPORT
            else if(target == btn_wifi){				
#ifdef WIFI_SUPPORT
                change_screen(SCREEN_WIFI);
#endif
            }
            else if(target == btn_wired){
#ifdef USBMIRROR_SUPPORT	
                change_screen(SCREEN_CHANNEL_USB_CAST);
#endif
            }
            else if(target == btn_wireless){
#ifdef WIFI_SUPPORT
                if(network_wifi_module_get() && projector_get_some_sys_param(P_WIFI_ONOFF)){
                    change_screen(SCREEN_CHANNEL_WIFI_CAST);
                }else{
                    win_msgbox_msg_open(STR_WIFI_NOT_CONNECT, 3000, NULL, NULL);
                }
#endif
            }else if(target == btn_channel){
                change_screen(SCREEN_CHANNEL);
            }
        #else
            else if(target == btn_hdmi){
#ifdef HDMIIN_SUPPORT				
                change_screen(SCREEN_CHANNEL_HDMI);
                projector_set_some_sys_param(P_CUR_CHANNEL, SCREEN_CHANNEL_HDMI);
#endif				
            } else if(target == btn_av){
#ifdef CVBSIN_SUPPORT
                change_screen(SCREEN_CHANNEL_CVBS);
                projector_set_some_sys_param(P_CUR_CHANNEL, SCREEN_CHANNEL_CVBS);
#endif				
            }
        #endif
            projector_sys_param_save();
        }else if(key == LV_KEY_UP || key == LV_KEY_DOWN){
            if(index % 2 == 1){
                lv_group_focus_obj(*btns[index-1]);
            }else if(index<BTNS_SIZE-1){
                lv_group_focus_obj(*btns[index+1]);
            }
        }
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        if(target == lv_group_get_focused(lv_group_get_default())){
            dsc->rect_dsc->outline_color = lv_color_white();
            dsc->rect_dsc->outline_width = 4;
            dsc->rect_dsc->outline_pad = 0;

        }else{
             dsc->rect_dsc->outline_width = 0;
        }
        dsc->rect_dsc->shadow_width = 0;
    }else if(code == LV_EVENT_SCREEN_LOADED){
     
    }
}

static void mainpage_scr_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SCREEN_LOADED){
        projector_set_some_sys_param(P_CUR_CHANNEL, SCREEN_CHANNEL_MAIN_PAGE);
        lv_group_set_default(main_page_g);
    }else if(code == LV_EVENT_SCREEN_UNLOADED){
        win_msgbox_msg_close();
    }
}


#if defined(CAST_SUPPORT) && defined(WIFI_SUPPORT)
static void wifi_show_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if(code == LV_EVENT_REFRESH){
        char *cur_ssid = app_get_connecting_ssid();
		
		if(!obj->user_data || !cur_ssid){
			lv_img_set_src(lv_obj_get_child(obj, 0), &wifi_no);
			if(lv_obj_get_child_cnt(obj)>1){
				lv_label_set_text(lv_obj_get_child(obj, 1), "");
			}			
			return;
		}
        if(lv_obj_get_child_cnt(obj)>1){
            lv_label_set_text(lv_obj_get_child(obj, 1), cur_ssid);
        }
        hccast_wifi_ap_info_t * p = sysdata_get_wifi_info(cur_ssid);
        if(!p){
            return;
        }
		int quality = p->quality;
		void *img = (quality<=100 && quality>=80) ? &wifi4 :
					(quality<80 && quality>=60) ? &wifi3 :
					(quality<60 && quality>=40) ? &wifi2 :
					(quality<40 && quality>=20) ? &wifi1: &wifi0;
		lv_img_set_src(lv_obj_get_child(obj, 0), img);
    }
}
#endif

#ifdef BLUETOOTH_SUPPORT
static void bt_show_event_handle(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if(code == LV_EVENT_REFRESH){
        char *cur_bt_name = projector_get_bt_name();
		if(!obj->user_data || !cur_bt_name){
			lv_img_set_src(lv_obj_get_child(obj, 0), &bt_discon);
			if(lv_obj_get_child_cnt(obj)>1){
				lv_label_set_text(lv_obj_get_child(obj, 1), "");
			}			
			return;
		}
		
        if(lv_obj_get_child_cnt(obj)>1){
            lv_label_set_text(lv_obj_get_child(obj, 1), cur_bt_name);
        }
		lv_img_set_src(lv_obj_get_child(obj, 0), &bt_con);
    }
}

#endif

static void win_main_page_control(void *arg1, void *arg2){
    (void)arg2;
     control_msg_t *ctl_msg = (control_msg_t*)arg1;

    switch (ctl_msg->msg_type){
    #if defined(CAST_SUPPORT) && defined(WIFI_SUPPORT)
        case MSG_TYPE_NETWORK_WIFI_CONNECTED:
            main_page_prompt_status_set(MAIN_PAGE_PROMPT_WIFI, MAIN_PAGE_PROMPT_STATUS_ON);
            lv_event_send(wifi_show, LV_EVENT_REFRESH, NULL);
            break;
        case MSG_TYPE_NETWORK_WIFI_DISCONNECTED:
        case MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL:
            main_page_prompt_status_set(MAIN_PAGE_PROMPT_WIFI, MAIN_PAGE_PROMPT_STATUS_OFF);
            lv_event_send(wifi_show, LV_EVENT_REFRESH, NULL);
            break;
    #endif
    #ifdef BLUETOOTH_SUPPORT   
        case MSG_TYPE_BT_CONNECTED:
            main_page_prompt_status_set(MAIN_PAGE_PROMOT_BT,MAIN_PAGE_PROMPT_STATUS_ON);
            lv_event_send(bt_show, LV_EVENT_REFRESH, NULL);
            break;
        case MSG_TYPE_BT_DISCONNECTED:
             main_page_prompt_status_set(MAIN_PAGE_PROMOT_BT,MAIN_PAGE_PROMPT_STATUS_OFF);
             lv_event_send(bt_show, LV_EVENT_REFRESH, NULL);
             break;
    #endif
        default:
            break;
    }
}

static void scr_event_handle(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
	char *cur_ssid = NULL;
    if(code == LV_EVENT_SCREEN_LOADED){
        key_set_group(main_page_g);
        #if defined(CAST_SUPPORT) && defined(WIFI_SUPPORT)
            if(!network_wifi_module_get()){
                return;
            }
            if(app_wifi_connect_status_get()){
                main_page_prompt_status_set(MAIN_PAGE_PROMPT_WIFI, MAIN_PAGE_PROMPT_STATUS_ON);
            }else{
                main_page_prompt_status_set(MAIN_PAGE_PROMPT_WIFI, MAIN_PAGE_PROMPT_STATUS_OFF);
            }
            lv_event_send(wifi_show, LV_EVENT_REFRESH, NULL);
        #endif
            #ifdef BLUETOOTH_SUPPORT
                if(app_bt_is_connected()){
                    main_page_prompt_status_set(MAIN_PAGE_PROMOT_BT,MAIN_PAGE_PROMPT_STATUS_ON);
                }else{
                    main_page_prompt_status_set(MAIN_PAGE_PROMOT_BT,MAIN_PAGE_PROMPT_STATUS_OFF);
                }
                lv_event_send(bt_show, LV_EVENT_REFRESH, NULL);
            #endif      
    }
}


static lv_obj_t* main_page_prompt_create(lv_obj_t* parent,void* icon){
	lv_obj_t* show = lv_obj_create(parent);
    //
    lv_obj_set_size(show, LV_PCT(20),LV_PCT(100));
    lv_obj_set_style_border_width(show, 1, 0);
    lv_obj_set_style_pad_hor(show, 0, 0);

    lv_obj_set_scrollbar_mode(show, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(show, LV_OPA_0, 0);
    lv_obj_set_style_text_color(show, lv_color_white(), 0);
    //lv_obj_align(show, LV_ALIGN_TOP_MID, lv_pct(38), 20);
    lv_obj_set_style_border_width(show, 0, 0);
    lv_obj_set_style_pad_all(show, 0, 0);

    lv_obj_set_flex_flow(show, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(show, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *img = lv_img_create(show);
    lv_img_set_src(img, icon);
    #if WIFI_NAME_SHOW
        lv_obj_set_flex_grow(img, 2);
        lv_obj_t *label = lv_label_create(show);
        lv_label_set_text(label, "");
        lv_obj_set_flex_grow(label, 17);
        lv_obj_set_style_text_font(label, &WIFI_SHOW_FONT, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    #endif
	
	return show;
}

void main_page_prompt_status_set(main_page_prompt_t t,main_page_prompt_v  v){
	switch (t){
		case MAIN_PAGE_PROMPT_WIFI:
            #if defined(CAST_SUPPORT) && defined(WIFI_SUPPORT)
			wifi_show->user_data = (void*)v;
            #endif
			break;
		case MAIN_PAGE_PROMOT_BT:
            #ifdef BLUETOOTH_SUPPORT
			    bt_show->user_data = (void*)v;
            #endif
			break;
        default:
            break;
	}
}

void main_page_init(){
    main_page_scr = lv_obj_create(NULL);
    main_page_g = lv_group_create();
    lv_group_t* d_g = lv_group_get_default();
    lv_group_set_default(main_page_g);
    lv_obj_add_event_cb(main_page_scr, scr_event_handle, LV_EVENT_ALL, 0);


    screen_entry_t main_page_entry;
    main_page_entry.screen = main_page_scr;
    main_page_entry.control = win_main_page_control;
    api_screen_regist_ctrl_handle(&main_page_entry);

    int h = lv_disp_get_hor_res(NULL);
    int v = lv_disp_get_ver_res(NULL);
    printf("h is: %d, v is: %d\n", h, v);

    lv_obj_t *grid = lv_obj_create(main_page_scr);

    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_ver(grid, v/6,0);
    lv_obj_set_style_pad_hor(grid, h/9, 0);

    lv_obj_set_style_bg_color(grid, lv_color_make(81, 100, 117), 0);
    lv_obj_center(grid);
    lv_obj_set_size(grid, h, v);
    lv_obj_set_style_radius(grid, 0, 0);
    
    lv_obj_add_event_cb(main_page_scr, mainpage_scr_event_handle, LV_EVENT_ALL, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_COLUMN_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    uint16_t col_num = BTNS_SIZE/2+(BTNS_SIZE%2);
    uint16_t sub_width =  (h/9*7)/col_num;//根据数量计算宽度
    uint16_t gap_col = sub_width/7*col_num/(col_num-1);
    sub_width = sub_width*6/7;

    uint16_t sub_height = (v/3*2)/2-v/36-1;
    uint16_t gap_row = v/18;

    lv_obj_set_style_pad_row(grid, gap_row, 0);
    lv_obj_set_style_pad_column(grid, gap_col, 0);


    #if defined(CAST_SUPPORT) || defined(WIFI_SUPPORT)
        #ifdef MAIN_PAGE_HELP
        int r[] =  {133,91,81,60,50,255 ,0};
        int g[] = {105,75,125,113,139, 201 ,64};
        int b[] = {54,112,60,128,137,14,64};
        int text_ids[] = {STR_INPUT_SOURCE,STR_WIRED_TITLE,STR_WIFI_TITLE,STR_WIRELESS_TITLE, STR_MEDIA_TITLE,STR_SETTING_TITLE,STR_HELP_TITLE};
        #else
        int r[] =  {133,91,81,60,50,255};
        int g[] = {105,75,125,113,139, 201};
        int b[] = {54,112,60,128,137,14};
        int text_ids[] = {STR_INPUT_SOURCE,STR_WIRED_TITLE,STR_WIFI_TITLE,STR_WIRELESS_TITLE, STR_MEDIA_TITLE,STR_SETTING_TITLE};   
        #endif
    #else
        #ifdef MAIN_PAGE_HELP
        int r[] =  {133,91,81,60,50};
        int g[] = {105,75,125,113,139};
        int b[] = {54,112,60,128,137};
        int text_ids[] = {STR_HDMI_TITLE, STR_AV_TITLE, STR_MEDIA_TITLE, STR_SETTING_TITLE, STR_HELP_TITLE};
        #else
        int r[] =  {133,91,81,60};
        int g[] = {105,75,125,113};
        int b[] = {54,112,60,128};
        int text_ids[] = {STR_HDMI_TITLE, STR_AV_TITLE, STR_MEDIA_TITLE, STR_SETTING_TITLE};
        #endif
    #endif

    lv_obj_t *label;
    lv_obj_t *img;
    for(int i=0; i<BTNS_SIZE; i++){
        *btns[i] = lv_btn_create(grid);
        lv_obj_set_flex_flow(*btns[i], LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(*btns[i], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_add_event_cb(*btns[i], event_handle, LV_EVENT_ALL, 0);
        if(i== BTNS_SIZE-1 && BTNS_SIZE%2!=0){
            lv_obj_set_size(*btns[i], sub_width, sub_height*2+v/18);
        }else{
            lv_obj_set_size(*btns[i], sub_width, sub_height);
        }

        label = lv_label_create(*btns[i]);
        lv_obj_center(label);  
        lv_obj_set_style_bg_color(*btns[i], lv_color_make(r[i], g[i], b[i]), 0);
        set_label_text2(label, text_ids[i], FONT_NORMAL);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    }
	
	lv_obj_t* prompt_show = lv_obj_create(main_page_scr);
	lv_obj_set_size(prompt_show, lv_pct(35), lv_pct(12));
	lv_obj_set_style_bg_opa(prompt_show, LV_OPA_0, 0);
	lv_obj_align(prompt_show, LV_ALIGN_TOP_RIGHT, 0, 10);
	lv_obj_set_style_pad_all(prompt_show, 0, 0);
	lv_obj_set_style_border_width(prompt_show, 0, 0);
	lv_obj_set_scrollbar_mode(prompt_show, LV_SCROLLBAR_MODE_OFF);
	//lv_obj_set_flex_flow(prompt_show, LV_FLEX_FLOW_ROW);
	//lv_obj_set_flex_align(prompt_show, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t col_dsc[] = {LV_GRID_FR(3), LV_GRID_FR(7), LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(prompt_show, col_dsc, row_dsc);
   

#if defined(CAST_SUPPORT) && defined(WIFI_SUPPORT)

	wifi_show = main_page_prompt_create(prompt_show, &wifi_no);
    lv_obj_set_grid_cell(wifi_show, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
	lv_obj_add_event_cb(wifi_show, wifi_show_event_handle, LV_EVENT_ALL, 0);
#endif

#ifdef BLUETOOTH_SUPPORT
	bt_show = main_page_prompt_create(prompt_show, &bt_discon);
    lv_obj_add_event_cb(bt_show, bt_show_event_handle, LV_EVENT_ALL, 0);
    lv_obj_set_grid_cell(bt_show, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
#endif

    //usb_show = main_page_prompt_create(prompt_show, &wifi_no);
    //lv_obj_set_grid_cell(sub, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_group_set_default(d_g);
}
