include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

include $(BR2_MKLINK)

cflags-y += -I$(srctree)include/

# Strip quotes and then whitespaces
qstrip=$(strip $(subst ",,$(1)))
#"))

target-y := $(call qstrip,$(CONFIG_APPS_NAME))
export target-y

all : $(target-y).bin FORCE

ifeq ($(CONFIG_BOOT_SELFCOMPRESSED),y)
all : $(target-y).z.bin
endif

main-y = main.o

obj-y += main.o
obj-y += cmd/
obj-y += common/
obj-y += lib/

$(target-y).bin: $(target-y).out
	$(OBJCOPY) -O binary $< $@

$(target-y).out: built-in.o
	$(Q)$(LD) $(ldflags-y) -o $@ -Map $@.map --start-group $^ $(ldlibs-y) --whole-archive $(wholearchivelibs-y) --no-whole-archive --end-group

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

$(target-y).z.bin: $(target-y).z.out FORCE
	$(OBJCOPY) -O binary $< $@

$(target-y).z.out: $(target-y).bin.z calc_vmlinuz_load_addr FORCE
	$(Q)$(MAKE) $(build)=boot-compressed

$(target-y).bin.z: $(target-y).bin FORCE
ifeq ($(CONFIG_BOOT_SELFCOMPRESSED_LZMA),y)
	lzma -zkf -c $< > $@
else ifeq ($(CONFIG_BOOT_SELFCOMPRESSED_LZO1X),y)
	$(HOST_DIR)/bin/hcprecomp2 $< $@
endif

calc_vmlinuz_load_addr: $(srctree)/boot-compressed/calc_vmlinuz_load_addr.c
	gcc -o $@ $^

install: FORCE
ifeq ($(CONFIG_BOOT_SELFCOMPRESSED),y)
	install -m 0664 $(target-y).z.out $(IMAGES_DIR)/$(target-y).out
	install -m 0664 $(target-y).z.out.map $(IMAGES_DIR)/$(target-y).out.map
	install -m 0664 $(target-y).z.bin $(IMAGES_DIR)/$(target-y).bin
	install -m 0664 $(target-y).out $(IMAGES_DIR)/$(target-y).uncompress.out
	install -m 0664 $(target-y).out.map $(IMAGES_DIR)/$(target-y).uncompress.out.map
	install -m 0664 $(target-y).bin $(IMAGES_DIR)/$(target-y).uncompress.bin
else
	install -m 0664 $(target-y).out $(IMAGES_DIR)/
	install -m 0664 $(target-y).out.map $(IMAGES_DIR)/
	install -m 0664 $(target-y).bin $(IMAGES_DIR)/
endif

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	$(Q)$(MAKE) $(clean)=boot-compressed
	-rm -f $(target-y).out
	-rm -f $(target-y).out.map
	-rm -f $(target-y).bin
	-rm -f $(target-y).z.out
	-rm -f $(target-y).z.out.map
	-rm -f $(target-y).z.bin
	-rm -f calc_vmlinuz_load_addr

.PHONY: FORCE

FORCE:

endif
