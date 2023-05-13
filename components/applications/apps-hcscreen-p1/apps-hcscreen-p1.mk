APPS_HCSCREEN_P1_VERSION = 4dad7948f3d44e6eb0286f64496715d33e21291c
APPS_HCSCREEN_P1_SITE_METHOD = git
APPS_HCSCREEN_P1_SITE = ssh://git@hichiptech.gitlab.com:33888/hccpkgs/apps-hcscreen-p1.git
APPS_HCSCREEN_P1_DEPENDENCIES += $(apps_deps-y)

APPS_HCSCREEN_P1_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(APPS_HCSCREEN_P1_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(APPS_HCSCREEN_P1_VERSION)

APPS_HCSCREEN_P1_MAKE_FLAGS ?= \
	CROSS_COMPILE=$(TARGET_CROSS) CXXFLAGS="$(TARGET_CXXFLAGS)" CFLAGS="$(TARGET_CFLAGS)" LDFLAGS="$(TARGET_LDFLAGS)"

APPS_HCSCREEN_P1_ALWAYS_BUILD = yes

APPS_HCSCREEN_P1_DTS_NAME = $(basename $(filter %.dts,$(notdir $(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)))))

APPS_HCSCREEN_P1_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(APPS_HCSCREEN_P1_MAKE_ENV) $(MAKE) $(APPS_HCSCREEN_P1_MAKE_FLAGS) -C $(@D) clean
APPS_HCSCREEN_P1_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(APPS_HCSCREEN_P1_MAKE_ENV) $(MAKE) $(APPS_HCSCREEN_P1_MAKE_FLAGS) -C $(@D) install

define APPS_HCSCREEN_P1_BUILD_CMDS
	cp $(@D)/Makefile.rtos $(@D)/Makefile
	cp $(@D)/main.rtos.c $(@D)/main.c
	cp $(@D)/hcscreen_app/HCCAST_1920x1080.264 ${IMAGES_DIR}/fs-partition1-root/HCCAST_1920x1080.264;
	cp $(@D)/hcscreen_app/music_bg_logo_19KB.264 ${IMAGES_DIR}/fs-partition1-root/music_bg_logo.264;
	mkdir -p $(@D)/src;
	ln -sf $(@D)/Makefile.src.rtos $(@D)/src/Makefile;
	ln -sf $(@D)/hcscreen_app $(@D)/src/;
	$(TARGET_MAKE_ENV) $(APPS_HCSCREEN_P1_MAKE_ENV) $(MAKE) $(APPS_HCSCREEN_P1_MAKE_FLAGS) -C $(@D) all
endef

ifneq ($(CONFIG_CUSTOM_DTS_PATH),)
define APPS_HCSCREEN_P1_ENTRY_ADDR_FROM_DTS
	-rm -rf $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr
	mkdir -p $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr
	$(foreach dts,$(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)),
		cp -f $(dts) $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/
	)

	echo "unsigned int avp_load_addr = (HCRTOS_SYSMEM_OFFSET + 0x1000);" >> $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREEN_P1_DTS_NAME)).dts
	gcc -O2 -I$(KERNEL_PKGD)/include -E -Wp,-MMD,$(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREEN_P1_DTS_NAME)).dtb.d.pre.tmp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREEN_P1_DTS_NAME)).dtb.dts.tmp $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREEN_P1_DTS_NAME)).dts
	cat $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREEN_P1_DTS_NAME)).dtb.dts.tmp | grep avp_load_addr > $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/main.c
	echo -e " \
#include <stdio.h>\n \
int main(int argc, char **argv) \
{ \
	printf(\"0x%08x\", avp_load_addr | 0x80000000); \
	return 0; \
}" >> $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/main.c
	gcc -o $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/a.out $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/main.c
	$(TOPDIR)/build/scripts/update_entry_addr.sh \
		--genaddrout $(APPS_HCSCREEN_P1_DIR)/.tmp-for-load-addr/a.out \
		--entryld $(@D)/entry.ld

endef
else ifeq ($(BR2_PACKAGE_APPS_HCSCREEN_P1),y)
$(info "CONFIG_CUSTOM_DTS_PATH is not defined, not able to update entry address of avp application")
endif

APPS_HCSCREEN_P1_PRE_BUILD_HOOKS += APPS_HCSCREEN_P1_ENTRY_ADDR_FROM_DTS

$(eval $(generic-package))
