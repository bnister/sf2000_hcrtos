/*
 * Amazon FreeRTOS+POSIX V1.0.4
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file FreeRTOS_POSIX_portable.h
 * @brief Port-specific configuration of FreeRTOS+POSIX.
 */

#ifndef _FREERTOS_POSIX_PORTABLE_H_
#define _FREERTOS_POSIX_PORTABLE_H_

/* This port uses the defaults in FreeRTOS_POSIX_portable_default.h, so this
 * file is empty. */

#define posixconfigENABLE_CLOCK_T                0 /**< clock_t in sys/types.h */
#define posixconfigENABLE_CLOCKID_T              0 /**< clockid_t in sys/types.h */
#define posixconfigENABLE_MODE_T                 0 /**< mode_t in sys/types.h */
#define posixconfigENABLE_PID_T                  0 /**< pid_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_ATTR_T         0 /**< pthread_attr_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_COND_T         1 /**< pthread_cond_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_CONDATTR_T     1 /**< pthread_condattr_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_MUTEX_T        1 /**< pthread_mutex_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_MUTEXATTR_T    1 /**< pthread_mutexattr_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_RWLOCK_T       1 /**< pthread_rwlock_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_RWLOCKATTR_T   1 /**< pthread_rwlockattr_t in sys/types.h */
#define posixconfigENABLE_PTHREAD_T              0 /**< pthread_t in sys/types.h */
#define posixconfigENABLE_SSIZE_T                0 /**< ssize_t in sys/types.h */
#define posixconfigENABLE_TIME_T                 0 /**< time_t in sys/types.h */
#define posixconfigENABLE_TIMER_T                0 /**< timer_t in sys/types.h */
#define posixconfigENABLE_USECONDS_T             0 /**< useconds_t in sys/types.h */
#define posixconfigENABLE_TIMESPEC               0 /**< struct timespec in time.h */
#define posixconfigENABLE_ITIMERSPEC             0 /**< struct itimerspec in time.h */
#define posixconfigENABLE_SEM_T                  1 /**< struct sem_t in semaphore.h */
#define posixconfigENABLE_PTHREAD_BARRIER_T      1 /**< pthread_barrier_t in sys/types.h */
#define posixconfigENABLE_OFF_T                  0 /**< off_t in sys/types.h */

#endif /* _FREERTOS_POSIX_PORTABLE_H_ */
