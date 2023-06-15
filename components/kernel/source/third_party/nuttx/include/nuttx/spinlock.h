/****************************************************************************
 * include/nuttx/spinlock.h
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

#ifndef __INCLUDE_NUTTX_SPINLOCK_H
#define __INCLUDE_NUTTX_SPINLOCK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

typedef struct
{
} spinlock_t;

typedef unsigned long irqstate_t;

#define spin_lock_irqsave(l)                                                   \
	({                                                                     \
		taskENTER_CRITICAL();                                          \
	})
#define spin_unlock_irqrestore(l, f)                                           \
	do {                                                                   \
		(f) = 0;                                                       \
		taskEXIT_CRITICAL();                                           \
	} while (0)

#endif /* __INCLUDE_NUTTX_SPINLOCK_H */
