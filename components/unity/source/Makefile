include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

TARGETS := libunity.a

obj-y += src/
obj-y += extras/fixture/src/
obj-y += port/src/

cflags-y += -I$(srctree)src/ \
	-I$(srctree)extras/fixture/src/ \
	-I$(srctree)port/include/unity/ \
	-DUNITY_OUTPUT_COLOR

all : $(TARGETS) FORCE

$(TARGETS): built-in.o
	rm -f $@
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -d -m 0775 $(STAGING_DIR)/usr/include
	install -d -m 0775 $(STAGING_DIR)/usr/include/unity
	install -m 0664 $(srctree)src/*.h $(STAGING_DIR)/usr/include/unity/
	install -m 0664 $(srctree)extras/fixture/src/*.h $(STAGING_DIR)/usr/include/unity/
	install -m 0664 $(srctree)port/include/unity/*.h $(STAGING_DIR)/usr/include/unity/
	install -m 0664 libunity.a $(STAGING_DIR)/usr/lib/libunity.a

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	rm -f $(TARGETS)

.PHONY: FORCE

FORCE:

endif	# skip-makefile
