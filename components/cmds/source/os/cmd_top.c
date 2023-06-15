#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <kernel/lib/console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define BUF_SIZE (16*1024)

static int cmd_top(int argc, char *argv[])
{
	char *RunTimeInfo;
	TickType_t tTickNow;
	int ch;

	/* Clear opt global variables
	 * opterr
	 * optind
	 */
	opterr = 0;
	optind = 0;

	RunTimeInfo = malloc(BUF_SIZE);
	if (RunTimeInfo == NULL) {
		printf(" @@malloc buffer return error !!\r\n");
		return -1;
	}
	memset(RunTimeInfo, 0, BUF_SIZE);

	while ((ch = getopt(argc, argv, "hp")) != EOF) {
		switch (ch) {
		case 'h':
			printf("DESCRIPTION :\r\n");
			printf("\ttop -h\t: help information\r\n");
			printf("\ttop -p\t: Shows the performance of all tasks from the last time you entered this command (top -p) until now\r\n");
			printf("\ttop \t: Shows the performance of all tasks during the period from RTOS startup until now\r\n");
			printf("\r\nNOTE: \r\n");
			printf("\t%% Time :\r\n");
			printf("\t       Shows the amount of time each task has spent in the Running state (how much CPU time each task has consumed).\r\n");
			printf("\tAbs Time :\r\n");
			printf("\t      The corresponding unit is about 38us (1024/27 us) \r\n");
			return 0;
		case 'p':
			tTickNow = xTaskGetTickCount();
			taskENTER_CRITICAL();
			vTaskGetRunTimePeriodStats(RunTimeInfo);
			taskEXIT_CRITICAL();
			printf(" SYSTEM TICK : %u h -%2u min - %2u sec - %3u ms\r\n", \
				(unsigned int)  (tTickNow / (1000 *3600) ),
				(unsigned int)  (tTickNow % (1000 *3600) / (60 * 1000)),
				(unsigned int)  (tTickNow % (1000 *3600) %  (60 * 1000) / 1000),
				(unsigned int)  (tTickNow % 1000) );
			printf("Task\t\t ID\t\t  Cost Time\t\t%% Time\r\n");
			printf("****************************************************************\r\n");
			printf("%s\r\n", RunTimeInfo);
			free(RunTimeInfo);
			return 0;

		default:
			printf("Invalid parameter %c\r\n", ch);
			return -1;
		}
	}

	taskENTER_CRITICAL();
	vTaskGetRunTimeStats(RunTimeInfo);
	taskEXIT_CRITICAL();
	printf("Task\t\tRunTimeCounter\tPercentage\r\n");
	printf("******************************************\r\n");
	printf("%s\r\n", RunTimeInfo);

	free(RunTimeInfo);
	return 0;
}

CONSOLE_CMD(top, "os", cmd_top, CONSOLE_CMD_MODE_SELF,
	    "FreeRTOS tasks performance statistics")
