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

static int cmd_ps(int argc, char *argv[])
{
	char *RunTimeInfo;
	int ch;

	/* Clear opt global variables
	 * opterr
	 * optind
	 */
	opterr = 0;
	optind = 0;

	while ((ch = getopt(argc, argv, "h")) != EOF) {
		switch (ch) {
		case 'h':
			printf("Task Status :\r\n");
			printf("\t'B' - Blocked\r\n");
			printf("\t'R' - Ready\r\n");
			printf("\t'D' - Deleted (waiting clean up)\r\n");
			printf("\t'S' - Suspended, or Blocked without a timeout\r\n");
			printf("\t'X' - Executing\r\n");
			printf("Task Pority :\r\n");
			printf("\t0 means lowest priority and 31 means highest priority\r\n");
			printf("Task Stack :\r\n");
			printf("\tIt means the size of the remaining stack space for the corresponding task\r\n");
			printf("\tNotice : In words , not bytes !!! \r\n");
			return 0;
		default:
			printf("Invalid parameter %c\r\n", ch);
			return -1;
		}
	}

	RunTimeInfo = malloc(BUF_SIZE);
	if (RunTimeInfo == NULL) {
		printf(" @@malloc buffer return error !!\r\n");
		return -1;
	}

	memset(RunTimeInfo, 0, BUF_SIZE);
	vTaskList(RunTimeInfo);
	printf("Task\t\tState\tPority\tStack\tID\tpxHandle\r\n");
	printf("*********************************************************\r\n");
	printf("%s\r\n", RunTimeInfo);

	free(RunTimeInfo);

	return 0;
}

CONSOLE_CMD(ps, "os", cmd_ps, CONSOLE_CMD_MODE_SELF,
	    "Display all FreeRTOS tasks status")
