#include <mips/cpu.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <kernel/soc/soc_common.h>
#include <kernel/ld.h>
#include <kernel/io.h>

#include "FreeRTOS.h"
#include "portmacro.h"
#include "int_handler.h"
#include "list.h"
#include "task.h"

#define US_TICKS (CONFIG_TIMER_CLOCK_HZ / 1000000)

uint32_t gTickHi = 0;
timer5_reg_t *gTmr = (timer5_reg_t *)&TIMER50;
timer3_reg_t *gHrTmr = (timer3_reg_t *)&TIMER30;
timer4_reg_t *gTmr4 = (timer4_reg_t *)&TIMER40;

static inline uint32_t uxPortTimerTickGet(void)
{
	return gTmr->cnt.val;
}

static inline uint32_t uxPortTimerTickCompareGet(void)
{
	return gTmr->aim.val;
}

static inline void uxPortTimerTickCompareSet(unsigned long uxTicks)
{
	gTmr->aim.val = uxTicks;
}

static inline reg_t xTickNewCompareVal(reg_t compare, reg_t count,
                                       TickType_t xJiffies)
{
	if (count - compare > xJiffies)
		compare = count + (xJiffies - (count - compare) % xJiffies);
	else
		compare += xJiffies;

	/*
	 * note: prevent "compare" is too close to current "count".
	 * If the differential is less than 4us, then extend "compare"
	 * with one more extra period.
	 */
	if (compare - count < xJiffies / 16)
		compare += xJiffies;

	return compare;
}

static inline void prvSetNextTick(uint32_t ms)
{
	TickType_t xJiffies;
	reg_t count;
	reg_t compare;

	/* Convert from milliseconds to timer ticks */
	xJiffies = ms * ((TickType_t)configTIMER_RATE_HZ / 1000UL);

	/* Get the last trigger cycle count */
	compare = uxPortTimerTickCompareGet();

	/* Make sure we've not missed the deadline */
	count = uxPortTimerTickGet();

	compare = xTickNewCompareVal(compare, count, xJiffies);
	uxPortTimerTickCompareSet(compare);
}

static void vPortIncrementTick(void)
{
	UBaseType_t uxSavedStatus;

	uxSavedStatus = uxPortInterruptSetInterruptMaskFromISR();
	{
		if (xTaskIncrementTick() != pdFALSE) {
			/* Pend a context switch. */
			portYIELD();
		}
	}
	vPortInterruptClearInterruptMaskFromISR(uxSavedStatus);
}

void vPortTickInterruptHandler(uint32_t param)
{
	if (gTmr->ctrl.int_st) {
		vPortIncrementTick();
		/* Clear timer interrupt. */
		gTmr->ctrl.int_st =  1;
		prvSetNextTick(portTICK_PERIOD_MS);
	}
}

static void vPortTimerStart(void)
{
	gTmr->ctrl.en = 1;
	gTmr->ctrl.int_en = 1;
}

static void vPortTimerInit(void)
{
	/* reset south timer*/
	REG32_WRITE((uint32_t)&TIMER00 + 0x8, BIT3);
	REG32_WRITE((uint32_t)&TIMER10 + 0x8, BIT3);
	REG32_WRITE((uint32_t)&TIMER20 + 0x8, BIT3);
	REG32_WRITE((uint32_t)&TIMER30 + 0x8, BIT3);
	REG32_WRITE((uint32_t)&TIMER40 + 0x8, BIT3);
	REG32_WRITE((uint32_t)&TIMER50 + 0x8, BIT3);
	REG32_WRITE((uint32_t)&TIMER60 + 0x8, BIT3);
	REG32_WRITE((uint32_t)&TIMER70 + 0x8, BIT3);
	gTmr->ctrl.val = 0;
	gTmr->cnt.val = 0;
	gTmr->aim.val = 0;
	gTmr->ctrl.int_st = 1;
	gTmr4->ctrl.int_st = 1;
	REG32_SET_BIT((void *)&MSYSIO0 + 0x7c, BIT6);
}

/* This is defined as weak, as it can (theoretically) be redefined by the
application that we are being built into. */
void vApplicationSetupTickTimerInterrupt(void)
{
	vPortTimerInit();
	vPortPlatformTimeSetup();

	/* Install interrupt handler, starting Tick Timer */
	xPortInterruptInstallISR(SB_TIMER_IRQ, vPortTickInterruptHandler, 0);

	/* Set the periodic interrupt to fire 1ms from now */
	prvSetNextTick(portTICK_PERIOD_MS);
	vPortTimerStart();
}

unsigned long vPortGetSatatisticsTimerValue(void)
{
	static unsigned int high_older = 0;
	static unsigned long low_older = 0;

	/*It is recommended to make the time base
	between 10 and 100 times faster than the tick interrupt.

	The working frequency of the timer is CONFIG_TIMER_CLOCK_HZ( note: 27/16Mhz),
	and the  tick interrupt for OS scheduler is 1ms.

	 (uxPortTimerTickGet() >> 6) & 0xfffffff means Timer tick for performance statistics is about
	 38us (64 * 16 /27 = 38us). So after about 45 hours, the overflow will occur.
	*/
	unsigned long tmp = (uxPortTimerTickGet() >> 6) & 0x3FFFFFF;

	if (low_older > tmp)
		high_older = (high_older + 1) & 0x3F;

	low_older = tmp;

	return (high_older << 26) | low_older;
}

void PortSysClkTimerIntHandler(uint32_t param)
{
	if (gHrTmr->ctrl.overflow) {
		gHrTmr->cnt.val = 0;
		gTickHi++;
		gHrTmr->ctrl.overflow = 1;
	}
}

#define usec2tick(usec) ((usec)*27 / 32)
#define tick2usec(tick) ((tick)*32 / 27)
void vPortPlatformTimeSetup(void)
{
	gHrTmr->ctrl.val = 0;
	gHrTmr->ctrl.overflow = 1;

	xPortInterruptInstallISR(SB_TIMER_IRQ, PortSysClkTimerIntHandler, 0);

	gTickHi = 0;
	gHrTmr->cnt.val = 0;

	/* enable timer and its interrupt */
	gHrTmr->ctrl.en = 1;
	gHrTmr->ctrl.int_en = 1;
}

void vPortPlatformTimeSet(time_t sec, long usec)
{
	uint64_t tmp_tick = usec2tick((uint64_t)sec * 1000000 + (uint64_t)usec);
	uint32_t ticklo;

	/* disable timer and interrupt */
	gHrTmr->ctrl.en = 0;
	gHrTmr->ctrl.int_en = 0;

	gTickHi = tmp_tick >> 32;
	ticklo = tmp_tick & 0xFFFFFFFF;

	gHrTmr->cnt.val = ticklo;

	/* enable timer and interrupt */
	gHrTmr->ctrl.en = 1;
	gHrTmr->ctrl.int_en = 1;

	gHrTmr->ctrl.overflow = 1;
}

void vPortPlatformTimeGet(time_t *sec, long *usec)
{
	uint32_t ticklo = gHrTmr->cnt.val;
	uint64_t tmp_usec =
		tick2usec((((uint64_t)gTickHi) << 32) + (uint64_t)ticklo);

	*sec = tmp_usec / 1000000;
	*usec = tmp_usec % 1000000;
}

uint64_t uxPortTimerHrTickGet(void)
{
	uint32_t ticklo = gHrTmr->cnt.val;
	uint64_t tmp_usec =
		tick2usec((((uint64_t)gTickHi) << 32) + (uint64_t)ticklo);
	return tmp_usec;
}
