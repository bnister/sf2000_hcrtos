/*
 * HiChip Kernel-Userspace message queue implementation
 *
 * Copyright (C) 2021 HiChip Corporation - http://www.hichiptech.com

 * Authors:  <@hichiptech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _HC_KUMSGQ_H_
#define _HC_KUMSGQ_H_

#include <hcuapi/iocbase.h>

#define KUMSGQ_FD_ACCESS		_IO (KUMSGQ_IOCBASE, 0)
#define KUMSGQ_NOTIFIER_SETUP		_IOW(KUMSGQ_IOCBASE, 1, struct kumsg_event)

struct kumsg_event {
	unsigned long evtype;
	unsigned long arg;
};

struct kumsgq;

#ifdef __KERNEL__
/**
 * hc_new_kumsgq - Create a new kernel-userspace message queue
 *
 * In the case of success, returns a pointer to a freshly initihichipsed msgq
 * structure. Returns NULL in the case of failure.
 */
struct kumsgq *hc_new_kumsgq(void);

/**
 * hc_destroy_kumsgq - Destroy msgq structure after usage
 *
 * Marks @msgq for destruction as soon as the last message file descriptor is
 * closed.
 */
int hc_destroy_kumsgq(struct kumsgq *msgq);

/**
 * hc_kumsgq_newfd - Create a new message file descriptor for @msgq
 *
 * Returns a new file descriptor from which @current can read messages or
 * poll for them. @flags can be any combination of O_NONBLOCK and O_CLOEXEC.
 *
 * Returns a negative error code in the case of failure.
 */
int hc_kumsgq_newfd(struct kumsgq *msgq, int flags);

/**
 * hc_kumsgq_sendmsg - Send a message to a kernel-userspace message queue
 *
 * Sends a @msg_size byte message @msg_buf to the message queue. The message
 * is broadcast to all file descriptors listening on the message queue at the
 * time this function is called.
 *
 * Returns 0 on success and a negative error code in the case of failure.
 */
int hc_kumsgq_sendmsg(struct kumsgq *msgq, void *msg_buf,
			unsigned long msg_size);
#endif

#endif /* _HC_KUMSGQ_H_ */
