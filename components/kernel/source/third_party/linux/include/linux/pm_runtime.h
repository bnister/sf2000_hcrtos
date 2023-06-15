/*
 * pm_runtime.h - Device run-time power management helper functions.
 *
 * Copyright (C) 2009 Rafael J. Wysocki <rjw@sisk.pl>
 *
 * This file is released under the GPLv2.
 */

#ifndef _LINUX_PM_RUNTIME_H
#define _LINUX_PM_RUNTIME_H

#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/pm.h>

#include <linux/jiffies.h>

/* Runtime PM flag argument bits */
#define RPM_ASYNC		0x01	/* Request is asynchronous */
#define RPM_NOWAIT		0x02	/* Don't wait for concurrent
					    state change */
#define RPM_GET_PUT		0x04	/* Increment/decrement the
					    usage_count */
#define RPM_AUTO		0x08	/* Use autosuspend_delay */

static inline bool queue_pm_work(struct work_struct *work) { return false; }

static inline int pm_generic_runtime_suspend(struct device *dev) { return 0; }
static inline int pm_generic_runtime_resume(struct device *dev) { return 0; }
static inline int pm_runtime_force_suspend(struct device *dev) { return 0; }
static inline int pm_runtime_force_resume(struct device *dev) { return 0; }

static inline int __pm_runtime_idle(struct device *dev, int rpmflags)
{
	return -ENOSYS;
}
static inline int __pm_runtime_suspend(struct device *dev, int rpmflags)
{
	return -ENOSYS;
}
static inline int __pm_runtime_resume(struct device *dev, int rpmflags)
{
	return 1;
}
static inline int pm_schedule_suspend(struct device *dev, unsigned int delay)
{
	return -ENOSYS;
}
static inline int __pm_runtime_set_status(struct device *dev,
					    unsigned int status) { return 0; }
static inline int pm_runtime_barrier(struct device *dev) { return 0; }
static inline void pm_runtime_enable(struct device *dev) {}
static inline void __pm_runtime_disable(struct device *dev, bool c) {}
static inline void pm_runtime_allow(struct device *dev) {}
static inline void pm_runtime_forbid(struct device *dev) {}

static inline bool pm_children_suspended(struct device *dev) { return false; }
static inline void pm_runtime_get_noresume(struct device *dev) {}
static inline void pm_runtime_put_noidle(struct device *dev) {}
static inline bool device_run_wake(struct device *dev) { return false; }
static inline void device_set_run_wake(struct device *dev, bool enable) {}
static inline bool pm_runtime_suspended(struct device *dev) { return false; }
static inline bool pm_runtime_active(struct device *dev) { return true; }
static inline bool pm_runtime_status_suspended(struct device *dev) { return false; }
static inline bool pm_runtime_enabled(struct device *dev) { return false; }

static inline void pm_runtime_no_callbacks(struct device *dev) {}
static inline void pm_runtime_irq_safe(struct device *dev) {}
static inline bool pm_runtime_is_irq_safe(struct device *dev) { return false; }

static inline bool pm_runtime_callbacks_present(struct device *dev) { return false; }
static inline void pm_runtime_mark_last_busy(struct device *dev) {}
static inline void __pm_runtime_use_autosuspend(struct device *dev,
						bool use) {}
static inline void pm_runtime_set_autosuspend_delay(struct device *dev,
						int delay) {}
static inline unsigned long pm_runtime_autosuspend_expiration(
				struct device *dev) { return 0; }
static inline void pm_runtime_set_memalloc_noio(struct device *dev,
						bool enable){}
static inline int pm_runtime_idle(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_suspend(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_autosuspend(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_resume(struct device *dev)
{
	return 0;
}

static inline int pm_request_idle(struct device *dev)
{
	return 0;
}

static inline int pm_request_resume(struct device *dev)
{
	return 0;
}

static inline int pm_request_autosuspend(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_get(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_get_sync(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_put(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_put_autosuspend(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_put_sync(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_put_sync_suspend(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_put_sync_autosuspend(struct device *dev)
{
	return 0;
}

static inline int pm_runtime_set_active(struct device *dev)
{
	return 0;
}

static inline void pm_runtime_set_suspended(struct device *dev)
{
}

static inline void pm_runtime_disable(struct device *dev)
{
}

static inline void pm_runtime_use_autosuspend(struct device *dev)
{
}

static inline void pm_runtime_dont_use_autosuspend(struct device *dev)
{
}

#endif
