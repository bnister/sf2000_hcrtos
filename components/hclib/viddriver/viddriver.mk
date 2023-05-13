################################################################################
#
# video driver
#
################################################################################
VIDDRIVER_VERSION = 751b6f3412ca4ab9d199ab18000fd8cceb5d5b71
VIDDRIVER_SITE_METHOD = git
VIDDRIVER_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libvid.git
VIDDRIVER_DEPENDENCIES = kernel

VIDDRIVER_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

VIDDRIVER_INSTALL_STAGING = YES

VIDDRIVER_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(VIDDRIVER_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(VIDDRIVER_VERSION)


VIDDRIVER_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(VIDDRIVER_MAKE_ENV) $(MAKE) $(VIDDRIVER_MAKE_FLAGS) -C $(@D) clean
VIDDRIVER_BUILD_CMDS = $(TARGET_MAKE_ENV) $(VIDDRIVER_MAKE_ENV) $(MAKE) $(VIDDRIVER_MAKE_FLAGS) -C $(@D) all
VIDDRIVER_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(VIDDRIVER_MAKE_ENV) $(MAKE) $(VIDDRIVER_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
