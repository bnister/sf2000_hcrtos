#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <hcuapi/dis.h>
#include <kernel/notify.h>
#include <linux/notifier.h>
#include <hcuapi/sys-blocking-notify.h>
#include <ffplayer.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <lvgl/hc_src/hc_lvgl_init.h>

#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include <kernel/fb.h>


#include "com_api.h"
//#include "cast_api.h"
#include "os_api.h"
#include "glist.h"


static uint32_t m_control_msg_id = INVALID_ID;
static cast_play_state_t m_cast_play_state = CAST_STATE_IDLE;
static bool m_ffplay_init = false;

static int usb_state = USB_STAT_INVALID;
int mmp_get_usb_stat(void)
{
    return usb_state;
}
static int usbd_notify(struct notifier_block *self,
                           unsigned long action, void* dev)
{
        switch (action) {
        case USB_MSC_NOTIFY_MOUNT:
              usb_state = USB_STAT_MOUNT;
              break;
        case USB_MSC_NOTIFY_UMOUNT:
              usb_state= USB_STAT_UNMOUNT;
              break;
        default:
              return 0;
        }

        if(usb_state== USB_STAT_UNMOUNT)
        {
           control_msg_t msg;
           memset(&msg, 0, sizeof(control_msg_t));
           msg.msg_code = MSG_TYPE_USB_UNMOUNT;
           msg.msg_type = MSG_TYPE_MSG;
           api_control_send_msg(&msg);
        }
        return NOTIFY_OK;
}

static struct notifier_block usb_switch_nb = {
       .notifier_call = usbd_notify,
};

int api_system_init()
{
    sys_register_notify(&usb_switch_nb);

    return 0;
}

int api_video_init()
{
    return 0;
}

int api_audio_init()
{
    return 0;
}

void fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p);

static void my_monitor_cb(lv_disp_drv_t * disp_drv, uint32_t time, uint32_t px)
{
	  printf("%lu px refreshed in %lu ms\n", px, time);
}

int api_lvgl_init(int width, int height)
{

    return hc_lvgl_init();
}

static void *m_logo_player = NULL;
int api_logo_show(const char *file)
{

    char *file_path = file;
    
    api_logo_off();

    if (!file)
        file_path = BACK_LOGO;

    HCPlayerInitArgs player_args;
	memset(&player_args, 0, sizeof(player_args));
    player_args.uri = file_path;
	player_args.msg_id = -1;
    m_logo_player = hcplayer_create(&player_args);
    if (!m_logo_player){
        printf("hcplayer_create() fail!\n");
        return -1;
    }
    //not block play, create a task to play.
    //hcplayer_play(m_logo_player);
    
    //block play
    hcplayer_play2(m_logo_player);

    //stop 
    //hcplayer_stop2(m_logo_player, false, false);
    printf("Show logo: %s ok!\n", file_path);
    return 0;
}

int api_logo_off()
{
    if (m_logo_player)
        hcplayer_stop(m_logo_player);
    m_logo_player = NULL;
    return 0;
}

/**
 * @brief turn on/off the video frame output
 * 
 * @param on_off 
 * @return int 
 */
int api_dis_show_onoff(bool on_off)
{
    int fd = -1;
    struct dis_win_onoff winon = { 0 };

    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0) {
        return -1;
    }

    winon.distype = DIS_TYPE_HD;
    winon.layer =  DIS_LAYER_MAIN;
    winon.on = on_off ? 1 : 0;
    
    ioctl(fd, DIS_SET_WIN_ONOFF, &winon);
    close(fd);

    return 0;
}

int api_control_send_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == m_control_msg_id){
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id){
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, control_msg, sizeof(control_msg_t));
}

int api_control_receive_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == m_control_msg_id){
        return -1;
    }
    return api_message_receive_tm(m_control_msg_id, control_msg, sizeof(control_msg_t), 5);
}

int api_control_send_key(uint32_t key)
{
    control_msg_t control_msg;
    control_msg.msg_type = MSG_TYPE_MSG;
    control_msg.msg_code = key;

    if (INVALID_ID == m_control_msg_id){
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id){
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, &control_msg, sizeof(control_msg_t));
}



cast_play_state_t api_cast_play_state_get(void)
{
    return m_cast_play_state;
}
void api_cast_play_state_set(cast_play_state_t state)
{
    m_cast_play_state = state;
}

/** check the string if it is IP address
 * @param
 * @return
 */
bool api_is_ip_addr(char *ip_buff)
{
    int ip1, ip2, ip3, ip4;
    char temp[64];
    if((sscanf(ip_buff,"%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4))!=4)
        return false;
    sprintf(temp,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
    if(strcmp(temp, ip_buff) != 0)
        return false;
    if(!((ip1 <= 255 && ip1 >= 0)&&(ip2 <= 255 && ip2 >= 0)&&(ip3 <= 255 && ip1 >= 0)))
        return false;
    else
        return true;
}


#define ETHE_MAC_PATH "/sys/class/net/eth0/address"
#define WIFI_MAC_PATH "/sys/class/net/wlan0/address"

#define MAC_ADDRESS_PATH   WIFI_MAC_PATH// ETHE_MAC_PATH
/**get the mac address, after modprobe cmmand, the ip address of wlan0
 * should be save tho "/sys/class/net/wlan0/address"
 * @param
 * @return
 */
int api_get_mac_addr(char *mac) 
{
    int ret;
    char buffer[32] = {0};
    FILE *fp = fopen(MAC_ADDRESS_PATH, "r");
    if (mac == NULL) return -1;
    if (fp == NULL) return -1;
    fread(buffer, 1, 32, fp);
    fclose(fp);
    buffer[strlen(buffer)-1] = '\0';
    printf(MAC_ADDRESS_PATH "=%s\n", buffer);
    ret = sscanf(buffer, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
            &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    if(ret == 6)
        return 0;
    else
        return -1;

}


void app_ffplay_init(void)
{
    if (m_ffplay_init) return;

    hcplayer_init(5);
    m_ffplay_init = true;
}

void app_ffplay_deinit(void)
{
    hcplayer_deinit();
    m_ffplay_init = false;
}

/**
 * @brief linux system will send the exit signal, then release system resource here
 * 
 */
void app_exit(void)
{
    if (INVALID_ID != m_control_msg_id)
        api_message_delete(m_control_msg_id);

    app_ffplay_deinit();
#ifdef CAST_AIRPLAY_ENABLE    
    cast_airpaly_close();
#endif
#ifdef CAST_DLNA_ENABLE    
    cast_dlna_close();
#endif
#ifdef CAST_MIRACAST_ENABLE    
    cast_miracast_close();
#endif    
}

void api_sleep_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

int api_shell_exe_result(char *cmd)
{
    int ret = API_FAILURE;
    char result_buf[32];
    int result_val = -1;
    FILE *fp = NULL;

    //step1: excute the shell command
    system(cmd);

    //step2: get the excuting result
    fp = popen("echo $?", "r");
    if (fgets(result_buf, sizeof(result_buf), fp) != NULL){
        if('\n' == result_buf[strlen(result_buf)-1]){
            result_buf[strlen(result_buf)-1] = '\0';
        }

        sscanf(result_buf,"%d", &result_val);
    }
    pclose(fp);
    if (0 == result_val){
        printf("Excute cmd: [%s] OK!\n", cmd);
        ret = API_SUCCESS;
    }else{
        printf("Excute cmd: [%s] fail, ret = %d!\n", cmd, result_val);
        ret = API_FAILURE;
    }

    return ret;

}
//send message to other task
int api_control_send_media_message(uint32_t mediamsg_type)
{
    control_msg_t control_msg;
    control_msg.msg_type = MSG_TYPE_MSG;
    control_msg.msg_code = mediamsg_type;

    if (INVALID_ID == m_control_msg_id){
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id){
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, &control_msg, sizeof(control_msg_t));

}
static glist *m_screen_list = NULL;
void api_screen_regist_ctrl_handle(screen_entry_t *entry)
{
    screen_entry_t *entry_tmp;
    glist *glist_tmp = NULL;

    glist_tmp = m_screen_list;
    while(glist_tmp){
        entry_tmp = glist_tmp->data;
        if (entry_tmp->screen == entry->screen){
            entry_tmp->control = entry->control;
            return;
        }
        glist_tmp = glist_tmp->next;
    }

    entry_tmp = (screen_entry_t*)malloc(sizeof(screen_entry_t));
    memcpy(entry_tmp, entry, sizeof(screen_entry_t));
    //printf("%s(), screen:%x, ctrl:%x\n", __func__, entry_tmp->screen, entry_tmp->control);
    m_screen_list = glist_append(m_screen_list, (void*)entry_tmp);
}

screen_ctrl api_screen_get_ctrl(void *screen)
{
    screen_entry_t *entry_tmp;
    glist *glist_tmp = NULL;

    glist_tmp = m_screen_list;
    while(glist_tmp){
        entry_tmp = glist_tmp->data;
         printf("%s(), entry_tmp->screen:%x,%x, screen, ctrl:%x\n", __func__, \
             entry_tmp->screen, screen, entry_tmp->control);
        if (entry_tmp->screen == screen){
            return entry_tmp->control;
        }
        glist_tmp = glist_tmp->next;
    }

    return NULL;
}

int api_osd_show_onoff(bool on_off)
{
    // Open the file for reading and writing
    int fbfd = open("/dev/fb0", O_RDWR);
    uint32_t blank_mode;

    if(fbfd == -1) {
        printf("%s(), line: %d. Error: cannot open framebuffer device", __func__, __LINE__);
        return API_FAILURE;
    }

    if (on_off)
        blank_mode = FB_BLANK_UNBLANK;
    else
        blank_mode = FB_BLANK_NORMAL;

    if (ioctl(fbfd, FBIOBLANK, blank_mode) != 0) {
        printf("%s(), line: %d. Error: FBIOBLANK", __func__, __LINE__);
    }

    close(fbfd);
    return API_SUCCESS;
}

static volatile int m_osd_off_time_cnt = 0;
#define ONE_COUNT_TIME  200
static void *osd_off_for_time(void *arg)
{
    uint32_t timeout = (uint32_t)arg;
    m_osd_off_time_cnt = timeout/ONE_COUNT_TIME;
    do{
        api_sleep_ms(ONE_COUNT_TIME);
    }while(m_osd_off_time_cnt --);

    m_osd_off_time_cnt = 0;
    api_osd_show_onoff(true);

    return NULL;
}

//Turn off OSD for a time, then turn on OSD.
//because sometimes enter dlna music play, the OSD is still show
//but the video screen is black(BUG #2848), so we turn off OSD for some time.
void api_osd_off_time(uint32_t timeout)
{

    //update the wait time
    if (m_osd_off_time_cnt){
        int timeout_cnt = timeout/ONE_COUNT_TIME;
        if (timeout_cnt > m_osd_off_time_cnt)
            m_osd_off_time_cnt = timeout_cnt;

        return;
    }

    api_osd_show_onoff(false);
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x1000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&thread_id, &attr, osd_off_for_time, (void*)timeout)) {
        return;
    }
}
static volatile bool m_hotkey_act = true;
bool api_hotkey_act_get(void)
{
    return m_hotkey_act;
}

void api_hotkey_act_set(bool act)
{
    m_hotkey_act = act;
}


