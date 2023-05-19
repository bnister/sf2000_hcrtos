################################################################################
#
# mipi driver
#
################################################################################
MIPIDRIVER_VERSION = 5a08b64bf11d974528761caf1ac6b3cf54912a0e
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
