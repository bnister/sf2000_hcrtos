#include <mips/hal.h>
#include <mips/m32c0.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <cpu_func.h>
/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include <kernel/io.h>
#include <kernel/ld.h>
#include <hcuapi/watchdog.h>
#include <sys/errno.h>
#include <kernel/list.h>

/* Interrupt handler */
#include "int_handler.h"
#include "portmacro.h"
#include "message_buffer.h"

/* Let the user override the pre-loading of the initial RA with the address of
prvTaskExitError() in case is messes up unwinding of the stack in the
debugger - in which case configTASK_RETURN_ADDRESS can be defined as 0 (NULL). */
#ifdef configTASK_RETURN_ADDRESS
#define portTASK_RETURN_ADDRESS configTASK_RETURN_ADDRESS
#else
#define portTASK_RETURN_ADDRESS prvTaskExitError
#endif

/*
 * Place the prototype here to ensure the interrupt vector is correctly installed.
 * Note that because the interrupt is written in assembly, the IPL setting in the
 * following line of code has no effect.  The interrupt priority is set by the
 * call to ConfigIntTimer1() in vApplicationSetupTickTimerInterrupt().
 */
extern void vPortTickInterruptHandler(uint32_t);

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void prvTaskExitError(void);

/* Records the interrupt nesting depth. - unused currently as no int stack */
volatile UBaseType_t uxInterruptNesting = 0x0;

/* Stores the task stack pointer when a switch is made to use the system stack. */
UBaseType_t uxSavedTaskStackPointer = 0;

/* The stack used by interrupt service routines that cause a context switch. */
StackType_t xISRStack[configISR_STACK_SIZE] = { 0 };

extern const uint8_t ucExpectedStackBytes[];
extern uint32_t ulSzExpectedStackBytes;

/* The top of stack value ensures there is enough space to store 6 registers on
the callers stack, as some functions seem to want to do this. */
const StackType_t *const xISRStackTop = &(xISRStack[configISR_STACK_SIZE - 7]);

void *pvPortMalloc(size_t xSize)
{
	void *p;
	p = malloc(xSize);
	if(p)
		memset(p, 0 ,  xSize);
	return p;

}

void vPortFree(void *pv)
{
	return free(pv);
}

/*
 * See header file for description.
 */
#if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack,
				   StackType_t *pxEndOfStack,
				   TaskFunction_t pxCode, void *pvParameters)
#else
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack,
				   TaskFunction_t pxCode, void *pvParameters)
#endif
{
	*pxTopOfStack = (StackType_t)0xDEADBEEF;
	pxTopOfStack--;

	*pxTopOfStack =
		(StackType_t)0x12345678; /* Word to which the stack pointer will be left pointing after context restore. */
	pxTopOfStack--;

	/* create a space for a full context */
	pxTopOfStack -= CTX_SIZE / 4;

	/* fill up some initial values for us to kick in */
	*(pxTopOfStack + CTX_CAUSE / 4) = (StackType_t)mips_getcr();
	*(pxTopOfStack + CTX_STATUS / 4) = (StackType_t)(
	        mips32_get_c0(C0_STATUS) | portINITIAL_SR | SR_TIMER_IRQ);
	*(pxTopOfStack + CTX_EPC / 4) = (StackType_t)pxCode;
	*(pxTopOfStack + CTX_RA / 4) = (StackType_t)portTASK_RETURN_ADDRESS;
	*(pxTopOfStack + CTX_A0 / 4) =
		(StackType_t)pvParameters; /* Parameters to pass in. */

	/* Save GP register value in the context */
	asm volatile("sw $gp, %0" : "=m"(*(pxTopOfStack + CTX_GP / 4)));

	return pxTopOfStack;
}

static void prvTaskExitError(void)
{
	/* A function that implements a task must not exit or attempt to return to
	its caller as there is nothing to return to.  If a task wants to exit it
	should instead call vTaskDelete( NULL ).

	Artificially force an assert() to be triggered if configASSERT() is
	defined, then stop here so application writers can catch the error. */
	portDISABLE_INTERRUPTS();
	configASSERT(uxSavedTaskStackPointer == 0UL);
	for (;;)
		;
}

void vPortEndScheduler(void)
{
	/* Not implemented in ports where there is nothing to return to.
	Artificially force an assert. */
	configASSERT(uxInterruptNesting == 1000UL);
}

extern void vApplicationSetupTickTimerInterrupt(void);
BaseType_t xPortStartScheduler(void)
{
	extern void vPortStartFirstTask(void);
	extern void *pxCurrentTCB;

#if (configCHECK_FOR_STACK_OVERFLOW > 2)
	{
		/* Fill the ISR stack to make it easy to asses how much is being used. */
		memcpy((void *)xISRStack, ucExpectedStackBytes,
		       ulSzExpectedStackBytes);
	}
#endif /* configCHECK_FOR_STACK_OVERFLOW > 2 */

	uxInterruptNesting++;

	vPortInterruptInit();

	/* Clear the pending interrupts */
	portCLEAR_PENDING_INTERRUPTS();

	/* Install the software yield handler */
	vPortInterruptAppSetupSoftwareInterrupt();

	/* Kick off the highest priority task that has been created so far.
	Its stack location is loaded into uxSavedTaskStackPointer. */
	uxSavedTaskStackPointer = *(UBaseType_t *)pxCurrentTCB;

	/* Setup the timer to generate the tick.  Interrupts will have been
	disabled by the time we get here. */
	vApplicationSetupTickTimerInterrupt();

	uxInterruptNesting--;

	portENABLE_INTERRUPTS();
	vPortStartFirstTask();

	/* Should never get here as the tasks will now be executing!  Call the task
	exit error function to prevent compiler warnings about a static function
	not being called in the case that the application writer overrides this
	functionality by defining configTASK_RETURN_ADDRESS. */
	prvTaskExitError();

	return pdFALSE;
}
/*-----------------------------------------------------------*/

#if configCHECK_FOR_STACK_OVERFLOW > 0
/* Assert at a stack overflow. */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	configASSERT(xTask == NULL);
}
#endif

void vPortCacheFlush(void *pvAddr, uint32_t usByteLen)
{
	configASSERT((pvAddr > (void *)0x80000000) &&
		     (pvAddr < (void *)0xc0000000));
	cache_flush(pvAddr, usByteLen);
}

void vPortCacheFlushAll(void)
{
	cache_flush_all();
}

void vPortCacheInvalidate(void *pvAddr, uint32_t usByteLen)
{
	configASSERT((pvAddr > (void *)0x80000000) &&
		     (pvAddr < (void *)0xc0000000));
	cache_invalidate(pvAddr, usByteLen);
}

typedef struct idlehook_item {
	uint32_t param;
	void (*callback)(uint32_t);
	struct list_head entries;
} idlehook_item_s;

static LIST_HEAD(idlehook_list);

int vApplicationIdleHookRegister(void (*callback)(uint32_t), uint32_t param)
{
	idlehook_item_s *idlehook_param;

	if (callback == NULL)
		return -EINVAL;

	idlehook_param = (idlehook_item_s *)malloc(sizeof(idlehook_item_s));
	if (idlehook_param == NULL)
		return -ENOMEM;

	idlehook_param->param = param;
	idlehook_param->callback = callback;

	list_add_tail(&idlehook_param->entries,&idlehook_list);
}

void vApplicationIdleHook( void )
{
	idlehook_item_s *sp;

	list_for_each_entry (sp, &idlehook_list, entries) {
		if (sp->callback != NULL)
		{
			taskENTER_CRITICAL();
			sp->callback(sp->param);
			taskEXIT_CRITICAL();
		}
	};

#ifdef CONFIG_SOC_HC16XX

	if (get_processor_id() == 0)
		return;

	 __asm__("	.set	push		\n"
		 "	.set	arch=r4000	\n"
		 "	wait			\n"
		 "	.set	pop 	\n");
#endif
}

BaseType_t xPortIsInISR(void)
{
	return uxInterruptNesting;
}

struct tskTaskControlBlock;
typedef struct tskTaskControlBlock tskTCB;
typedef tskTCB TCB_t;

void __attribute__((weak)) pthread_local_storage_cleanup(TaskHandle_t task)
{
}

static TCB_t *gTCBHead[100] = { 0 };
void vPortDeleteThread( void *pvThreadToDelete )
{
	int i;

	pthread_local_storage_cleanup((TaskHandle_t)pvThreadToDelete);

	taskENTER_CRITICAL();
	for (i = 0; i < 100; i++) {
		if (pvThreadToDelete == (void *)gTCBHead[i]) {
			gTCBHead[i] = NULL;
			break;
		}
	}
	taskEXIT_CRITICAL();
}

void vPortSetupThread( void *pvThreadToSetup )
{
	int i;

	taskENTER_CRITICAL();
	for (i = 0; i < 100; i++) {
		if (gTCBHead[i] == NULL) {
			gTCBHead[i] = (TCB_t *)pvThreadToSetup;
			break;
		}
	}
	taskEXIT_CRITICAL();
}
