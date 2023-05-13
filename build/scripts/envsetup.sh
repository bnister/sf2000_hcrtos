#!/bin/bash

export HC_TOP_DIR=$(cd $(dirname ${BASH_SOURCE[0]})/../.. && pwd)
export HC_OUT_DIR=${HC_TOP_DIR}/output
export HC_KERNEL_DIR=${HC_TOP_DIR}/components/kernel/source
export HC_BOOTLOADER_DIR=${HC_TOP_DIR}/components/applications/apps-bootloader/source
export HC_DTS_DIR=${HC_TOP_DIR}/board/hc16xx/common/dts

function print_env()
{
    echo ""
    echo "HC_TOP_DIR=${HC_TOP_DIR}"
    echo ""
    echo "command list:"
    echo "    croot   : jump to the hcrtos directory"
    echo "    ckernel : jump to the kernel directory"
    echo "    cboot   : jump to the bootloader directory"
    echo "    cdts    : jump to the dts directory"
    echo ""
    echo "    mkboot  : compile hcboot and packaging firmware, eg: kernel. bootloader. dts"
    echo "    mkernel : compile hcrtos and packaging firmware, eg: kernel and dts"
    echo "    mkall   : compile hcboot/hcrtos  and packaging firmware"
    echo "    mclean  : clean hcboot and hcrtos output directory"
    echo ""
}

function croot()
{
    echo "cd ${HC_TOP_DIR}"
	cd $HC_TOP_DIR
}

function ckernel()
{
    echo "cd ${HC_KERNEL_DIR}"
	cd $HC_KERNEL_DIR
}

function cout()
{
    echo "cd ${HC_OUT_DIR}"
	cd $HC_OUT_DIR
}

function cboot()
{
    echo "cd ${HC_BOOTLOADER_DIR}"
    cd $HC_BOOTLOADER_DIR
}

function cdts()
{
    echo "cd ${HC_DTS_DIR}"
	cd $HC_DTS_DIR
}

function __mkboot()
{
    make O=output-bl kernel-rebuild apps-bootloader-rebuild all
}

function __mkkernel()
{
    make kernel-rebuild cmds-rebuild
}

function mkboot()
{
    __mkboot
    make all
}

function mkernel()
{
    __mkkernel
    make all
}

function mclean()
{
    echo "rm -rf output*"
    rm -rf output*
}

function mkall()
{
    __mkboot
    __mkkernel
    make all
}

print_env
