ifeq ($(CONFIG_NET_LWIP_SACK_2_1_2),y)
LWIPDIR=../../third_party/lwip-2.1.2/src
else ifeq ($(CONFIG_NET_LWIP_SACK_2_1_3),y)
LWIPDIR=../../third_party/lwip-2.1.3/src
endif
LWIPCONTRIBDIR=../../third_party/lwip-contrib-2.1.0

cflags-y += -Wno-old-style-declaration
cflags-y += -D__LWIP__ -DLWIP_NOASSERT -D_BSD_SOURCE=1 -DLOSCFG_SHELL=1
#cflags-y += -D__LWIP__ -D_BSD_SOURCE=1 -DLOSCFG_SHELL=1
cflags-y += -I$(src)/enhancement/include
cflags-y += -I$(src)/porting/include
cflags-y += -I$(src)/$(LWIPDIR)/include
cflags-y += -I$(src)/$(LWIPCONTRIBDIR)/ports/freertos/include
cflags-y += -Wno-error=format=

CFLAGS_api_shell.o = -Wno-error=format=
CFLAGS_api_shell.o += -Wno-pointer-to-int-cast
CFLAGS_socket.o += -I$(srctree)/third_party/nuttx/fs/inode
CFLAGS_sockets.o += -Wno-unused-variable
#CFLAGS_api_shell.o += -O0
#CFLAGS_socket.o += -O0
#CFLAGS_sockets.o += -O0
#CFLAGS_driverif.o += -O0
#CFLAGS_netdb.o += -O0
#CFLAGS_fixme.o += -O0
#CFLAGS_dhcps.o += -O0

include $(src)/$(LWIPDIR)/Filelists.mk

LWIP_PORTING_FILES = porting/src/libc_s.c \
	porting/src/socket.c \
	porting/src/init.c \
	porting/src/netdb.c \
	porting/src/driverif.c \
	porting/src/sockets.c \
	enhancement/src/api_shell.c \
	enhancement/src/api_shell_iperf.c \
	enhancement/src/fixme.c \
	enhancement/src/dhcps.c \
	enhancement/packet/af_packet.c

LWIPNOAPPSFILES-y=$(COREFILES) \
	$(CORE4FILES) \
	$(APIFILES) \
	$(LWIPDIR)/netif/ethernet.c

LWIPNOAPPSFILES-y += $(LWIPERFFILES)

LWIPNOAPPSFILES-$(CONFIG_NET_LWIP_IPV6) += $(CORE6FILES)
LWIPNOAPPSFILES-$(CONFIG_NET_LWIP_PPP) += $(PPPFILES)
LWIPNOAPPSFILES-$(CONFIG_NET_LWIP_6LOWPAN) += $(SIXLOWPAN)
LWIPNOAPPSFILES-$(CONFIG_NET_LWIP_BRIDGE) += \
	$(LWIPDIR)/netif/bridgeif.c \
	$(LWIPDIR)/netif/bridgeif_fdb.c
LWIPNOAPPSFILES-$(CONFIG_NET_LWIP_SLIP) += \
	$(LWIPDIR)/netif/slipif.c

__sources = $(LWIP_PORTING_FILES)
__sources += $(LWIPNOAPPSFILES-y)
__sources += $(LWIPCONTRIBDIR)/ports/freertos/sys_arch.c

__sources_exclude = $(LWIPDIR)/api/sockets.c \
	$(LWIPDIR)/core/ipv4/dhcp.c \
	$(LWIPDIR)/core/ipv4/etharp.c

sources = $(filter-out $(__sources_exclude),$(__sources))

__lwip-y := ${sources:.c=.o}
obj-y += __lwip.o
