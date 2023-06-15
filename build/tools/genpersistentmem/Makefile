
genpersistentmem_src = \
	$(CURDIR)/main.c

$(CURDIR)/genpersistentmem : $(genpersistentmem_src)
	gcc -s \
	$^ \
	-Wall -Wno-uninitialized -Os -std=gnu99 \
	-I$(CURDIR)/../../../SOURCE/linux-drivers/include/uapi \
	-I$(CURDIR)/../../../components/kernel/source/include/uapi \
	-o $@

all : $(CURDIR)/genpersistentmem

.PHONY : clean
clean:
	-rm *.o *.mod
	-rm $(CURDIR)/genpersistentmem
