#define LOG_TAG "boot_backlight"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <kernel/hwspinlock.h>
#include <kernel/completion.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/dis.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/sysdata.h>
#include <backlight.h>
#include <hcuapi/lvds.h>
#include <kernel/delay.h>
#include <hcuapi/gpio.h>
#if defined(BR2_PACKAGE_BLUETOOTH)
#include <bluetooth.h>
#endif

#ifndef CONFIG_BOOT_BACKLIGHT_DELAY_TIME
#define CONFIG_BOOT_BACKLIGHT_DELAY_TIME 0
#endif

typedef enum BACKLGIHT_TYPE
{
	BACKLIGHT_DEV,
	BACKLIGHT_LVDS,
}backlight_type_e;

static int api_set_backlight_brightness(backlight_type_e type, int val)
{
	int lvds_fd;
	int backlight_fd = 0;

	if(type == BACKLIGHT_DEV)
	{
		backlight_fd = open("/dev/backlight",O_RDWR);
		if (backlight_fd < 0) {
			log_e ("open backlight failed\n");
			return -1;
		}
		write(backlight_fd,&val,4);
		close(backlight_fd);
	}
	else if(type == BACKLIGHT_LVDS)
	{
		lvds_fd = open("/dev/lvds",O_RDWR);
		if (lvds_fd < 0) {
			log_e ("open lvds failed\n");
			return -1;
		}

		ioctl(lvds_fd, LVDS_SET_PWM_BACKLIGHT,val);//lvds set pwm default
		ioctl(lvds_fd, LVDS_SET_GPIO_BACKLIGHT,val);//lvds gpio backlight close
		close(lvds_fd);
	}

	return 0;
}

int open_lcd_backlight(int argc, char *argv[])
{
	int fd;
	unsigned int tmp = 0, backlight_value = 0;
	lvds_channel_mode_e lvds_channel_mode = 0;

	msleep(CONFIG_BOOT_BACKLIGHT_DELAY_TIME);

	int backlight_np=fdt_node_probe_by_path("/hcrtos/backlight");
	if(backlight_np > 0)
	{
		backlight_value = 100;
		fdt_get_property_u_32_index(backlight_np, "default-brightness-level", 0, &backlight_value);//set default-brightness-level
		api_set_backlight_brightness(BACKLIGHT_DEV, backlight_value);
	}

	fd = open("/dev/lvds", O_RDWR);
	if(fd>0)
	{
		ioctl(fd,LVDS_GET_CHANNEL_MODE, &lvds_channel_mode);
		log_d("lvds_channel_mode %d\n", lvds_channel_mode);
		if(lvds_channel_mode == LVDS_CHANNEL_MODE_SINGLE_IN_DUAL_OUT)
		{
			usleep(16000);
			ioctl(fd, LVDS_SET_TRIGGER_EN, NULL);
		}

#ifdef CONFIG_BOOT_PWM_BACKLIGHT_MONITOR
		int lcd_np=fdt_node_probe_by_path("/hcrtos/lcd");
		unsigned int pwm_battery_duty = 50;
		unsigned int pwm_power_duty = 100;
		pinpad_e backlight_detection_pad = PINPAD_INVALID;
		u32 temp = PINPAD_INVALID;
		if(lcd_np > 0)
		{
			fdt_get_property_u_32_index(lcd_np, "backlight-detection", 0, (u32 *)&temp);
			backlight_detection_pad = temp;
			fdt_get_property_u_32_index(lcd_np, "pwm-batter-duty", 0, (u32 *)&pwm_battery_duty);
			fdt_get_property_u_32_index(lcd_np, "pwm-power-duty", 0, (u32 *)&pwm_power_duty);
			gpio_configure(backlight_detection_pad, GPIO_DIR_INPUT);
			msleep(10);
			if(gpio_get_input(backlight_detection_pad))
			{
				backlight_value = pwm_battery_duty;
			}
			else
			{
				backlight_value = pwm_power_duty;
			}
			printf("%s %d  %d %d pwm backlight_value = %d\n",__func__,__LINE__,pwm_power_duty,pwm_battery_duty,backlight_value);
			api_set_backlight_brightness(BACKLIGHT_LVDS, backlight_value);
			api_set_backlight_brightness(BACKLIGHT_DEV, backlight_value);
		}
#else
		backlight_value = 101;//lvds backlight default value
		api_set_backlight_brightness(BACKLIGHT_LVDS, backlight_value);
#endif
		close(fd);
	}

#ifdef CONFIG_BOOT_BLUETOOTH_BACKLIGHT
#ifdef BR2_PACKAGE_BLUETOOTH
	const char *devpath=NULL;
	int np=fdt_node_probe_by_path("/hcrtos/bluetooth");
	if(np>0)
	{
		if(!fdt_get_property_string_index(np, "devpath", 0, &devpath))
		{
			if(bluetooth_init(devpath, NULL) == 0){
				log_d("%s %d bluetooth_init ok\n",__FUNCTION__,__LINE__);
			}else{
				log_e("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);
			}
			bluetooth_set_gpio_backlight(1);
			bluetooth_set_gpio_mutu(0);
			bluetooth_set_cvbs_fiber_mode();
			bluetooth_deinit();
		}
	}
	else
		log_e("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);
#endif
#endif
}

CONSOLE_CMD(backlight, NULL, open_lcd_backlight, CONSOLE_CMD_MODE_SELF, "boot enter standby")
