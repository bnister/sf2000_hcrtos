################################################################################
#
# hccast
#
################################################################################

HCCAST_VERSION =
HCCAST_SITE_METHOD = UNDEFINED
HCCAST_SITE = UNDEFINED
HCCAST_LICENSE = LGPL-2.0+
HCCAST_LICENSE_FILES = COPYING
HCCAST_INSTALL_STAGING = YES

HCCAST_BUILD_CMDS = rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/hccast/source/ $(@D)
HCCAST_EXTRACT_CMDS = rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/hccast/source/ $(@D)

HCCAST_CONF_OPTS += -DHCRTOS=ON

ifeq ($(CONFIG_SOC_HC15XX),y)
HCCAST_CONF_OPTS += -DAIRMIRROR_P30=ON
HCCAST_CONF_OPTS += -DSOC_HC15XX=ON
endif

ifeq ($(BR2_PACKAGE_LVGL_CAST_APP),y)
BR2_PACKAGE_HCCAST_NET=y
BR2_PACKAGE_HCCAST_WIRELESS=y
endif

ifeq ($(BR2_PACKAGE_HCCAST_NET),y)
#HCCAST_DEPENDENCIES += wpa_supplicant
HCCAST_CONF_OPTS += -DNETWORK_API=ON
endif

ifeq ($(BR2_PACKAGE_HCCAST_WIRELESS),y)
HCCAST_DEPENDENCIES += kernel pthread cjson
HCCAST_CONF_OPTS += -DWIRELESS_CASTING=ON
endif

ifeq ($(BR2_PACKAGE_HCCAST_AIRCAST),y)
HCCAST_CONF_OPTS += -DHCCAST_AIRCAST=ON
ifeq ($(BR2_PACKAGE_HCCAST_AIRCAST_MIRROR_ONLY),y)
HCCAST_CONF_OPTS += -DHCCAST_AIRCAST_MIRROR_ONLY=ON
else
HCCAST_DEPENDENCIES += ffmpeg
endif
endif

ifeq ($(BR2_PACKAGE_HCCAST_MIRACAST),y)
HCCAST_CONF_OPTS += -DHCCAST_MIRACAST=ON
endif

ifeq ($(BR2_PACKAGE_HCCAST_DLNA),y)
HCCAST_CONF_OPTS += -DHCCAST_DLNA=ON
HCCAST_DEPENDENCIES += ffmpeg
endif

ifeq ($(BR2_PACKAGE_HCCAST_USBMIRROR),y)
HCCAST_DEPENDENCIES += libusb kernel cjson
HCCAST_CONF_OPTS += -DHCCAST_UM=ON
endif

ifeq ($(BR2_PACKAGE_HCCAST_IPTV),y)
HCCAST_CONF_OPTS += -DHCCAST_IPTV=ON
endif

ifeq ($(BR2_PACKAGE_PREBUILTS_RTL8188EU),y)
HCCAST_CONF_OPTS += -DWIFI_SUPPORT=ON
HCCAST_CONF_OPTS += -DWIFI_RTL8188ETV_SUPPORT=ON
endif

ifeq ($(BR2_PACKAGE_PREBUILTS_RTL8188FU),y)
HCCAST_CONF_OPTS += -DWIFI_SUPPORT=ON
HCCAST_CONF_OPTS += -DWIFI_RTL8188FU_SUPPORT=ON
endif

ifeq ($(BR2_PACKAGE_PREBUILTS_RTL8733BU),y)
HCCAST_CONF_OPTS += -DWIFI_SUPPORT=ON
HCCAST_CONF_OPTS += -DWIFI_RTL8733BU_SUPPORT=ON
endif

HTTPD_WEB_FOLDER = $(@D)/httpd/web
ifeq ($(BR2_PACKAGE_APPS_HCSCREEN_P1),y)

define HCCAST_PREPARE_WEB
	if test ! -e $(@D)/p1 ; then \
		git clone $(APPS_HCSCREEN_P1_SITE) $(@D)/p1; \
		git -C $(@D)/p1 checkout $(APPS_HCSCREEN_P1_VERSION); \
	fi
endef

HTTPD_WEB_FOLDER = $(@D)/p1/extra/web
HCCAST_CONF_OPTS += -DWEB_CSTM_P1=ON
endif

ifeq ($(BR2_PACKAGE_APPS_HCSCREEN_P2),y)

define HCCAST_PREPARE_WEB
	if test ! -e $(@D)/p2 ; then \
		git clone $(APPS_HCSCREEN_P2_SITE) $(@D)/p2; \
		git -C $(@D)/p2 checkout $(APPS_HCSCREEN_P2_VERSION); \
	fi
endef

HTTPD_WEB_FOLDER = $(@D)/p2/extra/web
HCCAST_CONF_OPTS += -DWEB_CSTM_P1=ON
endif

define HCCAST_INSTALL_HEADERS
	$(INSTALL) -d $(STAGING_DIR)/usr/include/hccast
	$(INSTALL) $(@D)/inc/* -m 0666 $(STAGING_DIR)/usr/include/hccast
endef

define HCCAST_GENERATE_HTTPD
	$(@D)/httpd/web_tool/web_generate $(HTTPD_WEB_FOLDER) $(@D)/httpd
endef

HCCAST_POST_INSTALL_TARGET_HOOKS += HCCAST_INSTALL_HEADERS

HCCAST_PRE_BUILD_HOOKS += HCCAST_PREPARE_WEB HCCAST_GENERATE_HTTPD


$(eval $(cmake-package))
