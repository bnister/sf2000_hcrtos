/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _LINUX_IO_H
#define _LINUX_IO_H

#include <linux/types.h>
#include <asm-generic/io.h>

#ifndef readl_relaxed
#define readl_relaxed readl
#endif

#ifndef writel_relaxed
#define writel_relaxed writel
#endif

#endif /* _LINUX_IO_H */
