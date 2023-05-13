################################################################################
#
# libefuse
#
################################################################################
LIBEFUSE_VERSION = e476c29cdb0dab3068c3be5caa77c4e6a0ed057f
LIBEFUSE_SITE_METHOD = git
LIBEFUSE_SITE = ssh://git@hichiptech.gitlab.com:33888/hcrtos_sdk/libefuse.git
LIBEFUSE_DEPENDENCIES = kernel

LIBEFUSE_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LIBEFUSE_INSTALL_STAGING = YES

LIBEFUSE_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBEFUSE_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBEFUSE_VERSION)

LIBEFUSE_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LIBEFUSE_MAKE_ENV) $(MAKE) $(LIBEFUSE_MAKE_FLAGS) -C $(@D) clean
LIBEFUSE_BUILD_CMDS = $(TARGET_MAKE_ENV) $(LIBEFUSE_MAKE_ENV) $(MAKE) $(LIBEFUSE_MAKE_FLAGS) -C $(@D) all
LIBEFUSE_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LIBEFUSE_MAKE_ENV) $(MAKE) $(LIBEFUSE_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
