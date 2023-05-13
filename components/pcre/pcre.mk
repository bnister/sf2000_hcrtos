PCRE_VERSION = 8.45
PCRE_SOURCE = pcre-$(PCRE_VERSION).tar.gz
PCRE_SITE = https://sourceforge.net/projects/pcre/files

PCRE_INSTALL_STAGING = YES
PCRE_CONF_OPTS = -DPCRE_BUILD_TESTS=OFF -DPCRE_BUILD_PCRECPP=OFF -DPCRE_BUILD_PCREGREP=OFF
PCRE_CONF_ENV = CFLAGS+=" -Wno-format-nonliteral"

PCRE_DEPENDENCIES += newlib

$(eval $(cmake-package))
