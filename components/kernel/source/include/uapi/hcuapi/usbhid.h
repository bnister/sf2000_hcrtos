#ifndef __HCUAPI_USB_HID_H__
#define __HCUAPI_USB_HID_H__

#include <hcuapi/iocbase.h>
#include <linux/usb/g_hid.h>

typedef int (*usbhid_hook_t)(char *data, int len);

#define USBHID_SET_HOOK			_IOW (USBHID_IOCBASE, 0, usbhid_hook_t)
#define USBHID_CLEAR_HOOK		_IO (USBHID_IOCBASE, 1)
#define USBHID_GET_REPORT_DESC		_IOR (USBHID_IOCBASE, 2, struct hidg_func_descriptor *)

#endif	/* __HCUAPI_USB_HID_H__ */
