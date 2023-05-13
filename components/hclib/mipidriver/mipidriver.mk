################################################################################
#
# mipi driver
#
################################################################################
MIPIDRIVER_VERSION = e2d9f2aaec8d53a2a3a4eefafcd6f9172250774c
MIPIDRIVER_SITE_METHOD = git
MIPIDRIVER_SITE = ssh://git@hichiptech.gitlab.com:33888/hcrtos_sdk/mipidriver.git
MIPIDRIVER_DEPENDENCIES = kernel

MIPIDRIVER_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

MIPIDRIVER_INSTALL_STAGING = YES

MIPIDRIVER_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(MIPIDRIVER_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(MIPIDRIVER_VERSION)


MIPIDRIVER_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(MIPIDRIVER_MAKE_ENV) $(MAKE) $(MIPIDRIVER_MAKE_FLAGS) -C $(@D) clean
MIPIDRIVER_BUILD_CMDS = $(TARGET_MAKE_ENV) $(MIPIDRIVER_MAKE_ENV) $(MAKE) $(MIPIDRIVER_MAKE_FLAGS) -C $(@D) all
MIPIDRIVER_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(MIPIDRIVER_MAKE_ENV) $(MAKE) $(MIPIDRIVER_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
