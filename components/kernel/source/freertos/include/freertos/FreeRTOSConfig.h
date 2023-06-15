/*
	FreeRTOS V8.1.2 - Copyright (C) 2014 Real Time Engineers Ltd.
	All rights reserved

	VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

	***************************************************************************
	 *                                                                       *
	 *    FreeRTOS provides completely free yet professionally developed,    *
	 *    robust, strictly quality controlled, supported, and cross          *
	 *    platform software that has become a de facto standard.             *
	 *                                                                       *
	 *    Help yourself get started quickly and support the FreeRTOS         *
	 *    project by purchasing a FreeRTOS tutorial book, reference          *
	 *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
	 *                                                                       *
	 *    Thank you!                                                         *
	 *                                                                       *
	***************************************************************************

	This file is part of the FreeRTOS distribution.

	FreeRTOS is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License (version 2) as published by the
	Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

	>>!   NOTE: The modification to the GPL is included to allow you to     !<<
	>>!   distribute a combined work that includes FreeRTOS without being   !<<
	>>!   obliged to provide the source code for proprietary components     !<<
	>>!   outside of the FreeRTOS kernel.                                   !<<

	FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE.  Full license text is available from the following
	link: http://www.freertos.org/a00114.html

	1 tab == 4 spaces!

	***************************************************************************
	 *                                                                       *
	 *    Having a problem?  Start by reading the FAQ "My application does   *
	 *    not run, what could be wrong?"                                     *
	 *                                                                       *
	 *    http://www.FreeRTOS.org/FAQHelp.html                               *
	 *                                                                       *
	***************************************************************************

	http://www.FreeRTOS.org - Documentation, books, training, latest versions,
	license and Real Time Engineers Ltd. contact details.

	http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
	including FreeRTOS+Trace - an indispensable productivity tool, a DOS
	compatible FAT file system, and our tiny thread aware UDP/IP stack.

	http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
	Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
	licenses offer ticketed support, indemnification and middleware.

	http://www.SafeRTOS.com - High Integrity Systems also provide a safety
	engineered and independently SIL3 certified version for use in safety and
	mission critical applications that require provable dependability.

	1 tab == 4 spaces!
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <generated/br2_autoconf.h>
#ifndef __ASSEMBLER__
#include <cpu_func.h>
#define CONFIG_CPU_CLOCK_HZ get_cpu_clock()
#endif
#define CONFIG_TIMER_CLOCK_HZ (27000000/16)

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define configTASK_STACK_DEPTH				( CONFIG_TASK_STACK_SIZE )
#define configUSE_PREEMPTION				1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION		1
#if defined(CONFIG_TASK_OPTIMISED_SCHEDULER)
#define configUSE_PORT_OPTIMISED_SCHEDULER		1
#else
#define configUSE_PORT_OPTIMISED_SCHEDULER		0
#endif
#define configCPU_CLOCK_HZ				( CONFIG_CPU_CLOCK_HZ )
#define configTIMER_RATE_HZ				( ( TickType_t ) CONFIG_TIMER_CLOCK_HZ )
#define configTICK_RATE_HZ				( CONFIG_TICK_RATE_HZ )
#define configUSE_16_BIT_TICKS				0
#define configMAX_PRIORITIES				( CONFIG_TASK_MAX_PRIORITIES )
#define configMINIMAL_STACK_SIZE			( CONFIG_MINIMAL_TASK_STACK_SIZE )
#define configISR_STACK_SIZE				( CONFIG_ISR_STACK_SIZE )
//#define configTOTAL_HEAP_SIZE				( ( size_t ) CONFIG_HEAP_SIZE/2 )
#define configMAX_TASK_NAME_LEN				( CONFIG_MAX_TASK_NAME_LEN )

/* Hook functions */
#define configUSE_IDLE_HOOK				1
#define configUSE_TICK_HOOK				0

#define configUSE_TIMERS				1
#define configUSE_COUNTING_SEMAPHORES			1
#define configTIMER_TASK_PRIORITY			( CONFIG_TIMER_TASK_PRIORITY )
#define configTIMER_QUEUE_LENGTH			( CONFIG_TIMER_QUEUE_LENGTH )
#define configTIMER_TASK_STACK_DEPTH			( configMINIMAL_STACK_SIZE * 8 )

#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 1

#define configTASK_NOTIFICATION_ARRAY_ENTRIES		(CONFIG_TASK_NOTIFICATION_ARRAY_ENTRIES + 3)
#define configTASK_NOTIFICATION_POLL			(configTASK_NOTIFICATION_ARRAY_ENTRIES - 1)
#define configTASK_NOTIFICATION_RPC			(configTASK_NOTIFICATION_ARRAY_ENTRIES - 2)
#define configTASK_NOTIFICATION_COMPLETION		(configTASK_NOTIFICATION_ARRAY_ENTRIES - 3)
#define configUSE_POSIX_ERRNO				1

/* Co routines */
#define configUSE_CO_ROUTINES				0

/* The interrupt priority of the RTOS kernel */
#define configKERNEL_INTERRUPT_PRIORITY			0x01

/* The maximum priority from which API functions can be called */
#define configMAX_API_CALL_INTERRUPT_PRIORITY		0x03
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS		5

#define configTHREAD_LOCAL_STORAGE_POINTER_IDX_KTHREAD	0
#define configTHREAD_LOCAL_STORAGE_POINTER_IDX_WORK	1
#define configTHREAD_LOCAL_STORAGE_POINTER_IDX_WORKQUEUE	2
#define configTHREAD_LOCAL_STORAGE_POINTER_IDX_LWIPNETCONNSEM	3
#define configTHREAD_LOCAL_STORAGE_POINTER_IDX_PTHREADTLS	4

//#define configUSE_NEWLIB_REENTRANT  			1


/* Prevent assert code from being used in assembly files */
#ifndef __ASSEMBLER__
#include <assert.h>
#define configASSERT(x) assert(x)

struct __cpustats {
	const char	*name;
	unsigned int    runtime;
	unsigned int    tmp;
};

#endif

#if  defined(CONFIG_TASK_PERFORMANCE)
/* use for tasks performance statistics */
#define configGENERATE_RUN_TIME_STATS 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1
#define configUSE_TRACE_FACILITY 1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()	do{}while(0)
#define portGET_RUN_TIME_COUNTER_VALUE		vPortGetSatatisticsTimerValue

#ifndef __ASSEMBLER__
#define __cpustats_entry __attribute__((__used__)) __attribute__ ((section(".cpustats.entry")))
#define CPUSTATS_ENTRY(_name)                                                  \
	static __cpustats_entry struct __cpustats _name = { .name = #_name,    \
							    .runtime = 0,      \
							    .tmp = 0 };

#define CPUSTATS_ENTER(_name)                                                  \
	({                                                                     \
		taskENTER_CRITICAL();                                          \
		(_name).tmp = vPortGetSatatisticsTimerValue();                 \
	})

#define CPUSTATS_EXIT(_name)                                                   \
	({                                                                     \
		unsigned int ____tmp = vPortGetSatatisticsTimerValue();        \
		if (____tmp > (_name).tmp) {                                   \
			(_name).runtime += ____tmp - (_name).tmp;              \
		}                                                              \
		taskEXIT_CRITICAL();                                           \
	})
#endif

#else
#define CPUSTATS_ENTRY(...)
#define CPUSTATS_ENTER(...)
#define CPUSTATS_EXIT(...)
#endif

#if  defined(CONFIG_GENERATE_DETAIL_ISR_RUN_TIME_STATS)
#define configGENERATE_DETAIL_ISR_RUN_TIME_STATS 1
#else
#define configGENERATE_DETAIL_ISR_RUN_TIME_STATS 0
#endif

/* Optional functions */
#define INCLUDE_uxTaskPriorityGet			1
#define INCLUDE_vTaskPrioritySet			1
#define INCLUDE_vTaskDelayUntil				1
#define INCLUDE_vTaskDelay				1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskSuspend				1
#define INCLUDE_xTaskGetCurrentTaskHandle		1
#define INCLUDE_xTimerPendFunctionCall			1
#define INCLUDE_xTaskAbortDelay				1

#if defined(ENABLE_TRACE)
#include "trace.h"
#endif

#define configSUPPORT_STATIC_ALLOCATION			1
#define configRECORD_STACK_HIGH_ADDRESS			1
#define configUSE_APPLICATION_TASK_TAG			1
#define INCLUDE_xSemaphoreGetMutexHolder		1

#define configSTACK_DEPTH_TYPE				uint32_t

#endif	/* FREERTOSCONFIG_H */
