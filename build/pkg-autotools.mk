
################################################################################
# Autotools package infrastructure
#
# This file implements an infrastructure that eases development of
# package .mk files for autotools packages. It should be used for all
# packages that use the autotools as their build system.
#
# See the Buildroot documentation for details on the usage of this
# infrastructure
#
# In terms of implementation, this autotools infrastructure requires
# the .mk file to only specify metadata informations about the
# package: name, version, download URL, etc.
#
# We still allow the package .mk file to override what the different
# steps are doing, if needed. For example, if <PKG>_BUILD_CMDS is
# already defined, it is used as the list of commands to perform to
# build the package, instead of the default autotools behaviour. The
# package can also define some post operation hooks.
#
################################################################################

# Variables used by other packages
ACLOCAL_DIR = /usr/share/aclocal
ACLOCAL_HOST_DIR = /usr/share/aclocal
ACLOCAL = /usr/bin/aclocal -I $(ACLOCAL_DIR)
AUTOMAKE = /usr/bin/automake
AUTOCONF = /usr/bin/autoconf
AUTOHEADER = /usr/bin/autoheader
AUTORECONF = $(HOST_CONFIGURE_OPTS) ACLOCAL="$(ACLOCAL)" AUTOCONF="$(AUTOCONF)" AUTOHEADER="$(AUTOHEADER)" AUTOMAKE="$(AUTOMAKE)" AUTOPOINT=/bin/true /usr/bin/autoreconf -f -i -I "$(ACLOCAL_DIR)" -I "$(ACLOCAL_HOST_DIR)"

#
# Utility function to upgrade config.sub and config.guess files
#
# argument 1 : directory into which config.guess and config.sub need
# to be updated. Note that config.sub and config.guess are searched
# recursively in this directory.
#
define CONFIG_UPDATE
	for file in config.guess config.sub; do \
		for i in $$(find $(1) -name $$file); do \
			cp build/gnuconfig/$$file $$i; \
		done; \
	done
endef

################################################################################
# inner-autotools-package -- defines how the configuration, compilation and
# installation of an autotools package should be done, implements a
# few hooks to tune the build process for autotools specifities and
# calls the generic package infrastructure to generate the necessary
# make targets
#
#  argument 1 is the lowercase package name
#  argument 2 is the uppercase package name, including an HOST_ prefix
#             for host packages
#  argument 3 is the uppercase package name, without the HOST_ prefix
#             for host packages
#  argument 4 is the type (target or host)
################################################################################

define inner-autotools-package

ifndef $(2)_MAKE
  $(2)_MAKE ?= $(MAKE)
endif

ifndef $(2)_AUTORECONF
  $(2)_AUTORECONF ?= NO
endif

$(2)_CONF_ENV			?=
$(2)_CONF_OPT			?=
$(2)_MAKE_ENV			?=
$(2)_MAKE_OPT			?=
$(2)_AUTORECONF_OPT		?= $($(2)_AUTORECONF_OPT)
$(2)_INSTALL_OPT                ?= install
$(2)_INSTALL_STAGING_OPT	?= DESTDIR=$$(STAGING_DIR) install
$(2)_INSTALL_TARGET_OPT		?= DESTDIR=$$(STAGING_DIR)  install


#
# Configure step. Only define it if not already defined by the package
# .mk file. And take care of the differences between host and target
# packages.
#
ifndef $(2)_CONFIGURE_CMDS

# Configure package for target
define $(2)_CONFIGURE_CMDS
	(cd $$($$(PKG)_DIR) && rm -rf config.cache && \
	$$(TARGET_CONFIGURE_OPTS) \
	$$(TARGET_CONFIGURE_ARGS) \
	$$($$(PKG)_CONF_ENV) \
	./configure \
		--target=$$(GNU_TARGET_NAME) \
		--host=$$(GNU_TARGET_NAME) \
		--build=$$(GNU_HOST_NAME) \
		--prefix=/usr \
		--exec-prefix=/usr \
		--sysconfdir=/etc \
		--program-prefix="" \
		--disable-gtk-doc \
		--disable-doc \
		--disable-docs \
		--disable-documentation \
		--disable-nls \
		--enable-static \
		--disable-shared \
		--disable-ipv6 \
		--disable-largefile \
		--with-xmlto=no \
		--with-fop=no \
		$$(QUIET) $$($$(PKG)_CONF_OPT) \
	)
endef
endif

#
# Hook to autoreconf the package if needed
#
define AUTORECONF_HOOK
	@$$(call MESSAGE,"Autoreconfiguring")
	$(Q)cd $$($$(PKG)_DIR) && $(AUTORECONF) $$($$(PKG)_AUTORECONF_OPT)
endef

ifeq ($$($(2)_AUTORECONF),YES)
$(2)_PRE_CONFIGURE_HOOKS += AUTORECONF_HOOK
endif

#
# Hook to update config.sub and config.guess if needed
#
define UPDATE_CONFIG_HOOK
       @$$(call MESSAGE,"Updating config.sub and config.guess")
       $$(call CONFIG_UPDATE,$$(@D))
endef

$(2)_POST_PATCH_HOOKS += UPDATE_CONFIG_HOOK

#
# Build step. Only define it if not already defined by the package .mk
# file.
#
ifndef $(2)_BUILD_CMDS
define $(2)_BUILD_CMDS
	$$(TARGET_MAKE_ENV) $$($$(PKG)_MAKE_ENV) $$($$(PKG)_MAKE) $$($$(PKG)_MAKE_OPT) -C  $$($$(PKG)_DIR)
endef
endif

#
# Host installation step. Only define it if not already defined by the
# package .mk file.
#
ifndef $(2)_INSTALL_CMDS
define $(2)_INSTALL_CMDS
	$$(HOST_MAKE_ENV) $$($$(PKG)_MAKE_ENV) $$($$(PKG)_MAKE) $$($$(PKG)_INSTALL_OPT) -C $$($$(PKG)_DIR)
endef
endif

#
# Staging installation step. Only define it if not already defined by
# the package .mk file.
#
ifndef $(2)_INSTALL_STAGING_CMDS
ifeq ($$($(2)_SUPPORT_SEPARATE_OUTPUT),YES)
define $(2)_INSTALL_STAGING_CMDS
	$$(TARGET_MAKE_ENV) $$($$(PKG)_MAKE_ENV) $$($$(PKG)_MAKE) $$($$(PKG)_INSTALL_STAGING_OPT) -C $$($$(PKG)_DIR)
	for i in $$$$(find $(STAGING_DIR)/usr/lib* -name "*.la"); do \
		cp -f $$$$i $$$$i~; \
		$$(SED) "s:\(['= ]\)/usr:\\1$(STAGING_DIR)/usr:g" $$$$i; \
	done
endef
else
define $(2)_INSTALL_STAGING_CMDS
	$$(TARGET_MAKE_ENV) $$($$(PKG)_MAKE_ENV) $$($$(PKG)_MAKE) $$($$(PKG)_INSTALL_STAGING_OPT) -C $$($$(PKG)_DIR)
	for i in $$$$(find $(STAGING_DIR)/usr/lib* -name "*.la"); do \
		cp -f $$$$i $$$$i~; \
		$$(SED) "s:\(['= ]\)/usr:\\1$(STAGING_DIR)/usr:g" $$$$i; \
	done
endef
endif
endif

# Call the generic package infrastructure to generate the necessary
# make targets
$(call inner-generic-package,$(1),$(2))

endef

################################################################################
# autotools-package -- the target generator macro for autotools packages
################################################################################

autotools-package = $(call inner-autotools-package,$(pkgname),$(call UPPERCASE,$(pkgname)))
