#include <limits.h>
#include <linux/compiler.h>
#include <kernel/completion.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct swait_queue {
	void *task;
	struct list_head task_list;
};

void init_completion(struct completion *x)
{
	taskENTER_CRITICAL();
	x->done = 0;
	INIT_LIST_HEAD(&x->task_list);
	taskEXIT_CRITICAL();
}

unsigned long wait_for_completion_timeout(struct completion *x,
					  unsigned long timeout)
{
	TickType_t xTicksToWait = timeout;
	TimeOut_t xTimeOut;

	if (likely(!x->done)) {
		struct swait_queue wait;

		wait.task = xTaskGetCurrentTaskHandle();
		INIT_LIST_HEAD(&wait.task_list);

		vTaskSetTimeOutState(&xTimeOut);

		taskENTER_CRITICAL();
		if (likely(!x->done)) {
			list_add_tail(&wait.task_list, &x->task_list);
		}
		taskEXIT_CRITICAL();

		while (!x->done) {
			if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) != pdFALSE) {
				/* Timed out */
				taskENTER_CRITICAL();
				list_del_init(&wait.task_list);
				taskEXIT_CRITICAL();
				return 0;
			}

			xTaskNotifyWaitIndexed(
				configTASK_NOTIFICATION_COMPLETION, ULONG_MAX,
				pdTRUE, NULL, xTicksToWait);
		}

		taskENTER_CRITICAL();
		list_del_init(&wait.task_list);
		taskEXIT_CRITICAL();
	}

	assert(x->done != 0);
	if (x->done != UINT_MAX)
		x->done--;

	return 1;
}

void wait_for_completion(struct completion *x)
{
	while (!wait_for_completion_timeout(x, portMAX_DELAY))
		;
}


bool completion_done(struct completion *x)
{
	if (!x->done)
		return false;
	return true;
}

static BaseType_t swake_up_locked(struct completion *x)
{
	struct swait_queue *curr;
	BaseType_t yield = pdFALSE;

	if (list_empty(&x->task_list)) {
		return yield;
	}

	curr = list_first_entry(&x->task_list, typeof(*curr), task_list);
	if (curr->task != NULL) {
		if (uxInterruptNesting != 0) {
			vTaskNotifyGiveIndexedFromISR(
				(TaskHandle_t)curr->task,
				configTASK_NOTIFICATION_COMPLETION, &yield);
		} else {
			xTaskNotifyGiveIndexed(
				(TaskHandle_t)curr->task,
				configTASK_NOTIFICATION_COMPLETION);
		}
	}

	list_del_init(&curr->task_list);

	return yield;
}

static BaseType_t swake_up_all_locked(struct completion *x)
{
	BaseType_t yield = pdFALSE;

	while (!list_empty(&x->task_list)) {
		if (swake_up_locked(x) == pdTRUE)
			yield = pdTRUE;
	}

	return yield;
}

void complete(struct completion *x)
{
	taskENTER_CRITICAL();

	if (x->done != UINT_MAX)
		x->done++;

	if (swake_up_locked(x) == pdTRUE) {
		portYIELD();
	}

	taskEXIT_CRITICAL();
}

void complete_all(struct completion *x)
{
	taskENTER_CRITICAL();

	x->done = UINT_MAX;

	if (swake_up_all_locked(x) == pdTRUE) {
		portYIELD();
	}

	taskEXIT_CRITICAL();
}
