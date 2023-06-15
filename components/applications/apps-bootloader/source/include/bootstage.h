/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * This file implements recording of each stage of the boot process. It is
 * intended to implement timing of each stage, reporting this information
 * to the user and passing it to the OS for logging / further analysis.
 * Note that it requires timer_get_boot_us() to be defined by the board
 *
 * Copyright (c) 2011 The Chromium OS Authors.
 */

#ifndef _BOOTSTAGE_H
#define _BOOTSTAGE_H

#include <stdio.h>

/* Flags for each bootstage record */
enum bootstage_flags {
	BOOTSTAGEF_ERROR	= 1 << 0,	/* Error record */
	BOOTSTAGEF_ALLOC	= 1 << 1,	/* Allocate an id */
};

/* bootstate sub-IDs used for kernel and ramdisk ranges */
enum {
	BOOTSTAGE_SUB_FORMAT,
	BOOTSTAGE_SUB_FORMAT_OK,
	BOOTSTAGE_SUB_NO_UNIT_NAME,
	BOOTSTAGE_SUB_UNIT_NAME,
	BOOTSTAGE_SUB_SUBNODE,

	BOOTSTAGE_SUB_CHECK,
	BOOTSTAGE_SUB_HASH = 5,
	BOOTSTAGE_SUB_CHECK_ARCH = 5,
	BOOTSTAGE_SUB_CHECK_ALL,
	BOOTSTAGE_SUB_GET_DATA,
	BOOTSTAGE_SUB_CHECK_ALL_OK = 7,
	BOOTSTAGE_SUB_GET_DATA_OK,
	BOOTSTAGE_SUB_LOAD,
};

/*
 * A list of boot stages that we know about. Each of these indicates the
 * state that we are at, and the action that we are about to perform. For
 * errors, we issue an error for an item when it fails. Therefore the
 * normal sequence is:
 *
 * progress action1
 * progress action2
 * progress action3
 *
 * and an error condition where action 3 failed would be:
 *
 * progress action1
 * progress action2
 * progress action3
 * error on action3
 */
enum bootstage_id {
	BOOTSTAGE_ID_START = 0,
	BOOTSTAGE_ID_CHECK_MAGIC,	/* Checking image magic */
	BOOTSTAGE_ID_CHECK_HEADER,	/* Checking image header */
	BOOTSTAGE_ID_CHECK_CHECKSUM,	/* Checking image checksum */
	BOOTSTAGE_ID_CHECK_ARCH,	/* Checking architecture */

	BOOTSTAGE_ID_CHECK_IMAGETYPE = 5,/* Checking image type */
	BOOTSTAGE_ID_DECOMP_IMAGE,	/* Decompressing image */
	BOOTSTAGE_ID_KERNEL_LOADED,	/* Kernel has been loaded */
	BOOTSTAGE_ID_DECOMP_UNIMPL = 7,	/* Odd decompression algorithm */
	BOOTSTAGE_ID_CHECK_BOOT_OS,	/* Calling OS-specific boot function */
	BOOTSTAGE_ID_BOOT_OS_RETURNED,	/* Tried to boot OS, but it returned */
	BOOTSTAGE_ID_CHECK_RAMDISK = 9,	/* Checking ram disk */

	BOOTSTAGE_ID_RD_MAGIC,		/* Checking ram disk magic */
	BOOTSTAGE_ID_RD_HDR_CHECKSUM,	/* Checking ram disk heder checksum */
	BOOTSTAGE_ID_RD_CHECKSUM,	/* Checking ram disk checksum */
	BOOTSTAGE_ID_COPY_RAMDISK = 12,	/* Copying ram disk into place */
	BOOTSTAGE_ID_RAMDISK,		/* Checking for valid ramdisk */
	BOOTSTAGE_ID_NO_RAMDISK,	/* No ram disk found (not an error) */

	BOOTSTAGE_ID_RUN_OS	= 15,	/* Exiting U-Boot, entering OS */

	BOOTSTAGE_ID_NEED_RESET = 30,
	BOOTSTAGE_ID_POST_FAIL,		/* Post failure */
	BOOTSTAGE_ID_POST_FAIL_R,	/* Post failure reported after reloc */

	/*
	 * This set is reported only by x86, and the meaning is different. In
	 * this case we are reporting completion of a particular stage.
	 * This should probably change in the x86 code (which doesn't report
	 * errors in any case), but discussion this can perhaps wait until we
	 * have a generic board implementation.
	 */
	BOOTSTAGE_ID_BOARD_INIT_R,	/* We have relocated */
	BOOTSTAGE_ID_BOARD_GLOBAL_DATA,	/* Global data is set up */

	BOOTSTAGE_ID_BOARD_INIT_SEQ,	/* We completed the init sequence */
	BOOTSTAGE_ID_BOARD_FLASH,	/* We have configured flash banks */
	BOOTSTAGE_ID_BOARD_FLASH_37,	/* In case you didn't hear... */
	BOOTSTAGE_ID_BOARD_ENV,		/* Environment is relocated & ready */
	BOOTSTAGE_ID_BOARD_PCI,		/* PCI is up */

	BOOTSTAGE_ID_BOARD_INTERRUPTS,	/* Exceptions / interrupts ready */
	BOOTSTAGE_ID_BOARD_DONE,	/* Board init done, off to main loop */
	/* ^^^ here ends the x86 sequence */

	/* Boot stages related to loading a kernel from an IDE device */
	BOOTSTAGE_ID_IDE_START = 41,
	BOOTSTAGE_ID_IDE_ADDR,
	BOOTSTAGE_ID_IDE_BOOT_DEVICE,
	BOOTSTAGE_ID_IDE_TYPE,

	BOOTSTAGE_ID_IDE_PART,
	BOOTSTAGE_ID_IDE_PART_INFO,
	BOOTSTAGE_ID_IDE_PART_TYPE,
	BOOTSTAGE_ID_IDE_PART_READ,
	BOOTSTAGE_ID_IDE_FORMAT,

	BOOTSTAGE_ID_IDE_CHECKSUM,	/* 50 */
	BOOTSTAGE_ID_IDE_READ,

	/* Boot stages related to loading a kernel from an NAND device */
	BOOTSTAGE_ID_NAND_PART,
	BOOTSTAGE_ID_NAND_SUFFIX,
	BOOTSTAGE_ID_NAND_BOOT_DEVICE,
	BOOTSTAGE_ID_NAND_HDR_READ = 55,
	BOOTSTAGE_ID_NAND_AVAILABLE = 55,
	BOOTSTAGE_ID_NAND_TYPE = 57,
	BOOTSTAGE_ID_NAND_READ,

	/* Boot stages related to loading a kernel from an network device */
	BOOTSTAGE_ID_NET_CHECKSUM = 60,
	BOOTSTAGE_ID_NET_ETH_START = 64,
	BOOTSTAGE_ID_NET_ETH_INIT,

	BOOTSTAGE_ID_NET_START = 80,
	BOOTSTAGE_ID_NET_NETLOOP_OK,
	BOOTSTAGE_ID_NET_LOADED,
	BOOTSTAGE_ID_NET_DONE_ERR,
	BOOTSTAGE_ID_NET_DONE,

	BOOTSTAGE_ID_FIT_FDT_START = 90,
	/*
	 * Boot stages related to loading a FIT image. Some of these are a
	 * bit wonky.
	 */
	BOOTSTAGE_ID_FIT_KERNEL_START = 100,

	BOOTSTAGE_ID_FIT_CONFIG = 110,
	BOOTSTAGE_ID_FIT_TYPE,
	BOOTSTAGE_ID_FIT_KERNEL_INFO,

	BOOTSTAGE_ID_FIT_COMPRESSION,
	BOOTSTAGE_ID_FIT_OS,
	BOOTSTAGE_ID_FIT_LOADADDR,
	BOOTSTAGE_ID_OVERWRITTEN,

	/* Next 10 IDs used by BOOTSTAGE_SUB_... */
	BOOTSTAGE_ID_FIT_RD_START = 120,	/* Ramdisk stages */

	/* Next 10 IDs used by BOOTSTAGE_SUB_... */
	BOOTSTAGE_ID_FIT_SETUP_START = 130,	/* x86 setup stages */

	BOOTSTAGE_ID_IDE_FIT_READ = 140,
	BOOTSTAGE_ID_IDE_FIT_READ_OK,

	BOOTSTAGE_ID_NAND_FIT_READ = 150,
	BOOTSTAGE_ID_NAND_FIT_READ_OK,

	BOOTSTAGE_ID_FIT_LOADABLE_START = 160,	/* for Loadable Images */
	/*
	 * These boot stages are new, higher level, and not directly related
	 * to the old boot progress numbers. They are useful for recording
	 * rough boot timing information.
	 */
	BOOTSTAGE_ID_AWAKE,
	BOOTSTAGE_ID_START_TPL,
	BOOTSTAGE_ID_END_TPL,
	BOOTSTAGE_ID_START_SPL,
	BOOTSTAGE_ID_END_SPL,
	BOOTSTAGE_ID_START_UBOOT_F,
	BOOTSTAGE_ID_START_UBOOT_R,
	BOOTSTAGE_ID_USB_START,
	BOOTSTAGE_ID_ETH_START,
	BOOTSTAGE_ID_BOOTP_START,
	BOOTSTAGE_ID_BOOTP_STOP,
	BOOTSTAGE_ID_BOOTM_START,
	BOOTSTAGE_ID_BOOTM_HANDOFF,
	BOOTSTAGE_ID_MAIN_LOOP,
	BOOTSTAGE_ID_ENTER_CLI_LOOP,
	BOOTSTAGE_KERNELREAD_START,
	BOOTSTAGE_KERNELREAD_STOP,
	BOOTSTAGE_ID_BOARD_INIT,
	BOOTSTAGE_ID_BOARD_INIT_DONE,

	BOOTSTAGE_ID_CPU_AWAKE,
	BOOTSTAGE_ID_MAIN_CPU_AWAKE,
	BOOTSTAGE_ID_MAIN_CPU_READY,

	BOOTSTAGE_ID_ACCUM_LCD,
	BOOTSTAGE_ID_ACCUM_SCSI,
	BOOTSTAGE_ID_ACCUM_SPI,
	BOOTSTAGE_ID_ACCUM_DECOMP,
	BOOTSTAGE_ID_ACCUM_OF_LIVE,
	BOOTSTAGE_ID_FPGA_INIT,
	BOOTSTAGE_ID_ACCUM_DM_SPL,
	BOOTSTAGE_ID_ACCUM_DM_F,
	BOOTSTAGE_ID_ACCUM_DM_R,
	BOOTSTAGE_ID_ACCUM_FSP_M,
	BOOTSTAGE_ID_ACCUM_FSP_S,
	BOOTSTAGE_ID_ACCUM_MMAP_SPI,

	/* a few spare for the user, from here */
	BOOTSTAGE_ID_USER,
	BOOTSTAGE_ID_ALLOC,
};

static void show_boot_progress(int progress)
{
#if 0
	printf("%i\n", progress);
#endif
}

static inline unsigned long bootstage_add_record(enum bootstage_id id,
		const char *name, int flags, unsigned long mark)
{
	return 0;
}

/*
 * This is a dummy implementation which just calls show_boot_progress(),
 * and won't even do that unless CONFIG_SHOW_BOOT_PROGRESS is defined
 */

static inline int bootstage_relocate(void)
{
	return 0;
}

static inline unsigned long bootstage_mark(enum bootstage_id id)
{
	show_boot_progress(id);
	return 0;
}

static inline unsigned long bootstage_error(enum bootstage_id id)
{
	show_boot_progress(-id);
	return 0;
}

static inline unsigned long bootstage_mark_name(enum bootstage_id id, const char *name)
{
	show_boot_progress(id);
	return 0;
}

static inline unsigned long bootstage_mark_code(const char *file, const char *func,
					int linenum)
{
	return 0;
}

static inline uint32_t bootstage_start(enum bootstage_id id, const char *name)
{
	return 0;
}

static inline uint32_t bootstage_accum(enum bootstage_id id)
{
	return 0;
}

static inline int bootstage_stash(void *base, int size)
{
	return 0;	/* Pretend to succeed */
}

static inline int bootstage_unstash(const void *base, int size)
{
	return 0;	/* Pretend to succeed */
}

static inline int bootstage_get_size(void)
{
	return 0;
}

static inline int bootstage_init(bool first)
{
	return 0;
}

/* Helper macro for adding a bootstage to a line of code */
#define BOOTSTAGE_MARKER()	\
		bootstage_mark_code(__FILE__, __func__, __LINE__)

#endif
