/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _LINUX_IOPOLL_H
#define _LINUX_IOPOLL_H

#include <linux/types.h>
#include <linux/io.h>
#include <linux/ktime.h>
#include <linux/errno.h>
#include <linux/delay.h>

#define readx_poll_timeout_atomic(op, addr, val, cond, delay_us, timeout_us) \
({ \
	ktime_t timeout = ktime_add_us(ktime_get(), timeout_us); \
	for (;;) { \
		(val) = op(addr); \
		if (cond) \
			break; \
		if (timeout_us && ktime_compare(ktime_get(), timeout) > 0) { \
			(val) = op(addr); \
			break; \
		} \
		if (delay_us) { \
			udelay(delay_us);	\
		} \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
})

#define readl_poll_timeout_atomic(addr, val, cond, delay_us, timeout_us) \
			readx_poll_timeout_atomic(readl, addr, val, cond, delay_us, timeout_us)

#endif /* _LINUX_IO_H */
