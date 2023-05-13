LIBUSBMIRROR_SUPPORT_SEPARATE_OUTPUT = YES

LIBUSBMIRROR_DEPENDENCIES = kernel newlib pthread libusb libopenssl
LIBUSBMIRROR_INSTALL_STAGING = YES
LIBUSBMIRROR_EXTRACT_CMDS = \
	rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/hclib/display/libusbmirror/source/ $(@D)

LIBUSBMIRROR_CONF_OPTS += -DHCRTOS=ON

define LIBUSBMIRROR_INSTALL_HEADERS
	$(INSTALL) -d $(STAGING_DIR)/usr/include/um
	$(INSTALL) $(@D)/inc/iumirror_api.h -m 0666 $(STAGING_DIR)/usr/include/um
	$(INSTALL) $(@D)/inc/aumirror_api.h -m 0666 $(STAGING_DIR)/usr/include/um
endef

LIBUSBMIRROR_POST_INSTALL_TARGET_HOOKS += LIBUSBMIRROR_INSTALL_HEADERS

$(eval $(cmake-package))
