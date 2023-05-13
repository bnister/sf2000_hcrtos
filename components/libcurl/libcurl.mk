################################################################################
#
# libcurl
#
################################################################################

LIBCURL_VERSION = 7.77.0
LIBCURL_SOURCE = curl-$(LIBCURL_VERSION).tar.xz
LIBCURL_SITE = https://curl.haxx.se/download
LIBCURL_DEPENDENCIES = \
	$(if $(BR2_PACKAGE_ZLIB),zlib) \
	$(if $(BR2_PACKAGE_RTMPDUMP),rtmpdump)
LIBCURL_LICENSE = curl
LIBCURL_LICENSE_FILES = COPYING
LIBCURL_CPE_ID_VENDOR = haxx
LIBCURL_CPE_ID_PRODUCT = libcurl
LIBCURL_INSTALL_STAGING = YES

# We disable NTLM support because it uses fork(), which doesn't work
# on non-MMU platforms. Moreover, this authentication method is
# probably almost never used. See
# http://curl.haxx.se/docs/manpage.html#--ntlm.
# Likewise, there is no compiler on the target, so libcurl-option (to
# generate C code) isn't very useful
LIBCURL_CONF_OPT = --disable-manual --disable-ntlm-wb \
	--enable-hidden-symbols --with-random=/dev/urandom --disable-curldebug \
	--disable-libcurl-option --disable-ldap --disable-ldaps

LIBCURL_CONF_OPT += \
	--without-libpsl \
	--without-libgsasl

ifeq ($(BR2_TOOLCHAIN_HAS_THREADS),y)
LIBCURL_CONF_OPT += --enable-threaded-resolver
else
LIBCURL_CONF_OPT += --disable-threaded-resolver
endif

ifeq ($(BR2_PACKAGE_LIBCURL_VERBOSE),y)
LIBCURL_CONF_OPT += --enable-verbose
else
LIBCURL_CONF_OPT += --disable-verbose
endif

LIBCURL_CONFIG_SCRIPTS = curl-config

ifeq ($(BR2_PACKAGE_LIBCURL_OPENSSL),y)
LIBCURL_DEPENDENCIES += libopenssl
# configure adds the cross openssl dir to LD_LIBRARY_PATH which screws up
# native stuff during the rest of configure when target == host.
# Fix it by setting LD_LIBRARY_PATH to something sensible so those libs
# are found first.
#LIBCURL_CONF_ENV += LD_LIBRARY_PATH=$(if $(LD_LIBRARY_PATH),$(LD_LIBRARY_PATH):)/lib:/usr/lib
LIBCURL_CONF_OPT += --with-ssl=$(STAGING_DIR)/usr \
	--with-ca-path=/etc/ssl/certs
else
LIBCURL_CONF_OPT += --without-ssl
endif

ifeq ($(BR2_PACKAGE_LIBCURL_BEARSSL),y)
LIBCURL_CONF_OPT += --with-bearssl=$(STAGING_DIR)/usr
LIBCURL_DEPENDENCIES += bearssl
else
LIBCURL_CONF_OPT += --without-bearssl
endif

ifeq ($(BR2_PACKAGE_LIBCURL_GNUTLS),y)
LIBCURL_CONF_OPT += --with-gnutls=$(STAGING_DIR)/usr \
	--with-ca-fallback
LIBCURL_DEPENDENCIES += gnutls
else
LIBCURL_CONF_OPT += --without-gnutls
endif

ifeq ($(BR2_PACKAGE_LIBCURL_LIBNSS),y)
LIBCURL_CONF_OPT += --with-nss=$(STAGING_DIR)/usr
LIBCURL_CONF_ENV += CPPFLAGS="$(TARGET_CPPFLAGS) `$(PKG_CONFIG_HOST_BINARY) nspr nss --cflags`"
LIBCURL_DEPENDENCIES += libnss
else
LIBCURL_CONF_OPT += --without-nss
endif

ifeq ($(BR2_PACKAGE_LIBCURL_MBEDTLS),y)
LIBCURL_CONF_OPT += --with-mbedtls=$(STAGING_DIR)/usr
LIBCURL_DEPENDENCIES += mbedtls
else
LIBCURL_CONF_OPT += --without-mbedtls
endif

ifeq ($(BR2_PACKAGE_LIBCURL_WOLFSSL),y)
LIBCURL_CONF_OPT += --with-wolfssl=$(STAGING_DIR)/usr
LIBCURL_DEPENDENCIES += wolfssl
else
LIBCURL_CONF_OPT += --without-wolfssl
endif

ifeq ($(BR2_PACKAGE_C_ARES),y)
LIBCURL_DEPENDENCIES += c-ares
LIBCURL_CONF_OPT += --enable-ares
else
LIBCURL_CONF_OPT += --disable-ares
endif

ifeq ($(BR2_PACKAGE_LIBIDN2),y)
LIBCURL_DEPENDENCIES += libidn2
LIBCURL_CONF_OPT += --with-libidn2
else
LIBCURL_CONF_OPT += --without-libidn2
endif

# Configure curl to support libssh2
ifeq ($(BR2_PACKAGE_LIBSSH2),y)
LIBCURL_DEPENDENCIES += libssh2
LIBCURL_CONF_OPT += --with-libssh2
else
LIBCURL_CONF_OPT += --without-libssh2
endif

ifeq ($(BR2_PACKAGE_BROTLI),y)
LIBCURL_DEPENDENCIES += brotli
LIBCURL_CONF_OPT += --with-brotli
else
LIBCURL_CONF_OPT += --without-brotli
endif

ifeq ($(BR2_PACKAGE_NGHTTP2),y)
LIBCURL_DEPENDENCIES += nghttp2
LIBCURL_CONF_OPT += --with-nghttp2
else
LIBCURL_CONF_OPT += --without-nghttp2
endif

ifeq ($(BR2_PACKAGE_LIBGSASL),y)
LIBCURL_DEPENDENCIES += libgsasl
LIBCURL_CONF_OPT += --with-gsasl
else
LIBCURL_CONF_OPT += --without-gsasl
endif

ifeq ($(BR2_PACKAGE_LIBCURL_COOKIES_SUPPORT),y)
LIBCURL_CONF_OPT += --enable-cookies
else
LIBCURL_CONF_OPT += --disable-cookies
endif

ifeq ($(BR2_PACKAGE_LIBCURL_PROXY_SUPPORT),y)
LIBCURL_CONF_OPT += --enable-proxy
else
LIBCURL_CONF_OPT += --disable-proxy
endif

ifeq ($(BR2_PACKAGE_LIBCURL_EXTRA_PROTOCOLS_FEATURES),y)
LIBCURL_CONF_OPT += \
	--enable-dict \
	--enable-gopher \
	--enable-imap \
	--enable-pop3 \
	--enable-rtsp \
	--enable-smb \
	--enable-smtp \
	--enable-telnet \
	--enable-tftp
else
LIBCURL_CONF_OPT += \
	--disable-dict \
	--disable-gopher \
	--disable-imap \
	--disable-pop3 \
	--disable-rtsp \
	--disable-smb \
	--disable-smtp \
	--disable-telnet \
	--disable-tftp
endif

define LIBCURL_FIX_DOT_PC
	printf 'Requires: openssl\n' >>$(@D)/libcurl.pc.in
endef
LIBCURL_POST_PATCH_HOOKS += $(if $(BR2_PACKAGE_LIBCURL_OPENSSL),LIBCURL_FIX_DOT_PC)

# ifeq ($(BR2_PACKAGE_LIBCURL_CURL),)
# define LIBCURL_TARGET_CLEANUP
# 	rm -rf $(TARGET_DIR)/usr/bin/curl
# endef
# LIBCURL_POST_INSTALL_TARGET_HOOKS += LIBCURL_TARGET_CLEANUP
# endif

HOST_LIBCURL_DEPENDENCIES = host-openssl
HOST_LIBCURL_CONF_OPT = \
	--disable-manual \
	--disable-ntlm-wb \
	--disable-curldebug \
	--with-ssl \
	--without-gnutls \
	--without-mbedtls \
	--without-nss

HOST_LIBCURL_POST_PATCH_HOOKS += LIBCURL_FIX_DOT_PC

define LIBCURL_FIX_CURL_CONFIG
	$(SED) "s~#define HAVE_SIGNAL_H 1~/* #undef HAVE_SIGNAL_H */~" $(@D)/lib/curl_config.h
	$(SED) "s~#define HAVE_RAND_EGD 1~/* #undef HAVE_RAND_EGD */~" $(@D)/lib/curl_config.h
	$(SED) "s~#define HAVE_GETPWUID_R 1~/* #undef HAVE_GETPWUID_R */~" $(@D)/lib/curl_config.h
	$(SED) "s~#define HAVE_GETEUID 1~/* #undef HAVE_GETEUID */~" $(@D)/lib/curl_config.h
	$(SED) "s~#define HAVE_SOCKETPAIR 1~/* #undef HAVE_SOCKETPAIR */~" $(@D)/lib/curl_config.h
endef

LIBCURL_POST_CONFIGURE_HOOKS += LIBCURL_FIX_CURL_CONFIG

$(eval $(autotools-package))
