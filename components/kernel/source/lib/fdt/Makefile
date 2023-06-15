libfdt-y = fdt_api.o fdt.o fdt_ro.o fdt_rw.o

obj-y += libfdt.o

cflags-y += -Wno-sign-compare

ifeq ($(CONFIG_OF_EMBED),y)
$(obj)/fdt_api.o: $(DTS).dtb
endif
