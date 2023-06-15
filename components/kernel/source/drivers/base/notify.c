#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/notifier.h>
#include <linux/mutex.h>

static BLOCKING_NOTIFIER_HEAD(sys_notifier_list);

void sys_register_notify(struct notifier_block *nb)
{
	blocking_notifier_chain_register(&sys_notifier_list, nb);
}

void sys_unregister_notify(struct notifier_block *nb)
{
	blocking_notifier_chain_unregister(&sys_notifier_list, nb);
}

void sys_notify_event(unsigned int evtype, void *v)
{
	blocking_notifier_call_chain(&sys_notifier_list, evtype, v);
}
