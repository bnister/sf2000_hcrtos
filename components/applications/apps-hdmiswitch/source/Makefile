include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

include $(BR2_MKLINK)

cflags-y += -I$(srctree)include/

# Strip quotes and then whitespaces
qstrip=$(strip $(subst ",,$(1)))
#"))

target-y := $(call qstrip,$(CONFIG_APPS_NAME))

all : $(target-y).bin FORCE

main-y = main.o hdmi_switch.o

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
