#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/delay.h>
#include <kernel/lib/console.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>

#include <nuttx/wqueue.h>
#include <hcuapi/iocbase.h>
#include <hcuapi/watchdog.h>

#define WATCHDOG_TIMEOUT 400000

static const char *device = "/dev/watchdog";

static void notify_watchdog_call(void *arg, unsigned long param)
{
    printf("%s:%d:receive watchdog timer notify\n", __func__, __LINE__);

    return ;
}

struct work_notifier_s notify_watchdog;

int watchdog_test(int argc, char * argv[])
{
    int ret = 0;
    int fd;
    uint32_t watchdog_value = WATCHDOG_TIMEOUT;

    notify_watchdog.evtype = WDIOC_NOTIFY_TIMEOUT;
    notify_watchdog.qid = HPWORK;
    notify_watchdog.remote = false;
    notify_watchdog.oneshot = false;
    notify_watchdog.qualifier = NULL;
    notify_watchdog.arg = NULL;
    notify_watchdog.worker2 = notify_watchdog_call;
    work_notifier_setup(&notify_watchdog);

    fd = open(device, O_RDWR);
    if (fd < 0) {
	    printf("can't open %s\n",device);
	    return -1;
    }

    ret = ioctl(fd, WDIOC_SETTIMEOUT, (uint32_t)watchdog_value);
    if (!ret) {
	    printf("%d set watchdog timer timeout = %ld\n",__LINE__, watchdog_value);
    }

    ret = ioctl(fd, WDIOC_GETTIMEOUT, (uint32_t)&watchdog_value);
    if (!ret) {
	    printf("%d get watchdog timer timeout = %ld\n",__LINE__, watchdog_value);
    } 

    ret = ioctl(fd, WDIOC_SETMODE, WDT_MODE_TIMER );
    if (!ret) {
	    printf("%d set watchdog timer mode to timer\n",__LINE__);
    }

    ret = ioctl(fd, WDIOC_START, 0);
    if (!ret) {
	    printf("%d start watchdog timer\n",__LINE__);
    }

    usleep(WATCHDOG_TIMEOUT + 1000000);

    ret = ioctl(fd, WDIOC_GETTIMELEFT, (uint32_t)&watchdog_value);
    if (!ret) {
	    printf("%d get watchdog timer residual value = %ld\n",__LINE__, watchdog_value);
    } 

    ret = ioctl(fd, WDIOC_GETTIMELEFT, (uint32_t)&watchdog_value);
    if (!ret) {
	    printf("%d get watchdog timer residual value = %ld\n",__LINE__, watchdog_value);
    } 

    ret = ioctl(fd, WDIOC_STOP, 0);
    if (!ret) {
            printf("%d stop watchdog timer\n",__LINE__);
    } 

    ret = ioctl(fd, WDIOC_KEEPALIVE, 0);
    if (!ret) {
	    printf("%d reset watchdog timer timerout value\n",__LINE__);
    }

    usleep(WATCHDOG_TIMEOUT / 2);

    ret = ioctl(fd, WDIOC_GETTIMELEFT, (uint32_t)&watchdog_value);
    if (!ret) {
	    printf("%d get watchdog timer residual value = %ld\n",__LINE__, watchdog_value);
    }

    close(fd);

    return ret;
}

CONSOLE_CMD(watchdog_test,NULL,watchdog_test,CONSOLE_CMD_MODE_SELF,"test watchdog function app")
