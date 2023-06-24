
#include <generated/br2_autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <kernel/elog.h>
#include <sys/poll.h>
#include <kernel/module.h>
#include <kernel/io.h>
#include <kernel/lib/console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <pthread.h>

static void app_main(void *pvParameters);
extern void* main_sf2000(void* arg);

int main(void)
{
    xTaskCreate(app_main, (const char *)"app_main", configTASK_STACK_DEPTH,
                NULL, portPRI_TASK_NORMAL, NULL);

    vTaskStartScheduler();

    abort();
    return 0;
}

int sf2000app_main(int argc, char **argv)
{
    pthread_t pid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x10000);
    pthread_create(&pid, &attr, main_sf2000, NULL);

    return 0;
}

static int sf2000_start(void)
{
	sf2000app_main(0, NULL);
	return 0;
}

__initcall(sf2000_start)


static void app_main(void *pvParameters)
{
    assert(module_init("all") == 0);

    /* Set default time zone is GMT+8 */
    setenv("TZ", CONFIG_APP_TIMEZONE, 1);
    tzset();

    console_init();
    /* Console loop */

    console_start();

    /* Program should not run to here. */
    for (;;);

    /* Delete current thread. */
    vTaskDelete(NULL);
}
