cflags-y += -I$(src)/../linux-porting/include -Wno-sign-compare

__usb_cmds-y = usb_cmds_main.o 
__usb_cmds-$(CONFIG_CMDS_USB_HID) += usb_cmd_hid.o
__usb_cmds-$(CONFIG_CMDS_USB_HID) += usb_cmd_hid_dev.o 

__usb_cmds-$(CONFIG_CMDS_USB_GADGET_MSC) += usb_gadget_cmd_msc.o 
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_NCM) += usb_gadget_cmd_ncm.o 
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_SERIAL) += usb_gadget_cmd_serial.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_ZERO) += usb_gadget_cmd_zero.o 
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_HID) += usb_gadget_cmd_hid.o 

obj-y += __usb_cmds.o

obj-$(CONFIG_CMDS_LIBUSB_EXAMPLES) += libusb_examples/
