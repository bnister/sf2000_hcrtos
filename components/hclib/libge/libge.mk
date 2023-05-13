LIBGE_VERSION = 102c969bd8038dc3d3af6454b255f3445c852187
LIBGE_SITE_METHOD = git
LIBGE_SITE =  ssh://git@hichiptech.gitlab.com:33888/hclib/libge.git
LIBGE_LICENSE = LGPL-2.0+
LIBGE_LICENSE_FILES = COPYING
LIBGE_CONF_OPTS = 
BUILD_CFLAGS = -g -O0
LIBGE_DEPENDENCIES +=  kernel

LIBGE_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBGE_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBGE_VERSION)


LIBGE_INSTALL_STAGING = YES

LIBGE_INSTALL_STAGING_OPTS = DESTDIR=$(STAGING_DIR) install


LIBGE_CONF_OPTS += -D__HCRTOS__=ON#-DBUILD_STATIC_LIBS=OFF #-DWITH_SSL=ON

define LIBGE_INSTALL_PREBUILTS
	mkdir -p $(PREBUILT_DIR)/usr/lib
	mkdir -p $(PREBUILT_DIR)/usr/include/hcge
	cp -arf $(@D)/include/hcge/ge_api.h $(PREBUILT_DIR)/usr/include/hcge/ge_api.h
	cp -arf $(@D)/libge.a $(PREBUILT_DIR)/usr/lib/libge.a
endef

LIBGE_POST_INSTALL_STAGING_HOOKS += LIBGE_INSTALL_PREBUILTS

$(eval $(cmake-package))
