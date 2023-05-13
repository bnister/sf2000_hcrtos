################################################################################
#
# lnxporting
#
################################################################################
LIBDSC_VERSION = 9bb37557a6018bbe5de38ccf78210221db9b11d6
LIBDSC_SITE_METHOD = git
LIBDSC_SITE = ssh://git@hichiptech.gitlab.com:33888/hcko/dsc-ko.git
LIBDSC_DEPENDENCIES = kernel

LIBDSC_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LIBDSC_INSTALL_STAGING = YES

LIBDSC_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBDSC_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBDSC_VERSION)


LIBDSC_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LIBDSC_MAKE_ENV) $(MAKE) $(LIBDSC_MAKE_FLAGS) -f $(@D)/Makefile.rtos -C $(@D) clean
LIBDSC_BUILD_CMDS = $(TARGET_MAKE_ENV) $(LIBDSC_MAKE_ENV) $(MAKE) $(LIBDSC_MAKE_FLAGS) -f $(@D)/Makefile.rtos -C $(@D) clean all
LIBDSC_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LIBDSC_MAKE_ENV) $(MAKE) $(LIBDSC_MAKE_FLAGS) -f $(@D)/Makefile.rtos -C $(@D) install

$(eval $(generic-package))
