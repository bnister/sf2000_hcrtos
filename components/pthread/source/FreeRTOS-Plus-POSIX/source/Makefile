sources = FreeRTOS_POSIX_clock.c \
	FreeRTOS_POSIX_mqueue.c \
	FreeRTOS_POSIX_pthread.c \
	FreeRTOS_POSIX_pthread_barrier.c \
	FreeRTOS_POSIX_pthread_cond.c \
	FreeRTOS_POSIX_pthread_mutex.c \
	FreeRTOS_POSIX_sched.c \
	FreeRTOS_POSIX_semaphore.c \
	FreeRTOS_POSIX_timer.c \
	FreeRTOS_POSIX_unistd.c \
	FreeRTOS_POSIX_utils.c \
	FreeRTOS_POSIX_pthread_local_storage.c \
	FreeRTOS_POSIX_pthread_rwlock.c

CFLAGS_FreeRTOS_POSIX_timer.o = -Wno-int-conversion
pthread-y := ${sources:.c=.o}

obj-y += pthread.o
