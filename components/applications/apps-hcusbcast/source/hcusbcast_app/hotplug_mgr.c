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
#else
    #include <kernel/notify.h>
    #include <linux/notifier.h>
    #include <hcuapi/sys-blocking-notify.h>
#endif
#include <pthread.h>
#include <netdb.h>
#include <hcuapi/common.h>
#include <hcuapi/kumsgq.h>
#include "com_api.h"
#include "data_mgr.h"
#include "cast_api.h"
#include <linux/usb.h>

static USB_STATE m_usb_state = USB_STAT_INVALID;

static int hotplug_usb_notify(struct notifier_block *self, unsigned long action, void *dev)
{
    switch (action)
    {
    case USB_MSC_NOTIFY_MOUNT:
        printf("USB Plug In!\n");
        m_usb_state = USB_STAT_MOUNT;
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

        break;
    }

    return NOTIFY_OK;
}

static struct notifier_block hotplug_usb_nb =
{
    .notifier_call = hotplug_usb_notify,
};


void hotplug_init(void)
{
    sys_register_notify(&hotplug_usb_nb);
}

int hotplug_usb_get(void)
{
    return m_usb_state;
}


