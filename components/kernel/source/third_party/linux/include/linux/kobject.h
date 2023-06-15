/*
 * kobject.h - generic kernel object infrastructure.
 *
 * Copyright (c) 2002-2003 Patrick Mochel
 * Copyright (c) 2002-2003 Open Source Development Labs
 * Copyright (c) 2006-2008 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (c) 2006-2008 Novell Inc.
 *
 * This file is released under the GPLv2.
 *
 * Please read Documentation/kobject.txt before using the kobject
 * interface, ESPECIALLY the parts about reference counts and object
 * destructors.
 */

#ifndef _KOBJECT_H_
#define _KOBJECT_H_

#include <linux/types.h>
#include <linux/list.h>
#include <linux/compiler.h>
#include <linux/spinlock.h>
#include <linux/kref.h>
//#include <linux/kobject_ns.h>
//#include <linux/kernel.h>
#include <linux/wait.h>
//#include <linux/atomic.h>
#include <linux/workqueue.h>

struct kobj_type;
struct kset;

#define UEVENT_HELPER_PATH_LEN		256
#define UEVENT_NUM_ENVP			32	/* number of env pointers */
#define UEVENT_BUFFER_SIZE		2048	/* buffer for the variables */

/*
 * The actions here must match the index to the string array
 * in lib/kobject_uevent.c
 *
 * Do not add new actions here without checking with the driver-core
 * maintainers. Action strings are not meant to express subsystem
 * or device specific properties. In most cases you want to send a
 * kobject_uevent_env(kobj, KOBJ_CHANGE, env) with additional event
 * specific variables added to the event environment.
 */
enum kobject_action {
	KOBJ_ADD,
	KOBJ_REMOVE,
	KOBJ_CHANGE,
	KOBJ_MOVE,
	KOBJ_ONLINE,
	KOBJ_OFFLINE,
	KOBJ_MAX
};

struct kobject {
	const char		*name;
	struct list_head	entry;
	struct kobject		*parent;
	struct kset		*kset;
	struct kobj_type	*ktype;
	struct kref		kref;
	unsigned int state_initialized:1;
	unsigned int state_in_sysfs:1;
	unsigned int state_add_uevent_sent:1;
	unsigned int state_remove_uevent_sent:1;
	unsigned int uevent_suppress:1;
};

struct kset {
	struct list_head list;
	spinlock_t list_lock;
	struct kobject kobj;
	const struct kset_uevent_ops *uevent_ops;
};

struct kobj_type {
	void (*release)(struct kobject *kobj);
};

struct kobj_uevent_env {
	char *argv[3];
	char *envp[UEVENT_NUM_ENVP];
	int envp_idx;
	char buf[UEVENT_BUFFER_SIZE];
	int buflen;
};

struct kset_uevent_ops {
	int (* const filter)(struct kset *kset, struct kobject *kobj);
	const char *(* const name)(struct kset *kset, struct kobject *kobj);
	int (* const uevent)(struct kset *kset, struct kobject *kobj,
		      struct kobj_uevent_env *env);
};

static inline int kobject_uevent(struct kobject *kobj, enum kobject_action action) { return 0; }
static inline int kobject_uevent_env(struct kobject *kobj, enum kobject_action action,
			char *envp[]) { return 0; }
extern void kobject_del(struct kobject *kobj);
extern void kobject_put(struct kobject *kobj);
extern int kset_register(struct kset *kset);
extern void kset_unregister(struct kset *kset);

extern struct kobject *kset_find_obj(struct kset *, const char *);
extern struct kobject *kobject_get(struct kobject *kobj);
extern void kobject_init(struct kobject *kobj, struct kobj_type *ktype);
int kobject_init_and_add(struct kobject *kobj,
			 struct kobj_type *ktype, struct kobject *parent,
			 const char *fmt, ...);

static inline struct kobj_type *get_ktype(struct kobject *kobj)
{
	return kobj->ktype;
}

static inline const char *kobject_name(const struct kobject *kobj)
{
	return kobj->name;
}

static inline void kset_put(struct kset *k)
{
	kobject_put(&k->kobj);
}

static inline struct kset *to_kset(struct kobject *kobj)
{
	return kobj ? container_of(kobj, struct kset, kobj) : NULL;
}

static inline struct kset *kset_get(struct kset *k)
{
	return k ? to_kset(kobject_get(&k->kobj)) : NULL;
}

int kobject_set_name_vargs(struct kobject *kobj, const char *fmt, va_list vargs);
int kobject_set_name(struct kobject *kobj, const char *name, ...);
void kset_init(struct kset *kset);
int kobject_add(struct kobject *kobj, struct kobject *parent,
		const char *fmt, ...);
extern struct kset * __must_check kset_create_and_add(const char *name,
						const struct kset_uevent_ops *u,
						struct kobject *parent_kobj);

#endif /* _KOBJECT_H_ */
