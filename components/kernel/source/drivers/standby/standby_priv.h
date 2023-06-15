#ifndef __STANDBY_PRIV_H
#define __STANDBY_PRIV_H

#include <linux/types.h>
#include <hcuapi/rc-proto.h>
#include <kernel/list.h>

#define STANDBY_ATTR_TEXT __attribute__ ((section(".standby.text")))
#define STANDBY_ATTR_DATA __attribute__ ((section(".standby.data")))
#define STANDBY_ATTR_BSS __attribute__ ((section(".standby.bss")))

void standby_enter(void);
int standby_get_dts_param(void);
int standby_set_wakeup_by_ir(struct standby_ir_setting *setting);
int standby_set_wakeup_by_gpio(struct standby_gpio_setting *setting);
int standby_set_wakeup_by_saradc(struct standby_saradc_setting *setting);
int standby_set_ddr(struct standby_pwroff_ddr_setting *setting);
int standby_release(struct standby_locker *locker);
int standby_request(struct standby_locker *locker);
void standby_lock_list_init(void);
int standby_get_bootup_mode(enum standby_bootup_mode *mode);

extern uint32_t upmode_n;
/* Define the max number of pulse/space transitions to buffer */
#define	MAX_IR_EVENT_SIZE	512

#define IRC_FIFO_THRESHOLD	32

#define US_TO_NS(usec)		((usec) * 1000)
#define MS_TO_US(msec)		((msec) * 1000)
#define IR_MAX_DURATION		MS_TO_US(500)
#define IR_DEFAULT_TIMEOUT	MS_TO_US(125)

/* Registers */
#define IRC_CFG			0x00
#define IRC_FIFOCFG		0x01
#define IRC_TMO_THRESHOLD	0x02
#define IRC_NOISE_THRESHOLD	0x03
#define IRC_IER			0x06
#define IRC_ISR			0x07
#define IRC_FIFO		0x08

/* define bit use in IRC_CFG */
#define IRC_CFG_EN		BIT(7)

/* define bit use in IRC_FIFOCFG */
#define IRC_FIFOCFG_RESET	BIT(7)

/* define bit use in IRC_IER */
#define IRC_IER_FIFO_EN		BIT(0)
#define IRC_IER_TMO_EN		BIT(1)

/* define bit use in IRC_ISR */
#define IRC_ISR_FIFO_ST		BIT(0)
#define IRC_ISR_TMO_ST		BIT(1)

/* Each bit is 8us */
#define BIT_DURATION 8

 /* maximum symbol period (microsecs),timeout to detect end of symbol train */
#define MAX_SYMB_TIME		24000

/* Noise threshold (microsecs) */
#define NOISE_DURATION		80

struct ir_raw_event {
	union {
		u32             duration;
		u32             carrier;
	};
	u8                      duty_cycle;

	unsigned                pulse:1;
	unsigned                reset:1;
	unsigned                timeout:1;
	unsigned                carrier_report:1;
};

struct ir_raw_event_ctrl {
	int kfifo_in;
	int kfifo_out;
	struct ir_raw_event kfifo[MAX_IR_EVENT_SIZE];

	struct ir_raw_event prev_ev;
	struct ir_raw_event this_ev;

	struct nec_dec {
		int state;
		unsigned count;
		u32 bits;
		bool is_nec_x;
		bool necx_repeat;
	} nec;
} raw;

struct locker_list {
	struct list_head list;
	char name[64];
	int cref;
};

struct standby_ctrl {
	struct standby_ir_setting ir;
	struct standby_gpio_setting gpio;
	struct standby_saradc_setting saradc;
	struct standby_pwroff_ddr_setting ddr;
	struct locker_list locker_list_head;
	int gpio_detect_count;
	int saradc_detect_count;
	uint8_t saradc_def_val;
	int ir_enabled : 1;
	int gpio_enabled : 1;
	int saradc_enabled : 1;
	int hdmi_dts_status : 1;
	int ddr_enabled : 1;

	struct rc_dev {
		bool idle;
		u32 timeout;
		struct ir_raw_event_ctrl raw;
	} rdev;
};

#endif
