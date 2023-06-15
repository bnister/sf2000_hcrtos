/* SPDX-License-Identifier: GPL-2.0 */
/* rwsem.h: R/W semaphores, public interface
 *
 * Written by David Howells (dhowells@redhat.com).
 * Derived from asm-i386/semaphore.h
 */

#ifndef _LINUX_RWSEM_H
#define _LINUX_RWSEM_H

#include <freertos/FreeRTOS.h>
#include <freertos/atomic.h>
#include <linux/list.h>

struct rw_semaphore {
	signed int              activity;
#define RWSEM_UNLOCKED_VALUE            0x00000000
	struct list_head        wait_list;
};

/* In all implementations activity != 0 means locked */
static inline int rwsem_is_locked(struct rw_semaphore *sem)
{
	int counter;
	ATOMIC_ENTER_CRITICAL();
	counter = sem->activity;
	ATOMIC_EXIT_CRITICAL();
	return (counter != 0);
}

#define __RWSEM_INITIALIZER(name)                                              \
	{                                                                      \
		.activity = RWSEM_UNLOCKED_VALUE,                              \
		.wait_list = LIST_HEAD_INIT((name).wait_list)                  \
	}

#define DECLARE_RWSEM(name) \
	struct rw_semaphore name = __RWSEM_INITIALIZER(name)

#define init_rwsem(sem)                                                        \
	do {                                                                   \
		(sem)->activity = 0;                                           \
		INIT_LIST_HEAD(&(sem)->wait_list);                             \
	} while (0)

/*
 * This is the same regardless of which rwsem implementation that is being used.
 * It is just a heuristic meant to be called by somebody alreadying holding the
 * rwsem to see if somebody from an incompatible type is wanting access to the
 * lock.
 */
static inline int rwsem_is_contended(struct rw_semaphore *sem)
{
	return !list_empty(&sem->wait_list);
}

/*
 * lock for reading
 */
extern void down_read(struct rw_semaphore *sem);

/*
 * trylock for reading -- returns 1 if successful, 0 if contention
 */
extern int down_read_trylock(struct rw_semaphore *sem);

/*
 * lock for writing
 */
extern void down_write(struct rw_semaphore *sem);

/*
 * trylock for writing -- returns 1 if successful, 0 if contention
 */
extern int down_write_trylock(struct rw_semaphore *sem);

/*
 * release a read lock
 */
extern void up_read(struct rw_semaphore *sem);

/*
 * release a write lock
 */
extern void up_write(struct rw_semaphore *sem);

/*
 * downgrade write lock to read lock
 */
extern void downgrade_write(struct rw_semaphore *sem);

# define down_read_nested(sem, subclass)		down_read(sem)
# define down_write_nest_lock(sem, nest_lock)	down_write(sem)
# define down_write_nested(sem, subclass)	down_write(sem)
# define down_read_non_owner(sem)		down_read(sem)
# define up_read_non_owner(sem)			up_read(sem)

#endif /* _LINUX_RWSEM_H */
