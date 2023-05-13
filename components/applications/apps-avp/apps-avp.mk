APPS_AVP_SUPPORT_SEPARATE_OUTPUT = YES
APPS_AVP_DEPENDENCIES += $(apps_deps-y)
APPS_AVP_ALWAYS_BUILD = yes

APPS_AVP_DTS_NAME = $(basename $(filter %.dts,$(notdir $(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)))))

ifneq ($(CONFIG_CUSTOM_DTS_PATH),)
define APPS_AVP_ENTRY_ADDR_FROM_DTS
	-rm -rf $(APPS_AVP_DIR)/.tmp-for-load-addr
	mkdir -p $(APPS_AVP_DIR)/.tmp-for-load-addr
	$(foreach dts,$(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)),
		cp -f $(dts) $(APPS_AVP_DIR)/.tmp-for-load-addr/
	)
	echo "unsigned int avp_load_addr = (HCRTOS_SYSMEM_OFFSET + 0x1000);" >> $(APPS_AVP_DIR)/.tmp-for-load-addr/$(strip $(APPS_AVP_DTS_NAME)).dts
	gcc -O2 -I$(KERNEL_PKGD)/include -E -Wp,-MMD,$(APPS_AVP_DIR)/.tmp-for-load-addr/$(strip $(APPS_AVP_DTS_NAME)).dtb.d.pre.tmp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o $(APPS_AVP_DIR)/.tmp-for-load-addr/$(strip $(APPS_AVP_DTS_NAME)).dtb.dts.tmp $(APPS_AVP_DIR)/.tmp-for-load-addr/$(strip $(APPS_AVP_DTS_NAME)).dts
	cat $(APPS_AVP_DIR)/.tmp-for-load-addr/$(strip $(APPS_AVP_DTS_NAME)).dtb.dts.tmp | grep avp_load_addr > $(APPS_AVP_DIR)/.tmp-for-load-addr/main.c
	echo -e " \
#include <stdio.h>\n \
int main(int argc, char **argv) \
{ \
	printf(\"0x%08x\", avp_load_addr | 0x80000000); \
	return 0; \
}" >> $(APPS_AVP_DIR)/.tmp-for-load-addr/main.c
	gcc -o $(APPS_AVP_DIR)/.tmp-for-load-addr/a.out $(APPS_AVP_DIR)/.tmp-for-load-addr/main.c
	$(TOPDIR)/build/scripts/update_entry_addr.sh \
		--genaddrout $(APPS_AVP_DIR)/.tmp-for-load-addr/a.out \
		--entryld $(APPS_AVP_PKGD)/entry.ld
endef
else ifeq ($(BR2_PACKAGE_APPS_AVP),y)
$(info "CONFIG_CUSTOM_DTS_PATH is not defined, not able to update entry address of avp application")
endif

APPS_AVP_PRE_BUILD_HOOKS += APPS_AVP_ENTRY_ADDR_FROM_DTS

$(eval $(generic-package))
