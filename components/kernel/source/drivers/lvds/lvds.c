#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR
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

#include "lvds.h"
#include "lvds_lld.h"
#include "lvds_hal.h"
#include <hcuapi/lvds.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>
#include <hcuapi/pinpad.h>
#include <hcuapi/pinmux.h>
#include <kernel/drivers/hc_clk_gate.h>
// #include "display/lcd_spi.h"
#define MODULE_NAME "/dev/lvds"
#include <nuttx/pwm/pwm.h>
#include <hcuapi/standby.h>

typedef struct pinpad_gpio
{
	pinpad_e padctl;
	bool active;
}pinpad_gpio_s;
#define LCD_BACKLIGHT_NUM_MAX 5
#define LCD_POWER_NUM_MAX 10
#define LVDS_GPIO_LOW 0
#define LVDS_GPIO_HIGH 1
struct hichip_lvds {
	void *reg_base;
	void *sys_base;
	int vcom_fd;
	int backlight_fd;
	struct pwm_info_s pwm_backlight;
	struct pwm_info_s pwm_vcom;
	pinpad_gpio_s rgb_backlight;
	pinpad_gpio_s lcd_backlight[LCD_BACKLIGHT_NUM_MAX];
	pinpad_gpio_s lcd_power[LCD_POWER_NUM_MAX];
	u32 lcd_backlight_num;
	u32 lcd_power_num;
	u32 src_sel;
};

static struct hichip_lvds lvds_misc;

static void lvds_set_pwm_vcom_duty(unsigned long duty)
{
	struct pwm_info_s info = lvds_misc.pwm_vcom;
	if (lvds_misc.vcom_fd > 0) {
		if(duty<=100)
		{
			info.duty_ns=info.period_ns/100*duty;
			ioctl(lvds_misc.vcom_fd, PWMIOC_START, 0);
			ioctl(lvds_misc.vcom_fd, PWMIOC_SETCHARACTERISTICS, &info);
		}
		else
		{
			ioctl(lvds_misc.vcom_fd, PWMIOC_START, 0);
			ioctl(lvds_misc.vcom_fd, PWMIOC_SETCHARACTERISTICS, &info);
		}
	}
}

static void lvds_set_pwm_backlight_duty(unsigned long duty)
{
	struct pwm_info_s info = lvds_misc.pwm_backlight;
	if (lvds_misc.backlight_fd > 0) {
		if(duty<=100)
		{
			info.duty_ns=info.period_ns/100*duty;
			ioctl(lvds_misc.backlight_fd, PWMIOC_START, 0);
			ioctl(lvds_misc.backlight_fd, PWMIOC_SETCHARACTERISTICS, &info);
		}
		else
		{
			ioctl(lvds_misc.backlight_fd, PWMIOC_START, 0);
			ioctl(lvds_misc.backlight_fd, PWMIOC_SETCHARACTERISTICS, &info);
		}
	}
}

static void lvds_set_gpio_backlight(unsigned long value)
{
	u32 i=0;
	for(i=0;i<lvds_misc.lcd_backlight_num;i++)
	{
		gpio_configure(lvds_misc.lcd_backlight[i].padctl, GPIO_DIR_OUTPUT);
		if(value==LVDS_GPIO_LOW)
			gpio_set_output(lvds_misc.lcd_backlight[i].padctl, lvds_misc.lcd_backlight[i].active);
		else
			gpio_set_output(lvds_misc.lcd_backlight[i].padctl, !lvds_misc.lcd_backlight[i].active);
	}
}

static void lvds_set_gpio_power(unsigned long value)
{
	u32 i=0;
	for(i=0;i<lvds_misc.lcd_power_num;i++)
	{
		gpio_configure(lvds_misc.lcd_power[i].padctl, GPIO_DIR_OUTPUT);
		if(value==LVDS_GPIO_LOW)
			gpio_set_output(lvds_misc.lcd_power[i].padctl, lvds_misc.lcd_power[i].active);
		else
			gpio_set_output(lvds_misc.lcd_power[i].padctl, !lvds_misc.lcd_power[i].active);
	}
}

static void lvds_get_channel_mode(lvds_channel_mode_e *val)
{
	if(val==NULL)return;
	*val = lvds_hal_get_channel_mode();
	log_d("val =%d\n",*val);
}
static int lvds_ioctl(FAR struct file *file, int cmd, unsigned long arg)
{
	u32 val = (u32)arg;
	switch (cmd){
	case LVDS_SET_CHANNEL_MODE:
		lvds_hal_set_channel_mode((lvds_channel_mode_e)val);
		break;
	case LVDS_SET_MAP_MODE:
		lvds_hal_set_data_map_format(val);
		break;
	case LVDS_SET_CH0_SRC_SEL:
		lvds_hal_set_ch0_src_sel(val);
		break;
	case LVDS_SET_CH1_SRC_SEL:
		lvds_hal_set_ch1_src_sel(val);
		break;
	case LVDS_SET_CH0_INVERT_CLK_SEL:
		lvds_hal_set_ch0_invert_clk_sel(val);
		break;
	case LVDS_SET_CH1_INVERT_CLK_SEL:
		lvds_hal_set_ch1_invert_clk_sel(val);
		break;
	case LVDS_SET_CH0_CLK_GATE:
		lvds_hal_set_ch0_clk_gate(val);
		break;
	case LVDS_SET_CH1_CLK_GATE:
		lvds_hal_set_ch1_clk_gate(val);
		break;
	case LVDS_SET_HSYNC_POLARITY:
		lvds_hal_set_hsync_polarity(val);
		break;
	case LVDS_SET_VSYNC_POLARITY:
		lvds_hal_set_vsync_polarity(val);
		break;
	case LVDS_SET_EVEN_ODD_ADJUST_MODE:
		lvds_hal_set_even_odd_adjust_mode(val);
		break;
	case LVDS_SET_EVEN_ODD_INIT_VALUE:
		lvds_hal_set_even_odd_init_value(val);
		break;
	case LVDS_SET_SRC_SEL:
		lvds_hal_set_src_sel(val);
		break;
	case LVDS_SET_RESET:
		lvds_hal_reset();
		break;
	case LVDS_SET_POWER_TRIGGER:
		lvds_lld_phy_mode_init(0,0);
		lvds_lld_phy_mode_init(1,0);
		break;
	case LVDS_SET_GPIO_OUT:
		lvds_set_gpio_output((struct lvds_set_gpio *)val);
		break;
	case LVDS_SET_PWM_BACKLIGHT:
		lvds_set_pwm_backlight_duty(val);
		break;
	case LVDS_SET_GPIO_BACKLIGHT:
		lvds_set_gpio_backlight(val);
		break;
	case LVDS_SET_PWM_VCOM:
		lvds_set_pwm_vcom_duty(val);
		break;
	case LVDS_SET_GPIO_POWER:
		lvds_set_gpio_power(val);
		break;
	case LVDS_SET_TRIGGER_EN:
		lvds_hal_set_trigger_en();
		break;
	case LVDS_GET_CHANNEL_MODE:
		lvds_get_channel_mode((lvds_channel_mode_e*)val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int lvds_close(struct file *filep)
{
	return 0;
}

static int lvds_open(struct file *filep)
{
	return 0;
}

static const struct file_operations lvds_fops = {
	.ioctl = lvds_ioctl,
	.open = lvds_open,
	.close = lvds_close,
	.read = dummy_read,
	.write = dummy_write,
};

static bool is_bootlogo_showing(void)
{
	u32 de, de4k;
	u32 deshowing = 0, de4kshowing = 0;
	if(lvds_misc.src_sel)
	{
		hc_clk_enable(DE4K_EXTO_CLK);
		hc_clk_enable(DE4K_CLK);
		hc_clk_enable(DE4K_DVI_CLK);
		hc_clk_enable(DE4K_HDMI_CLK);
	}
	else
	{
		hc_clk_enable(DE_CLK);
		hc_clk_enable(DVI_CLK);
		hc_clk_enable(DE_HDMI_CLK);
	}

	if(lvds_misc.src_sel==0)
	{
		de = 0xB8808000;
		deshowing = (REG32_READ(de + 0x190) & BIT(21)) && (REG32_READ(de) & BIT(0));
	}
	else
	{
		de4k = 0xB883a000;
		de4kshowing = (REG32_READ(de4k + 0x190) & BIT(21)) && (REG32_READ(de4k) & BIT(0));
	}
	return !!(deshowing || de4kshowing);
}

static void config_lvds(struct hichip_lvds *p, int np,uint8_t ch)
{
	T_LVDS_CFG lvds_cfg;

	memset(&lvds_cfg,0,sizeof(T_LVDS_CFG));
	fdt_get_property_u_32_index(np, "channel-mode", 0, (u32 *)&lvds_cfg.channel_mode);
	fdt_get_property_u_32_index(np, "map-mode", 0, (u32 *)&lvds_cfg.map_mode);
	fdt_get_property_u_32_index(np, "ch1-src-sel", 0, (u32 *)&lvds_cfg.ch1_src_sel);
	fdt_get_property_u_32_index(np, "ch1-invert-clk-sel", 0, (u32 *)&lvds_cfg.ch1_invert_clk_sel);
	fdt_get_property_u_32_index(np, "ch0-src-sel", 0, (u32 *)&lvds_cfg.ch0_src_sel);
	fdt_get_property_u_32_index(np, "ch0-invert-clk-sel", 0, (u32 *)&lvds_cfg.ch0_invert_clk_sel);
	fdt_get_property_u_32_index(np, "hsync-polarity", 0, (u32 *)&lvds_cfg.hsync_polarity);
	fdt_get_property_u_32_index(np, "vsync-polarity", 0, (u32 *)&lvds_cfg.vsync_polarity);
	fdt_get_property_u_32_index(np, "even-odd-adjust-mode", 0, (u32 *)&lvds_cfg.even_odd_adjust_mode);
	fdt_get_property_u_32_index(np, "even-odd-init-value", 0, (u32 *)&lvds_cfg.even_odd_init_value);
	fdt_get_property_u_32_index(np, "src-sel", 0, (u32 *)&lvds_cfg.src_sel);
	fdt_get_property_u_32_index(np, "ch0-clk-gate", 0, (u32 *)&lvds_cfg.ch0_clk_gate);
	fdt_get_property_u_32_index(np, "ch1-clk-gate", 0, (u32 *)&lvds_cfg.ch1_clk_gate);
	lvds_lld_set_cfg(&lvds_cfg,ch);
	lvds_lld_phy_mode_init(ch,lvds_cfg.channel_mode);
}

static void config_ttl(struct hichip_lvds *p, int np,const char *pin_type,uint8_t ch)
{
	LVDS_IO_TTL_SEL lvds_ttl=LVDS_IO_TTL_SEL_RGB888;
	E_VIDEO_SRC_SEL src_sel = E_SRC_SEL_FXDE;
	E_VIDEO_SRC_SEL src_sel2 = E_SRC_2_SEL_FXDE;
	u32 rgb_src_sel=0;
	int ret;
	u32 tmpVal = 0;
	const char *index_temp=pin_type;
	struct pinmux_setting *active_state;
	u32 i=0;

	if(index_temp != NULL && strcmp("rgb888", index_temp) == 0) {
		lvds_ttl= LVDS_IO_TTL_SEL_RGB888;
		active_state = fdt_get_property_pinmux(np, "rgb888");
		if (active_state) {
			pinmux_select_setting(active_state);
			free(active_state);
		}
	}
	else if(index_temp != NULL && strcmp("rgb666", index_temp) == 0) {
		lvds_ttl= LVDS_IO_TTL_SEL_RGB666;
		active_state = fdt_get_property_pinmux(np, "rgb666");
		if (active_state) {
			pinmux_select_setting(active_state);
			free(active_state);
		}
	}
	else if(index_temp != NULL && strcmp("rgb565", index_temp) == 0)
		lvds_ttl= LVDS_IO_TTL_SEL_RGB565;
	else if(index_temp != NULL && strcmp("i2so", index_temp) == 0)
		lvds_ttl= LVDS_IO_TTL_SEL_I2SO;
	else if(index_temp != NULL && strcmp("gpio", index_temp) == 0)
		lvds_ttl= LVDS_IO_TTL_SEL_LVDS_GPIO1;
	else lvds_ttl= LVDS_IO_TTL_SEL_LVDS_GPIO1;

	if(lvds_ttl<=LVDS_IO_TTL_SEL_RGB565)
	{
		ret = fdt_get_property_u_32_index(np, "rgb-clk-inv", 0, &tmpVal);
		if(ret == 0)
			lvds_hal_rgb_clk_inv_sel(tmpVal);

		hc_clk_enable(RGB_CLK);
		ret = fdt_get_property_u_32_index(np, "src-sel", 0, &tmpVal);
		if (ret == 0) {
			if (tmpVal == 0) {
				src_sel = E_SRC_SEL_FXDE;
				src_sel2 = E_SRC_2_SEL_FXDE;
			} else if (tmpVal == 1) {
				src_sel = E_SRC_SEL_4KDE;
				src_sel2 = E_SRC_2_SEL_4KDE;
			} else if (tmpVal == 2) {
				src_sel = E_SRC_SEL_HDMI_RX;
				src_sel2 = E_SRC_2_SEL_HDMI_RX;
			} else {
				src_sel = E_SRC_SEL_FXDE;
				src_sel2 = E_SRC_2_SEL_FXDE;
			}
		}

		p->rgb_backlight.padctl = PINPAD_INVALID;
		p->rgb_backlight.active = GPIO_ACTIVE_HIGH;
		ret = fdt_get_property_u_32_index(np, "rgb-backlight-gpios-rtos", 0, &tmpVal);
		if (ret == 0)
			p->rgb_backlight.padctl = (pinpad_e)tmpVal;

		ret = fdt_get_property_u_32_index(np, "rgb-backlight-gpios-rtos", 1, &tmpVal);
		if (ret == 0)
			p->rgb_backlight.active = (pinpad_e)tmpVal;

		gpio_configure(p->rgb_backlight.padctl, GPIO_DIR_OUTPUT);
		gpio_set_output(p->rgb_backlight.padctl, 1);

		fdt_get_property_string_index(np, "rgb-src-sel", 0, &index_temp);
		if(index_temp!=NULL&&strlen(index_temp)==3)
		{
			tmpVal=0;
			for(i=0;i<3;i++)
			{
				if(index_temp[i]=='r'){
					switch (i)
					{
						case 0: tmpVal|=0;break;
						case 1: tmpVal|=2;break;
						case 2: tmpVal|=1;break;
						default:break;
					}
				}
				else if(index_temp[i]=='g'){
					switch (i)
					{
						case 0: tmpVal|=1;break;
						case 1: tmpVal|=0;break;
						case 2: tmpVal|=2;break;
						default:break;
					}
				}
				else if(index_temp[i]=='b'){
					switch (i)
					{
						case 0: tmpVal|=2;break;
						case 1: tmpVal|=1;break;
						case 2: tmpVal|=0;break;
						default:break;
					}
				}
				tmpVal<<=4;
			}
			rgb_src_sel=tmpVal;
		}
		else 
			rgb_src_sel=0;
	}

	rgb_hal_set_src_video_sel(src_sel, src_sel2);
	rgb_hal_set_src_sel2((rgb_hal_set_src_e)(rgb_src_sel>>12),(rgb_hal_set_src_e)(rgb_src_sel>>4),(rgb_hal_set_src_e)(rgb_src_sel>>8));
	lvds_io_ttl_sel_set(lvds_ttl);
	lvds_lld_phy_ttl_mode_init(ch);

}

static void config_lvds_ttl(struct hichip_lvds *p, int np)
{
	const char *screen_type = NULL;
	char st_buf[20]={0};
	u8 i;
	for(i=0;i<2;i++)
	{
		sprintf(st_buf,"lvds_ch%d-type",i);
		fdt_get_property_string_index(np, st_buf, 0, &screen_type);
		if(screen_type != NULL && strcmp("lvds", screen_type) == 0)
			config_lvds(p, np, i);
		else
			config_ttl(p, np,screen_type,i);
	}
}

static int get_first_standby_status(void)//Get whether the bootloader has entered standby mode 0 no 1 yes
{
	int ret = 1;
#ifdef CONFIG_BOOT_STANDBY
	int fd_standby;
	standby_bootup_mode_e temp = 0;
	fd_standby = open("/dev/standby", O_RDWR);
	if(fd_standby<0){
		log_e("Open /dev/standby failed!\n");
		return ret;
	}
	ioctl(fd_standby, STANDBY_GET_BOOTUP_MODE, &temp);
	log_d("STANDBY_GET_BOOTUP_MODE temp =%d\n",temp);
	if(temp ==STANDBY_BOOTUP_COLD_BOOT)
		ret = 0;
	else
		ret =1;
	close(fd_standby);
#endif
	return ret;
}

static int lvds_probe(const char *node)
{
	int ret;
	u32 tmpVal = 0;
	u32 i;
	const char *path=NULL;
	int np = fdt_node_probe_by_path(node);

	if(np < 0){
		return 0;
	}

	memset(&lvds_misc, 0, sizeof(struct hichip_lvds));

	fdt_get_property_u_32_index(np, "reg", 0, (u32 *)&lvds_misc.reg_base);
	fdt_get_property_u_32_index(np, "reg", 2, (u32 *)&lvds_misc.sys_base);

	lvds_misc.reg_base = (void *)((u32)lvds_misc.reg_base | 0xa0000000);
	lvds_misc.sys_base = (void *)((u32)lvds_misc.sys_base | 0xa0000000);

	lvds_lld_init(lvds_misc.reg_base, lvds_misc.sys_base, false);

	if (fdt_get_property_data_by_name(np, "lcd-power-gpios-rtos", &lvds_misc.lcd_power_num) == NULL)
		lvds_misc.lcd_power_num = 0;

	lvds_misc.lcd_power_num >>= 3;

	if(lvds_misc.lcd_power_num > LCD_POWER_NUM_MAX)
		lvds_misc.lcd_power_num = LCD_POWER_NUM_MAX;

	for(i=0;i<lvds_misc.lcd_power_num;i++){
		lvds_misc.lcd_power[i].padctl = PINPAD_INVALID;
		lvds_misc.lcd_power[i].active = GPIO_ACTIVE_HIGH;

		if(fdt_get_property_u_32_index(np, "lcd-power-gpios-rtos", i * 2, &tmpVal)==0)
			lvds_misc.lcd_power[i].padctl = tmpVal;
		if(fdt_get_property_u_32_index(np, "lcd-power-gpios-rtos", i * 2 + 1, &tmpVal)==0)
			lvds_misc.lcd_power[i].active = tmpVal;
	}

	if (fdt_get_property_data_by_name(np, "lcd-backlight-gpios-rtos", &lvds_misc.lcd_backlight_num) == NULL)
		lvds_misc.lcd_backlight_num = 0;

	lvds_misc.lcd_backlight_num >>= 3;

	if(lvds_misc.lcd_backlight_num > LCD_BACKLIGHT_NUM_MAX)
		lvds_misc.lcd_backlight_num = LCD_BACKLIGHT_NUM_MAX;

	for(i=0;i<lvds_misc.lcd_backlight_num;i++){
		lvds_misc.lcd_backlight[i].padctl = PINPAD_INVALID;
		lvds_misc.lcd_backlight[i].active = GPIO_ACTIVE_HIGH;

		if(fdt_get_property_u_32_index(np, "lcd-backlight-gpios-rtos", i * 2, &tmpVal)==0)
			lvds_misc.lcd_backlight[i].padctl = tmpVal;
		if(fdt_get_property_u_32_index(np, "lcd-backlight-gpios-rtos", i * 2 + 1, &tmpVal)==0)
			lvds_misc.lcd_backlight[i].active = tmpVal;
	}

	if (fdt_get_property_string_index(np, "vcom-pwmdev", 0, &path)==0)
	{
		ret = fdt_get_property_u_32_index(np, "vcom-frequency", 0, &tmpVal);
		if(ret == 0)
			lvds_misc.pwm_vcom.period_ns = tmpVal;

		ret = fdt_get_property_u_32_index(np, "vcom-duty", 0, &tmpVal);
		if (ret == 0)
			lvds_misc.pwm_vcom.duty_ns = tmpVal;

		lvds_misc.vcom_fd = open(path, O_RDWR);
		if (lvds_misc.vcom_fd >= 0) {
			lvds_misc.pwm_vcom.polarity = 0;
			ioctl(lvds_misc.vcom_fd, PWMIOC_START, 0);
			ioctl(lvds_misc.vcom_fd, PWMIOC_SETCHARACTERISTICS, &lvds_misc.pwm_vcom);
		}
		else
			lvds_misc.vcom_fd = -1;
	}
	else
		lvds_misc.vcom_fd = -1;

	if (fdt_get_property_string_index(np, "backlight-pwmdev", 0, &path)==0)
	{
		ret = fdt_get_property_u_32_index(np, "backlight-frequency", 0, &tmpVal);
		if(ret == 0)
			lvds_misc.pwm_backlight.period_ns = tmpVal;

		ret = fdt_get_property_u_32_index(np, "backlight-duty", 0, &tmpVal);
		if (ret == 0)
			lvds_misc.pwm_backlight.duty_ns = tmpVal;

		lvds_misc.backlight_fd = open(path, O_RDWR);
	}
	else
		lvds_misc.backlight_fd = -1;

	fdt_get_property_u_32_index(np, "src-sel", 0, (u32 *)&lvds_misc.src_sel);

	// lvds_spi_display_init(NULL);

	/*Turn off the backlight and dev power*/
	if(get_first_standby_status() == 0)
	{
		for(i=0;i<lvds_misc.lcd_backlight_num;i++)
		{
			gpio_set_output(lvds_misc.lcd_backlight[i].padctl, lvds_misc.lcd_backlight[i].active);
			gpio_configure(lvds_misc.lcd_backlight[i].padctl, GPIO_DIR_OUTPUT);
		}
		for(i=0;i<lvds_misc.lcd_power_num;i++)
		{
			gpio_set_output(lvds_misc.lcd_power[i].padctl, lvds_misc.lcd_power[i].active);
			gpio_configure(lvds_misc.lcd_power[i].padctl, GPIO_DIR_OUTPUT);
		}
		goto lvds_register;
	}

	if (!is_bootlogo_showing())
	{
		/*reg init*/
		hc_clk_enable(LVDS_CH1_PIXEL_CLK);
		hc_clk_enable(LVDS_CLK);
		lvds_hal_reset();
		config_lvds_ttl(&lvds_misc, np);

		/*Turn off the backlight and turn it on after the logo is displayed*/
		for(i=0;i<lvds_misc.lcd_backlight_num;i++)
		{
			gpio_set_output(lvds_misc.lcd_backlight[i].padctl, lvds_misc.lcd_backlight[i].active);
			gpio_configure(lvds_misc.lcd_backlight[i].padctl, GPIO_DIR_OUTPUT);
		}

		for(i=0;i<lvds_misc.lcd_power_num;i++)
		{
			gpio_set_output(lvds_misc.lcd_power[i].padctl, !lvds_misc.lcd_power[i].active);
			gpio_configure(lvds_misc.lcd_power[i].padctl, GPIO_DIR_OUTPUT);
		}

		if(lvds_misc.lcd_power_num>0)
			usleep(100*1000);

		/*spi lcd init*/
		// lvds_display_init();
	}

	/*LVDS outputs high level in GPIO mode*/
	/*
	struct lvds_set_gpio pad={0,1};
	for(int i=0;i<32;i++)
	{
		lvds_set_gpio_output(&pad);
		pad.padctl++;
	}
	*/
lvds_register:
	return register_driver(MODULE_NAME, &lvds_fops, 0666, NULL);
}

static int lvds_init(void)
{
	lvds_probe("/hcrtos/lvds@0xb8860000");
	lvds_probe("/soc/lvds@0xb8860000");

	return 0;
}

static int lvds_exit(void)
{
	unregister_driver(MODULE_NAME);
}

module_driver(lvds, lvds_init, lvds_exit, 1)
