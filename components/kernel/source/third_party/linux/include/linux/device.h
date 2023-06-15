// SPDX-License-Identifier: GPL-2.0
/*
 * device.h - generic, centralized driver model
 *
 * Copyright (c) 2001-2003 Patrick Mochel <mochel@osdl.org>
 * Copyright (c) 2004-2009 Greg Kroah-Hartman <gregkh@suse.de>
 * Copyright (c) 2008-2009 Novell Inc.
 *
 * See Documentation/driver-api/driver-model/ for more information.
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <stdarg.h>
#include <linux/device/bus.h>
#include <linux/device/driver.h>
#include <linux/mutex.h>
#include <linux/kobject.h>
#include <linux/ioport.h>
#include <linux/bug.h>
#include <linux/pm.h>
#include <linux/slab.h>

struct device;
struct device_private;
struct device_node;

struct device_type {
	const char *name;
	int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
	void (*release)(struct device *dev);
};

struct device {
	struct device		*parent;

	struct device_private	*p;

	struct device_node      *of_node;

	struct kobject kobj;
	const char		*init_name; /* initial name of the device */
	const struct device_type *type;

	struct bus_type	*bus;		/* type of bus device is on */
	struct device_driver *driver;	/* which driver has allocated this
					   device */
	void		*platform_data;	/* Platform specific data, device
					   core doesn't touch it */
	void		*driver_data;	/* Driver data, set and get with
					   dev_set_drvdata/dev_get_drvdata */
	struct mutex		mutex;	/* mutex to synchronize calls to
					 * its driver.
					 */

	struct list_head        devres_head;

	struct dev_pm_info	power;

	u64		*dma_mask;	/* dma mask (if dma'able device) */
	u64		coherent_dma_mask;/* Like dma_mask, but for
					     alloc_coherent mappings as
					     not all hardware supports
					     64 bit addresses for consistent
					     allocations such descriptors. */
	void	(*release)(struct device *dev);
};

int dev_set_name(struct device *dev, const char *name, ...);

static inline struct device *kobj_to_dev(struct kobject *kobj)
{
	return container_of(kobj, struct device, kobj);
}

static inline const char *dev_name(const struct device *dev)
{
	/* Use the init name until the kobject becomes available */
	if (dev->init_name)
		return dev->init_name;

	return kobject_name(&dev->kobj);
}

static inline void *dev_get_drvdata(const struct device *dev)
{
	return dev->driver_data;
}

static inline void dev_set_drvdata(struct device *dev, void *data)
{
	dev->driver_data = data;
}

struct device *get_device(struct device *dev);
extern int __must_check device_add(struct device *dev);
void put_device(struct device *dev);
extern int __must_check device_register(struct device *dev);
extern void device_unregister(struct device *dev);

static inline void *dev_get_platdata(const struct device *dev)
{
	return dev->platform_data;
}

#define dev_WARN(dev, format, arg...) \
	WARN(1, "%s: " format, dev_name(dev), ## arg);

#define dev_WARN_ONCE(dev, condition, format, arg...) \
	WARN_ONCE(condition, "%s: " format, \
			dev_name(dev), ## arg)

#define linux_module_driver(__driver, __register, __unregister, ...) \
static int __init __driver##_init(void) \
{ \
	return __register(&(__driver) , ##__VA_ARGS__); \
} \
static int __exit __driver##_exit(void) \
{ \
	__unregister(&(__driver) , ##__VA_ARGS__); \
	return 0; \
} \
module_driver(__driver, __driver##_init, __driver##_exit, 0)

#define builtin_driver(__driver, __register, ...) \
static int __init __driver##_init(void) \
{ \
	return __register(&(__driver) , ##__VA_ARGS__); \
} \
module_driver(__driver, __driver##_init, NULL, 0)

extern void device_del(struct device *dev);

static inline void device_lock(struct device *dev)
{
	mutex_lock(&dev->mutex);
}

static inline int device_trylock(struct device *dev)
{
	return mutex_trylock(&dev->mutex);
}

static inline void device_unlock(struct device *dev)
{
	mutex_unlock(&dev->mutex);
}

extern int __must_check device_bind_driver(struct device *dev);
extern int __must_check driver_attach(struct device_driver *drv);
extern int  __must_check device_attach(struct device *dev);
extern void device_release_driver(struct device *dev);
extern void device_initialize(struct device *dev);
extern void device_initial_probe(struct device *dev);
void __iomem *devm_ioremap_resource(struct device *dev, struct resource *res);

static inline int device_is_registered(struct device *dev)
{
	return dev->kobj.state_in_sysfs;
}

extern void wait_for_device_probe(void);

static inline int dev_to_node(struct device *dev)
{
	return -1;
}
static inline void set_dev_node(struct device *dev, int node)
{
}

static inline void pm_suspend_ignore_children(struct device *dev, bool enable)
{
	dev->power.ignore_children = enable;
}

static inline void device_enable_async_suspend(struct device *dev)
{
	if (!dev->power.is_prepared)
		dev->power.async_suspend = true;
}

typedef void (*dr_release_t)(struct device *dev, void *dr);
typedef int (*dr_match_t)(struct device *dev, void *res, void *match_data);
extern int devres_release_all(struct device *dev);
extern void devres_add(struct device *dev, void *dr);
extern void *devres_alloc(dr_release_t release, size_t size, gfp_t gfp);
extern int devres_destroy(struct device *dev, dr_release_t release,
			  dr_match_t match, void *match_data);

#define devres_free(dr) kfree(dr)

#define device_remove_file(...) (0)
#define device_create_file(...) (0)
#define sysfs_attr_init(...) (0)

#endif /* _DEVICE_H_ */
