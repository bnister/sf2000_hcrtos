LIBAIRCAST_VERSION = 5113ab2e73e437e853609467ed10cf57e1514cc1
LIBAIRCAST_SITE_METHOD = git
LIBAIRCAST_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libaircast.git
LIBAIRCAST_DEPENDENCIES = kernel newlib pthread

LIBAIRCAST_CONF_OPTS += -DHCRTOS=ON

LIBAIRCAST_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBAIRCAST_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBAIRCAST_VERSION)

LIBAIRCAST_INSTALL_STAGING = YES

$(eval $(cmake-package))
