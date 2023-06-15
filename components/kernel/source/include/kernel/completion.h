#ifndef __KERNEL_COMPLETION_H
#define __KERNEL_COMPLETION_H

#include <kernel/list.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct completion {
	unsigned int done;
	struct list_head task_list;
};

void init_completion(struct completion *x);
extern void wait_for_completion(struct completion *);
static inline int wait_for_completion_interruptible(struct completion *x)
{
	wait_for_completion(x);
	return 0;
}
extern unsigned long wait_for_completion_timeout(struct completion *x,
						 unsigned long timeout);
static inline long
wait_for_completion_interruptible_timeout(struct completion *x,
                                          unsigned long timeout)
{
	return (long)wait_for_completion_timeout(x, timeout);
}
extern bool completion_done(struct completion *x);
extern void complete(struct completion *);
extern void complete_all(struct completion *);

#endif
