make O=bl sf2000_min_bl_defconfig
make O=bl all
make sf2000_min_defconfig
make all
dd if=/dev/zero bs=4096 count=1 | cat - output/images/avp.bin > output/images/bisrv.asd
cp output/images/bisrv.asd .
echo LCFG | dd of=bisrv.asd bs=4 conv=notrunc
cat crc.c | gcc -xc - && ./a.out && rm a.out
