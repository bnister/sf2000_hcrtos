#!/bin/bash

. $BR2_CONFIG > /dev/null 2>&1
export BR2_CONFIG

current_dir=$(dirname $0)
PYPATH=/usr/bin:/usr/local/bin:$PATH
FDTINFO=${TOPDIR}/build/scripts/fdt_get_property.py
BINMODIFY=${TOPDIR}/build/scripts/binmodify.py
GENBOOTMEDIA=${TOPDIR}/build/scripts/genbootmedia
GENSFBIN=${TOPDIR}/build/scripts/gensfbin.py
HCPROGINI=${TOPDIR}/build/scripts/hcprogini.py
GENPERSISTENTMEM=${TOPDIR}/build/tools/genpersistentmem/genpersistentmem
DDRCONFIGMODIFY=${TOPDIR}/build/tools/ddrconfig-modify/ddrconfig-modify
DTB2DTS=${TOPDIR}/build/tools/dtb2dts.py
DTS2DTB=${TOPDIR}/build/tools/dts2dtb.py
DTB=${IMAGES_DIR}/dtb.bin
[ ! -f ${HOST_DIR}/bin/mkimage ] && cp -vf ${TOPDIR}/build/tools/mkimage ${HOST_DIR}/bin/mkimage
[ ! -f ${HOST_DIR}/bin/hcprecomp2 ] && cp -vf ${TOPDIR}/build/tools/hcprecomp2 ${HOST_DIR}/bin/hcprecomp2
make -C $(dirname ${GENPERSISTENTMEM})

message()
{
	echo -e "\033[47;30m>>> $1\033[0m"
}

app=${CONFIG_APPS_NAME}

if [ -f ${DTB} ] ; then
	message "Update firmware filename to ${CONFIG_APPS_NAME}.uImage ....."
	PATH=$PYPATH ${DTB2DTS} --dtb ${DTB} --dts ${DTB}.dts
	fw_part=$(cat ${DTB}.dts | grep 'label = "firmware"' | head -n 1 | awk '{print $1}' | tr -cd "[0-9]")
	sed -i "s/part${fw_part}-filename =.*/part${fw_part}-filename = \"${CONFIG_APPS_NAME}.uImage\";/" ${DTB}.dts
	PATH=$PYPATH ${DTS2DTB} --dtb ${DTB} --dts ${DTB}.dts
	message "Update firmware filename to ${CONFIG_APPS_NAME}.uImage done!"
fi

if [ -f ${IMAGES_DIR}/${CONFIG_APPS_NAME}.out ] ; then
	app_ep_noncache=$(readelf -h ${IMAGES_DIR}/${app}.out | grep Entry | awk '{print $NF}' | sed 's/0x8/0xa/')
	app_ep=$(readelf -h ${IMAGES_DIR}/${app}.out | grep Entry | awk '{print $NF}')
	app_load_noncache=$(nm -n ${IMAGES_DIR}/${app}.out | awk '/T _start/ {print "0x"$1}' | sed 's/0x8/0xa/')
	app_load=$(nm -n ${IMAGES_DIR}/${app}.out | awk '/T _start/ {print "0x"$1}')
fi

if [ "${CONFIG_OF_SEPARATE}" = "y" ] && [ -f ${IMAGES_DIR}/dtb.bin ]; then
	if [ -f ${IMAGES_DIR}/${app}.out ] ; then
	message "Generating download-${app}.ini ....."
	cat << EOF > ${IMAGES_DIR}/download-${app}.ini
[Project]
RunMode=0
InitMode=0
RunAddr=${app_ep_noncache}
FileNum=2
[File0]
File=${app}.out
Type=5
Addr=${app_load_noncache}
[File1]
File=dtb.bin
Type=1
Addr=0xa5ff0000
[AutoRun]
AutoRun0=wm 0xb8800004 0x85ff0000
EOF
	message "Generating download-${app}.ini done!"
	fi
fi

if [ -f ${IMAGES_DIR}/hcboot.out ] && [ -f ${IMAGES_DIR}/hcboot.bin ] && [ -f "${BR2_EXTERNAL_BOARD_DDRINIT_FILE}" ]; then
	message "Generating bootloader.bin ....."
	fddrinit=$(basename ${BR2_EXTERNAL_BOARD_DDRINIT_FILE})
	hcboot_sz=$(wc -c ${IMAGES_DIR}/hcboot.bin | awk '{print $1}')
	hcboot_ep=$(readelf -h ${IMAGES_DIR}/hcboot.out | grep Entry | awk '{print $NF}')
	${DDRCONFIGMODIFY} --input ${BR2_EXTERNAL_BOARD_DDRINIT_FILE} --output ${IMAGES_DIR}/${fddrinit} \
		--size ${hcboot_sz} \
		--entry ${hcboot_ep} \
		--from 0xafc02000 \
		--to ${hcboot_ep}

	cat ${IMAGES_DIR}/${fddrinit} ${IMAGES_DIR}/hcboot.bin > ${IMAGES_DIR}/bootloader.bin
	message "Generating bootloader.bin done!"
fi

if [ -f ${IMAGES_DIR}/${app}.bin ] ; then
	message "Generating ${app}.uImage ....."
	if [ "${BR2_EXTERNAL_FW_COMPRESS_LZO1X}" = "y" ] ; then

		${HOST_DIR}/bin/hcprecomp2 ${IMAGES_DIR}/${app}.bin  ${IMAGES_DIR}/${app}.bin.lzo
		${HOST_DIR}/bin/mkimage -A mips -O u-boot -T standalone -C lzo -n ${app} -e ${app_ep} -a ${app_load} \
			-d ${IMAGES_DIR}/${app}.bin.lzo ${IMAGES_DIR}/${app}.uImage

	elif [ "${BR2_EXTERNAL_FW_COMPRESS_GZIP}" = "y" ] ; then

		gzip -kf9 ${IMAGES_DIR}/${app}.bin > ${IMAGES_DIR}/${app}.bin.gz
		${HOST_DIR}/bin/mkimage -A mips -O u-boot -T standalone -C gzip -n ${app} -e ${app_ep} -a ${app_load} \
			-d ${IMAGES_DIR}/${app}.bin.gz ${IMAGES_DIR}/${app}.uImage

	elif [ "${BR2_EXTERNAL_FW_COMPRESS_LZMA}" = "y" ] ; then

		lzma -zkf -c ${IMAGES_DIR}/${app}.bin > ${IMAGES_DIR}/${app}.bin.lzma
		${HOST_DIR}/bin/mkimage -A mips -O u-boot -T standalone -C lzma -n ${app} -e ${app_ep} -a ${app_load} \
			-d ${IMAGES_DIR}/${app}.bin.lzma ${IMAGES_DIR}/${app}.uImage

	elif [ "${BR2_EXTERNAL_FW_COMPRESS_NONE}" = "y" ] ; then
		${HOST_DIR}/bin/mkimage -A mips -O u-boot -T standalone -C none -n ${app} -e ${app_ep} -a ${app_load} \
			-d ${IMAGES_DIR}/${app}.bin ${IMAGES_DIR}/${app}.uImage

	fi
	message "Generating ${app}.uImage done!"
fi

if [ -f "${BR2_EXTERNAL_BOOTMEDIA_FILE}" ] ; then
	message "Generating logo.hc ....."
	${GENBOOTMEDIA} -i ${BR2_EXTERNAL_BOOTMEDIA_FILE} -o ${IMAGES_DIR}/fs-partition1-root/logo.hc
	message "Generating logo.hc done!"
fi

if [ -d ${IMAGES_DIR}/fs-partition1-root -a "`ls -A ${IMAGES_DIR}/fs-partition1-root`" != "" ] ; then
	message "Generating romfs.img ....."
	genromfs -f ${IMAGES_DIR}/romfs.img -d ${IMAGES_DIR}/fs-partition1-root/ -v "romfs"
	message "Generating romfs.img done!"
fi

firmware_version=$(date +%y%m%d%H%M)

message "Generating persistentmem.bin ....."
tvtype=$(PATH=$PYPATH ${FDTINFO} --dtb ${DTB} --node /hcrtos/de-engine --prop tvtype)
volume=$(PATH=$PYPATH ${FDTINFO} --dtb ${DTB} --node /hcrtos/i2so --prop volume)
${GENPERSISTENTMEM} -v ${firmware_version} -p ${BR2_EXTERNAL_PRODUCT_NAME} -V ${volume} -t ${tvtype} -o ${IMAGES_DIR}/persistentmem.bin
message "Generating persistentmem.bin done"

sfbin=${IMAGES_DIR}/sfburn.bin
message "Generating ${sfbin} ....."
sfbin=${IMAGES_DIR}/sfburn.bin
rm -f ${sfbin}
PATH=$PYPATH ${GENSFBIN} --wkdir ${IMAGES_DIR} --output ${sfbin} --dtb ${DTB} --fill 0xff
[ $? != 0 ] && exit 1;
message "Generating ${sfbin} done!"

message "Generating sfburn.ini ....."
md5=$(md5sum ${sfbin} | awk '{print $1}')
rm -f ${sfbin}.*
cp ${sfbin} ${sfbin}.${md5}

sfbin_size=$(wc -c ${sfbin} | awk '{print $1}' | xargs -i printf "0x%08x" {})
((sfbin_size_align=((sfbin_size + 0xffff) / 0x10000) * 0x10000))

cat << EOF > ${IMAGES_DIR}/sfburn.ini
[Project]
RunMode=0
InitMode=0
RunAddr=0xA0000200
FileNum=2
[File0]
File=$(basename ${sfbin}.${md5})
Type=1
Addr=0xA0060000
[File1]
File=flashwr_unify.abs
Type=1
Addr=0xA0000200
[AutoRun]
AutoRun0=wm 0x800001F0 $(printf 0x%08x $sfbin_size_align)
AutoRun1=wm 0x800001F4 0x00000000
AutoRun2=wm 0x800001F8 0x80060000
AutoRun3=wm 0xb8818504 0x0
EOF
message "Generating sfburn.ini done!"

message "Generating hcprog.ini ....!"
PATH=$PYPATH ${HCPROGINI} --output ${IMAGES_DIR}/hcprog.ini \
				--dtb ${DTB} \
				--chip H15XX \
				--product ${BR2_EXTERNAL_PRODUCT_NAME} \
				--draminit $(basename ${BR2_EXTERNAL_BOARD_DDRINIT_FILE}) \
				--version ${firmware_version}
message "Generating hcprog.ini done"

if [ -f ${IMAGES_DIR}/hcprog.ini ];then
	message "Generating HCFOTA_*******_*******.bin .....!"
	rm -f ${IMAGES_DIR}/HCFOTA*.bin
	${TOPDIR}/build/tools/HCFota_Generator --i=${IMAGES_DIR}/hcprog.ini
	cp ${IMAGES_DIR}/HCFOTA_*.bin  ${IMAGES_DIR}/HCFOTA.bin 
	message "Generating HCFOTA_*******_*******.bin done!"
fi

if [ "${BR2_PACKAGE_APPS_HCUSBCAST}" = "y" ];then
	message "Generating IUM_*******_*******.bin .....!"
	rm -f ${IMAGES_DIR}/IUM*.bin
	${IMAGES_DIR}/pcm2wav ${IMAGES_DIR}/HCFOTA_${BR2_EXTERNAL_PRODUCT_NAME}_${firmware_version}.bin ${IMAGES_DIR}/IUM_${BR2_EXTERNAL_PRODUCT_NAME}_${firmware_version}.bin
	message "Generating IUM_${BR2_EXTERNAL_PRODUCT_NAME}_${firmware_version} done!"
fi

cp -vf ${TOPDIR}/build/tools/flashwr_unify.abs ${IMAGES_DIR}/
cp -vf ${TOPDIR}/build/tools/spinandwr.out ${IMAGES_DIR}/
cp -vf ${TOPDIR}/build/tools/HCProgram_bridge.exe ${IMAGES_DIR}/
cp -vf ${TOPDIR}/build/tools/HCFota_Generator.exe ${IMAGES_DIR}/
cp -vf ${TOPDIR}/build/tools/updater.bin ${IMAGES_DIR}/


GEN_UPG_DIR=$(dirname $0)
chmod +x ${GEN_UPG_DIR}/gen_upgrade_pkt.sh
source ${GEN_UPG_DIR}/gen_upgrade_pkt.sh ${IMAGES_DIR} ${BR2_CONFIG}
