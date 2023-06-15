#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include "_internal.h"

int clock_settime(clockid_t clk_id, const struct timespec *tp)
{
	if (tp)
		vPortPlatformTimeSet(tp->tv_sec, tp->tv_nsec/1000);

	return 0;
}
