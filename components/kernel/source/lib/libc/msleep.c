#include <stdint.h>
#include <unistd.h>
#include <mips/cpu.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void msleep(unsigned int msecs)
{
	if (msecs >= portTICK_PERIOD_MS) {
		vTaskDelay(pdMS_TO_TICKS(msecs));
		return;
	} else if (msecs >= 1) {
		/* extent to 1 tick when 1ms <= msecs < portTICK_PERIOD_MS */
		vTaskDelay(1);
		return;
	}

	vTaskDelay(0);
}

