################################################################################
#
# cjson
#
################################################################################

CJSON_VERSION = 1.7.14
CJSON_SITE = $(call github,DaveGamble,cjson,v$(CJSON_VERSION))
CJSON_SOURCE = cjson-$(CJSON_VERSION).tar.gz
CJSON_INSTALL_STAGING = YES
CJSON_LICENSE = MIT
CJSON_LICENSE_FILES = LICENSE
CJSON_CPE_ID_VENDOR = cjson_project
# Set ENABLE_CUSTOM_COMPILER_FLAGS to OFF in particular to disable
# -fstack-protector-strong which depends on BR2_TOOLCHAIN_HAS_SSP
CJSON_CONF_OPTS += \
	-DENABLE_CJSON_TEST=OFF \
	-DENABLE_CUSTOM_COMPILER_FLAGS=OFF

# If BUILD_SHARED_AND_STATIC_LIBS is set to OFF, cjson uses the
# standard BUILD_SHARED_LIBS option which is passed by the
# cmake-package infrastructure.
ifeq ($(BR2_SHARED_STATIC_LIBS),y)
CJSON_CONF_OPTS += -DBUILD_SHARED_AND_STATIC_LIBS=ON
else
CJSON_CONF_OPTS += -DBUILD_SHARED_AND_STATIC_LIBS=OFF
endif

define CJSON_INSTALL_LIB_STAGING
	${INSTALL} -d $(STAGING_DIR)/usr/lib
	cp $(@D)/libcjson.a $(STAGING_DIR)/usr/lib
endef

define CJSON_INSTALL_H_STAGING
	${INSTALL} -d $(STAGING_DIR)/usr/include/cjson
	${INSTALL} $(@D)/cJSON.h -m 0666 $(STAGING_DIR)/usr/include/cjson
endef

CJSON_POST_BUILD_HOOKS += CJSON_INSTALL_LIB_STAGING CJSON_INSTALL_H_STAGING

$(eval $(cmake-package))
