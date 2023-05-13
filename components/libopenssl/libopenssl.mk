################################################################################
#
# libopenssl
#
################################################################################

LIBOPENSSL_VERSION = 1.1.1k
LIBOPENSSL_SITE = https://www.openssl.org/source
LIBOPENSSL_SOURCE = openssl-$(LIBOPENSSL_VERSION).tar.gz
LIBOPENSSL_LICENSE = OpenSSL or SSLeay
LIBOPENSSL_LICENSE_FILES = LICENSE
LIBOPENSSL_INSTALL_STAGING = YES
LIBOPENSSL_DEPENDENCIES = zlib pthread
LIBOPENSSL_TARGET_ARCH = $(call qstrip,$(BR2_PACKAGE_LIBOPENSSL_TARGET_ARCH))
LIBOPENSSL_CFLAGS = $(TARGET_CFLAGS)
LIBOPENSSL_CPE_ID_VENDOR = $(LIBOPENSSL_PROVIDES)
LIBOPENSSL_CPE_ID_PRODUCT = $(LIBOPENSSL_PROVIDES)

LIBOPENSSL_CFLAGS += -D__STDC_NO_ATOMICS__ -DNO_SYSLOG -DOPENSSL_NO_UI_CONSOLE -DHC_RTOS -Wno-missing-field-initializers -Wno-format-nonliteral -Wno-format

# fixes the following build failures:
#
# - musl
#   ./libcrypto.so: undefined reference to `getcontext'
#   ./libcrypto.so: undefined reference to `setcontext'
#   ./libcrypto.so: undefined reference to `makecontext'
#
# - uclibc:
#   crypto/async/arch/../arch/async_posix.h:32:5: error: unknown type name 'ucontext_t'
#

ifeq ($(BR2_TOOLCHAIN_USES_MUSL),y)
LIBOPENSSL_CFLAGS += -DOPENSSL_NO_ASYNC
endif
ifeq ($(BR2_TOOLCHAIN_HAS_UCONTEXT),)
LIBOPENSSL_CFLAGS += -DOPENSSL_NO_ASYNC
endif

define LIBOPENSSL_CONFIGURE_CMDS
	(cd $(@D); \
		$(TARGET_CONFIGURE_ARGS) \
		$(TARGET_CONFIGURE_OPTS) \
		./Configure \
			--prefix=/usr \
			--openssldir=/etc/ssl \
			threads \
			no-shared \
			gcc \
			no-afalgeng \
			no-aria \
			no-asan \
			no-asm \
			no-bf \
			no-blake2 \
			no-camellia \
			no-capieng \
			no-cast \
			no-chacha \
			no-cmac \
			no-cms \
			no-crypto-mdebug \
			no-crypto-mdebug-backtrace \
			no-ct \
			no-deprecated \
			no-devcryptoeng \
			no-dgram \
			no-dsa \
			no-dso \
			no-dtls \
			no-dynamic-engine \
			no-ec \
			no-ec2m \
			no-ecdh \
			no-ecdsa \
			no-ec_nistp_64_gcc_128 \
			no-egd \
			no-engine \
			no-external-tests \
			no-fuzz-libfuzzer \
			no-fuzz-afl \
			no-gost \
			no-heartbeats \
			no-idea \
			no-makedepend \
			no-md2 \
			no-md4 \
			no-mdc2 \
			$(if $(BR2_STATIC_LIBS),zlib,zlib-dynamic) \
			$(if $(BR2_STATIC_LIBS),no-dso) \
			$(if $(BR2_PACKAGE_LIBOPENSSL_DH_SUPPORT),,no-dh) \
	)
	$(SED) "s#-march=[-a-z0-9] ##" -e "s#-mcpu=[-a-z0-9] ##g" $(@D)/Makefile
	$(SED) "s#-O[0-9sg]#$(LIBOPENSSL_CFLAGS)#" $(@D)/Makefile
	$(SED) "s# build_tests##" $(@D)/Makefile
endef

# libdl is not available in a static build, and this is not implied by no-dso
define LIBOPENSSL_FIXUP_STATIC_MAKEFILE
	$(SED) 's#-ldl##g' $(@D)/Makefile
	$(SED) 's/^PROGRAMS=/#PROGRAMS=/g' $(@D)/Makefile
	$(SED) 's/^SCRIPTS=/#SCRIPTS=/g' $(@D)/Makefile
endef
LIBOPENSSL_POST_CONFIGURE_HOOKS += LIBOPENSSL_FIXUP_STATIC_MAKEFILE

define LIBOPENSSL_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define LIBOPENSSL_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) DESTDIR=$(STAGING_DIR) install_dev
endef

$(eval $(generic-package))
