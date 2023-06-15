/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_EXPORT_H
#define _LINUX_EXPORT_H

#define EXPORT_SYMBOL(sym)
#define EXPORT_SYMBOL_GPL(sym)

#define THIS_MODULE ((struct module *)0)
#define KBUILD_MODNAME ((const char *)0)

#endif /* _LINUX_EXPORT_H */
