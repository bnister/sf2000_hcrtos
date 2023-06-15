#ifndef __HCUSB_H__
#define __HCUSB_H__

#include <linux/types.h>
#include <linux/usb/musb.h>


/* predefined index for usb controller */
enum {
	USB_PORT_0	= 0,
	USB_PORT_1,
    USB_PORT_MAX
};
struct hcusb_info {
	u8			usb_port;
	const char		*s;
};

extern struct hcusb_info hcusb_controller[];

/* get usb device mode contorller name */
inline const char *get_udc_name(uint8_t usb_port)
{
    if(usb_port >= USB_PORT_MAX)
        return NULL;
    return hcusb_controller[usb_port].s;
}


/* 
** brief: setup USB gadget mode as MASS-STORAGE device
** parm: 
**   path -- the array of path name
**   luns -- numbers of logic units
** return:
**   0  -- successfully 
**   !0 -- failure
*/
int hcusb_gadget_msg_init(char **path, int luns);
int hcusb_gadget_msg_deinit(void);
int hcusb_gadget_msg_specified_init(const char *udc_name, char **path, int luns);


/*
** brief: setup USB gadget mode as SERIAL device
*/
int hcusb_gadget_serial_init(void);
void hcusb_gadget_serial_deinit(void);
int hcusb_gadget_serial_specified_init(const char *udc_name);


/*
** brief: setup USB gadget mode as NCM device
*/
int hcusb_gadget_ncm_init(void);
void hcusb_gadget_ncm_deinit(void);
int hcusb_gadget_ncm_specified_init(const char *udc_name);


/*
** brief: setup USB gadget mode as HID device
*/
struct hidg_func_descriptor;

int hcusb_gadget_hidg_init(void);
void hcusb_gadget_hidg_deinit(void);
int hcusb_gadget_hidg_specified_init(const char *udc_name);
int hcusb_gadget_hidg_mouse_kbd_init(const char *udc_name,
                    struct hidg_func_descriptor *mouse,
                    struct hidg_func_descriptor *keyboard);

/* 
** brief: setup USB mode as HOST or GADGET
** parm: 
**   usb_port -- 0:usb0, 1:usb1
**   mode -- usb mode (MUSB_HOST, MUSB_PERIPHERAL, MUSB_OTG)
** return:
**   0  -- successfully 
**   !0 -- failure
*/
int hcusb_set_mode(uint8_t usb_port, enum musb_mode mode);



/* 
** brief: return USB mode as HOST or GADGET
** parm: 
**   usb_port -- 0:usb0, 1:usb1
** return:
**   mode -- usb mode (MUSB_HOST, MUSB_PERIPHERAL, MUSB_OTG)
*/
enum musb_mode hcusb_get_mode(uint8_t usb_port);



/***************************************************/
/*************  usb gadget vendor ******************/
/***************************************************/


int hcusb_gadget_zero_init(void);
void hcusb_gadget_zero_deinit(void);
int hcusb_gadget_zero_specified_init(const char *udc_name);

#endif /* __HCUSB_H__ */
