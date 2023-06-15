// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.0.5
// LVGL VERSION: 8.2
// PROJECT: ProjectorSetting

#ifndef _PROJECTORSETTING_UI_H
#define _PROJECTORSETTING_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include <hcuapi/dis.h>
#define PROJECTER_C2_VERSION 0
#include <generated/br2_autoconf.h>

#ifdef BR2_PACKAGE_HCCAST_USBMIRROR	
#define USBMIRROR_SUPPORT
#endif

#define HDMI_RX_FLIP_SMOTHLY    0  // 1:enable; 0: disable

typedef enum flip_mode{
	FLIP_MODE_NORMAL,
    FLIP_MODE_ROTATE_180, // µõ×°Ç°Í¶
    FLIP_MODE_H_MIRROR,  //×ÀÃæ±³Í¶
    FLIP_MODE_V_MIRROR,  // Í¶ÆÁÄ£Ê½
}flip_mode_e;
typedef enum SCREEN_TYPE{
	SCREEN_CHANNEL_CVBS,
	SCREEN_CHANNEL_HDMI,
	#if PROJECTER_C2_VERSION
	SCREEN_CHANNEL_HDMI2,
	#endif
	SCREEN_CHANNEL_MP,
	#ifdef USBMIRROR_SUPPORT	
	SCREEN_CHANNEL_USB_CAST,
	#endif	
	SCREEN_CHANNEL,
	SCREEN_SETUP,
	SCREEN_VOLUME,
	
	
}SCREEN_TYPE_E;


extern SCREEN_TYPE_E prev_scr;
extern lv_obj_t *channel_scr;  
extern lv_obj_t *setup_scr;
extern lv_obj_t *slave_scr;

extern lv_obj_t *volume_scr; 
extern lv_obj_t *volume_bar;
extern lv_obj_t *ui_mainpage; 
 extern lv_obj_t * ui_fspage;
 extern lv_obj_t * ui_subpage;
 extern lv_indev_t* indev_keypad;
extern lv_obj_t *hdmi_scr;  
extern lv_obj_t *cvbs_scr; 
extern lv_obj_t *ui_um_play;


extern lv_group_t *channel_g;
extern lv_group_t *setup_g;
extern lv_group_t *volume_g;
extern lv_group_t *mp_g;
extern lv_group_t *hdmi_g;
extern lv_group_t *cvbs_g;



void change_screen(enum SCREEN_TYPE stype);
void _ui_screen_change(lv_obj_t * target,  int spd, int delay);
int projector_lvgl_start(int argc, char **argv);
void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

int set_flip_mode(flip_mode_e mode);
void hdmi_rx_set_flip_mode(flip_mode_e flip_mode);
int  fb_show_onoff(int on);

extern void channel_screen_init(void);
extern void setup_screen_init(void);
extern void hdmi_screen_init(void);
extern void cvbs_screen_init(void);
extern void volume_screen_init(void);
extern void ui_mainpage_screen_init(void);
extern void ui_subpage_screen_init(void);
extern void ui_fspage_screen_init(void);
extern void set_aspect_ratio(dis_tv_mode_e ratio);
extern void set_zoom(int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h);
extern void ui_ctrl_bar_screen_init(void);
extern void medai_message_ctrl_process(void);
extern int media_mute(bool mute);
extern void ui_ebook_screen_init(void);
extern void del_volume();
extern void del_setup_slave_scr_obj();
extern void BT_first_power_on();
extern void app_set_i2so_gpio_mute(bool val);//0 mute 1 no mute
extern uint8_t set_next_flip_mode();
int hdmi_rx_enter(void);
extern int hdmi_rx_leave(void);
int cvbs_rx_start(void);
int cvbs_rx_stop(void);
void enter_standby(void);
void ui_wifi_cast_init(void);
void win_cast_control(void *arg1, void *arg2);
//void medai_message_ctrl_process(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
