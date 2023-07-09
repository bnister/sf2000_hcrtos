#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <kernel/delay.h>

// TODO: include the proper headers that declares these functions
extern int rarch_main(int argc, char *argv[], void *data);
extern void verbosity_enable(void);


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

