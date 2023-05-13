APPS_HCSCREEN_SUPPORT_SEPARATE_OUTPUT = YES
APPS_HCSCREEN_DEPENDENCIES += $(apps_deps-y)
APPS_HCSCREEN_ALWAYS_BUILD = yes

APPS_HCSCREEN_DTS_NAME = $(basename $(filter %.dts,$(notdir $(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)))))

APPS_HCSCREEN_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(APPS_HCSCREEN_MAKE_ENV) $(MAKE) $(APPS_HCSCREEN_MAKE_FLAGS) -C $(@D) clean
APPS_HCSCREEN_BUILD_CMDS = rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/applications/apps-hcscreen/source/ $(@D) && \
		     if [ -f $(@D)/Makefile ];then \
		     mv $(@D)/Makefile $(@D)/Makefile.old; fi && \
		     cp $(@D)/Makefile.rtos $(@D)/Makefile;\
		     cp $(@D)/main.rtos.c $(@D)/main.c;\
			 cp $(@D)/hcscreen_app/HCCAST_1920x1080.264 ${IMAGES_DIR}/fs-partition1-root/HCCAST_1920x1080.264;\
			 cp $(@D)/hcscreen_app/music_bg_logo_19KB.264 ${IMAGES_DIR}/fs-partition1-root/music_bg_logo.264;\
			 mkdir -p $(@D)/src;\
			 ln -sf $(@D)/Makefile.src.rtos $(@D)/src/Makefile;\
			 ln -sf $(@D)/hcscreen_app $(@D)/src/;\
		     $(TARGET_MAKE_ENV) $(APPS_HCSCREEN_MAKE_ENV) $(MAKE) $(APPS_HCSCREEN_MAKE_FLAGS) -C $(@D) all

APPS_HCSCREEN_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(APPS_HCSCREEN_MAKE_ENV) $(MAKE) $(APPS_HCSCREEN_MAKE_FLAGS) -C $(@D) install

ifneq ($(CONFIG_CUSTOM_DTS_PATH),)
define APPS_HCSCREEN_ENTRY_ADDR_FROM_DTS
	-rm -rf $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr
	mkdir -p $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr
	$(foreach dts,$(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)),
		cp -f $(dts) $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/
	)

	echo "unsigned int avp_load_addr = (HCRTOS_SYSMEM_OFFSET + 0x1000);" >> $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREEN_DTS_NAME)).dts
	gcc -O2 -I$(KERNEL_PKGD)/include -E -Wp,-MMD,$(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREEN_DTS_NAME)).dtb.d.pre.tmp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREEN_DTS_NAME)).dtb.dts.tmp $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREEN_DTS_NAME)).dts
	cat $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREEN_DTS_NAME)).dtb.dts.tmp | grep avp_load_addr > $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/main.c
	echo -e " \
#include <stdio.h>\n \
int main(int argc, char **argv) \
{ \
	printf(\"0x%08x\", avp_load_addr | 0x80000000); \
	return 0; \
}" >> $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/main.c
	gcc -o $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/a.out $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/main.c
	$(TOPDIR)/build/scripts/update_entry_addr.sh \
		--genaddrout $(APPS_HCSCREEN_DIR)/.tmp-for-load-addr/a.out \
		--entryld $(APPS_HCSCREEN_PKGD)/entry.ld

endef
else ifeq ($(BR2_PACKAGE_APPS_HCSCREEN),y)
$(info "CONFIG_CUSTOM_DTS_PATH is not defined, not able to update entry address of avp application")
endif

APPS_HCSCREEN_PRE_BUILD_HOOKS += APPS_HCSCREEN_ENTRY_ADDR_FROM_DTS

$(eval $(generic-package))

