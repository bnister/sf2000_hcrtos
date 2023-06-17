make O=bl sf2000_min_bl_defconfig
make O=bl all
make sf2000_min_defconfig
make all
gcc -o crc crc.c && ./crc output/images/avp.bin output/images/bisrv.asd && rm crc
cp output/images/bisrv.asd .
