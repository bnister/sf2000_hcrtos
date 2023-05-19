################################################################################
#
# audio driver
#
################################################################################

AUDDRIVER_VERSION = 07dd083c35cdf45cbc1ef9557c5d75227b9776c6
AUDDRIVER_SITE_METHOD = git
AUDDRIVER_SITE = ssh://git@hichiptech.gitlab.com:33888/hcllav/avdriver.git
AUDDRIVER_DEPENDENCIES = kernel

ifeq ($(BR2_PACKAGE_OPENCORE_AMR),y)
	AUDDRIVER_DEPENDENCIES += opencore-amr
endif

AUDDRIVER_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

AUDDRIVER_INSTALL_STAGING = YES

AUDDRIVER_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(AUDDRIVER_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(AUDDRIVER_VERSION)

AUDDRIVER_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(AUDDRIVER_MAKE_ENV) $(MAKE) $(AUDDRIVER_MAKE_FLAGS) -C $(@D) clean
AUDDRIVER_BUILD_CMDS = $(TARGET_MAKE_ENV) $(AUDDRIVER_MAKE_ENV) $(MAKE) $(AUDDRIVER_MAKE_FLAGS) -C $(@D) all
AUDDRIVER_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(AUDDRIVER_MAKE_ENV) $(MAKE) $(AUDDRIVER_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
