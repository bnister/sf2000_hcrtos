/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 1999-2002 Vojtech Pavlik
 */
#ifndef _DRIVER_INPUT_H
#define _DRIVER_INPUT_H

#include <sys/poll.h>
#include <hcuapi/input.h>
#include <linux/types.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>

#include <linux/timer.h>

#define MAX_EVENT_SIZE  64 

#define TRKID_MAX       0xffff

#define INPUT_MT_POINTER	0x0001	/* pointer device, e.g. trackpad */
#define INPUT_MT_DIRECT		0x0002	/* direct device, e.g. touchscreen */
#define INPUT_MT_DROP_UNUSED	0x0004	/* drop contacts not seen in frame */
#define INPUT_MT_TRACK		0x0008	/* use in-kernel tracking */
#define INPUT_MT_SEMI_MT	0x0010	/* semi-mt device, finger count handled manually */


struct input_absinfo {
	__s32 value;
	__s32 minimum;
	__s32 maximum;
	__s32 fuzz;
	__s32 flat;
	__s32 resolution;
};

struct input_mt_slot {
	int abs[ABS_MT_LAST - ABS_MT_FIRST + 1];
	unsigned int frame;
};

struct input_mt {
	int trkid;
	int num_slots;
	int slot;
	unsigned int flags;
	unsigned int frame;
	int *red;
	struct input_mt_slot slots[];
};

struct input_dev {
	int id;

	DECLARE_KFIFO(kfifo, struct input_event, MAX_EVENT_SIZE);

	unsigned long propbit[BITS_TO_LONGS(INPUT_PROP_CNT)];

	unsigned long evbit[BITS_TO_LONGS(EV_CNT)];
	unsigned long keybit[BITS_TO_LONGS(KEY_CNT)];
	unsigned long relbit[BITS_TO_LONGS(REL_CNT)];
	unsigned long absbit[BITS_TO_LONGS(ABS_CNT)];
	unsigned long mscbit[BITS_TO_LONGS(MSC_CNT)];
	unsigned long ledbit[BITS_TO_LONGS(LED_CNT)];
	// unsigned long sndbit[BITS_TO_LONGS(SND_CNT)];
	// unsigned long ffbit[BITS_TO_LONGS(FF_CNT)];
	// unsigned long swbit[BITS_TO_LONGS(SW_CNT)];

	wait_queue_head_t wait;
	spinlock_t event_lock;

	int (*setkeycode)(struct input_dev *dev,
			  const struct input_keymap_entry *ke,
			  unsigned int *old_keycode);
	int (*getkeycode)(struct input_dev *dev,
			  struct input_keymap_entry *ke);

	int (*open)(struct input_dev *dev);
	void (*close)(struct input_dev *dev);
	int (*event)(struct input_dev *dev, unsigned int type, unsigned int code, int value);

	int (*ioctl)(struct file *filep, int cmd, unsigned long arg);

	unsigned int hint_events_per_packet;

	struct input_absinfo *absinfo;

	unsigned int repeat_key;
	struct timer_list timer;

	int rep[REP_CNT];

	unsigned long key[BITS_TO_LONGS(KEY_CNT)];
	unsigned long led[BITS_TO_LONGS(LED_CNT)];

	struct input_mt *mt;

	void *priv;
};

static inline void input_set_events_per_packet(struct input_dev *dev, int n_events)
{
	dev->hint_events_per_packet = n_events;
}

void input_event(struct input_dev *dev, unsigned int type, unsigned int code, int value);

static inline void input_report_abs(struct input_dev *dev, unsigned int code, int value)
{
	input_event(dev, EV_ABS, code, value);
}

static inline void input_report_key(struct input_dev *dev, unsigned int code, int value)
{
	input_event(dev, EV_KEY, code, !!value);
}

static inline void input_report_rel(struct input_dev *dev, unsigned int code, int value)
{
	input_event(dev, EV_REL, code, value);
}

static inline void input_sync(struct input_dev *dev)
{
	input_event(dev, EV_SYN, SYN_REPORT, 0);
}

static inline void *input_get_drvdata(struct input_dev *dev)
{
	return dev->priv;
}

static inline void input_set_drvdata(struct input_dev *dev, void *data)
{
	dev->priv = data;
}

int input_register_device(struct input_dev *);
int input_register_mouse_device(struct input_dev *);
int input_register_kbd_device(struct input_dev *);
void input_unregister_device(struct input_dev *);
void input_unregister_mouse_device(struct input_dev *dev);
void input_unregister_kbd_device(struct input_dev *dev);
void input_free_device(struct input_dev *dev);
struct input_dev *input_allocate_device(void);
void input_alloc_absinfo(struct input_dev *dev);
void input_set_abs_params(struct input_dev *dev, unsigned int axis,
                          int min, int max, int fuzz, int flat);

static inline void input_mt_slot(struct input_dev *dev, int slot)
{
	input_event(dev, EV_ABS, ABS_MT_SLOT, slot);
}

int input_mt_init_slots(struct input_dev *dev, unsigned int num_slots, unsigned int flags);
void input_mt_report_slot_state(struct input_dev *dev, unsigned int tool_type,
				bool active);
void input_mt_drop_unused(struct input_dev *dev);

void input_mt_sync_frame(struct input_dev *dev);

static inline void input_mt_sync(struct input_dev *dev)
{
	input_event(dev, EV_SYN, SYN_MT_REPORT, 0);
}

static inline void input_mt_set_value(struct input_mt_slot *slot,
				      unsigned code, int value)
{
	slot->abs[code - ABS_MT_FIRST] = value;
}

static inline int input_mt_get_value(const struct input_mt_slot *slot,
				     unsigned code)
{
	return slot->abs[code - ABS_MT_FIRST];
}

static inline int input_mt_new_trkid(struct input_mt *mt)
{
	return mt->trkid++ & TRKID_MAX;
}

static inline bool input_is_mt_value(int axis)
{
	return axis >= ABS_MT_FIRST && axis <= ABS_MT_LAST;
}

static inline bool input_mt_is_used(const struct input_mt *mt,
				    const struct input_mt_slot *slot)
{
	return slot->frame == mt->frame;
}

#define INPUT_GENERATE_ABS_ACCESSORS(_suffix, _item)			\
static inline int input_abs_get_##_suffix(struct input_dev *dev,	\
					  unsigned int axis)		\
{									\
	return dev->absinfo ? dev->absinfo[axis]._item : 0;		\
}									\
									\
static inline void input_abs_set_##_suffix(struct input_dev *dev,	\
					   unsigned int axis, int val)	\
{									\
	input_alloc_absinfo(dev);					\
	if (dev->absinfo)						\
		dev->absinfo[axis]._item = val;				\
}

INPUT_GENERATE_ABS_ACCESSORS(val, value)
INPUT_GENERATE_ABS_ACCESSORS(min, minimum)
INPUT_GENERATE_ABS_ACCESSORS(max, maximum)
INPUT_GENERATE_ABS_ACCESSORS(fuzz, fuzz)
INPUT_GENERATE_ABS_ACCESSORS(flat, flat)
INPUT_GENERATE_ABS_ACCESSORS(res, resolution)

void input_enable_softrepeat(struct input_dev *dev, int delay, int period);


#endif
