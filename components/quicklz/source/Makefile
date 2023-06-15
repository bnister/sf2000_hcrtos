include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

TARGETS := libquicklz.a

all : $(TARGETS) quicklz FORCE

quicklz-y := quicklz.o

obj-y = quicklz.o

quicklz: $(srctree)/quicklz.c $(srctree)/quicklz_sample.c
	gcc -o $@  $^

$(TARGETS): built-in.o
	rm -f $@
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -D -m 0664 $(TARGETS) $(STAGING_DIR)/usr/lib/$(TARGETS)
	cp -fr $(srctree)/quicklz.h $(STAGING_DIR)/usr/include/
	cp -fr $(srctree)/quicklz $(HOST_DIR)/bin/

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	rm -f $(TARGETS)

.PHONY: FORCE

FORCE:

endif	# skip-makefile
