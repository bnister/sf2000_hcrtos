
ddrconfig_modify_src = \
	$(CURDIR)/ddrconfig-modify.c

$(CURDIR)/ddrconfig-modify : $(ddrconfig_modify_src)
	gcc -s \
	$^ \
	-Wall -Wno-uninitialized -Os -std=gnu99 \
	-I$(CURDIR)/../../../SOURCE/linux-drivers/include/uapi \
	-o $@

all : $(CURDIR)/ddrconfig-modify

.PHONY : clean
clean:
	-rm *.o *.mod
	-rm $(CURDIR)/ddrconfig-modify
