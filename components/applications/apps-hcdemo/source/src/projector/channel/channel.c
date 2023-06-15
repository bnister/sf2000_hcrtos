#include <stdio.h>

#include <generated/br2_autoconf.h>

#include "../screen.h"
#include "channel.h"
#include "../setup/setup.h"
#include "../setup/setup_helper.h"
#include "../factory_setting.h"

#include <hcuapi/gpio.h>

#if PROJECTER_C2_VERSION
#define HDMI_RX_SWITCH_GPIO 2
#else
#define HDMI_RX_SWITCH_GPIO PINPAD_INVALID
#endif

//temp use, HC15XX do not support CVBS_IN, HDMI_IN, PQ
#ifdef  CONFIG_SOC_HC15XX
#define ONLY_SUPPORT_MEDIA_PLAYER
#endif


static const char* input_source = "Input Source\0输入源\0Input Source";
static const char* foot_sure = "Enter\0确定\0Enter";

#ifdef ONLY_SUPPORT_MEDIA_PLAYER
  #define CHANNEL_LEN 1
  static const char* btnm_map[] = {
                                 "MEDIA", "\n", 
                                 " ", "\n", 
                                 " ", "\n",
                                 " ", "\n", 
                                 " ", "\n", 
                                 " ", "\n", 
                                 " ","\n", 
                                 " ","\n", 
                                 " ","\n", 
                                 " ", "",
                                };
#else
  #if PROJECTER_C2_VERSION
      #define CHANNEL_LEN 4
      static const char* btnm_map[] = {
                                 "CVBS", "\n", 
                                 "HDMI1", "\n", 
                                 "HDMI2", "\n", 
                                 "MEDIA", "\n",
                                 " ", "\n", 
                                 " ", "\n", 
                                 " ", "\n", 
                                 " ","\n", 
                                 " ","\n", 
                                 " ","\n", 
                                 " ", "",
                                };
  #else
      #define CHANNEL_LEN 4
      static const char* btnm_map[] = {
                                 "CVBS", "\n", 
                                 "HDMI", "\n", 
                                 "MEDIA", "\n",
                                 "USBMIRROR", "\n", 
                                 " ", "\n", 
                                 " ", "\n", 
                                 " ","\n", 
                                 " ","\n", 
                                 " ","\n", 
                                 " ", "",
                                };
	#endif
#endif


lv_obj_t* channel_scr = NULL;
lv_group_t *channel_g = NULL;

LV_IMG_DECLARE(MENU_IMG_LIST_OK);


static lv_timer_t * timer = NULL;

static void turn_to_hdmi(void);
static void turn_to_hdmi1(void);
#if PROJECTER_C2_VERSION
static void turn_to_hdmi2(void);
#endif
static void turn_to_cvbs(void);
static void turn_to_mp(void);
static void turn_to_usb_cast(void);


static void timer_handler(lv_timer_t* timer_){
    switch (projector_get_some_sys_param(P_CUR_CHANNEL))
    {
    case SCREEN_CHANNEL_MP:
       turn_to_mp();
        break;
    case SCREEN_CHANNEL_CVBS:
       turn_to_cvbs();
        break;
    case SCREEN_CHANNEL_HDMI:
        turn_to_hdmi1();
        break;
#if PROJECTER_C2_VERSION
    case SCREEN_CHANNEL_HDMI2:
        turn_to_hdmi2();
        break;
#endif
#ifdef USBMIRROR_SUPPORT
	case SCREEN_CHANNEL_USB_CAST:
		turn_to_usb_cast();
		break;
#endif

    default:
        break;
    }
    printf("projector_get_some_sys_param=%d\n",projector_get_some_sys_param(P_CUR_CHANNEL));
    timer = NULL;
}


static void turn_to_hdmi(void){
    cvbs_rx_stop();
    hdmi_rx_enter();
    change_screen(SCREEN_CHANNEL_HDMI);
    _ui_screen_change(hdmi_scr,0,0);
    fb_show_onoff(0);
}

static void turn_to_hdmi1(void)
{
    printf("turn_to_hdmi1\n");
    gpio_configure(HDMI_RX_SWITCH_GPIO, GPIO_DIR_OUTPUT);
	gpio_set_output(HDMI_RX_SWITCH_GPIO, 1);
    cvbs_rx_stop();
    hdmi_rx_enter();
    change_screen(SCREEN_CHANNEL_HDMI);
    _ui_screen_change(hdmi_scr,0,0);
    fb_show_onoff(0);
}

#if PROJECTER_C2_VERSION
static void turn_to_hdmi2(void)
{
    printf("turn_to_hdmi2\n");
    gpio_configure(HDMI_RX_SWITCH_GPIO, GPIO_DIR_OUTPUT);
	gpio_set_output(HDMI_RX_SWITCH_GPIO, 0);
    cvbs_rx_stop();
    hdmi_rx_enter();
    change_screen(SCREEN_CHANNEL_HDMI2);
    _ui_screen_change(hdmi_scr,0,0);
    fb_show_onoff(0);
}
#endif

static void turn_to_cvbs(void){
    hdmi_rx_leave();

    cvbs_rx_start();            
    change_screen(SCREEN_CHANNEL_CVBS);
    _ui_screen_change(cvbs_scr,0,0);
}

static void turn_to_mp(void){
    cvbs_rx_stop();

    //local_mp_enter();            

    hdmi_rx_leave();         

    change_screen(SCREEN_CHANNEL_MP);
}

#ifdef USBMIRROR_SUPPORT
static void turn_to_usb_cast(void)
{
    cvbs_rx_stop();
    hdmi_rx_leave();         

    change_screen(SCREEN_CHANNEL_USB_CAST);
}
#endif 

static Channel_map channelMap[] = {
        {.channel = CVBS, .func = turn_to_cvbs},
        {.channel = HDMI, .func = turn_to_hdmi1},
        #if PROJECTER_C2_VERSION
        {.channel = HDMI2, .func = turn_to_hdmi2},
        #endif
        {.channel = MEDIA, .func = turn_to_mp},
        #ifdef USBMIRROR_SUPPORT
		{.channel = USBMIRROR, .func = turn_to_usb_cast},
        #endif
};

#ifdef ONLY_SUPPORT_MEDIA_PLAYER
static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    //printf(" event: %d\n", code);

   if(code == LV_EVENT_KEY) {
        lv_timer_pause(timer);

        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        
        if (key == LV_KEY_UP || key == LV_KEY_DOWN){
            lv_btnmatrix_set_selected_btn(target, 0);
            lv_btnmatrix_set_btn_ctrl(target, 0, LV_BTNMATRIX_CTRL_CHECKED);
        }else if (key == LV_KEY_ENTER){
            //channelMap[MEDIA].func();
			turn_to_mp();
            if (timer){
                lv_timer_del(timer);
                timer = NULL;
            }
            return;
        } else if(key == LV_KEY_ESC){
            lv_timer_ready(timer);
            lv_timer_resume(timer);
            return;
        } else if(key == LV_KEY_LEFT || key == LV_KEY_RIGHT){
            // lv_btnmatrix_set_selected_btn(target, index);
            // printf("%s(), line:%d, set id:%d\n", __func__, __LINE__, index);
        }
        lv_timer_resume(timer);
        lv_timer_reset(timer);
    }
    else if(code == LV_EVENT_FOCUSED) {
        // int id = lv_btnmatrix_get_selected_btn(target);
        // const char * txt = lv_btnmatrix_get_btn_text(target, id);
        // printf("%s %d was focused\n", txt, id);
    }else if(code == LV_EVENT_SCREEN_LOADED){
         if(timer){
            lv_timer_reset(timer);
            lv_timer_resume(timer);
            return;
        }
        lv_obj_set_style_bg_opa(slave_scr, LV_OPA_0, 0);
        timer = lv_timer_create(timer_handler, 15000, 0);
        lv_timer_set_repeat_count(timer, 1);
        lv_timer_reset(timer);
    }
    else if(code == LV_EVENT_SCREEN_UNLOADED){
        if(timer){
            lv_timer_pause(timer);
        }
        projector_sys_param_save();
    }else if (code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        dsc->rect_dsc->outline_width=0;
    }
}

#else

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    //printf(" event: %d\n", code);
   if(code == LV_EVENT_KEY) {
        lv_timer_pause(timer);
        int index = projector_get_some_sys_param(P_CUR_CHANNEL);
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_UP || key == LV_KEY_DOWN){
            key == LV_KEY_UP && --index < 0 && (index = CHANNEL_LEN-1);
            key == LV_KEY_DOWN &&  ++index >= CHANNEL_LEN && (index = 0);

            lv_btnmatrix_set_selected_btn(target, index);
            lv_btnmatrix_set_btn_ctrl(target, index, LV_BTNMATRIX_CTRL_CHECKED);
            projector_set_some_sys_param(P_CUR_CHANNEL, index);
        }else if (key == LV_KEY_ENTER){
            channelMap[index].func();
            if (timer){
                lv_timer_del(timer);
                timer = NULL;
            }
            return;
        } else if(key == LV_KEY_ESC){
            lv_timer_ready(timer);
            lv_timer_resume(timer);
            return;
        } else if(key == LV_KEY_LEFT || key == LV_KEY_RIGHT){
            lv_btnmatrix_set_selected_btn(target, index);
        }
        lv_timer_resume(timer);
        lv_timer_reset(timer);
    }
    else if(code == LV_EVENT_FOCUSED) {
        int id = lv_btnmatrix_get_selected_btn(target);
        const char * txt = lv_btnmatrix_get_btn_text(target, id);
        printf("%s %d was focused\n", txt, id);
    }else if(code == LV_EVENT_SCREEN_LOADED){
         if(timer){
            lv_timer_reset(timer);
            lv_timer_resume(timer);
            return;
        }
        lv_obj_set_style_bg_opa(slave_scr, LV_OPA_0, 0);
        timer = lv_timer_create(timer_handler, 15000, 0);
        lv_timer_set_repeat_count(timer, 1);
        lv_timer_reset(timer);
    }
    else if(code == LV_EVENT_SCREEN_UNLOADED){
        if(timer){
            lv_timer_pause(timer);
        }
        projector_sys_param_save();
    }else if (code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        dsc->rect_dsc->outline_width=0;
    }
}

#endif


LV_FONT_DECLARE(myFont1);
LV_FONT_DECLARE(font32_china);

lv_font_t select_font_channel[3];

void channel_screen_init(void)
{
    select_font_channel[0] = lv_font_montserrat_32;
    select_font_channel[1] = font32_china;
    select_font_channel[2] = myFont1;
    channel_scr = lv_obj_create(NULL);
    lv_obj_clear_flag(channel_scr, LV_OBJ_FLAG_SCROLLABLE);
    //lv_obj_set_size(channel_scr, 1280, 720);
    lv_obj_set_style_bg_color(channel_scr, lv_palette_main(LV_PALETTE_BLUE_GREY) ,LV_PART_MAIN);
    //lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(channel_scr, event_handler, LV_EVENT_SCREEN_LOADED, 0);
    lv_obj_add_event_cb(channel_scr, event_handler, LV_EVENT_SCREEN_UNLOADED, 0);

    channel_g = lv_group_create();
    lv_group_t* g = lv_group_get_default();
    lv_group_set_default(channel_g);

    lv_obj_t* input_sour = lv_obj_create(channel_scr);
    //lv_obj_set_style_shadow_color(input_sour, lv_palette_darken(LV_PALETTE_GREY, 1), 0);

    //lv_obj_set_style_shadow_opa(input_sour, LV_OPA_50, 0);
    

    lv_obj_set_size(input_sour, lv_disp_get_hor_res(NULL)/4, lv_disp_get_ver_res(NULL)/5*4);
    lv_obj_align(input_sour, LV_ALIGN_TOP_LEFT, LV_PCT(70), LV_PCT(3));
    lv_obj_set_style_pad_all(input_sour, 0, 0);
    lv_obj_set_style_border_width(input_sour, 0, 0);
    lv_obj_set_style_pad_gap(input_sour, 0, 0);
    lv_obj_set_style_radius(input_sour, 0, 0);
    lv_obj_set_flex_flow(input_sour, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(input_sour, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *head = lv_obj_create(input_sour);
    lv_obj_set_size(head,LV_PCT(100),LV_PCT(12));
    lv_obj_set_style_radius(head, 0, 0);
    lv_obj_set_style_border_width(head, 0, 0);
    lv_obj_set_style_outline_width(head, 0, 0);
    lv_obj_set_style_bg_color(head, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_scrollbar_mode(head, LV_SCROLLBAR_MODE_OFF);


    lv_obj_t *label = lv_label_create(head);
    lv_label_set_recolor(label, true);
    language_choose_add_label(label, input_source, 0);
    int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    set_label_text_with_font(label, id, "#ffffff ", &select_font_channel[id]);
   
    lv_obj_center(label);

    lv_obj_t *body_btnmatrtix = lv_btnmatrix_create(input_sour);

    static lv_style_t style_bg, style_item;

    INIT_STYLE_BG(&style_bg);
    lv_obj_add_style(body_btnmatrtix, &style_bg, 0);
    INIT_STYLE_ITEM(&style_item);
    lv_obj_add_style(body_btnmatrtix, &style_item, LV_PART_ITEMS);
    lv_obj_set_size(body_btnmatrtix,LV_PCT(100),LV_PCT(80));
    lv_obj_set_style_bg_color(body_btnmatrtix, lv_palette_darken(LV_PALETTE_GREY, 1), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(body_btnmatrtix, lv_palette_darken(LV_PALETTE_YELLOW, 1), LV_BTNMATRIX_CTRL_CHECKED & LV_PART_ITEMS );
    lv_btnmatrix_set_btn_ctrl_all(body_btnmatrtix, LV_BTNMATRIX_CTRL_CHECKABLE);

    lv_btnmatrix_set_one_checked(body_btnmatrtix, true);
    lv_obj_set_style_radius(body_btnmatrtix, 0, LV_PART_ITEMS);
    lv_btnmatrix_set_map(body_btnmatrtix, btnm_map);
     lv_obj_set_style_text_font(body_btnmatrtix, &lv_font_montserrat_26, 0);
    lv_group_focus_obj(body_btnmatrtix);

#ifdef ONLY_SUPPORT_MEDIA_PLAYER
    lv_btnmatrix_set_btn_ctrl(body_btnmatrtix, 0, LV_BTNMATRIX_CTRL_CHECKED);
    lv_btnmatrix_set_selected_btn(body_btnmatrtix, 0);
#else    
    lv_btnmatrix_set_btn_ctrl(body_btnmatrtix, projector_get_some_sys_param(P_CUR_CHANNEL), LV_BTNMATRIX_CTRL_CHECKED);
    lv_btnmatrix_set_selected_btn(body_btnmatrtix, projector_get_some_sys_param(P_CUR_CHANNEL));
#endif    
    lv_obj_add_event_cb(body_btnmatrtix, event_handler, LV_EVENT_ALL, timer);

    lv_obj_t *foot = lv_obj_create(input_sour);

    lv_obj_set_size(foot,LV_PCT(100),LV_PCT(9));
    lv_obj_set_style_bg_color(foot, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_border_width(foot, 0, 0);
    lv_obj_set_style_outline_width(foot, 0, 0);
    lv_obj_set_style_radius(foot, 0, 0);
    lv_obj_set_style_shadow_width(foot, 0, 0);
    lv_obj_set_scrollbar_mode(foot, LV_SCROLLBAR_MODE_OFF);

    label = lv_label_create(foot);
      lv_obj_set_style_border_width(label, 0, 0);
    lv_obj_set_style_outline_width(label, 0, 0);
    lv_label_set_text(label, " ");
    lv_obj_center(label);
    lv_obj_t* img = lv_img_create(label);
    lv_obj_align(img, LV_ALIGN_LEFT_MID,0,0);
    lv_img_set_src(img, &MENU_IMG_LIST_OK);
    label = lv_label_create(label);
    lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_TOP, 5, 0);
    lv_label_set_recolor(label, true);
    
    language_choose_add_label(label, foot_sure, 0);
    set_label_text_with_font(label, id, "#ffffff ", &select_font_channel[id]);
     lv_group_set_default(g);
}
