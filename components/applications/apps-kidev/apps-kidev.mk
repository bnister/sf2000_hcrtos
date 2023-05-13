APPS_KIDEV_VERSION = ed09ab024d8c5a339739b61657cd60e86a6e6bc4
APPS_KIDEV_SITE_METHOD = git
APPS_KIDEV_SITE = ssh://git@hichiptech.gitlab.com:33888/hccpkgs/apps-kidev.git
APPS_KIDEV_DEPENDENCIES += $(apps_deps-y)

APPS_KIDEV_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(APPS_KIDEV_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(APPS_KIDEV_VERSION)

APPS_KIDEV_MAKE_FLAGS ?= \
	CROSS_COMPILE=$(TARGET_CROSS) CXXFLAGS="$(TARGET_CXXFLAGS)" CFLAGS="$(TARGET_CFLAGS)" LDFLAGS="$(TARGET_LDFLAGS)"

APPS_KIDEV_ALWAYS_BUILD = yes

APPS_KIDEV_DTS_NAME = $(basename $(filter %.dts,$(notdir $(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)))))

APPS_KIDEV_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(APPS_KIDEV_MAKE_ENV) $(MAKE) $(APPS_KIDEV_MAKE_FLAGS) -C $(@D) clean
APPS_KIDEV_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(APPS_KIDEV_MAKE_ENV) $(MAKE) -C $(@D) install

ifeq ($(BR2_PACKAGE_HCCAST_DLNA),y)
APPS_KIDEV_BUILD_CMDS = \
	cp $(@D)/src/kidev/hc_cast_logo.m2v ${IMAGES_DIR}/fs-partition1-root/hc_cast_logo.m2v; \
	cp $(@D)/src/kidev/music_bg_logo.m2v ${IMAGES_DIR}/fs-partition1-root/music_bg_logo.m2v; \
	$(TARGET_MAKE_ENV) $(APPS_KIDEV_MAKE_ENV) $(MAKE) $(APPS_KIDEV_MAKE_FLAGS) -C $(@D) all
else			 
APPS_KIDEV_BUILD_CMDS = \
	$(TARGET_MAKE_ENV) $(APPS_KIDEV_MAKE_ENV) $(MAKE) $(APPS_KIDEV_MAKE_FLAGS) -C $(@D) all
endif

ifneq ($(CONFIG_CUSTOM_DTS_PATH),)
define APPS_KIDEV_ENTRY_ADDR_FROM_DTS
	-rm -rf $(@D)/.tmp-for-load-addr
	mkdir -p $(@D)/.tmp-for-load-addr
	$(foreach dts,$(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)),
		cp -f $(dts) $(@D)/.tmp-for-load-addr/
	)
	echo "unsigned int avp_load_addr = (HCRTOS_SYSMEM_OFFSET + 0x1000);" >> $(@D)/.tmp-for-load-addr/$(strip $(APPS_KIDEV_DTS_NAME)).dts
	gcc -O2 -I$(KERNEL_PKGD)/include -E -Wp,-MMD,$(@D)/.tmp-for-load-addr/$(strip $(APPS_KIDEV_DTS_NAME)).dtb.d.pre.tmp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o $(@D)/.tmp-for-load-addr/$(strip $(APPS_KIDEV_DTS_NAME)).dtb.dts.tmp $(@D)/.tmp-for-load-addr/$(strip $(APPS_KIDEV_DTS_NAME)).dts
	cat $(@D)/.tmp-for-load-addr/$(strip $(APPS_KIDEV_DTS_NAME)).dtb.dts.tmp | grep avp_load_addr > $(@D)/.tmp-for-load-addr/main.c
	echo -e " \
#include <stdio.h>\n \
int main(int argc, char **argv) \
{ \
	printf(\"0x%08x\", avp_load_addr | 0x80000000); \
	return 0; \
}" >> $(@D)/.tmp-for-load-addr/main.c
	gcc -o $(@D)/.tmp-for-load-addr/a.out $(@D)/.tmp-for-load-addr/main.c
	$(TOPDIR)/build/scripts/update_entry_addr.sh \
		--genaddrout $(@D)/.tmp-for-load-addr/a.out \
		--entryld $(@D)/entry.ld
endef
else ifeq ($(BR2_PACKAGE_APPS_KIDEV),y)
$(info "CONFIG_CUSTOM_DTS_PATH is not defined, not able to update entry address of avp application")
endif

APPS_KIDEV_PRE_BUILD_HOOKS += APPS_KIDEV_ENTRY_ADDR_FROM_DTS

$(eval $(generic-package))
