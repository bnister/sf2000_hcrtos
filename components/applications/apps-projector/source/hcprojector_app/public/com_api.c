#include "app_config.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <hcuapi/dis.h>

#ifdef __HCRTOS__
#include <hcuapi/watchdog.h>

#include <kernel/io.h>
#include <kernel/notify.h>
#include <linux/notifier.h>
#include <hcuapi/sys-blocking-notify.h>
#include <freertos/FreeRTOS.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/fb.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#include <sys/mount.h>
#endif

#else
#include <sys/epoll.h>
#include <sys/socket.h>
#include <linux/fb.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <linux/watchdog.h>
#endif


#include <ffplayer.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include <hcuapi/vidsink.h>
#include <hcuapi/common.h>
#include <hcuapi/kumsgq.h>
#include <hcuapi/hdmi_tx.h>

#include "network_api.h"
#include <hcuapi/fb.h>
#include <hcuapi/standby.h>
#include <lvgl/hc_src/hc_lvgl_init.h>
#include <dirent.h>

#ifdef WIFI_SUPPORT
#include <net/if.h>
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_net.h>
#endif

#include <hcuapi/input.h>
#include <hcuapi/input-event-codes.h>

#include <hcuapi/snd.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "gpio_ctrl.h"

#include "app_config.h"
#include "factory_setting.h"
#include "setup.h"

#include "screen.h"

#include "tv_sys.h"
#include "com_api.h"
#include "os_api.h"
#include "glist.h"
#include "hcuapi/lvds.h"
#ifdef __HCRTOS__
#include <hcuapi/gpio.h>
#include <nuttx/wqueue.h>
#endif

static uint32_t m_control_msg_id = INVALID_ID;
static cast_play_state_t m_cast_play_state = CAST_STATE_IDLE;
static bool m_ffplay_init = false;
static int m_usb_state = USB_STAT_INVALID;
static partition_info_t* partition_info=NULL;

static volatile int m_osd_off_time_cnt = 0;
static volatile char m_osd_off_time_flag = 0;
static pthread_mutex_t m_osd_off_mutex = PTHREAD_MUTEX_INITIALIZER;

int mmp_get_usb_stat(void)
{
    return m_usb_state;
}

static void mmp_set_usb_stat(int state)
{
    m_usb_state = state;
}

//return usb device: /meida/hdd(linux), /meida/sda (rtos)
static char mounted_dev_name[64];
char *mmp_get_usb_dev_name(void)
{
    char *dev_name = glist_nth_data(partition_info->dev_list, 0);
    if (!dev_name)
        return NULL;
    
#ifdef __HCRTOS__
    sprintf(mounted_dev_name, "%s/%s", MOUNT_ROOT_DIR, dev_name);
#else
    strcpy(mounted_dev_name, dev_name);
#endif
    return mounted_dev_name;
}

static glist* partition_glist_find(char * string_to_find)
{
    glist* glist_found = NULL;
    glist* temp_glist = (glist*)partition_info->dev_list;
    while(temp_glist){
        if(temp_glist!=NULL&&temp_glist->data!=NULL){
            if(strcmp(string_to_find,temp_glist->data)==0){
                glist_found = temp_glist;
                break;
            }
            else{
                temp_glist=temp_glist->next;
            }
                
        }
    }
    
    return glist_found;
}

static int partition_delnode_form_glist(char * string_to_find)
{
    int ret = 0;
    glist* glist_del=NULL;

    glist_del = partition_glist_find(string_to_find);
    if(glist_del){
        if(glist_del->data){
            free(glist_del->data);
            glist_del->data=NULL;//free data 
        }
        partition_info->dev_list=glist_delete_link(partition_info->dev_list,glist_del);
        glist_del=NULL;
        ret = 0;
    } else {
        ret = -1;
    }
    return ret;
}
static int partition_info_update(int usb_state,void* dev)
{
    if(usb_state==USB_STAT_MOUNT||usb_state==SD_STAT_MOUNT){
        char *dev_name = NULL;
        if (partition_glist_find((char*)dev))
            return 0;

        partition_info->count++;
        dev_name = strdup((char*)dev);
        partition_info->dev_list=glist_append((glist*)partition_info->dev_list, dev_name);
    }else if(usb_state==USB_STAT_UNMOUNT||usb_state==SD_STAT_UNMOUNT||usb_state==SD_STAT_UNMOUNT_FAIL||usb_state==USB_STAT_UNMOUNT_FAIL){
        if (!partition_delnode_form_glist(dev)){
            partition_info->count--;
            if (partition_info->count < 0)
                partition_info->count = 0;
        }

    }
    partition_info->m_storage_state=usb_state;
    return 0;
}

static int partition_info_init()
{
    if(partition_info==NULL){
        partition_info=(partition_info_t *)malloc(sizeof(partition_info_t));
        memset(partition_info,0,sizeof(partition_info_t));
    }
    return 0;
}

void* mmp_get_partition_info()
{
    return partition_info;
}


static void _hotplug_hdmi_tx_tv_sys_set(void)
{
    int ap_tv_sys;
    int ap_tv_sys_ret;
    int last_tv_sys;

    last_tv_sys = projector_get_some_sys_param(P_DE_TV_SYS);
    ap_tv_sys = projector_get_some_sys_param(P_SYS_RESOLUTION);
    ap_tv_sys_ret = tv_sys_app_auto_set(ap_tv_sys, 2000);
    if (ap_tv_sys_ret >= 0)
    {
        if (APP_TV_SYS_AUTO == ap_tv_sys)
            sysdata_app_tv_sys_set(APP_TV_SYS_AUTO);
        else
            sysdata_app_tv_sys_set(ap_tv_sys_ret);
        projector_sys_param_save();
    }

#ifdef CAST_SUPPORT
    if (((last_tv_sys < TV_LINE_4096X2160_30) && (projector_get_some_sys_param(P_DE_TV_SYS) >= TV_LINE_4096X2160_30)) \
            || ((last_tv_sys >= TV_LINE_4096X2160_30) && (projector_get_some_sys_param(P_DE_TV_SYS) < TV_LINE_4096X2160_30)))
    {
        control_msg_t ctl_msg = {0};
        ctl_msg.msg_type = MSG_TYPE_HDMI_TX_CHANGED;
        api_control_send_msg(&ctl_msg);

    }
#endif

}

#ifdef __HCRTOS__

#ifdef WIFI_SUPPORT

typedef struct _wifi_model_st_
 {
    char name[16];
    char desc[16];
    int  type;
 } wifi_model_st;

wifi_model_st wifi_model_list[] =
{
    {"", "", HCCAST_NET_WIFI_NONE},
#ifdef WIFI_RTL8188FU_SUPPORT
    {"rtl8188fu", "v0BDApF179", HCCAST_NET_WIFI_8188FTV},
#endif
#ifdef WIFI_RTL8188ETV_SUPPORT
    {"rtl8188eu", "v0BDAp0179", HCCAST_NET_WIFI_8188FTV},
    {"rtl8188eu", "v0BDAp8179", HCCAST_NET_WIFI_8188FTV},
#endif
#ifdef WIFI_RTL8733BU_SUPPORT
    {"rtl8733bu", "v0BDApB733", HCCAST_NET_WIFI_8733BU},
    {"rtl8733bu", "v0BDApF72B", HCCAST_NET_WIFI_8733BU},
#endif   
    //{"rtl8811cu", "v0BDApC811", HCCAST_NET_WIFI_8811FTV},
};

wifi_model_st g_hotplug_wifi_model = {0};

#include <linux/usb.h>

static int usb_wifi_notify_check(unsigned long action, void *dev)
{
    unsigned char found = 0;
    control_msg_t ctl_msg = {0};
    char devpath[64] = { 0 };
	struct removable_notify_info *notify_info =
             (struct removable_notify_info *)dev;

    for (int index = 0; \
            index < sizeof(wifi_model_list)/sizeof(wifi_model_list[0]); \
            index++)
    {
        if(strncmp(wifi_model_list[index].desc,
                 notify_info->devname,
                 strlen(notify_info->devname)))
        {
            if(index == sizeof(wifi_model_list)/sizeof(wifi_model_list[0]) - 1)
                return -1;
            continue;
        }
        else
            break;
    }

    switch (action)
    {
        case USB_DEV_NOTIFY_CONNECT:
        {
            char path[64] = {0};
            FILE* fp;

            printf("Wi-Fi Plug In -> ");

            for (int i = 0; i < sizeof(wifi_model_list)/sizeof(wifi_model_list[0]); i++)
            {
                snprintf(path, sizeof(path) - 1, "/dev/bus/usb/%s", wifi_model_list[i].desc);
                fp = fopen(path, "rb");
                if (fp)
                {
                    fclose(fp);
                    found = 1;
                    printf("Model: %s\n", wifi_model_list[i].name);

                    memcpy(&g_hotplug_wifi_model, &wifi_model_list[i], sizeof(g_hotplug_wifi_model));
                    
                    network_wifi_module_set(g_hotplug_wifi_model.type);
                    ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGIN;
                    api_control_send_msg(&ctl_msg);
                    break;
                }
            }

            if (!found)
            {
                printf ("Unsupport Model\n");
            }
            // snd msg

            break;
        }
        case USB_DEV_NOTIFY_DISCONNECT:
        {
            if (HCCAST_NET_WIFI_NONE != g_hotplug_wifi_model.type){
                printf("Wi-Fi Plug Out -> model: %s\n", g_hotplug_wifi_model.name);
                memset (&g_hotplug_wifi_model, 0, sizeof(g_hotplug_wifi_model));
                network_wifi_module_set(0);
                ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGOUT;
                api_control_send_msg(&ctl_msg);
            }
            break;
        }     
        default:
            break;
    }

    return NOTIFY_OK;
}
#endif



static int usbd_notify(struct notifier_block *self,
                           unsigned long action, void* dev)
{
    control_msg_t msg = {0};
    memset(&msg, 0, sizeof(control_msg_t));
    switch (action) {
    case USB_MSC_NOTIFY_MOUNT:
        if(strstr(dev,"sd")){
            m_usb_state = USB_STAT_MOUNT;
        }else if(strstr(dev,"mmc")){
            m_usb_state = SD_STAT_MOUNT;
        }
        if (dev)
		  printf("USB_STAT_MOUNT: %s\n", dev);
        break;
    case USB_MSC_NOTIFY_UMOUNT:
        if(strstr(dev,"sd")){
            m_usb_state = USB_STAT_UNMOUNT;
        }else if(strstr(dev,"mmc")){
            m_usb_state = SD_STAT_UNMOUNT;
        }        
        break;
    case USB_MSC_NOTIFY_MOUNT_FAIL:
        if(strstr(dev,"sd")){
            m_usb_state = USB_STAT_MOUNT_FAIL;
        }else if(strstr(dev,"mmc")){
            m_usb_state = SD_STAT_MOUNT_FAIL;
        }
        break;
    case USB_MSC_NOTIFY_UMOUNT_FAIL:
        if(strstr(dev,"sd")){
            m_usb_state = USB_STAT_UNMOUNT_FAIL;
        }else if(strstr(dev,"mmc")){
            m_usb_state = SD_STAT_UNMOUNT_FAIL;
        }
        break;
    default:
    #ifdef WIFI_SUPPORT
        usb_wifi_notify_check(action, dev);
    #endif
        return NOTIFY_OK;
        break;
    }
    if(m_usb_state== USB_STAT_UNMOUNT)
    {
       msg.msg_type = MSG_TYPE_USB_UNMOUNT;
       api_control_send_msg(&msg);
    }
    partition_info_update(m_usb_state,dev);
    // partition_info_update(temp,dev);

    return NOTIFY_OK;
}

static struct notifier_block usb_switch_nb = {
       .notifier_call = usbd_notify,
};
#ifdef WIFI_SUPPORT
static void* hotplug_first_wifi_detect_probed(void* arg)
{
    FILE* fp;
    control_msg_t ctl_msg = {0};

    do
    {
        char path[64] = {0};
        for (int i = 0; i < sizeof(wifi_model_list)/sizeof(wifi_model_list[0]); i++)
        {
            snprintf(path, sizeof(path) - 1, "/dev/bus/usb/%s", wifi_model_list[i].desc);
            fp = fopen(path, "rb");
            if (fp)
            {
                fclose(fp);
                printf("detect wifi model: %s\n", wifi_model_list[i].name);

                memcpy(&g_hotplug_wifi_model, &wifi_model_list[i], sizeof(g_hotplug_wifi_model));
                network_wifi_module_set(g_hotplug_wifi_model.type);
                ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGIN;
                api_control_send_msg(&ctl_msg);
                
                goto EXIT;
            }
        }

        usleep(500*1000);
    }
    while (1);

EXIT:
    return NULL;
}
#endif

static void _notifier_hdmi_tx_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    return ;
}

static void _notifier_hdmi_tx_plugout(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);
}

static void _notifier_hdmi_tx_ready(void *arg, unsigned long param){

    struct hdmi_edidinfo *edid = (struct hdmi_edidinfo *)param;
    printf("%s(), best_tvsys: %d\n", __func__, edid->best_tvsys);
    _hotplug_hdmi_tx_tv_sys_set();
}

static int hotplug_hdmi_tx_init(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    struct work_notifier_s notify_plugin;
    struct work_notifier_s notify_plugout;
    struct work_notifier_s notify_edid;
    memset(&notify_plugin, 0, sizeof(notify_plugin));
    memset(&notify_plugout, 0, sizeof(notify_plugout));
    memset(&notify_edid, 0, sizeof(notify_edid));

    notify_plugin.evtype   = HDMI_TX_NOTIFY_CONNECT;
    notify_plugin.qid          = LPWORK;
    notify_plugin.remote   = false;
    notify_plugin.oneshot  = false;
    notify_plugin.qualifier  = NULL;
    notify_plugin.arg          = NULL;
    notify_plugin.worker2  = _notifier_hdmi_tx_plugin;
    work_notifier_setup(& notify_plugin);

    notify_plugout.evtype   = HDMI_TX_NOTIFY_DISCONNECT;
    notify_plugout.qid          = LPWORK;
    notify_plugout.remote   = false;
    notify_plugout.oneshot  = false;
    notify_plugout.qualifier  = NULL;
    notify_plugout.arg          = NULL;
    notify_plugout.worker2  = _notifier_hdmi_tx_plugout;
    work_notifier_setup(& notify_plugout);

    notify_edid.evtype = HDMI_TX_NOTIFY_EDIDREADY;
    notify_edid.qid = LPWORK;
    notify_edid.remote = false;
    notify_edid.oneshot = false;
    notify_edid.qualifier = NULL;
    notify_edid.arg = (void*)(&notify_edid);
    notify_edid.worker2 = _notifier_hdmi_tx_ready;
    work_notifier_setup(& notify_edid);

    return 0;
}


static int hotplug_init()
{
    pthread_t pid;
    sys_register_notify(&usb_switch_nb);
    hotplug_hdmi_tx_init();
#ifdef WIFI_SUPPORT
    pthread_create(&pid,NULL,hotplug_first_wifi_detect_probed,NULL);
#endif
    return 0;
}

#else

#define MX_EVENTS (10)
#define EPL_TOUT (1000)

static int hotplug_init();

static int m_wifi_plugin = 0;

enum EPOLL_EVENT_TYPE
{
    EPOLL_EVENT_TYPE_KUMSG = 0,
    EPOLL_EVENT_TYPE_HOTPLUG_CONNECT,
    EPOLL_EVENT_TYPE_HOTPLUG_MSG,
};

struct epoll_event_data
{
    int fd;
    enum EPOLL_EVENT_TYPE type;
};

typedef struct{
    int epoll_fd;
    int hdmi_tx_fd;
    int kumsg_fd;
    int hotplug_fd;
}hotplug_fd_t;

static hotplug_fd_t m_hotplug_fd;
static struct epoll_event_data hotplg_data = { 0 };
static struct epoll_event_data hotplg_msg_data = { 0 };
static struct epoll_event_data kumsg_data = { 0 };

static void process_hotplug_msg(char *msg)
{
    //plug-out: ACTION=wifi-remove INFO=v0BDApF179
    //plug-in: ACTION=wifi-add INFO=v0BDApF179

    control_msg_t ctl_msg = {0};
    const char *plug_msg;

    plug_msg = (const char*)msg;
    if(strstr(plug_msg, "ACTION=wifi"))
    {
    #ifdef WIFI_SUPPORT
        //usb wifi plugin/plugout message
        if (strstr(plug_msg, "ACTION=wifi-remove"))
        {
            m_wifi_plugin = 0;
            printf("Wi-Fi plug-out\n");
            network_wifi_module_set(0);
            ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGOUT;
            api_control_send_msg(&ctl_msg);
        }
        else if (strstr(plug_msg, "ACTION=wifi-add"))
        {
            m_wifi_plugin = 1;
            if (strstr(plug_msg, "INFO=v0BDApF179"))
            {
                printf("Wi-Fi probed RTL8188_FTV\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8188FTV);

            }
            else if (strstr(plug_msg, "INFO=v0BDAp0179") ||
                    strstr(plug_msg, "INFO=v0BDAp8179"))
            {
                printf("Wi-Fi probed RTL8188_ETV\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8188FTV);

            }
            else if ( strstr(plug_msg, "INFO=v0BDAp8733") ||
                    strstr(plug_msg, "INFO=v0BDApB733") ||
                    strstr(plug_msg, "INFO=v0BDApF72B"))
            {
                printf("Wi-Fi probed RTL8731BU\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8733BU);

            }
            else if (strstr(plug_msg, "INFO=v0BDAp8731"))
            {
                printf("Wi-Fi probed RTL8731AU\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8811FTV);
            }
            else if (strstr(plug_msg, "INFO=v0BDApC811"))
            {
                printf("Wi-Fi probed RTL8811_FTV\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8811FTV);
            }
            else
            {
                printf("Unknown Wi-Fi probed: %s!\n", plug_msg);
                return;
            }

            ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGIN;
            api_control_send_msg(&ctl_msg);
        }
    #endif
    }
    else
    {
        //usb disk plugin/plugout message
        uint8_t mount_name[32];
        //usb-disk is plug in (SD??)
        if (strstr(plug_msg, "ACTION=mount"))
        {
            sscanf(plug_msg, "ACTION=mount INFO=%s", mount_name);
            printf("U-disk is plug in: %s\n", mount_name);
            m_usb_state = USB_STAT_MOUNT;
            ctl_msg.msg_type = MSG_TYPE_USB_DISK_PLUGIN;
            // action=USB_MSC_NOTIFY_MOUNT;
            //Enter upgrade window if there is upgraded file in USB-disk(hc_upgradexxxx.bin)
            //sys_upg_usb_check(5000);

        }    
        else if (strstr(plug_msg, "ACTION=umount"))
        {
            m_usb_state = USB_STAT_UNMOUNT;
            sscanf(plug_msg, "ACTION=umount INFO=%s", mount_name);
            printf("U-disk is plug out: %s\n", mount_name);
            ctl_msg.msg_type = MSG_TYPE_USB_DISK_PLUGOUT;
        }    
        partition_info_update(m_usb_state,mount_name);

        api_control_send_msg(&ctl_msg);
    }
}


static void do_kumsg(KuMsgDH *msg)
{
    switch (msg->type)
    {
    case HDMI_TX_NOTIFY_CONNECT:
        //m_hdmi_tx_plugin = 1;
        printf("%s(), line: %d. hdmi tx connect\n", __func__, __LINE__);
        break;
    case HDMI_TX_NOTIFY_DISCONNECT:
        //m_hdmi_tx_plugin = 0;
        printf("%s(), line: %d. hdmi tx disconnect\n", __func__, __LINE__);
        break;
    case HDMI_TX_NOTIFY_EDIDREADY:
    {
        struct hdmi_edidinfo *edid = (struct hdmi_edidinfo *)&msg->params;
        printf("%s(), best_tvsys: %d\n", __func__, edid->best_tvsys);
        _hotplug_hdmi_tx_tv_sys_set();
        break;
    }
    default:
        break;
    }
}


static void *hotplug_receive_event_func(void *arg)
{
    struct epoll_event events[MX_EVENTS];
    int n = -1;
    int i;
    struct sockaddr_in client;
    socklen_t sock_len = sizeof(client);
    int len;

    while (1)
    {
        n = epoll_wait(m_hotplug_fd.epoll_fd, events, MX_EVENTS, EPL_TOUT);
        if (n == -1)
        {
            if (EINTR == errno)
            {
                continue;
            }
            usleep(100 * 1000);
            continue;
        }
        else if (n == 0)
        {
            continue;
        }

        for (i = 0; i < n; i++)
        {
            struct epoll_event_data *d = (struct epoll_event_data *)events[i].data.ptr;
            int fd = (int)d->fd;
            enum EPOLL_EVENT_TYPE type = d->type;

            switch (type)
            {
                case EPOLL_EVENT_TYPE_KUMSG:
                {
                    unsigned char msg[MAX_KUMSG_SIZE] = {0};
                    len = read(fd, (void *)msg, MAX_KUMSG_SIZE);
                    if (len > 0)
                    {
                        do_kumsg((KuMsgDH *)msg);
                    }
                    break;
                }
                case EPOLL_EVENT_TYPE_HOTPLUG_CONNECT:
                {
                    printf("%s(), line: %d. get hotplug connect...\n", __func__, __LINE__);
                    struct epoll_event ev;
                    int new_sock = accept(fd, (struct sockaddr *)&client, &sock_len);
                    if (new_sock < 0)
                        break;

                    hotplg_msg_data.fd = new_sock;
                    hotplg_msg_data.type = EPOLL_EVENT_TYPE_HOTPLUG_MSG;
                    ev.events = EPOLLIN;
                    ev.data.ptr = (void *)&hotplg_msg_data;
                    epoll_ctl(m_hotplug_fd.epoll_fd, EPOLL_CTL_ADD, new_sock, &ev);
                    break;
                }
                case EPOLL_EVENT_TYPE_HOTPLUG_MSG:
                {
                    printf("%s(), line: %d. get hotplug msg...\n", __func__, __LINE__);
                    char msg[128] = {0};
                    len = read(fd, (void*)msg, sizeof(msg) - 1);
                    if (len > 0)
                    {
                        printf("%s\n", msg);
                        epoll_ctl(m_hotplug_fd.epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                        if (strstr(msg, "ACTION="))
                        {
                            process_hotplug_msg(msg);
                        }
                    }
                    else
                    {
                        printf("read hotplug msg fail\n");
                    }
                    break;
                }
                default:
                    break;
            }
        }

        usleep(10 * 1000);

    }

    return NULL;
}


static int hotplug_init()
{
    pthread_attr_t attr;
    pthread_t tid;
    struct sockaddr_un serv;
    struct epoll_event ev;
    struct kumsg_event event = { 0 };
    
    int ret;

    // start hot-plug detect
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&tid, &attr, hotplug_receive_event_func, (void *)NULL))
    {
        printf("pthread_create receive_event_func fail\n");
        goto out;
    }
    pthread_attr_destroy(&attr);

    m_hotplug_fd.hotplug_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (m_hotplug_fd.hotplug_fd < 0)
    {
        printf("socket error\n");
        goto out;
    }
    else
    {
        printf("socket success\n");
    }

    unlink("/tmp/hotplug.socket");
    bzero(&serv, sizeof(serv));
    serv.sun_family = AF_LOCAL;
    strcpy(serv.sun_path, "/tmp/hotplug.socket");
    ret = bind(m_hotplug_fd.hotplug_fd, (struct sockaddr *)&serv, sizeof(serv));
    if (ret < 0)
    {
        printf("bind error\n");
        goto out;
    }
    else
    {
        printf("bind success\n");
    }

    ret = listen(m_hotplug_fd.hotplug_fd, 1);
    if (ret < 0)
    {
        printf("listen error\n");
        goto out;
    }
    else
    {
        printf("listen success\n");
    }

    m_hotplug_fd.epoll_fd = epoll_create1(0);

    hotplg_data.fd = m_hotplug_fd.hotplug_fd;
    hotplg_data.type = EPOLL_EVENT_TYPE_HOTPLUG_CONNECT;
    ev.events = EPOLLIN;
    ev.data.ptr = (void *)&hotplg_data;
    if (epoll_ctl(m_hotplug_fd.epoll_fd, EPOLL_CTL_ADD, m_hotplug_fd.hotplug_fd, &ev) != 0)
    {
        printf("EPOLL_CTL_ADD hotplug fail\n");
        goto out;
    }
    else
    {
        printf("EPOLL_CTL_ADD hotplug success\n");
    }

    //hdmi hotplug message
    m_hotplug_fd.hdmi_tx_fd = open(DEV_HDMI, O_RDWR);
    if (m_hotplug_fd.hdmi_tx_fd < 0)
    {
        printf("%s(), line:%d. open device: %s error!\n",
               __func__, __LINE__, DEV_HDMI);
        goto out;
    }
    m_hotplug_fd.kumsg_fd = ioctl(m_hotplug_fd.hdmi_tx_fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
    kumsg_data.fd = m_hotplug_fd.kumsg_fd;
    kumsg_data.type = EPOLL_EVENT_TYPE_KUMSG;
    ev.events = EPOLLIN;
    ev.data.ptr = (void *)&kumsg_data;
    if (epoll_ctl(m_hotplug_fd.epoll_fd, EPOLL_CTL_ADD, m_hotplug_fd.kumsg_fd, &ev) != 0)
    {
        printf("EPOLL_CTL_ADD fail\n");
        goto out;
    }

    event.evtype = HDMI_TX_NOTIFY_CONNECT;
    event.arg = 0;
    ret = ioctl(m_hotplug_fd.hdmi_tx_fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret)
    {
        printf("KUMSGQ_NOTIFIER_SETUP 0x%08x fail\n", (int)event.evtype);
        goto out;
    }

    event.evtype = HDMI_TX_NOTIFY_DISCONNECT;
    event.arg = 0;
    ret = ioctl(m_hotplug_fd.hdmi_tx_fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret)
    {
        printf("KUMSGQ_NOTIFIER_SETUP 0x%08x fail\n", (int)event.evtype);
        goto out;
    }
    event.evtype = HDMI_TX_NOTIFY_EDIDREADY;
    event.arg = 0;
    ret = ioctl(m_hotplug_fd.hdmi_tx_fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret)
    {
        printf("KUMSGQ_NOTIFIER_SETUP 0x%08x fail\n", (int)event.evtype);
        goto out;
    }


out:
    return 0;

}
#endif


static uint16_t screen_init_rotate,screen_init_h_flip,screen_init_v_flip;
void api_get_screen_rotate_info(void)
{
    unsigned int rotate=0,h_flip=0,v_flip=0;
#ifdef __HCRTOS__
	int np;

	np = fdt_node_probe_by_path("/hcrtos/rotate");
	if(np>=0)
	{
		fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
		fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
		fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
		screen_init_rotate = rotate;
		screen_init_h_flip = h_flip;
		screen_init_v_flip = v_flip;
	}
	else
	{
		screen_init_rotate = 0;
		screen_init_h_flip = 0;
		screen_init_v_flip = 0;
	}
#else
#define ROTATE_CONFIG_PATH "/proc/device-tree/hcrtos/rotate"
	char status[16] = {0};
	api_dts_string_get(ROTATE_CONFIG_PATH "/status", status, sizeof(status));
	if(!strcmp(status, "okay")){
		rotate = api_dts_uint32_get(ROTATE_CONFIG_PATH "/rotate");
		h_flip = api_dts_uint32_get(ROTATE_CONFIG_PATH "/h_flip");
		v_flip = api_dts_uint32_get(ROTATE_CONFIG_PATH "/v_flip");
	}else{
		rotate = 0;
		h_flip = 0;
		v_flip = 0;
	}
	screen_init_rotate = rotate;
	screen_init_h_flip = h_flip;
	screen_init_v_flip = v_flip;
#endif
	printf("->>> init_rotate = %u h_flip %u v_flip = %u\n",rotate,h_flip,v_flip);
}

uint16_t api_get_screen_init_rotate(void)
{
    return screen_init_rotate;
}

uint16_t api_get_screen_init_h_flip(void)
{
    return screen_init_h_flip;
}

uint16_t api_get_screen_init_v_flip(void)
{
    return screen_init_v_flip;
}

#ifdef __linux__
static void exit_console(int signo)
{
    printf("%s(), signo: %d, error: %s\n", __FUNCTION__, signo, strerror(errno));

    api_watchdog_stop();
    exit(0);
}

#endif

int api_system_init()
{

#ifdef __HCRTOS__    
    api_romfs_resources_mount();
#else
    //start up wifi/network service script
    system("/etc/wifiprobe.sh &");

    extern void lv_fb_hotplug_support_set(bool enable);
    lv_fb_hotplug_support_set(false);
#endif    
    partition_info_init();
    hotplug_init();
    api_get_screen_rotate_info();
#ifdef BACKLIGHT_MONITOR_SUPPORT
    api_pwm_backlight_monitor_init();
#endif
    api_set_i2so_gpio_mute(false);

    if (!api_watchdog_init()){
        api_watchdog_timeout_set(30000);
        api_watchdog_start();
        uint32_t timeout_ms = 0;;
        api_watchdog_timeout_get(&timeout_ms);
        printf("%s(), line:%d. set watchdog timeout: %d ms\n", __func__, __LINE__, timeout_ms);
    }

#ifdef __linux__
    signal(SIGTERM, exit_console); //kill signal
    signal(SIGINT, exit_console); //Ctrl+C signal
    signal(SIGSEGV, exit_console); //segmentation fault, invalid memory
    signal(SIGBUS, exit_console);  //bus error, memory addr is not aligned.
#endif

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

int api_get_flip_info(int *rotate_type, int *flip_type)
{
   
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    int init_rotate = api_get_screen_init_rotate();
    int init_h_flip = api_get_screen_init_h_flip();
    int init_v_flip = api_get_screen_init_v_flip();

    get_rotate_by_flip_mode(projector_get_some_sys_param(P_FLIP_MODE) ,
                            &rotate ,
                            &h_flip ,
                            &v_flip);

    api_transfer_rotate_mode_for_screen(
        init_rotate,init_h_flip,init_v_flip,
        &rotate , &h_flip , &v_flip , NULL);

    *rotate_type = rotate;
    *flip_type = h_flip;
    return 0;
}

static void *m_logo_player = NULL;
static char m_logo_file[128] = {0};

int api_logo_off_no_black()
{
    if (m_logo_player){
        hcplayer_stop2(m_logo_player, 0, 0);
    }
    m_logo_player = NULL;
    return 0;

}

int api_logo_reshow(void)
{
    if (m_logo_player && strlen(m_logo_file))
    {
        api_logo_off_no_black();
        api_logo_show(m_logo_file);
    }

    return 0;
}

int api_logo_show(const char *file)
{
    char *file_path = file;
    int rotate_type = ROTATE_TYPE_0;
    int flip_type = MIRROR_TYPE_NONE;
    
    api_logo_off();

    if (!file)
        file_path = BACK_LOGO;

    strcpy(m_logo_file, file_path);

    HCPlayerInitArgs player_args;
	memset(&player_args, 0, sizeof(player_args));
    player_args.uri = file_path;
#ifdef __linux__
    player_args.msg_id = -1;
#else
    player_args.msg_id = 0;
#endif

    api_get_flip_info(&rotate_type, &flip_type);
    player_args.img_effect.mode = IMG_SHOW_NORMAL;    
    player_args.rotate_type = rotate_type;
    player_args.mirror_type = flip_type;
    player_args.rotate_enable = 1;

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
    if (m_logo_player){
        hcplayer_stop(m_logo_player);
    }
    m_logo_player = NULL;
    return 0;
}

int api_logo_off2(int closevp, int fillblack)
{

    if (m_logo_player)
    {
        hcplayer_stop2(m_logo_player,closevp,fillblack);
        m_logo_player = NULL;
        printf("%s =============================Close logo: ok!=============================\n", __func__);
    }
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

void api_control_clear_msg(msg_type_t msg_type)
{
    control_msg_t msg_buffer;
    while(1){
        if(api_control_receive_msg(&msg_buffer)){
            break;
        }
    }
    return;
}

int api_control_send_key(uint32_t key)
{
    control_msg_t control_msg;
    control_msg.msg_type = MSG_TYPE_KEY;
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
 #ifdef WIFI_SUPPORT
int api_get_mac_addr(char *mac) 
{
#ifdef __linux__    
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
#else
    struct ifreq ifr;
    int skfd;

    if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        printf("socket error\n");
        return -1;
    }

    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0)
    {
        printf( "%s net_get_hwaddr: ioctl SIOCGIFHWADDR\n",__func__);
        close(skfd);
        return -1;
    }
    close(skfd);
    memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
    return 0;
#endif

}

#endif

void app_ffplay_init(void)
{
    if (m_ffplay_init) return;

    hcplayer_init(1);
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
    if (INVALID_ID != m_control_msg_id){
        api_message_delete(m_control_msg_id);
        m_control_msg_id = INVALID_ID;
    }

    app_ffplay_deinit();
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

int api_osd_show_onoff(bool on_off)
{
    pthread_mutex_lock(&m_osd_off_mutex);    
    // Open the file for reading and writing
    int fbfd = open("/dev/fb0", O_RDWR);
    uint32_t blank_mode;

    if(fbfd == -1) {
        printf("%s(), line: %d. Error: cannot open framebuffer device", __func__, __LINE__);
        pthread_mutex_unlock(&m_osd_off_mutex);
        return API_FAILURE;
    }

    m_osd_off_time_flag = 0;

    if (on_off)
        blank_mode = FB_BLANK_UNBLANK;
    else
        blank_mode = FB_BLANK_NORMAL;

    if (ioctl(fbfd, FBIOBLANK, blank_mode) != 0) {
        printf("%s(), line: %d. Error: FBIOBLANK", __func__, __LINE__);
    }

    close(fbfd);
    pthread_mutex_unlock(&m_osd_off_mutex);
    return API_SUCCESS;
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

#ifdef __linux__
int api_dts_uint32_get(const char *path)
{
    int fd = open(path, O_RDONLY);
    int value = 0;;
    if(fd >= 0){
        uint8_t buf[4];
        if(read(fd, buf, 4) != 4){
            close(fd);
            return value;
        }
        close(fd);
        value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    }
    //printf("dts value: %d\n", value);
    return value;
}

void api_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    int value = 0;;
    if(fd >= 0){
        read(fd, string, size);
        close(fd);
    }
    //printf("dts string: %s\n", string);
}
#endif

void api_system_reboot(void)
{
    printf("%s(): reboot now!!\n", __func__);
#ifdef __linux__
    system("reboot");
#else    
    hw_watchdog_reset(10);
#endif
    while(1);
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
         // printf("%s(), entry_tmp->screen:%x,%x, screen, ctrl:%x\n", __func__, \
         //     entry_tmp->screen, screen, entry_tmp->control);
        if (entry_tmp->screen == screen){
            return entry_tmp->control;
        }
        glist_tmp = glist_tmp->next;
    }

    return NULL;
}



/*
we can wake up by 3 ways: ir, gpio and sacadc key. 
 */
void api_system_standby(void)
{
    int fd = -1;

    fd = open("/dev/standby", O_RDWR);
    if (fd < 0) {
        printf("%s(), line:%d. open standby device error!\n", __func__, __LINE__);
        return;
    }

    printf("%s(), line:%d.\n", __func__, __LINE__);

    //step 1: off the display
    api_osd_show_onoff(false);
    api_logo_off();
    api_dis_show_onoff(false);
    //sleep for a while so that hardware display is really off.
    api_sleep_ms(100);
    

    //Step 2: off other devices


    //Step 3: config the standby wake up methods
        
    //config wake up ir scancode(so far, default is power key:28)   
    //check hclinux\SOURCE\linux-drivers\drivers\hcdrivers\rc\keymaps\rc-hcdemo.c
    //for scan code. 
    struct standby_ir_setting ir = { 0 };
    ir.num_of_scancode = 1;
    ir.scancode[0] = 28;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_IR, (unsigned long)&ir);

    //config wake up GPIO
    struct standby_gpio_setting gpio = { 0 };
    gpio.pin = PINPAD_L08;
    gpio.polarity = 0;//low is active;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_GPIO, (unsigned long)&gpio);

#if 0
    //config wake up adc key
    struct standby_saradc_setting adc = { 0 };
    adc.channel = 1;
    adc.min = 1000;
    adc.max = 1500;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_SARADC, (unsigned long)&adc);
#endif    

#if 0
    //lower the volatage of ddr via the GPIO
    struct standby_pwroff_ddr_setting ddr = { 0 };
    ddr.pin = PINPAD_L09;
    ddr.polarity = 0;//low is active;
    ioctl(fd, STANDBY_SET_PWROFF_DDR, (unsigned long)&ddr);
#endif

    //Step 4: entering system standby
    ioctl(fd, STANDBY_ENTER, 0);
    close(fd);
    while(1);
}

#define ONE_COUNT_TIME  100
static void *osd_off_for_time(void *arg)
{
    uint32_t timeout = (uint32_t)arg;
    char time_flag;
    m_osd_off_time_cnt = timeout/ONE_COUNT_TIME;
    do{
        api_sleep_ms(ONE_COUNT_TIME);
    }while(m_osd_off_time_cnt --);

    m_osd_off_time_cnt = 0;
    pthread_mutex_lock(&m_osd_off_mutex);
    time_flag = m_osd_off_time_flag;
    pthread_mutex_unlock(&m_osd_off_mutex);
    if (time_flag)
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
    pthread_mutex_lock(&m_osd_off_mutex);
    m_osd_off_time_flag = 1;
    pthread_mutex_unlock(&m_osd_off_mutex);
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x1000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&thread_id, &attr, osd_off_for_time, (void*)timeout)) {
        return;
    }
    pthread_attr_destroy(&attr);    
}


static bool m_hotkey_all_enable = true;
#define ENBALE_KEY_MAX 10
static uint32_t m_hotkey_enable_key[ENBALE_KEY_MAX] = {0,};
static uint32_t m_enable_key_cnt = 0;
void api_hotkey_enable_set(uint32_t *enable_key, uint32_t key_cnt)
{
    m_enable_key_cnt = key_cnt > ENBALE_KEY_MAX ? ENBALE_KEY_MAX :  key_cnt;
    if (!enable_key)
        m_enable_key_cnt = 0;

    if (enable_key && key_cnt)
        memcpy(m_hotkey_enable_key, enable_key, m_enable_key_cnt*sizeof(uint32_t));

    m_hotkey_all_enable = false;
}

void api_hotkey_disable_clear(void)
{
    m_enable_key_cnt = 0;
    m_hotkey_all_enable = true;
}

bool api_hotkey_enable_get(uint32_t key)
{
    int i;
    if (m_hotkey_all_enable)
        return true;

    for (i = 0; i < m_enable_key_cnt; i ++){
        if (key == m_hotkey_enable_key[i])
            return true;
    }
    return false;
}
static bt_connect_status_e bt_connet_status = BT_CONNECT_STATUS_DEFAULT;
void api_set_bt_connet_status(bt_connect_status_e val)
{
    bt_connet_status = val;
}

bt_connect_status_e api_get_bt_connet_status(void)
{
    return bt_connet_status;
}

#define SOUND_DEV   "/dev/sndC0i2so"
#define DEFAULT_VOL 50
static uint8_t m_vol_back = DEFAULT_VOL;
// vol and mute ctrl i2si when cvbs in,else ctrl i2so
static int api_mute_ctrl(bool mute,int snd_fd)
{
    if(snd_fd<0){
        return API_FAILURE;
    }
    uint8_t volume = 0;
    if (mute){
        ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &m_vol_back);
        ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
		//printf("fd:%d, m_vol_back:%d\n", snd_fd, m_vol_back);
    } else {
        volume = m_vol_back;
        //cvbs or hdmi will ctrl gpio mute so enable gpio here 
        // api_set_i2so_gpio_mute_auto();
        /*if(api_get_bt_connet_status()<BT_CONNECT_STATUS_CONNECTED){
            api_set_i2so_gpio_mute(false);
        }*/
        ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
    }

    printf("mute is %d, vol: %d\n", mute, volume);
    return API_SUCCESS;
}
// #include 
int api_media_mute(bool mute)
{
    int snd_fd_i2so=-1;
    snd_fd_i2so = open("/dev/sndC0i2so", O_WRONLY);
    if(snd_fd_i2so<0){
        printf("open snd_fd_i2so fail\n");
        return API_FAILURE;
    }else{
        api_mute_ctrl(mute,snd_fd_i2so);
        close(snd_fd_i2so);
    }
#ifdef CVBS_AUDIO_I2SI_I2SO
	if(cvbs_is_playing()){
	    int snd_fd_i2si=-1;
	    snd_fd_i2si = open("/dev/sndC0i2si", O_WRONLY);
	    if(snd_fd_i2si<0){
	        printf("open snd_fd_i2si fail\n");
	        return API_FAILURE;
	    }else{
	        api_mute_ctrl(mute,snd_fd_i2si);
	        close(snd_fd_i2si);
	    }
}
#endif 
    return API_SUCCESS;

}

int api_media_set_vol(uint8_t volume)
{
    int snd_fd = -1;

    snd_fd = open(SOUND_DEV, O_WRONLY);
    if (snd_fd < 0) {
        printf ("open snd_fd %d failed\n", snd_fd);
        return API_FAILURE;
    }

    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
    volume = 0;
    ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &volume);
    printf("current volume is %d\n", volume);

    close(snd_fd);
    return API_SUCCESS;
}

// gpio mute
void api_set_i2so_gpio_mute(bool val)//1 mute false no mute
{
	int snd_fd = -1;
#ifdef BLUETOOTH_SUPPORT
	bluetooth_set_gpio_mutu(val);// C2 used?	
	if(api_get_bt_connet_status() == BT_CONNECT_STATUS_CONNECTED && !val)// snd --x-->i2so, but snd ->bt
		return;
#endif
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return;
	}
	ioctl(snd_fd, SND_IOCTL_SET_MUTE, val);
	close(snd_fd);
}

// this api would be deprecated.
void api_set_i2so_gpio_mute_auto(void)
{
	return ;
	int snd_fd = -1;
	uint8_t m_vol_back=0;
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return;
	}
#ifdef BLUETOOTH_SUPPORT
	if(api_get_bt_connet_status() == BT_CONNECT_STATUS_CONNECTED)
		ioctl(snd_fd, SND_IOCTL_SET_MUTE, 1);
	else
#endif
    {
		ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &m_vol_back);
		printf("m_vol_back =%d\n", m_vol_back);
		if(m_vol_back==0)
			ioctl(snd_fd, SND_IOCTL_SET_MUTE, 1);
		else
            ioctl(snd_fd, SND_IOCTL_SET_MUTE, 0);
	}
	close(snd_fd);
    printf("bt_get_connet_state = %d\n",api_get_bt_connet_status());
}

#ifdef __HCRTOS__
/* Load HDCP key*/
int api_get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
    static int np = -1;
    static u32 part_num = 0;
    u32 i = 1;
    const char *label;
    char property[32];

    if (np < 0) {
        np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
    }

    if (np < 0)
        return -1;

    if (part_num == 0)
        fdt_get_property_u_32_index(np, "part-num", 0, &part_num);

    for (i = 1; i <= part_num; i++) {
        snprintf(property, sizeof(property), "part%d-label", i);
        if (!fdt_get_property_string_index(np, property, 0, &label) &&
            !strcmp(label, partname)) {
            memset(devpath, 0, len);
            snprintf(devpath, len, "/dev/mtdblock%d", i);
            return i;
        }
    }

    return -1;
}
#else
#define DTS_FLASH_CONFIG_PATH "/proc/device-tree/hcrtos/sfspi/spi_nor_flash/partitions"
int api_get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
    uint32_t i = 1;
    uint32_t part_num = 0;
    const char label[32];
    char property[128];

    if (part_num == 0)
        part_num = api_dts_uint32_get(DTS_FLASH_CONFIG_PATH "/part-num");

    for (i = 1; i <= part_num; i++) {
        snprintf(property, sizeof(property), DTS_FLASH_CONFIG_PATH "/part%d-label", i);
        api_dts_string_get(property, label, sizeof(label));
        if (!strcmp(label, partname)){
            memset(devpath, 0, len);
            //snprintf(devpath, len, "/dev/mtdblock%d", i);
            snprintf(devpath, len, "/dev/mtd%d", i);

            printf("%s(), line:%d. devpath:%s\n", __func__, __LINE__, devpath);
            return i;
        }
    }
    printf("%s(), line:%d. cannot find mtd dev path!\n", __func__, __LINE__);
    return -1;
}

#endif


#ifdef __HCRTOS__
int api_romfs_resources_mount(void)
{
    static char m_mount = 0;
    char devpath[64];
    int ret = 0;

    if (m_mount)
    {
        printf("%s: resources alread mount!\n", __func__);
        return -1;
    }

    ret = api_get_mtdblock_devpath(devpath, sizeof(devpath), "eromfs");
    if (ret >= 0)
        ret = mount(devpath, "/hcdemo_files", "romfs", MS_RDONLY, NULL);

    if (ret >= 0)
    {
        printf("mount ok!\n");
        m_mount = 1;
    }

    return 0;
}
#endif

// set display area in projector
void api_set_display_area(dis_tv_mode_e ratio)//void set_aspect_ratio(dis_tv_mode_e ratio)
{
   struct dis_aspect_mode aspect = { 0 };
   struct dis_zoom dz;
   int fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return;
	}
#if 0
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    if(ratio == DIS_TV_4_3)
        aspect.dis_mode = DIS_LETTERBOX; //DIS_PANSCAN
    else if(ratio == DIS_TV_16_9)
        aspect.dis_mode = DIS_PILLBOX; //DIS_PILLBOX
    else if(ratio == DIS_TV_AUTO){
        aspect.dis_mode = DIS_NORMAL_SCALE;
    }
	ioctl(fd , DIS_SET_ASPECT_MODE , &aspect);
#else
    dz.layer = DIS_LAYER_MAIN;
    dz.distype = DIS_TYPE_HD;
    int h = get_display_h();//projector_get_some_sys_param(P_SYS_SCALE_MAIN_LAYER_H);
    int v = get_display_v();//projector_get_some_sys_param(P_SYS_SCALE_MAIN_LAYER_V);
    if(h*3 == v*4 && (ratio == DIS_TV_16_9 || ratio == DIS_TV_AUTO)){
        v = v*3/4;
    }else if(h*9 == v*16 && ratio == DIS_TV_4_3){
        h = h*3/4;
    }
    dz.dst_area.x =  get_display_x()+(get_display_h()-h)/2;
    dz.dst_area.y =  get_display_y()+(get_display_v()-v)/2;
    dz.dst_area.w =  h;
    dz.dst_area.h =  v;
    dz.src_area.x = 0;
    dz.src_area.y = 0;
    dz.src_area.w = 1920;
    dz.src_area.h = 1080;
	aspect.distype = DIS_TYPE_HD;
	aspect.tv_mode = DIS_TV_16_9;
	aspect.dis_mode = DIS_NORMAL_SCALE;
    if(ratio == DIS_TV_AUTO){
		aspect.dis_mode = DIS_PILLBOX;
	}
	ioctl(fd , DIS_SET_ZOOM, &dz);
	usleep(30*1000);
	ioctl(fd , DIS_SET_ASPECT_MODE, &aspect);
#endif
	close(fd);
}

void api_set_display_zoom(int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h){
    struct dis_zoom dz = { 0 };
    dz.layer = DIS_LAYER_MAIN;
    dz.distype = DIS_TYPE_HD;
    dz.src_area.x = s_x;
    dz.src_area.y = s_y;
    dz.src_area.w = s_w;
    dz.src_area.h = s_h;
    dz.dst_area.x = d_x;
    dz.dst_area.y = d_y;
    dz.dst_area.w = d_w;
    dz.dst_area.h = d_h;

    int fd = -1;

    fd = open("/dev/dis", O_WRONLY);
    if(fd < 0){
        return;
    }
    ioctl(fd , DIS_SET_ZOOM , &dz);
    close(fd);
}
 
void api_set_display_aspect(dis_tv_mode_e ratio , dis_mode_e dis_mode)
{
    int ret = 0;
    dis_aspect_mode_t aspect = { 0 };

    printf("ratio: %d, dis_mode: %d\n" , ratio , dis_mode);
    int fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0) {
    	return;
    }

    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    ret = ioctl(fd, DIS_SET_ASPECT_MODE , &aspect);
    if( ret != 0 ){
        printf("%s:%d: err: DIS_SET_ASPECT_MODE failed\n", __func__, __LINE__);
        close(fd);
        return;
    }
    close(fd);
    return;
}

/**
 * @description: 获取的是图像在实际LCD屏幕显示的区域
 * @param {dis_screen_info_t *} dis_area
 * @return {*}
 * @author: Yanisin
 */
int api_get_display_area(dis_screen_info_t * dis_area)
{
	int fd;
	fd = open("/dev/dis", O_WRONLY);
    if(fd < 0){
        return -1 ;
    }
	dis_area->distype=DIS_TYPE_HD;
    ioctl(fd , DIS_GET_MP_AREA_INFO, dis_area);
    close(fd);
	return 0;
}
/**
 * @description: 获取的是实际输出LCD屏幕的信息，类似通过fb获取实际LCD信息
 * @param {dis_screen_info_t *} dis_area
 * @return {*}
 * @author: Yanisin
 */
int api_get_screen_info(dis_screen_info_t * dis_area)
{
	int fd;
	fd = open("/dev/dis", O_WRONLY);
    if(fd < 0){
        return -1 ;
    }
	dis_area->distype=DIS_TYPE_HD;
    ioctl(fd , DIS_GET_SCREEN_INFO, dis_area);
    close(fd);
	return 0;
}

int api_set_display_zoom2(dis_zoom_t* diszoom_param)
{
    struct dis_zoom dz = { 0 };
    memcpy(&dz,diszoom_param, sizeof(dis_zoom_t));
    dz.layer = DIS_LAYER_MAIN;
    dz.distype = DIS_TYPE_HD;
    dz.active_mode=DIS_SCALE_ACTIVE_IMMEDIATELY;
    int fd = -1;

    fd = open("/dev/dis", O_WRONLY);
    if(fd < 0){
        return -1;
    }
    ioctl(fd , DIS_SET_ZOOM , &dz);
    close(fd);
    return 0;

}

typedef void *(player_get_func)(void);
static player_get_func *ffmpeg_player_get = NULL;
void *api_ffmpeg_player_get(void)
{
    if (ffmpeg_player_get)
        return ffmpeg_player_get();
    else
        return NULL;
}


void api_ffmpeg_player_get_regist(void *(func)(void))
{
    ffmpeg_player_get = func;
}

/*
* Get the mounted usb disk path. save usb dev path to dev_path: /media/hdd, /media/usb1, etc
* return the count of USB disk device path.
* dev_cnt: the count of dev_path[] for saving the device path.
*/
int api_usb_dev_path_get(char dev_path[][MAX_UDISK_DEV_LEN], int dev_cnt)
{
    int real_dev_cnt = 0;
    DIR *dirp = NULL;
    struct dirent *entry = NULL;

    if ((dirp = opendir(MOUNT_ROOT_DIR)) == NULL) {
        printf("%s(), line: %d. open dir:%s error!\n", __func__, __LINE__, MOUNT_ROOT_DIR);
        return 0;
    }

    do{
        entry = readdir(dirp);
        if (!entry)
            break;

        if(!strcmp(entry->d_name, ".") ||
            !strcmp(entry->d_name, "..")){
            //skip the upper dir
            continue;
        }

        if (strlen(entry->d_name) && entry->d_type == 4){ //dir
            sprintf(dev_path[real_dev_cnt], "%s/%s", MOUNT_ROOT_DIR, entry->d_name);
            printf("%s(), line: %d. found USB device: %s!\n", __func__, __LINE__,dev_path[real_dev_cnt]);        
            real_dev_cnt++;
        }
    }while(real_dev_cnt < dev_cnt);

    return real_dev_cnt;
}


void api_transfer_rotate_mode_for_screen(
                                        int init_rotate,
                                        int init_h_flip,
                                        int init_v_flip,
                                        int *p_rotate_mode ,
                                        int *p_h_flip ,
                                        int *p_v_flip,
                                        int *p_fbdev_rotate)
{
    int fbdev_rotate[4] = { 0,270,180,90 }; //setting is anticlockwise for fvdev
    int rotate = 0 , h_flip = 0 , v_flip = 0;

    rotate = *p_rotate_mode;

    //if screen is V screen,h_flip and v_flip exchange
    if(init_rotate == 0 || init_rotate == 180)
    {
        h_flip = *p_h_flip;
        v_flip = *p_v_flip;
    }
    else
    {
        h_flip = *p_v_flip;
        v_flip = *p_h_flip;
    }
 
    /*setting in dts is anticlockwise */
    /*calc rotate mode*/
    if(init_rotate == 270)
    {
        rotate = (rotate + 1) & 3;
    }
    else if(init_rotate == 90)
    {
        rotate = (rotate + 3) & 3;
    }
    else if(init_rotate == 180)
    {
        rotate = (rotate + 2) & 3;
    }

    /*transfer v_flip to h_flip with rotate
    *rotate 0 + H
    *rotate 0 + V--> rotate 180 +H
    *rotate 180 + H
    *rotate 180 + V --> rotate 0  + H 
    *rotate 90 + H
    *rotate 90 + V--> rotate 270 +H
    *rotate 270 +H
    *rotate 270 +V--> rotate 90 + H 
    */
    if(v_flip == 1)
    {
        switch(rotate)
        {
            case ROTATE_TYPE_0:
                rotate = ROTATE_TYPE_180;
                break;
            case ROTATE_TYPE_90:
                rotate = ROTATE_TYPE_270;
                break;
            case ROTATE_TYPE_180:
                rotate = ROTATE_TYPE_0;
                break;
            case ROTATE_TYPE_270:
                rotate = ROTATE_TYPE_90;
                break;
            default:
                break;
        }
        v_flip = 0;
        h_flip = 1;
    }

    h_flip = h_flip ^ init_h_flip;

    if(p_rotate_mode != NULL)
    {
        *p_rotate_mode = rotate;
    }
    
    if(p_h_flip != NULL)
    {
        *p_h_flip = h_flip;
    }
    
    if(p_v_flip != NULL)
    {
        *p_v_flip = 0;
    }
    
    if(p_fbdev_rotate !=  NULL)
    {
        *p_fbdev_rotate = fbdev_rotate[rotate];
    }
    


}
#ifdef MIRROR_ES_DUMP_SUPPORT

//#define DUMP_FOLDER   "mirror_dump"
//Dump file to: /mirror_dump/aircast-X.h264, etc

static bool m_mirror_dump_enable = false;
void api_mirror_dump_enable_set(bool enable)
{
    m_mirror_dump_enable = enable;
}

bool api_mirror_dump_enable_get(char* folder)
{
    char usb_dev[1][MAX_UDISK_DEV_LEN];

    if (!m_mirror_dump_enable)
        return false;

    if (!api_usb_dev_path_get(usb_dev, 1)){
        printf("%s(), line: %d!. No dump file path!\n", __func__, __LINE__);
        return false;
    }
    //sprintf(folder, "%s/%s", usb_dev[0], DUMP_FOLDER);
    strcpy(folder, usb_dev[0]);
    printf("%s(), line: %d!. dump:%s\n", __func__, __LINE__, folder);
    return m_mirror_dump_enable;
}

#endif


#ifdef __linux__
static void dev_ready_check(void *param)
{
    DIR *dirp = NULL;
    int usb_dev_found = 0;
    struct dirent *entry = NULL;
    uint32_t timeout = (uint32_t)param;
    char usb_path[512];
    int check_count = timeout/100;

    if (USB_STAT_MOUNT == mmp_get_usb_stat())
        return;

    if ((dirp = opendir(MOUNT_ROOT_DIR)) == NULL) {
        printf("%s(), line: %d. open dir:%s error!\n", __func__, __LINE__, MOUNT_ROOT_DIR);
        return;
    }

    while(check_count --){
        while (1) {
            entry = readdir(dirp);
            if (!entry)
                break;

            if(!strcmp(entry->d_name, ".") ||
                !strcmp(entry->d_name, "..")){
                //skip the upper dir
                continue;
            }

            if (strlen(entry->d_name) && entry->d_type == 4){ //dir
                sprintf(usb_path, "%s/%s", MOUNT_ROOT_DIR, entry->d_name);
                usb_dev_found = 1;
                partition_info_update(USB_STAT_MOUNT, usb_path);
                printf("%s(), line: %d. found USB device: %s!\n", __func__, __LINE__,usb_path);        
                //break;
            }

        }
        api_sleep_ms(100);
    }
	if (usb_dev_found)
		mmp_set_usb_stat(USB_STAT_MOUNT);
	
    if (check_count <= 0){
        printf("%s(), line: %d. No USB device!\n", __func__, __LINE__);        
    }
    if (dirp)
        closedir(dirp);

}

//create a task to check if usb disk if ready.
//sometimes there is no mount message while start up in linux.
void api_usb_dev_check(void)
{
    pthread_t id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); //release task resource itself
    pthread_attr_setstacksize(&attr, 0x1000);
    pthread_create(&id, &attr,dev_ready_check, (void*)6000);
    pthread_attr_destroy(&attr);
}
#endif


//call VIDSINK_DISABLE_IMG_EFFECT command to release
//the picture effect buffer.
int api_pic_effect_enable(bool enable)
{
    int vidsink_fd;

    vidsink_fd = open("/dev/vidsink", O_WRONLY);
    if (vidsink_fd < 0) {
        return -1;
    }

    if (enable) {
        ioctl(vidsink_fd, VIDSINK_ENABLE_IMG_EFFECT, 0);
    } else {
        ioctl(vidsink_fd, VIDSINK_DISABLE_IMG_EFFECT, 0);
    }

    if (vidsink_fd >= 0)
        close(vidsink_fd);

    return 0;

}

int api_set_backlight_brightness(int val)
{
	int lvds_fd;
	int backlight_fd = 0;
	backlight_fd = open("/dev/backlight",O_RDWR);
	lvds_fd = open("/dev/lvds",O_RDWR);
	if (lvds_fd < 0 && backlight_fd < 0) {
		printf ("open backlight failed\n");
		return -1;
	}

	if(backlight_fd > 0)
	{
		write(backlight_fd,&val,4);
		close(backlight_fd);
	}
	if(lvds_fd)
	{
		ioctl(lvds_fd, LVDS_SET_PWM_BACKLIGHT,val);//lvds set pwm default
		ioctl(lvds_fd, LVDS_SET_GPIO_BACKLIGHT,val);//lvds gpio backlight close
		close(lvds_fd);
	}
	return 0;
}

#ifdef BACKLIGHT_MONITOR_SUPPORT
typedef enum{
    POWER_SOURCE_DEFAULT,
    POWER_SOURCE_DCPOWER,
    POWER_SOURCE_BATTERY,
}power_source_t;
static pinpad_e backlight_detection_pad = INVALID_VALUE_8;
static power_source_t power_source = POWER_SOURCE_DEFAULT;
static unsigned int pwm_battery_duty = 50;
static unsigned int pwm_power_duty = 100;
void api_pwm_backlight_monitor_init(void)
{
	int lcd_np=fdt_node_probe_by_path("/hcrtos/lcd");
	u32 tmp = INVALID_VALUE_8;

	if(lcd_np > 0)
	{
		fdt_get_property_u_32_index(lcd_np, "backlight-detection", 0, (u32 *)&tmp);
		backlight_detection_pad = (pinpad_e)tmp;
		fdt_get_property_u_32_index(lcd_np, "pwm-batter-duty", 0, (u32 *)&pwm_battery_duty);
		fdt_get_property_u_32_index(lcd_np, "pwm-power-duty", 0, (u32 *)&pwm_power_duty);
	}
	else
	{
		printf("%s %d error\n",__func__,__LINE__);
	}
    gpio_configure(backlight_detection_pad,GPIO_DIR_INPUT);
    printf("%s %d %d\n",__func__,__LINE__,backlight_detection_pad);
}

void api_pwm_backlight_monitor_loop(void)
{
    if(gpio_get_input(backlight_detection_pad))
    {
        if(power_source != POWER_SOURCE_DCPOWER)
        {
            printf("%s %d pwm duty %d\n",__func__,__LINE__,pwm_battery_duty);
			api_set_backlight_brightness(pwm_battery_duty);
            // ioctl(lvds_fd, LVDS_SET_PWM_BACKLIGHT,pwm_battery_duty);//lvds pwm set default value
            power_source = POWER_SOURCE_DCPOWER;
        }
    }
    else
    {
        if(power_source != POWER_SOURCE_BATTERY)
        {
            printf("%s %d pwm duty %d\n",__func__,__LINE__,pwm_power_duty);
			api_set_backlight_brightness(pwm_power_duty);
            // ioctl(lvds_fd, LVDS_SET_PWM_BACKLIGHT,pwm_power_duty);//lvds pwm set default value
            power_source = POWER_SOURCE_BATTERY;
        }
    }
}

void api_pwm_backlight_monitor_update_status(void)
{
    power_source = POWER_SOURCE_DEFAULT;
}
#endif

int api_set_partition_info(int index)
{
    if(partition_info!=NULL&&partition_info->count>0){
        partition_info->used_dev=glist_nth_data((glist*)partition_info->dev_list,index);
    }
    return 0;
}
/**
 * @description: check used_dev had be hotplug(umount)
 * @return {*} true is unoumt 
 * @author: Yanisin
 */
bool api_check_partition_used_dev_ishotplug(void){
    char *dev_name=NULL;
    if(partition_info!=NULL&&partition_info->used_dev!=NULL){
        for(int i=0;i<partition_info->count;i++){
            dev_name=(char *)glist_nth_data((glist*)partition_info->dev_list,i);
            if(strcmp(dev_name,partition_info->used_dev)==0){
                return false;
            }
        }
        return true ;
    }
}

static const char *m_dev_dog = "/dev/watchdog";
static int m_dog_fd = -1;

int api_watchdog_init(void)
{
    if (m_dog_fd >= 0)
        return 0;

    m_dog_fd = open(m_dev_dog, O_RDWR);
    if (m_dog_fd < 0) {
        printf("%s(), line:%d. No watchdog!!!\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}


//Set the timeout of watchdog , system would reboot if watchdog not feed
//within the timeout.
int api_watchdog_timeout_set(uint32_t timeout_ms)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__    
    ioctl(m_dog_fd, WDIOC_SETMODE, WDT_MODE_WATCHDOG);
    return ioctl(m_dog_fd, WDIOC_SETTIMEOUT, timeout_ms*1000);
#else
    uint32_t timeout_second = timeout_ms/1000;
    return ioctl(m_dog_fd, WDIOC_SETTIMEOUT, &timeout_second);
#endif    
}

int api_watchdog_timeout_get(uint32_t *timeout_ms)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__    
    uint32_t timeout_us = 0;
    if (ioctl(m_dog_fd, WDIOC_GETTIMEOUT, &timeout_us))
        return -1;
    *timeout_ms = timeout_us/1000;
#else
    uint32_t timeout_second;
    if (ioctl(m_dog_fd, WDIOC_GETTIMEOUT, &timeout_second))
        return -1;

    *timeout_ms = timeout_second*1000;

#endif    

    return 0;
}

int api_watchdog_start(void)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__        
    return ioctl(m_dog_fd, WDIOC_START, 0);
#else
    uint32_t val = WDIOS_ENABLECARD;
    return ioctl(m_dog_fd, WDIOC_SETOPTIONS, &val);
#endif
}

int api_watchdog_stop(void)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__        

    ioctl(m_dog_fd, WDIOC_STOP, 0);
#else    

    uint32_t val;

    ioctl(m_dog_fd, WDIOC_GETSTATUS, &val);
    printf("%s(), line: %d: watchdog status = %s \n", __func__, __LINE__,
        val ? "running" : "stop");

    uint32_t timeout_second;
    ioctl(m_dog_fd, WDIOC_GETTIMEOUT, &timeout_second);
    printf("%s(), line: %d: watchdog timeout = %d \n", __func__, __LINE__, timeout_second);


    write(m_dog_fd, "V", 1);
    val = WDIOS_DISABLECARD;
    ioctl(m_dog_fd, WDIOC_SETOPTIONS, &val);

    ioctl(m_dog_fd, WDIOC_GETSTATUS, &val);
    printf("%s(), line: %d: watchdog status = %s \n", __func__, __LINE__,
           val ? "running" : "stop");


#endif
    return 0;
}


int api_watchdog_feed(void)
{
    if (m_dog_fd < 0)
        return -1;

    return ioctl(m_dog_fd, WDIOC_KEEPALIVE, 0);    
}


uint32_t api_sys_tick_get(void)
{
    extern uint32_t custom_tick_get(void);
    return custom_tick_get();
}
/**
 * @description: use this func to switch video and pic without black flash  
 * @return {*}
 * @author: Yanisin
 */
int api_media_pic_backup(void)
{
    int distype = DIS_TYPE_HD;
    int fd;
    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0) {
     return -1;
    }
    usleep(400*1000);
    ioctl(fd ,DIS_BACKUP_MP , distype);
    usleep(400*1000);

    close(fd);
    return 0;
}
int api_media_pic_backup_free(void)
{
    int distype = DIS_TYPE_HD;
    int fd;
    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0) {
     return -1;
    }
    ioctl(fd ,DIS_FREE_BACKUP_MP , distype);

    close(fd);
    return 0;
}

#ifdef __HCRTOS__

#include <nuttx/drivers/ramdisk.h>
#include <nuttx/fs/fs.h>
#include <fsutils/mkfatfs.h>

static char * m_ram_path = "/dev/ram0";
//static char * m_mnt_path = "/mnt/ram0";
static char * m_mnt_path = "/tmp";

static uint8_t *m_ram_buffer = NULL;
static int _ramdisk_create(int minor, uint32_t nsectors, uint16_t sectsize, uint8_t rdflags)
{
    int ret = 0;

    /* Allocate the memory backing up the ramdisk from the kernel heap */

    m_ram_buffer = (uint8_t *)malloc(sectsize * nsectors);
    if (m_ram_buffer == NULL)
    {
      printf("ERROR: mmz_malloc() failed: %d\n", ret);
      return -ENOMEM;
    }

    memset(m_ram_buffer, 0, sectsize * nsectors);
    ret = ramdisk_register(minor, m_ram_buffer, nsectors, sectsize, rdflags);
    if (ret < 0)
    {
        printf("ERROR: ramdisk_register() failed: %d\n", ret);
        free(m_ram_buffer);
        m_ram_buffer = NULL;
    }

    return ret;
}

//Create a Ramdisk with the size in byte
int api_ramdisk_open(uint32_t size)
{
    struct fat_format_s fmt = FAT_FORMAT_INITIALIZER;
    int ret = 0;
    uint32_t nsectors;
    uint16_t sectsize = 512;

    if (m_ram_buffer)
        return 0;

    nsectors = size/sectsize;

    do{
        ret = _ramdisk_create(0,nsectors,sectsize,RDFLAG_WRENABLED | RDFLAG_FUNLINK);
        if (ret < 0 )
        {
            printf("%s(), error mkrd\n", __func__);
            if (m_ram_buffer)
                free(m_ram_buffer);
            m_ram_buffer = NULL;
            break;
        }

        ret = mkfatfs(m_ram_path, &fmt);
        if (ret < 0)
        {
            printf("%s(), error mkfatfs\n", __func__);
            break;
        }

        ret = mount(m_ram_path, m_mnt_path, "vfat", 0, NULL);
        if (ret < 0)
        {
            printf("%s(), error mount ramdisk\n", __func__);
            break;
        }    

        printf("%s(), mount %s to %s OK!\n", __func__, m_ram_path, m_mnt_path);

    }while(0);

    if (ret)
    {
        unlink(m_ram_path);
        m_ram_path = NULL;
    }
    return ret;
}

//Get the mount path of ramddisl
char *api_ramdisk_path_get()
{
    if (m_ram_buffer)
        return m_mnt_path;
    else
        return NULL;
}

//Close the ramdisk
int api_ramdisk_close()
{
    int ret = 0;
    ret = umount(m_mnt_path);
    if (ret < 0)
        printf("error umount ramdisk\n");

    //unlink will free m_ram_buffer
    unlink(m_ram_path);
    // if (m_ram_buffer)
    //     free(m_ram_buffer);

    m_ram_buffer = NULL;
}

#endif
