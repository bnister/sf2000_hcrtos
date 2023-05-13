APPS_B2BUPDATER_SUPPORT_SEPARATE_OUTPUT = YES
APPS_B2BUPDATER_DEPENDENCIES += $(apps_deps-y)
APPS_B2BUPDATER_ALWAYS_BUILD = yes

APPS_B2BUPDATER_DTS_NAME = $(basename $(filter %.dts,$(notdir $(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)))))

ifneq ($(CONFIG_CUSTOM_DTS_PATH),)
define APPS_B2BUPDATER_ENTRY_ADDR_FROM_DTS
	-rm -rf $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr
	mkdir -p $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr
	$(foreach dts,$(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)),
		cp -f $(dts) $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/
	)
	echo "unsigned int bootloader_load_addr = (HCRTOS_SYSMEM_OFFSET + 0x1000);" >> $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/$(strip $(APPS_B2BUPDATER_DTS_NAME)).dts
	gcc -O2 -I$(KERNEL_PKGD)/include -E -Wp,-MMD,$(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/$(strip $(APPS_B2BUPDATER_DTS_NAME)).dtb.d.pre.tmp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/$(strip $(APPS_B2BUPDATER_DTS_NAME)).dtb.dts.tmp $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/$(strip $(APPS_B2BUPDATER_DTS_NAME)).dts
	cat $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/$(strip $(APPS_B2BUPDATER_DTS_NAME)).dtb.dts.tmp | grep bootloader_load_addr > $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/main.c
	echo -e " \
#include <stdio.h>\n \
int main(int argc, char **argv) \
{ \
	printf(\"0x%08x\", bootloader_load_addr | 0x80000000); \
	return 0; \
}" >> $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/main.c
	gcc -o $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/a.out $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/main.c
	$(TOPDIR)/build/scripts/update_entry_addr.sh \
		--genaddrout $(APPS_B2BUPDATER_DIR)/.tmp-for-load-addr/a.out \
		--entryld $(APPS_B2BUPDATER_PKGD)/entry.ld
endef
else ifeq ($(BR2_PACKAGE_APPS_B2BUPDATER),y)
$(info "CONFIG_CUSTOM_DTS_PATH is not defined, not able to update entry address of bootloader application")
endif

APPS_B2BUPDATER_PRE_BUILD_HOOKS += APPS_B2BUPDATER_ENTRY_ADDR_FROM_DTS

$(eval $(generic-package))
