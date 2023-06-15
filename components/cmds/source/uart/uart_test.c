#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <kernel/delay.h>
#include <kernel/lib/console.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <hcuapi/sci.h>

static void print_help(void) {
	printf("\tfor example : uart_test -n1 -m1 -b1 -p2				 \n");
	printf("\t		'n'	1 means open /dev/uart1				 \n");
	printf("\t		'm'	0 means mode SCIIOC_SET_HIGH_SPEED:3.375MHz	\n");
	printf("\t			1 means mode SCIIOC_SET_NORMAL_SPEED:115200	\n");
	printf("\t			2 means mode SCIIOC_SET_BAUD_RATE_115200 	\n");
	printf("\t			3 means mode SCIIOC_SET_BAUD_RATE_57600  	\n");
	printf("\t			4 means mode SCIIOC_SET_BAUD_RATE_19200  	\n");
	printf("\t			5 means mode SCIIOC_SET_BAUD_RATE_9600   	\n");
	printf("\t			6 means mode SCIIOC_SET_SETTING          	\n");
	//printf("\t			7 means mode SCIIOC_SET_BAUD_RATE_1125000	\n");
	//printf("\t			8 means mode SCIIOC_SET_BAUD_RATE_921600 	\n");
	//printf("\t			9 means mode SCIIOC_SET_BAUD_RATE_675000 	\n");
	printf("\t		'b'	1 means bits_mode(range:0~5):		 	\
		\n\t				1 stop bit and 8 data bits	 	\
		\n\t				1.5 stop bits and 5 data bits	 	\
		\n\t				2 stop bits and 6 data bits 	 	\
		\n\t				2 stop bits and 7 data bits 	 	\
		\n\t				2 stop bits and 8 data bits 	 	\n");
	printf("\t		'p'	1 means parity_mode(range:0~2):		 	\
		\n\t				PARITY_EVEN			 	\
		\n\t				PARITY_ODD 			 	\
		\n\t				PARITY_NONE			 	\n");
}

int uart_test(int argc, char *argv[])
{
	int fd;
	char dev_path[32];
	char ch;
	opterr = 0;
	optind = 0;
	uint32_t uart 	= 0;
	uint32_t mode 	= 0;
	uint32_t bits 	= 0;
	uint32_t parity = 0;
	struct sci_setting sci_setting_para = { 0 };

	if (argc < 2) {
		print_help();
		return -1;
	}

	while ((ch = getopt(argc, argv, "hn:m:b:p:")) != EOF) {
		switch (ch) {
			case 'h':
				print_help();
				return 0;
			case 'n':
				uart = atoi(optarg);
				break;
			case 'm':
				mode = atoi(optarg);
				break;
			case 'b':
				bits = atoi(optarg);
				break;
			case 'p':
				parity = atoi(optarg);
				break;
			default:
				printf("Invalid parameter %c\r\n", ch);
				print_help();
				return -1;
		}
	}

	sprintf(dev_path, "/dev/uart%d", (int)uart);
	fd = open(dev_path, O_RDWR);
	if (fd < 0) {
		printf("can't open %s\n",dev_path);
		return -EINVAL;
	}

	if (bits > bits_mode4 || parity > PARITY_NONE) {
		printf("bits mode / parity mode: Invalid parameter\n");
		return -1;
	}

	sci_setting_para.bits_mode = bits;
	sci_setting_para.parity_mode = parity;

	switch (mode) {
	case 0:
		ioctl(fd, SCIIOC_SET_HIGH_SPEED, 0);
		break;
	case 1:
		ioctl(fd, SCIIOC_SET_NORMAL_SPEED, 0);
		break;
	case 2:
		ioctl(fd, SCIIOC_SET_BAUD_RATE_115200, 0);
		break;
	case 3:
		ioctl(fd, SCIIOC_SET_BAUD_RATE_57600, 0);
		break;
	case 4:
		ioctl(fd, SCIIOC_SET_BAUD_RATE_19200, 0);
		break;
	case 5:
		ioctl(fd, SCIIOC_SET_BAUD_RATE_9600, 0);
		break;
	case 6:
		ioctl(fd, SCIIOC_SET_SETTING, &sci_setting_para);
		break;
//	case 7:
//		ioctl(fd, SCIIOC_SET_BAUD_RATE_1125000, 0);
//		break;
//	case 8:
//		ioctl(fd, SCIIOC_SET_BAUD_RATE_921600, 0);
//		break;
//	case 9:
//		ioctl(fd, SCIIOC_SET_BAUD_RATE_675000, 0);
//		break;
	}

	close(fd);

	return 0;
}

CONSOLE_CMD(uart_test, NULL, uart_test, CONSOLE_CMD_MODE_SELF, "uart test function app")
