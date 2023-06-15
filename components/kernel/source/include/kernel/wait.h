#ifndef __KERNEL_WAIT_H
#define __KERNEL_WAIT_H

#include <generated/br2_autoconf.h>
#include <kernel/list.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct file;
struct poll_table_struct;

typedef struct wait_queue_head {
	struct list_head task_list;
} wait_queue_head_t;

typedef void (*poll_queue_proc)(struct file *, wait_queue_head_t *, struct poll_table_struct *);
typedef struct poll_table_struct {
	void *_priv;
	poll_queue_proc _qproc;
	bool _is_notification_poll;
} poll_table;

struct poll_wqueues;
struct poll_table_entry {
	struct list_head task_list;
	struct list_head poll_list;
	struct poll_wqueues *wait;
};

struct poll_wqueues {
	poll_table pt;
	struct poll_table_entry
		inline_entry[CONFIG_INLINE_POLL_NFILE_DESCRIPTORS];
	int inline_entry_idx;
	struct list_head poll_list_head;
};

typedef struct poll_wqueues wait_queue_t;

#define init_waitqueue_head(q)                                                 \
	do {                                                                   \
		INIT_LIST_HEAD(&((q)->task_list));                             \
	} while (0)

#define __WAIT_QUEUE_HEAD_INITIALIZER(name)                                    \
	{                                                                      \
		.task_list = { &(name).task_list, &(name).task_list }          \
	}

#define DECLARE_WAIT_QUEUE_HEAD(name)                                          \
	struct wait_queue_head name = __WAIT_QUEUE_HEAD_INITIALIZER(name)

#define __WAITQUEUE_INITIALIZER(name, tsk)                                     \
	{                                                                      \
		.pt =                                                          \
			{                                                      \
				._priv = tsk,                                  \
				._qproc = NULL,                                \
				._is_notification_poll = false,                \
			},                                                     \
		.poll_list_head = {                                            \
			&(name).poll_list_head,                                \
			&(name).poll_list_head                                 \
		}                                                              \
	}

#define DECLARE_WAITQUEUE(name, tsk)                                           \
	wait_queue_t name = __WAITQUEUE_INITIALIZER(name, tsk)

extern void wake_up(wait_queue_head_t *wait_address);
extern void poll_wait(struct file *filp, wait_queue_head_t *wait_address, poll_table *p);

extern void poll_setup(struct poll_wqueues *q);
extern void destroy_poll_wait(struct poll_wqueues *wait);
extern void __pollwait(struct file *filp, wait_queue_head_t *wait_address, poll_table *p);

#define __wait_event_timeout(wq_head, condition, timeout)                      \
	({                                                                     \
		TickType_t __xTicksToWait = timeout;                           \
		TimeOut_t __xTimeOut;                                          \
		int __timed_out = 0;                                           \
		struct poll_wqueues __wait = { 0 };                            \
		long ___ret = 0;                                               \
                                                                               \
		vTaskSetTimeOutState(&__xTimeOut);                             \
                                                                               \
		poll_setup(&__wait);                                           \
		__pollwait(NULL, &(wq_head), &__wait.pt);                      \
                                                                               \
		while (!(condition)) {                                         \
			if (xTaskCheckForTimeOut(&__xTimeOut,                  \
						 &__xTicksToWait) !=           \
			    pdFALSE) {                                         \
				__timed_out = 1;                               \
				break;                                         \
			}                                                      \
			ulTaskNotifyTakeIndexed(configTASK_NOTIFICATION_POLL,  \
						pdTRUE, __xTicksToWait);       \
		}                                                              \
                                                                               \
		destroy_poll_wait(&__wait);                                    \
                                                                               \
		if (!(condition))                                              \
			___ret = 0;                                            \
		else if (__timed_out || !__xTicksToWait)                       \
			___ret = 1;                                            \
		else                                                           \
			___ret = __xTicksToWait;                               \
                                                                               \
		___ret;                                                        \
	})

#define ___wait_cond_timeout(condition)                                        \
	({                                                                     \
		bool __cond = (condition);                                     \
		if (__cond && !__ret)                                          \
			__ret = 1;                                             \
		__cond || !__ret;                                              \
	})

/**
 * wait_event_timeout - sleep until a condition gets true or a timeout elapses
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @timeout: timeout, in jiffies
 *
 * The process is put to suspend until the
 * @condition evaluates to true.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * Returns:
 * 0 if the @condition evaluated to %false after the @timeout elapsed,
 * 1 if the @condition evaluated to %true after the @timeout elapsed,
 * the remaining jiffies (at least 1) if the @condition evaluated
 * to %true before the @timeout elapsed
 */
#define wait_event_timeout(wq_head, condition, timeout)                        \
	({                                                                     \
		long __ret = timeout;                                          \
		if (!___wait_cond_timeout(condition))                          \
			__ret = __wait_event_timeout(wq_head, condition,       \
						     timeout);                 \
		__ret;                                                         \
	})

#define wait_event_interruptible_timeout(wq, condition, timeout)               \
	({                                                                     \
		long __ret = timeout;                                          \
		if (!___wait_cond_timeout(condition))                          \
			__ret = __wait_event_timeout(wq, condition, timeout);  \
		__ret;                                                         \
	})

#define wait_event(wq_head, condition)                                         \
	wait_event_timeout(wq_head, condition, portMAX_DELAY)

#define wait_event_interruptible(wq_head, condition) wait_event(wq_head, condition)

#define wake_up_interruptible(x) wake_up(x)

static inline void add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	__pollwait(NULL, q, &wait->pt);
}

static inline void remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	destroy_poll_wait(wait);
}

#endif
