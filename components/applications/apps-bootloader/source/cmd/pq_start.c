#define LOG_TAG "pq_start"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/elog.h>
#include <hcuapi/pq.h>
#include <kernel/lib/console.h>

int open_pq_start(int argc, char *argv[])
{
	int pq_fd = -1;

	pq_fd = open("/dev/pq", O_WRONLY);
	if (pq_fd < 0) {
		log_e("pq_start open error\n");
		return -1;
	}

	ioctl(pq_fd, PQ_START);
	//close(pq_fd);
	return 0;
}

CONSOLE_CMD(pq_start, NULL, open_pq_start, CONSOLE_CMD_MODE_SELF,
	    "boot enter standby")
