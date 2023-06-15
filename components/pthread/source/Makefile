include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

cflags-y += -I$(srctree)include
cflags-y += -I$(srctree)include/private
cflags-y += -I$(srctree)FreeRTOS-Plus-POSIX/include
cflags-y += -I$(srctree)FreeRTOS-Plus-POSIX/include/portable
cflags-y += -I$(srctree)FreeRTOS-Plus-POSIX/include/portable/hichip
cflags-y += -I$(srctree)FreeRTOS-Plus-POSIX/include/portable/hichip
cflags-y += -I$(STAGING_DIR)/usr/include_fix/freertos
cflags-y += -I$(STAGING_DIR)/usr/include/freertos

TARGETS = libpthread.a

all : $(TARGETS) FORCE

src ?= $(CURDIR)

obj-y += FreeRTOS-Plus-POSIX/source/

$(TARGETS): built-in.o
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -D -m 0664 $(TARGETS) $(STAGING_DIR)/usr/lib/$(TARGETS)
	cp -rf $(srctree)include/* $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)FreeRTOS-Plus-POSIX/include/* $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)include/private/iot_doubly_linked_list.h $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)FreeRTOS-Plus-POSIX/include/portable/hichip/FreeRTOS_POSIX_portable.h $(STAGING_DIR)/usr/include_fix/
	cp -rf $(srctree)FreeRTOS-Plus-POSIX/include/portable/FreeRTOS_POSIX_portable_default.h $(STAGING_DIR)/usr/include_fix/

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	-rm -f $(TARGETS)

.PHONY: FORCE

FORCE:

endif
