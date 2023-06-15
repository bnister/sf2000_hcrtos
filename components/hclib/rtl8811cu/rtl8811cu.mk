################################################################################
#
# rtl8811cu
#
################################################################################
RTL8811CU_VERSION = 805981f0c2f1f9216af0b2d9fa46bb9f3b19c933
RTL8811CU_SITE_METHOD = git
RTL8811CU_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/rtl8811cu.git
RTL8811CU_DEPENDENCIES = kernel

RTL8811CU_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

RTL8811CU_INSTALL_STAGING = YES

RTL8811CU_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(RTL8811CU_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(RTL8811CU_VERSION)


RTL8811CU_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(RTL8811CU_MAKE_ENV) $(MAKE) $(RTL8811CU_MAKE_FLAGS) -C $(@D) clean
RTL8811CU_BUILD_CMDS = $(TARGET_MAKE_ENV) $(RTL8811CU_MAKE_ENV) $(MAKE) $(RTL8811CU_MAKE_FLAGS) -C $(@D) all
RTL8811CU_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(RTL8811CU_MAKE_ENV) $(MAKE) $(RTL8811CU_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
