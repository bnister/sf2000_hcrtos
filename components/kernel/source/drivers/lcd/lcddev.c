#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR
#define MODULE_NAME "/dev/lcddev"

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
#include <hcuapi/lvds.h>
#include <nuttx/pwm/pwm.h>
#include <kernel/delay.h>

#include "lcd_main.h"
#define LCD_PINPAD_GPIO_NUM 5
#ifndef LCD_GPIO_DISABLE
#define LCD_GPIO_DISABLE	0
#endif
#ifndef LCD_GPIO_ENABLE
#define LCD_GPIO_ENABLE		1
#endif

typedef struct pinpad_gpio
{
	u32 padctl;
	bool active;
}pinpad_gpio_s;

typedef struct pinpad_gpio_pack
{
	pinpad_gpio_s pad[LCD_PINPAD_GPIO_NUM];
	u32 num;
}pinpad_gpio_pack_s;

typedef struct pwm_info_pack
{
	struct pwm_info_s info;
	const char *path;
	int fd;
}pwm_info_pack_s;

struct hichip_lcd {
    pwm_info_pack_s pwm_backlight;
	pinpad_gpio_pack_s gpio_backlight;
	pwm_info_pack_s pwm_vcom;
	pinpad_gpio_pack_s gpio_power;
    u32 init_delay_ms;
	u32 reset_delay_ms;
	u32 enable_delay_ms;
	u32 prepare_delay_ms;
	pinpad_gpio_s gpio_reset;
	int lcd_default_off;
	u32 lcd_vcom_default_off;
	const char *map_name;
	struct lcd_map *disdev;
	int lcd_int_status;
};
static struct hichip_lcd *lcddev = NULL;

static void lcd_set_gpio_power(unsigned long value)
{
	u32 i=0;
	if(lcddev->disdev!=NULL && lcddev->disdev->lcd_power_onoff != NULL)
	{
		lcddev->disdev->lcd_power_onoff(value);
	}

	for(i=0;i<lcddev->gpio_power.num;i++)
	{
		gpio_configure(lcddev->gpio_power.pad[i].padctl, GPIO_DIR_OUTPUT);
		if(value == LCD_GPIO_ENABLE)
			lcd_gpio_set_output(lcddev->gpio_power.pad[i].padctl, !lcddev->gpio_power.pad[i].active);
		else
			lcd_gpio_set_output(lcddev->gpio_power.pad[i].padctl, lcddev->gpio_power.pad[i].active);
	}
}

static void lcd_gpio_reset(void)
{
	log_d("lcddev->gpio_reset.padctl = %d active %d\n",lcddev->gpio_reset.padctl,lcddev->gpio_reset.active);
	if(lcddev->gpio_reset.padctl == PINPAD_INVALID) return;
	gpio_configure(lcddev->gpio_reset.padctl,GPIO_DIR_OUTPUT);
	lcd_gpio_set_output(lcddev->gpio_reset.padctl,!lcddev->gpio_reset.active);
	msleep(lcddev->reset_delay_ms);
	lcd_gpio_set_output(lcddev->gpio_reset.padctl,lcddev->gpio_reset.active);
	msleep(lcddev->prepare_delay_ms);
}

static void ioctl_lcd_init_all(void)
{
	if(lcddev == NULL) return;

	lcd_set_gpio_power(1);

	if(lcddev->gpio_power.num)
		msleep(lcddev->enable_delay_ms);

	lcd_gpio_reset();

	/*SPI display initialization*/
	if(lcddev->disdev==NULL)
	{
		log_d("p_lcd_dev = NULL\n");
		return;
	}

    if(lcddev->disdev->lcd_init!=NULL)
	{
		lcddev->disdev->lcd_init();
	}

	if(lcddev->disdev->lcd_onoff!=NULL)
	{
		lcddev->disdev->lcd_onoff(1);
	}
}


static void ioctl_lcd_init(void)
{
    if(lcddev->disdev==NULL)
	{
		log_d("p_lcd_dev = NULL\n");
		return;
	}
    if(lcddev->disdev->lcd_init==NULL)
	{
		log_d(" lcd_init\n");
		return;
	}
    lcddev->disdev->lcd_init();
}

static int lcd_ioctl_set_rotate(lcd_rotate_type_e rotate)
{
	if(lcddev->disdev == NULL)
	{
		log_e("disdev ==NULL\n");
		return LCD_RET_ERROR;
	}

	if(lcddev->disdev->lcd_rorate ==NULL)
	{
		log_e("lcddev->disdev->lcd_rorate\n");
		return LCD_RET_ERROR;
	}

	log_d("rotate =%d\n",rotate);
	lcddev->disdev->lcd_rorate(rotate);
	return LCD_RET_SUCCESS;
}

static int lcd_ioctl_set_onoff(unsigned long onoff)
{
	if(lcddev->disdev == NULL)
	{
		log_e("disdev ==NULL\n");
		return LCD_RET_ERROR;
	}

	if(lcddev->disdev->lcd_onoff ==NULL)
	{
		log_e("lcddev->disdev->lcd_onoff\n");
		return LCD_RET_ERROR;
	}

	log_d("onoff =%ld\n",onoff);
	lcddev->disdev->lcd_onoff(onoff);
	return LCD_RET_SUCCESS;
}

static void lcd_set_pwm_vcom_duty(unsigned long duty)
{
	struct pwm_info_s info = lcddev->pwm_vcom.info;
	if(lcddev->pwm_vcom.path ==NULL)
		return;

	if(lcddev->pwm_vcom.fd <= 0)
	{
		lcddev->pwm_vcom.fd = open(lcddev->pwm_vcom.path, O_RDWR);
	}

	if (lcddev->pwm_vcom.fd > 0) {
		if(duty<=100)
		{
			info.duty_ns=info.period_ns/100*duty;
			ioctl(lcddev->pwm_vcom.fd, PWMIOC_START, 0);
			ioctl(lcddev->pwm_vcom.fd, PWMIOC_SETCHARACTERISTICS, &info);
		}
		else if(duty == LCD_PWM_SET_DEFAULT)
		{
			ioctl(lcddev->pwm_vcom.fd, PWMIOC_START, 0);
			ioctl(lcddev->pwm_vcom.fd, PWMIOC_SETCHARACTERISTICS, &info);
		}
	}
}

#if 1
static int lcd_ioctl_send_cmds(struct hc_lcd_master_dev *cmds)
{

	log_d("%s %d\n",__func__,__LINE__);
	if(cmds == NULL)
	{
		log_e("%s %d date ==NULL \n",__func__,__LINE__);
		return LCD_RET_ERROR;
	}
	if(lcddev->disdev ==NULL)
	{
		log_e("%s %d lcddev->disdev == NULL \n",__func__,__LINE__);
		return LCD_RET_ERROR;
	}
	if(lcddev->disdev->lcd_write_cmds ==NULL)
	{
		log_e("%s %d lcd_write_cmds == NULL \n",__func__,__LINE__);
		return LCD_RET_ERROR;
	}

	lcddev->disdev->lcd_write_cmds(cmds->packet,cmds->count);
}

static int lcd_ioctl_send_data(struct hc_lcd_master_dev *data)
{

	log_d("%s %d\n",__func__,__LINE__);
	if(data == NULL)
	{
		log_e("%s %d date ==NULL \n",__func__,__LINE__);
		return LCD_RET_ERROR;
	}
	if(lcddev->disdev ==NULL)
	{
		log_e("%s %d lcddev->disdev == NULL \n",__func__,__LINE__);
		return LCD_RET_ERROR;
	}
	if(lcddev->disdev->lcd_write_data ==NULL)
	{
		log_e("%s %d lcd_write_data == NULL \n",__func__,__LINE__);
		return LCD_RET_ERROR;
	}

	log_d("%s %d %d\n",__func__,__LINE__,data->packet[i]);
	lcddev->disdev->lcd_write_data(data->packet,data->count);

}

static int lcd_ioctl_read_data(struct hc_lcd_spi_read *pack)
{
	int i = 0;
	if(pack == NULL)return LCD_RET_ERROR;
	if(lcddev->disdev->lcd_write_cmds ==NULL)return LCD_RET_ERROR;
	if(lcddev->disdev->lcd_read_data ==NULL)return LCD_RET_ERROR;

	lcddev->disdev->lcd_write_cmds(pack->command_data,pack->command_size);

	while(pack->received_size--)
		pack->received_data[i++] = lcddev->disdev->lcd_read_data(0);
}
#endif

void lcd_ioctl_driver_init(void)
{
    if(lcddev->disdev==NULL)
	{
		log_e("lcddev->disdev = NULL\n");
		return;
	}
    if(lcddev->disdev->lcd_init==NULL)
	{
		log_e(" lcd_init\n");
		return;
	}
    lcddev->disdev->lcd_init();
}

#if 1
static int lcd_ioctl(FAR struct file *file, int cmd, unsigned long arg)
{
	int ret = 0;
	u32 val = (u32)arg;
	switch (cmd){
	case LCD_INIT_ALL:
		ioctl_lcd_init_all();
		break;
	case LCD_DRIVER_INIT:
		lcd_ioctl_driver_init();
		break;
	case LCD_DRIVER_GET_INIT_STATUS:
		memcpy((int*)val,&lcddev->lcd_int_status,sizeof(int));
		break;
	case LCD_SEND_CMDS:
		ret = lcd_ioctl_send_cmds((struct hc_lcd_master_dev *)val);
		break;
	case LCD_SEND_DATE:
		ret = lcd_ioctl_send_data((struct hc_lcd_master_dev *)val);
		break;
	case LCD_READ_DATE:
		ret = lcd_ioctl_read_data((struct hc_lcd_spi_read *)val);
		break;
	case LCD_SET_ROTATE:
		lcd_ioctl_set_rotate((lcd_rotate_type_e)val);
		break;
	case LCD_SET_ONOFF:
		lcd_ioctl_set_onoff(val);
		break;
	case LCD_SET_PWM_VCOM:
		lcd_set_pwm_vcom_duty(val);
		break;
	case LCD_SET_GPIO_POWER:
		lcd_set_gpio_power(val);
		break;
	case LCD_SET_GPIO_RESET:
		lcd_gpio_reset();
		break;
	default:
		return -EINVAL;
	}

	return ret;
}
#endif

static int lcd_close(struct file *filep)
{
	return 0;
}

static int lcd_open(struct file *filep)
{
	return 0;
}

static ssize_t lcd_write(struct file *filep, const char *buffer, size_t buflen)
{
	if(buffer == NULL)
		return -EFAULT;
	log_d("%s %d %d %ld\n",__func__,__LINE__,buffer[0],buflen);
	
	if(buffer[0])
	{
		ioctl_lcd_init_all();
	}
	else
	{
		lcd_ioctl_set_onoff(0);
		lcd_set_gpio_power(LCD_GPIO_DISABLE);
	}
	return buflen;
}

static ssize_t lcd_read(struct file *filep, char *buffer, size_t buflen)
{
	if(buffer == NULL)
		return -EFAULT;

	buffer[0] = lcddev->lcd_int_status;

	return buflen;
}

static const struct file_operations lcd_fops = {
	.open = lcd_open,
	.close = lcd_close,
	.read = lcd_read,
	.write = lcd_write,
	// .ioctl = lcd_ioctl,
};

static bool is_bootlogo_showing(void)
{
	u32 de;
	u32 deshowing = 0;
	hc_clk_enable(DE_CLK);
	hc_clk_enable(DVI_CLK);
	hc_clk_enable(DE_HDMI_CLK);

	de = 0xB8808000;
	deshowing = (REG32_READ(de + 0x190) & BIT(21)) && (REG32_READ(de) & BIT(0));
	return !!(deshowing);
}

static int lcd_probe(const char *node)
{
	int ret;
	u32 tmpVal = 0;
	u32 i;
	const char *path=NULL;
	int np = fdt_node_probe_by_path(node);

	if(np < 0){
		goto error;
	}

	if(lcddev != NULL) return 0;

	lcddev = (struct hichip_lcd *)malloc(sizeof(struct hichip_lcd));
	if(lcddev == NULL)
	{
		log_e("malloc error\n");
		goto malloc_error;
	}

	memset(lcddev, 0, sizeof(struct hichip_lcd));

	//Get device information
	if (fdt_get_property_data_by_name(np, "power-gpios-rtos", &lcddev->gpio_power.num) == NULL)
		lcddev->gpio_power.num = 0;

	lcddev->gpio_power.num >>= 3;

	if(lcddev->gpio_power.num > LCD_PINPAD_GPIO_NUM)
		lcddev->gpio_power.num = LCD_PINPAD_GPIO_NUM;

	for(i=0;i<lcddev->gpio_power.num;i++){
		lcddev->gpio_power.pad[i].padctl = PINPAD_INVALID;
		lcddev->gpio_power.pad[i].active = GPIO_ACTIVE_HIGH;

		if(fdt_get_property_u_32_index(np, "power-gpios-rtos", i * 2, &tmpVal)==0)
			lcddev->gpio_power.pad[i].padctl = tmpVal;
		if(fdt_get_property_u_32_index(np, "power-gpios-rtos", i * 2 + 1, &tmpVal)==0)
			lcddev->gpio_power.pad[i].active = tmpVal;
	}
	log_d("%s %d %d\n",__func__,__LINE__,lcddev->gpio_power.num);

	if (fdt_get_property_string_index(np, "vcom-pwmdev", 0, &path)==0)
	{
		ret = fdt_get_property_u_32_index(np, "vcom-frequency", 0, &tmpVal);
		if(ret == 0)
			lcddev->pwm_vcom.info.period_ns = tmpVal;

		ret = fdt_get_property_u_32_index(np, "vcom-duty", 0, &tmpVal);
		if (ret == 0)
			lcddev->pwm_vcom.info.duty_ns = tmpVal;

		ret = fdt_get_property_u_32_index(np, "vcom-polar", 0, &tmpVal);
		if (ret == 0)
			lcddev->pwm_vcom.info.duty_ns = tmpVal;

		lcddev->pwm_vcom.path = path;
		// lcddev->pwm_vcom.fd = open(path, O_RDWR);
	}
	else
	{
		lcddev->pwm_vcom.fd = -1;
		lcddev->pwm_vcom.path = NULL;
	}

	lcddev->gpio_reset.padctl = PINPAD_INVALID;

	fdt_get_property_u_32_index(np, "lcd-reset-gpios-rtos", 			0, &lcddev->gpio_reset.padctl);
	if(!fdt_get_property_u_32_index(np, "lcd-reset-gpios-rtos", 			1, &tmpVal))
		lcddev->gpio_reset.active = tmpVal;

	lcddev->init_delay_ms		= 10;
	lcddev->enable_delay_ms		= 100;
	lcddev->reset_delay_ms		= 120;
	lcddev->prepare_delay_ms	= 10;
	fdt_get_property_u_32_index(np, "init-delay-ms", 	0, &lcddev->init_delay_ms);
	fdt_get_property_u_32_index(np, "reset-delay-ms", 	0, &lcddev->reset_delay_ms);
	fdt_get_property_u_32_index(np, "enable-delay-ms", 	0, &lcddev->enable_delay_ms);
	fdt_get_property_u_32_index(np, "prepare-delay-ms", 0, &lcddev->prepare_delay_ms);

	lcddev->lcd_default_off = fdt_property_read_bool(np, "default-off");
	lcddev->lcd_vcom_default_off = fdt_property_read_bool(np, "vcom-default-off");

	lcddev->map_name = NULL;
	fdt_get_property_string_index(np, "lcd-map-name", 0, &lcddev->map_name);

	log_d("lcddev->map_name =%s\n",lcddev->map_name);
	lcddev->disdev = lcd_map_get(lcddev->map_name);
	if(lcddev->disdev == NULL)
	{
		log_e("lcddev->disdev == NULL error");
	}
	lcddev->lcd_int_status = LCD_CURRENT_NOT_INIT;
	//Determine whether the initialization is repeated
	if(is_bootlogo_showing())
	{
		lcddev->lcd_int_status = LCD_CURRENT_IS_INIT;
		goto lcd_register;
	}

	if(lcddev->lcd_default_off == 1)
	{
		printf("%s %d lcd default off\n",__func__,__LINE__);
		goto lcd_register;
	}

	/*
		Device initialization
	*/
	if(lcddev->lcd_vcom_default_off == 0)
		lcd_set_pwm_vcom_duty(LCD_PWM_SET_DEFAULT);

	lcd_set_gpio_power(1);
	lcd_gpio_reset();
	if(lcddev->disdev != NULL && lcddev->disdev->lcd_init !=NULL)
	{
		if(lcddev->disdev->default_off_val == 1)
		{
			lcddev->disdev->lcd_init();
		}
		lcddev->lcd_int_status = LCD_CURRENT_IS_INIT;
	}

lcd_register:
	return register_driver(MODULE_NAME, &lcd_fops, 0666, NULL);
malloc_error:
error:
	lcddev=NULL;
	return 0;
}

static int lcd_init(void)
{
	lcd_probe("/hcrtos/lcd");
	return 0;
}

static int lcd_exit(void)
{
	if(lcddev!=NULL)
		free(lcddev);
	unregister_driver(MODULE_NAME);
}

module_driver(lcd, lcd_init, lcd_exit, 3)

