#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>

#include <hcuapi/audsink.h>
#include <time.h>
#include "hcuapi/gpio.h"
#include "hcuapi/pinpad.h"

// TODO: proper include
extern rarch_main(int argc, char *argv[], void *data);
extern void verbosity_enable(void);


static void exit_console(int signo)
{
    printf("%s(), signo: %d, error: %s\n", __FUNCTION__, signo, strerror(errno));

    api_watchdog_stop();

    exit(0);
}

static void *watchdog_task(void *arg)
{
    while (1)
    {
        usleep(3 * 1000 * 1000);
        api_watchdog_feed();
    }
}

static void start_watchdog_task()
{
    pthread_t pid;

#ifdef WATCHDOG_KERNEL_FEED
    return ;
#endif

    pthread_create(&pid, NULL, watchdog_task, NULL);
}

void * main_sf2000(void *arg)
{
	for (int i=0; i<100; i++) {
		printf("**** %d\n", i);
		msleep(50);
	}

    printf("Init Retroarch!\n");

	/*
	char *argv[] = {
		"retroarch",
		"--menu",
		"-v"
	};
	int argc = sizeof(argv) / sizeof(argv[0]);

    rarch_main(argc, argv, NULL);
	*/

	// TODO: learn how to properly pass startup params to retroarch
	// or maybe better to pass via retroarch.cfg file instead
	// for now just force logging verbosity and dont pass anything
	verbosity_enable();
    rarch_main(0, NULL, NULL);


    //api_lvgl_init(OSD_MAX_WIDTH, OSD_MAX_HEIGHT);

    /*Handle LitlevGL tasks (tickless mode)*/
    while (1)
    {
        //lv_task_handler();
        usleep(1000);//frank, the sleep time will result in the OSD UI flush
    }

    printf("sf2000_demo exit!\n");

    return 0;
}

