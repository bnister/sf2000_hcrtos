#ifndef _HCUAPI_SYS_BLOCKING_NOTIFY_H_
#define _HCUAPI_SYS_BLOCKING_NOTIFY_H_

#if defined(__HCRTOS__)

#include <hcuapi/iocbase.h>

#define DISK_NAME_LEN 32

struct removable_notify_info {
	char devname[DISK_NAME_LEN];
};

struct hidg_func_descriptor;
struct usb_hid_dev_info {
	char devname[DISK_NAME_LEN];
	struct hidg_func_descriptor *report;
};

#define USB_MSC_NOTIFY_CONNECT		_IOR(USBDEVFS_IOCBASE, 100, struct removable_notify_info)
#define USB_MSC_NOTIFY_DISCONNECT	_IOR(USBDEVFS_IOCBASE, 101, struct removable_notify_info)

#define USB_MSC_NOTIFY_MOUNT		_IOR(USBDEVFS_IOCBASE, 102, struct removable_notify_info)
#define USB_MSC_NOTIFY_UMOUNT		_IOR(USBDEVFS_IOCBASE, 103, struct removable_notify_info)

#define SDMMC_NOTIFY_CONNECT		_IOR(SDMMC_IOCBASE, 100, struct removable_notify_info)
#define SDMMC_NOTIFY_DISCONNECT		_IOR(SDMMC_IOCBASE, 101, struct removable_notify_info)

#define SDMMC_NOTIFY_MOUNT		_IOR(SDMMC_IOCBASE, 102, struct removable_notify_info)
#define SDMMC_NOTIFY_UMOUNT		_IOR(SDMMC_IOCBASE, 103, struct removable_notify_info)

#define USB_DEV_NOTIFY_CONNECT		_IOR(USBDEVFS_IOCBASE, 104, struct removable_notify_info)	//!< devname contains vendor & product id: "v%04Xp%04X", eg: "v0BDApF179"
#define USB_DEV_NOTIFY_DISCONNECT	_IOR(USBDEVFS_IOCBASE, 105, struct removable_notify_info)	//!< devname contains vendor & product id: "v%04Xp%04X", eg: "v0BDApF179"

#define USB_MSC_NOTIFY_MOUNT_FAIL	_IOR(USBDEVFS_IOCBASE, 106, struct removable_notify_info)
#define USB_MSC_NOTIFY_UMOUNT_FAIL	_IOR(USBDEVFS_IOCBASE, 107, struct removable_notify_info)

#define USB_HID_KBD_NOTIFY_CONNECT	_IOR(USBDEVFS_IOCBASE, 108, struct usb_hid_dev_info)
#define USB_HID_KBD_NOTIFY_DISCONNECT	_IOR(USBDEVFS_IOCBASE, 109, struct usb_hid_dev_info)

#define USB_HID_MOUSE_NOTIFY_CONNECT	_IOR(USBDEVFS_IOCBASE, 110, struct usb_hid_dev_info)
#define USB_HID_MOUSE_NOTIFY_DISCONNECT	_IOR(USBDEVFS_IOCBASE, 111, struct usb_hid_dev_info)

#endif

#endif /* _HCUAPI_USBMSC_H_ */
