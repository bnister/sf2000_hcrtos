#ifndef _LINUX_OF_H
#define _LINUX_OF_H
/*
 * Definitions for talking to the Open Firmware PROM on
 * Power Macintosh and other computers.
 *
 * Copyright (C) 1996-2005 Paul Mackerras.
 *
 * Updates for PPC64 by Peter Bergner & David Engebretsen, IBM Corp.
 * Updates for SPARC64 by David S. Miller
 * Derived from PowerPC and Sparc prom.h files by Stephen Rothwell, IBM Corp.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/kobject.h>
#include <linux/mod_devicetable.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/property.h>
#include <linux/list.h>
#include <linux/compiler.h>
#include <kernel/lib/fdt_api.h>

struct device_node;
struct property;

#define of_match_ptr(_ptr)	(_ptr)

#define of_node_get(node) (node)
#define of_node_put(node)

#define for_each_child_of_node(parent, child) while(0)

extern int of_device_is_compatible(const struct device_node *device,
				   const char *);

#define of_property_read_u32(np, propname, out_value)                          \
	fdt_get_property_u_32_index((int)(np), propname, 0, out_value)

#define of_property_read_bool(np, propname)                                    \
	fdt_property_read_bool((int)(np), propname)

#define of_property_read_u32_array(np, propname, out_values, sz)               \
	fdt_get_property_u_32_array((int)(np), propname, out_values, sz)

#define of_alias_get_id(np, stem) fdt_get_property_alias_id((int)(np), stem)

#define of_property_count_strings(np, propname)                                \
	fdt_property_count_strings((int)(np), propname)

#define of_find_property(np, name, lenp)                                       \
	(struct property *)fdt_get_property_data_by_name((int)(np), name, lenp)

#define of_parse_phandle(np, phandle_name, index)                              \
	(struct device_node *)fdt_parse_phandle((int)(np), phandle_name, index)

#define of_get_property(np, name, lenp)                                        \
	fdt_get_property_data_by_name((int)(np), name, lenp)

#define of_property_read_string(np, propname, out_string)                      \
	fdt_get_property_string_index((int)(np), propname, 0, out_string)

static inline const struct of_device_id *
of_match_node(const struct of_device_id *matches,
	      const struct device_node *node)
{
	if (!matches)
		return NULL;
	for (; matches->name[0] || matches->type[0] || matches->compatible[0]; matches++) {
		if (of_device_is_compatible(node, matches->compatible))
			return matches;
	}

	return NULL;
}

void __iomem *of_iomap(struct device_node *node, int index);

#endif /* _LINUX_OF_H */
