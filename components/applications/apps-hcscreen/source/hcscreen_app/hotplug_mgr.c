/*
hotplug_mgr.c: to manage the hotplug device, such as usb wifi, usb disk etc
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#ifdef __linux__
    #include <sys/epoll.h>
    #include <hcuapi/kumsgq.h>
#else
    #include <kernel/notify.h>
    #include <linux/notifier.h>
    #include <hcuapi/sys-blocking-notify.h>
#endif
#include <pthread.h>
#include <netdb.h>
#include <hccast/hccast_net.h>
#include <hcuapi/common.h>
#include <hcuapi/hdmi_tx.h>
#include "tv_sys.h"
#include "com_api.h"
#include "network_api.h"
#include "data_mgr.h"
#include "hotplug_mgr.h"
#include "cast_api.h"

#define MX_EVNTS (10)
#define EPL_TOUT (1000)

#define DEV_HDMI  "/dev/hdmi"

static int m_hdmi_tx_plugin = 0;
static int m_hdmi_rx_plugin = 0;
static USB_STATE m_usb_state = USB_STAT_INVALID;
static int m_wifi_plugin = 0;

static void _hotplug_hdmi_tx_tv_sys_set(void)
{
    int ap_tv_sys;
    int ap_tv_sys_ret;
    int last_tv_sys;

    last_tv_sys = data_mgr_de_tv_sys_get();
    ap_tv_sys = data_mgr_app_tv_sys_get();
    ap_tv_sys_ret = tv_sys_app_auto_set(ap_tv_sys, 2000);
    if (ap_tv_sys_ret >= 0)
    {
        if (APP_TV_SYS_AUTO == ap_tv_sys)
            data_mgr_app_tv_sys_set(APP_TV_SYS_AUTO);
        else
            data_mgr_app_tv_sys_set(ap_tv_sys_ret);
        data_mgr_save();
    }

    if (((last_tv_sys < TV_LINE_4096X2160_30) && (data_mgr_de_tv_sys_get() >= TV_LINE_4096X2160_30)) \
            || ((last_tv_sys >= TV_LINE_4096X2160_30) && (data_mgr_de_tv_sys_get() < TV_LINE_4096X2160_30)))
    {
        restart_air_service_by_hdmi_change();
    }
}

#ifdef __linux__
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT

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

typedef struct
{
    int epoll_fd;
    int hdmi_tx_fd;
    int kumsg_fd;
    int hotplug_fd;
} hotplug_fd_t;

static hotplug_fd_t m_hotplug_fd;

static struct epoll_event_data kumsg_data = { 0 };
static struct epoll_event_data hotplg_data = { 0 };
static struct epoll_event_data hotplg_msg_data = { 0 };

static void process_hotplug_msg(char *msg)
{
    //plug-out: ACTION=wifi-remove INFO=v0BDApF179
    //plug-in: ACTION=wifi-add INFO=v0BDApF179

    control_msg_t ctl_msg = {0};
    const char *plug_msg;

    plug_msg = (const char *)msg;
    if (strstr(plug_msg, "ACTION=wifi"))
    {
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
            //Enter upgrade window if there is upgraded file in USB-disk(hc_upgradexxxx.bin)
            //sys_upg_usb_check(5000);
        }
        else if (strstr(plug_msg, "ACTION=umount"))
        {
            m_usb_state = USB_STAT_UNMOUNT;
            sscanf(plug_msg, "ACTION=mount INFO=%s", mount_name);
            printf("U-disk is plug out: %s\n", mount_name);
            ctl_msg.msg_type = MSG_TYPE_USB_DISK_PLUGOUT;
        }
        api_control_send_msg(&ctl_msg);
    }
}

static void do_kumsg(KuMsgDH *msg)
{
    switch (msg->type)
    {
    case HDMI_TX_NOTIFY_CONNECT:
        m_hdmi_tx_plugin = 1;
        printf("%s(), line: %d. hdmi tx connect\n", __func__, __LINE__);
        break;
    case HDMI_TX_NOTIFY_DISCONNECT:
        m_hdmi_tx_plugin = 0;
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
    struct epoll_event events[MX_EVNTS];
    int n = -1;
    int i;
    struct sockaddr_in client;
    socklen_t sock_len = sizeof(client);
    int len;

    while (1)
    {
        n = epoll_wait(m_hotplug_fd.epoll_fd, events, MX_EVNTS, EPL_TOUT);
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
                len = read(fd, (void *)msg, sizeof(msg) - 1);
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

void hotplug_init(void)
{
    pthread_attr_t attr;
    pthread_t tid;
    struct sockaddr_un serv;
    struct epoll_event ev;
    int ret;

    struct kumsg_event event = { 0 };

    // start hot-plug detect
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    if (pthread_create(&tid, &attr, hotplug_receive_event_func, (void *)NULL))
    {
        printf("pthread_create receive_event_func fail\n");
        goto out;
    }

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
    return;
}

void hotplug_deinit(void)
{
    if (m_hotplug_fd.epoll_fd > 0)
        close(m_hotplug_fd.epoll_fd);
    if (m_hotplug_fd.hdmi_tx_fd > 0)
        close(m_hotplug_fd.hdmi_tx_fd);
    if (m_hotplug_fd.hotplug_fd > 0)
        close(m_hotplug_fd.hotplug_fd);

    // if (m_hotplug_fd.kumsg_fd > 0)
    //     close(m_hotplug_fd.kumsg_fd);
}
#endif
#endif

#else //rtos

#include <linux/usb.h>

#ifdef NETWORK_SUPPORT
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

static int hotplug_usb_notify(struct notifier_block *self, unsigned long action, void *dev)
{
    switch (action)
    {
    case USB_MSC_NOTIFY_MOUNT:
        printf("USB Plug In!\n");
        m_usb_state = USB_STAT_MOUNT;
        /*
        if(mounted_dev_name){
            free(mounted_dev_name);
            mounted_dev_name = NULL;
        }
        if(dev)
            mounted_dev_name = strdup(dev);
        //printf("USB_STAT_MOUNT: %s\n", mounted_dev_name);
        */
        break;
    case USB_MSC_NOTIFY_UMOUNT:
        printf("USB Plug Out!\n");
        m_usb_state= USB_STAT_UNMOUNT;
        break;
    case USB_MSC_NOTIFY_MOUNT_FAIL:
        printf("USB Plug mount fail!\n");
        m_usb_state = USB_STAT_MOUNT_FAIL;
        break;
    case USB_MSC_NOTIFY_UMOUNT_FAIL:
        printf("USB Plug unmount fail!\n");
        m_usb_state = USB_STAT_UNMOUNT_FAIL;
        break;
    default:
    #ifdef WIFI_SUPPORT
        usb_wifi_notify_check(action, dev);
        return NOTIFY_OK;
    #endif
        break;
    }

    return NOTIFY_OK;
}

static struct notifier_block hotplug_usb_nb =
{
    .notifier_call = hotplug_usb_notify,
};

static void *hotplug_first_wifi_detect_probed(void *arg)
{
    FILE *fp;
    control_msg_t ctl_msg = {0};

    do
    {
        char path[64] = {0};
        for (int i = 0; i < sizeof(wifi_model_list) / sizeof(wifi_model_list[0]); i++)
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

        usleep(500 * 1000);
    }
    while (1);

EXIT:
    return NULL;
}


static void _notifier_hdmi_tx_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);
    m_hdmi_tx_plugin = 1;

    return ;
}

static void _notifier_hdmi_tx_plugout(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);
    m_hdmi_tx_plugin = 0;
}

static void _notifier_hdmi_tx_ready(void *arg, unsigned long param)
{

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


void hotplug_init(void)
{
    pthread_t pid;

    sys_register_notify(&hotplug_usb_nb);
    hotplug_hdmi_tx_init();
    pthread_create(&pid, NULL, hotplug_first_wifi_detect_probed, NULL);
}

#endif
#endif

#endif

int hotplug_hdmi_tx_get(void)
{
    return m_hdmi_tx_plugin;
}

int hotplug_hdmi_rx_get(void)
{
    return m_hdmi_rx_plugin;
}

int hotplug_usb_get(void)
{
    return m_usb_state;
}

int hotplug_wifi_get(void)
{
    return m_wifi_plugin;
}
