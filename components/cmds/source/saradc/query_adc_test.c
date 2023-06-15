#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/input.h>
#include <kernel/lib/console.h>
#include <string.h>

#define BUF_SIZE 1024

static void print_help(void) {
	printf("***********************************\n");
	printf("input test cmds help\n");
	printf("\tfor example : query_test -i1\n");
	printf("\t'i'	1 means queryadc1\n");
	printf("***********************************\n");
}

#define read_buf_len    16

static int queryadc_test(int argc, char *argv[])
{
	int fd, ret;
	char device_node[BUF_SIZE];
	char *s = "/dev/queryadc";
	unsigned char read_buf[read_buf_len];
	uint8_t sar_dout = 0;

	long tmp;
	int event_num = -1;
	char ch;
	opterr = 0;
	optind = 0;

	while((ch = getopt(argc, argv, "hi:")) != EOF){
		switch (ch) {
			case 'h':
				print_help();
				return 0;
			case 'i':
				tmp = strtoll(optarg, NULL,10);
				event_num = tmp;
				break;
			default:
				printf("Invalid parameter %c\r\n", ch);
				print_help();
				return -1;
		}
	}

	if (event_num == -1) {
		print_help();
		return -1;
	}

	sprintf(device_node,"/dev/queryadc%d",event_num);

	fd = open(device_node, O_RDONLY);

	if(fd < 0){
		printf("can't open %s\n",device_node);
		return -1;
	}

	ret = read(fd, read_buf, sizeof(read_buf));
	printf("adc value is %d\n", read_buf[0]);
	printf("voltage value is %d mV\n", read_buf[0] * 2000 / 255);

	close(fd);

	return 0;
}

CONSOLE_CMD(queryadc, "adc_test", queryadc_test, CONSOLE_CMD_MODE_SELF, "queryadc test")
