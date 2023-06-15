/*
 * SPI master driver using generic bitbanged GPIO
 *
 * Copyright (C) 2006,2008 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <errno.h>
#include <kernel/module.h>
#include <kernel/completion.h>
#include <kernel/drivers/spi.h>
#include <kernel/lib/fdt_api.h>
#include <hcuapi/pinmux.h>

#include "spi_bitbang.h"
#include "spi_gpio.h"

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>
#include <kernel/drivers/hc_clk_gate.h>

#include <hcuapi/gpio.h>

/*
 * This bitbanging SPI master driver should help make systems usable
 * when a native hardware SPI engine is not available, perhaps because
 * its driver isn't yet working or because the I/O pins it requires
 * are used for other purposes.
 *
 * platform_device->driver_data ... points to spi_gpio
 *
 * spi->controller_state ... reserved for bitbang framework code
 * spi->controller_data ... holds chipselect GPIO
 *
 * spi->master->dev.driver_data ... points to spi_gpio->bitbang
 */

/*----------------------------------------------------------------------*/

/*
 * Because the overhead of going through four GPIO procedure calls
 * per transferred bit can make performance a problem, this code
 * is set up so that you can use it in either of two ways:
 *
 *   - The slow generic way:  set up platform_data to hold the GPIO
 *     numbers used for MISO/MOSI/SCK, and issue procedure calls for
 *     each of them.  This driver can handle several such busses.
 *
 *   - The quicker inlined way:  only helps with platform GPIO code
 *     that inlines operations for constant GPIOs.  This can give
 *     you tight (fast!) inner loops, but each such bus needs a
 *     new driver.  You'll define a new C file, with Makefile and
 *     Kconfig support; the C code can be a total of six lines:
 *
 *              #define DRIVER_NAME     "myboard_spi2"
 *              #define SPI_MISO_GPIO   119
 *              #define SPI_MOSI_GPIO   120
 *              #define SPI_SCK_GPIO    121
 *              #define SPI_N_CHIPSEL   4
 *              #include "spi-gpio.c"
 */

#ifndef DRIVER_NAME
#define DRIVER_NAME     "spi_gpio"

#define GENERIC_BITBANG /* vs tight inlines */

/* all functions referencing these symbols must define pdata */
#define SPI_MISO_GPIO   ((pdata)->miso)
#define SPI_MOSI_GPIO   ((pdata)->mosi)
#define SPI_SCK_GPIO    ((pdata)->sck)

#define SPI_N_CHIPSEL   ((pdata)->num_chipselect)

#endif

/*----------------------------------------------------------------------*/

static inline struct spi_gpio *__pure
spi_to_spi_gpio(const struct spi_device *spi)
{
	spi_gpio_s *spi_gpio = spi_master_get_devdata(spi->master);

	return spi_gpio;
}

static inline struct spi_gpio_platform_data *__pure
spi_to_pdata(const struct spi_device *spi)
{
	return &spi_to_spi_gpio(spi)->pdata;
}

#define pdata           spi_to_pdata(spi)

static inline void setsck(const struct spi_device *spi, int is_on)
{
	gpio_set_output(SPI_SCK_GPIO, is_on);
}

static inline void setmosi(const struct spi_device *spi, int is_on)
{
	gpio_set_output(SPI_MOSI_GPIO, is_on);
}

static inline int getmiso(const struct spi_device *spi)
{
	return (int)gpio_get_input(SPI_MISO_GPIO);
}

#undef pdata

#define spidelay(nsecs) do {} while (0)

#include "spi-bitbang-txrx.h"

static u32 spi_gpio_txrx_word_mode0(struct spi_device *spi,
                unsigned nsecs, u32 word, u8 bits)
{
        return bitbang_txrx_be_cpha0(spi, nsecs, 0, 0, word, bits);
}

static u32 spi_gpio_txrx_word_mode1(struct spi_device *spi,
                unsigned nsecs, u32 word, u8 bits)
{
        return bitbang_txrx_be_cpha1(spi, nsecs, 0, 0, word, bits);
}

static u32 spi_gpio_txrx_word_mode2(struct spi_device *spi,
                unsigned nsecs, u32 word, u8 bits)
{
        return bitbang_txrx_be_cpha0(spi, nsecs, 1, 0, word, bits);
}

static u32 spi_gpio_txrx_word_mode3(struct spi_device *spi,
                unsigned nsecs, u32 word, u8 bits)
{
        return bitbang_txrx_be_cpha1(spi, nsecs, 1, 0, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode0(struct spi_device *spi,
                unsigned nsecs, u32 word, u8 bits)
{
        unsigned flags = spi->master->flags;
        return bitbang_txrx_be_cpha0(spi, nsecs, 0, flags, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode1(struct spi_device *spi,
                unsigned nsecs, u32 word, u8 bits)
{
        unsigned flags = spi->master->flags;
        return bitbang_txrx_be_cpha1(spi, nsecs, 0, flags, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode2(struct spi_device *spi,
                unsigned nsecs, u32 word, u8 bits)
{
        unsigned flags = spi->master->flags;
        return bitbang_txrx_be_cpha0(spi, nsecs, 1, flags, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode3(struct spi_device *spi,
                unsigned nsecs, u32 word, u8 bits)
{
        unsigned flags = spi->master->flags;
        return bitbang_txrx_be_cpha1(spi, nsecs, 1, flags, word, bits);
}

/*----------------------------------------------------------------------*/

static void spi_gpio_chipselect(struct spi_device *spi, int is_active)
{
	spi_gpio_s *spi_gpio = spi_to_spi_gpio(spi);
	int cs = spi_gpio->cs_gpios[spi->chip_select];

	/* set initial clock polarity */
	if (is_active)
		setsck(spi, spi->mode & SPI_CPOL);

	if (cs != SPI_GPIO_NO_CHIPSELECT) {
		/* SPI is normally actice-low */

		gpio_set_output(cs, (spi->mode & SPI_CS_HIGH) ? is_active: !is_active);
	}
}

static int spi_gpio_setup(struct spi_device *spi)
{
	pinpad_e		cs;
	int			status = 0;

	spi_gpio_s *spi_gpio = spi_to_spi_gpio(spi);
	cs = spi_gpio->cs_gpios[spi->chip_select];

	if (!spi->controller_state) {
		if (cs != SPI_GPIO_NO_CHIPSELECT) {
			status = gpio_configure(cs, GPIO_DIR_OUTPUT);
			if (status)
				return status;
			gpio_set_output(cs, !(spi->mode) & SPI_CS_HIGH);
		}
	}
	
	if (!status) {
		/* in case it was initialized from static board data */
		spi_gpio->cs_gpios[spi->chip_select] = cs;
		status = spi_bitbang_setup(spi);
	}

	return status;
}

static void spi_gpio_cleanup(struct spi_device *spi)
{
	spi_gpio_s *spi_gpio = spi_to_spi_gpio(spi);
	unsigned long cs = spi_gpio->cs_gpios[spi->chip_select];

	spi_bitbang_cleanup(spi);
}

static int spi_gpio_alloc(unsigned pin, bool is_in)
{
	int value;
	pinpad_e padctl = pin;
	
	if (is_in) {
		value = gpio_configure(padctl, GPIO_DIR_INPUT);
	} else {
		value = gpio_configure(padctl, GPIO_DIR_OUTPUT);
		if (value == 0)
			gpio_set_output(padctl, 0);
	}

	return value;	
}

static int spi_gpio_request(struct spi_gpio_platform_data *pdata, u16 *res_flags)
{
	int value;

	/* NOTE:  SPI_*_GPIO symbols may reference "pdata" */

	if (SPI_MOSI_GPIO != SPI_GPIO_NO_MOSI) {
		value = spi_gpio_alloc(SPI_MOSI_GPIO, false);
		if (value)
			return value;
	} else {
		/* HW configuration without MOSI pin */
		*res_flags |= SPI_MASTER_NO_TX;
	}

	if (SPI_MISO_GPIO != SPI_GPIO_NO_MISO) {
		value = spi_gpio_alloc(SPI_MISO_GPIO, true);
		if (value)
			return value;
	} else {
		/* HW configuration without MISO pin */
		*res_flags |= SPI_MASTER_NO_RX;
	}
	value = spi_gpio_alloc(SPI_SCK_GPIO, false);

	return value;
}

static int spi_gpio_probe_dt(const char *node, struct spi_gpio_platform_data *pdata)
{
	int ret, np;
	u32 tmp;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return -ENODEV;

	if (!pdata) {
		return -ENOMEM;
	}
	
	ret = fdt_get_property_u_32_index(np, "gpio-sck", 0, (u32 *)&pdata->sck);
	if (ret < 0) {
		dev_err(dev, "gpio-sck property not found\n");
		goto error_free;
	}

	ret = fdt_get_property_u_32_index(np, "gpio-miso", 0, (u32 *)&pdata->miso);
	if (ret < 0) {
		printf("spi-gpio: gpio-miso property not found, switching to no-rx mode\n");
		pdata->miso = SPI_GPIO_NO_MISO;
	}

	ret = fdt_get_property_u_32_index(np, "gpio-mosi", 0, (u32 *)&pdata->mosi);
	if (ret < 0) {
		printf("spi-gpio: gpio-mosi property not found, switching to no-rx mode\n");
		pdata->miso = SPI_GPIO_NO_MOSI;
	}

	ret = fdt_get_property_u_32_index(np, "num-chipselects", 0, (u32 *)&tmp);
	if (ret < 0) {
		dev_err(dev, "num-chipselects property not found\n");
		goto error_free;
	}	
	pdata->num_chipselect = tmp;
	
	return np;

error_free:
	free(pdata);
	return ret;
}

static int spi_gpio_probe(const char *node)
{
	spi_gpio_s			*spi_gpio;
	struct spi_master		*master;
	struct spi_gpio_platform_data	*pdata = NULL;
	uint16_t			master_flags = 0;
	bool				use_of = 0;
	int				num_devices;
	int				status;
	int				np = -1;
	size_t				ctrl_size = ALIGN(sizeof(*master), 32);

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	status = spi_gpio_probe_dt(node, pdata);

	if (status < 0)
		return 0;
	if (status > 0) {
		use_of = 1;
		np = status;
	}

#ifdef GENERIC_BITBANG
	if (!pdata || (!use_of && !pdata->num_chipselect)) {
                return -ENODEV;
	}
#endif

	if (use_of && !SPI_N_CHIPSEL)
		num_devices = 1;
	else 
		num_devices = SPI_N_CHIPSEL;

	status = spi_gpio_request(pdata, &master_flags);
	if (status < 0)
		return status;

	master = kzalloc(sizeof(*spi_gpio) + ctrl_size +
			 (sizeof(unsigned long) * num_devices), GFP_KERNEL);
	if (!master) {
		kfree(pdata);
		return -ENOMEM;
	}
	
	spi_controller_set_devdata(master, (void *)master + ctrl_size);

	spi_gpio = spi_master_get_devdata(master);

	if (pdata)
		spi_gpio->pdata = *pdata;
	
	master->name = "hc-gpio-spi";
	master->bits_per_word_mask = SPI_BPW_RANGE_MASK(1, 32);
	master->flags = master_flags;
	master->bus_num = 1;
	master->num_chipselect = num_devices;
	master->setup = spi_gpio_setup;
	master->cleanup = spi_gpio_cleanup;

	if (use_of) {
		int i;

		/*
		 * In DT environments, take the CS GPIO from the "cs-gpios"
		 * property of the node.
		 */

	if (!SPI_N_CHIPSEL) {
			spi_gpio->cs_gpios[0] = SPI_GPIO_NO_CHIPSELECT;

	}
	else {
			for (i = 0; i < SPI_N_CHIPSEL; i++) {
				status = fdt_get_property_u_32_index(np, "cs-gpios", 0, (u32 *)&spi_gpio->cs_gpios[i]);
				if (status < 0) {
					dev_err(&pdev->dev, "invalid cs-gpios property\n");
					kfree(pdata);
					kfree(master);
					return status;
				}	
			}
	}
	}

	spi_gpio->bitbang.master = master;
	spi_gpio->bitbang.chipselect = spi_gpio_chipselect;

	if ((master_flags & (SPI_MASTER_NO_TX | SPI_MASTER_NO_RX)) == 0) {
		spi_gpio->bitbang.txrx_word[SPI_MODE_0] = spi_gpio_txrx_word_mode0;
		spi_gpio->bitbang.txrx_word[SPI_MODE_1] = spi_gpio_txrx_word_mode1;
		spi_gpio->bitbang.txrx_word[SPI_MODE_2] = spi_gpio_txrx_word_mode2;
		spi_gpio->bitbang.txrx_word[SPI_MODE_3] = spi_gpio_txrx_word_mode3;
	} else {
		spi_gpio->bitbang.txrx_word[SPI_MODE_0] = spi_gpio_spec_txrx_word_mode0;
		spi_gpio->bitbang.txrx_word[SPI_MODE_1] = spi_gpio_spec_txrx_word_mode1;
		spi_gpio->bitbang.txrx_word[SPI_MODE_2] = spi_gpio_spec_txrx_word_mode2;
		spi_gpio->bitbang.txrx_word[SPI_MODE_3] = spi_gpio_spec_txrx_word_mode3;
	}
	spi_gpio->bitbang.setup_transfer = spi_bitbang_setup_transfer;
	spi_gpio->bitbang.flags = SPI_CS_HIGH;

	status = spi_bitbang_start(&spi_gpio->bitbang);	

	return status;
}

static int spi_gpio_init(void)
{
	int rc = 0;

	rc = spi_gpio_probe("/hcrtos/spi-gpio");

	return 0;
}

module_driver(hc_gpio_spi, spi_gpio_init, NULL, 0)
