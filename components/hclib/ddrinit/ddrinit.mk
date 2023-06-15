################################################################################
#
# ddrinit
#
################################################################################
DDRINIT_VERSION = a8fe717a9aa9f32d0d1c8975e74bb1716bf7ec44
DDRINIT_SITE_METHOD = git
DDRINIT_SITE = ssh://git@hichiptech.gitlab.com:33888/hcrtos_sdk/ddrinit.git

DDRINIT_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

DDRINIT_INSTALL_STAGING = YES

DDRINIT_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(DDRINIT_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(DDRINIT_VERSION)


DDRINIT_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(DDRINIT_MAKE_ENV) $(MAKE) $(DDRINIT_MAKE_FLAGS) -C $(@D) clean
DDRINIT_BUILD_CMDS = $(TARGET_MAKE_ENV) $(DDRINIT_MAKE_ENV) $(MAKE) $(DDRINIT_MAKE_FLAGS) -C $(@D) all
DDRINIT_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(DDRINIT_MAKE_ENV) $(MAKE) $(DDRINIT_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
