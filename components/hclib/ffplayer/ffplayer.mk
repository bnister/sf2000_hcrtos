################################################################################
#
# ffplayer
#
################################################################################

FFPLAYER_VERSION = 71007dc4b5a73371aeeea54fa9afbe25ec2bcf52
FFPLAYER_SITE_METHOD = git
FFPLAYER_SITE = ssh://git@hichiptech.gitlab.com:33888/hcmedia/hcffplayer.git
FFPLAYER_DEPENDENCIES = ffmpeg pthread

TARGET_CFLAGS += -DFFPLAYER_ENABLE_BMPDEC

FFPLAYER_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS) -Wno-sign-compare -Wno-format-security -Wno-format-nonliteral" \
		LDFLAGS="${TARGET_LDFLAGS}"

FFPLAYER_INSTALL_STAGING = YES

FFPLAYER_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(FFPLAYER_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(FFPLAYER_VERSION)


#FFPLAYER_CONFIGURE_CMDS = $(TARGET_MAKE_ENV) $(FFPLAYER_MAKE_ENV) $(MAKE) $(FFPLAYER_MAKE_FLAGS) -C $(@D) default_defconfig
FFPLAYER_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(FFPLAYER_MAKE_ENV) $(MAKE) $(FFPLAYER_MAKE_FLAGS) -C $(@D) clean
FFPLAYER_BUILD_CMDS = $(TARGET_MAKE_ENV) $(FFPLAYER_MAKE_ENV) $(MAKE) $(FFPLAYER_MAKE_FLAGS) -C $(@D) all
FFPLAYER_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(FFPLAYER_MAKE_ENV) $(MAKE) $(FFPLAYER_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
