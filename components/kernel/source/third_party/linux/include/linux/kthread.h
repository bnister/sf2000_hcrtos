/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KTHREAD_H
#define _LINUX_KTHREAD_H

#include <stdarg.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct task_struct;

#define current (struct task_struct *)xTaskGetCurrentTaskHandle()

struct task_struct *__kthread_create_on_node(int (*threadfn)(void *data),
					   int stacksz,
					   unsigned long prio,
					   const char * const name,
					   void *data);
struct task_struct *kthread_create_on_node(int (*threadfn)(void *data),
					   void *data,
					   const char namefmt[],
					   ...);

#define kthread_create(threadfn, data, namefmt, arg...) \
	kthread_create_on_node(threadfn, data, namefmt, ##arg)

int kthread_stop(struct task_struct *k);

#define kthread_run(threadfn, data, namefmt, ...)			   \
({									   \
	struct task_struct *__k						   \
		= kthread_create(threadfn, data, namefmt, ## __VA_ARGS__); \
	__k;								   \
})

bool kthread_should_stop(void);

#endif /* _LINUX_KTHREAD_H */
