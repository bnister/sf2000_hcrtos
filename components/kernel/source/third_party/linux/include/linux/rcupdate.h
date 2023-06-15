#ifndef __LINUX_RCUPDATE_H
#define __LINUX_RCUPDATE_H

#include <linux/types.h>
#include <linux/compiler.h>

void call_rcu(struct rcu_head *head,
	      void (*func)(struct rcu_head *head));

#define rcu_lockdep_assert(c, s) do { } while (0)
#define rcu_sleep_check() do { } while (0)
#define rcu_dereference_sparse(p, space)
#define rcu_check_sparse(p, space)

static inline int rcu_read_lock_held(void)
{
	return 1;
}

#define __rcu_access_pointer(p, space) \
({ \
	typeof(*p) *_________p1 = (typeof(*p) *__force)READ_ONCE(p); \
	rcu_check_sparse(p, space); \
	((typeof(*p) __force __kernel *)(_________p1)); \
})

#define __rcu_dereference_check(p, c, space) \
	({ \
		typeof(*p) *_________p1 = (typeof(*p)*__force )ACCESS_ONCE(p); \
		rcu_lockdep_assert(c, "suspicious rcu_dereference_check()" \
				      " usage"); \
		rcu_dereference_sparse(p, space); \
		smp_read_barrier_depends(); \
		((typeof(*p) __force __kernel *)(_________p1)); \
	})

#define rcu_dereference_check(p, c) \
	__rcu_dereference_check((p), rcu_read_lock_held() || (c), __rcu)

#define rcu_dereference_raw(p) rcu_dereference_check(p, 1) /*@@@ needed? @@@*/

#define RCU_INITIALIZER(v) (typeof(*(v)) __force __rcu *)(v)

#define RCU_INIT_POINTER(p, v) \
	do { \
		p = RCU_INITIALIZER(v); \
	} while (0)

#define rcu_assign_pointer(p, v) \
	do { \
		smp_wmb(); \
		ACCESS_ONCE(p) = RCU_INITIALIZER(v); \
	} while (0)

#define rcu_access_pointer(p) __rcu_access_pointer((p), __rcu)
#define rcu_dereference(p)                                                     \
	({                                                                     \
		typeof(p) _________p1 = p;                                     \
		smp_read_barrier_depends();                                    \
		(_________p1);                                                 \
	})

#endif /* __LINUX_RCUPDATE_H */
