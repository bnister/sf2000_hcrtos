/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KERNEL_PRINTK__
#define __KERNEL_PRINTK__

#ifndef LOG_TAG
#define LOG_TAG "LNX"
#endif

#ifndef ELOG_OUTPUT_LVL
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR
#endif

#include <kernel/elog.h>
#include <stdio.h>

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define pr_emerg(fmt, ...) log_e(pr_fmt(fmt), ##__VA_ARGS__)

#define pr_alert(fmt, ...) log_w(pr_fmt(fmt), ##__VA_ARGS__)

#define pr_crit(fmt, ...) log_e(pr_fmt(fmt), ##__VA_ARGS__)

#define pr_err(fmt, ...) log_e(pr_fmt(fmt), ##__VA_ARGS__)

#define pr_warn(fmt, ...) log_w(pr_fmt(fmt), ##__VA_ARGS__)

#define pr_notice(fmt, ...) log_d(pr_fmt(fmt), ##__VA_ARGS__)

#define pr_info(fmt, ...) log_i(pr_fmt(fmt), ##__VA_ARGS__)

#define pr_cont(fmt, ...) log_d(fmt, ##__VA_ARGS__)

#define pr_debug(fmt, ...) log_d(fmt, ##__VA_ARGS__)

#define dev_dbg(dev, fmt, ...) log_d(fmt, ##__VA_ARGS__)

#define dev_err(dev, fmt, ...) log_e(fmt, ##__VA_ARGS__)

#define dev_warn_once(dev, fmt, ...) log_w(fmt, ##__VA_ARGS__)

#define dev_info(dev, fmt, ...) log_i(fmt, ##__VA_ARGS__)

#define dev_warn(dev, fmt, ...) log_w(fmt, ##__VA_ARGS__)

#define dev_printk(level, dev, fmt, ...) log_e(fmt, ##__VA_ARGS__)

#define pr_warn_once(fmt, ...) log_w(fmt, ##__VA_ARGS__)

#define dev_notice(dev, fmt, ...) log_i(fmt, ##__VA_ARGS__)

#define printk(fmt, ...)                                                       \
	do {                                                                   \
		elog_output_lock();                                            \
		printf(fmt, ##__VA_ARGS__);                                    \
		elog_output_unlock();                                          \
	} while (0)

#define KERN_EMERG
#define KERN_ERR
#define KERN_INFO
#define KERN_DEBUG
#define KERN_WARNING
#define KERN_CONT

static inline int printk_ratelimit(void)
{
	return 0;
}

#define printk_ratelimited(fmt, ...) log_e(fmt, ##__VA_ARGS__)

#define pr_warn_ratelimited(fmt, ...) log_w(fmt, ##__VA_ARGS__)

#define dev_vdbg(dev, fmt, ...) log_i(fmt, ##__VA_ARGS__)

#define seq_printf(s, fmt, ...) log_e(fmt, ##__VA_ARGS__)

#define dump_stack() do {} while(0)

#endif
