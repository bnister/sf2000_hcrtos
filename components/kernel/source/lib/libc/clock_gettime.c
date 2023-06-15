#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "_internal.h"

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
	time_t sec;
	long usec;

	if (clk_id == CLOCK_MONOTONIC) {
		tp->tv_sec = (xTaskGetTickCount() / portTICK_PERIOD_MS) / 1000;
		tp->tv_nsec = (xTaskGetTickCount() / portTICK_PERIOD_MS) % 1000;
		tp->tv_nsec *= 1000000;

		return 0;
	}

	if (tp) {
		vPortPlatformTimeGet(&sec, &usec);
		tp->tv_sec = sec;
		tp->tv_nsec = usec*1000;

		return 0;
	}

	return 0;
}
