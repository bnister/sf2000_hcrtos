include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

all : 
	@:

install: FORCE
	cp -rf $(srctree)mips-mti-elf/* $(STAGING_DIR)/

clean: FORCE
	@:

.PHONY: FORCE

FORCE:

endif	# skip-makefile
