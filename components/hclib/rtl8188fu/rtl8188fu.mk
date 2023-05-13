################################################################################
#
# rtl8188fu
#
################################################################################
RTL8188FU_VERSION = 48966f647b5110939db531364f53c62ea98e8310
RTL8188FU_SITE_METHOD = git
RTL8188FU_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/rtl8188fu.git
RTL8188FU_DEPENDENCIES = kernel

RTL8188FU_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

RTL8188FU_INSTALL_STAGING = YES

RTL8188FU_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(RTL8188FU_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(RTL8188FU_VERSION)


RTL8188FU_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(RTL8188FU_MAKE_ENV) $(MAKE) $(RTL8188FU_MAKE_FLAGS) -C $(@D) clean
RTL8188FU_BUILD_CMDS = $(TARGET_MAKE_ENV) $(RTL8188FU_MAKE_ENV) $(MAKE) $(RTL8188FU_MAKE_FLAGS) -C $(@D) all
RTL8188FU_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(RTL8188FU_MAKE_ENV) $(MAKE) $(RTL8188FU_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
