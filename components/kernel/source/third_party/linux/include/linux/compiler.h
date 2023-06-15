/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_COMPILER_H
#define __LINUX_COMPILER_H

#include <linux/compiler_types.h>

#define likely(x)          __builtin_expect(!!(x), 1)
#define unlikely(x)        __builtin_expect(!!(x), 0)

#define __inline__ inline
#define __inline inline

#define CKUSEG			0x00000000
#define CKSEG0			0x80000000
#define CKSEG1			0xa0000000
#define CKSEG2			0xc0000000
#define CKSEG3			0xe0000000

/* Optimization barrier */
#ifndef barrier
/* The "volatile" is due to gcc bugs */
# define barrier() __asm__ __volatile__("": : :"memory")
#endif

#define cpu_relax()     barrier()

#define __sync()                                                               \
	asm volatile(".set	push\n\t"                                      \
		     ".set	noreorder\n\t"                                 \
		     ".set	mips2\n\t"                                     \
		     "sync\n\t"                                                \
		     ".set	pop"                                           \
		     : /* no output */                                         \
		     : /* no input */                                          \
		     : "memory")

#define __fast_iob()                                                           \
	asm volatile(".set	push\n\t"                                      \
		     ".set	noreorder\n\t"                                 \
		     "lw	$0,%0\n\t"                                     \
		     "nop\n\t"                                                 \
		     ".set	pop"                                           \
		     : /* no output */                                         \
		     : "m"(*(int *)CKSEG1)                                     \
		     : "memory")

#define smp_read_barrier_depends()                                             \
	do {                                                                   \
	} while (0)

#define fast_wmb()		__sync()
#define fast_rmb()		__sync()
#define fast_mb()		__sync()
#define fast_iob()		do { __sync(); __fast_iob(); } while (0)

#define wmb()			fast_mb()
#define rmb()			fast_mb()
#define mb()			fast_mb()
#define iob()			fast_iob()
#define smp_mb()		barrier()
#define smp_rmb()		barrier()
#define smp_wmb()		barrier()

#define __WEAK_LLSC_MB	    "		\n"
#define smp_llsc_mb()	        __asm__ __volatile__(__WEAK_LLSC_MB : : :"memory")
#define smp_mb__before_clear_bit()	smp_mb__before_llsc()
#define smp_mb__after_clear_bit()	smp_llsc_mb()
#define smp_mb__before_llsc()   smp_llsc_mb()

#ifndef barrier_data
/*
 * This version is i.e. to prevent dead stores elimination on @ptr
 * where gcc and llvm may behave differently when otherwise using
 * normal barrier(): while gcc behavior gets along with a normal
 * barrier(), llvm needs an explicit input variable to be assumed
 * clobbered. The issue is as follows: while the inline asm might
 * access any memory it wants, the compiler could have fit all of
 * @ptr into memory registers instead, and since @ptr never escaped
 * from that, it proved that the inline asm wasn't touching any of
 * it. This version works well with both compilers, i.e. we're telling
 * the compiler that the inline asm absolutely may see the contents
 * of @ptr. See also: https://llvm.org/bugs/show_bug.cgi?id=15495
 */
# define barrier_data(ptr) __asm__ __volatile__("": :"r"(ptr) :"memory")
#endif

# define __user
# define __kernel
# define __safe
# define __force
# define __nocast
# define __iomem
# define __rcu
# define __percpu
# define __deprecated
# define __must_check
# define __cold
# define __init
# define __exit

#define __weak		__attribute__((weak))
#define __alias(symbol)	__attribute__((alias(#symbol)))

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#define __READ_ONCE(x)	(*(const volatile typeof(x) *)&(x))
#define READ_ONCE(x)							\
({									\
	__READ_ONCE(x);							\
})

#define __attribute_const__	__attribute__((__const__))

#ifndef __maybe_unused
# define __maybe_unused		/* unimplemented */
#endif

#ifndef __always_unused
# define __always_unused	/* unimplemented */
#endif

#ifndef noinline
#define noinline
#endif

#endif /* __LINUX_COMPILER_H */
