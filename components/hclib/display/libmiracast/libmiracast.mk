LIBMIRACAST_VERSION = fe2e57c316eaad36a82ebca2b79cdf3edbd93ed8
LIBMIRACAST_SITE_METHOD = git
LIBMIRACAST_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libmiracast.git
LIBMIRACAST_DEPENDENCIES = kernel newlib pthread libopenssl

LIBMIRACAST_CONF_OPTS += -DBUILD_LIBRARY_TYPE="STATIC" -DBUILD_OS_TARGET="HCRTOS"

LIBMIRACAST_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBMIRACAST_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master

define LIBMIRACAST_INSTALL_PREBUILT
	$(INSTALL) -D -m 0664 $(@D)/libmiracast.a $(PREBUILT_DIR)/usr/lib/libmiracast.a
endef

LIBMIRACAST_POST_INSTALL_TARGET_HOOKS += LIBMIRACAST_INSTALL_PREBUILT

LIBMIRACAST_INSTALL_STAGING = YES

$(eval $(cmake-package))
