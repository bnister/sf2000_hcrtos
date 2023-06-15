#include <generated/br2_autoconf.h>
#include <linux/kconfig.h>
#include <mips/cpu.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <kernel/soc/soc_common.h>
#include <kernel/io.h>
#include <kernel/ld.h>
#include "FreeRTOS.h"
#include "portmacro.h"
#include "int_handler.h"
#include "list.h"
#include "task.h"

struct isr_ctrl_blk {
	struct list_head head;
	void (*proc)(uint32_t);
	uint32_t param;
	int type;
#if ( configGENERATE_DETAIL_ISR_RUN_TIME_STATS == 1 )
	uint32_t runtime_counter;
	char name[20];
	UBaseType_t id;
#endif
};

extern unsigned long arch_local_irq_save(void);
extern void arch_local_irq_restore(unsigned long flags);
extern unsigned long SYSIO0;
extern unsigned long MBOX_INTR;
static uint32_t status_reg = ((uint32_t)&SYSIO0 + 0x30);
static uint32_t enable_reg = ((uint32_t)&SYSIO0 + 0x38);
#ifdef CONFIG_SOC_HC16XX
static uint32_t mbox_status_reg = ((uint32_t)&SYSIO0 + 0x20);
#else
static uint32_t mbox_status_reg = 0xffffffff;
#endif
static uint32_t int_stat[10] = { 0 };
static uint32_t enable_reg_bank1 = 0;

static struct list_head isr_head[IRQ_NUM + EIRQ_NUM];
extern StackType_t xISRStack[];
#if ( configGENERATE_DETAIL_ISR_RUN_TIME_STATS == 1 )
static UBaseType_t uxISRID = 1UL;
extern UBaseType_t uxISRNumber;
#if IS_ENABLED(CONFIG_DRV_SOFTIRQ)
static UBaseType_t uxSIRQRunTimeCounter;
#endif
#endif

/* Set configCHECK_FOR_STACK_OVERFLOW to 3 to add ISR stack checking to task
stack checking.  A problem in the ISR stack will trigger an assert, not call the
stack overflow hook function (because the stack overflow hook is specific to a
task stack, not the ISR stack). */
#if (configCHECK_FOR_STACK_OVERFLOW > 2)

/* Don't use 0xa5 as the stack fill bytes as that is used by the kernel for
	the task stacks, and so will legitimately appear in many positions within
	the ISR stack. */
#define portISR_STACK_FILL_BYTE 0xee

const uint8_t ucExpectedStackBytes[] = {
	portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,
	portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,
	portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,
	portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,
	portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,
	portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,
	portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,
	portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,
	portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,
	portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE
};

uint32_t ulSzExpectedStackBytes = sizeof(ucExpectedStackBytes);

#define portCHECK_ISR_STACK()                                                  \
	configASSERT((memcmp((void *)xISRStack, (void *)ucExpectedStackBytes,  \
			     sizeof(ucExpectedStackBytes)) == 0))
#else
/* Define the function away. */
#define portCHECK_ISR_STACK()
#endif /* configCHECK_FOR_STACK_OVERFLOW > 2 */

/* Initialise the GIC for use with the interrupt system */
void vPortInterruptInit(void)
{
	size_t i = 0;
	uint32_t sr = 0;

	sr = mips_getsr();

	for (i = 0; i < ARRAY_SIZE(isr_head); i++) {
		isr_head[i].next = &isr_head[i];
		isr_head[i].prev = &isr_head[i];
	}

	for (i = 0; i < IRQ_NUM; i++) {
		sr = sr & ~(1 << (i + IRQ_NUM));
	}

	mips_setsr(sr);
}

void xPortInterruptEnable(uint32_t irq)
{
	uint32_t status = 0;
	int irq_bank = 0;
	int irq_num = 0;
	uint32_t reg = 0;

	portENTER_CRITICAL();

	status = mips_getsr();

	if (irq >= IRQ_NUM) {
		if (irq == (uint32_t)&MBOX_INTR) {
			status |= 1 << (IRQ_EIRQ2_NUM + IRQ_NUM);
		} else {
			status |= 1 << (IRQ_EIRQ3_NUM + IRQ_NUM);
		}

		irq_num = irq - IRQ_NUM;
		irq_bank = irq_num / 32;
		reg = enable_reg + irq_bank * 4;
		if (irq_bank == 1) {
			enable_reg_bank1 |= (1 << (irq_num % 32));
			mips_put_word(reg, enable_reg_bank1);
		} else {
			mips_put_word(reg, mips_get_word(reg, errp) |
						   (1 << (irq_num % 32)));
		}
	} else {
		status |= 1 << (irq + IRQ_NUM);
	}

	mips_setsr(status);

	portEXIT_CRITICAL();
}


static inline int __xPortInterruptInstallISR(uint32_t irq, void (*fn)(uint32_t), uint32_t param, int type)
{
	struct list_head *head = &isr_head[irq];
	struct isr_ctrl_blk *isrcb = NULL;

	configASSERT(fn != NULL);
	configASSERT(irq < (IRQ_NUM + EIRQ_NUM));

	isrcb = malloc(sizeof(struct isr_ctrl_blk));
	if (NULL == isrcb) {
		return -1;
	}

	memset(isrcb, 0, sizeof(struct isr_ctrl_blk));

	isrcb->proc = fn;
	isrcb->param = param;
	isrcb->type = type;
#if ( configGENERATE_DETAIL_ISR_RUN_TIME_STATS == 1 )
	uxISRID++;
	isrcb->id = uxISRID;
	snprintf(isrcb->name, sizeof(isrcb->name), "irq%03d_%x", (unsigned char)irq, (unsigned int)fn);
#endif

	portENTER_CRITICAL();

	list_add_tail(&isrcb->head, head);

	xPortInterruptEnable(irq);

#if ( configGENERATE_DETAIL_ISR_RUN_TIME_STATS == 1 )
	uxISRNumber++;
#endif
	portEXIT_CRITICAL();

	return 0;
}

int xPortInterruptInstallISR(uint32_t irq, void (*fn)(uint32_t), uint32_t param)
{
	return __xPortInterruptInstallISR(irq, fn, param, 0);
}

int xPortInterruptInstallISR2(uint32_t irq, int (*fn)(int, void *), void *dev)
{
	return __xPortInterruptInstallISR(irq, (void (*)(uint32_t))fn, (uint32_t)dev, 1);
}

UBaseType_t uxPortInterruptSetInterruptMaskFromISR(void)
{
	/* Not support interrupt nesting */
	return 0;
}
/*-----------------------------------------------------------*/

void vPortInterruptClearInterruptMaskFromISR(UBaseType_t uxSavedStatusRegister)
{
	/* Not support interrupt nesting */
	return ;
}

void xPortInterruptDisable(uint32_t irq)
{
	uint32_t status = 0;
	int irq_bank = 0;
	int irq_num = 0;
	uint32_t reg = 0;
	int i = 0;

	portENTER_CRITICAL();
	status = mips_getsr();
	if (irq >= IRQ_NUM) {
		for (i = IRQ_NUM; i < IRQ_NUM + EIRQ3_NUM; i++) {
			if (!list_empty(&isr_head[i])) {
				break;
			}
		}

		if (i == IRQ_NUM + EIRQ3_NUM) {
			status &= ~(1 << (IRQ_EIRQ3_NUM + IRQ_NUM));
		}

		if (irq == (uint32_t)&MBOX_INTR) {
			status &= ~(1 << (IRQ_EIRQ2_NUM + IRQ_NUM));
		}

		irq_num = irq - IRQ_NUM;
		irq_bank = irq_num / 32;
		reg = enable_reg + irq_bank * 4;
		if (irq_bank == 1) {
			enable_reg_bank1 &= (~(1 << (irq_num % 32)));
			mips_put_word(reg, enable_reg_bank1);
		} else {
			mips_put_word(reg,
				      mips_get_word(reg, errp) &
					      (~(1 << (irq_num % 32))));
		}
	} else {
		status &= ~(1 << (irq + IRQ_NUM));
	}

	mips_setsr(status);

	portEXIT_CRITICAL();
}

int xPortInterruptRemoveISR(uint32_t irq, void (*fn)(uint32_t))
{
	struct list_head *ele, *next;
	struct list_head *head = &isr_head[irq];
	struct isr_ctrl_blk *tmp = NULL;
	struct isr_ctrl_blk *isrcb = NULL;

	configASSERT(fn != NULL);
	configASSERT(irq < (IRQ_NUM + EIRQ_NUM));

	portENTER_CRITICAL();

	list_for_each_safe (ele, next, head) {
		tmp = list_entry(ele, struct isr_ctrl_blk, head);
		if ((tmp->type == 0 && fn == tmp->proc) ||
		    (tmp->type == 1 && (uint32_t)fn == tmp->param)) {
			isrcb = tmp;
			break;
		}
	}

	if (isrcb == NULL) {
		portEXIT_CRITICAL();
		return -1;
	}

	list_del(&isrcb->head);
	free(isrcb);

	if (list_empty(head)) {
		xPortInterruptDisable(irq);
	}

#if ( configGENERATE_DETAIL_ISR_RUN_TIME_STATS == 1 )
	uxISRNumber--;
#endif

	portEXIT_CRITICAL();

	return 0;
}

void vPortInterruptYieldISR(uint32_t param)
{
	vTaskSwitchContext();
	mips_biccr(SR_SINT0);
}

void vPortInterruptAppSetupSoftwareInterrupt(void)
{
	xPortInterruptInstallISR(SW0, vPortInterruptYieldISR, 0);
}

/*
 * save the current running __isrcb for debug
 * in case exception happened in isr
 */
volatile struct isr_ctrl_blk *__isrcb = NULL;
static int prvDoIrq(uint32_t irq_no)
{
	struct list_head *ele, *next;
	void (*proc)(uint32_t);
	int (*proc2)(int, void *);
	uint32_t param = 0;
	struct list_head *head;

#if ( configGENERATE_DETAIL_ISR_RUN_TIME_STATS == 1 )
	uint32_t enter_time, leave_time;
	#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
		portALT_GET_RUN_TIME_COUNTER_VALUE( enter_time );
	#else
		enter_time = portGET_RUN_TIME_COUNTER_VALUE();
	#endif
#endif

	if (irq_no >= ARRAY_SIZE(isr_head))
		return -1;

	head = &isr_head[irq_no];

	list_for_each_safe (ele, next, head) {
		__isrcb = list_entry(ele, struct isr_ctrl_blk, head);
		if (__isrcb->type == 0) {
			proc = __isrcb->proc;
			param = __isrcb->param;
			proc(param);
		} else {
			proc2 = (int (*)(int, void *))__isrcb->proc;
			param = __isrcb->param;
			proc2((int)irq_no, (void *)param);
		}
#if ( configGENERATE_DETAIL_ISR_RUN_TIME_STATS == 1 )
		#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
			portALT_GET_RUN_TIME_COUNTER_VALUE( leave_time );
		#else
			leave_time = portGET_RUN_TIME_COUNTER_VALUE();
		#endif
		if (leave_time > enter_time)
			__isrcb->runtime_counter += (leave_time - enter_time);
		enter_time = leave_time;
#endif
	}

	__isrcb = NULL;

	return 0;
}

static void prvNBInterruptHandler(void)
{
	uint32_t nb_irq_no = -1;
	uint32_t irq_no = -1;
	uint32_t i = 0;

	for (i = 0; i < EIRQ_NUM / 32; i++) {
		if (!int_stat[i]) {
			continue;
		}

		for (nb_irq_no = 0; nb_irq_no < 32; nb_irq_no++) {
			if (!(int_stat[i] & (1 << nb_irq_no))) {
				continue;
			}

			nb_irq_no += 32 * i;
			irq_no = nb_irq_no + IRQ_NUM;

			/*
			 * Don't Break here, we handle all the
			 * nb interrupts in one irq handler.
			 */
			configASSERT(prvDoIrq(irq_no) == 0);
		}
	}
}

extern void __do_softirq(void);
void vPortInterruptHandoff(void)
{
	uint32_t cause = 0, status = 0;
	int irq_no = 0;
	int i = 0;

#if ( configGENERATE_RUN_TIME_STATS == 1 )
	uint32_t enter_time, leave_time;
	extern configRUN_TIME_COUNTER_TYPE ulTotalISRRunTime;
	#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
		portALT_GET_RUN_TIME_COUNTER_VALUE( enter_time );
	#else
		enter_time = portGET_RUN_TIME_COUNTER_VALUE();
	#endif
#endif

	cause = mips_getcr();
	status = mips_getsr();

	/* Exception */
	configASSERT((cause & 0x7c) == 0);

	/* No nesting */
	configASSERT(uxInterruptNesting == 0);

	uxInterruptNesting++;

	for (i = 0; i < (EIRQ_NUM / 32) - 1; i++)
		int_stat[i] = mips_get_word(status_reg + i * 4, errp);

	if (mbox_status_reg != (0xffffffff) &&
	    mips_get_word(mbox_status_reg, errp) & 0xff)
		int_stat[i] = (1 << 2);
	else
		int_stat[i] = 0;

	/* Get IP bits from cause and save back to cause */
	cause = (cause & status & 0xff00) >> 8;
	for (irq_no = 0; irq_no < IRQ_NUM; irq_no++) {
		/*check every interrupt pending bit */
		if (!(cause & (1 << irq_no)))
			continue;

		if (IRQ_EIRQ3_NUM == irq_no || IRQ_EIRQ2_NUM == irq_no) {
			prvNBInterruptHandler();
		} else {
			configASSERT(prvDoIrq(irq_no) == 0);
		}
	}

#if IS_ENABLED(CONFIG_DRV_SOFTIRQ)
	{
#if ( configGENERATE_DETAIL_ISR_RUN_TIME_STATS == 1 )
	uint32_t t1, t2;
	#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
		portALT_GET_RUN_TIME_COUNTER_VALUE( t1 );
	#else
		t1 = portGET_RUN_TIME_COUNTER_VALUE();
	#endif
#endif
	__do_softirq();
#if ( configGENERATE_DETAIL_ISR_RUN_TIME_STATS == 1 )
	#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
		portALT_GET_RUN_TIME_COUNTER_VALUE( t2 );
	#else
		t2 = portGET_RUN_TIME_COUNTER_VALUE();
	#endif
	if (t2 > t1)
		uxSIRQRunTimeCounter += (t2 - t1);
#endif
	}
#endif

	uxInterruptNesting--;

	mips_setsr(status);

	portCHECK_ISR_STACK();

#if ( configGENERATE_RUN_TIME_STATS == 1 )
	#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
		portALT_GET_RUN_TIME_COUNTER_VALUE( leave_time );
	#else
		leave_time = portGET_RUN_TIME_COUNTER_VALUE();
	#endif
	if (leave_time > enter_time)
		ulTotalISRRunTime += (leave_time - enter_time);
#endif
}
#if ( configGENERATE_DETAIL_ISR_RUN_TIME_STATS == 1 )
UBaseType_t portGetDetailISRRunTimeStats(TaskStatus_t *pxTaskStatusArray, UBaseType_t uxMaxISRNumber)
{
	unsigned i, j;
	UBaseType_t n = 0;
	struct list_head *head;
	struct list_head *ele, *next;
	struct isr_ctrl_blk *isrcb = NULL;

	pxTaskStatusArray[n].pcTaskName = (const char *)"SIRQinISR";
	pxTaskStatusArray[n].uxCurrentPriority = 0;
	pxTaskStatusArray[n].pxStackBase = NULL;
	pxTaskStatusArray[n].xTaskNumber = 10000;
	pxTaskStatusArray[n].uxBasePriority = 0;
	pxTaskStatusArray[n].ulRunTimeCounter = uxSIRQRunTimeCounter;
	pxTaskStatusArray[n].eCurrentState = eBlocked;
	pxTaskStatusArray[n].usStackHighWaterMark = 0;
	n++;

	for (i = 0; i < ARRAY_SIZE(isr_head); i++) {
		head = &isr_head[i];
		if (list_empty(head)) {
			continue;
		}
		j = 0;
		list_for_each_safe (ele, next, head) {
			isrcb = list_entry(ele, struct isr_ctrl_blk, head);
			if (n >= uxMaxISRNumber)
				return n; /* Prevent overflow */
			pxTaskStatusArray[n].pcTaskName = isrcb->name;
			pxTaskStatusArray[n].uxCurrentPriority = 0;
			pxTaskStatusArray[n].pxStackBase = NULL;
			pxTaskStatusArray[n].xTaskNumber = 10000 + (uint32_t)isrcb->id;
			pxTaskStatusArray[n].uxBasePriority = 0;
			pxTaskStatusArray[n].ulRunTimeCounter = isrcb->runtime_counter;
			pxTaskStatusArray[n].eCurrentState = eBlocked;
			pxTaskStatusArray[n].usStackHighWaterMark = 0;
			j++;
			n++;
		}
	}

	return n;
}
#endif

static uint32_t irq_bank[3] = { 0 };
void irq_save_all(void)
{
	irq_bank[0] = REG32_READ((uint32_t)&SYSIO0 + 0x38);
	irq_bank[1] = enable_reg_bank1;
	irq_bank[2] = REG32_READ((uint32_t)&SYSIO0 + 0x40) & BIT2;

	REG32_WRITE((uint32_t)&SYSIO0 + 0x38, 0);
	REG32_WRITE((uint32_t)&SYSIO0 + 0x3c, 0);
	enable_reg_bank1 = 0;
	REG32_CLR_BIT((uint32_t)&SYSIO0 + 0x40, BIT2);
}

void irq_restore_all(void)
{
	REG32_WRITE((uint32_t)&SYSIO0 + 0x38, irq_bank[0]);
	enable_reg_bank1 = irq_bank[1];
	REG32_WRITE((uint32_t)&SYSIO0 + 0x3c, enable_reg_bank1);
	if (irq_bank[2])
		REG32_SET_BIT((uint32_t)&SYSIO0 + 0x40, BIT2);
}

static uint32_t av_irqs[] = {
	(uint32_t)&H264_INTR,
	(uint32_t)&DE_INTR,
	(uint32_t)&VE_INTR,
	(uint32_t)&DE4K_INTR,
	(uint32_t)&DMA0_INTR,
	(uint32_t)&HDMI_INTR,
	(uint32_t)&I2C0_INTR,
	(uint32_t)&SND_INTR,
};

static uint32_t av_irq_bank[2] = { 0 };
void irq_save_av(void)
{
	uint32_t irq_bank[2] = { 0 };
	uint32_t tmp;
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(av_irqs); i++) {
		tmp = av_irqs[i];
		if (tmp == 0xffffffff)
			continue;
		tmp -= 8;
		irq_bank[tmp / 32] |= BIT(tmp % 32);
	}

	av_irq_bank[0] = REG32_READ((uint32_t)&SYSIO0 + 0x38) & irq_bank[0];
	av_irq_bank[1] = enable_reg_bank1 & irq_bank[1];
	REG32_CLR_BIT((uint32_t)&SYSIO0 + 0x38, irq_bank[0]);
	enable_reg_bank1 &= ~irq_bank[1];
	REG32_WRITE((uint32_t)&SYSIO0 + 0x3c, enable_reg_bank1);
}
