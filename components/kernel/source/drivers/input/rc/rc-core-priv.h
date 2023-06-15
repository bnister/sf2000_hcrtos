/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Remote Controller core raw events header
 *
 * Copyright (C) 2010 by Mauro Carvalho Chehab
 */

#ifndef _RC_CORE_PRIV
#define _RC_CORE_PRIV

#include <stdbool.h>
#include <linux/kconfig.h>
#include <kernel/list.h>
#include <linux/kfifo.h>
#include <linux/ktime.h>
#include "rc-core.h"

/* Define the max number of pulse/space transitions to buffer */
#define	MAX_IR_EVENT_SIZE	512

/**
 * rc_open - Opens a RC device
 *
 * @rdev: pointer to struct rc_dev.
 */
int rc_open(struct rc_dev *rdev);

/**
 * rc_close - Closes a RC device
 *
 * @rdev: pointer to struct rc_dev.
 */
void rc_close(struct rc_dev *rdev);

struct ir_raw_handler {
	struct list_head list;

	u64 protocols; /* which are handled by this handler */
	int (*decode)(struct rc_dev *dev, struct ir_raw_event event);
	int (*encode)(enum rc_proto protocol, u32 scancode,
		      struct ir_raw_event *events, unsigned int max);
	u32 carrier;
	u32 min_timeout;

	/* These two should only be used by the mce kbd decoder */
	int (*raw_register)(struct rc_dev *dev);
	int (*raw_unregister)(struct rc_dev *dev);
};

struct ir_raw_event_ctrl {
	struct list_head		list;		/* to keep track of raw clients */
	struct task_struct		*thread;
	/* fifo for the pulse/space durations */
	DECLARE_KFIFO(kfifo, struct ir_raw_event, MAX_IR_EVENT_SIZE);
	ktime_t				last_event;	/* when last event occurred */
	struct rc_dev			*dev;		/* pointer to the parent rc_dev */

	/* raw decoder state follows */
	struct ir_raw_event prev_ev;
	struct ir_raw_event this_ev;

#if IS_ENABLED(CONFIG_IR_NEC_DECODER)
	struct nec_dec {
		int state;
		unsigned count;
		u32 bits;
		bool is_nec_x;
		bool necx_repeat;
	} nec;
#endif
#if IS_ENABLED(CONFIG_IR_RC5_DECODER)
	struct rc5_dec {
		int state;
		u32 bits;
		unsigned count;
		bool is_rc5x;
	} rc5;
#endif
#if IS_ENABLED(CONFIG_IR_RC6_DECODER)
	struct rc6_dec {
		int state;
		u8 header;
		u32 body;
		bool toggle;
		unsigned count;
		unsigned wanted_bits;
	} rc6;
#endif
#if IS_ENABLED(CONFIG_IR_SONY_DECODER)
	struct sony_dec {
		int state;
		u32 bits;
		unsigned count;
	} sony;
#endif
#if IS_ENABLED(CONFIG_IR_JVC_DECODER)
	struct jvc_dec {
		int state;
		u16 bits;
		u16 old_bits;
		unsigned count;
		bool first;
		bool toggle;
	} jvc;
#endif
#if IS_ENABLED(CONFIG_IR_SANYO_DECODER)
	struct sanyo_dec {
		int state;
		unsigned count;
		u64 bits;
	} sanyo;
#endif
#if IS_ENABLED(CONFIG_IR_SHARP_DECODER)
	struct sharp_dec {
		int state;
		unsigned count;
		u32 bits;
		unsigned int pulse_len;
	} sharp;
#endif
#if IS_ENABLED(CONFIG_IR_MCE_KBD_DECODER)
	struct mce_kbd_dec {
		/* locks key up timer */
		spinlock_t keylock;
		struct timer_list rx_timeout;
		int state;
		u8 header;
		u32 body;
		unsigned count;
		unsigned wanted_bits;
	} mce_kbd;
#endif
#if IS_ENABLED(CONFIG_IR_XMP_DECODER)
	struct xmp_dec {
		int state;
		unsigned count;
		u32 durations[16];
	} xmp;
#endif
#if IS_ENABLED(CONFIG_IR_IMON_DECODER)
	struct imon_dec {
		int state;
		int count;
		int last_chk;
		unsigned int bits;
		bool stick_keyboard;
	} imon;
#endif
#if IS_ENABLED(CONFIG_IR_RCMM_DECODER)
	struct rcmm_dec {
		int state;
		unsigned int count;
		u32 bits;
	} rcmm;
#endif
};

/* Mutex for locking raw IR processing and handler change */
extern struct mutex ir_raw_handler_lock;

/* macros for IR decoders */
static inline bool geq_margin(unsigned d1, unsigned d2, unsigned margin)
{
	return d1 > (d2 - margin);
}

static inline bool eq_margin(unsigned d1, unsigned d2, unsigned margin)
{
	return ((d1 > (d2 - margin)) && (d1 < (d2 + margin)));
}

static inline bool is_transition(struct ir_raw_event *x, struct ir_raw_event *y)
{
	return x->pulse != y->pulse;
}

static inline void decrease_duration(struct ir_raw_event *ev, unsigned duration)
{
	if (duration > ev->duration)
		ev->duration = 0;
	else
		ev->duration -= duration;
}

/* Returns true if event is normal pulse/space event */
static inline bool is_timing_event(struct ir_raw_event ev)
{
	return !ev.carrier_report && !ev.reset;
}

#define TO_STR(is_pulse)		((is_pulse) ? "pulse" : "space")

/*
 * Routines from rc-raw.c to be used internally and by decoders
 */
u64 ir_raw_get_allowed_protocols(void);
int ir_raw_event_prepare(struct rc_dev *dev);
int ir_raw_event_register(struct rc_dev *dev);
void ir_raw_event_free(struct rc_dev *dev);
void ir_raw_event_unregister(struct rc_dev *dev);
int ir_raw_handler_register(struct ir_raw_handler *ir_raw_handler);
void ir_raw_handler_unregister(struct ir_raw_handler *ir_raw_handler);
void ir_raw_load_modules(u64 *protocols);

#endif /* _RC_CORE_PRIV */
