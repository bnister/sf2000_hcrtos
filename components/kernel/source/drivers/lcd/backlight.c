#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#define MODULE_NAME "/dev/backlight"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/module.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>
#include <hcuapi/pinpad.h>
#include <hcuapi/pinmux.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <nuttx/pwm/pwm.h>
#include <kernel/delay.h>
#include "lcd_main.h"

#define BACKLIGHT_PINPAD_GPIO_NUM 5
#define BACKLIGHT_PWM_SET_DEFAULT 0x7f
#define BACKLIGHT_GPIO_ENABLE 1

typedef struct pinpad_gpio
{
	u32 padctl;
	bool active;
}pinpad_gpio_s;

typedef struct pinpad_gpio_pack
{
	pinpad_gpio_s pad[BACKLIGHT_PINPAD_GPIO_NUM];
	u32 num;
}pinpad_gpio_pack_t;

struct pwm_bl_data {
	pinpad_gpio_pack_t gpio_backlight;
	u32 max_brightness;
	u32 *levels;
	u32 dft_brightness;
	u32 dft_brightness_max;
	struct pwm_info_s blpwm_info;
	const char *pwmbl_path;
	int pwm_backlight_fd;
	int duty_cycle;
	unsigned int scale;
	int lcd_default_off;
};

static struct pwm_bl_data *pbldev = NULL;

static int backlight_set_pwm_duty(u32 duty_cycle)
{
	struct pwm_info_s info = pbldev->blpwm_info;
	if(pbldev->pwmbl_path ==NULL)
		return 0;

	if(pbldev->pwm_backlight_fd <= 0)
	{
		pbldev->pwm_backlight_fd = open(pbldev->pwmbl_path, O_RDWR);
		if(pbldev->pwm_backlight_fd < 0)
		{
			pbldev->pwm_backlight_fd = -1;
			return -1;
		}
	}

	if (pbldev->pwm_backlight_fd > 0) {
		if(pbldev->levels)
		{
			if(duty_cycle >= pbldev->max_brightness)
			{
				duty_cycle = pbldev->dft_brightness_max;
			}
			info.duty_ns=info.period_ns * pbldev->levels[duty_cycle] / pbldev->scale;
			ioctl(pbldev->pwm_backlight_fd, PWMIOC_START, 0);
			ioctl(pbldev->pwm_backlight_fd, PWMIOC_SETCHARACTERISTICS, &info);
		}
		else if(duty_cycle != BACKLIGHT_PWM_SET_DEFAULT)
		{
			info.duty_ns = info.period_ns * duty_cycle / pbldev->scale;
			ioctl(pbldev->pwm_backlight_fd, PWMIOC_START, 0);
			ioctl(pbldev->pwm_backlight_fd, PWMIOC_SETCHARACTERISTICS, &info);
		}
		else if(duty_cycle == BACKLIGHT_PWM_SET_DEFAULT)
		{
			ioctl(pbldev->pwm_backlight_fd, PWMIOC_START, 0);
			ioctl(pbldev->pwm_backlight_fd, PWMIOC_SETCHARACTERISTICS, &info);
		}
	}
	log_d("%s %d info.duty_ns =%ld info.period_ns = %ld\n",__func__,__LINE__,info.duty_ns,info.period_ns);
	return 0;
}

static void backlight_set_gpio_status(char value)
{
	u32 i=0;
	for(i=0;i<pbldev->gpio_backlight.num;i++)
	{
		gpio_configure(pbldev->gpio_backlight.pad[i].padctl, GPIO_DIR_OUTPUT);
		if(value != 0)
			lcd_gpio_set_output(pbldev->gpio_backlight.pad[i].padctl, !pbldev->gpio_backlight.pad[i].active);
		else
			lcd_gpio_set_output(pbldev->gpio_backlight.pad[i].padctl, pbldev->gpio_backlight.pad[i].active);
	}
	log_d("%s %d pbldev->gpio_backlight.pad[i].padctl = %d value = %d\n",__func__,__LINE__,pbldev->gpio_backlight.pad[i].padctl,value);
}

static int backlight_close(struct file *filep)
{
	return 0;
}

static int backlight_open(struct file *filep)
{
	return 0;
}

static ssize_t backlight_write(struct file *filep, const char *buffer, size_t buflen)
{
	if(buffer == NULL)
		return -EFAULT;
	log_d("%s %d %d %ld\n",__func__,__LINE__,buffer[0],buflen);
	
	if(backlight_set_pwm_duty(buffer[0]) != 0)
	{
		return 0;
	}
	backlight_set_gpio_status(buffer[0]);
	pbldev->duty_cycle = buffer[0];
	return buflen;
}

static ssize_t backlight_read(struct file *filep, char *buffer, size_t buflen)
{
	if(buffer == NULL)
		return -EFAULT;

	buffer[0] = pbldev->duty_cycle;

	return buflen;
}

static const struct file_operations backlight_fops = {
	.open = backlight_open,
	.close = backlight_close,
	.read = backlight_read,
	.write = backlight_write,
};

static int backlight_probe(const char *node)
{
	int ret;
	u32 tmpVal = 0;
	u32 i = 0;
	const char *path=NULL;
	int np = fdt_node_probe_by_path(node);

	if(np < 0){
		goto error;
	}

	log_d("%s %d\n",__func__,__LINE__);
	if(pbldev != NULL) return 0;

	pbldev = (struct pwm_bl_data *)malloc(sizeof(struct pwm_bl_data));
	if(pbldev == NULL)
	{
		log_e("malloc error\n");
		goto malloc_error;
	}

	memset(pbldev, 0, sizeof(struct pwm_bl_data));

	if (fdt_get_property_data_by_name(np, "backlight-gpios-rtos", &pbldev->gpio_backlight.num) == NULL)
	{
		pbldev->gpio_backlight.num = 0;
	}
	else
	{
		pbldev->dft_brightness = 1;
	}

	pbldev->gpio_backlight.num >>= 3;

	if(pbldev->gpio_backlight.num > BACKLIGHT_PINPAD_GPIO_NUM)
		pbldev->gpio_backlight.num = BACKLIGHT_PINPAD_GPIO_NUM;

	for(i=0;i<pbldev->gpio_backlight.num;i++){
		pbldev->gpio_backlight.pad[i].padctl = PINPAD_INVALID;
		pbldev->gpio_backlight.pad[i].active = GPIO_ACTIVE_HIGH;

		if(fdt_get_property_u_32_index(np, "backlight-gpios-rtos", i * 2, &tmpVal)==0)
			pbldev->gpio_backlight.pad[i].padctl = tmpVal;
		if(fdt_get_property_u_32_index(np, "backlight-gpios-rtos", i * 2 + 1, &tmpVal)==0)
			pbldev->gpio_backlight.pad[i].active = tmpVal;
		log_d("%s %d pad = %d active = %d\n",__func__,__LINE__,pbldev->gpio_backlight.pad[i].padctl,pbldev->gpio_backlight.pad[i].active);
	}

	pbldev->dft_brightness_max = 1;
	if (fdt_get_property_string_index(np, "backlight-pwmdev", 0, &path)==0)
	{
		pbldev->pwmbl_path = path;
		

		pbldev->max_brightness = 0;
		if (fdt_get_property_data_by_name(np, "brightness-levels", &pbldev->max_brightness) == NULL)
			pbldev->max_brightness = 0;

		pbldev->max_brightness >>= 2;
		
		if(pbldev->max_brightness > 0)
		{
			pbldev->levels = malloc(pbldev->max_brightness * sizeof(u32));
			for(i = 0;i < pbldev->max_brightness;i++)
			{
				fdt_get_property_u_32_index(np, "brightness-levels", i, &pbldev->levels[i]);
				log_d("%s %d brightness-levels = %d\n",__func__,__LINE__,pbldev->levels[i]);
			}
			if (pbldev->levels) {
				for (i = 0; i < pbldev->max_brightness; i++)
					if (pbldev->levels[i] > pbldev->scale)
					{
						pbldev->dft_brightness_max = i;
						pbldev->scale = pbldev->levels[i];
					}
					
				} else
					pbldev->scale = 100;//pbldev->max_brightness;
		}
		else
		{
			pbldev->levels = NULL;
		}
		ret = fdt_get_property_u_32_index(np, "default-brightness-level", 0, &tmpVal);
		if(ret == 0)
			pbldev->dft_brightness = tmpVal;
		else
			pbldev->dft_brightness = pbldev->dft_brightness_max;

		ret = fdt_get_property_u_32_index(np, "backlight-frequency", 0, &tmpVal);
		if(ret == 0)
			pbldev->blpwm_info.period_ns = 1000000000/tmpVal;

		log_d("%s %d pbldev->pwmbl_path = %s pbldev->blpwm_info.period_ns = %ld pbldev->max_brightness = %d pbldev->dft_brightness =%d pbldev->scale =%d pbldev->dft_brightness_max = %d\n", \
		__func__,__LINE__,pbldev->pwmbl_path, pbldev->blpwm_info.period_ns,pbldev->max_brightness,pbldev->dft_brightness,pbldev->scale,pbldev->dft_brightness_max);
		pbldev->pwm_backlight_fd = 0;
	}
	else
	{
		pbldev->pwmbl_path = NULL;
		pbldev->pwm_backlight_fd = -1;
	}

	pbldev->duty_cycle = pbldev->dft_brightness;
	pbldev->lcd_default_off = fdt_property_read_bool(np, "default-off");
	if(pbldev->lcd_default_off)
		goto backlight_register;

	backlight_set_pwm_duty(pbldev->dft_brightness);
	backlight_set_gpio_status(1);

backlight_register:
	return register_driver(MODULE_NAME, &backlight_fops, 0666, NULL);
malloc_error:
error:
	pbldev=NULL;
	return 0;
}

static int backlight_init(void)
{
	backlight_probe("/hcrtos/backlight");
	return 0;
}

static int backlight_exit(void)
{
	if(pbldev!=NULL)
		free(pbldev);
	unregister_driver(MODULE_NAME);
}

module_driver(backlight, backlight_init, backlight_exit, 2)

