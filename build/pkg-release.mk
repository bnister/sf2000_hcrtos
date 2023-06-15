
BR2_VERSION:=$(shell git show-ref --tags -d | grep `git log -1 --pretty=%h` | sed 's/\^{}//g' | awk -F '/' '{print $$3}')
ifeq ($(BR2_VERSION),)
BR2_VERSION:=$(shell git log -1 --pretty=%h)
endif
OUT=hcrtos-$(BR2_VERSION)

release:
	-rm -rf $(TOPDIR)/.tmp
	mkdir -p $(TOPDIR)/.tmp
	cd $(TOPDIR) && (git archive --format=tar --prefix=$(OUT)/ $(BR2_VERSION) | tar -x -C $(TOPDIR)/.tmp)
	git submodule foreach 'git archive --format=tar $$sha1 | tar -x -C $(TOPDIR)/.tmp/$(OUT)/$$path'
	cd $(TOPDIR)/.tmp && tar -cjf $(TOPDIR)/$(OUT).tar.bz2 $(OUT)
	rm -rf $(TOPDIR)/.tmp
