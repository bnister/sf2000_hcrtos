/****************************************************************************
 * include/nuttx/semaphore.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_SEMAPHORE_H
#define __INCLUDE_NUTTX_SEMAPHORE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <kernel/module.h>

#define sem_t SemaphoreHandle_t
#define nxsem_post(s)                                                          \
	do {                                                                   \
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;                 \
		if (uxInterruptNesting != 0) {                                 \
			xSemaphoreGiveFromISR(*(s),                            \
					      &xHigherPriorityTaskWoken);      \
		} else {                                                       \
			xSemaphoreGive(*(s));                                  \
		}                                                              \
		if (xHigherPriorityTaskWoken != pdFALSE) {                     \
			portYIELD();                                           \
		}                                                              \
	} while (0)

#define nxsem_init(s, pshared, value)                                          \
	({                                                                     \
		int __retval = OK;                                             \
		assert(pshared == 0);                                          \
		*(s) = xSemaphoreCreateCounting((value) == 0 ? 1 : (value),    \
						value);                        \
		if (*(s) == NULL) {                                            \
			__retval = -EINVAL;                                    \
		} else {                                                       \
			__retval = OK;                                         \
		}                                                              \
		__retval;                                                      \
	})

#define nxsem_wait_uninterruptible(s)                                          \
	({                                                                     \
		xSemaphoreTake(*(s), portMAX_DELAY);                           \
		OK;                                                            \
	})

#define nxsem_wait(s) nxsem_wait_uninterruptible(s)

#define nxsem_destroy(s) vSemaphoreDelete(*(s))

#define SEM_INITIALIZER(semname, count)                                        \
	sem_t semname;                                                         \
	static int semname##_initialize(void)                                  \
	{                                                                      \
		semname = xSemaphoreCreateCounting((count) == 0 ? 1 : (count), \
						   count);                     \
		return 0;                                                      \
	}                                                                      \
	module_core(semname, semname##_initialize, NULL, 0);

#define nxsem_set_protocol(sem, protocol)

#endif /* __INCLUDE_NUTTX_SEMAPHORE_H */
