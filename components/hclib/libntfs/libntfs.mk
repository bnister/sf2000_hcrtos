################################################################################
#
# libntfs
#
################################################################################
LIBNTFS_VERSION = 57ddb26f8a2deeb53934319cdaf3f34fc13ff9fb
LIBNTFS_SITE_METHOD = git
LIBNTFS_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libntfs.git
LIBNTFS_DEPENDENCIES = kernel pthread

LIBNTFS_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LIBNTFS_INSTALL_STAGING = YES

LIBNTFS_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBNTFS_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBNTFS_VERSION)


LIBNTFS_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LIBNTFS_MAKE_ENV) $(MAKE) $(LIBNTFS_MAKE_FLAGS) -C $(@D) clean
LIBNTFS_BUILD_CMDS = $(TARGET_MAKE_ENV) $(LIBNTFS_MAKE_ENV) $(MAKE) $(LIBNTFS_MAKE_FLAGS) -C $(@D) all
LIBNTFS_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LIBNTFS_MAKE_ENV) $(MAKE) $(LIBNTFS_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
