OPENCORE_AMR_VERSION = 0.1.6
OPENCORE_AMR_SOURCE = opencore-amr-$(OPENCORE_AMR_VERSION).tar.gz
OPENCORE_AMR_SITE = http://downloads.sourceforge.net/project/opencore-amr/opencore-amr

OPENCORE_AMR_INSTALL_STAGING = YES
OPENCORE_AMR_CONF_OPT = --enable-threading --enable-static --disable-shared 
#OPENCORE_AMR_CONF_ENV = CFLAGS+=" -Wno-format-nonliteral"

OPENCORE_AMR_DEPENDENCIES += newlib

$(eval $(autotools-package))
