#ifndef _LINUX_SUSPEND_H
#define _LINUX_SUSPEND_H

#include <linux/notifier.h>
#include <linux/pm.h>

static inline int register_pm_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int unregister_pm_notifier(struct notifier_block *nb)
{
	return 0;
}

#endif /* _LINUX_SUSPEND_H */
