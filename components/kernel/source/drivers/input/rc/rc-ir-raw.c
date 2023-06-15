// SPDX-License-Identifier: GPL-2.0
// rc-ir-raw.c - handle IR pulse/space events
//
// Copyright (C) 2010 by Mauro Carvalho Chehab

#include <errno.h>
#include <kernel/list.h>
#include <kernel/module.h>

#include <linux/printk.h>
#include <linux/export.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/atomic.h>
#include <linux/sched.h>
#include "rc-core-priv.h"

/* Used to keep track of IR raw clients, protected by ir_raw_handler_lock */
static LIST_HEAD(ir_raw_client_list);

/* Used to handle IR raw handler extensions */
DEFINE_MUTEX(ir_raw_handler_lock);
static LIST_HEAD(ir_raw_handler_list);
static atomic64_t available_protocols = ATOMIC64_INIT(0);

static int ir_raw_event_thread(void *data)
{
	struct ir_raw_event ev;
	struct ir_raw_handler *handler;
	struct ir_raw_event_ctrl *raw = data;
	struct rc_dev *dev = raw->dev;

	while (1) {
		mutex_lock(&ir_raw_handler_lock);
		while (kfifo_out(&raw->kfifo, &ev, 1)) {
			if (is_timing_event(ev)) {
				if (ev.duration == 0)
					dev_dbg(&dev->dev, "nonsensical timing event of duration 0\n");
				if (is_timing_event(raw->prev_ev) &&
				    !is_transition(&ev, &raw->prev_ev))
					dev_dbg(&dev->dev, "two consecutive events of type %s\n",
						      TO_STR(ev.pulse));
				if (raw->prev_ev.reset && ev.pulse == 0)
					dev_dbg(&dev->dev, "timing event after reset should be pulse\n");
			}
			list_for_each_entry(handler, &ir_raw_handler_list, list)
				if (dev->enabled_protocols &
				    handler->protocols || !handler->protocols)
					handler->decode(dev, ev);
			raw->prev_ev = ev;
		}
		mutex_unlock(&ir_raw_handler_lock);

		set_current_state(TASK_INTERRUPTIBLE);

		if (kthread_should_stop()) {
			__set_current_state(TASK_RUNNING);
			break;
		} else if (!kfifo_is_empty(&raw->kfifo))
			set_current_state(TASK_RUNNING);

		schedule();
	}

	return 0;
}

/**
 * ir_raw_event_store() - pass a pulse/space duration to the raw ir decoders
 * @dev:	the struct rc_dev device descriptor
 * @ev:		the struct ir_raw_event descriptor of the pulse/space
 *
 * This routine (which may be called from an interrupt context) stores a
 * pulse/space duration for the raw ir decoding state machines. Pulses are
 * signalled as positive values and spaces as negative values. A zero value
 * will reset the decoding state machines.
 */
int ir_raw_event_store(struct rc_dev *dev, struct ir_raw_event *ev)
{
	if (!dev->raw)
		return -EINVAL;

	dev_dbg(&dev->dev, "sample: (%05dus %s)\n",
		ev->duration, TO_STR(ev->pulse));

	if (!kfifo_put(&dev->raw->kfifo, *ev)) {
		dev_err(&dev->dev, "IR event FIFO is full!\n");
		return -ENOSPC;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ir_raw_event_store);

/**
 * ir_raw_event_store_with_filter() - pass next pulse/space to decoders with some processing
 * @dev:	the struct rc_dev device descriptor
 * @ev:		the event that has occurred
 *
 * This routine (which may be called from an interrupt context) works
 * in similar manner to ir_raw_event_store_edge.
 * This routine is intended for devices with limited internal buffer
 * It automerges samples of same type, and handles timeouts. Returns non-zero
 * if the event was added, and zero if the event was ignored due to idle
 * processing.
 */
int ir_raw_event_store_with_filter(struct rc_dev *dev, struct ir_raw_event *ev)
{
	if (!dev->raw)
		return -EINVAL;

	/* Ignore spaces in idle mode */
	if (dev->idle && !ev->pulse)
		return 0;
	else if (dev->idle) {
		ir_raw_event_set_idle(dev, false);
	}

	if (!dev->raw->this_ev.duration)
		dev->raw->this_ev = *ev;
	else if (ev->pulse == dev->raw->this_ev.pulse)
		dev->raw->this_ev.duration += ev->duration;
	else {
		ir_raw_event_store(dev, &dev->raw->this_ev);
		dev->raw->this_ev = *ev;
	}

	/* Enter idle mode if necessary */
	if (!ev->pulse && dev->timeout &&
	    dev->raw->this_ev.duration >= dev->timeout) {
		ir_raw_event_set_idle(dev, true);
	}

	return 1;
}
EXPORT_SYMBOL_GPL(ir_raw_event_store_with_filter);

/**
 * ir_raw_event_set_idle() - provide hint to rc-core when the device is idle or not
 * @dev:	the struct rc_dev device descriptor
 * @idle:	whether the device is idle or not
 */
void ir_raw_event_set_idle(struct rc_dev *dev, bool idle)
{
	if (!dev->raw)
		return;

	dev_dbg(&dev->dev, "%s idle mode\n", idle ? "enter" : "leave");

	if (idle) {
		dev->raw->this_ev.timeout = true;
		ir_raw_event_store(dev, &dev->raw->this_ev);
		dev->raw->this_ev = (struct ir_raw_event) {};
	}

	if (dev->s_idle)
		dev->s_idle(dev, idle);

	dev->idle = idle;
}
EXPORT_SYMBOL_GPL(ir_raw_event_set_idle);

/**
 * ir_raw_event_handle() - schedules the decoding of stored ir data
 * @dev:	the struct rc_dev device descriptor
 *
 * This routine will tell rc-core to start decoding stored ir data.
 */
void ir_raw_event_handle(struct rc_dev *dev)
{
	if (!dev->raw || !dev->raw->thread)
		return;

	wake_up_process(dev->raw->thread);
}
EXPORT_SYMBOL_GPL(ir_raw_event_handle);

/* used internally by the sysfs interface */
u64
ir_raw_get_allowed_protocols(void)
{
	return atomic64_read(&available_protocols);
}

static int change_protocol(struct rc_dev *dev, u64 *rc_proto)
{
	struct ir_raw_handler *handler;
	u32 timeout = 0;

	mutex_lock(&ir_raw_handler_lock);
	list_for_each_entry(handler, &ir_raw_handler_list, list) {
		if (!(dev->enabled_protocols & handler->protocols) &&
		    (*rc_proto & handler->protocols) && handler->raw_register)
			handler->raw_register(dev);

		if ((dev->enabled_protocols & handler->protocols) &&
		    !(*rc_proto & handler->protocols) &&
		    handler->raw_unregister)
			handler->raw_unregister(dev);
	}
	mutex_unlock(&ir_raw_handler_lock);

	if (!dev->max_timeout)
		return 0;

	mutex_lock(&ir_raw_handler_lock);
	list_for_each_entry(handler, &ir_raw_handler_list, list) {
		if (handler->protocols & *rc_proto) {
			if (timeout < handler->min_timeout)
				timeout = handler->min_timeout;
		}
	}
	mutex_unlock(&ir_raw_handler_lock);

	if (timeout == 0)
		timeout = IR_DEFAULT_TIMEOUT;
	else
		timeout += MS_TO_US(10);

	if (timeout < dev->min_timeout)
		timeout = dev->min_timeout;
	else if (timeout > dev->max_timeout)
		timeout = dev->max_timeout;

	if (dev->s_timeout)
		dev->s_timeout(dev, timeout);
	else
		dev->timeout = timeout;

	return 0;
}

/*
 * Used to (un)register raw event clients
 */
int ir_raw_event_prepare(struct rc_dev *dev)
{
	if (!dev)
		return -EINVAL;

	dev->raw = kzalloc(sizeof(*dev->raw), GFP_KERNEL);
	if (!dev->raw)
		return -ENOMEM;

	dev->raw->dev = dev;
	dev->change_protocol = change_protocol;
	dev->idle = true;
	INIT_KFIFO(dev->raw->kfifo);

	return 0;
}

int ir_raw_event_register(struct rc_dev *dev)
{
	struct task_struct *thread;

	thread = kthread_run(ir_raw_event_thread, dev->raw, "rc%u", dev->minor);
	if (IS_ERR(thread))
		return PTR_ERR(thread);

	dev->raw->thread = thread;

	mutex_lock(&ir_raw_handler_lock);
	list_add_tail(&dev->raw->list, &ir_raw_client_list);
	mutex_unlock(&ir_raw_handler_lock);

	return 0;
}

void ir_raw_event_free(struct rc_dev *dev)
{
	if (!dev)
		return;

	kfree(dev->raw);
	dev->raw = NULL;
}

void ir_raw_event_unregister(struct rc_dev *dev)
{
	struct ir_raw_handler *handler;

	if (!dev || !dev->raw)
		return;

	kthread_stop(dev->raw->thread);

	mutex_lock(&ir_raw_handler_lock);
	list_del(&dev->raw->list);
	list_for_each_entry(handler, &ir_raw_handler_list, list)
		if (handler->raw_unregister &&
		    (handler->protocols & dev->enabled_protocols))
			handler->raw_unregister(dev);

	ir_raw_event_free(dev);

	/*
	 * A user can be calling bpf(BPF_PROG_{QUERY|ATTACH|DETACH}), so
	 * ensure that the raw member is null on unlock; this is how
	 * "device gone" is checked.
	 */
	mutex_unlock(&ir_raw_handler_lock);
}

/*
 * Extension interface - used to register the IR decoders
 */

int ir_raw_handler_register(struct ir_raw_handler *ir_raw_handler)
{
	mutex_lock(&ir_raw_handler_lock);
	list_add_tail(&ir_raw_handler->list, &ir_raw_handler_list);
	atomic64_or(ir_raw_handler->protocols, &available_protocols);
	mutex_unlock(&ir_raw_handler_lock);

	return 0;
}
EXPORT_SYMBOL(ir_raw_handler_register);
