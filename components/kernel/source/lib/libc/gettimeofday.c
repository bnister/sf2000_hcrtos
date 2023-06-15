#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include "_internal.h"

/* Not support get timezone through this API */
int gettimeofday(struct timeval *tv, void *tz)
{
	(void)tz;
	time_t sec;
	long usec;

	if (tv == NULL)
		return -1;

	if (tv) {
		vPortPlatformTimeGet(&sec, &usec);
		tv->tv_sec = sec;
		tv->tv_usec = usec;

		return 0;
	}

	return 0;
}
