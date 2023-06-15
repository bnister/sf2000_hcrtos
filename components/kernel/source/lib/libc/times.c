#include <sys/times.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

clock_t times(struct tms *buf)
{
	clock_t tick;

	if (buf == NULL)
		return -1;

	if (!xPortIsInISR())
		tick= (clock_t)xTaskGetTickCount();
	else
		tick= (clock_t)xTaskGetTickCountFromISR();

	buf->tms_stime = tick;
	buf->tms_utime = 0;
	buf->tms_cutime = 0;
	buf->tms_cstime = 0;

	return tick;
}
