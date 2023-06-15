#include "app_config.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <hcuapi/standby.h>
#include <hcuapi/lvds.h>
#include <hcuapi/mipi.h>
#include "screen.h"
#include "factory_setting.h"
#include "com_api.h"

#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif

#ifdef __HCRTOS__
//#include <kernel/elog.h>
//#include <kernel/delay.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <kernel/io.h>
#endif
#include "channel/local_mp/media_player.h"
#include "channel/local_mp/mp_ctrlbarpage.h"

#ifdef __HCRTOS__
//Do not enter standby repeatedly
static void set_bootloader_standby_flag(void)
{
    int i;
    unsigned int ddr_addr=0xa0000200;
	unsigned int ddr_value=0x12345678;
    for (i = 0; i < 1024; i += 4) {
        REG32_WRITE(ddr_addr + i, ddr_value);
    }
}
#endif

static void standby_pre_process(void);
// pre process before enter standby mode
static void standby_pre_process(void)
{
    int fd;
    int temp = 0;
    //step 1: stop device & save system data	
    hdmi_rx_leave();
    cvbs_rx_stop();
    media_player_close();

    projector_sys_param_save();

#if PROJECTER_C2_D3000_VERSION
    api_set_i2so_gpio_mute(true);
#endif

#ifdef  BLUETOOTH_SUPPORT   
    bluetooth_set_gpio_backlight(0);
    bluetooth_set_gpio_mutu(1);
    printf("bluetooth disconnect test\n");
    bluetooth_poweroff();//stop bluetooth
    api_sleep_ms(200);
    bluetooth_deinit();
#endif    

    //step 2:  close lcd/backlight/light-machine etc.
    printf("close lcd/backlight/ etc.\n");

	api_set_backlight_brightness(0);

	fd = open("/dev/lvds",O_RDWR);
    if(fd)
    {
        ioctl(fd, LVDS_SET_GPIO_POWER,0);//lvds gpio power close
        close(fd);
    }

	fd = open("/dev/mipi",O_RDWR);
	{
		ioctl(fd, MIPI_DSI_GPIO_ENABLE,0);//mipi close gpio enable
        close(fd);
	}

	fd = open("/dev/lcddev",O_RDWR);//lcddev close gpio enable
	if(fd)
	{
		temp = 0;
		write(fd,&temp,4);
		close(fd);
	}

    api_osd_show_onoff(false);
    api_logo_off();
    api_dis_show_onoff(false);
    api_sleep_ms(100);

    //step 3: stop media player/ 

#ifdef __HCRTOS__
    //setp 4: set standby flag,Do not enter standby repeatedly
    set_bootloader_standby_flag();
#endif    
}

// the Keys set in DTS, not here
void enter_standby(void)
{
    int fd_standby;    

    standby_pre_process();
    
    fd_standby = open("/dev/standby", O_RDWR);
    if(fd_standby<0){
        printf("Open /dev/standby failed!\n");
        return;
    }
    printf("enter standby!\n");

    api_watchdog_stop();

#ifdef __HCRTOS__    
    taskDISABLE_INTERRUPTS();
#endif    
    ioctl(fd_standby, STANDBY_ENTER, 0);
    close(fd_standby);
}

