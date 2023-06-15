include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

include $(BR2_MKLINK)

cflags-y +=  -Wall -Wshadow -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare -Wno-error=format=


ifeq ($(BR2_PACKAGE_HCCAST_DLNA),y)
	cflags-y += -DDLNA_SUPPORT
endif

ifeq ($(BR2_PACKAGE_HCCAST_MIRACAST),y)
	cflags-y += -DMIRACAST_SUPPORT
endif

ifeq ($(BR2_PACKAGE_HCCAST_AIRCAST),y)
	cflags-y += -DAIRCAST_SUPPORT
endif

ifeq ($(BR2_PACKAGE_HCCAST_USBMIRROR),y)
	cflags-y += -DUSBMIRROR_SUPPORT
endif

ifeq ($(BR2_PACKAGE_HCCAST_NET),y)
	cflags-y += -DNETWORK_API
endif

ifeq ($(BR2_PACKAGE_HCCAST_NET),y)
	cflags-y += -DNETWORK_SUPPORT
endif

ifeq ($(BR2_PACKAGE_HCCAST_NET),y)
	cflags-y += -DWIFI_SUPPORT
endif

# Strip quotes and then whitespaces
qstrip=$(strip $(subst ",,$(1)))
#"))

target-y := $(call qstrip,$(CONFIG_APPS_NAME))

all : $(target-y).bin FORCE

main-y = main.o hccast_sample.o

obj-y += main.o

$(target-y).bin: $(target-y).out
	$(OBJCOPY) -O binary $< $@

$(target-y).out: built-in.o
	$(Q)$(LD) $(ldflags-y) -o $@ -Map $@.map --start-group $^ $(ldlibs-y) --whole-archive $(wholearchivelibs-y) --no-whole-archive --end-group

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -m 0664 $(target-y).out $(IMAGES_DIR)/
	install -m 0664 $(target-y).out.map $(IMAGES_DIR)/
	install -m 0664 $(target-y).bin $(IMAGES_DIR)/


clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	-rm -f $(target-y).out
	-rm -f $(target-y).out.map
	-rm -f $(target-y).bin

.PHONY: FORCE

FORCE:

endif
