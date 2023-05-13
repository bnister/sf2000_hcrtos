LIBDLNA_VERSION = c0e273c2aeac46ef0d339b6be7ca4b7f1a5e28c7
LIBDLNA_SITE_METHOD = git
LIBDLNA_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libdlna.git
LIBDLNA_DEPENDENCIES = kernel newlib pthread

LIBDLNA_CONF_OPTS += -DBUILD_LIBRARY_TYPE="STATIC" -DBUILD_OS_TARGET="HCRTOS"

LIBDLNA_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBDLNA_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBDLNA_VERSION) && \
	cd $(@D) && sh ./gen-version.sh

define LIBDLNA_INSTALL_PREBUILT
	$(INSTALL) -D -m 0664 $(@D)/libdlna.a $(PREBUILT_DIR)/usr/lib/libdlna.a
endef

LIBDLNA_POST_INSTALL_TARGET_HOOKS += LIBDLNA_INSTALL_PREBUILT

LIBDLNA_INSTALL_STAGING = YES

$(eval $(cmake-package))
