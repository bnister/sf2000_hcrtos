/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/linux/irqflags.h
 *
 * IRQ flags tracing: follow the state of the hardirq and softirq flags and
 * provide callbacks for transitions between ON and OFF states.
 *
 * This file gets included from lowlevel asm headers too, to provide
 * wrapped versions of the local_irq_*() APIs, based on the
 * raw_local_irq_*() macros from the lowlevel headers.
 */
#ifndef _LINUX_TRACE_IRQFLAGS_H
#define _LINUX_TRACE_IRQFLAGS_H

#include <kernel/irqflags.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <linux/typecheck.h>

/*
 * Wrap the arch provided IRQ routines to provide appropriate checks.
 */
#define raw_local_irq_disable()		taskENTER_CRITICAL()
#define raw_local_irq_enable()		taskEXIT_CRITICAL()
#define raw_local_irq_save(flags)	taskENTER_CRITICAL()
#define raw_local_irq_restore(flags)	taskEXIT_CRITICAL()
/*
 * The local_irq_*() APIs are equal to the raw_local_irq*()
 * if !TRACE_IRQFLAGS.
 */
#define local_irq_enable()	do { raw_local_irq_enable(); } while (0)
#define local_irq_disable()	do { raw_local_irq_disable(); } while (0)
#define local_irq_save(flags)	do { (void) flags; raw_local_irq_save(flags); } while (0)
#define local_irq_restore(flags) do { (void) flags; raw_local_irq_restore(flags); } while (0)

#endif
