################################################################################
#
# memtester
#
################################################################################

MEMTESTER_VERSION = 4.5.0
MEMTESTER_SOURCE = memtester-$(MEMTESTER_VERSION).tar.gz
MEMTESTER_SITE = http://pyropus.ca/software/memtester/old-versions
MEMTESTER_LICENSE = GPL-2.0
MEMTESTER_LICENSE_FILES = COPYING
MEMTESTER_CPE_ID_VENDOR = pryopus

MEMTESTER_TARGET_INSTALL_OPTS = INSTALLPATH=$(TARGET_DIR)/usr

MEMTESTER_MAKE_FLAGS ?= \
	CROSS_COMPILE=$(TARGET_CROSS) CXXFLAGS="$(TARGET_CXXFLAGS)" CFLAGS="$(TARGET_CFLAGS)" LDFLAGS="$(TARGET_LDFLAGS)"

MEMTESTER_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(MEMTESTER_MAKE_ENV) $(MAKE) $(MEMTESTER_MAKE_FLAGS) -C $(@D) clean
MEMTESTER_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(MEMTESTER_MAKE_ENV) $(MAKE) -C $(@D) install

MEMTESTER_BUILD_CMDS = \
	$(TARGET_MAKE_ENV) $(APPS_KIDEV_MAKE_ENV) $(MAKE) $(APPS_KIDEV_MAKE_FLAGS) -C $(@D) all

$(eval $(generic-package))
