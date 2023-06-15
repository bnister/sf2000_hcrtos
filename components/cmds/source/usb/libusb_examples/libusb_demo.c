#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <kernel/delay.h>
#include <kernel/lib/console.h>
#include <linux/bitops.h>
#include <linux/bits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <nuttx/fs/dirent.h>
#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>
#include <string.h>

#include <libusb.h>


#undef LOG_TAG
#define LOG_TAG "libusb_example"
#undef ELOG_OUTPUT_LVL
#define ELOG_OUTPUT_LVL ELOG_LVL_DEBUG
#include <kernel/elog.h>

#include <uapi/linux/usbdevice_fs.h>


#define USBFS_SPEED_UNKNOWN			0
#define USBFS_SPEED_LOW				1
#define USBFS_SPEED_FULL			2
#define USBFS_SPEED_HIGH			3
#define USBFS_SPEED_WIRELESS			4
#define USBFS_SPEED_SUPER			5
#define USBFS_SPEED_SUPER_PLUS			6

static void usbfs_get_speed(int fd)
{
	int r;

	r = ioctl(fd, USBDEVFS_GET_SPEED, NULL);
	switch (r) {
	case USBFS_SPEED_UNKNOWN:	log_i("-- UNKNOWN SPEED\n"); break;
	case USBFS_SPEED_LOW:		log_i("-- LOW SPEED\n"); break;
	case USBFS_SPEED_FULL:		log_i("-- FULL SPEED\n"); break;
	case USBFS_SPEED_HIGH:		log_i("-- HIGH SPEED\n"); break;
	case USBFS_SPEED_WIRELESS:	log_i("-- WIRELESS SPEED\n"); break;
	case USBFS_SPEED_SUPER:		log_i("-- SUPER SPEED\n"); break;
	case USBFS_SPEED_SUPER_PLUS:	log_i("-- SUPER PLUS SPEED\n"); break;
	default:
		log_w("Error getting device speed: %d\n", r);
	}
}

static void usbfs_test(char *path, uint32_t mode)
{
    int fd = open(path, mode);
    if(fd < 0){
        log_e("cannot open %s, mode:%lu\n", path, mode);
        return;
    }
    log_i("open successfully  %s, mode:%lu, fd=%u\n", path, mode, fd);
    usbfs_get_speed(fd);
    close(fd);
}



int libusb_helloworld_demo(int argc, char **argv)
{
    // usbfs_test("/dev/bus/usb/001/001", O_RDONLY);
    // usbfs_test("/dev/bus/usb/001/001", O_RDWR);
    // usbfs_test("/dev/bus/usb/001/001", O_RDONLY | O_CLOEXEC);
    // usbfs_test("/dev/bus/usb/001/001", O_RDWR | O_CLOEXEC);

    // usbfs_test("/dev/bus/usb/001/002", O_RDONLY);
    // usbfs_test("/dev/bus/usb/001/002", O_RDWR);
    // usbfs_test("/dev/bus/usb/001/002", O_RDONLY | O_CLOEXEC);
    // usbfs_test("/dev/bus/usb/001/002", O_RDWR | O_CLOEXEC);

    // usbfs_test("/dev/bus/usb/002/001", O_RDONLY);
    // usbfs_test("/dev/bus/usb/002/001", O_RDWR);
    // usbfs_test("/dev/bus/usb/002/001", O_RDONLY | O_CLOEXEC);
    // usbfs_test("/dev/bus/usb/002/001", O_RDWR | O_CLOEXEC);

    printf(" ==> hello world for libusb examples\n");
    printf(" ==> try libusb_init\n");
    libusb_init(NULL);
    printf(" ==> try libusb_exit\n");
    libusb_exit(NULL);
    return 0;
}


static int count = 0;
static libusb_hotplug_callback_handle usb_hotplug_cb_handle;

static int um_hotplug_func(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data)
{
    struct libusb_device_descriptor desc;
    struct libusb_config_descriptor *config;
    int rc;

    if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event)
    {
        rc = libusb_get_device_descriptor(device, &desc);
        if (LIBUSB_SUCCESS != rc)
            log_e("Error getting device descriptor\n");
        log_i("USB plug-in %p: %.4x:%.2x\n", device, desc.idVendor, desc.bDeviceClass);
        // if (!um_hotplug_ignore_device(&desc))
        //     um_hotplug_add_device(device, &desc);

        if(0 != libusb_get_active_config_descriptor(device, &config)){
            log_e("Cannot get active config descriptor\n");
        }else{
            log_i("Get active config descriptor : \n");
            log_i("bLength:0x%x\n", config->bLength);
            log_i("bDescriptorType:0x%x\n", config->bDescriptorType);
            log_i("wTotalLength:0x%x\n", config->wTotalLength);
            log_i("bNumInterfaces:0x%x\n", config->bNumInterfaces);
            log_i("bConfigurationValue:0x%x\n", config->bConfigurationValue);
            log_i("iConfiguration:0x%x\n", config->iConfiguration);
            log_i("bmAttributes:0x%x\n", config->bmAttributes);
            log_i("MaxPower:0x%x\n\n", config->MaxPower);
        }
    }
    else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event)
    {
        log_i("USB plug-out %p\n", device);
        // um_hotplug_del_device(device);
    }
    else
    {
        log_e("Unhandled event %d\n", event);
    }

    count++;
    return 0;
}


static void hotplug_event_daemon(void *arg)
{
    count = 0;
    while (count < 20) {
        libusb_handle_events_completed(NULL, NULL);
        msleep(100);
    }

    log_i("hotplug_event_daemon exit .....\n");
    libusb_hotplug_deregister_callback(NULL, usb_hotplug_cb_handle);
    libusb_exit(NULL);
    vTaskDelete(NULL);
}


int hotplug(int argc, char** argv)
{
    int res;
    struct libusb_context *ctx;
    const struct libusb_version* libusb_version_info;
	int event_count_no_enumerate = 0;

    libusb_version_info = libusb_get_version();
    log_i("Using libusb %d.%d.%d\n", libusb_version_info->major,
                   libusb_version_info->minor, libusb_version_info->micro);
    res = libusb_init(&ctx);
    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
    if (res != 0) {
        log_e("libusb_init failed: %s\n", libusb_error_name(res));
        return -1;
    }

    if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)){
        log_e("libusb do not support hotplug\n");
        return -1;
    }

    log_i("Registering for libusb hotplug events\n");
    res = libusb_hotplug_register_callback(ctx, 
                                            LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                                            LIBUSB_HOTPLUG_ENUMERATE, 
                                            LIBUSB_HOTPLUG_MATCH_ANY,
                                            LIBUSB_HOTPLUG_MATCH_ANY,
                                            LIBUSB_HOTPLUG_MATCH_ANY,
                                            um_hotplug_func,
                                            &event_count_no_enumerate,
                                            &usb_hotplug_cb_handle);
    if (res == LIBUSB_SUCCESS)
        log_i("Register libusb hotplug success\n");
    else
        log_e("ERROR: Could not register for libusb hotplug events. %sc", libusb_error_name(res));

    xTaskCreate(hotplug_event_daemon, (const char *)"libusb_daemon", 
                configTASK_STACK_DEPTH,
                NULL, portPRI_TASK_NORMAL, 
                NULL);

    return 0;
}
