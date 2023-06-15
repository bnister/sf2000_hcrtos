/*
 *  linux/include/linux/clk.h
 *
 *  Copyright (C) 2004 ARM Limited.
 *  Written by Deep Blue Solutions Limited.
 *  Copyright (C) 2011-2012 Linaro Ltd <mturquette@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __LINUX_CLK_H
#define __LINUX_CLK_H

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/notifier.h>

struct clk;

#define clk_prepare_enable(...) (0)
#define clk_disable_unprepare(...)                                             \
	do {                                                                   \
	} while (0)

#define clk_put(...)                                                           \
	do {                                                                   \
	} while (0)

#define clk_get(...) (struct clk *)(NULL)
#define clk_get_rate(...) (-1)
#define clk_set_rate(...) (-1)
#define devm_clk_get(...) (struct clk *)((unsigned long)-MAX_ERRNO)

#endif
