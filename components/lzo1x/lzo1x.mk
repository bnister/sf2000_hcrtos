################################################################################
#
# lzo
#
################################################################################

LZO1X_VERSION = 2.10
LZO1X_SOURCE = lzo-$(LZO1X_VERSION).tar.gz
LZO1X_SITE = http://www.oberhumer.com/opensource/lzo/download
LZO1X_LICENSE = GPL-2.0+
LZO1X_LICENSE_FILES = COPYING
LZO1X_CPE_ID_VENDOR = lzo_project
LZO1X_INSTALL_STAGING = YES
LZO1X_SUPPORTS_IN_SOURCE_BUILD = NO

ifeq ($(BR2_SHARED_LIBS)$(BR2_SHARED_STATIC_LIBS),y)
LZO1X_CONF_OPTS += -DENABLE_SHARED=ON
else
LZO1X_CONF_OPTS += -DENABLE_SHARED=OFF
endif

ifeq ($(BR2_STATIC_LIBS)$(BR2_SHARED_STATIC_LIBS),y)
LZO1X_CONF_OPTS += -DENABLE_STATIC=ON
else
LZO1X_CONF_OPTS += -DENABLE_STATIC=OFF
endif

HOST_LZO1X_CONF_OPTS += -DENABLE_SHARED=ON -DENABLE_STATIC=OFF

define HOST_LZO1X_INSTALL_PRECOMP2
	$(INSTALL) -m 0755 -D $(@D)/buildroot-build/precomp2 $(HOST_DIR)/bin/hcprecomp2
endef
HOST_LZO1X_POST_INSTALL_HOOKS += HOST_LZO1X_INSTALL_PRECOMP2

$(eval $(cmake-package))
$(eval $(host-cmake-package))
