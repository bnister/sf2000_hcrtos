/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TIMER_H
#define _LINUX_TIMER_H

#include <string.h>
#include <nuttx/wqueue.h>
#include <linux/jiffies.h>

struct timer_list {
	struct work_s		work;
	unsigned long		expires;
	void			(*function2)(struct timer_list *);
	void			(*function)(unsigned long);
	unsigned long		data;
};

#define timer_pending(t) ((t)->work.worker != NULL)
extern int del_timer(struct timer_list *timer);
extern int mod_timer(struct timer_list *timer, unsigned long expires);

#define from_timer(var, callback_timer, timer_fieldname) \
	container_of(callback_timer, typeof(*var), timer_fieldname)

#define timer_setup(timer, callback, flags)                                    \
	do {                                                                   \
		memset((timer), 0, sizeof(struct timer_list));                 \
		(timer)->function2 = callback;                                 \
	} while (0)

#define setup_timer(timer, fn, d)                                              \
	do {                                                                   \
		memset((timer), 0, sizeof(struct timer_list));                 \
		(timer)->function = (fn);                                      \
		(timer)->data = (d);                                           \
	} while (0)

#define del_timer_sync(t) del_timer(t)

#define init_timer(timer) memset((timer), 0, sizeof(struct timer_list))

#endif
