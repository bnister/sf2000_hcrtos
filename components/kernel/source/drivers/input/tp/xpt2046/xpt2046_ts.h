/*
 * drivers/input/touchscreen/xpt2046_ts.h
 *
 * Copyright (C) 2010 ROCKCHIP, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __DRIVERS_TOUCHSCREEN_XPT2046_TS_H
#define __DRIVERS_TOUCHSCREEN_XPT2046_TS_H

#define IOMUX_NAME_SIZE 40

#define SCREEN_MAX_X 1000
#define SCREEN_MAX_Y 800

#include <linux/timer.h>

enum xpt2046_filter {
	XPT2046_FILTER_OK,
	XPT2046_FILTER_REPEAT,
	XPT2046_FILTER_IGNORE,
};

struct xpt2046_platform_data {
	u16	model;			/* 2046. */
	bool	keep_vref_on;		/* set to keep vref on for differential
					 * measurements as well */
	bool	swap_xy;		/* swap x and y axes */

	/* If set to non-zero, after samples are taken this delay is applied
	 * and penirq is rechecked, to help avoid false events.  This value
	 * is affected by the material used to build the touch layer.
	 */
	u16	penirq_recheck_delay_usecs;

	u16	x_min, x_max;
	u16	y_min, y_max;

	u16	debounce_max;		/* max number of additional readings
					 * per sample */
	u16	debounce_tol;		/* tolerance used for filtering */
	u16	debounce_rep;		/* additional consecutive good readings
					 * required after the first two */
	int	gpio_pendown;		/* the GPIO used to decide the pendown
					 * state if get_pendown_state == NULL
					 */
	char	pendown_iomux_name[IOMUX_NAME_SIZE];	
	int		pendown_iomux_mode;	
	int 	touch_ad_top;
	int     touch_ad_bottom;
	int 	touch_ad_left;
	int 	touch_ad_right;
	int		touch_virtualkey_length;
	int	(*get_pendown_state)(void);
	int	(*filter_init)	(struct xpt2046_platform_data *pdata,
				 void **filter_data);
	int	(*filter)	(void *filter_data, int data_idx, int *val);
	void	(*filter_cleanup)(void *filter_data);
	void	(*wait_for_sync)(void);
	int (* io_init)(void);
	int (* io_deinit)(void);
};
/* -------------------------------- */
struct ts_event {
	/* For portability, we can't read 12 bit values using SPI (which
	 * would make the controller deliver them as native byteorder u16
	 * with msbs zeroed).  Instead, we read them as two 8-bit values,
	 * *** WHICH NEED BYTESWAPPING *** and range adjustment.
	 */
	u16	x;
	u16	y;
	int	ignore;
};
struct xpt2046_packet {
	u8			read_x, read_y, pwrdown;
	u16			dummy;		/* for the pwrdown read */
	struct ts_event		tc;
};

struct xpt2046 {
	struct input_dev	*input;
	char	phys[32];
	char	name[32];
	char	pendown_iomux_name[IOMUX_NAME_SIZE];	
	struct spi_device	*spi;

	u16		model;
	u16		x_min, x_max;	
	u16		y_min, y_max; 
	u16		debounce_max;
	u16		debounce_tol;
	u16		debounce_rep;
	u16		penirq_recheck_delay_usecs;
	bool	swap_xy;

	struct xpt2046_packet	*packet;

	struct spi_transfer	xfer[18];
	struct spi_message	msg[5];
	struct spi_message	*last_msg;
	int		msg_idx;
	int		read_cnt;
	int		read_rep;
	int		last_read;
	int		pendown_iomux_mode;	
	int 	touch_virtualkey_length;
	
	spinlock_t		lock;
	struct timer_list		timer;
	unsigned		pendown:1;	/* P: lock */
	unsigned		pending:1;	/* P: lock */
// FIXME remove "irq_disabled"
	unsigned		irq_disabled:1;	/* P: lock */
	unsigned		disabled:1;
	unsigned		is_suspended:1;

	int			(*filter)(void *data, int data_idx, int *val);
	void		*filter_data;
	void		(*filter_cleanup)(void *data);
	int			(*get_pendown_state)(void);
	int			gpio_pendown;

	void		(*wait_for_sync)(void);
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

volatile struct adc_point gADPoint;
volatile int gFirstIrq;

struct dfr_req {
	u8			command;
	u8			pwrdown;
	u16			dummy;		/* for the pwrdown read */
	__be16			sample;
	struct spi_message	msg;
	struct spi_transfer	xfer[4];
};


/* leave chip selected when we're done, for quicker re-select? */
#if	0
#define	CS_CHANGE(xfer)	((xfer).cs_change = 1)
#else
#define	CS_CHANGE(xfer)	((xfer).cs_change = 0)
#endif

/*--------------------------------------------------------------------------*/

/* The xpt2046 has touchscreen and other sensors.
 * Earlier xpt2046 chips are somewhat compatible.
 */
#define	XPT2046_START			(1 << 7)
#define	XPT2046_A2A1A0_d_y		(1 << 4)	/* differential */
#define	XPT2046_A2A1A0_d_z1		(3 << 4)	/* differential */
#define	XPT2046_A2A1A0_d_z2		(4 << 4)	/* differential */
#define	XPT2046_A2A1A0_d_x		(5 << 4)	/* differential */
#define	XPT2046_A2A1A0_temp0	(0 << 4)	/* non-differential */
#define	XPT2046_A2A1A0_vbatt	(2 << 4)	/* non-differential */
#define	XPT2046_A2A1A0_vaux		(6 << 4)	/* non-differential */
#define	XPT2046_A2A1A0_temp1	(7 << 4)	/* non-differential */
#define	XPT2046_8_BIT			(1 << 3)
#define	XPT2046_12_BIT			(0 << 3)
#define	XPT2046_SER				(1 << 2)	/* non-differential */
#define	XPT2046_DFR				(0 << 2)	/* differential */
#define	XPT2046_PD10_PDOWN		(0 << 0)	/* lowpower mode + penirq */
#define	XPT2046_PD10_ADC_ON		(1 << 0)	/* ADC on */
#define	XPT2046_PD10_REF_ON		(2 << 0)	/* vREF on + penirq */
#define	XPT2046_PD10_ALL_ON		(3 << 0)	/* ADC + vREF on */

#define	MAX_12BIT	((1<<12)-1)

/* leave ADC powered up (disables penirq) between differential samples */
#define	READ_12BIT_DFR(x, adc, vref) (XPT2046_START | XPT2046_A2A1A0_d_ ## x \
	| XPT2046_12_BIT | XPT2046_DFR | \
	(adc ? XPT2046_PD10_ADC_ON : 0) | (vref ? XPT2046_PD10_REF_ON : 0))

#define	READ_Y(vref)	(READ_12BIT_DFR(y,  1, vref))
//#define	READ_Y(vref)	(READ_12BIT_DFR(y,  0, 0))

#define	READ_Z1(vref)	(READ_12BIT_DFR(z1, 1, vref))
#define	READ_Z2(vref)	(READ_12BIT_DFR(z2, 1, vref))

//#define	READ_X(vref)	(READ_12BIT_DFR(x,  0, 0))
#define	READ_X(vref)	(READ_12BIT_DFR(x,  1, vref))

#define	PWRDOWN		(READ_12BIT_DFR(y,  0, 0))	/* LAST */

/* single-ended samples need to first power up reference voltage;
 * we leave both ADC and VREF powered
 */
#define	READ_12BIT_SER(x) (XPT2046_START | XPT2046_A2A1A0_ ## x \
	| XPT2046_12_BIT | XPT2046_SER)

#define	REF_ON	(READ_12BIT_DFR(x, 1, 1))
#define	REF_OFF	(READ_12BIT_DFR(y, 0, 0))

#define DEBOUNCE_MAX 10
#define DEBOUNCE_REP 5
#define DEBOUNCE_TOL 10
#define PENIRQ_RECHECK_DELAY_USECS 1
#define PRESS_MAX 255

#endif
