################################################################################
#
# wireless_tools
#
################################################################################

WIRELESS_TOOLS_VERSION = 4bc0713f45b8d5cbe5bbc43d56b5fe51c0c7048d
WIRELESS_TOOLS_SITE_METHOD = git
WIRELESS_TOOLS_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/wireless_tools.git
WIRELESS_TOOLS_LICENSE = LGPL-2.0+
WIRELESS_TOOLS_LICENSE_FILES = COPYING
WIRELESS_TOOLS_CONF_OPTS = 

WIRELESS_TOOLS_DEPENDENCIES += kernel


WIRELESS_TOOLS_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(WIRELESS_TOOLS_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(WIRELESS_TOOLS_VERSION)


WIRELESS_TOOLS_INSTALL_STAGING = YES

WIRELESS_TOOLS_BUILD_TARGETS += libiw.a
WIRELESS_TOOLS_INSTALL_TARGETS += install-static

define WIRELESS_TOOLS_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) PREFIX="$(STAGING_DIR)" LDCONFIG=/bin/true \
		PREFIX=$(PREBUILT_DIR)	install-static
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)   PREFIX=$(PREBUILT_DIR) install-hdr
endef

define WIRELESS_TOOLS_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) CC="$(TARGET_CC)" CFLAGS="$(TARGET_CFLAGS) -D__HCRTOS__" \
		$(WIRELESS_TOOLS_BUILD_TARGETS) V=1
endef

define WIRELESS_TOOLS_INSTALL_TARGET_CMDS
	#$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) PREFIX="$(TARGET_DIR)" LDCONFIG=/bin/true \
		$(WIRELESS_TOOLS_INSTALL_TARGETS)
endef

$(eval $(generic-package))
