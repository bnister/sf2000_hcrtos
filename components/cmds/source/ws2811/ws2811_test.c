#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <ws2811.h>
#include <hcuapi/gpio.h>
#include <kernel/lib/console.h>

struct ws2811_parm {
	pinpad_e pin;
	int tim;
};

static struct ws2811_parm gparm = {0};
static int ws2811_test_start(int argc, char **argv);


static void print_usage(const char *prog)
{
	printf("Usage: %s [-p]\n", prog);
	puts("  -p --pin      ws2811 gpio pin\n");
}

static void ws2811_delay_ns(int ns)
{
	int i=0, j=0;

	for(i=0; i<ns; i++){
		j++;
	}
}

static void ws2811_turn_off(pinpad_e pin)
{
	char j = 0;
	char bindex = 0;

	gpio_set_output(pin, 0);
	for(bindex=0; bindex<12; bindex++){///000000,000000,000000
		for(j=0; j<24; j++)
		{
			gpio_set_output(pin, 1);
			//ws2811_delay_ns(1);
			gpio_set_output(pin, 0);
			usleep(1);
			ws2811_delay_ns(450);
		}
	}
}

static void ws2811_turn_r(pinpad_e pin)
{
	char j = 0;
	char bindex = 0;

	gpio_set_output(pin, 0);
	for(bindex=0; bindex<12; bindex++){///FF0000,FF0000,FF0000
		for(j=0; j<8; j++)
		{
			gpio_set_output(pin, 1);
			usleep(1);
			ws2811_delay_ns(500);
			gpio_set_output(pin, 0);
		}
	
		for(j=0; j<16; j++)
		{
			gpio_set_output(pin, 1);
			//ws2811_delay_ns(50);
			gpio_set_output(pin, 0);
			usleep(1);
			ws2811_delay_ns(450);
		}
	}
}

static void ws2811_turn_g(pinpad_e pin)
{
	char j = 0;
	char bindex = 0;

	gpio_set_output(pin, 0);
	for(bindex=0; bindex<12; bindex++){///00FF00,00FF00,00FF00
		for(j=0; j<8; j++)
		{
			gpio_set_output(pin, 1);
			//ws2811_delay_ns(50);
			gpio_set_output(pin, 0);
			usleep(1);
			ws2811_delay_ns(450);
		}
		
		for(j=0; j<8; j++)
		{
			gpio_set_output(pin, 1);
			usleep(1);
			ws2811_delay_ns(500);
			gpio_set_output(pin, 0);
		}
	
		for(j=0; j<8; j++)
		{
			gpio_set_output(pin, 1);
			//ws2811_delay_ns(50);
			gpio_set_output(pin, 0);
			usleep(1);
			ws2811_delay_ns(450);
		}
	}
}

static void ws2811_turn_b(pinpad_e pin)
{
	char j = 0;
	char bindex = 0;
	
	gpio_set_output(pin, 0);
	for(bindex=0; bindex<12; bindex++){///0000FF,0000FF,0000FF
		for(j=0; j<16; j++)
		{
			gpio_set_output(pin, 1);
			//ws2811_delay_ns(50);
			gpio_set_output(pin, 0);
			usleep(1);
			ws2811_delay_ns(450);
		}
		
		for(j=0; j<8; j++)
		{
			gpio_set_output(pin, 1);
			usleep(1);
			ws2811_delay_ns(500);
			gpio_set_output(pin, 0);
		}
	}
}

static void ws2811_turn_rgb(pinpad_e pin)
{
	char j = 0;
	char bindex = 0;

	gpio_set_output(pin, 0);
	for(bindex=0; bindex<4; bindex++){///FF0000,00FF00.0000FF
		for(j=0; j<8; j++)
		{
			gpio_set_output(pin, 1);
			usleep(1);
			ws2811_delay_ns(500);
			gpio_set_output(pin, 0);
		}
	
		for(j=0; j<24; j++)
		{
			gpio_set_output(pin, 1);
			//ws2811_delay_ns(50);
			gpio_set_output(pin, 0);
			usleep(1);
			ws2811_delay_ns(450);
		}
	
		for(j=0; j<8; j++)
		{
			gpio_set_output(pin, 1);
			usleep(1);
			ws2811_delay_ns(500);
			gpio_set_output(pin, 0);
		}
	
		for(j=0; j<24; j++)
		{
			gpio_set_output(pin, 1);
			//ws2811_delay_ns(50);
			gpio_set_output(pin, 0);
			usleep(1);
			ws2811_delay_ns(450);
		}			
	
		for(j=0; j<8; j++)
		{
			gpio_set_output(pin, 1);
			usleep(1);
			ws2811_delay_ns(500);
			gpio_set_output(pin, 0);
		}
	}
}

static void ws2811_main(void *arg)
{
	int delay = gparm.tim;
	pinpad_e pin = gparm.pin;

	gpio_configure(pin, GPIO_DIR_OUTPUT);
	gpio_set_output(pin, 0);
	usleep(100);
	while(1){
		arch_local_irq_disable();
		ws2811_turn_rgb(pin);
		arch_local_irq_enable();
		usleep(delay);
		
		arch_local_irq_disable();
		ws2811_turn_off(pin);
		arch_local_irq_enable();
		usleep(delay);
		
		arch_local_irq_disable();
		ws2811_turn_r(pin);
		arch_local_irq_enable();
		usleep(delay);
		
		arch_local_irq_disable();
		ws2811_turn_off(pin);
		arch_local_irq_enable();
		usleep(delay);
		
		arch_local_irq_disable();
		ws2811_turn_g(pin);
		arch_local_irq_enable();
		usleep(delay);
		
		arch_local_irq_disable();
		ws2811_turn_off(pin);
		arch_local_irq_enable();
		usleep(delay);
		
		arch_local_irq_disable();
		ws2811_turn_b(pin);
		arch_local_irq_enable();
		usleep(delay);
		
		arch_local_irq_disable();
		ws2811_turn_off(pin);
		arch_local_irq_enable();
		usleep(delay);
	}

	vTaskDelete(NULL);
}


void *ws2811_start(pinpad_e pin, int tim)
{
	int ret = 0;
	TaskHandle_t lvgl_thread = NULL;
	
	printf("ws2811 task start&pin=%d, tim=%d!\n", pin, tim);
	gparm.pin = pin;
	gparm.tim = tim;
	ret = xTaskCreate(ws2811_main , "ws2811_main" ,
					  0x1000 , NULL, portPRI_TASK_NORMAL , &lvgl_thread);

	if(ret != pdTRUE)
	{
		return NULL;
	}

	/* SUCCESS */
	return lvgl_thread;
}

int ws2811_stop(void *handle)
{
	//ws2811_turn_off(gparm.pin);
	gpio_set_output(gparm.pin, 0);
	if(handle != NULL){
		printf("ws2811 task delete!\n");
		vTaskDelete(handle);
	}
}

static void *handle = NULL;

static int ws2811_test_start(int argc, char **argv)
{
	int time = 0;
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

	if (handle != NULL) {
		printf("Warning, stop previous ws2811 first\n");
		ws2811_stop(handle);
	}

	handle = ws2811_start(pin, time);
	if (!handle) {
		printf("ws2811 start fail\n");
		return -1;
	}

	return 0;
}

static int ws2811_test_stop(int argc, char **argv)
{
	if (handle) {
		ws2811_stop(handle);
		handle = NULL;
	}

	return 0;
}

CONSOLE_CMD(ws2811, NULL, NULL, CONSOLE_CMD_MODE_SELF, "ws2811 cmds entry")
CONSOLE_CMD(start, NULL, ws2811_test_start, CONSOLE_CMD_MODE_SELF, "ws2811 start")
CONSOLE_CMD(stop, NULL, ws2811_test_stop, CONSOLE_CMD_MODE_SELF, "ws2811 stop")
