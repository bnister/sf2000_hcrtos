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

#ifndef INT_HANDLER_H
#define	INT_HANDLER_H

#ifdef	__cplusplus
extern "C"
{
#endif

#include "portmacro.h"

/* Nicer names for the interrupt bits */
#define SW0			0x00
#define SW1			0x01
#define HW0			0x02
#define HW1			0x03
#define HW2			0x04
#define HW3			0x05
#define HW4			0x06
#define HW5			0x07

/* Where timer irq is wired as reported by hw */
#define EIRQ3_NUM		64
#define EIRQ2_NUM		32
#define EIRQ_NUM		(EIRQ2_NUM + EIRQ3_NUM)
#define SB_TIMER_IRQ		(8 + 22)		/*south brigde timer*/

#define IRQ_NUM			8
#define IRQ_EIRQ3_NUM		3
#define IRQ_EIRQ2_NUM		2

#define TIMER_IRQ		7
#define SR_TIMER_IRQ		0 /*(SR_IM0 << TIMER_IRQ)*/ /* Use south brigde timer instead of SR timer */

/* Initialise the GIC */
void vPortInterruptInit(void);

/* Interrupt manipulation */
extern int xPortInterruptInstallISR(uint32_t, void (*)(uint32_t), uint32_t);
int xPortInterruptInstallISR2(uint32_t irq, int (*fn)(int, void *), void *dev);
extern int xPortInterruptRemoveISR(uint32_t irq, void (*fn)(uint32_t));
void xPortInterruptDisable(uint32_t irq);
void xPortInterruptEnable(uint32_t irq);
extern BaseType_t xPortIsInISR(void);

/*
 * The software interrupt handler that performs the yield.
 */
extern void vPortInterruptYieldISR(uint32_t param);

extern UBaseType_t uxPortInterruptSetInterruptMaskFromISR( void );
extern void vPortInterruptClearInterruptMaskFromISR( UBaseType_t );
extern void vPortInterruptAppSetupSoftwareInterrupt( void );

#define portINITIAL_SR      ( SR_SINT0 | SR_EXL | SR_IE )
#define portCLEAR_PENDING_INTERRUPTS()      mips_bicsr( SR_IMASK )

#define portSET_INTERRUPT_MASK_FROM_ISR()   uxPortInterruptSetInterruptMaskFromISR()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedStatusRegister ) \
	vPortInterruptClearInterruptMaskFromISR( uxSavedStatusRegister )

#ifdef	__cplusplus
}
#endif

#endif	/* INT_HANDLER_H */

