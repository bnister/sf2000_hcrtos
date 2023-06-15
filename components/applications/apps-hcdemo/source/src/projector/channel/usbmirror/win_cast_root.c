/*
win_cast_root.c
 */
#ifdef WIFI_SUPPORT
#include <pthread.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "osd_com.h"
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_dhcpd.h>
#include <hccast/hccast_httpd.h>
#include <hcfota.h>

#include "cast_api.h"

//#include "../lv_drivers/display/fbdev.h"
#include "../../screen.h"

#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
//#include "network_api.h"
//#include "data_mgr.h"


#define IMG_PHONE_X  336
#define IMG_PHONE_Y  524

#define IMG_DONGLE_X  546
#define IMG_DONGLE_Y   434

#define LABEL_LOCAL_SSID_W   300
#define LABEL_LOCAL_SSID_X  ((OSD_MAX_WIDTH-LABEL_LOCAL_SSID_W) >> 1) //480
#define LABEL_LOCAL_SSID_Y   30

#define LABEL_WIFI_SSID_X  40
#define LABEL_WIFI_SSID_Y  110
#define LABEL_WIFI_SSID_W  600//280

#define LABEL_LOCAL_PWD_X  LABEL_LOCAL_SSID_X + 400
#define LABEL_LOCAL_PWD_Y   LABEL_LOCAL_SSID_Y
#define LABEL_LOCAL_PWD_W   300

#define LABEL_VERSION_X  LABEL_WIFI_SSID_X
#define LABEL_VERSION_Y  (OSD_MAX_HEIGHT-80)
#define LABEL_VERSION_W   300

#define AIR_DEMO_X  (OSD_MAX_WIDTH-AIR_DEMO_W)//30
#define AIR_DEMO_Y  LABEL_VERSION_Y
#define AIR_DEMO_W 200

#define AIR_STATUS_W 400
#define AIR_STATUS_X    ((OSD_MAX_WIDTH-AIR_STATUS_W)/2)
#define AIR_STATUS_Y    LABEL_VERSION_Y

#define LABEL_CONNECT_MSG_W   600
#define LABEL_CONNECT_MSG_X  ((OSD_MAX_WIDTH-LABEL_CONNECT_MSG_W) >> 1)
#define LABEL_CONNECT_MSG_Y  LABEL_VERSION_Y

#define QR_MSG_X        20
#define QR_MSG_Y        220
#define QR_MSG_W        220

#define QR_BOX_X        QR_MSG_X+10
#define QR_BOX_Y        QR_MSG_Y+80
#define QR_BOX_W        160

static lv_obj_t *win_root_obj = NULL;
lv_obj_t * ui_wifi_cast_root = NULL;

static lv_obj_t *m_label_local_ssid = NULL;
static lv_obj_t *m_label_wifi_ssid = NULL;
static lv_obj_t *m_label_password = NULL; //also for ip addr

static lv_obj_t *m_label_ip = NULL;

static lv_obj_t *m_label_connect_state = NULL;
static lv_obj_t *m_label_wifi_mode = NULL;

//static lv_obj_t *m_label_connect_msg = NULL;

static lv_obj_t *m_label_qr_msg = NULL;
static lv_obj_t *m_label_state_msg = NULL;

static lv_obj_t *m_label_version = NULL;
static lv_obj_t *m_label_demo = NULL;
static lv_obj_t *m_cast_qr = NULL;

static lv_style_t m_large_text_style;
static lv_style_t m_mid_text_style;
static lv_style_t m_small_text_style;

static volatile bool m_win_cast_open = false;

static lv_timer_t *m_connect_timer = NULL;

typedef enum
{
    QR_CLEAR,
    QR_CONNECT_AP,
    QR_SCAN_WIFI,
    QR_CONFIG,
} qr_show_type_t;

static int win_cast_open();
static int win_cast_close();

static void hc_background_show(bool show)
{
    lv_obj_t* bgk;
    bgk = lv_obj_create(lv_scr_act());//创建对象
    //lv_obj_clean_style_list(bgk, LV_OBJ_PART_MAIN); //清空对象风格
    lv_obj_set_style_bg_opa(bgk, LV_OPA_10, LV_PART_MAIN);//设置颜色覆盖度100%，数值越低，颜色越透。
    lv_obj_set_style_bg_color(bgk, lv_color_hex(0x4B50B0), LV_PART_MAIN);//设置背景颜色为绿色
    //省去下方两行代码，默认是从0,0处开始绘制
    lv_obj_set_x(bgk, 50);//设置X轴起点
    lv_obj_set_y(bgk, 50);//设置Y轴起点

    lv_obj_set_size(bgk, 600, 400);//设置覆盖大小
//  lv_task_create(bgk_anim, 500, LV_TASK_PRIO_LOW, bgk);//创建任务 500ms一次
}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (ta == ui_wifi_cast_root){
        if(event == LV_EVENT_SCREEN_LOAD_START) {
            win_cast_open();
        }
        if(event == LV_EVENT_SCREEN_UNLOAD_START) {
            win_cast_close();
        }
    } else if (ta == win_root_obj){
        lv_indev_t *key_indev = lv_indev_get_act();
        if(event == LV_EVENT_KEY && key_indev->proc.state == LV_INDEV_STATE_PRESSED){
            uint32_t value = lv_indev_get_key(key_indev);
            if (value == LV_KEY_ESC)
            {
                //_ui_screen_change(channel_scr,0,0);
                change_screen(SCREEN_CHANNEL);
            }
            // vkey = key_convert_vkey(value);
            // if (!key_filter(vkey))
            //     return;
            // printf("media vkey: %d...\n", vkey);
            // api_control_send_key(vkey);
        }


    }
}

static lv_obj_t *lv_img_open(lv_obj_t *parent, const char *bitmap, int x, int y)
{
    lv_obj_t * img = lv_img_create(parent);
    lv_img_set_src(img, bitmap);
    lv_obj_set_pos(img, x, y);
    //lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, 0);
    return img;
}

static lv_obj_t *lv_label_open(lv_obj_t *parent, int x, int y, int w, char *str, lv_style_t *style)
{
    lv_obj_t *label = lv_label_create(parent);

    if (x && y)
        lv_obj_set_pos(label, x, y);

    if (w)
        lv_obj_set_width(label, w);

    if (str)
        lv_label_set_text(label, str);
    else
        lv_label_set_text(label, "");

    lv_obj_add_style(label, style, 0);

    return label;
}

static void win_cast_update_qr_code(qr_show_type_t qr_type)
{


    /*Set data*/
    //const char * data = "https://lvgl.io";
    char qr_txt[128] = {0};
    char msg_txt[128] = {0};

    lv_obj_clear_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
    switch (qr_type)
    {
        case QR_CLEAR:
            sprintf(msg_txt," ");
            lv_obj_add_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
            break;
        case QR_CONNECT_AP:
            sprintf(msg_txt,"First scan QR code\nSet AP network");
            sprintf(qr_txt, "WIFI:T:WPA;S:%s;P:%s;", \
                    data_mgr_get_device_name(), data_mgr_get_device_psk());
            break;
        case QR_SCAN_WIFI:
            sprintf(msg_txt,"Second scan QR code\nSet WiFi network\nhttp://%s", HCCAST_HOSTAP_IP);
            sprintf(qr_txt, "http://%s", HCCAST_HOSTAP_IP);
            break;
        case QR_CONFIG:
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
            sprintf(msg_txt,"Scan QR code\nSet device parameters\nhttp://%s", (const char*)wifi_local_ip_get());
            sprintf(qr_txt, "http://%s", (const char*)wifi_local_ip_get());
#else // Ethernet
            sprintf(msg_txt,"Scan QR code\nSet device parameters\nhttp://%s", (const char*)eth_local_ip_get());
            sprintf(qr_txt, "http://%s", (const char*)eth_local_ip_get());
#endif
#endif
            break;
        default:
            break;
    }

    if (strlen(msg_txt))
        lv_label_set_text(m_label_qr_msg, msg_txt);

    if (strlen(qr_txt))
        lv_qrcode_update(m_cast_qr, qr_txt, strlen(qr_txt));

}

static void win_cast_no_wifi_device_show()
{
#ifdef WIFI_SUPPORT

    //lv_label_set_text(m_label_local_ssid, "No WiFi device");
    lv_label_set_text_fmt(m_label_local_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");

    lv_label_set_text(m_label_ip, "");
    //lv_label_set_text(m_label_connect_state, "No connection");
    lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_CLOSE, "No connection");
    lv_label_set_text(m_label_wifi_mode, "");
    lv_label_set_text(m_label_wifi_ssid, "");
    lv_label_set_text(m_label_password, "");

    win_cast_update_qr_code(QR_CLEAR);
#endif
}

static bool connect_show = 0;
static void connect_timer_cb(lv_timer_t * t)
{
    if (!win_root_obj)
        return;

    if (connect_show)
        lv_obj_add_flag(m_label_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_clear_flag(m_label_wifi_ssid, LV_OBJ_FLAG_HIDDEN);

    connect_show = !connect_show;
}

static void active_connect_timer(bool active)
{
    if (NULL == m_connect_timer)
        m_connect_timer = lv_timer_create(connect_timer_cb, 800, NULL);

    if (active)
    {
        lv_timer_resume(m_connect_timer);
    }
    else
    {
        lv_timer_pause(m_connect_timer);
        lv_obj_clear_flag(m_label_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
    }
}

//The first time enter screen application, WiFi should be station mode
//if there is wifi ap information in data node.
static volatile int m_first_flag = 1;
static void win_cast_connect_state_upate(bool force_station)
{
#ifdef WIFI_SUPPORT

    char show_txt[64] = {0};
    int station_mode = 0;

    active_connect_timer(false);
    lv_label_set_text(m_label_state_msg, "");

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
    if (!network_wifi_module_get())
    {
        win_cast_no_wifi_device_show();
        return;
    }

    //lv_label_set_text(m_label_local_ssid, data_mgr_get_device_name());
    lv_label_set_text_fmt(m_label_local_ssid, "SSID: %s", data_mgr_get_device_name());

    //lv_label_set_text(m_label_password, "12345678");
    lv_label_set_text_fmt(m_label_password, "Password: %s", data_mgr_get_device_psk());

    if (hccast_wifi_mgr_get_connect_status())
    {
        station_mode = 1;
    }
    else
    {
        if (m_first_flag)
        {
            hccast_wifi_ap_info_t wifi_ap;
            if (data_mgr_wifi_ap_get(&wifi_ap))
            {
                station_mode = 1;
            }
        }
    }
#else
    station_mode = 1;
#endif // WIFI_SUPPORT
#else // NO NETWORK_SUPPORT
    win_cast_no_wifi_device_show();
    return;
#endif // NETWORK_SUPPORT

#ifdef NETWORK_SUPPORT
    m_first_flag = 0;

    if (force_station)
        station_mode = 1;

    printf("%s(), line:%d, force_station:%d, station_mode:%d\n", __func__, __LINE__, force_station, station_mode);

    if (station_mode)
    {
#ifdef WIFI_SUPPORT
        //connect to wifi
        hccast_wifi_ap_info_t wifi_ap;
        if (data_mgr_wifi_ap_get(&wifi_ap))
        {
            lv_label_set_text_fmt(m_label_wifi_ssid, "%s %s", LV_SYMBOL_WIFI, wifi_ap.ssid);
        }

        char *local_ip = wifi_local_ip_get();
#else // Ethernet
        lv_label_set_text_fmt(m_label_local_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");
        lv_label_set_text_fmt(m_label_wifi_ssid, "%s Ethernet", LV_SYMBOL_WIFI);
        char *local_ip = eth_local_ip_get();
#endif // WIFI_SUPPORT

        sprintf(show_txt, "IP: %s", local_ip);
        if (local_ip[0])
        {
            lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_OK, "Connected");
            lv_label_set_text_fmt(m_label_ip, "%s %s", LV_SYMBOL_HOME, show_txt);
            win_cast_update_qr_code(QR_CONFIG);
        }
        else
        {
            lv_label_set_text(m_label_ip, "");
            lv_label_set_text(m_label_state_msg, "WiFi connecting ...");
            active_connect_timer(true);
            lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_CLOSE, "No connection");
            win_cast_update_qr_code(QR_CLEAR);
        }

#ifdef WIFI_SUPPORT
        lv_label_set_text(m_label_wifi_mode, "WiFi mode: Station");
#else // Ethernet
        lv_label_set_text(m_label_wifi_mode, "ETH mode");
#endif
    }
    else
    {
#ifdef WIFI_SUPPORT
        lv_label_set_text(m_label_wifi_ssid, "");
        lv_label_set_text(m_label_wifi_mode, "WiFi mode: AP");
        int connected_cnt = hostap_get_connect_count();
        if ( connected_cnt > 0)
        {
            //AP mode, phone has connected.
            sprintf(show_txt, "IP: %s", HCCAST_HOSTAP_IP);
            //lv_label_set_text(m_label_ip, show_txt);
            lv_label_set_text_fmt(m_label_ip, "%s %s", LV_SYMBOL_HOME, show_txt);
            //lv_label_set_text(m_label_connect_state, "Connected");
            lv_label_set_text_fmt(m_label_connect_state, "%s [%d] %s", LV_SYMBOL_OK, connected_cnt, "Connected");
            win_cast_update_qr_code(QR_SCAN_WIFI);
        }
        else
#endif
        {
            //lv_label_set_text(m_label_connect_state, "No connection");
            lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_CLOSE, "No connection");
            win_cast_update_qr_code(QR_CONNECT_AP);
        }
    }
#endif
#endif
}

static int win_cast_open(void)
{   
#ifdef WIFI_SUPPORT

    m_stop_service_exit = true;
    m_exit_to_cast_by_key = false;        

    cast_start_service();

    win_root_obj = lv_obj_create(ui_wifi_cast_root);
    struct sysdata *sys_data;
    osd_draw_background(win_root_obj, true);
    lv_obj_clear_flag(win_root_obj, LV_OBJ_FLAG_SCROLLABLE);
   	
    m_cast_group = lv_group_create();
    lv_indev_set_group(get_lv_key_dev(), m_cast_group);
    lv_obj_add_event_cb(win_root_obj, event_handler, LV_EVENT_ALL, NULL);    
    lv_group_add_obj(m_cast_group, win_root_obj);


    sys_data = &(projector_get_sys_param()->sys_data);

    printf("%s(), line: %d!, fw_ver: 0x%x, product_id:%s\n", 
        __func__, __LINE__, sys_data->firmware_version, sys_data->product_id);
    
    api_logo_show(NULL);

//    lv_demo_music();
    lv_obj_t *parent_obj = win_root_obj;

    lv_style_init(&m_large_text_style);
    lv_style_init(&m_mid_text_style);
    lv_style_init(&m_small_text_style);
    lv_style_set_text_font(&m_large_text_style, FONT_SIZE_LARGE);
    lv_style_set_text_font(&m_mid_text_style, FONT_SIZE_MID);
    lv_style_set_text_font(&m_small_text_style, FONT_SIZE_SMALL);
    lv_style_set_text_color(&m_large_text_style, COLOR_WHITE);
    lv_style_set_text_color(&m_mid_text_style, COLOR_WHITE);
    lv_style_set_text_color(&m_small_text_style, COLOR_WHITE);

    // lv_style_set_text_color(&m_mid_text_style, COLOR_WHITE);
    // lv_style_set_text_font(&m_mid_text_style, FONT_SIZE_MID);

#ifdef USBMIRROR_SUPPORT
    lv_obj_t *usb_wire = lv_label_create(parent_obj);
    lv_obj_set_pos(usb_wire, LABEL_LOCAL_SSID_X - 80, LABEL_LOCAL_SSID_Y);
    lv_obj_set_style_text_font(usb_wire, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(usb_wire, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_label_set_text_fmt(usb_wire, "%s ", LV_SYMBOL_USB);
#endif

    m_label_local_ssid = lv_label_open(parent_obj, LABEL_LOCAL_SSID_X, LABEL_LOCAL_SSID_Y, LABEL_LOCAL_SSID_W, NULL, &m_mid_text_style);
    lv_label_set_long_mode(m_label_local_ssid, LV_LABEL_LONG_DOT);
    m_label_wifi_ssid = lv_label_open(parent_obj, LABEL_WIFI_SSID_X, LABEL_WIFI_SSID_Y, LABEL_WIFI_SSID_W, NULL, &m_mid_text_style);
    lv_label_set_long_mode(m_label_wifi_ssid, LV_LABEL_LONG_DOT);
    m_label_password = lv_label_open(parent_obj, LABEL_LOCAL_PWD_X, LABEL_LOCAL_PWD_Y, LABEL_LOCAL_PWD_W, NULL, &m_mid_text_style);
    m_label_ip = lv_label_open(parent_obj, LABEL_LOCAL_SSID_X, LABEL_LOCAL_PWD_Y+40, LABEL_LOCAL_SSID_W, NULL, &m_mid_text_style);
    m_label_connect_state = lv_label_open(parent_obj, LABEL_LOCAL_PWD_X, LABEL_LOCAL_PWD_Y+40, LABEL_LOCAL_PWD_W, NULL, &m_mid_text_style);
    m_label_wifi_mode = lv_label_open(parent_obj, LABEL_LOCAL_PWD_X, LABEL_LOCAL_PWD_Y+40*2, LABEL_LOCAL_PWD_W, NULL, &m_mid_text_style);
    //the ending user may not need to know wifi mode, so hide here.
    lv_obj_add_flag(m_label_wifi_mode, LV_OBJ_FLAG_HIDDEN);
    
    m_label_qr_msg = lv_label_open(parent_obj, QR_MSG_X, QR_MSG_Y, QR_MSG_W, NULL, &m_small_text_style);

                
    m_label_demo = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_demo, LV_ALIGN_BOTTOM_LEFT, 20, -20);

    if (cast_is_demo())
        lv_label_set_text_fmt(m_label_demo, "demo: %s", sys_data->product_id);
    else
        lv_label_set_text_fmt(m_label_demo, "%s", sys_data->product_id);

    m_label_version = lv_label_open(parent_obj, 0, 0, 0, 
        NULL, &m_mid_text_style);
    lv_label_set_text_fmt(m_label_version, "Ver: %u", sys_data->firmware_version);
    lv_obj_align(m_label_version, LV_ALIGN_BOTTOM_RIGHT, -20, -20);

	m_label_state_msg = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_state_msg, LV_ALIGN_BOTTOM_MID, 0, -20);
  
    //QR code init
    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);
    m_cast_qr = lv_qrcode_create(parent_obj, QR_BOX_W, fg_color, bg_color);
    lv_obj_set_pos(m_cast_qr, QR_BOX_X, QR_BOX_Y);
    /*Add a border with bg_color(white: 0)*/
    lv_obj_set_style_border_color(m_cast_qr, bg_color, 0);
    lv_obj_set_style_border_width(m_cast_qr, 5, 0);


    win_cast_connect_state_upate(false);

    m_win_cast_open = true;
#endif
    return API_SUCCESS;
}



static int win_cast_close(void)
{
#ifdef WIFI_SUPPORT

    printf("%s(), line: %d!\n", __func__, __LINE__);
    cast_stop_service();

    lv_obj_del(win_root_obj);
    // lv_obj_add_flag(lv_scr_act(), LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(m_img_phone_connect, LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(m_img_dongle_connect, LV_OBJ_FLAG_HIDDEN);
	win_msgbox_msg_close();
	
    api_osd_off_time(1000);
    api_logo_off();

    if (m_cast_group){
        lv_group_remove_all_objs(m_cast_group);
        lv_group_del(m_cast_group);
    }

    if (m_connect_timer){
        lv_timer_pause(m_connect_timer);
        lv_timer_del(m_connect_timer);
    }
    m_connect_timer = NULL;

    m_win_cast_open = false;
    win_root_obj = NULL;
#endif
    return API_SUCCESS;
}


static win_ctl_result_t win_cast_msg_proc(void *arg1, void *arg2)
{
#if 0
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;
    win_des_t *cur_win = NULL;
    int cur_scene;
    //sys_data_t *sys_data;
    //sys_data = data_mgr_sys_get();
	printf("%s,ctl_msg->msg_type:%d\n",__func__,ctl_msg->msg_type);
    switch (ctl_msg->msg_type)
    {
    #ifdef WIFI_SUPPORT
        case MSG_TYPE_CAST_DLNA_START:
        case MSG_TYPE_CAST_AIRCAST_START:
			 #ifdef DLNA_SUPPORT
            cur_win = &g_win_dlna_play;
            cur_win->param = (void*)(((ctl_msg->msg_type&0xFFFF) << 16) | (ctl_msg->msg_code&0xFFFF));
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
			#endif
            break;
        case MSG_TYPE_CAST_AIRMIRROR_START:
        case MSG_TYPE_CAST_MIRACAST_START:
            cur_win = &g_win_cast_play;
            cur_win->param = (void*)ctl_msg->msg_code;
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
            break;
		#endif
        case MSG_TYPE_CAST_AUSB_START:
        case MSG_TYPE_CAST_IUSB_START:
            win_msgbox_msg_close();
            cur_win = &g_win_um_play;
            cur_win->param = (void*)ctl_msg->msg_type;
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
            break;
		#ifdef WIFI_SUPPORT
        case MSG_TYPE_NETWORK_WIFI_CONNECTED:
        case MSG_TYPE_NETWORK_WIFI_DISCONNECTED:
        case MSG_TYPE_NETWORK_DEVICE_BE_CONNECTED:
        case MSG_TYPE_NETWORK_DEVICE_BE_DISCONNECTED:
            win_cast_connect_state_upate(false);
            break;
        case MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL:
        {
            win_msgbox_msg_open("Connect WiFi fail!", 5000, NULL, NULL);
            win_cast_connect_state_upate(false);
            break;
        }
        case MSG_TYPE_NETWORK_WIFI_CONNECTING:
            win_cast_connect_state_upate(true);
            break;
        case MSG_TYPE_NETWORK_WIFI_SCANNING:
            lv_label_set_text(m_label_state_msg, "WiFi is scanning ...");
            break;
        case MSG_TYPE_NETWORK_WIFI_SCAN_DONE:
            lv_label_set_text(m_label_state_msg, "");
            break;
        case MSG_TYPE_CAST_MIRACAST_CONNECTING:
            lv_label_set_text(m_label_state_msg, "Miracast is connecting ...");
            break;
        case MSG_TYPE_CAST_MIRACAST_CONNECTED:
            lv_label_set_text(m_label_state_msg, "Miracast connect OK.");
            break;
        case MSG_TYPE_CAST_MIRACAST_SSID_DONE:
            lv_label_set_text(m_label_state_msg, "");
            break;
        case MSG_TYPE_USB_WIFI_PLUGOUT:
            //stop current playing? stop http
            win_cast_no_wifi_device_show();
            break;
        case MSG_TYPE_USB_WIFI_PLUGIN:
            break;
        case MSG_TYPE_AIR_INVALID_CERT:
            lv_label_set_text_fmt(m_label_demo, "demo: %s", sys_data->product_id);

            break;
        case MSG_TYPE_CAST_AIRCAST_AUDIO_START:
            lv_label_set_text(m_label_state_msg, "Aircast is playing music");
            break;
        case MSG_TYPE_CAST_AIRCAST_AUDIO_STOP:
            lv_label_set_text(m_label_state_msg, "");
            break;
		#endif
        case MSG_TYPE_USB_UPGRADE:
        case MSG_TYPE_NET_UPGRADE:
            cur_win = &g_win_upgrade;
            cur_win->param = (void*)(ctl_msg->msg_type);
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
            break;
		#ifdef WIFI_SUPPORT
        case MSG_TYPE_NETWORK_DEV_NAME_SET:
            //lv_label_set_text(m_label_local_ssid, data_mgr_get_device_name());
            break;
        case MSG_TYPE_NETWORK_CONNECTING:
            break;
        case MSG_TYPE_NETWORK_INIT_OK:
            break;
        case MSG_TYPE_NETWORK_WIFI_PWD_WRONG:
            break;
        case MSG_TYPE_NETWORK_DHCP_ON:
            //start up web server.
            //webs_start();

            //start up cast service.

            break;
        case MSG_TYPE_HDMI_TX_CHANGED:
			#ifdef AIRCAST_SUPPORT
            cur_scene = hccast_get_current_scene();
            if((cur_scene != HCCAST_SCENE_AIRCAST_PLAY) && (cur_scene != HCCAST_SCENE_AIRCAST_MIRROR))
            {
                if(hccast_air_service_is_start())
                {
                    hccast_air_service_stop();
                    hccast_air_service_start();
                }
            }
			#endif
            break;
        case MSG_TYPE_AIR_HOSTAP_SKIP_URL:
        case MSG_TYPE_DLNA_HOSTAP_SKIP_URL:
            win_msgbox_msg_open("Please connect WiFi first!", 3000, NULL, NULL);
            break;
		#endif
        default:
            break;
    }
    return ret;
	#endif
}

void win_cast_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

	if (ctl_msg->msg_type == MSG_TYPE_KEY){
	//remote or pan key process
	
	} else if (ctl_msg->msg_type > MSG_TYPE_KEY) {
		win_cast_msg_proc(arg1, arg2);
	}
	else{
		//ret = WIN_CTL_SKIP;
	}	
	//	  return ret;

}

bool win_cast_root_wait_open(uint32_t timeout)
{
    uint32_t count;
    count = timeout/20;

    while(count--)
    {
        if (m_win_cast_open)
            break;
        api_sleep_ms(20);
    }
    printf("%s(), m_win_cast_open(%d):%d\n", __func__, m_win_cast_open, count);
    return m_win_cast_open;
}

void ui_wifi_cast_init(void)
{
    screen_entry_t cast_root_entry;

    ui_wifi_cast_root = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_wifi_cast_root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_wifi_cast_root, event_handler, LV_EVENT_ALL, NULL);
    // lv_obj_set_style_bg_color(ui_wifi_cast_root, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_bg_opa(ui_wifi_cast_root, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_wifi_cast_root, LV_OPA_TRANSP, 0);

    cast_root_entry.screen = ui_wifi_cast_root;
    cast_root_entry.control = win_cast_control;
    api_screen_regist_ctrl_handle(&cast_root_entry);

    cast_main_ui_wait_init(win_cast_root_wait_open);
}
#endif
/*
win_des_t g_win_cast_root =
{
    .open = win_cast_open,
    .close = win_cast_close,
    .control = win_cast_control,
};
*/
