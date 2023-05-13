################################################################################
#
# usbdriver
#
################################################################################
USBDRIVER_VERSION = 1eaf0805f84fe3a65ff6cb238eedf3477cbd2071 
USBDRIVER_SITE_METHOD = git
USBDRIVER_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libusb.git
USBDRIVER_DEPENDENCIES = kernel

USBDRIVER_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

USBDRIVER_INSTALL_STAGING = YES

USBDRIVER_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(USBDRIVER_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(USBDRIVER_VERSION)


USBDRIVER_CONFIGURE_CMDS = $(TARGET_MAKE_ENV) $(USBDRIVER_MAKE_ENV) $(MAKE) $(USBDRIVER_MAKE_FLAGS) -C $(@D) default_defconfig
USBDRIVER_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(USBDRIVER_MAKE_ENV) $(MAKE) $(USBDRIVER_MAKE_FLAGS) -C $(@D) clean
USBDRIVER_BUILD_CMDS = $(TARGET_MAKE_ENV) $(USBDRIVER_MAKE_ENV) $(MAKE) $(USBDRIVER_MAKE_FLAGS) -C $(@D) all
USBDRIVER_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(USBDRIVER_MAKE_ENV) $(MAKE) $(USBDRIVER_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
