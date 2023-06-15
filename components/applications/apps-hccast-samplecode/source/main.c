
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

#include "hccast_sample.h"
//extern int hccast_sample_wl_start(int argc, char **argv);
//extern int hccast_sample_um_start(int argc, char **argv);

static void app_main(void *pvParameters);
int main(void)
{
	xTaskCreate(app_main, (const char *)"app_main", configTASK_STACK_DEPTH,
		    NULL, portPRI_TASK_NORMAL, NULL);

	vTaskStartScheduler();

	abort();
	return 0;
}

static void app_main(void *pvParameters)
{
	assert(module_init("all") == 0);

	/* Set default time zone is GMT+8 */
	setenv("TZ", CONFIG_APP_TIMEZONE, 1);
	tzset();

	console_init();
	/* Console loop */

	console_register_cmd(NULL , "hccast_wl_sample" , hccast_sample_wl_start , CONSOLE_CMD_MODE_SELF , "hccast_wl_sample");
	console_register_cmd(NULL , "hccast_um_sample" , hccast_sample_um_start , CONSOLE_CMD_MODE_SELF , "hccast_um_sample");
    console_register_cmd(NULL , "hccast_um_stop_mirroring" , hccast_sample_um_stop_mirroring , CONSOLE_CMD_MODE_SELF , "hccast_um_stop_mirroring");
	console_register_cmd(NULL , "hccast_hostap_start" , hccast_sample_hostapd_start , CONSOLE_CMD_MODE_SELF , "hccast_hostap_start");
	console_register_cmd(NULL , "hccast_hostap_stop" , hccast_sample_hostapd_stop , CONSOLE_CMD_MODE_SELF , "hccast_hostap_stop");
	console_register_cmd(NULL , "hccast_wifi_scan" , hccast_sample_wifi_scan , CONSOLE_CMD_MODE_SELF , "hccast_wifi_scan");
	console_register_cmd(NULL , "hccast_wifi_connect" , hccast_sample_wifi_connect , CONSOLE_CMD_MODE_SELF , "hccast_wifi_connect");
	console_register_cmd(NULL , "hccast_wifi_disconnect" , hccast_sample_wifi_disconnect , CONSOLE_CMD_MODE_SELF , "hccast_wifi_disconnect");
	
	console_start();

	/* Program should not run to here. */
	for (;;);

	/* Delete current thread. */
	vTaskDelete(NULL);
}
