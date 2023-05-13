LIBYOUTUBE_VERSION = 98604d2051259f5000ea3292fd0b148d88e0c8c3
LIBYOUTUBE_SITE_METHOD = git
LIBYOUTUBE_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libyoutube.git
LIBYOUTUBE_DEPENDENCIES = kernel newlib pthread libopenssl libcurl cjson pcre hccast
LIBYOUTUBE_CONF_OPTS += -DBUILD_LIBRARY_TYPE="STATIC" -DBUILD_OS_TARGET="HCRTOS"

LIBYOUTUBE_BUILD_CMDS = [ -d "$(TOPDIR)/components/hclib/display/libyoutube/source" ] && (echo "exist source" ) || (rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/output/build/libyoutube-$(LIBYOUTUBE_VERSION) $(@D))

LIBYOUTUBE_EXTRACT_CMDS = \
	[ -d "$(TOPDIR)/components/hclib/display/libyoutube/source" ] && (echo "exist source" && rm $(TOPDIR)/output/build/libyoutube-$(LIBYOUTUBE_VERSION) -rf && cd $(TOPDIR)/components/hclib/display/libyoutube/source && sh ./gen-version.sh &&\
 rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/hclib/display/libyoutube/source/ $(@D)) || \
	(git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBYOUTUBE_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	cd $(@D) && sh ./gen-version.sh)

define LIBYOUTUBE_INSTALL_PREBUILT
	#$(INSTALL) -D -m 0664 $(@D)/include/yt/iptv_inter_yt_api.h $(PREBUILT_DIR)/usr/include/yt/iptv_inter_yt_api.h
	$(INSTALL) -D -m 0664 $(@D)/libiptv_yt.a $(PREBUILT_DIR)/usr/lib/libiptv_yt.a
endef

LIBYOUTUBE_POST_INSTALL_TARGET_HOOKS += LIBYOUTUBE_INSTALL_PREBUILT

LIBYOUTUBE_INSTALL_STAGING = YES

$(eval $(cmake-package))
