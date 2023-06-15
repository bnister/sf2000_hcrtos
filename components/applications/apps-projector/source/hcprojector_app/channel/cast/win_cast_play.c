
#include "app_config.h"

#if defined(AIRCAST_SUPPORT) || defined(MIRACAST_SUPPORT)

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
//#include <lvgl/lvgl.h>
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include "mul_lang_text.h"

#ifdef __HCRTOS__
#include <miracast/miracast_api.h>
#else
#include <hccast/miracast_api.h>
#endif

#include <hcuapi/input.h>
#include <hcuapi/input-event-codes.h>

#include "screen.h"
#include "setup.h"
//#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
#include "osd_com.h"
#include "win_cast_root.h"
#include "factory_setting.h"


lv_obj_t *ui_cast_play = NULL;
static lv_group_t *m_cast_play_group = NULL;

static volatile bool m_win_cast_play_open = false;
static void event_handler(lv_event_t * e);
static int win_cast_play_open(void *arg);
static int win_cast_play_close(void *arg);

static void event_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (ta == ui_cast_play){
        if(event == LV_EVENT_SCREEN_LOAD_START) {
            win_cast_play_open(NULL);
        } else if (event == LV_EVENT_SCREEN_UNLOAD_START) {
            win_cast_play_close(NULL);
        } else if (event == LV_EVENT_KEY) {
            lv_indev_t *key_indev = lv_indev_get_act();
            if (key_indev->proc.state == LV_INDEV_STATE_PRESSED)
            {
                uint32_t value = lv_indev_get_key(key_indev);
                if (value == LV_KEY_ESC)
                {
                    win_exit_to_cast_root_by_key_set(true);
                    
                    _ui_screen_change(ui_wifi_cast_root,0,0);
                    
                    //change_screen(SCREEN_CHANNEL);
                }
                else if (value == FUNC_KEY_SCREEN_ROTATE){
                    win_cast_mirror_rotate_switch();
                }

            }

        }
    }
}

static bool win_cast_play_wait_open(uint32_t timeout)
{
    uint32_t count;
    count = timeout/20;

    while(count--){
        if (m_win_cast_play_open)
            break;
        api_sleep_ms(20);
    }
    printf("%s(), m_win_cast_play_open(%d):%d\n", __func__, (int)m_win_cast_play_open, (int)count);
    return m_win_cast_play_open; 
}


static uint32_t m_hotkey[] = {KEY_POWER, KEY_VOLUMEUP, \
                    KEY_VOLUMEDOWN, KEY_MUTE, KEY_ROTATE_DISPLAY, KEY_FLIP,KEY_CAMERA_FOCUS,KEY_FORWARD,KEY_BACK,KEY_HOME};
static int win_cast_play_open(void *arg)
{
    (void)arg;
    uint32_t cast_type = win_cast_play_param();
	win_cast_set_m_stop_service_exit(true);
    printf("%s(): %s!\n", __func__, cast_type == MSG_TYPE_CAST_AIRMIRROR_START ? "Air mirror!" : "Miracast");    

    api_hotkey_enable_set(m_hotkey, sizeof(m_hotkey)/sizeof(m_hotkey[0]));

    m_cast_play_group = lv_group_create();
    key_set_group(m_cast_play_group);

    //lv_obj_add_event_cb(ui_cast_play, event_handler, LV_EVENT_ALL, NULL);    
    lv_group_add_obj(m_cast_play_group, ui_cast_play);
    lv_group_focus_obj(ui_cast_play);

    //extern void api_set_i2so_gpio_mute_auto(void);
    api_set_i2so_gpio_mute_auto();

    //exit by phone, the flag would be set false, so set true here
    //to avoid hot key exit.
    win_exit_to_cast_root_by_key_set(true);
	m_win_cast_play_open = true;
	return API_SUCCESS;
}

static int win_cast_play_close(void *arg)
{
    printf("%s(), line: %d!\n", __func__, __LINE__);
    //lv_obj_clean(lv_scr_act());

    uint32_t cast_type = win_cast_play_param();

    if (win_exit_to_cast_root_by_key_get()){
        if (MSG_TYPE_CAST_AIRMIRROR_START == cast_type){
            hccast_air_service_stop();
            hccast_air_service_start();
        }else{
            hccast_mira_disconnect();
        }
    }

   if (m_cast_play_group){
        lv_group_remove_all_objs(m_cast_play_group);
        lv_group_del(m_cast_play_group);
        lv_group_set_default(NULL);
    }

    win_msgbox_msg_close();
	m_win_cast_play_open = false;
    printf("%s(), line: %d!\n", __func__, __LINE__);

    api_hotkey_disable_clear();
    return API_SUCCESS;
}

static void win_cast_play_control(void *arg1, void *arg2)
{

	printf("%s()!\n", __FUNCTION__);

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

    if(ctl_msg->msg_type == MSG_TYPE_CAST_AIRMIRROR_STOP ||
        ctl_msg->msg_type == MSG_TYPE_CAST_MIRACAST_STOP){
        
        win_exit_to_cast_root_by_key_set(false);
        _ui_screen_change(ui_wifi_cast_root,0,0);
        //ret = WIN_CTL_POPUP_CLOSE;
	}
	else if(ctl_msg->msg_type == MSG_TYPE_AIR_MIRROR_BAD_NETWORK)
	{
		win_msgbox_msg_open(STR_NETWORK_ERR, 1000*60*60, NULL, NULL);	
	}
}

void ui_cast_play_init(void)
{
    screen_entry_t cast_play_entry;

    ui_cast_play = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_cast_play, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_cast_play, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_opa(ui_cast_play, LV_OPA_TRANSP, 0);

    cast_play_entry.screen = ui_cast_play;
    cast_play_entry.control = win_cast_play_control;
    api_screen_regist_ctrl_handle(&cast_play_entry);
    cast_mira_ui_wait_init(win_cast_play_wait_open);
    cast_air_ui_wait_init(win_cast_play_wait_open);
}


// win_des_t g_win_cast_play = 
// {
//     .open = win_cast_play_open,
//     .close = win_cast_play_close,
//     .control = win_cast_play_control,
// };


#endif
