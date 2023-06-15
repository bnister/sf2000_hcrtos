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

#ifndef PORTMACRO_H
#define	PORTMACRO_H

/* Include system headers */
#include <mips/cpu.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

#include <generated/br2_autoconf.h>

#ifdef	__cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------
 * Port specific definitions.
 *
 * The settings in this file configure FreeRTOS correctly for the
 * given hardware and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */
#define portCHAR                char
#define portFLOAT               float
#define portDOUBLE              double
#define portLONG                long
#define portSHORT               short
#define portSTACK_TYPE          uint32_t
#define portBASE_TYPE           long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

/* We'll use 32-bit ticks for efficiency */
typedef uint32_t TickType_t;
#define portMAX_DELAY           ( TickType_t ) 0xffffffffUL
#define portTICK_PERIOD_MS   ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
/*-----------------------------------------------------------*/

/* Hardware specifics. */
#define portBYTE_ALIGNMENT		8
#define portSTACK_GROWTH		-1
//#define configCHECK_FOR_STACK_OVERFLOW	3
/*-----------------------------------------------------------*/

/* Critical section management is done in the crit_sect.h file for the
interrupt mechanism being used; similarly, interrupt enable and disable are
also defined in that file. The directory for the interrupt mechanism MUST be
on the include path, things will not compile. */
#include "crit_task.h"
#include "int_handler.h"

/* Task function macros as described on the FreeRTOS.org WEB site. */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )  \
			void vFunction( void *pvParameters ) __attribute__((noreturn))
#define portTASK_FUNCTION( vFunction, pvParameters )        \
			void vFunction( void *pvParameters )
#define portHSRFUNCTION( vFunction, pvParameters )        \
			void vFunction( uint32_t pvParameters )
/*-----------------------------------------------------------*/

#define portEND_SWITCHING_ISR( xSwitchRequired )	\
if( xSwitchRequired )								\
{                                                   \
	portYIELD();                                    \
}
#define portYIELD_FROM_ISR( x ) portEND_SWITCHING_ISR( x )

#define portMEMORY_BARRIER()                                                   \
	do {                                                                   \
		asm volatile("" : : : "memory");                               \
	} while (0)

/* Required by the kernel aware debugger. */
#ifdef __DEBUG__
	#define portREMOVE_STATIC_QUALIFIER
#endif

enum
{
	portPRI_TASK_NORMAL	= CONFIG_TASK_NORMAL_PRIORITY,
	portPRI_TASK_HIGH	= CONFIG_TASK_HIGH_PRIORITY,
	portPRI_TASK_CRITICL	= CONFIG_TASK_CRITICAL_PRIORITY,
};

void portClearTickTimerInterrupt( void );
#define configCLEAR_TICK_TIMER_INTERRUPT() portClearTickTimerInterrupt()

extern void vApplicationSetupTickTimerInterrupt( void );
extern void vPortCacheFlush(void *pvAddr, uint32_t usByteLen);
extern void vPortCacheFlushAll(void);
extern void vPortCacheInvalidate(void *pvAddr, uint32_t usByteLen);
extern unsigned long vPortGetSatatisticsTimerValue(void);
extern void vPortPlatformTimeSetup(void);
extern void vPortPlatformTimeSet(time_t sec, long usec);
extern void vPortPlatformTimeGet(time_t *sec, long *usec);
uint64_t uxPortTimerHrTickGet(void);
void vPortDeleteThread( void *pvThreadToDelete );
void vPortSetupThread( void *pvThreadToSetup );
#define portCLEAN_UP_TCB( pxTCB )       vPortDeleteThread( pxTCB )
#define portSETUP_TCB( pxTCB )          vPortSetupThread( pxTCB )

#ifdef	__cplusplus
}
#endif

#endif	/* PORTMACRO_H */

