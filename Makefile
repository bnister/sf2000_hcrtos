# we want bash as shell
SHELL:=$(shell if [ -x "$$BASH" ]; then echo $$BASH; \
 	else if [ -x /bin/bash ]; then echo /bin/bash; \
 	else echo sh; fi; fi)

export SHELL 

TOPDIR:=$(shell pwd)
CONFIG_KCONFIG=Kconfig
CONFIG=build/kconfig
DATE:=$(shell date +%Y%m%d)

XSUM ?= md5sum
GIT ?= git
RSYNC_VCS_EXCLUSIONS = \
	--exclude .svn --exclude .git --exclude .hg --exclude .bzr \
	--exclude CVS

gitexist = $(shell if [ -d ".git" ]; then echo "exist"; else echo "notexist"; fi;)
ifeq (gitexist, "exist")
GIT_REMOTE := $(shell $(GIT) remote)
GIT_USER_NAME := $(shell $(GIT) config user.name)
GIT_USER_EMAIL := $(shell $(GIT) config user.email)
endif

hash = $(shell echo $(1) | $(XSUM) | awk '{print $$1}')

# Strip quotes and then whitespaces
qstrip=$(strip $(subst ",,$(1)))
#"))

ifneq ("$(origin O)", "command line")
O?=output
CONFIG_DIR:=$(O)
else
override O:=$(O)

# We call make recursively to build packages. The command-line overrides that
# are passed to Buildroot don't apply to those package build systems. In
# particular, we don't want to pass down the O=<dir> option for out-of-tree
# builds, because the value specified on the command line will not be correct
# for packages.
MAKEOVERRIDES :=

CONFIG_DIR:=$(O)
endif

ifeq ("$(origin CONFIG_CUSTOM_DTS_PATH)", "command line")
export CONFIG_CUSTOM_DTS_PATH
endif

export CDPATH:=
BASE_DIR := $(shell mkdir -p $(O) && cd $(O) >/dev/null && pwd)
$(if $(BASE_DIR),, $(error output directory "$(O)" does not exist))

BR2_CONFIG=$(CONFIG_DIR)/.config
BR2_AUTOHEADER=$(BASE_DIR)/include/generated/br2_autoconf.h
BR2_TRISTATE=$(BUILD_DIR)/buildroot-config/tristate.config
BR2_AUTOCONF=$(BASE_DIR)/include/config/br2_auto.conf
BR2_KCONFIG_DUMMY=$(TOPDIR)/build/Kconfig.dummy
BR2_MKENTRY=$(TOPDIR)/build/Makefile.entry
BR2_MKLINK=$(TOPDIR)/build/Makefile.link
BR2_BUILD=$(TOPDIR)/build/Makefile.build
BR2_RULES=$(TOPDIR)/build/Makefile.rules
BR2_CLEAN=$(TOPDIR)/build/Makefile.clean
BR2_PKGCLEAN=$(TOPDIR)/build/scripts/pkg-clean.sh

HOST_DIR := $(BASE_DIR)/host
IMAGES_DIR := $(TOPDIR)/output/images
DATA_DIR := $(TOPDIR)/output/images/user_data
STAGING_DIR := $(BASE_DIR)/staging
STAGING_SUBDIR := $(STAGING_DIR)
BUILD_DIR := $(BASE_DIR)/build
DL_DIR := $(TOPDIR)/dl
PREBUILT_DIR := $(TOPDIR)/components/prebuilts
BR_PATH = "$(HOST_DIR)/bin:$(HOST_DIR)/sbin:$(PATH)"

HOST_DIR := $(shell mkdir -p $(HOST_DIR) && cd $(HOST_DIR) >/dev/null && pwd)
IMAGES_DIR := $(shell mkdir -p $(IMAGES_DIR) && cd $(IMAGES_DIR) >/dev/null && pwd)
DATA_DIR := $(shell mkdir -p $(DATA_DIR) && cd $(DATA_DIR) >/dev/null && pwd)
STAGING_DIR := $(shell mkdir -p $(STAGING_DIR) && cd $(STAGING_DIR) >/dev/null && pwd)
DL_DIR := $(shell mkdir -p $(DL_DIR) && cd $(DL_DIR) >/dev/null && pwd)
BUILD_DIR := $(shell mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) >/dev/null && pwd)

ifeq ($(V),1)
  Q=
else
  Q=@
endif

export TOPDIR Q HOST_DIR IMAGES_DIR DATA_DIR STAGING_DIR STAGING_SUBDIR DL_DIR BUILD_DIR
export BR2_CONFIG BR2_AUTOHEADER BR2_AUTOCONF BR2_KCONFIG_DUMMY
export BR2_BUILD BR2_RULES BR2_CLEAN BR2_MKENTRY BR2_MKLINK
export BR2_PKGCLEAN
export BR_PATH

-include $(BR2_CONFIG)
-include $(TOPDIR)/build/app-deps.mk
export apps_deps-y

ZCAT := $(call qstrip,$(BR2_ZCAT))
BZCAT := $(call qstrip,$(BR2_BZCAT))
XZCAT := $(call qstrip,$(BR2_XZCAT))
LZCAT := $(call qstrip,$(BR2_LZCAT))
TAR_OPTIONS = $(call qstrip,$(BR2_TAR_OPTIONS)) -xf

FS_PARTITION1_ROOT := $(IMAGES_DIR)/fs-partition1-root
FS_PARTITION2_ROOT := $(IMAGES_DIR)/fs-partition2-root
FS_PARTITION3_ROOT := $(IMAGES_DIR)/fs-partition3-root
FS_PARTITION1_ROOT := $(shell mkdir -p $(FS_PARTITION1_ROOT) && cd $(FS_PARTITION1_ROOT) >/dev/null && pwd)
FS_PARTITION2_ROOT := $(shell mkdir -p $(FS_PARTITION2_ROOT) && cd $(FS_PARTITION2_ROOT) >/dev/null && pwd)
FS_PARTITION3_ROOT := $(shell mkdir -p $(FS_PARTITION3_ROOT) && cd $(FS_PARTITION3_ROOT) >/dev/null && pwd)

PREBUILT_DIR :=$(call qstrip,$(TOPDIR)/components/prebuilts/$(CONFIG_PREBUILT_SUBDIR)/sysroot)

export FS_PARTITION1_ROOT FS_PARTITION2_ROOT FS_PARTITION3_ROOT PREBUILT_DIR

ifndef HOSTAR
HOSTAR:=ar
endif
ifndef HOSTAS
HOSTAS:=as
endif
ifndef HOSTCC
HOSTCC:=gcc
HOSTCC:=$(shell which $(HOSTCC) || type -p $(HOSTCC) || echo gcc)
endif
HOSTCC_NOCCACHE:=$(HOSTCC)
ifndef HOSTCXX
HOSTCXX:=g++
HOSTCXX:=$(shell which $(HOSTCXX) || type -p $(HOSTCXX) || echo g++)
endif
HOSTCXX_NOCCACHE:=$(HOSTCXX)
ifndef HOSTFC
HOSTFC:=gfortran
endif
ifndef HOSTCPP
HOSTCPP:=cpp
endif
ifndef HOSTLD
HOSTLD:=ld
endif
ifndef HOSTLN
HOSTLN:=ln
endif
ifndef HOSTNM
HOSTNM:=nm
endif
ifndef HOSTOBJCOPY
HOSTOBJCOPY:=objcopy
endif
ifndef HOSTRANLIB
HOSTRANLIB:=ranlib
endif
HOSTAR:=$(shell which $(HOSTAR) || type -p $(HOSTAR) || echo ar)
HOSTAS:=$(shell which $(HOSTAS) || type -p $(HOSTAS) || echo as)
HOSTFC:=$(shell which $(HOSTLD) || type -p $(HOSTLD) || echo || which g77 || type -p g77 || echo gfortran)
HOSTCPP:=$(shell which $(HOSTCPP) || type -p $(HOSTCPP) || echo cpp)
HOSTLD:=$(shell which $(HOSTLD) || type -p $(HOSTLD) || echo ld)
HOSTLN:=$(shell which $(HOSTLN) || type -p $(HOSTLN) || echo ln)
HOSTNM:=$(shell which $(HOSTNM) || type -p $(HOSTNM) || echo nm)
HOSTOBJCOPY:=$(shell which $(HOSTOBJCOPY) || type -p $(HOSTOBJCOPY) || echo objcopy)
HOSTRANLIB:=$(shell which $(HOSTRANLIB) || type -p $(HOSTRANLIB) || echo ranlib)

export HOSTAR HOSTAS HOSTCC HOSTCXX HOSTFC HOSTLD
export HOSTCC_NOCCACHE HOSTCXX_NOCCACHE

ifeq ($(BR2_HAVE_DOT_CONFIG),y)

all: world

else # ifeq ($(BR2_HAVE_DOT_CONFIG),y)

all: menuconfig

endif # ifeq ($(BR2_HAVE_DOT_CONFIG),y)

-include local.mk
include build/Makefile.in

include $(sort $(wildcard components/*/*.mk))

$(BUILD_DIR)/buildroot-config/%onf:
	mkdir -p $(@D)/lxdialog
	$(MAKE) CC="$(HOSTCC)" HOSTCC="$(HOSTCC)" \
	    obj=$(@D) -C $(CONFIG) -f Makefile.br $(@F)

COMMON_CONFIG_ENV = \
	BR2_CONFIG=$(BR2_CONFIG) \
	KCONFIG_AUTOHEADER=$(BR2_AUTOHEADER) \
	KCONFIG_TRISTATE=$(BR2_TRISTATE) \
	KCONFIG_AUTOCONFIG=$(BR2_AUTOCONF) \
	KCONFIG_DUMMY=$(BR2_KCONFIG_DUMMY)

menuconfig: $(BUILD_DIR)/buildroot-config/mconf
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_KCONFIG)
	make O=$(O) syncconfig

oldconfig syncconfig: $(BUILD_DIR)/buildroot-config/conf
	@$(COMMON_CONFIG_ENV) $< --oldconfig $(CONFIG_KCONFIG)

syncconfig: $(BUILD_DIR)/buildroot-config/conf oldconfig
	@mkdir -p $(BASE_DIR)/include/generated
	@mkdir -p $(BASE_DIR)/include/config
	$(COMMON_CONFIG_ENV) $< --syncconfig $(CONFIG_KCONFIG)

%_defconfig: $(BUILD_DIR)/buildroot-config/conf configs/%_defconfig
	@rm -f $(CONFIG_DIR)/.*_defconfig
	@touch $(CONFIG_DIR)/.$@
	@$(COMMON_CONFIG_ENV) $< --defconfig=configs/$@ $(CONFIG_KCONFIG)
	make O=$(O) syncconfig

defconfig-update:
	$(Q)( \
	for D in $(shell ls configs/*defconfig); do \
	  $(call MESSAGE, "updating $${D}"); \
	  make O=$(O) `basename $${D}` && cp -vf $(CONFIG_DIR)/.config $${D}; \
	done; \
	)

%defconfig-update:
	@$(call MESSAGE, "updating $(@:%-update=%)")
	make O=$(O) $(@:%-update=%)
	cp -vf $(CONFIG_DIR)/.config configs/$(@:%-update=%)

LASTDEFCONFIG=$(shell ls $(CONFIG_DIR)/.*_defconfig 2>/dev/null | awk -F '.' '{print $$2}')
lastdefconfig: $(BUILD_DIR)/buildroot-config/conf
	@if [ "$(LASTDEFCONFIG)x" = "x" ] ; then \
		echo -e "#\n# No lastdefconfig\n#" && exit 1; \
	fi
	@echo -e "#\n# Using $(LASTDEFCONFIG)\n#"
	make O=$(O) $(LASTDEFCONFIG)

TARGETS_INFO:=$(patsubst %,%-info,$(PACKAGES))
ALL_TARGETS_INFO:=$(patsubst %,%-info,$(ALLPACKAGES))
TARGETS_SOURCE:=$(patsubst %,%-show-source,$(PACKAGES))
ALL_TARGETS_SOURCE:=$(patsubst %,%-show-source,$(ALLPACKAGES))
TARGETS_DISTCLEAN:=$(patsubst %,%-distclean,$(PACKAGES))
TARGETS_CLEAN:=$(patsubst %,%-clean,$(PACKAGES))
ALL_TARGETS_DISTCLEAN:=$(patsubst %,%-distclean,$(ALLPACKAGES))
ALL_TARGETS_CLEAN:=$(patsubst %,%-clean,$(ALLPACKAGES))

show-source: $(sort $(TARGETS_SOURCE))
show-all-source: $(sort $(ALL_TARGETS_SOURCE))
show-targets: $(sort $(TARGETS_INFO))
show-all-targets: $(sort $(ALL_TARGETS_INFO))
distclean: $(sort $(TARGETS_DISTCLEAN))
	rm -rf $(STAGING_DIR)
	rm -rf $(BUILD_DIR)
	rm -rf $(BASE_DIR)/.config
	rm -rf $(BASE_DIR)/.config.old
	rm -rf $(BASE_DIR)/host
	rm -rf $(BASE_DIR)/include
all-distclean: $(sort $(ALL_TARGETS_DISTCLEAN))
	rm -rf $(STAGING_DIR)
	rm -rf $(BUILD_DIR)
	rm -rf $(BASE_DIR)/.config
	rm -rf $(BASE_DIR)/.config.old
	rm -rf $(BASE_DIR)/host
	rm -rf $(BASE_DIR)/include
clean: $(sort $(TARGETS_CLEAN))
	rm -rf $(STAGING_DIR)
	rm -rf $(BUILD_DIR)
	rm -rf $(BASE_DIR)/.config
	rm -rf $(BASE_DIR)/.config.old
	rm -rf $(BASE_DIR)/host
	rm -rf $(BASE_DIR)/include

all-clean: $(sort $(ALL_TARGETS_CLEAN))
	rm -rf $(STAGING_DIR)
	rm -rf $(BUILD_DIR)
	rm -rf $(BASE_DIR)/.config
	rm -rf $(BASE_DIR)/.config.old
	rm -rf $(BASE_DIR)/host
	rm -rf $(BASE_DIR)/include

$(BR2_AUTOHEADER) $(BR2_AUTOCONF): syncconfig

dirs: $(STAGING_DIR) $(HOST_DIR) $(IMAGES_DIR) $(DATA_DIR) $(DL_DIR) $(BUILD_DIR)

world: dirs $(PACKAGES)
	@$(foreach s, $(call qstrip,$(BR2_ROOTFS_POST_BUILD_SCRIPT)), \
		@$(call MESSAGE,"Executing post-build script $(s)")$(sep) \
		$(Q)$(EXTRA_ENV) $(s) $(IMAGES_DIR) $(call qstrip,$(BR2_ROOTFS_POST_SCRIPT_ARGS))$(sep))
	@$(foreach s, $(call qstrip,$(BR2_ROOTFS_POST_IMAGE_SCRIPT)), \
		$(call MESSAGE,"Executing post-image script $(s)"); \
		$(EXTRA_ENV) $(s) $(IMAGES_DIR) $(call qstrip,$(BR2_ROOTFS_POST_SCRIPT_ARGS))$(sep))

help:
	@echo 'Cleaning:'
	@echo '  clean                  - delete all files created by build'
	@echo '  all-clean              - delete all files created by all builds with different O=<>'
	@echo
	@echo '  distclean              - delete all non-source files (including .config)'
	@echo '  all-distclean          - delete all non-source files (including .config) of all builds with different O=<>'
	@echo
	@echo 'Build:'
	@echo '  all                    - make world'
	@echo '  release                - generate released sdk'
	@echo '  <package>-release      - generate released <package> and patches of <package>'
	@echo '  <package>-clean        - delete all files created by build of <package>'
	@echo '  <package>-distclean    - force distclean <package> (including revert the sourcecode of <package>)'
	@echo '  <package>-rebuild      - force recompile <package>'
	@echo '  <package>-reconfigure  - force reconfigure <package>'
	@echo
	@echo 'Configuration:'
	@echo '  menuconfig             - interactive curses-based configurator'
	@echo '  oldconfig              - resolve any unresolved symbols in .config'
	@echo '  syncconfig             - Same as oldconfig, but quietly, additionally update deps'
	@echo
	@echo 'Built-in configs:'
	@$(foreach b, $(sort $(notdir $(wildcard $(TOPDIR)/configs/*_defconfig))), \
	  printf "  %-35s - Build for %s\\n" $(b) $(b:_defconfig=);)
	@echo
	@echo 'Miscellaneous:'
	@echo '  show-source            - show sources of specific build'
	@echo '  show-all-source        - show all available sources'
	@echo
	@echo '  show-targets           - show targets of specific build'
	@echo '  show-all-targets       - show all available targets'
	@echo
	@echo '  show-lastdefconfig     - show defconfig name used for last build'
	@echo
	@echo '  make V=0|1             - 0 => quiet build (default), 1 => verbose build'
	@echo '  make O=dir             - Locate all output files in "dir", including .config'
	@echo
	@echo 'Miscellaneous - update configs:'
	@echo '  defconfig-update                              - update all Built-in configs with answer to all options'
	@$(foreach b, $(sort $(notdir $(wildcard $(TOPDIR)/configs/*_defconfig))), \
	  printf "  %-45s - update for %s with answer to all options\\n" $(b)-update $(b:_defconfig=);)
	@echo


.PHONY: all syncconfig menuconfig oldconfig world dirs $(BR2_AUTOHEADER) \
	$(BR2_AUTOCONF) $(PACKAGES)

