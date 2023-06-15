LIBVORBIS_VERSION = 1.3.7
LIBVORBIS_SOURCE = libvorbis-$(LIBVORBIS_VERSION).tar.xz
LIBVORBIS_SITE = https://downloads.xiph.org/releases/vorbis

LIBVORBIS_INSTALL_STAGING = YES

LIBVORBIS_CONF_OPT = --enable-static=yes --enable-shared=no --enable-examples=no --disable-oggtest --disable-docs
LIBVORBIS_DEPENDENCIES += newlib libogg

$(eval $(autotools-package))
