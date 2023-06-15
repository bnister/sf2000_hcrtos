/* Atomic operations usable in machine independent code */
#ifndef _LINUX_ATOMIC_H
#define _LINUX_ATOMIC_H

#include <freertos/FreeRTOS.h>
#include <freertos/atomic.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/irqflags.h>
#include <linux/spinlock.h>

#define atomic_read(v)		(*(volatile int *)&(v)->counter)
#define atomic_set(v, i)	((v)->counter = (i))

static __inline__ void atomic_add(int i, atomic_t *v)
{
	int temp;

	__asm__ __volatile__(
		"	.set	mips3				\n"
		"1:	ll	%0, %1		# atomic_add	\n"
		"	addu	%0, %2				\n"
		"	sc	%0, %1				\n"
		"	beqz	%0, 2f				\n"
		"	.subsection 2				\n"
		"2:	b	1b				\n"
		"	.previous				\n"
		"	.set	mips0				\n"
		: "=&r"(temp), "=m"(v->counter)
		: "Ir"(i), "m"(v->counter));
}

static __inline__ void atomic_sub(int i, atomic_t *v)
{
	int temp;

	__asm__ __volatile__(
		"	.set	mips3				\n"
		"1:	ll	%0, %1		# atomic_sub	\n"
		"	subu	%0, %2				\n"
		"	sc	%0, %1				\n"
		"	beqz	%0, 2f				\n"
		"	.subsection 2				\n"
		"2:	b	1b				\n"
		"	.previous				\n"
		"	.set	mips0				\n"
		: "=&r"(temp), "=m"(v->counter)
		: "Ir"(i), "m"(v->counter));
}

static __inline__ int atomic_sub_return(int i, atomic_t *v)
{
	int result;
	int temp;

	smp_mb__before_llsc();

	__asm__ __volatile__(
		"	.set	mips3				\n"
		"1:	ll	%1, %2		# atomic_sub_return	\n"
		"	subu	%0, %1, %3			\n"
		"	sc	%0, %2				\n"
		"	beqz	%0, 2f				\n"
		"	subu	%0, %1, %3			\n"
		"	.subsection 2				\n"
		"2:	b	1b				\n"
		"	.previous				\n"
		"	.set	mips0				\n"
		: "=&r"(result), "=&r"(temp), "=m"(v->counter)
		: "Ir"(i), "m"(v->counter)
		: "memory");

	smp_llsc_mb();

	return result;
}

#define atomic_dec_and_test(v)		(atomic_sub_return(1, (v)) == 0)

#define atomic_inc(v)			atomic_add(1, (v))

#define atomic_dec(v)			atomic_sub(1, (v))

static inline unsigned long __cmpxchg_local_generic(volatile void *ptr,
						    unsigned long old,
						    unsigned long new, int size)
{
	unsigned long flags = 0, prev = 0;

	local_irq_save(flags);
	switch (size) {
	case 1:
		prev = *(u8 *)ptr;
		if (prev == old)
			*(u8 *)ptr = (u8) new;
		break;
	case 2:
		prev = *(u16 *)ptr;
		if (prev == old)
			*(u16 *)ptr = (u16) new;
		break;
	case 4:
		prev = *(u32 *)ptr;
		if (prev == old)
			*(u32 *)ptr = (u32) new;
		break;
	case 8:
		prev = *(u64 *)ptr;
		if (prev == old)
			*(u64 *)ptr = (u64) new;
		break;
	}
	local_irq_restore(flags);
	return prev;
}

#define cmpxchg(ptr, o, n)                                                     \
	((__typeof__(*(ptr)))__cmpxchg_local_generic(                          \
		(ptr), (unsigned long)(o), (unsigned long)(n),                 \
		sizeof(*(ptr))))

#define atomic_cmpxchg(v, old, new) (cmpxchg(&((v)->counter), (old), (new)))

static inline int __atomic_add_unless(atomic_t *v, int a, int u)
{
	int c, old;
	c = atomic_read(v);
	while (c != u && (old = atomic_cmpxchg(v, c, c + a)) != c)
		c = old;
	return c;
}

static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
	return __atomic_add_unless(v, a, u) != u;
}

static inline int atomic_dec_and_lock(atomic_t *atomic, spinlock_t *lock)
{
	/* Subtract 1 from counter unless that drops it to 0 (ie. it was 1) */
	if (atomic_add_unless(atomic, -1, 1))
		return 0;

	/* Otherwise do it the slow way */
	spin_lock(lock);
	if (atomic_dec_and_test(atomic))
		return 1;
	spin_unlock(lock);
	return 0;
}

#define atomic_long_read(v) atomic_read(v)
#define atomic_long_set(v, new) atomic_set(v, new)
#define atomic_long_dec(v) atomic_dec(v)
#define atomic_long_inc(v) atomic_inc(v)

static inline int atomic_inc_return(atomic_t *v)
{
	int c;
	atomic_inc(v);
	c = atomic_read(v);
	return c;
}

#define atomic64_or(a, b)                                                      \
	do {                                                                   \
		ATOMIC_ENTER_CRITICAL();                                       \
		(b)->counter |= (a);                                           \
		ATOMIC_EXIT_CRITICAL();                                        \
	} while (0)

#define atomic64_read(v)                                                       \
	({                                                                     \
		s64 __counter;                                                 \
		ATOMIC_ENTER_CRITICAL();                                       \
		__counter = (v)->counter;                                      \
		ATOMIC_EXIT_CRITICAL();                                        \
		__counter;                                                     \
	})

#endif /* _LINUX_ATOMIC_H */
