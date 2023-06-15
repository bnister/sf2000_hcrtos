#ifndef _LINUX_PROPERTY_H_
#define _LINUX_PROPERTY_H_

#include <linux/types.h>
#include <kernel/lib/fdt_api.h>
#include <linux/device.h>

static inline int device_property_read_string(struct device *dev, const char *propname,
				const char **val)
{
	return fdt_get_property_string_index((int)dev->of_node, propname, 0, val);
}

static inline int device_property_read_u32(struct device *dev,
					   const char *propname, u32 *val)
{
	return fdt_get_property_u_32_index((int)dev->of_node, propname, 0, val);
}

#endif /* _LINUX_PROPERTY_H_ */
