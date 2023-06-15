include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

TARGETS := liblzo.a

all : $(TARGETS) FORCE

liblzo-y := lzo_main.o lzo1x_d1.o

obj-y = liblzo.o

$(TARGETS): built-in.o
	rm -f $@
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -D -m 0664 $(TARGETS) $(STAGING_DIR)/usr/lib/$(TARGETS)
	cp -fr $(srctree)/lzo1x.h $(STAGING_DIR)/usr/include/
	cp -fr $(srctree)/lzoconf.h $(STAGING_DIR)/usr/include/
	cp -fr $(srctree)/lzodefs.h $(STAGING_DIR)/usr/include/

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	rm -f $(TARGETS)

.PHONY: FORCE

FORCE:

endif	# skip-makefile
