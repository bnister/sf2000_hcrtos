/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_JIFFIES_H
#define _LINUX_JIFFIES_H

#include <linux/typecheck.h>
#include <linux/ktime.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define jiffies                                                                \
	({                                                                     \
		TickType_t __tick;                                             \
		if (!xPortIsInISR())                                           \
			__tick = xTaskGetTickCount();                          \
		else                                                           \
			__tick = xTaskGetTickCountFromISR();                   \
		__tick;                                                        \
	})

#define jiffies_to_msecs(x) (x)

#define time_after(a,b)		\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)((b) - (a)) < 0))
#define time_before(a,b)	time_after(b,a)

#define time_after_eq(a,b)	\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)((a) - (b)) >= 0))
#define time_before_eq(a,b)	time_after_eq(b,a)

/*
 * Calculate whether a is in the range of [b, c].
 */
#define time_in_range(a,b,c) \
	(time_after_eq(a,b) && \
	 time_before_eq(a,c))

/*
 * Calculate whether a is in the range of [b, c).
 */
#define time_in_range_open(a,b,c) \
	(time_after_eq(a,b) && \
	 time_before(a,c))

/*
 * These four macros compare jiffies and 'a' for convenience.
 */

/* time_is_before_jiffies(a) return true if a is before jiffies */
#define time_is_before_jiffies(a) time_after(jiffies, a)

/* time_is_after_jiffies(a) return true if a is after jiffies */
#define time_is_after_jiffies(a) time_before(jiffies, a)

/* time_is_before_eq_jiffies(a) return true if a is before or equal to jiffies*/
#define time_is_before_eq_jiffies(a) time_after_eq(jiffies, a)

/* time_is_after_eq_jiffies(a) return true if a is after or equal to jiffies*/
#define time_is_after_eq_jiffies(a) time_before_eq(jiffies, a)

static inline unsigned long usecs_to_jiffies(const unsigned int u)
{
	return (u + (USEC_PER_SEC / configTICK_RATE_HZ) - 1) /
	       (USEC_PER_SEC / configTICK_RATE_HZ);
}

static inline unsigned long msecs_to_jiffies(const unsigned int m)
{
	return (m + (MSEC_PER_SEC / configTICK_RATE_HZ) - 1) /
	       (MSEC_PER_SEC / configTICK_RATE_HZ);
}

#endif
