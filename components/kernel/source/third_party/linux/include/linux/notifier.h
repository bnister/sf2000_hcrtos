/* SPDX-License-Identifier: GPL-2.0 */
/*
 *	Routines to manage notifier chains for passing status changes to any
 *	interested routines. We need this instead of hard coded call lists so
 *	that modules can poke their nose into the innards. The network devices
 *	needed them so here they are for the rest of you.
 *
 *				Alan Cox <Alan.Cox@linux.org>
 */
 
#ifndef _LINUX_NOTIFIER_H
#define _LINUX_NOTIFIER_H
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>

struct notifier_block;

typedef	int (*notifier_fn_t)(struct notifier_block *nb,
			unsigned long action, void *data);

struct notifier_block {
	notifier_fn_t notifier_call;
	struct notifier_block __rcu *next;
	int priority;
};

struct atomic_notifier_head {
	spinlock_t lock;
	struct notifier_block __rcu *head;
};

struct blocking_notifier_head {
	struct rw_semaphore rwsem;
	struct notifier_block __rcu *head;
};

#define BLOCKING_INIT_NOTIFIER_HEAD(name) do {	\
		init_rwsem(&(name)->rwsem);	\
		(name)->head = NULL;		\
	} while (0)

#define BLOCKING_NOTIFIER_INIT(name) {				\
		.rwsem = __RWSEM_INITIALIZER((name).rwsem),	\
		.head = NULL }

#define BLOCKING_NOTIFIER_HEAD(name)				\
	struct blocking_notifier_head name =			\
		BLOCKING_NOTIFIER_INIT(name)

extern int atomic_notifier_chain_register(struct atomic_notifier_head *nh,
		struct notifier_block *nb);

extern int blocking_notifier_chain_register(struct blocking_notifier_head *nh,
		struct notifier_block *nb);

extern int atomic_notifier_chain_unregister(struct atomic_notifier_head *nh,
		struct notifier_block *nb);
extern int blocking_notifier_chain_unregister(struct blocking_notifier_head *nh,
		struct notifier_block *nb);

extern int atomic_notifier_call_chain(struct atomic_notifier_head *nh,
		unsigned long val, void *v);
extern int blocking_notifier_call_chain(struct blocking_notifier_head *nh,
		unsigned long val, void *v);

extern int register_restart_handler(struct notifier_block *nb);
extern int unregister_restart_handler(struct notifier_block *nb);

#define NOTIFY_DONE		0x0000		/* Don't care */
#define NOTIFY_OK		0x0001		/* Suits me */
#define NOTIFY_STOP_MASK	0x8000		/* Don't call further */
#define NOTIFY_BAD		(NOTIFY_STOP_MASK|0x0002)
						/* Bad/Veto action */
/*
 * Clean way to return from the notifier and stop further calls.
 */
#define NOTIFY_STOP		(NOTIFY_OK|NOTIFY_STOP_MASK)

/* Encapsulate (negative) errno value (in particular, NOTIFY_BAD <=> EPERM). */
static inline int notifier_from_errno(int err)
{
	if (err)
		return NOTIFY_STOP_MASK | (NOTIFY_OK - err);

	return NOTIFY_OK;
}

/* Restore (negative) errno value from notify return value. */
static inline int notifier_to_errno(int ret)
{
	ret &= ~NOTIFY_STOP_MASK;
	return ret > NOTIFY_OK ? NOTIFY_OK - ret : 0;
}

/*
 *	Declared notifiers so far. I can imagine quite a few more chains
 *	over time (eg laptop power reset chains, reboot chain (to clean 
 *	device units up), device [un]mount chain, module load/unload chain,
 *	low memory chain, screenblank chain (for plug in modular screenblankers) 
 *	VC switch chains (for loadable kernel svgalib VC switch helpers) etc...
 */
 
/* CPU notfiers are defined in include/linux/cpu.h. */

/* netdevice notifiers are defined in include/linux/netdevice.h */

/* reboot notifiers are defined in include/linux/reboot.h. */

/* Hibernation and suspend events are defined in include/linux/suspend.h. */

/* Virtual Terminal events are defined in include/linux/vt.h. */

#define NETLINK_URELEASE	0x0001	/* Unicast netlink socket released */

/* Console keyboard events.
 * Note: KBD_KEYCODE is always sent before KBD_UNBOUND_KEYCODE, KBD_UNICODE and
 * KBD_KEYSYM. */
#define KBD_KEYCODE		0x0001 /* Keyboard keycode, called before any other */
#define KBD_UNBOUND_KEYCODE	0x0002 /* Keyboard keycode which is not bound to any other */
#define KBD_UNICODE		0x0003 /* Keyboard unicode */
#define KBD_KEYSYM		0x0004 /* Keyboard keysym */
#define KBD_POST_KEYSYM		0x0005 /* Called after keyboard keysym interpretation */

#endif /* _LINUX_NOTIFIER_H */
