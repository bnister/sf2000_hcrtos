################################################################################
#
# libws2811
#
################################################################################
LIBWS2811_VERSION = f66d0e981b69e699940233f2e5c03e6064fdc499
LIBWS2811_SITE_METHOD = git
LIBWS2811_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libws2811.git
LIBWS2811_DEPENDENCIES = kernel

LIBWS2811_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LIBWS2811_INSTALL_STAGING = YES

LIBWS2811_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBWS2811_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBWS2811_VERSION)


LIBWS2811_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LIBWS2811_MAKE_ENV) $(MAKE) $(LIBWS2811_MAKE_FLAGS) -C $(@D) clean
LIBWS2811_BUILD_CMDS = $(TARGET_MAKE_ENV) $(LIBWS2811_MAKE_ENV) $(MAKE) $(LIBWS2811_MAKE_FLAGS) -C $(@D) all
LIBWS2811_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LIBWS2811_MAKE_ENV) $(MAKE) $(LIBWS2811_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
