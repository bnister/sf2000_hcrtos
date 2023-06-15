/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2008 Intel Corporation
 * Author: Matthew Wilcox <willy@linux.intel.com>
 *
 * Please see kernel/locking/semaphore.c for documentation of these functions
 */
#ifndef __LINUX_SEMAPHORE_H
#define __LINUX_SEMAPHORE_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <kernel/module.h>

struct semaphore {
	StaticSemaphore_t sem;
};

#define DEFINE_SEMAPHORE(name)                                                 \
	struct semaphore name;                                                 \
	static int name##_initialize(void)                                     \
	{                                                                      \
		xSemaphoreCreateCountingStatic(1, 1,                           \
					       (StaticQueue_t *)(&name));      \
		return 0;                                                      \
	}                                                                      \
	module_core(name, name##_initialize, NULL, 0);

#define sema_init(sem, val)                                                    \
	xSemaphoreCreateCountingStatic((val == 0) ? 1 : val, val,              \
				       (StaticQueue_t *)(sem))

#define down(sem) ({ xSemaphoreTake((QueueHandle_t)(sem), portMAX_DELAY); })

#define down_interruptible(sem)                                                \
	({                                                                     \
		int ___ret = 0;                                                \
		down(sem);                                                     \
		___ret;                                                        \
	})

#define down_timeout(sem, jiffies)                                             \
	({                                                                     \
		int ___ret = 0;                                                \
		if (xSemaphoreTake((QueueHandle_t)(sem), jiffies) == pdTRUE)   \
			___ret = 0;                                            \
		else                                                           \
			___ret = -ETIME;                                       \
		___ret;                                                        \
	})

#define up(sem)                                                                \
	do {                                                                   \
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;                 \
		if (uxInterruptNesting != 0) {                                 \
			xSemaphoreGiveFromISR((QueueHandle_t)(sem),            \
					      &xHigherPriorityTaskWoken);      \
		} else {                                                       \
			xSemaphoreGive((QueueHandle_t)(sem));                  \
		}                                                              \
		if (xHigherPriorityTaskWoken != pdFALSE) {                     \
			portYIELD();                                           \
		}                                                              \
	} while (0)

#define down_trylock(sem)                                                      \
	(xSemaphoreTake((QueueHandle_t)(sem), (TickType_t)0) != pdTRUE)

#endif /* __LINUX_SEMAPHORE_H */
