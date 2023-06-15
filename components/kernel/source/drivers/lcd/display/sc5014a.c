#define LOG_TAG "lcd_sc5014a"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/module.h>
#include <hcuapi/gpio.h>
#include <hcuapi/pinpad.h>
#include <hcuapi/pinmux.h>
#include <hcuapi/lvds.h>
#include <kernel/io.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../lcd_main.h"
/*
*	TIME: 2022 11 12
*	support: TPR AK02 BOARD
*	PINPAD_LVDS_DN6 clk
*	PINPAD_LVDS_DP6 cs
*	PINPAD_LVDS_DP5 sda
*	PINPAD_LVDS_DN5 STBYB
*	PINPAD_T04 backlight
*	PINPAD_T03 power
*	PINPAD_T02 fan

	lcd-sc5014a{
		spi-gpio-sck	= <PINPAD_LVDS_DN6>;
		spi-gpio-mosi	= <PINPAD_LVDS_DP5>;
		spi-gpio-cs		= <PINPAD_LVDS_DP6>;
		spi-gpio-stbyb	= <PINPAD_LVDS_DN5>;
		default-off;
		status = "okay";
	};
	lcd{
		lcd-map-name = "lcd-sc5014a";
		default-off;
		status = "okay";
	};
*/


#define SET_DWORD(i, d)         (*(volatile unsigned long *)(i)) = (d)
#define GET_DWORD(i)            (*(volatile unsigned long *)(i))
#define LVDS_GPIO_SUPPORT 1

static void sc5014a_write_command(unsigned short cmds);
static void sc5014a_write_data(unsigned char data);
static int sc5014a_display_init(void);
static void lcd_stbyb_start(void);
static void lcd_stbyb_normal(void);

typedef struct sc5014a_dev{
	u32 spi_clk_num;
	u32 spi_clk_vaild_edge;
	u32 spi_cs_num;
	u32 spi_cs_polar;  
	u32 spi_mosi_num;
	u32 spi_miso_num;
	u32 spi_is_9bit;
	u32 spi_mode;
	u32 lcd_reset_num;
	u32 lcd_reset_polar;
	u32 lcd_stbyb_num;
	u32 lcd_stbyb_polar;
	u32 cur_type;
	u32 power_on;
}sc5014a_dev_t;

static sc5014a_dev_t sc5014adev;

static void sc5014a_send_cmds_date(unsigned short cmd, unsigned char date)
{
	sc5014a_write_command(cmd);
	sc5014a_write_data(date);
}
static int sc5014a_gpio_power_on(u32 val)
{
	if(val == 0)
	{
		gpio_configure(sc5014adev.power_on,GPIO_DIR_OUTPUT);//miso
		lcd_gpio_set_output(sc5014adev.power_on,0);
	}
	else
	{
		gpio_configure(sc5014adev.power_on,GPIO_DIR_OUTPUT);//miso
		lcd_gpio_set_output(sc5014adev.power_on,1);
	}
}

static int sc5014a_display_init(void)
{
	gpio_configure(sc5014adev.spi_clk_num,GPIO_DIR_OUTPUT);//clk
	gpio_configure(sc5014adev.spi_mosi_num,GPIO_DIR_OUTPUT);//mosi
	gpio_configure(sc5014adev.spi_cs_num,GPIO_DIR_OUTPUT);//cs
	gpio_configure(sc5014adev.lcd_stbyb_num,GPIO_DIR_OUTPUT);//STBYB
	gpio_configure(sc5014adev.spi_miso_num,GPIO_DIR_OUTPUT);//miso
	sc5014a_gpio_power_on(1);
	lcd_stbyb_start();
	printf("%s %d\n",__FUNCTION__,__LINE__);

	sc5014a_send_cmds_date(0x00, 0x68);//P6
	sc5014a_send_cmds_date(0x0E, 0xAA);//Reload off
	sc5014a_send_cmds_date(0x00, 0x68);//P6, enter eng
	sc5014a_send_cmds_date(0x0A, 0x94);//Enter eng.
	sc5014a_send_cmds_date(0x0B, 0xA2);//Enter eng.
	sc5014a_send_cmds_date(0x00, 0x98);//P9
	sc5014a_send_cmds_date(0x02, 0x1A);//BIST CK=50MHz
	sc5014a_send_cmds_date(0x00, 0x68);//P6
	sc5014a_send_cmds_date(0x0A, 0x00);//Escape eng.
	sc5014a_send_cmds_date(0x0B, 0x00);//Escape eng.
	sc5014a_send_cmds_date(0x00, 0x08);//P0
	sc5014a_send_cmds_date(0x01, 0xB2);//LVDS
	sc5014a_send_cmds_date(0x03, 0xD4);//1280x720
	sc5014a_send_cmds_date(0x05, 0x0A);//PN inversion
	sc5014a_send_cmds_date(0x00, 0x18);//P1 GOA Timing
	sc5014a_send_cmds_date(0x01, 0x4E);//GOA, do not modify
	sc5014a_send_cmds_date(0x03, 0x30);//GOA, do not modify
	sc5014a_send_cmds_date(0x04, 0x12);//GOA, do not modify
	sc5014a_send_cmds_date(0x05, 0x00);//GOA, do not modify
	sc5014a_send_cmds_date(0x06, 0x02);//GOA, do not modify
	sc5014a_send_cmds_date(0x07, 0x00);//GOA, do not modify
	sc5014a_send_cmds_date(0x08, 0xB0);//GOA, do not modify
	sc5014a_send_cmds_date(0x09, 0x00);//GOA, do not modify
	sc5014a_send_cmds_date(0x0A, 0xDF);//GOA, do not modify
	sc5014a_send_cmds_date(0x0D, 0x37);//GOA, do not modify
	sc5014a_send_cmds_date(0x0E, 0xA0);//GOA, do not modify
	sc5014a_send_cmds_date(0x0F, 0x00);//GOA, do not modify
	sc5014a_send_cmds_date(0x10, 0xF8);//GOA, do not modify
	sc5014a_send_cmds_date(0x11, 0x00);//GOA, do not modify
	sc5014a_send_cmds_date(0x12, 0x31);//GOA, do not modify
	sc5014a_send_cmds_date(0x13, 0x03);//GOA, do not modify
	sc5014a_send_cmds_date(0x14, 0x10);//GOA, do not modify
	sc5014a_send_cmds_date(0x15, 0x20);//GOA, do not modify
	sc5014a_send_cmds_date(0x19, 0x14);//GOA, do not modify
	sc5014a_send_cmds_date(0x1A, 0x0A);//GOA, do not modify
	sc5014a_send_cmds_date(0x1B, 0x48);//GOA, do not modify
	sc5014a_send_cmds_date(0x1C, 0x04);//GOA, do not modify
	sc5014a_send_cmds_date(0x1D, 0x0C);//GOA, do not modify
	sc5014a_send_cmds_date(0x1E, 0xC0);//GOA, do not modify
	sc5014a_send_cmds_date(0x1F, 0x55);//GOA, do not modify
	sc5014a_send_cmds_date(0x20, 0x73);//GOA, do not modify
	sc5014a_send_cmds_date(0x21, 0xF0);//GOA, do not modify
	sc5014a_send_cmds_date(0x23, 0xF4);//GOA, do not modify
	sc5014a_send_cmds_date(0x24, 0x77);//GOA, do not modify
	sc5014a_send_cmds_date(0x31, 0x21);//GOA, do not modify
	sc5014a_send_cmds_date(0x40, 0x80);//GOA, do not modify
	sc5014a_send_cmds_date(0x45, 0x00);//GOA, do not modify
	sc5014a_send_cmds_date(0x4F, 0x12);//GOA, do not modify
	sc5014a_send_cmds_date(0x52, 0x02);//GOA, do not modify
	sc5014a_send_cmds_date(0x53, 0x02);//GOA, do not modify
	sc5014a_send_cmds_date(0x54, 0x02);//GOA, do not modify
	sc5014a_send_cmds_date(0x57, 0x94);//GOA, do not modify
	sc5014a_send_cmds_date(0x58, 0x36);//GOA, do not modify
	sc5014a_send_cmds_date(0x59, 0x25);//GOA, do not modify
	sc5014a_send_cmds_date(0x5A, 0x25);//GOA, do not modify
	sc5014a_send_cmds_date(0x5B, 0x14);//GOA, do not modify
	sc5014a_send_cmds_date(0x5C, 0x36);//GOA, do not modify
	sc5014a_send_cmds_date(0x68, 0xC0);//GOA, do not modify
	sc5014a_send_cmds_date(0x00, 0x58);//P5 Power
	sc5014a_send_cmds_date(0x01, 0x04);//VSP=5.4V
	sc5014a_send_cmds_date(0x02, 0x04);//VSN=-5.4V
	sc5014a_send_cmds_date(0x03, 0x50);//VGMPH=5.0V, VGMPH=-5.0V
	sc5014a_send_cmds_date(0x04, 0xBB);//VGMPH=5.0V, VGMPH=-5.0V
	sc5014a_send_cmds_date(0x06, 0x08);//VGH=8V(0x02),VGH=11V(0x08),
	sc5014a_send_cmds_date(0x07, 0x08);//VGL=-8V(0x02),VGL=-11V(0x08),
	sc5014a_send_cmds_date(0x0C, 0x44);//VGH(x2), VGL(x2)
	sc5014a_send_cmds_date(0x25, 0x66);//VSP_en, VSN_en
	sc5014a_send_cmds_date(0x00, 0x78);//P7 GOA output
	sc5014a_send_cmds_date(0x01, 0x49);//assign left GOA signal
	sc5014a_send_cmds_date(0x02, 0x45);//assign left GOA signal
	sc5014a_send_cmds_date(0x03, 0x4D);//assign left GOA signal
	sc5014a_send_cmds_date(0x04, 0x4B);//assign left GOA signal
	sc5014a_send_cmds_date(0x05, 0xC0);//assign left GOA signal
	sc5014a_send_cmds_date(0x06, 0x57);//assign left GOA signal
	sc5014a_send_cmds_date(0x07, 0x58);//assign left GOA signal
	sc5014a_send_cmds_date(0x08, 0x59);//assign left GOA signal
	sc5014a_send_cmds_date(0x09, 0x5A);//assign left GOA signal
	sc5014a_send_cmds_date(0x0A, 0x5B);//assign left GOA signal
	sc5014a_send_cmds_date(0x0B, 0x5C);//assign left GOA signal
	sc5014a_send_cmds_date(0x11, 0x41);//assign left GOA signal
	sc5014a_send_cmds_date(0x12, 0x57);//assign left GOA signal
	sc5014a_send_cmds_date(0x13, 0x5D);//assign left GOA signal
	sc5014a_send_cmds_date(0x14, 0x58);//assign left GOA signal
	sc5014a_send_cmds_date(0x15, 0x49);//assign right GOA signal
	sc5014a_send_cmds_date(0x16, 0x45);//assign right GOA signal
	sc5014a_send_cmds_date(0x17, 0x4E);//assign right GOA signal
	sc5014a_send_cmds_date(0x18, 0x4C);//assign right GOA signal
	sc5014a_send_cmds_date(0x19, 0xC0);//assign right GOA signal
	sc5014a_send_cmds_date(0x1A, 0x57);//assign right GOA signal
	sc5014a_send_cmds_date(0x1B, 0x58);//assign right GOA signal
	sc5014a_send_cmds_date(0x1C, 0x59);//assign right GOA signal
	sc5014a_send_cmds_date(0x1D, 0x5A);//assign right GOA signal
	sc5014a_send_cmds_date(0x1E, 0x5B);//assign right GOA signal
	sc5014a_send_cmds_date(0x1F, 0x5C);//assign right GOA signal
	sc5014a_send_cmds_date(0x25, 0x42);//assign right GOA signal
	sc5014a_send_cmds_date(0x26, 0x57);//assign right GOA signal
	sc5014a_send_cmds_date(0x27, 0x5D);//assign right GOA signal
	sc5014a_send_cmds_date(0x28, 0x58);//assign right GOA signal
	sc5014a_send_cmds_date(0x00, 0x88);//P8
	sc5014a_send_cmds_date(0x05, 0x01);//DCLK>20MHz, TR=100Ohm
	lcd_stbyb_normal();
	return 0;
}

static void gpio_spi_set_mosi(unsigned char data)
{
    lcd_gpio_set_output(sc5014adev.spi_mosi_num,(bool)data);
}

static void gpio_spi_generate_clk(void)
{   
    if(sc5014adev.spi_clk_vaild_edge == 1)//1 
		lcd_gpio_set_output(sc5014adev.spi_clk_num,1);
	else
		lcd_gpio_set_output(sc5014adev.spi_clk_num,0);
	usleep(2);
	if(sc5014adev.spi_clk_vaild_edge == 1)//1 
		lcd_gpio_set_output(sc5014adev.spi_clk_num,0);
	else
		lcd_gpio_set_output(sc5014adev.spi_clk_num,1);
}
static void gpio_spi_enable_cs(void)//==0
{
	if(sc5014adev.spi_cs_polar == 0)	
		lcd_gpio_set_output(sc5014adev.spi_cs_num,0);
	else
		lcd_gpio_set_output(sc5014adev.spi_cs_num,1);
}

static void gpio_spi_disable_cs(void)
{
	if(sc5014adev.spi_cs_polar == 0)
		lcd_gpio_set_output(sc5014adev.spi_cs_num,1);//cs
	else
		lcd_gpio_set_output(sc5014adev.spi_cs_num,0);//cs
}

static void gpio_spi_init_clk(void)
{
	if(sc5014adev.spi_clk_vaild_edge == 1)//1 //==1
		lcd_gpio_set_output(sc5014adev.spi_clk_num,0);//sck
	else
		lcd_gpio_set_output(sc5014adev.spi_clk_num,1);//sck
		
}

static void sc5014a_write_data(unsigned char data)
{
	int i=0;
	unsigned char data_val = 0;
	for(i=8;i>0;i--){
		data_val = (data>>(i-1))&0x1;
		gpio_spi_set_mosi(data_val);
		usleep(2);
		gpio_spi_generate_clk();
	}
	usleep(2);
	gpio_spi_disable_cs();
	gpio_spi_set_mosi(0);
	usleep(10);
}

static void sc5014a_write_command(unsigned short cmds)
{
	int i=0;
	unsigned char cmd_val = 0;
	gpio_spi_disable_cs();//cs
	usleep(10);
	gpio_spi_init_clk();//sck
	gpio_spi_enable_cs();
	usleep(2);
	for(i=8;i>0;i--){
		cmd_val = (cmds>>(i-1))&0x1;
		gpio_spi_set_mosi(cmd_val);
		usleep(2);
		gpio_spi_generate_clk();
	}
}

static void lcd_stbyb_start(void)
{
	lcd_gpio_set_output(sc5014adev.lcd_stbyb_num,!sc5014adev.lcd_stbyb_polar);
	usleep(200*500);
	lcd_gpio_set_output(sc5014adev.lcd_stbyb_num,sc5014adev.lcd_stbyb_polar);
	usleep(200*500);
}

static void lcd_stbyb_normal(void)
{
	usleep(10*1000);
	lcd_gpio_set_output(sc5014adev.lcd_stbyb_num,!sc5014adev.lcd_stbyb_polar);
}

static struct lcd_map_list sc5014a_map = {
	.map = {
		.lcd_init = sc5014a_display_init,
		.lcd_power_onoff = sc5014a_gpio_power_on,
		.name = "lcd-sc5014a",
	}
};

static int sc5014a_probe(const char *node)
{
	int np = fdt_node_probe_by_path(node);

	if(np < 0){
		goto error;
	}

	memset(&sc5014adev,0,sizeof(struct sc5014a_dev));

	sc5014adev.spi_clk_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.sck;
	sc5014adev.spi_clk_vaild_edge=1;
	sc5014adev.spi_cs_polar=0;
	sc5014adev.spi_is_9bit=1;
	sc5014adev.spi_cs_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.cs;
	sc5014adev.spi_mosi_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.mosi;
	sc5014adev.spi_miso_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.miso;
	sc5014adev.lcd_stbyb_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.stbyb;
	sc5014adev.lcd_stbyb_polar = 0;
	sc5014adev.lcd_reset_num = PINPAD_INVALID;

	fdt_get_property_u_32_index(np, "reset", 			0, &sc5014adev.lcd_reset_num);
	fdt_get_property_u_32_index(np, "spi-gpio-sck", 	0, &sc5014adev.spi_clk_num);
	fdt_get_property_u_32_index(np, "spi-gpio-mosi", 	0, &sc5014adev.spi_mosi_num);
	fdt_get_property_u_32_index(np, "spi-gpio-miso", 	0, &sc5014adev.spi_miso_num);
	fdt_get_property_u_32_index(np, "spi-gpio-cs", 		0, &sc5014adev.spi_cs_num);
	fdt_get_property_u_32_index(np, "spi-gpio-stbyb", 	0, &sc5014adev.lcd_stbyb_num);
	fdt_get_property_u_32_index(np, "power-gpio-rtos", 	0, &sc5014adev.power_on);

	#if 0
	sc5014adev.spi_clk_num=PINPAD_LVDS_DN6;
	sc5014adev.spi_clk_vaild_edge=1;
	sc5014adev.spi_cs_polar=0;
	sc5014adev.spi_is_9bit=1;
	sc5014adev.spi_cs_num=PINPAD_LVDS_DP6;
	sc5014adev.spi_mosi_num=PINPAD_LVDS_DP5;
	sc5014adev.lcd_stbyb_num=PINPAD_LVDS_DN5;
	sc5014adev.lcd_stbyb_polar=0;
	#endif
	printf("sc5014adev.lcd_stbyb_num = %d %d %d %d\n",sc5014adev.spi_clk_num,sc5014adev.spi_mosi_num,sc5014adev.spi_cs_num,sc5014adev.lcd_stbyb_num);
	int default_off = 0;
	fdt_get_property_u_32_index(np, "default-off", 	0, &default_off);
	if(default_off ==0)
		sc5014a_display_init();

	sc5014a_map.map.default_off_val = default_off;
	lcd_map_register(&sc5014a_map);
error:
	return 0;
}

static int sc5014a_init(void)
{
	sc5014a_probe("/hcrtos/lcd-sc5014a");
	return 0;
}

module_driver(sc5014a, sc5014a_init, NULL, 2)
