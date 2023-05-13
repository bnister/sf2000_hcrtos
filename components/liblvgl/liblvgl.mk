LIBLVGL_MAKE_FLAGS += \
		TARGET_CROSS=$(TARGET_CROSS) \
		TARGET_CFLAGS="$(TARGET_CFLAGS)"
	
#CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LIBLVGL_INSTALL_STAGING = YES

LIBLVGL_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LIBLVGL_MAKE_ENV) $(MAKE) $(LIBLVGL_MAKE_FLAGS) -f $(@D)/Makefile.rtos -C $(@D) clean
LIBLVGL_BUILD_CMDS = rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/liblvgl/source/ $(@D) && \
		     if [ ! -f $(@D)/.config ];then \
		     cp $(BR2_PACKAGE_LIBLVGL_CONFIG) $(@D)/.config; \
		     cp $(BR2_PACKAGE_LIBLVGL_CONFIG) $(@D)/.config.old;\
		     $(@D)/scripts/merge_config.sh -m -O $(@D) $(@D)/.config;fi && \
		     if [ ! -f $(@D)/include/generated/autoconf.h -o $(@D)/.config -nt $(@D)/include/generated/autoconf.h ];then \
		     $(MAKE) -C $(@D) MENUCONFIG=y -f Makefile.rtos olddefconfig; \
		     $(MAKE) -C $(@D) MENUCONFIG=y -f Makefile.rtos silentoldconfig;fi && \
		     $(TARGET_MAKE_ENV) $(LIBLVGL_MAKE_ENV) $(MAKE) $(LIBLVGL_MAKE_FLAGS) -f $(@D)/Makefile.rtos -C $(@D) all

LIBLVGL_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LIBLVGL_MAKE_ENV) $(MAKE) $(LIBLVGL_MAKE_FLAGS) -f $(@D)/Makefile.rtos -C $(@D) install

LIBLVGL_SUPPORT_SEPARATE_OUTPUT = YES

LIBLVGL_DEPENDENCIES = kernel pthread
ifneq ($(BR2_PACKAGE_LIBGE),)
LIBLVGL_DEPENDENCIES += libge
endif

$(eval $(generic-package))

liblvgl-menuconfig:
	rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/liblvgl/source/ $(BUILD_DIR)/liblvgl
	if [ ! -f $(BUILD_DIR)/liblvgl/.config ];then \
		cp $(BR2_PACKAGE_LIBLVGL_CONFIG) $(BUILD_DIR)/liblvgl/.config; \
		cp $(BR2_PACKAGE_LIBLVGL_CONFIG) $(BUILD_DIR)/liblvgl/.config.old;fi
	$(MAKE) menuconfig -C $(BUILD_DIR)/liblvgl MENUCONFIG=y -f Makefile.rtos

