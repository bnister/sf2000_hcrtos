// SPDX-License-Identifier: GPL-2.0
/*
 * This file is used to hold the spi-nor-manufacturer parts that datasheet unknown or to be verified.
 *
 * IMPORTANT: You'd better to ask help from spi-nor-manufacturer to confirm this.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

static const struct flash_info general_parts[] = {
	{ "25q32B",  INFO(0x544016, 0, 64 * 1024,	64, SECT_4K ) },
	{ "25q128B",  INFO(0x544018, 0, 64 * 1024,     256, SECT_4K ) },

	// Zbit
	{ "25VQ16B",  INFO(0x5E4015, 0, 64 * 1024,	32, SECT_4K ) },
	{ "25VQ32B",  INFO(0x5E4016, 0, 64 * 1024,	64, SECT_4K ) },
	{ "25VQ64B",  INFO(0x5E4017, 0, 64 * 1024,	128, SECT_4K ) },
};

const struct spi_nor_manufacturer spi_nor_general= {
	.name = "General",
	.parts = general_parts,
	.nparts = ARRAY_SIZE(general_parts),
};

#if defined(CONFIG_SUPPORT_UNKNOW_FLASH)
static const struct flash_info unknown_parts[] = {
	{ "Unknow2M", INFO(0x5E4015, 0, 64 * 1024, 32, SECT_4K) },
	{ "Unknow4M", INFO(0x5E4016, 0, 64 * 1024, 64, SECT_4K) },
	{ "Unknow8M", INFO(0x5E4017, 0, 64 * 1024, 128, SECT_4K) },
	{ "Unknow16M",INFO(0x5E4018, 0, 64 * 1024, 256, SECT_4K) },
	{ "Unknow32M",INFO(0x5E4019, 0, 64 * 1024, 512, SECT_4K|
			SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_4B_OPCODES | SPI_NOR_HAS_LOCK |
			SPI_NOR_HAS_TB | SPI_NOR_TB_SR_BIT6) },
};

const struct spi_nor_manufacturer spi_nor_unknown = {
	.name = "Unknow",
	.parts = unknown_parts,
	.nparts = ARRAY_SIZE(unknown_parts),
};
#endif
