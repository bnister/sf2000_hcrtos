//The control center, process the message and key

#include <getopt.h>
#include <pthread.h>
#include <hcuapi/gpio.h>
#if defined(SUPPORT_FFPLAYER) || defined(__linux__)
#include <ffplayer.h>
#endif
#include "com_api.h"
#include "menu_mgr.h"
#include <hccast/hccast_wifi_mgr.h>
#include "cast_api.h"
#include "lvgl/lvgl.h"
#include "../lvgl/src/misc/lv_types.h"
#include "lv_drivers/display/fbdev.h"
#include "gpio_ctrl.h"
#include "data_mgr.h"
#include "osd_com.h"
#include "tv_sys.h"

#ifdef __HCRTOS__
#include <kernel/lib/fdt_api.h>
#endif

#define KEY_CHECK_SUPPORT

static volatile int m_sys_reset = 0;

#if 0

//mDNS protocal is used for aircast to discover the device
static int hc_mdnsd_start()
{
    FILE *fp = NULL;
    char result_buf[256];
    char parse_buf1[128];
    char parse_buf2[128];
    char parse_buf3[128];

    //step1: check if the mdnsd is ON
    fp = popen("ps", "r");
    if (!fp)
        return API_FAILURE;

    while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
    {
        if('\n' == result_buf[strlen(result_buf)-1])
        {
            result_buf[strlen(result_buf)-1] = '\0';
        }

        if(sscanf(result_buf,"%s %s %s", parse_buf1, parse_buf2, parse_buf3) == 3)
        {
            //check if mdns daemon is on
            if (strstr(parse_buf3, "mdnsd") != NULL)
            {
                printf("mDNS is already ON!\n");
                pclose(fp);
                return API_SUCCESS;
            }
        }
    }

    //step 2. start up msdns
    system("mdnsd");
    printf("hc_mdnsd_start() OK!\n");
    pclose(fp);
    return API_SUCCESS;

}

static int hc_dhcpc_start()
{
    FILE *fp = NULL;
    int ret = API_FAILURE;

    char result_buf[256];
    char parse_buf1[128];
    char parse_buf2[128];
    char parse_buf3[128];

    snprintf(result_buf, sizeof(result_buf)-1, "udhcpc -b -i %s -q", WLAN0_NAME);

    fp = popen(result_buf, "r");
    if (!fp)
        return API_FAILURE;

    printf("start hdcp: %s!\n", result_buf);

    while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
    {
        if('\n' == result_buf[strlen(result_buf)-1])
        {
            result_buf[strlen(result_buf)-1] = '\0';
        }

        if(sscanf(result_buf,"%s %s %s", parse_buf1, parse_buf2, parse_buf3) == 3)
        {
            //If the hdcp success, it return string like that: "adding dns 192.168.51.1"
            if (api_is_ip_addr(parse_buf3))
            {
                printf("DHCP success: %s!\n", parse_buf3);
                ret = API_SUCCESS;
                break;
            }
        }
    }
    if (!fp)
        pclose(fp);

    return ret;

}

static int hc_network_station_mode_init(void *wifi_handle)
{
    int ret = API_FAILURE;

//just for test wifi WPA command
    hccast_wifi_ap_info_t *wifi_info = NULL;
    hccast_wifi_ap_info_t *wifi_list = NULL;
    int ap_count;
    int i;

    ret = wifi_ap_scan(wifi_handle);
    if (ret == API_SUCCESS)
        printf(" *** wifi_ap_scan() OK!");
    ret = wifi_ap_list_get(wifi_handle, &wifi_list, &ap_count);
    if (ret == 0)
        printf("*** wifi_ap_list_get() OK, ap_count = %d!\n", ap_count);
    //list the ap
    for (i = 0; i < ap_count; i ++)
    {
        printf("ssid[%d]: %s, quality: %d%%\n", i, wifi_list[i].ssid, wifi_list[i].quality);
    }

    wifi_info = wifi_choose_ap_from_scan(wifi_handle);
    if (NULL == wifi_info)
        return API_FAILURE;

    printf("*** Connect to AP: %s ...!\n", wifi_info->ssid ? "wifi_info->ssid" : "NO AP");
    ret = wifi_ap_connect(wifi_handle, wifi_info);
    if (ret == API_SUCCESS)
        printf(" *** wifi_ap_connect() OK!");

    ret = hc_dhcpc_start();
    if (ret == API_SUCCESS)
    {
        control_msg_t ctl_msg = {0};
        ctl_msg.msg_type = MSG_TYPE_NETWORK_DHCP_ON;
        api_control_send_msg(&ctl_msg);
    }
    return API_SUCCESS;

}


#define DEFAULT_AP_ADDR  "192.168.68.1"
#define DEFAULT_AP_PSW  "12345678"
#define DEFAULT_HOSTAPD_CONF  "/tmp/hostapd.conf"
#define HOST_AP_CHNNNEL 6

/**
 *  the hostapd config file can like follow:
 *  ***************************************************
interface=wlan0
driver=nl80211
ctrl_interface_group=0
hw_mode=g
max_num_sta=2
ssid=hichip_wifi
auth_algs=1
channel=6
#country_code=CN
ieee80211d=1
wpa=2
wpa_key_mgmt=WPA-PSK
wpa_passphrase=12345678
rsn_pairwise=CCMP
wpa_pairwise=CCMP
*  ***************************************************
 */
static int hc_create_hostapd_conf(char *file_name)
{
    FILE *fp = NULL;
    char service_name[64] = {0};
    cast_get_service_name(CAST_TYPE_NONE, service_name, sizeof(service_name));

    fp = fopen(file_name, "w+");
    if (NULL == fp)
    {
        printf("Create hostapd conf fail!\n");
        return API_FAILURE;
    }
    fprintf(fp, "interface=%s\n", WLAN0_NAME);
    fprintf(fp, "driver=nl80211\n");
    fprintf(fp, "ctrl_interface_group=0\n");
    fprintf(fp, "hw_mode=g\n");
    fprintf(fp, "max_num_sta=2\n");
    fprintf(fp, "ssid=%s\n", service_name);
    fprintf(fp, "auth_algs=1\n");
    fprintf(fp, "channel=%d\n", HOST_AP_CHNNNEL);
    //fprintf(fp, "country_code=CN\n");
    //fprintf(fp, "ieee80211d=1\n");
    fprintf(fp, "wpa=2\n");
    fprintf(fp, "wpa_key_mgmt=WPA-PSK\n");
    fprintf(fp, "wpa_passphrase=%s\n", DEFAULT_AP_PSW);
    fprintf(fp, "rsn_pairwise=CCMP\n");
    fprintf(fp, "wpa_pairwise=CCMP\n");

    fflush(fp);
    int wfd = fileno(fp);
    if(wfd != -1)
        fsync(wfd);

    fclose(fp);
    return API_SUCCESS;
}

static int hc_network_ap_mode_init()
{
    FILE *fp = NULL;
    int ret = API_FAILURE;
    char result_buf[256];

    char *cmd1 = "ifconfig "WLAN0_NAME" up";
    char *cmd2 = "ifconfig "WLAN0_NAME " " DEFAULT_AP_ADDR;
    char *cmd3 = "hostapd "DEFAULT_HOSTAPD_CONF" -B";
    char *cmd4 = "dhcpcd";

    ret = hc_create_hostapd_conf(DEFAULT_HOSTAPD_CONF);
    if (API_FAILURE == ret)
    {
    }

    ret = api_shell_exe_result(cmd1);
    if (API_FAILURE == ret)
        return ret;
    ret = api_shell_exe_result(cmd2);
    if (API_FAILURE == ret)
        return ret;

    fp = popen(cmd3, "r");
    while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
    {
        if('\n' == result_buf[strlen(result_buf)-1])
        {
            result_buf[strlen(result_buf)-1] = '\0';
        }

        if (NULL != strstr(result_buf, "Failed"))
        {
            printf("*** [%s] fail!\n", cmd3);
            pclose(fp);
            return API_FAILURE;
        }
    }
    pclose(fp);

    ret = api_shell_exe_result(cmd4);
    if (API_FAILURE == ret)
        return ret;

    return API_SUCCESS;
}

static int hc_network_p2p_mode_init()
{
    return API_SUCCESS;
}


//AP + P2P mode
//Station + P2P mode
static void *hc_network_init_task(void *arg)
{
    int sys_ap_list_count = 0;
    void *wifi_handle = NULL;
    int ret;
    control_msg_t ctl_msg = {0};

    if (API_FAILURE == wifi_driver_module_init())
    {
        return NULL;
    }
    ctl_msg.msg_type = MSG_TYPE_NETWORK_MAC_OK;
    api_control_send_msg(&ctl_msg);


#if 1 //only test AP mode
    //if there is WIFI ap information in system, that means the WIFI AP is
    // configured, initialize WIFI to station mode to connect AP.
    sys_ap_list_count = wifi_sys_ap_init_list();
    if (sys_ap_list_count > 0)
    {
        wifi_handle = wifi_wpa_open(WLAN0_NAME);
        if (wifi_handle)
            printf("*** wifi_wpa_open() OK!\n");
        else
            return NULL;

        hc_network_station_mode_init(wifi_handle);
    }
    else
        hc_network_ap_mode_init();
#else
    ret = hc_network_ap_mode_init();
#endif


    ctl_msg.msg_type = MSG_TYPE_NETWORK_INIT_OK;
    api_control_send_msg(&ctl_msg);


    return NULL;
}

#endif

#if 0
static uint8_t detect_reset_press(uint32_t timeout)
{
    int time_cnt = 0;
    int gpio_val = -1;
    time_cnt = timeout/10;
    int print_cnt = 0;

    do{
        if (m_sys_reset)
            return 0;

        //GPIO for factory reset and reboot.
        gpio_val = gpio_get_input(GPIO_RESET_NUMBER);
        if (-1 == gpio_val)
            continue;

        if (GPIO_RESET_ACTIVE_VAL == gpio_val){
            if (print_cnt ++ > 10){
                printf("gpio active %d\n", gpio_val);
                print_cnt = 0;
            }
        } else {
            printf("gpio inactive %d\n", gpio_val);
            return 0;
        }
        api_sleep_ms(10);
    }while(time_cnt --);

    return 1;
}

static bool key_reset_proc()
{
    bool reset = false;
    uint8_t wifi_param_reset = 0;
    wifi_param_reset = detect_reset_press(1500);
    control_msg_t msg;

    if (wifi_param_reset == 1)
    {
        msg.msg_type = MSG_TYPE_KEY_TRIGER_RESET;
        api_control_send_msg(&msg);
        reset = true;
    }
    else
    {
#if (defined (AIRPLAY_ROTATE_SUPPORT) || (defined MIRCASTCAT_ROTATE_SUPPORT))
        // BOOL b_auto_rotate = sys_data_get_video_mirror_rotate();
        // sys_data_set_video_mirror_rotate(!b_auto_rotate);
#endif
    }
    return reset;
}

static void *key_monitor_task(void *arg)
{
    bool reset = false;
    control_msg_t ctl_msg = {0};
    int gpio_val = -1;


    if (INVALID_VALUE_32 == GPIO_RESET_NUMBER || !gpio_configure(GPIO_RESET_NUMBER, GPIO_DIR_INPUT))
        return NULL;

    for(;;)
    {
#if 0
        //test GPIO out
        static int count = 0;

        gpio_configure(GPIO_RESET_NUMBER, GPIO_DIR_OUTPUT);

        if (count == 0){
            printf("%s(), set GPIO%d 0\n", __func__, GPIO_RESET_NUMBER);
            gpio_set_output(GPIO_RESET_NUMBER, 0);

            api_sleep_ms(300);

            gpio_configure(GPIO_RESET_NUMBER, GPIO_DIR_INPUT);
            gpio_val = gpio_get_input(GPIO_RESET_NUMBER);
            printf("%s(), get GPIO%d %d\n", __func__, GPIO_RESET_NUMBER, gpio_val);

        }
        else if (count == 100){
            printf("%s(), set GPIO%d 1\n", __func__, GPIO_RESET_NUMBER);
            gpio_set_output(GPIO_RESET_NUMBER, 1);

            api_sleep_ms(300);

            gpio_configure(GPIO_RESET_NUMBER, GPIO_DIR_INPUT);
            gpio_val = gpio_get_input(GPIO_RESET_NUMBER);
            printf("%s(), get GPIO%d %d\n", __func__, GPIO_RESET_NUMBER, gpio_val);

        }
        else if (count == 200){
            count = 0;
            continue;
        }

        count ++;

        api_sleep_ms(30);
#else

        if (m_sys_reset)
            break;

        //reset GPIO is press
        gpio_val = gpio_get_input(GPIO_RESET_NUMBER);
        if (GPIO_RESET_ACTIVE_VAL == gpio_val){
            reset = key_reset_proc();
            if (!reset){
                ctl_msg.msg_type = MSG_TYPE_ENTER_STANDBY;
                printf("GPIO trigger standby!\n");
                api_control_send_msg(&ctl_msg);
            }
            break;
        }

        api_sleep_ms(20);
#endif        
        
    }
    return NULL;
}
#else

static void key_cast_rotate(bool is_active)
{
    int rotate_type = ROTATE_TYPE_0;

    if (data_mgr_cast_rotation_get()){
        rotate_type = ROTATE_TYPE_0;
        data_mgr_cast_rotation_set(0);
        // fbdev_set_rotate(0, 0, 0);
    }
    else{
        rotate_type = ROTATE_TYPE_270;
        data_mgr_cast_rotation_set(1);
        // fbdev_set_rotate(90, 0, 0);
    }

#ifdef USBMIRROR_SUPPORT 
    extern hccast_um_param_t *hccast_um_param_get();

    hccast_um_param_t um_param_set = {0};
    hccast_um_param_t *um_param_get;
    um_param_get = hccast_um_param_get();
    memcpy(&um_param_set, um_param_get, sizeof(hccast_um_param_t));
    if (data_mgr_cast_rotation_get())
        um_param_set.screen_rotate_en = 1;
    else
        um_param_set.screen_rotate_en = 0;
    hccast_um_param_set(&um_param_set);
#endif


#if defined(SUPPORT_FFPLAYER) || defined(__linux__)
    //api_logo_reshow();
    void *player = api_ffmpeg_player_get();
    if(player !=NULL && !is_active){
        hcplayer_change_rotate_type(player, rotate_type);
        hcplayer_change_mirror_type(player, MIRROR_TYPE_NONE);
    }
#endif

}

static void *key_monitor_task(void *arg)
{
    int gpio_val = -1;
    int count = 0;
    int active_num = 0;
    int inactive_num = 0;
    bool is_active = false;
    control_msg_t msg = {0};

    int long_press_cnt = 1800/20; //ms
    int short_press_cnt = 500/20;
    int reset_gpio = INVALID_VALUE_32;
    int reset_active = 0;

#ifdef __HCRTOS__    
    const char *st;
    int np;
    np = fdt_node_probe_by_path("/hcrtos/reset_gpio");
    if(np>=0){
        fdt_get_property_string_index(np, "status", 0, &st);
        if (!strcmp(st, "okay")){
            fdt_get_property_u_32_index(np, "gpio_num", 0, &reset_gpio);
            fdt_get_property_u_32_index(np, "gpio_active", 0, &reset_active);
        }
    }

#else    

    #define GPIO_CONFIG_PATH "/proc/device-tree/hcrtos/reset_gpio"

    char status[16] = {0};
    api_dts_string_get(GPIO_CONFIG_PATH "/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        reset_gpio = api_dts_uint32_get(GPIO_CONFIG_PATH "/gpio_num");
        reset_active = api_dts_uint32_get(GPIO_CONFIG_PATH "/gpio_active");
    }
#endif

    printf("%s(), line:%d. reset gpio:%d, active:%d!\n", __func__, __LINE__, (int)reset_gpio, (int)reset_active);
    if (INVALID_VALUE_32 == reset_gpio)
        return NULL;

    gpio_configure(reset_gpio, GPIO_DIR_INPUT);

    while(1)
    {
        gpio_val = gpio_get_input(reset_gpio);
        if(gpio_val == reset_active){
            active_num++;
           
            if(active_num>=long_press_cnt){
                // data_mgr_factory_reset();
                // api_system_reboot();
                msg.msg_type = MSG_TYPE_KEY_TRIGER_RESET;
                api_control_send_msg(&msg);
                break;

            } 
            is_active = true;
            inactive_num=0;
        }else{
            if(is_active){
                count++;
                is_active = false;
            }
            if(count>0){
                inactive_num++;
                if(inactive_num>=short_press_cnt){
                    if(count==1){
                        printf("%s(), line:%d. switch rotate!\n", __func__, __LINE__);
                        key_cast_rotate(is_active);
                    }else if(count == 3){
                        printf("%s(), line:%d. 720P!\n", __func__, __LINE__);
                        tv_sys_app_set(APP_TV_SYS_720P); 
                    }else if(count == 5){
                        printf("%s(), line:%d. 1080P!\n", __func__, __LINE__);
                        tv_sys_app_set(APP_TV_SYS_1080P);
                    }
                    count=0;
                }
            }

            active_num = 0;
        }    
        api_sleep_ms(20);
    }
    return NULL;
}

#endif


static void key_monitor()
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, key_monitor_task, NULL)) {
        return;
    }
}

static void sys_reboot_func(void *user_data)
{
    (void)user_data;
    api_system_reboot();
    m_sys_reset = 0;
}

static void sys_standby_func(void *user_data)
{
    (void)user_data;

    api_system_standby();
}

static void win_demo_str_show()
{
#if 0
    static lv_obj_t *demo_label = NULL;
    if (NULL == demo_label)
    {   
        demo_label = lv_label_create(lv_layer_top());
        
        lv_obj_set_style_text_font(demo_label, &lv_font_montserrat_22, 0); 
        lv_obj_set_style_text_color(demo_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(demo_label, LV_ALIGN_TOP_LEFT, 20, 20);
    }
    if (cast_is_demo())
        lv_label_set_text(demo_label, "demo");
    else
        lv_label_set_text(demo_label, " ");
#endif
}

static bool m_wifi_plug_out = false;
static win_ctl_result_t common_msg_proc(control_msg_t *msg)
{
    win_ctl_result_t ctl_result = WIN_CTL_NONE;

    switch(msg->msg_type)
    {
        case MSG_TYPE_KEY_TRIGER_RESET:
            m_sys_reset = 1;
            printf("%s(): factory reset!!\n", __func__);
            data_mgr_factory_reset();
            win_msgbox_msg_open("System reset, then reboot ...", 2000, sys_reboot_func, NULL);
            break;
        case MSG_TYPE_ENTER_STANDBY:
            win_msgbox_msg_open("Entering system standby ...", 2000, sys_standby_func, NULL);
            break;
    #ifdef WIFI_SUPPORT
        case MSG_TYPE_USB_WIFI_PLUGIN:
            if (!m_wifi_plug_out){
                network_connect();
                m_wifi_plug_out = false;
            }
            break;
        case MSG_TYPE_USB_WIFI_PLUGOUT:
            network_plugout_reboot();
            hccast_stop_services();
            win_msgbox_msg_open("WiFi plug out, reboot now ...", 2000, sys_reboot_func, NULL);
            //network_disconnect;
            m_wifi_plug_out = true;
            break;
    #endif
        case MSG_TYPE_AIR_INVALID_CERT:
            win_demo_str_show();
        default:
            break;
    }

    return ctl_result;
}

void hc_control_process(void)
{
    control_msg_t ctl_msg;
    win_des_t *cur_win = NULL;
    win_ctl_result_t ctl_result = WIN_CTL_NONE;
    int ret = -1;

    do
    {
        //get message
        ret = api_control_receive_msg(&ctl_msg);
        if (0 != ret)
        {
#if 0
            //get key
            ret = api_get_key();
#endif
            if (0 != ret)
                break;
        }

        cur_win = menu_mgr_get_top();
        if (cur_win)
        {
            ctl_result = cur_win->control((void*)(&ctl_msg), NULL);

            if (ctl_result == WIN_CTL_PUSH_CLOSE ||
                ctl_result == WIN_CTL_POPUP_CLOSE )
            {
                cur_win->close((void*)(ctl_result));

                if (ctl_result == WIN_CTL_POPUP_CLOSE)
                {
                    //popup current menu, back to parent menu.
                    menu_mgr_pop();
                }

                cur_win = menu_mgr_get_top();
                if (cur_win)
                {
                    printf("open next win!\n");
                    cur_win->open(cur_win->param);
                }
            }
            else if (ctl_result == WIN_CTL_SKIP)
            {
                //menu do not process the message or key, may process here
            }
        }

        ctl_result = common_msg_proc(&ctl_msg);

    }
    while(0);

}

void hc_control_init()
{
    win_des_t *cur_win;

    menu_mgr_init();
#ifdef KEY_CHECK_SUPPORT    
    key_monitor();
#endif    

    //Set the screen to transprent
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(lv_scr_act(), LV_OPA_TRANSP, 0);
    
    osd_obj_com_set();
    
    cur_win = &g_win_um_play;
    //cur_win = &g_win_dlna_play;

#ifdef USBMIRROR_SUPPORT
    cast_usb_mirror_init();
    cast_usb_mirror_start();
#endif
    sys_upg_usb_check(5000);
    cur_win->open(cur_win->param);
    menu_mgr_push(cur_win);

}


