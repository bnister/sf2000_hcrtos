#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/input.h>
#include <kernel/lib/console.h>

#define BUF_SIZE 1024

static void print_help(void) {
	printf("***********************************\n");
	printf("input test cmds help\n");
	printf("\tfor example : input_test -i 1\n");
	printf("\t'i'	1 means event1\n");
	printf("***********************************\n");
}

static int input_test(int argc, char *argv[])
{
	int fd;
	struct input_event t;
	struct pollfd pfd;
	char input_buf[BUF_SIZE];
	char *s = "/dev/input/event";

	long tmp;
	int x = 0, y = 0;
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
	if(event_num == -1)
	{
		print_help();
		return -1;
	}

	sprintf(input_buf,"/dev/input/event%d",event_num);

	fd = open(input_buf, O_RDONLY);
	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDNORM;

	if(fd < 0){
		printf("can't open %s\n",input_buf);
		return -1;
	}

	while (1) {
		if (poll(&pfd, 1, -1) <= 0)
			continue;

		if (read(fd, &t, sizeof(t)) != sizeof(t))
			continue;

		printf("type:%d, code:%d, value:%ld\n", t.type, t.code, t.value);

		if (t.type == EV_KEY) {
			printf("key %d %s\n", t.code,
					(t.value) ? "Pressed" : "Released");
			if (t.code == KEY_POWER && !t.value) {
				while (read(fd, &t, sizeof(t)) == sizeof(t))
					;
				break;
			}
		}
		else{
			if (t.type == EV_ABS)
			{
				if (t.type == EV_ABS&& t.code == ABS_X) {
					x = t.value;
				}
				if (t.type == EV_ABS && t.code == ABS_Y) {
					y = t.value;
				}
			}
			if (t.type == EV_SYN) {
				printf("(%4d %4d)\n",x,y);
			}
		}
	}

	close(fd);

	return 0;
}

CONSOLE_CMD(input, NULL, input_test, CONSOLE_CMD_MODE_SELF, "input test, press power to exit test")
