/*
power_mgr.c
 */
#include <unistd.h>
#include <hcuapi/dis.h>
#include <fcntl.h>
#include <hcuapi/standby.h>
#include <sys/ioctl.h>
#include "com_api.h"
#include "cast_api.h"

static void power_pre_set()
{
    //stet 1: Off the display
    api_osd_show_onoff(false);
    api_logo_off();
    api_dis_show_onoff(false);
    //sleep for a while so that hardware display is really off.
    api_sleep_ms(100);


    //Step 2: off other devices
}

/*
we can wake up by 3 ways: ir, gpio and sacadc key. 
 */
int power_enter_standby(void)
{
    int fd = -1;

    fd = open("/dev/standby", O_RDWR);
    if (fd < 0) {
        printf("%s(), line:%d. open standby device error!\n", __func__, __LINE__);
        return API_FAILURE;
    }

    power_pre_set();

    //Step 3: config the standby wake up methods
        
    //config wake up ir scancode(so far, default is power key:28)   
    //check hclinux\SOURCE\linux-drivers\drivers\hcdrivers\rc\keymaps\rc-hcdemo.c
    //for scan code. 
    struct standby_ir_setting ir = { 0 };
    ir.num_of_scancode = 1;
    ir.scancode[0] = 28;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_IR, (unsigned long)&ir);

    //config wake up GPIO
    struct standby_gpio_setting gpio = { 0 };
    gpio.pin = GPIO_NUM_RESET;
    gpio.polarity = 0;//low is active;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_GPIO, (unsigned long)&gpio);

#if 0
    //config wake up adc key
    struct standby_saradc_setting adc = { 0 };
    adc.channel = 1;
    adc.min = 1000;
    adc.max = 1500;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_SARADC, (unsigned long)&adc);
#endif    

#if 0
    //lower the volatage of ddr via the GPIO
    struct standby_pwroff_ddr_setting ddr = { 0 };
    ddr.pin = PINPAD_L09;
    ddr.polarity = 0;//low is active;
    ioctl(fd, STANDBY_SET_PWROFF_DDR, (unsigned long)&ddr);
#endif

    //Step 4: entering system standby
    ioctl(fd, STANDBY_ENTER, 0);
    close(fd);
    while(1);

    return API_SUCCESS;
}

int power_reboot(void)
{
    printf("%s(): reboot now!!\n", __func__);
#ifdef __linux__
    system("reboot");
#else    
    extern int reset(void);
    reset();
#endif    
    while(1);

    return API_SUCCESS;
}


