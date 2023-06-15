#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <kernel/io.h>
#include <linux/types.h>
#include <kernel/ld.h>
#include <hcuapi/standby.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cpu_func.h>
#include <kernel/lib/fdt_api.h>
#include <hcuapi/pinmux.h>
#include "standby_priv.h"

#define writeb(value, addr) REG8_WRITE(addr, value)
#define readb(addr) REG8_READ(addr)

/*
 * For low level debug
 * CAUTION: Different boards use different UART port
 */
#define __putc(c)                                                              \
	do {                                                                   \
		REG8_WRITE(&UART3, c);                                         \
		while (!REG8_READ((void *)&UART3 + 5) & BIT5)                  \
		;                                                      \
	} while (0)

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

void standby_clock_usb(uint32_t usb_port);
#define configSTANDBY_STACH_SIZE 2048
STANDBY_ATTR_BSS unsigned char standby_stack[configSTANDBY_STACH_SIZE];
STANDBY_ATTR_DATA const char *const standby_stack_top = &(standby_stack[configSTANDBY_STACH_SIZE - 16]);
STANDBY_ATTR_BSS unsigned long standby_save_sp = 0;
STANDBY_ATTR_BSS struct standby_ctrl standby_ctrl = { 0 };
extern void standby_prepare_enter(uint32_t cacheline);
extern void standby_prepare_exit(void);

#define NEC_NBITS		32
#define NEC_UNIT		563  /* us */
#define NEC_HEADER_PULSE	(16 * NEC_UNIT)
#define NECX_HEADER_PULSE	(8  * NEC_UNIT) /* Less common NEC variant */
#define NEC_HEADER_SPACE	(8  * NEC_UNIT)
#define NEC_REPEAT_SPACE	(4  * NEC_UNIT)
#define NEC_BIT_PULSE		(1  * NEC_UNIT)
#define NEC_BIT_0_SPACE		(1  * NEC_UNIT)
#define NEC_BIT_1_SPACE		(3  * NEC_UNIT)
#define	NEC_TRAILER_PULSE	(1  * NEC_UNIT)
#define	NEC_TRAILER_SPACE	(10 * NEC_UNIT) /* even longer in reality */
#define NECX_REPEAT_BITS	1

uint32_t upmode_n = 1;

enum nec_state {
	STATE_INACTIVE,
	STATE_HEADER_SPACE,
	STATE_BIT_PULSE,
	STATE_BIT_SPACE,
	STATE_TRAILER_PULSE,
	STATE_TRAILER_SPACE,
};

STANDBY_ATTR_TEXT u8 __bitrev8(u8 in)
{
	int i = 0;
	u8 out = 0;
	for (i = 0; i < 8; i++) {
		if (in & (1 << i)) {
			out |= (1 << (7 - i));
		}
	}

	return out;
}

/* macros for IR decoders */
STANDBY_ATTR_TEXT bool __geq_margin(unsigned d1, unsigned d2, unsigned margin)
{
	return d1 > (d2 - margin);
}

STANDBY_ATTR_TEXT bool __eq_margin(unsigned d1, unsigned d2, unsigned margin)
{
	return ((d1 > (d2 - margin)) && (d1 < (d2 + margin)));
}

/* Returns true if event is normal pulse/space event */
STANDBY_ATTR_TEXT bool __is_timing_event(struct ir_raw_event ev)
{
	return !ev.carrier_report && !ev.reset;
}

/* Get NEC scancode and protocol type from address and command bytes */
STANDBY_ATTR_TEXT u32 __ir_nec_bytes_to_scancode(u8 address, u8 not_address,
		u8 command, u8 not_command,
		enum rc_proto *protocol)
{
	u32 scancode;

	if ((command ^ not_command) != 0xff) {
		/* NEC transport, but modified protocol, used by at
		 * least Apple and TiVo remotes
		 */
		scancode = not_address << 24 |
			   address     << 16 |
			   not_command <<  8 |
			   command;
		*protocol = RC_PROTO_NEC32;
	} else if ((address ^ not_address) != 0xff) {
		/* Extended NEC */
		scancode = address     << 16 |
			   not_address <<  8 |
			   command;
		*protocol = RC_PROTO_NECX;
	} else {
		/* Normal NEC */
		scancode = address << 8 | command;
		*protocol = RC_PROTO_NEC;
	}

	return scancode;
}

/**
 * standby_ir_nec_decode() - Decode one NEC pulse or space
 * @dev:	the struct rc_dev descriptor of the device
 * @ev:		the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
STANDBY_ATTR_TEXT u32 standby_ir_nec_decode(struct nec_dec *data, struct ir_raw_event ev)
{
	u32 scancode;
	enum rc_proto rc_proto;
	u8 address, not_address, command, not_command;
	u32 rc = 0xffffffff;

	if (!__is_timing_event(ev)) {
		if (ev.reset)
			data->state = STATE_INACTIVE;
		return 0xffffffff;
	}

	do {
		if (data->state == STATE_INACTIVE) {
			if (!ev.pulse)
				break;

			if (__eq_margin(ev.duration, NEC_HEADER_PULSE, NEC_UNIT * 2)) {
				data->is_nec_x = false;
				data->necx_repeat = false;
			} else if (__eq_margin(ev.duration, NECX_HEADER_PULSE, NEC_UNIT / 2))
				data->is_nec_x = true;
			else
				break;

			data->count = 0;
			data->state = STATE_HEADER_SPACE;
			return 0xffffffff;
		} else if (data->state == STATE_HEADER_SPACE) {
			if (ev.pulse)
				break;

			if (__eq_margin(ev.duration, NEC_HEADER_SPACE, NEC_UNIT)) {
				data->state = STATE_BIT_PULSE;
				return 0xffffffff;
			} else if (__eq_margin(ev.duration, NEC_REPEAT_SPACE, NEC_UNIT / 2)) {
				data->state = STATE_TRAILER_PULSE;
				return 0xffffffff;
			}
		} else if (data->state ==  STATE_BIT_PULSE) {
			if (!ev.pulse)
				break;

			if (!__eq_margin(ev.duration, NEC_BIT_PULSE, NEC_UNIT / 2))
				break;

			data->state = STATE_BIT_SPACE;
			return 0xffffffff;
		} else if (data->state == STATE_BIT_SPACE) {
			if (ev.pulse)
				break;

			if (data->necx_repeat && data->count == NECX_REPEAT_BITS &&
					__geq_margin(ev.duration, NEC_TRAILER_SPACE, NEC_UNIT / 2)) {
				//rc_repeat(dev);
				data->state = STATE_INACTIVE;
				return 0xffffffff;
			} else if (data->count > NECX_REPEAT_BITS)
				data->necx_repeat = false;

			data->bits <<= 1;
			if (__eq_margin(ev.duration, NEC_BIT_1_SPACE, NEC_UNIT / 2))
				data->bits |= 1;
			else if (!__eq_margin(ev.duration, NEC_BIT_0_SPACE, NEC_UNIT / 2)) {
				break;
			}

			data->count++;

			if (data->count == NEC_NBITS)
				data->state = STATE_TRAILER_PULSE;
			else
				data->state = STATE_BIT_PULSE;

			return 0xffffffff;
		} else if (data->state == STATE_TRAILER_PULSE) {
			if (!ev.pulse)
				break;

			if (!__eq_margin(ev.duration, NEC_TRAILER_PULSE, NEC_UNIT / 2)) {
				break;
			}

			data->state = STATE_TRAILER_SPACE;
			return 0xffffffff;
		} else if (data->state == STATE_TRAILER_SPACE) {
			if (ev.pulse)
				break;

			if (!__geq_margin(ev.duration, NEC_TRAILER_SPACE, NEC_UNIT / 2))
				break;

			if (data->count == NEC_NBITS) {
				address     = __bitrev8((data->bits >> 24) & 0xff);
				not_address = __bitrev8((data->bits >> 16) & 0xff);
				command	    = __bitrev8((data->bits >>  8) & 0xff);
				not_command = __bitrev8((data->bits >>  0) & 0xff);

				scancode = __ir_nec_bytes_to_scancode(address,
						not_address,
						command,
						not_command,
						&rc_proto);

				if (data->is_nec_x)
					data->necx_repeat = true;

				//rc_keydown(dev, rc_proto, scancode, 0);
				rc = scancode;
			} else {
				//rc_repeat(dev);
			}

			data->state = STATE_INACTIVE;
			return rc;
		}
	} while (0);

	data->state = STATE_INACTIVE;
	return 0xffffffff;
}

#define MHZ2MCTRL2(x) (((((x) * 10) - 27) / 27) | 0x8000)
void STANDBY_ATTR_TEXT standby_set_cpu_clock(void)
{
	hw_watchdog_reset(50000);
	if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1512) {
#if 0
		/* Case 1: using digital pll, the ve clock should keep open */
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x380, 16, 16, MHZ2MCTRL2(27));
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 8, 3, 7);
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x7c, 7, 1, 1);
#else
		/* Case 2: 13.5MHz */
		REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x3c0, BIT0);
#endif
	} else if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
		/* scpu: 24MHz */
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x9c, 8, 3, 5);
		/* mcpu: 24MHz */
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 8, 3, 6);
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 22, 1, 1);
	}
	hw_watchdog_disable();
}

void STANDBY_ATTR_TEXT standby_reset_cpu_clock(void)
{
	REG32_CLR_BIT(0xb8800490, 0x1 << 5);
	hw_watchdog_reset(50000);
	if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1512) {
		/* TODO: reset cpu clock as per strapin (current: 594Mhz) */
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 8, 3, 0);
	} else if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
		/* scpu clock will be reset in hcboot after reset */

		/* TODO: reset mcpu clock as per strapin (current: 900Mhz) */
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 8, 3, 4);
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 22, 1, 1);
	}
	hw_watchdog_disable();
}

int STANDBY_ATTR_TEXT standby_ddr_refresh_mode_exit(void)
{
	REG32_WRITE(0xb883e038, 0x33c00012);
	REG8_WRITE(0xb883e03c, 0x04);
	REG16_WRITE(0xb883e042, 0xf000);

	REG8_WRITE(0xb883e08f, 0x80);
	REG8_WRITE(0xb883e03e, 0xcc);

	REG32_CLR_BIT(0xb8801033, BIT0);

	return 0;
}

int STANDBY_ATTR_TEXT standby_reboot(void)
{
	void *wdt_addr = (void *)&WDT0;

	REG32_CLR_BIT(0xb8800064, BIT1);
	REG32_SET_BIT(0xb8802000, BIT0);
	REG32_WRITE(0xb8802244, 0x80000000);
	REG32_WRITE(0xb8802248, 0x1fffffff);
	REG32_WRITE(0xb8802244, 0x00000000);

	standby_reset_cpu_clock();

	REG32_WRITE(wdt_addr + 4, 0);
	REG32_WRITE(wdt_addr, 0xfffffff0);
	REG32_WRITE(wdt_addr + 4, 0x26);
	asm volatile(".word 0x1000ffff; nop; nop;"); /* Wait for reboot */
}

int STANDBY_ATTR_TEXT standby_exit(void)
{
#ifndef CONFIG_STANDBY_REAL_MODE
	standby_reboot();
#else
	standby_ddr_refresh_mode_exit();
	return 1;
#endif
	return 0;
}

static int standby_ir_init(void)
{
	unsigned int ithr;
	unsigned int nthr;
	unsigned int sampling_freq_select;

	sampling_freq_select = (12 * BIT_DURATION) >> 5;
	writeb(IRC_CFG_EN | sampling_freq_select, (void *)&IRC0 + IRC_CFG);

	writeb(IRC_FIFOCFG_RESET | (IRC_FIFO_THRESHOLD & 0x7f), (void *)&IRC0 + IRC_FIFOCFG);

	ithr = (MAX_SYMB_TIME / (BIT_DURATION << 7)) - 1;
	writeb(ithr, (void *)&IRC0 + IRC_TMO_THRESHOLD);

	nthr = NOISE_DURATION / BIT_DURATION;
	writeb(nthr, (void *)&IRC0 + IRC_NOISE_THRESHOLD);

	writeb(IRC_ISR_FIFO_ST | IRC_ISR_TMO_ST, (void *)&IRC0 + IRC_ISR);

	writeb(IRC_IER_FIFO_EN | IRC_IER_TMO_EN, (void *)&IRC0 + IRC_IER);

	standby_ctrl.rdev.idle = true;
	standby_ctrl.rdev.timeout = IR_DEFAULT_TIMEOUT;

	return 0;
}

#define IER_REG                         0x00
#define RIS_IER_REG                     0x04
#define FALL_IER_REG                    0x08
#define INPUT_ST_REG                    0x0c
#define OUTPUT_VAL_REG                  0x10
#define DIR_REG                         0x14
#define ISR_REG                         0x18

#define DEF_ADC_VAL			1860

static int standby_ddr_init(void)
{
	pinpad_e padctl = standby_ctrl.ddr.pin;

	pinmux_configure(padctl, 0);

	return 0;
}

static int standby_gpio_init(void)
{
	void * ctrlreg[4] = {
		(void *)&GPIOLCTRL,
		(void *)&GPIOBCTRL,
		(void *)&GPIORCTRL,
		(void *)&GPIOTCTRL,
	};

	void * pinmux[4] = {
		(void *)&PINMUXL,
		(void *)&PINMUXB,
		(void *)&PINMUXR,
		(void *)&PINMUXT,
	};

	void *reg;
	pinpad_e padctl = standby_ctrl.gpio.pin;
	uint8_t bit = BIT(padctl % 32);

	pinmux_configure(padctl, 0);

	reg = pinmux[padctl / 32];
	REG32_CLR_BIT(reg, bit);

	reg = ctrlreg[padctl / 32] + DIR_REG;
	REG32_CLR_BIT(reg, bit);

	return 0;
}

#ifdef CONFIG_SOC_HC16XX
static int standby_saradc_init(void)
{
	int ch = standby_ctrl.saradc.channel;
	if (ch > 3)
		ch += 2;

	REG8_WRITE(0xb8818400 + 0x90 +ch, 0x06);
	REG8_WRITE(0xb8818400 + 0x96 +ch, 0x02);
	REG8_WRITE(0xb8818400 + 0xa2 +ch, 0x0a);
	REG8_WRITE(0xb8818400 + 0xa8 +ch, 0x05);
	REG32_SET_BIT(0xb8818400 + 0x8c, 0x1 << (16+ch));
	REG32_SET_BIT(0xb8818400 + 0x88, 0x1 << 6);
	REG32_SET_BIT(0xb8818400 + 0x8c, 0x1 << 15);
	REG8_WRITE(0xb8818400 + 0x73, 0x00);
	REG32_CLR_BIT(0xb8818400 + 0x0c, 0x1 << 0);

	REG32_SET_BIT(0xb8818400 + 0x74, 1);
	while (REG8_READ(0xb88184c0 + ch) == 0);
	//standby_ctrl.saradc_def_val = REG8_READ(0xb88184c0 + ch);
	standby_ctrl.saradc_def_val = 0xf0;
	standby_ctrl.saradc_detect_count = 0;

	return 0;
}
#elif defined(CONFIG_SOC_HC15XX)
static int standby_saradc_init(void)
{
	REG32_WRITE(0xb8818400 + 0x0c, 0x00000000);
	REG32_WRITE(0xb8818400 + 0x08, 0x000001ff);
	REG32_WRITE(0xb8818400 + 0x04, 0x0f000f01);
	//REG32_SET_BIT(0xb8818400 + 0x04, 0);
	while (REG8_READ(0xb8818402) == 0);
	standby_ctrl.saradc_def_val = REG8_READ(0xb8818402);
	return 0;
}
#endif

static int standby_init(void)
{

	if (standby_ctrl.ddr_enabled)
		standby_ddr_init();

	if (standby_ctrl.ir_enabled)
		standby_ir_init();

	if (standby_ctrl.gpio_enabled)
		standby_gpio_init();

	if (standby_ctrl.saradc_enabled)
		standby_saradc_init();

	return 0;
}

int STANDBY_ATTR_TEXT standby_ir_raw_event_store(struct rc_dev *dev, struct ir_raw_event *ev)
{
	if ((dev->raw.kfifo_in + 1) % MAX_IR_EVENT_SIZE == dev->raw.kfifo_out)
		return 0;
	dev->raw.kfifo[dev->raw.kfifo_in++] = *ev;
	dev->raw.kfifo_in %= MAX_IR_EVENT_SIZE;
	return 0;
}

void STANDBY_ATTR_TEXT standby_ir_raw_event_set_idle(struct rc_dev *dev, bool idle)
{
	if (idle) {
		dev->raw.this_ev.timeout = true;
		standby_ir_raw_event_store(dev, &dev->raw.this_ev);
		dev->raw.this_ev = (struct ir_raw_event) {};
	}

	dev->idle = idle;
}

int STANDBY_ATTR_TEXT standby_ir_raw_event_store_with_filter(struct rc_dev *dev, struct ir_raw_event *ev)
{
	/* Ignore spaces in idle mode */
	if (dev->idle && !ev->pulse)
		return 0;
	else if (dev->idle) {
		standby_ir_raw_event_set_idle(dev, false);
	}

	if (!dev->raw.this_ev.duration)
		dev->raw.this_ev = *ev;
	else if (ev->pulse == dev->raw.this_ev.pulse)
		dev->raw.this_ev.duration += ev->duration;
	else {
		standby_ir_raw_event_store(dev, &dev->raw.this_ev);
		dev->raw.this_ev = *ev;
	}

	/* Enter idle mode if necessary */
	if (!ev->pulse && dev->timeout &&
			dev->raw.this_ev.duration >= dev->timeout) {
		standby_ir_raw_event_set_idle(dev, true);
	}

	return 1;
}

void STANDBY_ATTR_TEXT standby_ir_rx_interrupt(struct standby_ctrl *pctrl)
{
	struct ir_raw_event ev = { 0 };
	volatile u8 status, count;
	u8 irdata;

	status = readb((void *)&IRC0 + IRC_ISR) & 0x3;
	writeb(status, (void *)&IRC0 + IRC_ISR);

	if (status) {
		count = readb((void *)&IRC0 + IRC_FIFOCFG) & 0x7f;
		while (count-- > 0) {
			irdata = readb((void *)&IRC0 + IRC_FIFO);
			ev.pulse = (irdata & 0x80) ? false : true;
			ev.duration = ((irdata & 0x7f) + 1) * BIT_DURATION;
			standby_ir_raw_event_store_with_filter(&pctrl->rdev, &ev);
		}
	}

	if (status & IRC_ISR_TMO_ST) {
		standby_ir_raw_event_set_idle(&pctrl->rdev, true);
	}
}

int STANDBY_ATTR_TEXT standby_ir_raw_event_thread(struct standby_ctrl *pctrl)
{
	struct ir_raw_event ev;
	struct ir_raw_event_ctrl *raw = &pctrl->rdev.raw;
	u32 scancode = 0xffffffff;
	int i, ret;

	while (raw->kfifo_in != raw->kfifo_out) {
		ev = raw->kfifo[raw->kfifo_out++];
		raw->kfifo_out %= MAX_IR_EVENT_SIZE;
		scancode = standby_ir_nec_decode(&raw->nec, ev);
#ifdef CONFIG_OUT_HC_SCANCODE
		if (scancode != 0xffffffff)
			printf("scancode = 0x%x\n", scancode);
#endif
		for (i = 0; i < pctrl->ir.num_of_scancode; i++) {
			if (scancode == (u32)pctrl->ir.scancode[i]) {
				ret = standby_exit();
				return ret;
			}
		}
		raw->prev_ev = ev;
	}

	return -1;
}

int STANDBY_ATTR_TEXT standby_ddr_set(struct standby_ctrl *pctrl)
{
	void *ctrlreg[4] = {
		(void *)&GPIOLCTRL,
		(void *)&GPIOBCTRL,
		(void *)&GPIORCTRL,
		(void *)&GPIOTCTRL,
	};

	pinpad_e padctl = pctrl->ddr.pin;
	uint8_t polarity = pctrl->ddr.polarity;
	uint8_t bit = padctl % 32;
	void *reg = ctrlreg[padctl / 32] + DIR_REG;
	uint32_t temp = REG32_READ(reg);

	REG32_WRITE(reg, temp | (1 << bit));

	reg = ctrlreg[padctl / 32] + OUTPUT_VAL_REG;

	temp = REG32_READ(reg);

	if (polarity)
		REG32_WRITE(reg, temp | (1 << bit));
	else
		REG32_WRITE(reg, temp & (~(1 << bit)));

	return -1;
}

int STANDBY_ATTR_TEXT standby_gpio_input_interrupt(struct standby_ctrl *pctrl)
{
	int ret;
	void *ctrlreg[4] = {
		(void *)&GPIOLCTRL,
		(void *)&GPIOBCTRL,
		(void *)&GPIORCTRL,
		(void *)&GPIOTCTRL,
	};

	uint8_t polarity = pctrl->gpio.polarity;
	pinpad_e padctl = pctrl->gpio.pin;

	uint8_t bit = padctl % 32;
	void *reg = ctrlreg[padctl / 32] + DIR_REG;

	REG32_CLR_BIT(reg, bit);

	bit = padctl % 32;
	reg = ctrlreg[padctl / 32] + INPUT_ST_REG;

	if (((REG32_READ(reg) >> bit) & 0x1) == polarity)
		pctrl->gpio_detect_count++;
	else
		pctrl->gpio_detect_count = 0;

	if (pctrl->gpio_detect_count > 3) {
		ret = standby_exit();
		return ret;
	}

	return -1;
}

#ifdef CONFIG_SOC_HC16XX
int STANDBY_ATTR_TEXT standby_saradc_interrupt(struct standby_ctrl *pctrl)
{
	int ret;
	int ch = pctrl->saradc.channel;
	if (ch > 3)
		ch += 2;
	uint8_t val = REG8_READ(0xb88184c0 + ch);
	int min = (0xf1 * pctrl->saradc.min) / DEF_ADC_VAL;
	int max = (0xf1 * pctrl->saradc.max) / DEF_ADC_VAL;

	if ((val >= min) && (val <= max))
		pctrl->saradc_detect_count++;
	else
		pctrl->saradc_detect_count = 0;

	if (pctrl->saradc_detect_count > 3) {
		ret = standby_exit();
		return ret;
	}

	return -1;
}

#elif defined(CONFIG_SOC_HC15XX)
int STANDBY_ATTR_TEXT standby_saradc_interrupt(struct standby_ctrl *pctrl)
{
	int min = pctrl->saradc.min;
	int max = pctrl->saradc.max;
	uint8_t val = REG8_READ(0xb8818402);
	uint8_t def_val = pctrl->saradc_def_val;
	int Val = val * DEF_ADC_VAL;
	int ret;
	if ((Val >= min * def_val) &&
			(Val <= max *def_val))
		pctrl->saradc_detect_count++;
	else
		pctrl->saradc_detect_count = 0;

	if (pctrl->saradc_detect_count > 3) {
		ret = standby_exit();
		return ret;
	}
}

#endif
int STANDBY_ATTR_TEXT standby_wakeup_ir_monitor(struct standby_ctrl *pctrl)
{
	standby_ir_rx_interrupt(pctrl);
	return standby_ir_raw_event_thread(pctrl);
}

int STANDBY_ATTR_TEXT standby_wakeup_gpio_monitor(struct standby_ctrl *pctrl)
{
	standby_gpio_input_interrupt(pctrl);
	return -1;
}

int STANDBY_ATTR_TEXT standby_wakeup_saradc_monitor(struct standby_ctrl *pctrl)
{
	standby_saradc_interrupt(pctrl);
	return -1;
}

#if 1
void STANDBY_ATTR_TEXT standby_clock_usb(uint32_t usb_port)
{
#undef USB_O_UTMI
#define USB_O_UTMI 0x00000380
	uint32_t *musb_base;
	uint32_t reg;

	if(usb_port == 0)
		musb_base = (uint32_t *)&USB0;
	else if(usb_port == 1)
		musb_base = (uint32_t *)&USB1;
	else{
		//log_e("Error parameter, only support USB0 or USB1\n");
		return;
	}

	/* setup as HOST mode */
	REG8_WRITE(musb_base + USB_O_UTMI, 0x1 << 7);
	usleep(1000); // delay 1 ms
	reg = REG8_READ(musb_base + USB_O_UTMI);
	reg |= ((0x1 << 7) | (0x3 << 2)); // disable OTG function
	reg &= ~(0x1 << 4); // set as host mode
	REG8_WRITE(musb_base + USB_O_UTMI, reg);

	/* setup SUSPEND bit */
	reg = REG8_READ(musb_base + 0x01);
	reg |= (0x1 << 1);
	REG8_WRITE(musb_base + 0x01, reg);
}
#endif 

#if defined(CONFIG_SOC_HC16XX)
#ifndef CONFIG_STANDBY_REAL_MODE
void STANDBY_ATTR_TEXT standby_close_clocks(void)
{
	REG32_WRITE_SYNC(0xbb8800060, 0x00000000);
	REG32_WRITE_SYNC(0xbb8800064, 0x00000000);
	REG32_WRITE_SYNC(0xbb880008c, 0x00000000);

	REG32_WRITE(0xb8800080, 0x11000000);
	REG32_WRITE(0xb8800084, 0x00c20000);
	//REG32_WRITE(0xb8800080, 0xff07ffff);
	//REG32_WRITE(0xb8800084, 0xa3efffff);

	REG32_SET_BIT(0xb8800490, 0x1 << 5);

	REG32_SET_BIT(0xb8800470, 0x1 << 5);
	REG32_SET_BIT(0xb8800470, 0x1 << 4);
	REG32_SET_BIT(0xb8800470, 0x1 << 3);

	REG32_CLR_BIT(0xb8800380, 0x1 << 31);
	REG32_SET_BIT(0xb8800380, 0x1 << 30);

	REG32_CLR_BIT(0xb88003b0, 0x1 << 31);
	REG32_SET_BIT(0xb88003b0, 0x1 << 30);

	REG32_CLR_BIT(0xb8800390, 0x1 << 31);
	REG32_SET_BIT(0xb8800390, 0x1 << 30);

	REG32_CLR_BIT(0xb88003a0, 0x1 << 31);
	REG32_SET_BIT(0xb88003a0, 0x1 << 30);

	REG32_SET_BIT(0xb8800200, 0x1 << 16);

	REG32_SET_BIT(0xb8800184, 0x1 << 25);
	REG32_CLR_BIT(0xb8800184, 0x1 << 23);
	REG32_CLR_BIT(0xb8800184, 0x1 << 24);

	REG32_SET_BIT(0xb8804004, 0x1 << 0);
	REG32_SET_BIT(0xb8804084, 0x07 << 8);

	REG32_SET_BIT(0xb8800440, 0x1 << 2);
	REG32_CLR_BIT(0xb886010f, 0x1 << 3);
	REG32_CLR_BIT(0xb886014f, 0x1 << 3);

	REG32_SET_BIT(0xb8800064, 0x1 << 28);

	REG32_SET_BIT(0xb8800180, 0x1 << 24);
	REG32_SET_BIT(0xb8800180, 0x1 << 25);
	REG32_CLR_BIT(0xb8800180, 0x1 << 26);
	REG32_SET_BIT(0xb8800180, 0x1 << 27);
	REG32_SET_BIT(0xb8800180, 0x1 << 28);
	REG32_SET_BIT(0xb8800180, 0x1 << 29);

	//REG32_SET_BIT(0xb881840c, 0x1 << 0);
	//REG32_CLR_BIT(0xb881840c, 0x1 << 5);

	REG32_SET_BIT(0xb8814030, 0x1 << 3);
	REG32_SET_BIT(0xb8814030, 0x1 << 4);
	REG32_SET_BIT(0xb8814010, 0x1 << 20);
	REG32_CLR_BIT(0xb8814010, 0x1 << 18);
	REG32_SET_BIT(0xb881401c, 0x1 << 31);
	REG32_CLR_BIT(0xb881401c, 0x1 << 24);
	REG32_SET_BIT(0xb8814014, 0x1 << 23);
	REG32_CLR_BIT(0xb8814014, 0x1 << 20);
	REG32_CLR_BIT(0xb8814014, 0x1 << 28);
	REG32_CLR_BIT(0xb8814018, 0x1 << 4);
	REG32_SET_BIT(0xb8814030, 0x1 << 23);
	REG32_CLR_BIT(0xb8814030, 0x1 << 16);
	REG32_CLR_BIT(0xb8814030, 0x1 << 17);
	REG32_CLR_BIT(0xb8814030, 0x1 << 18);
	REG32_SET_BIT(0xb8814000, 0x1 << 7);
	REG32_CLR_BIT(0xb8814020, 0x1 << 6);
	REG32_CLR_BIT(0xb8814020, 0x1 << 14);
	REG32_CLR_BIT(0xb8814020, 0x1 << 22);
	REG32_SET_BIT(0xb8814010, 0x1 << 11);
	REG32_CLR_BIT(0xb8814010, 0x1 << 8);
	REG32_CLR_BIT(0xb8814010, 0x1 << 9);
	REG32_CLR_BIT(0xb8814010, 0x1 << 10);

	REG32_SET_BIT(0xb8800060, 0x1 << 0);

	REG32_SET_BIT(0xb8800060, 0x1 << 2);
	REG32_SET_BIT(0xb8800060, 0x1 << 1);

	REG32_SET_BIT(0xb8800064, 0x1 << 24);
	REG32_SET_BIT(0xb8800064, 0x1 << 25);

	REG32_SET_BIT(0xb8800060, 0x1 << 11);

	REG32_SET_BIT(0xb8800060, 0x1 << 13);
	REG32_SET_BIT(0xb8800063, 0x1 << 1);

	REG32_SET_BIT(0xb8800064, 0x1 << 23);

	REG32_SET_BIT(0xb880008c, 0x1 << 0);
	REG32_SET_BIT(0xb880008c, 0x1 << 1);
	REG32_SET_BIT(0xb880008c, 0x1 << 2);
	REG32_SET_BIT(0xb880008c, 0x1 << 3);

	REG32_SET_BIT(0xb8800064, 0x01 << 2);
	REG32_SET_BIT(0xb8800064, 0x01 << 7);
	REG32_SET_BIT(0xb8800064, 0x01 << 18);
	REG32_SET_BIT(0xb8800064, 0x01 << 17);

	REG32_SET_BIT(0xb8800064, 0x01 << 5);
	REG32_SET_BIT(0xb8800064, 0x01 << 3);
	REG32_SET_BIT(0xb8800064, 0x01 << 8);
	REG32_SET_BIT(0xb8800064, 0x01 << 9);
	REG32_SET_BIT(0xb8800064, 0x01 << 10);

	REG32_SET_BIT(0xb880008c, 0x01 << 14);
	REG32_SET_BIT(0xb880008c, 0x01 << 27);

	REG32_SET_BIT(0xb880008c, 0x01 << 8);
	REG32_SET_BIT(0xb880008c, 0x01 << 7);

	REG32_SET_BIT(0xb8800064, 0x01 << 30);
	REG32_SET_BIT(0xb880008c, 0x01 << 10);

	REG32_SET_BIT(0xb880008c, 0x01 << 11);
	REG32_SET_BIT(0xb880008c, 0x01 << 12);

	REG32_SET_BIT(0xb880008c, 0x01 << 9);
	REG32_SET_BIT(0xb8800064, 0x01 << 6);

	REG32_SET_BIT(0xb880008c, 0x01 << 26);
	REG32_SET_BIT(0xb880008c, 0x01 << 25);
	REG32_SET_BIT(0xb880008c, 0x01 << 24);
	REG32_SET_BIT(0xb880008c, 0x01 << 23);
	REG32_SET_BIT(0xb880008c, 0x01 << 22);
	REG32_SET_BIT(0xb880008c, 0x01 << 21);
	REG32_SET_BIT(0xb880008c, 0x01 << 20);
	REG32_SET_BIT(0xb880008c, 0x01 << 19);
	REG32_SET_BIT(0xb880008c, 0x01 << 18);
	REG32_SET_BIT(0xb880008c, 0x01 << 17);
	REG32_SET_BIT(0xb880008c, 0x01 << 16);
	REG32_SET_BIT(0xb8800064, 0x01 << 15);

	REG32_SET_BIT(0xb8800064, 0x01 << 16);

	REG32_SET_BIT(0xb880008c, 0x01 << 14);

	REG32_SET_BIT(0xb8800064, 0x01 << 12);
	REG32_SET_BIT(0xb8800064, 0x01 << 13);

	REG32_SET_BIT(0xb8800064, 0x01 << 22);

	REG32_SET_BIT(0xb8800060, 0x01 << 3);

	REG32_SET_BIT(0xb8800060, 0x01 << 10);
	REG32_SET_BIT(0xb8800060, 0x01 << 9);
	REG32_SET_BIT(0xb8800060, 0x01 << 8);
	REG32_SET_BIT(0xb8800060, 0x01 << 7);

#if 0
	REG8_WRITE(0xb8801004, 0x10);
	volatile uint32_t i  = 999999;
	while (i--);
	*((volatile uint8_t *)0xb8818300) = 0x32;

	REG8_WRITE(0xb8801004, 0x00);
	REG32_CLR_BIT(0xb8801033, 0x1 << 5);
	REG8_WRITE(0xb883e01b, 0x20);
	REG32_WRITE(0xb883e038, 0x3bc4011a);
	REG8_WRITE(0xb883e03c, 0x1c);
	REG16_WRITE(0xb883e042, 0xf110);

	REG32_WRITE_SYNC(0xbb8800060, 0x0e0f2f8e);
	REG32_WRITE_SYNC(0xbb8800064, 0x7fffffff);
	REG32_WRITE_SYNC(0xbb880008c, 0xffffffff);

	/* close ddr clock */
	REG32_SET_BIT(0xb8800060, 0x1 << 15);

	//REG8_WRITE(0xb8800358, 0x1);
	standby_ddr_set(&standby_ctrl);
#endif
}
#else //CONFIG_STANDBY_REAL_MODE
void STANDBY_ATTR_TEXT standby_close_clocks(void)
{
	/*DDR enter Clean up redundant code*/
	REG32_SET_BIT(0xb8801033, BIT0);
	REG32_WRITE(0xb883e038, 0x3bc4011a);
	REG8_WRITE(0xb883e03c, 0x1c);
	REG16_WRITE(0xb883e042, 0xf110);

}
#endif //CONFIG_STANDBY_REAL_MODE
#else //CONFIG_SOC_HC16XX

void STANDBY_ATTR_TEXT standby_close_clocks(void)
{
	REG32_WRITE_SYNC(0xbb8800060, 0x00000000);
	REG32_WRITE_SYNC(0xbb8800064, 0x00000000);

	REG32_SET_BIT(0xb8800080, 0x1 << 24);
	REG32_SET_BIT(0xb8800084, 0x1 << 29);
	REG32_SET_BIT(0xb8800084, 0x1 << 17);
	REG32_SET_BIT(0xb8818200, 0x1 << 16);

	REG32_SET_BIT(0xb8804004, 0x1 << 1);
	REG32_SET_BIT(0xb8804084, 0x7 << 8);

	REG32_SET_BIT(0xb880b000, 0x7 << 16);

	REG32_CLR_BIT(0xb8800380, 0x1 << 31);
	REG32_SET_BIT(0xb8800380, 0x1 << 30);

	if (standby_ctrl.hdmi_dts_status) {
		REG32_CLR_BIT(0xb882c1b2, 0x1 << 0);
		REG32_CLR_BIT(0xb882c1b2, 0x1 << 1);
		REG32_CLR_BIT(0xb882c1b2, 0x1 << 2);
		REG32_CLR_BIT(0xb882c1b2, 0x1 << 3);

		REG32_CLR_BIT(0xb882c1c2, 0x1 << 0);
		REG32_CLR_BIT(0xb882c1c2, 0x1 << 1);
		REG32_CLR_BIT(0xb882c1c2, 0x1 << 2);

		REG32_CLR_BIT(0xb882c1cc, 0x1 << 0);
		REG32_CLR_BIT(0xb882c1cc, 0x1 << 1);
		REG32_CLR_BIT(0xb882c1cc, 0x1 << 2);
		REG32_CLR_BIT(0xb882c1cc, 0x1 << 3);

		REG32_CLR_BIT(0xb882c1b4, 0x1 << 0);
		REG32_CLR_BIT(0xb882c1b4, 0x1 << 1);
		REG32_CLR_BIT(0xb882c1b4, 0x1 << 2);

		REG32_SET_BIT(0xb882c1a0, 0x1 << 0);
		REG32_SET_BIT(0xb882c1aa, 0x1 << 0);

		REG32_CLR_BIT(0xb882c1b0, 0x1 << 2);
		REG32_CLR_BIT(0xb882c1b0, 0x1 << 3);
	}

	REG32_SET_BIT(0xb8800470, 0x1 << 5);
	REG32_SET_BIT(0xb8800470, 0x1 << 4);
	REG32_SET_BIT(0xb8800470, 0x1 << 3);

	REG32_SET_BIT(0xb8800060, 0x1 << 26);
	REG32_SET_BIT(0xb8800060, 0x1 << 25);
	REG32_SET_BIT(0xb8800060, 0x1 << 18);
	REG32_SET_BIT(0xb8800060, 0x1 << 17);
	REG32_SET_BIT(0xb8800060, 0x1 << 16);

	REG32_SET_BIT(0xb8800060, 0x1 << 19);
	REG32_SET_BIT(0xb8800064, 0x1 << 27);
	REG32_SET_BIT(0xb8800064, 0x1 << 14);


	REG32_SET_BIT(0xb8800060, 0x1 << 10);
	REG32_SET_BIT(0xb8800060, 0x1 << 9);
	REG32_SET_BIT(0xb8800060, 0x1 << 8);
	REG32_SET_BIT(0xb8800060, 0x1 << 7);

	REG32_SET_BIT(0xb8800060, 0x1 << 3);
	REG32_SET_BIT(0xb8800060, 0x1 << 1);

	REG32_SET_BIT(0xb8800060, 0x1 << 0);

	REG32_SET_BIT(0xb8800064, 0x1 << 29);
	REG32_SET_BIT(0xb8800064, 0x1 << 25);
	REG32_SET_BIT(0xb8800064, 0x1 << 24);
	REG32_SET_BIT(0xb8800064, 0x1 << 23);
	REG32_SET_BIT(0xb8800064, 0x1 << 22);
	REG32_SET_BIT(0xb8800064, 0x1 << 21);
	REG32_SET_BIT(0xb8800064, 0x1 << 20);
	REG32_SET_BIT(0xb8800064, 0x1 << 15);
	REG32_SET_BIT(0xb8800064, 0x1 << 0);
	REG32_SET_BIT(0xb8800064, 0x1 << 18);
	REG32_SET_BIT(0xb8800064, 0x1 << 17);
	REG32_SET_BIT(0xb8800064, 0x1 << 2);
	REG32_SET_BIT(0xb8800064, 0x1 << 7);
	REG32_SET_BIT(0xb8800064, 0x1 << 5);
	REG32_SET_BIT(0xb8800064, 0x1 << 3);
	REG32_SET_BIT(0xb8800064, 0x1 << 12);
	REG32_SET_BIT(0xb8800064, 0x1 << 11);
	REG32_SET_BIT(0xb8800064, 0x1 << 10);
	REG32_SET_BIT(0xb8800064, 0x1 << 9);
	REG32_SET_BIT(0xb8800064, 0x1 << 8);
	REG32_SET_BIT(0xb8800064, 0x1 << 6);
	REG32_SET_BIT(0xb8800064, 0x1 << 4);
	REG32_SET_BIT(0xb8800064, 0x1 << 1);

	//REG8_WRITE(0xb8801004, 0x10);
	//volatile uint32_t i  = 999999;
	//while (i--);
	//*((volatile uint8_t *)0xb8818300) = 0x32;
	REG8_WRITE(0xb8801004, 0x00);
	REG32_CLR_BIT(0xb8801033, 0x1 << 5);
	REG32_SET_BIT(0xb8801000, 0x9 << 10);
	REG32_SET_BIT(0xb8801000, 0x1 << 14);

	while ((REG32_READ(0xb8801000) >> 15 && 0x01) != 1);
	/* close ddr clock */
	REG32_SET_BIT(0xb8800060, 0x1 << 15);

	REG32_WRITE_SYNC(0xbb8800060, 0x060f07fe);
	REG32_WRITE_SYNC(0xbb8800064, 0xffffffff);

	standby_ddr_set(&standby_ctrl);

	return ;
}
#endif

int STANDBY_ATTR_TEXT standby_wakeup_monitor(struct standby_ctrl *pctrl)
{
	int ret = 0;

#ifndef CONFIG_OUT_HC_SCANCODE
	standby_close_clocks();
#endif

	while (1) {
		if (pctrl->ir_enabled)
			ret = standby_wakeup_ir_monitor(pctrl);
		if (pctrl->gpio_enabled)
			ret = standby_wakeup_gpio_monitor(pctrl);
		if (pctrl->saradc_enabled)
			ret = standby_wakeup_saradc_monitor(pctrl);

		if (ret == 1)
			break;
	}

	/* Never return, just reboot in case waked up */
	return 0;
}

int standby_lock_traverse(void);
void STANDBY_ATTR_TEXT standby_enter_reboot(void)
{
	uint32_t cacheline = 16;

	taskENTER_CRITICAL();

	if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1512) {
		cacheline = 16;
	} else if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
		if (get_processor_id() == 0)
			cacheline = 16;
		else
			cacheline = 32;
	}

	standby_set_cpu_clock();
	REG32_WRITE(0xb8800064, 0x00000000);
	standby_clock_usb(0);
	standby_clock_usb(1);
	standby_init();
	standby_prepare_enter(cacheline);
	standby_wakeup_monitor(&standby_ctrl);
	//standby_reboot();
}

void STANDBY_ATTR_TEXT standby_enter_real(void)
{
	uint32_t cacheline = 16;

	taskENTER_CRITICAL();

	if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1512) {
		cacheline = 16;
	} else if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
		if (get_processor_id() == 0)
			cacheline = 16;
		else
			cacheline = 32;
	}

	//standby_set_cpu_clock();
	//standby_clock_usb(0);
	//standby_clock_usb(1);
	standby_init();
	standby_prepare_enter(cacheline);
	standby_wakeup_monitor(&standby_ctrl);
	standby_prepare_exit();
	taskEXIT_CRITICAL();
}

void standby_enter(void) 
{
	if (!list_empty(&standby_ctrl.locker_list_head.list)) {
		printf("enter standby fail\n");
		standby_lock_traverse();
		return;
	}

#ifndef CONFIG_STANDBY_REAL_MODE
	standby_enter_reboot();
#else
	standby_enter_real();
#endif
}

int standby_set_wakeup_by_ir(struct standby_ir_setting *setting)
{
	memcpy(&standby_ctrl.ir, setting, sizeof(*setting));
	standby_ctrl.ir_enabled = 1;
	return 0;
}

int standby_set_wakeup_by_gpio(struct standby_gpio_setting *setting)
{
	memcpy(&standby_ctrl.gpio, setting, sizeof(*setting));
	standby_ctrl.gpio_enabled = 1;
	return 0;
}

int standby_set_wakeup_by_saradc(struct standby_saradc_setting *setting)
{
	memcpy(&standby_ctrl.saradc, setting, sizeof(*setting));
	standby_ctrl.saradc_enabled = 1;
	return 0;
}

int standby_set_ddr(struct standby_pwroff_ddr_setting *setting)
{
	memcpy(&standby_ctrl.ddr, setting, sizeof(*setting));
	standby_ctrl.ddr_enabled = 1;
	return 0;
}

int standby_get_dts_param(void)
{
	int np;
	const char *st;
	unsigned int i, gpio_pinpad, gpio_polarity = 0, num_pins = 0;
	u32 pin;
	int channel, adc_min, adc_max;

	np = fdt_node_probe_by_path("/hcrtos/hdmi");
	if (np < 0) {
		standby_ctrl.hdmi_dts_status = 0;
	} else {
		fdt_get_property_string_index(np, "status", 0, &st);

		if (!strcmp(st, "okay"))
			standby_ctrl.hdmi_dts_status = 1;
		else 
			standby_ctrl.hdmi_dts_status = 0;
	}

	np = fdt_node_probe_by_path("/hcrtos/standby");
	if (np < 0) {
		return -1;
	}

	if (fdt_get_property_u_32_index(np, "gpio", 0, &gpio_pinpad) != 0)
		standby_ctrl.gpio_enabled = 0;
	else {
		if (fdt_get_property_u_32_index(np, "gpio", 1, &gpio_polarity) != 0)
			standby_ctrl.gpio_enabled = 0;
		else {
			standby_ctrl.gpio.pin = gpio_pinpad;
			standby_ctrl.gpio.polarity = gpio_polarity;
			standby_ctrl.gpio_enabled = 1;
		}
	}

	if (fdt_get_property_u_32_index(np, "ddr-gpio", 0, &gpio_pinpad) == 0) {
		if (fdt_get_property_u_32_index(np, "ddr-gpio", 1, &gpio_polarity) == 0) {
			standby_ctrl.ddr.pin = gpio_pinpad;
			standby_ctrl.ddr.polarity = gpio_polarity;
			standby_ctrl.ddr_enabled = 1;
		}
	}

	if (fdt_get_property_data_by_name(np, "ir", &num_pins) == NULL) {
		standby_ctrl.ir_enabled = 0;
	} else {
		standby_ctrl.ir_enabled = 1;
		num_pins >>= 2;

		standby_ctrl.ir.num_of_scancode = num_pins;

		for (i = 0; i < num_pins; i++) {
			fdt_get_property_u_32_index(np, "ir", i, &pin);
			standby_ctrl.ir.scancode[i] = pin;
			if (i >= 16)
				break;
		}
	}

	if (fdt_get_property_data_by_name(np, "adc", &num_pins) == NULL)
		standby_ctrl.saradc_enabled = 0;		
	else {
		num_pins >>= 2;
		if (num_pins == 3) {
			fdt_get_property_u_32_index(np, "adc", 0, (u32 *)&channel);	
			fdt_get_property_u_32_index(np, "adc", 1, (u32 *)&adc_min);	
			fdt_get_property_u_32_index(np, "adc", 2, (u32 *)&adc_max);	
			standby_ctrl.saradc_enabled = 1;		

			standby_ctrl.saradc.channel = channel;
			standby_ctrl.saradc.min = adc_min;
			standby_ctrl.saradc.max = adc_max;
		}
	}	

	return 0;
}

void standby_lock_list_init(void)
{
	INIT_LIST_HEAD(&standby_ctrl.locker_list_head.list);
}

int standby_get_bootup_mode(enum standby_bootup_mode *mode)
{
	if (upmode_n == 0x1fffffff)
		*mode = STANDBY_BOOTUP_WARM_BOOT;
	else
		*mode = STANDBY_BOOTUP_COLD_BOOT;
	return 0;
}

int standby_release(struct standby_locker *locker)
{
	struct locker_list *t_list;

	list_for_each_entry(t_list, &standby_ctrl.locker_list_head.list, list) {
		if (strcmp(t_list->name, locker->name) == 0) {
			if (!--t_list->cref) {
				list_del(&t_list->list);
				free(t_list);
			}
			return 0;
		}
	}

	return -1;
}

int standby_request(struct standby_locker *locker)
{
	struct locker_list *t_list;
	struct locker_list *s_list;

	list_for_each_entry(t_list, &standby_ctrl.locker_list_head.list, list) {
		if (strcmp(t_list->name, locker->name) == 0) {
			t_list->cref++;
			return 0;
		}
	}

	s_list = (struct locker_list *)malloc(sizeof(struct locker_list));
	s_list->cref = 1;
	strcpy(s_list->name, locker->name);

	list_add_tail(&s_list->list, &standby_ctrl.locker_list_head.list);	
	return 0;
}

int standby_lock_traverse(void)
{
	struct locker_list *t_list;

	list_for_each_entry(t_list, &standby_ctrl.locker_list_head.list, list) {
		printf("name: %s, ctef: %d\n",t_list->name, t_list->cref);
	}

}
