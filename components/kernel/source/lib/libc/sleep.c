#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

unsigned sleep(unsigned seconds)
{
	vTaskDelay(pdMS_TO_TICKS(seconds * 1000));
	return 0;
}
