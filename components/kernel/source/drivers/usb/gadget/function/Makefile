cflags-y	+= -I$(src)
cflags-y	+= -I$(src)/../
cflags-y	+= -I$(src)/../function/
cflags-y	+=  -Wno-implicit-fallthrough -Wno-int-conversion -Wno-sign-compare -Wno-error=format=

## g_zero for usb gadget demo
usb_f_ss_lb-y			:= f_loopback.o f_sourcesink.o
obj-$(CONFIG_USB_ZERO)	+= usb_f_ss_lb.o
