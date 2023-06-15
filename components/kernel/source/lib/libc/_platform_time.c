#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <mips/cpu.h>

#include "_internal.h"

#if 0
static QueueHandle_t tm_mutex;

void platform_settime(time_t sec, long usec)
{
	if(tm_mutex == NULL) {
		tm_mutex = xSemaphoreCreateMutex();
	}

	xSemaphoreTake(tm_mutex, portMAX_DELAY);

	vPortPlatformTimeSet(sec, usec);

	xSemaphoreGive(tm_mutex);
}


void platform_gettime(time_t *sec, long *usec)
{
	if(tm_mutex == NULL) {
		tm_mutex = xSemaphoreCreateMutex();
	}

	if(!xPortIsInISR())
		xSemaphoreTake(tm_mutex, portMAX_DELAY);
	
	vPortPlatformTimeGet(sec, usec);

	if(!xPortIsInISR())
		xSemaphoreGive(tm_mutex);
}
#endif
