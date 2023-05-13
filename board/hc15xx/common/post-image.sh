#!/bin/bash

DTB=${IMAGES_DIR}/dtb.bin

if [ -d ${DATA_DIR} ] ; then
    echo "Generating user_data.bin ....."
    echo "1" > ${DATA_DIR}/.tmp

		cp ${IMAGES_DIR}/dtb.bin ${IMAGES_DIR}/user_data/ -f
    PYPATH=/usr/bin:/usr/local/bin:$PATH
    PARTINFO=${TOPDIR}/build/scripts/partinfo.py
    SIZE=$(PATH=$PYPATH ${PARTINFO} --dtb ${DTB} --partname user_data --get-size true)

    echo "data partition size is ${SIZE}"
    #mkfs.jffs2 -r ${DATA_DIR} -o ${IMAGES_DIR}/user_data.bin -e 0x4000 -pad ${SIZE} -n
    ./build/tools/mklittlefs -c ${IMAGES_DIR}/user_data/ -d 5 -b 4096 -s 0x10000 ${IMAGES_DIR}/user_data.bin
    echo "Generating user_data.bin done"
fi
