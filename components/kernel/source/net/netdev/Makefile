ifeq ($(CONFIG_NET_LWIP_SACK_2_1_2),y)
LWIPDIR=../../third_party/lwip-2.1.2/src
LWIPPORTDIR=../lwip-2.1.2
else ifeq ($(CONFIG_NET_LWIP_SACK_2_1_3),y)
LWIPDIR=../../third_party/lwip-2.1.3/src
LWIPPORTDIR=../lwip-2.1.3
endif
LWIPCONTRIBDIR=../../third_party/lwip-contrib-2.1.0

CFLAGS_LWIP_INCLUDE  = -I$(src)/$(LWIPPORTDIR)/enhancement/include
CFLAGS_LWIP_INCLUDE += -I$(src)/$(LWIPPORTDIR)/porting/include
CFLAGS_LWIP_INCLUDE += -I$(src)/$(LWIPDIR)/include
CFLAGS_LWIP_INCLUDE += -I$(src)/$(LWIPCONTRIBDIR)/ports/freertos/include

CFLAGS_net_device_adapter.o += -I$(src)/$(LWIPPORTDIR)/enhancement/include
CFLAGS_net_device_adapter.o += -I$(src)/$(LWIPPORTDIR)/porting/include
CFLAGS_net_device_adapter.o += -I$(src)/$(LWIPDIR)/include
CFLAGS_net_device_adapter.o += -I$(src)/$(LWIPCONTRIBDIR)/ports/freertos/include

CFLAGS_net_device_adapter.o += -Wno-format-nonliteral $(CFLAGS_LWIP_INCLUDE)
CFLAGS_wext-core.o += $(CFLAGS_LWIP_INCLUDE) -DCONFIG_WEXT_PRIV -DCONFIG_WIRELESS_EXT
#CFLAGS_wext-core.o += -O0
#CFLAGS_wext-priv.o += -O0
CFLAGS_af_monitor.o += $(CFLAGS_LWIP_INCLUDE)

# CAUTION: -O0 option can only be used when lwip is built with -O0
#          because the size of struct may be different
# CFLAGS_net_device_adapter.o += -O0

__netdev-y := dev.o skbuff.o net_device_adapter.o
__netdev-y += wext-core.o wext-priv.o
__netdev-y += af_netlink.o
__netdev-y += eth.o
__netdev-y += af_monitor.o

obj-y += __netdev.o
