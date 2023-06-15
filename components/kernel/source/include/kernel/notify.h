#ifndef __KERNEL_NOTIFY_H
#define __KERNEL_NOTIFY_H

#include <linux/notifier.h>

void sys_register_notify(struct notifier_block *nb);
void sys_unregister_notify(struct notifier_block *nb);
void sys_notify_event(unsigned int evtype, void *v);

#endif
