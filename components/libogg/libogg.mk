LIBOGG_VERSION = 1.3.4
LIBOGG_SOURCE = libogg-$(LIBOGG_VERSION).tar.xz
LIBOGG_SITE = http://downloads.xiph.org/releases/ogg
LIBOGG_INSTALL_STAGING = YES

LIBOGG_CONF_OPT = --enable-threading --enable-static --disable-shared
LIBOGG_DEPENDENCIES += newlib

define LIBOGG_PRE_AUTOCONFIG
	cd $(@D) && aclocal -I m4
	cd $(@D) && autoheader
	cd $(@D) && automake --add-missing -a -c
	cd $(@D) && autoconf
endef

LIBOGG_PRE_CONFIGURE_HOOKS += LIBOGG_PRE_AUTOCONFIG

$(eval $(autotools-package))
