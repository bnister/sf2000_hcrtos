/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mutexes: blocking mutual exclusion locks
 *
 * started by Ingo Molnar:
 *
 *  Copyright (C) 2004, 2005, 2006 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *
 * This file contains the main data structure and API definitions.
 */
#ifndef __LINUX_MUTEX_H
#define __LINUX_MUTEX_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <kernel/module.h>

struct mutex {
	StaticSemaphore_t sem;
};

#define mutex_init(mutex) xSemaphoreCreateMutexStatic((StaticQueue_t *)(mutex))

#define mutex_destroy(mutex)                                                   \
	do {                                                                   \
	} while (0)

#define DEFINE_MUTEX(mutexname)                                                \
	struct mutex mutexname;                                                \
	static int mutexname##_initialize(void)                                \
	{                                                                      \
		xSemaphoreCreateMutexStatic((StaticQueue_t *)(&mutexname));    \
		return 0;                                                      \
	}                                                                      \
	module_core(mutexname, mutexname##_initialize, NULL, 0);

#define mutex_lock(lock)                                                       \
	({                                                                     \
		int ___ret = 0;                                                 \
		xSemaphoreTake((QueueHandle_t)(lock), portMAX_DELAY);          \
		___ret;                                                        \
	})

#define mutex_unlock(lock)                                                     \
	({                                                                     \
		int ___ret = 0;                                                \
		xSemaphoreGive((QueueHandle_t)(lock));                         \
		___ret;                                                        \
	})

#define mutex_trylock(lock) (xSemaphoreTake((QueueHandle_t)(lock), 0) == pdTRUE)
#define mutex_lock_interruptible(lock) mutex_lock(lock)

#endif /* __LINUX_MUTEX_H */
