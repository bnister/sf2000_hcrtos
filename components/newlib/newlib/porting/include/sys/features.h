#ifndef _ADAPT_SYS_FEATURES_H
#define _ADAPT_SYS_FEATURES_H

#define _GNU_SOURCE

/* adapt time.h */
#define _POSIX_TIMERS 1
//#define _POSIX_CPUTIME 1
//#define _POSIX_THREAD_CPUTIME 1
#define _POSIX_MONOTONIC_CLOCK 1
//#define _POSIX_CLOCK_SELECTION 1
#define _POSIX_GETGR_R_SIZE_MAX 0
#define _POSIX_GETPW_R_SIZE_MAX 0
#define _POSIX_CLK_TICK time((time_t *)NULL)
#define _POSIX_TIMER_MAX 32
#define _POSIX_VERSION 199309L
#define POSIX_VERSION _POSIX_VERSION

/* adapt sys/signal.h */
#define _POSIX_REALTIME_SIGNALS 1

/* adapt pthread */
#define _POSIX_THREADS 1
#define _POSIX_PRIORITY_SCHEDULING 1
//#define _POSIX_TIMEOUTS
//#define _POSIX_THREAD_PRIORITY_SCHEDULING
//#define _UNIX98_THREAD_MUTEX_ATTRIBUTES
#define _POSIX_THREAD_PROCESS_SHARED
#define __LINUX_ERRNO_EXTENSIONS__

#define __have_long64 0
#define _REDIR_TIME64 0

#define	__LITTLE_ENDIAN	_LITTLE_ENDIAN
#define	__BIG_ENDIAN	_BIG_ENDIAN
#define	__BYTE_ORDER	_BYTE_ORDER

#endif /* !_ADAPT_SYS_FEATURES_H */

#include_next <sys/features.h>
