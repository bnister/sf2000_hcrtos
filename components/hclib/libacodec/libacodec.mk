################################################################################
#
# libacodec
#
################################################################################

LIBACODEC_VERSION = bc4c6c353fee90b9d5fafa13f51162146243c57c
LIBACODEC_SITE_METHOD = git
LIBACODEC_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libacodec.git
LIBACODEC_DEPENDENCIES = kernel

LIBACODEC_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LIBACODEC_INSTALL_STAGING = YES

LIBACODEC_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBACODEC_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBACODEC_VERSION)

LIBACODEC_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LIBACODEC_MAKE_ENV) $(MAKE) $(LIBACODEC_MAKE_FLAGS) -C $(@D) clean
LIBACODEC_BUILD_CMDS = $(TARGET_MAKE_ENV) $(LIBACODEC_MAKE_ENV) $(MAKE) $(LIBACODEC_MAKE_FLAGS) -C $(@D) all
LIBACODEC_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LIBACODEC_MAKE_ENV) $(MAKE) $(LIBACODEC_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
