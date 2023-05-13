################################################################################
#
# rtl8733bu
#
################################################################################
RTL8733BU_VERSION = a8bdc504d3b0a95c5f66630fc338a367fbed3cc2
RTL8733BU_SITE_METHOD = git
RTL8733BU_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/rtl8733bu.git
RTL8733BU_DEPENDENCIES = kernel

RTL8733BU_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

RTL8733BU_INSTALL_STAGING = YES

RTL8733BU_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(RTL8733BU_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(RTL8733BU_VERSION)


RTL8733BU_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(RTL8733BU_MAKE_ENV) $(MAKE) $(RTL8733BU_MAKE_FLAGS) -C $(@D) clean
RTL8733BU_BUILD_CMDS = $(TARGET_MAKE_ENV) $(RTL8733BU_MAKE_ENV) $(MAKE) $(RTL8733BU_MAKE_FLAGS) -C $(@D) all
RTL8733BU_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(RTL8733BU_MAKE_ENV) $(MAKE) $(RTL8733BU_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
