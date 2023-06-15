#ifndef __LINUX_SPINLOCK_API_UP_H
#define __LINUX_SPINLOCK_API_UP_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifndef __LINUX_SPINLOCK_H
# error "please don't include this file directly"
#endif

/*
 * include/linux/spinlock_api_up.h
 *
 * spinlock API implementation on UP-nondebug (inlined implementation)
 *
 * portions Copyright 2005, Red Hat, Inc., Ingo Molnar
 * Released under the General Public License (GPL).
 */

#define in_lock_functions(ADDR)		0

#define assert_raw_spin_locked(lock)	do { (void)(lock); } while (0)

#define preempt_disable()                                                      \
	do {                                                                   \
		if (!uxInterruptNesting)                                       \
			vTaskSuspendAll();                                     \
	} while (0)
#define preempt_enable()                                                       \
	do {                                                                   \
		if (!uxInterruptNesting)                                       \
			xTaskResumeAll();                                      \
	} while (0)

/*
 * In the UP-nondebug case there's no real locking going on, so the
 * only thing we have to do is to keep the preempt counts and irq
 * flags straight, to suppress compiler warnings of unused lock
 * variables, and to add the proper checker annotations:
 */
#define ___LOCK(lock) \
  do { (void)(lock); } while (0)

#define __LOCK(lock) \
  do { taskENTER_CRITICAL(); ___LOCK(lock); } while (0)

#define __LOCK_IRQ(lock) \
  do { taskENTER_CRITICAL(); } while (0)

#define __LOCK_IRQSAVE(lock, flags) \
  do { taskENTER_CRITICAL(); flags = 0;} while (0)

#define ___UNLOCK(lock) \
  do { (void)(lock); } while (0)

#define __UNLOCK(lock) \
  do { taskEXIT_CRITICAL(); ___UNLOCK(lock); } while (0)

#define __UNLOCK_IRQ(lock) \
  do { taskEXIT_CRITICAL(); } while (0)

#define __UNLOCK_IRQRESTORE(lock, flags) \
  do { taskEXIT_CRITICAL(); (void)(flags);} while (0)

#define _raw_spin_lock(lock)			__LOCK(lock)
#define _raw_spin_lock_nested(lock, subclass)	__LOCK(lock)
#define _raw_read_lock(lock)			__LOCK(lock)
#define _raw_write_lock(lock)			__LOCK(lock)
#define _raw_spin_lock_irq(lock)		__LOCK_IRQ(lock)
#define _raw_read_lock_irq(lock)		__LOCK_IRQ(lock)
#define _raw_write_lock_irq(lock)		__LOCK_IRQ(lock)
#define _raw_spin_lock_irqsave(lock, flags)	__LOCK_IRQSAVE(lock, flags)
#define _raw_read_lock_irqsave(lock, flags)	__LOCK_IRQSAVE(lock, flags)
#define _raw_write_lock_irqsave(lock, flags)	__LOCK_IRQSAVE(lock, flags)
#define _raw_spin_trylock(lock)			({ __LOCK(lock); 1; })
#define _raw_read_trylock(lock)			({ __LOCK(lock); 1; })
#define _raw_write_trylock(lock)			({ __LOCK(lock); 1; })
#define _raw_spin_unlock(lock)			__UNLOCK(lock)
#define _raw_read_unlock(lock)			__UNLOCK(lock)
#define _raw_write_unlock(lock)			__UNLOCK(lock)
#define _raw_spin_unlock_irq(lock)		__UNLOCK_IRQ(lock)
#define _raw_read_unlock_irq(lock)		__UNLOCK_IRQ(lock)
#define _raw_write_unlock_irq(lock)		__UNLOCK_IRQ(lock)
#define _raw_spin_unlock_irqrestore(lock, flags) \
					__UNLOCK_IRQRESTORE(lock, flags)
#define _raw_read_unlock_irqrestore(lock, flags) \
					__UNLOCK_IRQRESTORE(lock, flags)
#define _raw_write_unlock_irqrestore(lock, flags) \
					__UNLOCK_IRQRESTORE(lock, flags)

#endif /* __LINUX_SPINLOCK_API_UP_H */
