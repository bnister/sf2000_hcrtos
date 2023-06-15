/*
 * consumer.h -- SoC Regulator consumer support.
 *
 * Copyright (C) 2007, 2008 Wolfson Microelectronics PLC.
 *
 * Author: Liam Girdwood <lrg@slimlogic.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Regulator Consumer Interface.
 *
 * A Power Management Regulator framework for SoC based devices.
 * Features:-
 *   o Voltage and current level control.
 *   o Operating mode control.
 *   o Regulator status.
 *   o sysfs entries for showing client devices and status
 *
 * EXPERIMENTAL FEATURES:
 *   Dynamic Regulator operating Mode Switching (DRMS) - allows regulators
 *   to use most efficient operating mode depending upon voltage and load and
 *   is transparent to client drivers.
 *
 *   e.g. Devices x,y,z share regulator r. Device x and y draw 20mA each during
 *   IO and 1mA at idle. Device z draws 100mA when under load and 5mA when
 *   idling. Regulator r has > 90% efficiency in NORMAL mode at loads > 100mA
 *   but this drops rapidly to 60% when below 100mA. Regulator r has > 90%
 *   efficiency in IDLE mode at loads < 10mA. Thus regulator r will operate
 *   in normal mode for loads > 10mA and in IDLE mode for load <= 10mA.
 *
 */

#ifndef __LINUX_REGULATOR_CONSUMER_H_
#define __LINUX_REGULATOR_CONSUMER_H_

#include <linux/compiler.h>
#include <linux/err.h>

struct device;
struct regulator;

static inline struct regulator *__must_check
devm_regulator_get_optional(struct device *dev, const char *id)
{
	return ERR_PTR(-ENODEV);
}

static inline int regulator_enable(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_disable(struct regulator *regulator)
{
	return 0;
}

#endif
