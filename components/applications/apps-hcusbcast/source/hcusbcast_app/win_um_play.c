/*
win_um_play.c
 */
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
//#include <lvgl/lvgl.h>
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"

#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
#include "osd_com.h"
#include "data_mgr.h"

#ifdef LVGL_RESOLUTION_240P_SUPPORT
#define QR_UM_MSG_W        110
#define QR_UM_BOX_W        60

#else

#define QR_UM_MSG_W        350
#define QR_UM_BOX_W        160
#endif

static lv_obj_t *m_um_play_root = NULL;
static lv_obj_t *m_msg_show = NULL;
static lv_obj_t *m_version = NULL;
static lv_obj_t *m_label_qr_msg = NULL;
static lv_obj_t *m_cast_qr = NULL;
static lv_timer_t *m_um_timer = NULL;
static lv_obj_t *m_label_demo = NULL;

static int m_win_um_play = 0;
static pthread_mutex_t g_win_um_menu_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_win_um_timer_start = 0;
static int g_um_plugined = 0;//check usbmirror has been connected.

typedef enum{
    UM_TYPE_NONE,
    UM_YTPE_I, //apple device    
    UM_YTPE_A, //android devide
}UM_TYPE_E;


static volatile bool m_playing = false;
static char m_um_type = UM_TYPE_NONE;

//extern int cast_usb_mirror_start(void);
//extern int cast_usb_mirror_stop(void);
typedef enum{
    QR_UM_CLEAR,
    QR_UM_SHOW_UPGRADE,
}qr_um_show_type_t;


//http://119.3.89.190:8080/upgrade/link/upgrade.html?product=HCT-AT01&chip=1512ae01&vendor=HCC00&version=1.0.0-430314    
#define IOS_UPGRADE_URL	"http://119.3.89.190:8080/upgrade/link/upgrade.html?"
static void win_um_update_qr_code(qr_um_show_type_t qr_type)
{
    sys_data_t *sys_data = data_mgr_sys_get();
    char qr_txt[256] = {0};

    uint32_t chip_id = *((uint32_t*)(0xb8800000));
    uint32_t version = sys_data->firmware_version;

    lv_obj_clear_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(m_label_qr_msg, LV_OBJ_FLAG_HIDDEN);
    switch (qr_type)
    {
    case QR_UM_CLEAR:
        lv_obj_add_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_label_qr_msg, LV_OBJ_FLAG_HIDDEN);
        break;
    case QR_UM_SHOW_UPGRADE:
        sprintf(qr_txt, "%sproduct=%s&chip=%x&verdor=%s&version=%u", 
        	IOS_UPGRADE_URL, sys_data->product_id, chip_id, "HCC00", version);
		lv_label_set_text(m_label_qr_msg, "Scan to upgrade");
	printf("qr_txt:%s\n",qr_txt);	
        break;
    default:
        break;
    }

    if (strlen(qr_txt))
        lv_qrcode_update(m_cast_qr, qr_txt, strlen(qr_txt));
}


static void um_timer_cb(lv_timer_t * t)
{
    win_um_update_qr_code(QR_UM_SHOW_UPGRADE);
}

static unsigned int win_um_get_tick(void)
{
    unsigned int  cur_tick;
    struct timespec time;

    clock_gettime(CLOCK_REALTIME, &time);

    cur_tick = (time.tv_sec * 1000) + (time.tv_nsec / 1000000);

    return cur_tick;
}


static void *um_timer_process(void *arg)
{
    int count_tick = 30*1000;//30s
    unsigned int tick = win_um_get_tick();
    control_msg_t ctl_msg = {0};
    
    printf("[%s] is running\n",__func__);
    
    while(1)
    {
        pthread_mutex_lock(&g_win_um_menu_mutex);
        
        if((win_um_get_tick() - tick) < count_tick)
        {
            if((m_win_um_play == 0) || g_um_plugined) //menu close or plugined, need exit.
            {
                printf("[%s] menu close[%d] or plugined[%d], need exit\n",__func__,m_win_um_play,g_um_plugined);
                break;
            }
        }
        else //wait time out.
        {
            printf("[%s] g_um_plugined[%d] m_win_um_play[%d]\n",__func__,g_um_plugined,m_win_um_play);
            if((g_um_plugined == 0) && m_win_um_play)
            {
                //win_um_update_qr_code(QR_UM_SHOW_UPGRADE);
                ctl_msg.msg_type = MSG_TYPE_UM_SHOW_QR;
                api_control_send_msg(&ctl_msg);
            }

            break;
        }
        
        pthread_mutex_unlock(&g_win_um_menu_mutex);
        
        sleep(1);
    }

    g_win_um_timer_start = 0;
    
    pthread_mutex_unlock(&g_win_um_menu_mutex);
    
    printf("[%s] exit!\n",__func__);
    
    return NULL;
}

static int win_um_play_open(void *arg)
{
    pthread_mutex_lock(&g_win_um_menu_mutex);
    pthread_t tid;
    sys_data_t *sys_data = data_mgr_sys_get();
    // uint32_t msg_type;
    // msg_type = (uint32_t)arg;
    printf("%s()\n", __func__);
    //hccast_stop_services();
	
    m_um_type = UM_TYPE_NONE;
    api_set_display_aspect(DIS_TV_16_9, DIS_PILLBOX);    

    m_um_play_root = lv_obj_create(lv_scr_act());

    osd_draw_background(m_um_play_root, true);
    lv_obj_clear_flag(m_um_play_root, LV_OBJ_FLAG_SCROLLABLE);

    m_msg_show = lv_label_create(m_um_play_root);
    lv_obj_align(m_msg_show, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(m_msg_show, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(m_msg_show, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(m_msg_show, &lv_font_montserrat_22, 0);
    lv_label_set_text(m_msg_show, "Please connect the device!");

    m_version = lv_label_create(m_um_play_root);
    lv_obj_align(m_version, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_opa(m_version, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(m_version, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(m_version, &lv_font_montserrat_22, 0);
    unsigned int ver = (unsigned int)sys_data->firmware_version;
    //lv_label_set_text_fmt(m_version, "Version: %u", (unsigned int)sys_data->firmware_version);    
    lv_label_set_text_fmt(m_version, "Version:%02u.%02u.%02u", ver/100000000, (ver%100000000)/1000000, ((ver%1000000)/10000));

    m_label_qr_msg = lv_label_create(m_um_play_root);
    lv_obj_align(m_label_qr_msg, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_opa(m_label_qr_msg, LV_OPA_TRANSP, 0);
    //lv_obj_set_width(m_label_qr_msg, QR_UM_MSG_W);
    lv_obj_set_style_text_color(m_label_qr_msg, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(m_label_qr_msg, &lv_font_montserrat_22, 0);

    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);
    m_cast_qr = lv_qrcode_create(m_um_play_root, QR_UM_BOX_W, fg_color, bg_color);
    lv_obj_align_to(m_cast_qr, m_label_qr_msg, LV_ALIGN_OUT_TOP_RIGHT, -10, -20);
    lv_obj_set_style_border_color(m_cast_qr, bg_color, 0);
    lv_obj_set_style_border_width(m_cast_qr, 5, 0);

    m_label_demo = lv_label_create(m_um_play_root);
    lv_obj_set_style_text_font(m_label_demo, &lv_font_montserrat_22, 0); 
    lv_obj_set_style_text_color(m_label_demo, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(m_label_demo, LV_ALIGN_TOP_LEFT, 20, 20);
    if (cast_is_demo())
        lv_label_set_text(m_label_demo, "demo");
    else
        lv_label_set_text(m_label_demo, " ");

    win_um_update_qr_code(QR_UM_CLEAR);

#if 0    
    if (!m_um_timer)
    {
    	m_um_timer = lv_timer_create(um_timer_cb, 30000, NULL);
    	lv_timer_set_repeat_count(m_um_timer, 1);
    }
#endif    

    if ((g_win_um_timer_start == 0) && (g_um_plugined == 0))
    {
        g_win_um_timer_start = 1;

        if (pthread_create(&tid, NULL, um_timer_process, NULL) != 0)
        {
            g_win_um_timer_start = 0;
        }
    }
    
    m_win_um_play = 1;
    pthread_mutex_unlock(&g_win_um_menu_mutex);
    
    return API_SUCCESS;
}

static int win_um_play_close(void *arg)
{
    pthread_mutex_lock(&g_win_um_menu_mutex);
    m_win_um_play = 0;
    
    lv_obj_del(m_um_play_root);

    m_playing = false;
    m_um_type = UM_TYPE_NONE;
    //recover the dispaly aspect.
    //api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);
#if 0
    if (m_um_timer)
    	lv_timer_del(m_um_timer);
    m_um_timer = NULL;
#endif
    pthread_mutex_unlock(&g_win_um_menu_mutex);

    return API_SUCCESS;
}


static void win_um_play_stop(bool stop_by_key)
{
#if 1    
    if (stop_by_key)
    {
        if (UM_YTPE_I == m_um_type){
            hccast_ium_stop_mirroring();
        }
        else if (UM_YTPE_A == m_um_type){
            hccast_aum_stop_mirroring();
        }
    }
#endif    
    m_playing = false;
    m_um_type = UM_TYPE_NONE;
    lv_obj_clear_flag(m_um_play_root, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(m_msg_show, "Please connect the device!");
    win_um_update_qr_code(QR_UM_CLEAR);
    printf("%s().\n", __func__);
}


static win_ctl_result_t win_um_play_control(void *arg1, void *arg2)
{

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_SKIP;
    win_des_t *cur_win = NULL;
    static int ium_dev_add = 0;
    
    if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_STOP ||
        ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_STOP){
        win_um_play_stop(false);
	}else if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_START ||
        ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_START){
    	m_playing = true;
    	lv_obj_add_flag(m_um_play_root, LV_OBJ_FLAG_HIDDEN);

        if (ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_START)
        {
            m_um_type = UM_YTPE_I;
            ium_dev_add = 1;
        }    
        else
        {
            m_um_type = UM_YTPE_A;
        }    

        //extern void api_set_i2si_gpio_mute_auto(void);
        //api_set_i2si_gpio_mute_auto();
        //lv_timer_pause(m_um_timer);

	}else if(ctl_msg->msg_type == MSG_TYPE_AUM_DEV_ADD ||
        ctl_msg->msg_type == MSG_TYPE_IUM_DEV_ADD){

            pthread_mutex_lock(&g_win_um_menu_mutex);
            g_um_plugined = 1;
            pthread_mutex_unlock(&g_win_um_menu_mutex);

	    lv_label_set_text(m_msg_show, "Device connecting ...");

    #ifdef MIRROR_ES_DUMP_SUPPORT
        extern bool api_mirror_dump_enable_get(char* folder);
        char dump_folder[64];

        if (USB_STAT_MOUNT != mmp_get_usb_stat())
        {
            printf("%s(), line: %d. No disk, disable dump!\n", __func__, __LINE__);
            hccast_um_es_dump_stop();
            return;
        }
        printf("%s(), line: %d. Statr USB mirror ES dump!\n", __func__, __LINE__);
        if (api_mirror_dump_enable_get(dump_folder)){
            hccast_um_es_dump_start(dump_folder);
        } else {
            hccast_um_es_dump_stop();
        }
    #endif    
    }
    else if((ctl_msg->msg_type == MSG_TYPE_IUM_START_UPGRADE) || (ctl_msg->msg_type == MSG_TYPE_USB_UPGRADE))
    {
        cur_win = &g_win_upgrade;
        cur_win->param = (void*)(ctl_msg->msg_type);
        menu_mgr_push(cur_win);
        ret = WIN_CTL_PUSH_CLOSE;
    }
    else if(ctl_msg->msg_type == MSG_TYPE_UM_SHOW_QR)
    {
        //pthread_mutex_lock(&g_win_um_menu_mutex);
        
        if(g_um_plugined == 0)
        {
            win_um_update_qr_code(QR_UM_SHOW_UPGRADE);
        }
        
        //pthread_mutex_unlock(&g_win_um_menu_mutex);
    }
    else if(ctl_msg->msg_type == MSG_TYPE_AIR_INVALID_CERT)
    {
        lv_label_set_text(m_label_demo, "demo");
    }
    else if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_NEED_TRUST)
    {
        win_msgbox_msg_open("Please select \"Trust\" to \nstart mirror cast", 5000, NULL, NULL);
    }
    else if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_DEVICE_REMOVE)
    {
        win_msgbox_msg_close();
        win_lable_pop_msg_close();
        ium_dev_add = 0;
    }
    else if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_NO_DATA)
    {
        if(ium_dev_add)
        {
            win_lable_pop_msg_open("Connect fail, reboot the phone and try again.");
        }
    }

	return ret;
}

/*
bool win_um_play_get(void)
{
	return m_win_um_play;
}
*/

win_des_t g_win_um_play = 
{
    .open = win_um_play_open,
    .close = win_um_play_close,
    .control = win_um_play_control,
};
