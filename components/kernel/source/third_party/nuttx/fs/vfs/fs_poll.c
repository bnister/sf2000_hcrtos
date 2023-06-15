/****************************************************************************
 * fs/vfs/fs_poll.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <generated/br2_autoconf.h>

#include <stdbool.h>
#include <poll.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/errno.h>
#include <sys/times.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <kernel/list.h>
#include <kernel/wait.h>

#include <nuttx/fs/fs.h>
#include <sys/poll.h>

#include "../inode/inode.h"

/****************************************************************************
 * Name: file_poll
 *
 * Description:
 *   Low-level poll operation based on struct file.  This is used both to (1)
 *   support detached file, and also (2) by poll_fdsetup() to perform all
 *   normal operations on file descriptors.
 *
 * Input Parameters:
 *   file     File structure instance
 *   fds   - The structure describing the events to be monitored, OR NULL if
 *           this is a request to stop monitoring events.
 *   setup - true: Setup up the poll; false: Teardown the poll
 *
 * Returned Value:
 *  0: Success; Negated errno on failure
 *
 ****************************************************************************/

static int file_poll(FAR struct file *filep, poll_table *wait)
{
	FAR struct inode *inode;
	int mask = 0;

	DEBUGASSERT(filep != NULL);
	inode = filep->f_inode;

	if (inode != NULL) {
		/* Is a driver registered? Does it support the poll method?
		 * If not, return -ENOSYS
		 */

		if (inode->u.i_ops != NULL && inode->u.i_ops->poll != NULL) {
			/* Yes, it does... Setup the poll */

			mask = (int)inode->u.i_ops->poll(filep, wait);
		}
	} else {
		return -1;
	}

	return mask;
}

void __pollwait(struct file *filp, wait_queue_head_t *wait_address,
		       poll_table *p)
{
	struct poll_wqueues *pwq = container_of(p, struct poll_wqueues, pt);
	struct poll_table_entry *entry = NULL;

	if (pwq->inline_entry_idx < CONFIG_INLINE_POLL_NFILE_DESCRIPTORS) {
		entry = pwq->inline_entry + pwq->inline_entry_idx++;
	} else {
		entry = malloc(sizeof(struct poll_table_entry));
		if (!entry)
			return;
	}

	entry->wait = pwq;

	taskENTER_CRITICAL();
	list_add_tail(&entry->task_list, &wait_address->task_list);
	list_add_tail(&entry->poll_list, &pwq->poll_list_head);
	taskEXIT_CRITICAL();
}

void poll_wait(struct file *filp, wait_queue_head_t *wait_address,
	       poll_table *p)
{
	if (p && p->_qproc && wait_address) {
		p->_qproc(filp, wait_address, p);
	}
}

void wake_up(wait_queue_head_t *wait_address)
{
	struct poll_table_entry *curr, *next;
	BaseType_t yield = pdFALSE, tmp;
	TaskHandle_t tsk;

	taskENTER_CRITICAL();
	list_for_each_entry_safe (curr, next, &wait_address->task_list, task_list) {
		tsk = (TaskHandle_t)curr->wait->pt._priv;
		if (tsk != NULL) {
			if (curr->wait->pt._is_notification_poll) {
				if (uxInterruptNesting != 0) {
					tmp = pdFALSE;
					vTaskNotifyGiveIndexedFromISR(tsk, configTASK_NOTIFICATION_POLL, &tmp);
					if (tmp == pdTRUE)
						yield = pdTRUE;
				} else {
					xTaskNotifyGiveIndexed(tsk, configTASK_NOTIFICATION_POLL);
				}
			} else {
				if (xPortIsInISR()) {
					xTaskResumeFromISR(tsk);
				} else {
					xTaskAbortDelay(tsk);
					vTaskResume(tsk);
				}
			}
		}
	}

	if (yield == pdTRUE) {
		portYIELD();
	}

	taskEXIT_CRITICAL();
}

void poll_setup(struct poll_wqueues *q)
{
	poll_table *pt;

	pt = &q->pt;

	pt->_priv = xTaskGetCurrentTaskHandle();
	pt->_qproc = __pollwait;
	pt->_is_notification_poll = true;
	INIT_LIST_HEAD(&q->poll_list_head);

	return;
}

void destroy_poll_wait(struct poll_wqueues *wait)
{
	struct poll_table_entry *curr, *next;
	poll_table *pt;
	int i;

	pt = &wait->pt;

	taskENTER_CRITICAL();
	for (i = 0; i < wait->inline_entry_idx; i++) {
		curr = &wait->inline_entry[i];
		list_del(&curr->task_list);
		list_del(&curr->poll_list);
	}

	list_for_each_entry_safe (curr, next, &wait->poll_list_head, poll_list) {
		list_del(&curr->task_list);
		list_del(&curr->poll_list);
		free(curr);
	}
	pt->_priv = NULL;
	taskEXIT_CRITICAL();
}

static int query_fd(int fd, poll_table *wait)
{
	FAR struct file *filep;
	int ret;

	/* Get the file pointer corresponding to this file descriptor */

	ret = fs_getfilep(fd, &filep);
	if (ret < 0) {
		return ret;
	}

	DEBUGASSERT(filep != NULL);

	return file_poll(filep, wait);
}

int poll_query(struct pollfd *fds, nfds_t nfds, struct poll_wqueues *wait)
{
	int mask = 0, filter, fdcount = 0;
	poll_table *pt = &wait->pt;
	nfds_t i;

	for (i = 0; i < nfds; i++) {
		mask = query_fd(fds[i].fd, pt);
		if (mask < 0) {
			set_errno(-mask);
			return -1;
		}
		filter = fds[i].events | POLLERR | POLLHUP;
		mask &= filter;
		fds[i].revents = mask;
		if (mask) {
			fdcount++;
			pt->_qproc = NULL;
		}
	}

	pt->_qproc = NULL;

	return fdcount;
}

/****************************************************************************
 * Name: nx_poll
 *
 * Description:
 *   nx_poll() is similar to the standard 'poll' interface except that is
 *   not a cancellation point and it does not modify the errno variable.
 *
 *   nx_poll() is an internal NuttX interface and should not be called from
 *   applications.
 *
 * Returned Value:
 *   Zero is returned on success; a negated value is returned on any failure.
 *
 ****************************************************************************/

int nx_poll(FAR struct pollfd *fds, unsigned int nfds, int timeout)
{
	int fdcount = -EFAULT, timed_out = 0;
	struct poll_wqueues wait = { 0 };
	TickType_t xTicksToWait;

	poll_setup(&wait);

	if ((TickType_t)timeout == portMAX_DELAY) {
		xTicksToWait = portMAX_DELAY;
	} else {
		xTicksToWait = timeout * portTICK_PERIOD_MS;
	}

	for (;;) {
		fdcount = poll_query(fds, nfds, &wait);

		if (fdcount || timed_out || xTicksToWait == 0)
			break;

		if (ulTaskNotifyTakeIndexed(configTASK_NOTIFICATION_POLL,
					    pdTRUE, xTicksToWait) == 0) {
			timed_out = 1;
		}
	}

	destroy_poll_wait(&wait);

	return fdcount;
}

/****************************************************************************
 * Name: poll
 *
 * Description:
 *   poll() waits for one of a set of file descriptors to become ready to
 *   perform I/O.  If none of the events requested (and no error) has
 *   occurred for any of  the  file  descriptors,  then  poll() blocks until
 *   one of the events occurs.
 *
 * Input Parameters:
 *   fds  - List of structures describing file descriptors to be monitored
 *   nfds - The number of entries in the list
 *   timeout - Specifies an upper limit on the time for which poll() will
 *     block in milliseconds.  A negative value of timeout means an infinite
 *     timeout.
 *
 * Returned Value:
 *   On success, the number of structures that have non-zero revents fields.
 *   A value of 0 indicates that the call timed out and no file descriptors
 *   were ready.  On error, -1 is returned, and errno is set appropriately:
 *
 *   EBADF  - An invalid file descriptor was given in one of the sets.
 *   EFAULT - The fds address is invalid
 *   EINTR  - A signal occurred before any requested event.
 *   EINVAL - The nfds value exceeds a system limit.
 *   ENOMEM - There was no space to allocate internal data structures.
 *   ENOSYS - One or more of the drivers supporting the file descriptor
 *     does not support the poll method.
 *
 ****************************************************************************/

int poll(FAR struct pollfd *fds, nfds_t nfds, int timeout)
{
	int ret;

	/* poll() is a cancellation point */

	//enter_cancellation_point();

	/* Let nx_poll() do all of the work */

	ret = nx_poll(fds, nfds, timeout);
	if (ret < 0) {
		set_errno(-ret);
		ret = VFS_ERROR;
	}

	//leave_cancellation_point();
	return ret;
}
