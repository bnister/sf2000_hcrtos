include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

TARGETS := libac6955f.a

all : $(TARGETS) FORCE

ac6955f-y := ac6955f.o

obj-y = ac6955f.o

$(TARGETS): built-in.o
	rm -f $@
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -D -m 0664 $(TARGETS) $(STAGING_DIR)/usr/lib/$(TARGETS)
	cp -fr $(srctree)/bluetooth.h $(STAGING_DIR)/usr/include/

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	rm -f $(TARGETS)

.PHONY: FORCE

FORCE:

endif	# skip-makefile
