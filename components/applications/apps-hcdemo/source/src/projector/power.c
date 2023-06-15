#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/elog.h>
#include <kernel/delay.h>
#include <hcuapi/standby.h>
#include <hcuapi/lvds.h>
#include "screen.h"
#include "factory_setting.h"
#include <bluetooth.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void standby_pre_process(void);
// pre process before enter standby mode
static void standby_pre_process(void)
{
    int fd;
    //step 1:  save system data
    projector_sys_param_save();
    
    //step 2:  close lcd/backlight/light-machine etc.
    log_d("close lcd/backlight/ etc.");
    fd=open("/dev/lvds",O_RDWR);
    if(fd)
    {
        ioctl(fd, LVDS_SET_GPIO_BACKLIGHT,0);//lvds gpio backlight close
		ioctl(fd, LVDS_SET_PWM_BACKLIGHT,0);//lvds pwm backlight close
        close(fd);
    }
    //step 3: stop media player/ 
    bluetooth_set_gpio_backlight(0);
    msleep(200);
    bluetooth_set_gpio_mutu(1);
    msleep(200);
    bluetooth_poweroff();//stop bluetooth
    bluetooth_deinit();
}

// the Keys set in DTS, not here
void enter_standby(void)
{
    int fd_standby;    

    standby_pre_process();
    
    fd_standby = open("/dev/standby", O_RDWR);
    if(fd_standby<0){
        log_e("Open /dev/standby failed!\n");
        return;
    }
    log_d("enter standby!");

    ioctl(fd_standby, STANDBY_ENTER, 0);
    close(fd_standby);
}

