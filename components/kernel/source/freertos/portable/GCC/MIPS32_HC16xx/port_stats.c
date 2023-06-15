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
#include "task.h"
#include "timers.h"
#include "event_groups.h"

UBaseType_t portGetOtherRunTimeStats(TaskStatus_t *pxTaskStatusArray, UBaseType_t uxMaxNumber)
{
	struct __cpustats *stats_start = (struct __cpustats *)&_cpustats_start;
	struct __cpustats *stats_end = (struct __cpustats *)&_cpustats_end;
	struct __cpustats *p;
	UBaseType_t n = 0;

	for (p = stats_start; p < stats_end; p++) {
		if (n >= uxMaxNumber)
			return n;

		pxTaskStatusArray[n].pcTaskName = p->name;
		pxTaskStatusArray[n].uxCurrentPriority = 0;
		pxTaskStatusArray[n].pxStackBase = NULL;
		pxTaskStatusArray[n].xTaskNumber = (uint32_t)p;
		pxTaskStatusArray[n].uxBasePriority = 0;
		pxTaskStatusArray[n].ulRunTimeCounter = p->runtime;
		pxTaskStatusArray[n].eCurrentState = eBlocked;
		pxTaskStatusArray[n].usStackHighWaterMark = 0;
		n++;
	}

	return n;
}

UBaseType_t portGetOtherRunTimeStatsNum( void )
{
	struct __cpustats *stats_start = (struct __cpustats *)&_cpustats_start;
	struct __cpustats *stats_end = (struct __cpustats *)&_cpustats_end;
	struct __cpustats *p;
	static UBaseType_t other_stats_num = -1;

	if ((int)other_stats_num >= 0)
		return other_stats_num;

	other_stats_num = 0;
	for (p = stats_start; p < stats_end; p++) {
		other_stats_num++;
	}

	return other_stats_num;
}

UBaseType_t __attribute__((weak)) uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray,
                                      const UBaseType_t uxArraySize,
                                      configRUN_TIME_COUNTER_TYPE * const pulTotalRunTime )
{
	return 0;
}

void __attribute__((weak)) vTaskGetRunTimePeriodStats( char *pcWriteBuffer )
{
	*pcWriteBuffer = 0x00;
	sprintf( pcWriteBuffer, "Not configured, please enable CONFIG_TASK_PERFORMANCE using `make menuconfig`\r\n");
	return;
}

void __attribute__((weak)) vTaskGetRunTimeStats( char * pcWriteBuffer )
{
	*pcWriteBuffer = 0x00;
	sprintf( pcWriteBuffer, "Not configured, please enable CONFIG_TASK_PERFORMANCE using `make menuconfig`\r\n");
	return;
}

void __attribute__((weak)) vTaskList( char * pcWriteBuffer )
{
	*pcWriteBuffer = 0x00;
	sprintf( pcWriteBuffer, "Not configured, please enable CONFIG_TASK_PERFORMANCE using `make menuconfig`\r\n");
}

void __attribute__((weak)) vTaskGetInfo(TaskHandle_t xTask, TaskStatus_t *pxTaskStatus, BaseType_t xGetFreeStackSpace, eTaskState eState)
{
        pxTaskStatus->xTaskNumber = 0;
}

#if ( configUSE_TRACE_FACILITY == 1 )
#else
#undef xEventGroupSetBitsFromISR
BaseType_t __attribute__((weak))
xEventGroupSetBitsFromISR(EventGroupHandle_t xEventGroup,
			  const EventBits_t uxBitsToSet,
			  BaseType_t *pxHigherPriorityTaskWoken)
{
	return xTimerPendFunctionCallFromISR(vEventGroupSetBitsCallback,
					       (void *)xEventGroup,
					       (uint32_t)uxBitsToSet,
					       pxHigherPriorityTaskWoken);
}
#endif
