CROSS_COMPILE=../output/host/usr/bin/mips-mti-linux-gnu-
all: dsc_test.c
	$(CROSS_COMPILE)gcc -I../output/staging/usr/include/  dsc_test.c -o dsc_test
	cp dsc_test ~/../media/weiwen.liu/

dsc_file: dsc_file.c
	$(CROSS_COMPILE)gcc -I../output/staging/usr/include/  $^ -o $@
	cp $@ ~/../media/weiwen.liu/

test_sha: test_sha.c
	$(CROSS_COMPILE)gcc -I../output/staging/usr/include/  $^ -o $@
	cp $@ ~/../media/weiwen.liu/
