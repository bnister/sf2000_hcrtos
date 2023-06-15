/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PREEMPT_H
#define __LINUX_PREEMPT_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define in_interrupt()		(xPortIsInISR())
#define preemptible()	(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING && !in_interrupt())

#endif /* __LINUX_PREEMPT_H */
