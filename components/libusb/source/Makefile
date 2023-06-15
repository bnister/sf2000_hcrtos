include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

cflags-y += -I$(srctree)
cflags-y += -I$(srctree)libusb/

target-y += libusb.a

obj-$(BR2_PACKAGE_LIBUSB) += libusb/
# obj-$(BR2_PACKAGE_LIBUSB_EXAMPLES) += examples/

all : $(target-y)

libusb.a: built-in.o
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -d -m 0775 $(STAGING_DIR)/usr/include	
	cp -fr $(srctree)/libusb/libusb.h $(STAGING_DIR)/usr/include/
	install -d -m 0775 $(STAGING_DIR)/usr/lib
	install -m 0664 $(target-y) $(STAGING_DIR)/usr/lib/

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	-rm -f libusb.a

.PHONY: FORCE

FORCE:

endif	# skip-makefile
