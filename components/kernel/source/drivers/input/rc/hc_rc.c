// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright © 2021 HiChip Semiconductor Co., Ltd.
 *              http://www.hichiptech.com
 */
#include <errno.h>
#include <kernel/module.h>
#include <kernel/lib/fdt_api.h>
#include <hcuapi/pinmux.h>

#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/io.h>
#include "rc-core.h"

#define IRC_FIFO_THRESHOLD	32

extern unsigned long IRC_INTR;
extern unsigned long IRC0;

struct hc_rc_device {
	int				irq;
	void				*base;
	struct rc_dev			*rdev;

	pinpad_e			pin;
	pinmux_pinset_t			muxset;

	const char			*map_name;
};

/* Registers */
#define IRC_CFG			0x00
#define IRC_FIFOCFG		0x01
#define IRC_TMO_THRESHOLD	0x02
#define IRC_NOISE_THRESHOLD	0x03
#define IRC_IER			0x06
#define IRC_ISR			0x07
#define IRC_FIFO		0x08

/* define bit use in IRC_CFG */
#define IRC_CFG_EN		BIT(7)

/* define bit use in IRC_FIFOCFG */
#define IRC_FIFOCFG_RESET	BIT(7)

/* define bit use in IRC_IER */
#define IRC_IER_FIFO_EN		BIT(0)
#define IRC_IER_TMO_EN		BIT(1)

/* define bit use in IRC_ISR */
#define IRC_ISR_FIFO_ST		BIT(0)
#define IRC_ISR_TMO_ST		BIT(1)

/* Each bit is 8us */
#define BIT_DURATION 8

 /* maximum symbol period (microsecs),timeout to detect end of symbol train */
#define MAX_SYMB_TIME		24000

/* Noise threshold (microsecs) */
#define NOISE_DURATION		80

#define IR_HC_NAME "hc-rc"

static void hc_rc_rx_interrupt(uint32_t data)
{
	struct hc_rc_device *dev = (struct hc_rc_device *)data;
	struct ir_raw_event ev = { 0 };
	volatile u8 status, count;
	u8 irdata;

	status = readb(dev->base + IRC_ISR) & 0x3;
	writeb(status, dev->base + IRC_ISR);

	if (status) {
		count = readb(dev->base + IRC_FIFOCFG) & 0x7f;
		while (count-- > 0) {
			irdata = readb(dev->base + IRC_FIFO);
			ev.pulse = (irdata & 0x80) ? false : true;
			ev.duration = ((irdata & 0x7f) + 1) * BIT_DURATION;
			ir_raw_event_store_with_filter(dev->rdev, &ev);
		}
	}

	if (status & IRC_ISR_TMO_ST) {
		ir_raw_event_set_idle(dev->rdev, true);
	}

	/* Empty software fifo */
	ir_raw_event_handle(dev->rdev);

	return;
}

static int hc_rc_set_timeout(struct rc_dev *rdev, unsigned int timeout)
{
	unsigned int ithr;
	struct hc_rc_device *dev = rdev->priv;

	ithr = (timeout / (BIT_DURATION << 7)) - 1;

	dev_dbg(rdev->dev.parent, "setting idle threshold to %u\n", ithr);

	rdev->timeout = (ithr + 1) * (BIT_DURATION << 7);

	writeb(ithr, dev->base + IRC_TMO_THRESHOLD);

	return 0;
}

static void hc_rc_hardware_init(struct hc_rc_device *dev)
{
	unsigned int ithr;
	unsigned int nthr;
	unsigned int sampling_freq_select;

	sampling_freq_select = (12 * BIT_DURATION) >> 5;
	writeb(IRC_CFG_EN | sampling_freq_select, dev->base + IRC_CFG);

	writeb(IRC_FIFOCFG_RESET | (IRC_FIFO_THRESHOLD & 0x7f),
	       dev->base + IRC_FIFOCFG);

	ithr = (MAX_SYMB_TIME / (BIT_DURATION << 7)) - 1;
	writeb(ithr, dev->base + IRC_TMO_THRESHOLD);

	nthr = NOISE_DURATION / BIT_DURATION;
	writeb(nthr, dev->base + IRC_NOISE_THRESHOLD);

	writeb(IRC_ISR_FIFO_ST | IRC_ISR_TMO_ST, dev->base + IRC_ISR);

	writeb(IRC_IER_FIFO_EN | IRC_IER_TMO_EN, dev->base + IRC_IER);
}

static int hc_rc_open(struct rc_dev *rdev)
{
	struct hc_rc_device *dev = rdev->priv;

	hc_rc_hardware_init(dev);

	return 0;
}

static void hc_rc_close(struct rc_dev *rdev)
{
	struct hc_rc_device *dev = rdev->priv;
	u8 cfg = readb(dev->base + IRC_CFG);

	writeb(cfg & (~IRC_CFG_EN), dev->base + IRC_CFG);
	writeb(0, dev->base + IRC_IER);
	writeb(IRC_ISR_FIFO_ST | IRC_ISR_TMO_ST, dev->base + IRC_ISR);
}

static int hc_rc_init(void)
{
	int ret = -EINVAL;
	struct rc_dev *rdev;
	struct hc_rc_device *rc_dev;
	int np;
	struct pinmux_setting *active_state;

	np = fdt_node_probe_by_path("/hcrtos/irc");
	if (np < 0)
		return 0;

	rc_dev = kzalloc(sizeof(struct hc_rc_device), GFP_KERNEL);

	if (!rc_dev)
		return -ENOMEM;

	rdev = rc_allocate_device(RC_DRIVER_IR_RAW);

	if (!rdev)
		return -ENOMEM;

	fdt_get_property_string_index(np, "linux,rc-map-name", 0,
				      &rc_dev->map_name);
	active_state = fdt_get_property_pinmux(np, "active");
	if (active_state) {
		pinmux_select_setting(active_state);
		free(active_state);
	}

	rc_dev->irq = (int)&IRC_INTR;
	rc_dev->base = (void *)&IRC0;

	rdev->allowed_protocols = RC_PROTO_BIT_ALL_IR_DECODER;
	rdev->rx_resolution = BIT_DURATION;
	rdev->timeout = IR_DEFAULT_TIMEOUT;
	rdev->priv = rc_dev;
	rdev->open = hc_rc_open;
	rdev->close = hc_rc_close;
	rdev->map_name = rc_dev->map_name ?: RC_MAP_EMPTY;
	rdev->s_timeout = hc_rc_set_timeout;
	rdev->device_name = "HC Remote Control Receiver";

	ret = rc_register_device(rdev);
	if (ret < 0)
		goto err;

	rc_dev->rdev = rdev;

	xPortInterruptInstallISR(rc_dev->irq, hc_rc_rx_interrupt, (uint32_t)rc_dev);

	return 0;
rcerr:
	rc_unregister_device(rdev);
	rdev = NULL;
err:
	rc_free_device(rdev);
	dev_err(dev, "Unable to register device (%d)\n", ret);

	return ret;
}

module_driver(hc_rc, hc_rc_init, NULL, 1)
