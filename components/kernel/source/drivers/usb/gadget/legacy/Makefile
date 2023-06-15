cflags-y	+= -I$(src)
cflags-y	+= -I$(src)/../
cflags-y	+= -I$(src)/../function/

## g_zero for usb gadget demo
g_zero-y			:= zero.o

g_hid-y 			:= hid.o

obj-$(CONFIG_USB_ZERO)		+= g_zero.o
obj-$(CONFIG_USB_GADGET_HID_VENDOR) += g_hid.o
