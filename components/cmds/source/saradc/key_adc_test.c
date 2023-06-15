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

#define buf_size 64 

static void print_help(void) {
	printf("***********************************\n");
	printf("key adc test cmds help\n");
	printf("\tfor example : key_adc_test -i1\n");
	printf("\t'i'	1 means event1\n");
	printf("***********************************\n");
}

int key_adc_test(int argc, char * argv[])
{
	int fd;
	struct input_event t;
	struct pollfd pfd;
	long tmp;
	char input_buf[buf_size];
	char *s = "/dev/input/event";
	int input_num = -1;

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
				input_num = tmp;
				break;
			default:
				printf("Invalid parameter %c\r\n", ch);
				print_help();
				return -1;
		}
	}

	if(input_num == -1){
		print_help();
		return -1;
	}

	sprintf(input_buf,"/dev/input/event%d",input_num);

	fd = open(input_buf, O_RDONLY);
	if(fd < 0){
		printf("can't open %s\n",input_buf);
		return -1;
	}

	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDNORM;

	while (1) {
		if (poll(&pfd, 1, -1) <= 0) {
			continue;
		}

		if (read(fd, &t, sizeof(t)) != sizeof(t)) {
			continue;
		}

		printf("type:%d, code:%d, value:%ld\n", t.type, t.code, t.value);
		if (t.type == EV_KEY) {
			printf("key %d %s\n", t.code,
			       (t.value) ? "Pressed" : "Released");
/* 			if (t.code == KEY_POWER && !t.value) {
				while (read(fd, &t, sizeof(t)) == sizeof(t))
					;
				break;
			} */
		}
	}

	close(fd);

	return 0;
}

CONSOLE_CMD(key_adc_test,"adc_test",key_adc_test,CONSOLE_CMD_MODE_SELF,"test key-adc function app of 1512")