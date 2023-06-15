boot_compressed-y := head.o decompress.o piggy.o string.o

obj-y += boot_compressed.o
ifeq ($(CONFIG_SPINANDWR_SELFCOMPRESSED_LZMA),y)
obj-y += lzma/
else ifeq ($(CONFIG_SPINANDWR_SELFCOMPRESSED_LZO1X),y)
obj-y += lzo1x/
endif

$(obj)/piggy.o: $(obj)/dummy.o $(target-y).bin.z
	$(Q)$(OBJCOPY) --remove-section=.reginfo --add-section=.image=$(target-y).bin.z --set-section-flags=.image=contents,alloc,load,readonly,data $< $@

BOOT_HEAP_SIZE := 0x400000

cflags-y += -DBOOT_HEAP_SIZE=$(BOOT_HEAP_SIZE) -D"VMLINUX_LOAD_ADDRESS_ULL=$(VMLINUX_LOAD_ADDRESS)ull" -DKERNEL_ENTRY=$(VMLINUX_ENTRY_ADDRESS)

VMLINUX_ENTRY_ADDRESS := $(shell readelf -h $(target-y).out | grep Entry | awk '{print $$NF}')
VMLINUX_LOAD_ADDRESS := $(shell nm -n $(target-y).out | awk '/T _start/ {print "0x"$$1}')
VMLINUZ_LOAD_ADDRESS := $(shell ./calc_vmlinuz_load_addr $(target-y).bin $(VMLINUX_LOAD_ADDRESS))

__build: $(target-y).z.out

$(target-y).z.out: $(obj)/built-in.o
	$(Q)$(LD) -EL -Ttext $(VMLINUZ_LOAD_ADDRESS) -T $(srctree)/boot-compressed/ld.script -o $@ -Map $@.map $^
