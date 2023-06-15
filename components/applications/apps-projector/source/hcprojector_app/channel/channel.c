#include <stdio.h>

#include "screen.h"
#include "channel.h"
#include "setup.h"
#include "factory_setting.h"
#include <hcuapi/gpio.h>
#include "mul_lang_text.h"
#include "osd_com.h"
#include "include/com_api.h"
#include "app_config.h"

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define EN_FONT lv_font_montserrat_18
    #define INPUT_SOURCE_W 32
    #define INPUT_SOURCE_H 85
    #define FOOT_LEFT_PAD_PCT 5
#else 
    #define EN_FONT lv_font_montserrat_32
    #define INPUT_SOURCE_W 25
    #define INPUT_SOURCE_H 80
    #define FOOT_LEFT_PAD_PCT 17
#endif

static uint8_t prev_id = 0;
lv_obj_t* channel_scr = NULL;
lv_group_t *channel_g = NULL;
static lv_obj_t *body_btnmatrix;
#if PROJECTER_C2_VERSION
static int channels[] = {SCREEN_CHANNEL_CVBS, SCREEN_CHANNEL_HDMI, SCREEN_CHANNEL_HDMI2,SCREEN_CHANNEL_MP, SCREEN_CHANNEL_MAIN_PAGE};
#else


static int channels[] = {
#ifdef	CVBSIN_SUPPORT
	SCREEN_CHANNEL_CVBS, 
#endif	
#ifdef HDMIIN_SUPPORT
	SCREEN_CHANNEL_HDMI, 
#endif	
	SCREEN_CHANNEL_MP, 

#ifdef MAIN_PAGE_SUPPORT
	SCREEN_CHANNEL_MAIN_PAGE
#endif
};

#endif


#define CHANNEL_LEN  (sizeof(channels)/sizeof(channels[0]))


LV_IMG_DECLARE(MENU_IMG_LIST_OK);


static lv_timer_t * timer = NULL;

static void scr_event_handler(lv_event_t *e);
void turn_to_hdmi1(void);
#if PROJECTER_C2_VERSION
void turn_to_hdmi2(void);
#endif
void turn_to_cvbs(void);
void turn_to_mp(void);
void turn_to_main_page(void);

extern void set_change_lang(btnmatrix_p *p, int index, int v);


static void timer_handler(lv_timer_t* timer_){
#if 1
    change_screen(projector_get_some_sys_param(P_CUR_CHANNEL));
    timer = NULL;
#else
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
    default:
        break;
    }
#endif    

    printf("projector_get_some_sys_param=%d\n",projector_get_some_sys_param(P_CUR_CHANNEL));
    timer = NULL;
}


void turn_to_hdmi1(void)
{
    printf("turn_to_hdmi1\n");
    change_screen(SCREEN_CHANNEL_HDMI);
}

#if PROJECTER_C2_VERSION
void turn_to_hdmi2(void)
{

    change_screen(SCREEN_CHANNEL_HDMI2);

}
#endif

void turn_to_cvbs(void){
          
    change_screen(SCREEN_CHANNEL_CVBS);

}

void turn_to_mp(void){
    change_screen(SCREEN_CHANNEL_MP);
}

void turn_to_main_page(void){
    change_screen(SCREEN_CHANNEL_MAIN_PAGE);
}

static Channel_map channelMap[] = {
#ifdef CVBSIN_SUPPORT
        {.channel = CVBS_ID, .func = turn_to_cvbs},
#endif
#ifdef HDMIIN_SUPPORT
        {.channel = HDMI_ID, .func = turn_to_hdmi1},     
        #if PROJECTER_C2_VERSION
        {.channel = HDMI2_ID, .func = turn_to_hdmi2},
#endif
#endif		
        {.channel = MEDIA_ID, .func = turn_to_mp},
#ifdef MAIN_PAGE_SUPPORT
        {.channel = MAIN_PAGE_ID, .func = turn_to_main_page}
#endif
};

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    //printf(" event: %d\n", code);
    if (code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        dsc->rect_dsc->outline_width=0;

    }else if(code == LV_EVENT_KEY) {
        lv_timer_pause(timer);
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        uint8_t sel_id = lv_btnmatrix_get_selected_btn(target);
        if (key == LV_KEY_UP || key == LV_KEY_DOWN){
            if(sel_id == prev_id && (prev_id==0)){
                sel_id = CHANNEL_LEN-1;
                lv_btnmatrix_set_selected_btn(target, sel_id);
            }else if(sel_id >= CHANNEL_LEN){
                prev_id = 0;
                sel_id = 0;
                lv_btnmatrix_set_selected_btn(target, sel_id);
                
            }else{
                prev_id = sel_id;
            }
 
             lv_btnmatrix_set_btn_ctrl(target, sel_id, LV_BTNMATRIX_CTRL_CHECKED);

        }else if (key == LV_KEY_ENTER){
            projector_set_some_sys_param(P_CUR_CHANNEL, channels[sel_id]);

            channelMap[sel_id].func();
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
            if(prev_id == 0 && key == LV_KEY_LEFT){
                lv_btnmatrix_set_selected_btn(target, CHANNEL_LEN-1);
            }else{
                lv_btnmatrix_set_selected_btn(target, prev_id );
            }
        }
        lv_timer_resume(timer);
        lv_timer_reset(timer);
    }
    else if(code == LV_EVENT_FOCUSED) {
        prev_id = -1;
    }
}

static void scr_event_handler(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SCREEN_LOADED){
         key_set_group(channel_g);
        for(int i=0; i<CHANNEL_LEN;i++){
            if(channels[i] == projector_get_some_sys_param(P_CUR_CHANNEL)){
                lv_btnmatrix_set_selected_btn(body_btnmatrix, i);
                lv_btnmatrix_set_btn_ctrl(body_btnmatrix, i, LV_BTNMATRIX_CTRL_CHECKED);
                printf("i: %d\n", i);
            }
        }
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_0, 0);
        timer = lv_timer_create(timer_handler, 15000, 0);
        lv_timer_set_repeat_count(timer, 1);
        lv_timer_reset(timer);
    }
    else if(code == LV_EVENT_SCREEN_UNLOADED){
        if(timer){
            lv_timer_del(timer);
            timer = NULL;
        }
        projector_sys_param_save();
    }
}

#if PROJECTER_C2_VERSION
    static int btnm_vec[] = {STR_AV_TITLE, LINE_BREAK_STR, STR_HDMI_TITLE1, LINE_BREAK_STR,STR_HDMI_TITLE2, LINE_BREAK_STR, STR_MEDIA_TITLE, LINE_BREAK_STR, STR_HOME_TITLE, LINE_BREAK_STR,
    BLANK_SPACE_STR, LINE_BREAK_STR, BLANK_SPACE_STR, LINE_BREAK_STR,BLANK_SPACE_STR, LINE_BREAK_STR,BLANK_SPACE_STR, LINE_BREAK_STR,
    BLANK_SPACE_STR, BTNS_VEC_END};
#else    
    static int btnm_vec[] = {
#ifdef CVBSIN_SUPPORT    
    STR_AV_TITLE, LINE_BREAK_STR,
#endif    
#ifdef HDMIIN_SUPPORT
	STR_HDMI_TITLE, LINE_BREAK_STR,
#endif	
	STR_MEDIA_TITLE, LINE_BREAK_STR,
#ifdef  MAIN_PAGE_SUPPORT	
	STR_HOME_TITLE, LINE_BREAK_STR,
#endif
    BLANK_SPACE_STR, LINE_BREAK_STR,
    BLANK_SPACE_STR, LINE_BREAK_STR,
    BLANK_SPACE_STR, LINE_BREAK_STR,
    BLANK_SPACE_STR, LINE_BREAK_STR,
    BLANK_SPACE_STR, LINE_BREAK_STR,
    BLANK_SPACE_STR, BTNS_VEC_END};
#endif


lv_font_t select_font_channel[3];

void channel_screen_init(void)
{
    channel_scr = lv_obj_create(NULL);
    lv_obj_clear_flag(channel_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(channel_scr, LV_OPA_TRANSP, 0);
    //lv_obj_set_size(channel_scr, 1280, 720);
    lv_obj_set_style_bg_color(channel_scr, lv_palette_main(LV_PALETTE_BLUE_GREY) ,LV_PART_MAIN);
    //lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(channel_scr, scr_event_handler, LV_EVENT_SCREEN_LOADED, 0);
    lv_obj_add_event_cb(channel_scr, scr_event_handler, LV_EVENT_SCREEN_UNLOADED, 0);

    channel_g = lv_group_create();
    lv_group_t* g = lv_group_get_default();
    lv_group_set_default(channel_g);

    lv_obj_t* input_sour = lv_obj_create(channel_scr);
    //lv_obj_set_style_shadow_color(input_sour, lv_palette_darken(LV_PALETTE_GREY, 1), 0);

    //lv_obj_set_style_shadow_opa(input_sour, LV_OPA_50, 0);
    

    lv_obj_set_size(input_sour, LV_PCT(INPUT_SOURCE_W), LV_PCT(INPUT_SOURCE_H));
    lv_obj_align(input_sour, LV_ALIGN_TOP_LEFT, LV_PCT(100-INPUT_SOURCE_W), LV_PCT(3));
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
    lv_obj_center(label);
    //lv_label_set_recolor(label, true);
    //language_choose_add_label1(label, );
    //int id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    //set_label_text_with_font1(label, id, "#ffffff ", &select_font_channel[id]);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    set_label_text2(label, STR_INPUT_SOURCE, FONT_LARGE);
    

    body_btnmatrix = lv_btnmatrix_create(input_sour);

    static lv_style_t style_bg, style_item;

    INIT_STYLE_BG(&style_bg);
    lv_obj_add_style(body_btnmatrix, &style_bg, 0);
    INIT_STYLE_ITEM(&style_item);
    lv_obj_add_style(body_btnmatrix, &style_item, LV_PART_ITEMS);
    lv_obj_set_style_text_align(body_btnmatrix, LV_TEXT_ALIGN_CENTER, LV_PART_ITEMS);
    lv_obj_set_size(body_btnmatrix,LV_PCT(100),LV_PCT(77));
    lv_obj_set_style_bg_color(body_btnmatrix, lv_palette_darken(LV_PALETTE_GREY, 1), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(body_btnmatrix, lv_palette_darken(LV_PALETTE_YELLOW, 1), LV_BTNMATRIX_CTRL_CHECKED & LV_PART_ITEMS );
    lv_btnmatrix_set_btn_ctrl_all(body_btnmatrix, LV_BTNMATRIX_CTRL_CHECKABLE);

    lv_btnmatrix_set_one_checked(body_btnmatrix, true);
    lv_obj_set_style_radius(body_btnmatrix, 0, LV_PART_ITEMS);

    //btnmatrix_p *p = language_choose_add_label(body_btnmatrix, (char*)btnm_map, 20);
    //btnmatrix_p1 *p = language_choose_add_btns(body_btnmatrix, btnm_vec, 20);
    //set_change_lang(p, 2, 0);
   // set_btns_lang(body_btnmatrix, projector_get_some_sys_param(P_OSD_LANGUAGE));
   set_btns_lang2(body_btnmatrix, sizeof(btnm_vec)/sizeof(int), FONT_NORMAL, btnm_vec);
    //set_btnmatrix_language(body_btnmatrix, projector_get_some_sys_param(P_OSD_LANGUAGE));

    
    lv_group_focus_obj(body_btnmatrix);
    lv_btnmatrix_set_btn_ctrl(body_btnmatrix, projector_get_some_sys_param(P_CUR_CHANNEL), LV_BTNMATRIX_CTRL_CHECKED);
    lv_btnmatrix_set_selected_btn(body_btnmatrix, projector_get_some_sys_param(P_CUR_CHANNEL));
    prev_id = projector_get_some_sys_param(P_CUR_CHANNEL);
    lv_obj_add_event_cb(body_btnmatrix, event_handler, LV_EVENT_ALL, timer);

    lv_obj_t *foot = lv_obj_create(input_sour);

    lv_obj_set_size(foot,LV_PCT(100),LV_PCT(12));
    lv_obj_set_style_bg_color(foot, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_border_width(foot, 0, 0);
    lv_obj_set_style_outline_width(foot, 0, 0);
    lv_obj_set_style_radius(foot, 0, 0);
    lv_obj_set_style_shadow_width(foot, 0, 0);
    lv_obj_set_scrollbar_mode(foot, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* img = lv_img_create(foot);
    lv_obj_align(img, LV_ALIGN_LEFT_MID,LV_PCT(FOOT_LEFT_PAD_PCT),0);
    lv_img_set_src(img, &MENU_IMG_LIST_OK);

    label = lv_label_create(foot);
    lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_TOP, 5, -5);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    set_label_text2(label, STR_FOOT_SURE, FONT_LARGE);
     
    lv_group_set_default(g);
}
