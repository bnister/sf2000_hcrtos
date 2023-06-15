include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

qstrip=$(strip $(subst ",,$(1)))
#"))
DTS:=$(basename $(call qstrip,$(CONFIG_DEFAULT_DEVICE_TREE)))
DTS_SOURCE:=$(call qstrip,$(CONFIG_CUSTOM_DTS_PATH))
ARCH := $(CONFIG_SYS_ARCH:"%"=%)
export DTS

cflags-y += -I$(srctree)include/
cflags-y += -I$(srctree)include/uapi/
cflags-y += -I$(srctree)freertos/include/
cflags-y += -I$(srctree)lib/easylogger/inc/
cflags-y += -I$(srctree)arch/$(ARCH)/include/
cflags-y += -I$(srctree)arch/$(ARCH)/include/uapi/
cflags-y += -I$(srctree)/third_party/nuttx/include
cflags-y += -I$(srctree)/third_party/linux/include
cflags-y += -Wunused-variable

target-y += libkernel.a

obj-y += arch/$(ARCH)/
obj-y += freertos/
obj-y += drivers/
obj-y += lib/
obj-y += fs/
obj-y += net/
obj-y += sound/
obj-y += sched/
obj-y += third_party/linux/

all : $(target-y)

libkernel.a: built-in.o
	$(Q)rm -f $@
	$(Q)$(AR) -rc $@ $^

ifeq ($(CONFIG_CUSTOM_DTS),y)
built-in.o: $(DTS).dtb
endif

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -d -m 0775 $(STAGING_DIR)/usr/include_fix
	cp -rf $(srctree)include/* $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)include/uapi/* $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)freertos/include/* $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)lib/easylogger/inc/* $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)arch/$(ARCH)/include/* $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)arch/$(ARCH)/include/uapi/* $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)/third_party/nuttx/include/* $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)/third_party/linux/include/* $(STAGING_DIR)/usr/include_fix/
ifeq ($(CONFIG_NET),y)
	cp -rf $(srctree)/net/include/* $(STAGING_DIR)/usr/include_fix/
endif
	install -d -m 0775 $(STAGING_DIR)/usr/lib
	install -m 0664 $(target-y) $(STAGING_DIR)/usr/lib/
ifeq ($(CONFIG_CUSTOM_DTS),y)
	install -m 0755 -D $(DTS).dtb $(IMAGES_DIR)/$(DTS).dtb
	install -m 0755 -D $(DTS).dtb $(IMAGES_DIR)/dtb.bin
endif

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	-rm -f libkernel.a
	-rm -f $(DTS).dtb

$(DTS).dtb: $(DTS_SOURCE)
	if test -z "$(DTS_SOURCE)"; then \
		echo "No device tree defined in CONFIG_CUSTOM_DTS_PATH, Please check make menuconfig"; \
		exit 1; \
	fi
	if test ! -f $(HOST_DIR)/bin/dtc; then \
		cp -vf $(TOPDIR)/build/tools/dtc $(HOST_DIR)/bin/dtc; \
	fi
	cp -vf $(DTS_SOURCE) $(@D)
	$(Q)cat $(@D)/$(DTS).dts > $(@D)/.$(DTS).dtb.pre.tmp;
	$(Q)$(CC) -I$(srctree)/include -E -Wp,-MD,$(@D)/.$(DTS).dtb.d.pre.tmp \
		-nostdinc $(cflags-y) $(ccflags-y) $(cflags) \
		-D__ASSEMBLY__ -undef -D__DTS__ -x assembler-with-cpp \
		-o $(@D)/.$(DTS).dtb.dts.tmp \
		$(@D)/.$(DTS).dtb.pre.tmp
	$(Q)$(HOST_DIR)/bin/dtc -O dtb -o $(@D)/$(DTS).dtb -b 0 \
		-Wno-unit_address_vs_reg -Wno-unit_address_format \
		-Wno-avoid_unnecessary_addr_size -Wno-alias_paths \
		-Wno-graph_child_address -Wno-graph_port -Wno-unique_unit_address \
		-Wno-simple_bus_reg -Wno-pci_device_reg -Wno-pci_bridge \
		-Wno-pci_device_bus_num -Wno-unit_address_vs_reg \
		-Wno-unit_address_format -Wno-avoid_unnecessary_addr_size \
		-Wno-alias_paths -Wno-graph_child_address -Wno-graph_port \
		-Wno-unique_unit_address -Wno-simple_bus_reg -Wno-pci_device_reg \
		-Wno-pci_bridge -Wno-pci_device_bus_num \
		-d $(@D)/.$(DTS).dtb.d.dtc.tmp \
		$(@D)/.$(DTS).dtb.dts.tmp || \
		(echo "Check $(@D)/.$(DTS).dtb.pre.tmp for errors" && false)

.PHONY: FORCE

FORCE:

endif	# skip-makefile
