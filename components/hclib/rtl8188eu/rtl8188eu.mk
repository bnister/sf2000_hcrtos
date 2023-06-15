################################################################################
#
# rtl8188eu
#
################################################################################
RTL8188EU_VERSION = 60893df79b79b014ce06bcf9b0c7db01729ac9e3
RTL8188EU_SITE_METHOD = git
RTL8188EU_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/rtl8188eu.git
RTL8188EU_DEPENDENCIES = kernel

RTL8188EU_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

RTL8188EU_INSTALL_STAGING = YES

RTL8188EU_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(RTL8188EU_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(RTL8188EU_VERSION)


RTL8188EU_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(RTL8188EU_MAKE_ENV) $(MAKE) $(RTL8188EU_MAKE_FLAGS) -C $(@D) clean
RTL8188EU_BUILD_CMDS = $(TARGET_MAKE_ENV) $(RTL8188EU_MAKE_ENV) $(MAKE) $(RTL8188EU_MAKE_FLAGS) -C $(@D) all
RTL8188EU_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(RTL8188EU_MAKE_ENV) $(MAKE) $(RTL8188EU_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
