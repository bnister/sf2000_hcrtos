#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <hcuapi/gpio.h>
#include <hcuapi/i2c-master.h>
#include <kernel/lib/console.h>


#define PIN_L22      22
#define PIN_L23      23
#define PIN_L24      24
#define PIN_L25      25
#define PIN_B00      32


static int sorting_test_start(int argc, char **argv);


static void print_usage(const char *prog)
{
	printf("Usage: %s [-p]\n", prog);
	puts("  -p --pin      sorting gpio pin\n");
}

static int sorting_i2c2_xf(uint8_t addr, char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret;
	int fd;

	fd = open("/dev/i2c1", O_RDWR);
	if(fd < 0)
	{
		printf("open i2c1 error fd=%d\n", fd);
		return -1;
	}

#if 1 
	if (writelen > 0) {
		struct i2c_transfer_s xfer_read;
	        struct i2c_msg_s i2c_msg_read[2] = {0};
		
	        i2c_msg_read[0].addr = addr;
	        i2c_msg_read[0].flags = 0x0;
	        i2c_msg_read[0].buffer = writebuf;
	        i2c_msg_read[0].length = writelen;
	
	        i2c_msg_read[1].addr = addr;
	        i2c_msg_read[1].flags = 0x1;
	        i2c_msg_read[1].buffer = readbuf;       
	      	i2c_msg_read[1].length = readlen;
	
	        xfer_read.msgv = i2c_msg_read;
	        xfer_read.msgc = 2;
	
	        ret = ioctl(fd,I2CIOC_TRANSFER,&xfer_read);
	
		if (ret < 0)
			printf("sorting_i2c2_xf writelen > 0 error\n");
	}
	else {
		struct i2c_transfer_s xfer_write;
		 struct i2c_msg_s i2c_msg_write ;

		  i2c_msg_write.addr = addr;
		  i2c_msg_write.buffer = readbuf;
		  i2c_msg_write.length = readlen;
		
		  xfer_write.msgv = &i2c_msg_write;
		  xfer_write.msgc = 1;
		
		  ret = ioctl(fd,I2CIOC_TRANSFER,&xfer_write);	
		if (ret < 0)
			printf("sorting_i2c2_xf writelen == 0 error\n");
	}
#endif
	close(fd);

	return ret;
}



static void sorting_main(void *arg)
{
	char wbuf[100] ={0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
	char rbuf[100] ={0};
	int wlen = 10, rlen = 0;
	
	gpio_configure(PIN_L22, GPIO_DIR_OUTPUT);
	gpio_configure(PIN_L23, GPIO_DIR_OUTPUT);
	gpio_configure(PIN_L24, GPIO_DIR_OUTPUT);
	gpio_configure(PIN_L25, GPIO_DIR_OUTPUT);
	gpio_configure(PIN_B00, GPIO_DIR_OUTPUT);

	while(1){
		gpio_set_output(PIN_B00, 0);
		//sorting_i2c2_xf(0x50, wbuf, wlen, rbuf, rlen);
		usleep(10000);
		gpio_set_output(PIN_B00, 1);
		usleep(10000);
	}

	vTaskDelete(NULL);
}


void *sorting_start(/*pinpad_e pin, int tim*/void)
{
	int ret = 0;
	TaskHandle_t lvgl_thread = NULL;
	
	printf("sorting task start\n");

	ret = xTaskCreate(sorting_main , "sorting_main" ,
					  0x1000 , NULL, portPRI_TASK_NORMAL , &lvgl_thread);

	if(ret != pdTRUE)
	{
		return NULL;
	}

	/* SUCCESS */
	return lvgl_thread;
}

int sorting_stop(void *handle)
{
	if(handle != NULL){
		printf("sorting task delete!\n");
		vTaskDelete(handle);
	}
}

static void *handle = NULL;

static int sorting_test_start(int argc, char **argv)
{
	int time = 0;
	#if 0
	pinpad_e pin = (pinpad_e)-1;

	opterr = 0;
	optind = 0;

	while (1) {
		static const struct option lopts[] = {
			{ "pin",    1, 0, 'p' },
			{ "time",	1, 0, 't' },
		};
		int c;

		c = getopt_long(argc, argv, "p:t:", lopts, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'p':
			pin = atoi(optarg);
			break;
		case 't':
			time = atoi(optarg);
			break;	
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if (pin == (pinpad_e)-1) {
		printf("no pin specified\n");
		print_usage(argv[0]);
		return -1;
	}
	#endif
	
	if (handle != NULL) {
		printf("Warning, stop previous sorting first\n");
		sorting_stop(handle);
	}

	handle = sorting_start(/*pin, time*/);
	if (!handle) {
		printf("sorting start fail\n");
		return -1;
	}

	return 0;
}

static int sorting_test_stop(int argc, char **argv)
{
	if (handle) {
		sorting_stop(handle);
		handle = NULL;
	}

	return 0;
}

CONSOLE_CMD(sorting, NULL, NULL, CONSOLE_CMD_MODE_SELF, "sorting cmds entry")
CONSOLE_CMD(start, NULL, sorting_test_start, CONSOLE_CMD_MODE_SELF, "sorting start")
CONSOLE_CMD(stop, NULL, sorting_test_stop, CONSOLE_CMD_MODE_SELF, "sorting stop")
