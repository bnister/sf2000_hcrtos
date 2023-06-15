#include <stdint.h>
#include <unistd.h>
#include <mips/cpu.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

int usleep(useconds_t us)
{
	uint32_t us_ticks = 0, old_tick = 0, new_tick = 0;

	if (us >= (1000 * portTICK_PERIOD_MS)) {
		vTaskDelay(pdMS_TO_TICKS(us / 1000 + (us % 1000 != 0)));
		return 0;
	} else if (us >= 1000) {
		/* extent to 1 tick when 1ms <= us < portTICK_PERIOD_MS */
		vTaskDelay(1);
		return 0;
	}

	if (us > 0) {
		old_tick = mips_getcount();
		us_ticks = us * (CONFIG_CPU_CLOCK_HZ / 2000000);

		while (1) {
			new_tick = mips_getcount();
			if ((new_tick - old_tick) > us_ticks) {
				break;
			}
		}
	}

	return 0;
}

