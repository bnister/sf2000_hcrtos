ZLIB_VERSION = 1.2.11
ZLIB_SOURCE = zlib-$(ZLIB_VERSION).tar.xz
ZLIB_SITE = http://zlib.net

define ZLIB_CONFIGURE_CMDS
     (cd $(@D); rm -rf config.cache; \
     $(TARGET_CONFIGURE_ARGS) \
     $(TARGET_CONFIGURE_OPTS) \
     CFLAGS="$(TARGET_CFLAGS) $(ZLIB_PIC)" \
	 LDFLAGS="$(TARGET_LDFLAGS)"\
     ./configure \
     $(ZLIB_SHARED) \
     --prefix=/usr \
	 )
endef

define ZLIB_BUILD_CMDS
	$(MAKE) -C $(@D)
endef

define ZLIB_INSTALL_STAGING_CMDS
    $(MAKE1) -C $(@D) DESTDIR=$(STAGING_DIR) LDCONFIG=true install
endef

$(eval $(generic-package))
