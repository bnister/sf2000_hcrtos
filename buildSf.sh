make O=bl sf2000_min_bl_defconfig
make O=bl all
make sf2000_min_defconfig
make all
dd if=/dev/zero bs=4096 count=1 | cat - output/images/avp.bin > bisrv.asd
