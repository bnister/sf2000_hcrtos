ifeq ($(CONFIG_NEWLIB_USE_PREBUILT),y)
NEWLIB_SUPPORT_SEPARATE_OUTPUT = YES
$(eval $(generic-package))
else
include components/newlib/newlib/newlib.mk
endif
