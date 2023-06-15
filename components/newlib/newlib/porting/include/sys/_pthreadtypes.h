#ifndef _ADAPT_SYS_PTHREADTYPES_H
#define _ADAPT_SYS_PTHREADTYPES_H

#define pthread_attr_t __pthread_attr_t_discard
#define pthread_once_t __pthread_once_t_discard
#define pthread_cond_t __pthread_cond_t_discard
#define pthread_condattr_t __pthread_condattr_t_discard
#define pthread_mutex_t __pthread_mutex_t_discard
#define pthread_mutexattr_t __pthread_mutexattr_t_discard
#define pthread_key_t __pthread_key_t_discard
#define pthread_rwlock_t __pthread_rwlock_t
#define pthread_rwlockattr_t __pthread_rwlockattr_t
#define _PTHREAD_RWLOCK_INITIALIZER ___PTHREAD_RWLOCK_INITIALIZER
//#define sched_param __sched_param_discard

#include_next <sys/_pthreadtypes.h>

#undef pthread_attr_t
#undef pthread_once_t
#undef pthread_cond_t
#undef pthread_condattr_t
#undef pthread_mutex_t
#undef pthread_mutexattr_t
#undef pthread_key_t
#undef pthread_rwlock_t
#undef pthread_rwlockattr_t
#undef _PTHREAD_RWLOCK_INITIALIZER
//#undef sched_param

typedef struct
{
    uint32_t usStackSize;                /**< Stack size. */
    uint16_t usSchedPriorityDetachState; /**< Schedule priority 15 bits (LSB) Detach state: 1 bits (MSB) */
} pthread_attr_t;

typedef int pthread_key_t;
typedef int pthread_once_t;
/* init value for pthread_once_t */
#define PTHREAD_ONCE_INIT       0

#endif /* !_ADAPT_SYS_PTHREADTYPES_H */
