
# Variables for use in Make constructs
comma := ,
empty :=
space := $(empty) $(empty)


[FROM] := a b c d e f g h i j k l m n o p q r s t u v w x y z . -
[TO]   := A B C D E F G H I J K L M N O P Q R S T U V W X Y Z _ _

UPPERCASE = $(strip $(eval __tmp := $1) \
	$(foreach c, $(join $(addsuffix :,$([FROM])),$([TO])), \
		$(eval __tmp :=	\
		$(subst $(word 1,$(subst :, ,$c)),$(word 2,$(subst :, ,$c)),\
	$(__tmp)))) \
	$(__tmp))

define LOWERCASE
$(shell echo $1 | tr '[:upper:]' '[:lower:]')
endef

define KCONFIG_ENABLE_OPT
	$(SED) "/\\<$(1)\\>/d" $(2)
	echo "$(1)=y" >> $(2)
endef

define KCONFIG_SET_OPT
	$(SED) "/\\<$(1)\\>/d" $(3)
	echo "$(1)=$(2)" >> $(3)
endef

define KCONFIG_DISABLE_OPT
	$(SED) "/\\<$(1)\\>/d" $(2)
	echo "# $(1) is not set" >> $(2)
endef

define KCONFIG_EXPORT_OPT
	$(SED) "/\\<$(1)\\>/d" $(2)
	echo "export $(1)=y" >> $(2)
endef

define KCONFIG_SET_DEFINE
	$(SED) "/\\<$(1)\\>/d" $(3)
	echo '#define $(1) $(2)' >> $(3)
endef

pkgdir       = $(dir $(lastword $(MAKEFILE_LIST)))
pkgname      = $(lastword $(subst /, ,$(pkgdir)))

# Sanitize macro cleans up generic strings so it can be used as a filename
# and in rules. Particularly useful for VCS version strings, that can contain
# slashes, colons (OK in filenames but not in rules), and spaces.
sanitize = $(subst $(space),_,$(subst :,_,$(subst /,_,$(strip $(1)))))

MESSAGE     = echo "$(TERM_BOLD)>>> $($(PKG)_NAME) $($(PKG)_VERSION) $(1)$(TERM_RESET)"
TERM_BOLD  := $(shell tput smso)
TERM_RESET := $(shell tput rmso)

BR2_TAR_OPTIONS=""

TAR ?= tar
TAR_STRIP_COMPONENTS = --strip-components

# Define extractors for different archive suffixes
INFLATE.bz2  = $(BZCAT)
INFLATE.gz   = $(ZCAT)
INFLATE.lz   = $(LZCAT)
INFLATE.lzma = $(XZCAT)
INFLATE.tbz  = $(BZCAT)
INFLATE.tbz2 = $(BZCAT)
INFLATE.tgz  = $(ZCAT)
INFLATE.xz   = $(XZCAT)
INFLATE.tar  = cat

define sep


endef
