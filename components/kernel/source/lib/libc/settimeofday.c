#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include "_internal.h"

/* Not support set timezone through this API */
int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
	(void)tz;
	if (tv == NULL)
		return -1;

	if (tv)
		vPortPlatformTimeSet(tv->tv_sec, tv->tv_usec);

	return 0;
}
