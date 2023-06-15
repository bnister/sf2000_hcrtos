#define LOG_TAG "lcd_jdi026"
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
#include <kernel/io.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../lcd_main.h"

/*
*	TIME: 2022 11 12
*	support: TPR WS200 BOARD
*	PINPAD_T01 clk
*	PINPAD_T02 mosi
*	PINPAD_T00 cs
*	PINPAD_T03 STBYB
*	PINPAD_T04 backlight

	lcd-jdi026{
		spi-gpio-sck	= <PINPAD_T01>;
		spi-gpio-mosi	= <PINPAD_T02>;
		spi-gpio-cs		= <PINPAD_T00>;
		spi-gpio-stbyb	= <PINPAD_T03>;
		default-off;
		status = "okay";
	};
	lcd{
		lcd-map-name = "lcd-jdi026";
		default-off;
		status = "okay";
	};
*/
typedef struct jdi026_dev{
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
}jdi026_dev_t;

#define SET_DWORD(i, d)         (*(volatile unsigned long *)(i)) = (d)
#define GET_DWORD(i)            (*(volatile unsigned long *)(i))
#define LVDS_GPIO_SUPPORT 0

static void jdi026_write_command(unsigned short cmds);
static void jdi026_write_data(unsigned char data);
static void lcd_stbyb_start(void);
static void lcd_stbyb_normal(void);

static jdi026_dev_t jdi026dev;

void jdi026_send_cmds_date(unsigned short cmd, unsigned char date)
{
	jdi026_write_command(cmd);
	jdi026_write_data(date);
}
static int jdi026_display_init(void)
{
	// gpio_configure(PINPAD_T04,GPIO_DIR_OUTPUT);//backlight
	// lcd_gpio_set_output(PINPAD_T04,1);
	gpio_configure(jdi026dev.spi_clk_num,GPIO_DIR_OUTPUT);//clk
	gpio_configure(jdi026dev.spi_cs_num,GPIO_DIR_OUTPUT);//cs
	gpio_configure(jdi026dev.spi_mosi_num,GPIO_DIR_OUTPUT);//mosi
	gpio_configure(jdi026dev.lcd_stbyb_num,GPIO_DIR_OUTPUT);//STBYB
	gpio_configure(jdi026dev.spi_miso_num,GPIO_DIR_OUTPUT);//miso

	lcd_stbyb_start();
	printf("%s %d\n",__FUNCTION__,__LINE__);
	jdi026_send_cmds_date(0x00, 0x68);//P6
	jdi026_send_cmds_date(0x0E, 0xAA);//Reload off
	jdi026_send_cmds_date(0x00, 0x08);//P0
	jdi026_send_cmds_date(0x01, 0x82);//LVDS B2��82(Dot inversion)
	jdi026_send_cmds_date(0x03, 0xD4);//1280x720
	jdi026_send_cmds_date(0x05, 0x0A);//PN inversion
	jdi026_send_cmds_date(0x00, 0x18);//P1 GOA Timing
	jdi026_send_cmds_date(0x01, 0x4E);//GOA, do not modify
	jdi026_send_cmds_date(0x03, 0x10);//GOA, do not modify
	jdi026_send_cmds_date(0x04, 0x02);//GOA, do not modify
	jdi026_send_cmds_date(0x05, 0x70);//GOA, do not modify
	jdi026_send_cmds_date(0x06, 0x00);//GOA, do not modify
	jdi026_send_cmds_date(0x07, 0x00);//GOA, do not modify
	jdi026_send_cmds_date(0x08, 0xB0);//GOA, do not modify
	jdi026_send_cmds_date(0x09, 0x00);//GOA, do not modify
	jdi026_send_cmds_date(0x0A, 0x00);//GOA, do not modify
	jdi026_send_cmds_date(0x0D, 0x12);//GOA, do not modify
	jdi026_send_cmds_date(0x0E, 0x21);//GOA, do not modify
	jdi026_send_cmds_date(0x0F, 0x8E);//GOA, do not modify
	jdi026_send_cmds_date(0x10, 0x00);//GOA, do not modify
	jdi026_send_cmds_date(0x11, 0x0F);//GOA, do not modify
	jdi026_send_cmds_date(0x12, 0x11);//GOA, do not modify
	jdi026_send_cmds_date(0x13, 0x02);//GOA, do not modify
	jdi026_send_cmds_date(0x14, 0x87);//GOA, do not modify
	jdi026_send_cmds_date(0x15, 0x00);//GOA, do not modify
	jdi026_send_cmds_date(0x19, 0x0E);//GOA, do not modify
	jdi026_send_cmds_date(0x1A, 0x14);//GOA, do not modify
	jdi026_send_cmds_date(0x1C, 0x14);//GOA, do not modify
	jdi026_send_cmds_date(0x1D, 0x0F);//GOA, do not modify
	jdi026_send_cmds_date(0x1F, 0xF5);//GOA, do not modify
	jdi026_send_cmds_date(0x23, 0x30);//GOA, do not modify
	jdi026_send_cmds_date(0x31, 0x11);//GOA, do not modify
	jdi026_send_cmds_date(0x40, 0x80);//GOA, do not modify
	jdi026_send_cmds_date(0x45, 0x00);//GOA, do not modify
	jdi026_send_cmds_date(0x4F, 0x3B);//GOA, do not modify
	jdi026_send_cmds_date(0x52, 0x03);//GOA, do not modify
	jdi026_send_cmds_date(0x53, 0x03);//GOA, do not modify
	jdi026_send_cmds_date(0x54, 0x04);//GOA, do not modify
	jdi026_send_cmds_date(0x57, 0x93);//GOA, do not modify
	jdi026_send_cmds_date(0x58, 0x24);//GOA, do not modify
	jdi026_send_cmds_date(0x59, 0x65);//GOA, do not modify
	jdi026_send_cmds_date(0x5A, 0x21);//GOA, do not modify
	jdi026_send_cmds_date(0x5B, 0x35);//GOA, do not modify
	jdi026_send_cmds_date(0x5C, 0x46);//GOA, do not modify
	jdi026_send_cmds_date(0x68, 0xC0);//GOA, do not modify
	jdi026_send_cmds_date(0x00, 0x58);//P5 Power
	jdi026_send_cmds_date(0x01, 0x04);//VSP=5.4V
	jdi026_send_cmds_date(0x02, 0x04);//VSN=-5.4V
	jdi026_send_cmds_date(0x03, 0x50);//VGMPH=5.0V, VGMNH=-5.0V
	jdi026_send_cmds_date(0x04, 0xBB);//VGMPH=5.0V, VGMNH=-5.0V
	jdi026_send_cmds_date(0x06, 0x00);//VGH=7V
	jdi026_send_cmds_date(0x07, 0x00);//VGL=-7V
	jdi026_send_cmds_date(0x0C, 0x44);//VGH(x2), VGL(x2)
	jdi026_send_cmds_date(0x25, 0x66);//VSP_EN, VSN_EN
	jdi026_send_cmds_date(0x00, 0x78);//P7 GOA output
	jdi026_send_cmds_date(0x01, 0x57);//assign left GOA signal
	jdi026_send_cmds_date(0x02, 0x58);//assign left GOA signal
	jdi026_send_cmds_date(0x03, 0x59);//assign left GOA signal
	jdi026_send_cmds_date(0x04, 0x5A);//assign left GOA signal
	jdi026_send_cmds_date(0x05, 0x5B);//assign left GOA signal
	jdi026_send_cmds_date(0x06, 0x5C);//assign left GOA signal
	jdi026_send_cmds_date(0x07, 0xDC);//assign left GOA signal
	jdi026_send_cmds_date(0x08, 0xDB);//assign left GOA signal
	jdi026_send_cmds_date(0x09, 0xDA);//assign left GOA signal
	jdi026_send_cmds_date(0x0A, 0xD9);//assign left GOA signal
	jdi026_send_cmds_date(0x0B, 0xD8);//assign left GOA signal
	jdi026_send_cmds_date(0x0C, 0xD7);//assign left GOA signal
	jdi026_send_cmds_date(0x0D, 0xC9);//assign left GOA signal
	jdi026_send_cmds_date(0x0E, 0x59);//assign left GOA signal
	jdi026_send_cmds_date(0x0F, 0x4B);//assign left GOA signal
	jdi026_send_cmds_date(0x10, 0x4C);//assign left GOA signal
	jdi026_send_cmds_date(0x11, 0x4F);//assign left GOA signal
	jdi026_send_cmds_date(0x13, 0x57);//assign left GOA signal
	jdi026_send_cmds_date(0x14, 0x41);//assign left GOA signal
	jdi026_send_cmds_date(0x15, 0x57);//assign right GOA signal
	jdi026_send_cmds_date(0x16, 0x58);//assign right GOA signal
	jdi026_send_cmds_date(0x17, 0x59);//assign right GOA signal
	jdi026_send_cmds_date(0x18, 0x5A);//assign right GOA signal
	jdi026_send_cmds_date(0x19, 0x5B);//assign right GOA signal
	jdi026_send_cmds_date(0x1A, 0x5C);//assign right GOA signal
	jdi026_send_cmds_date(0x1B, 0xDC);//assign right GOA signal
	jdi026_send_cmds_date(0x1C, 0xDB);//assign right GOA signal
	jdi026_send_cmds_date(0x1D, 0xDA);//assign right GOA signal
	jdi026_send_cmds_date(0x1E, 0xD9);//assign right GOA signal
	jdi026_send_cmds_date(0x1F, 0xD8);//assign right GOA signal
	jdi026_send_cmds_date(0x20, 0xD7);//assign right GOA signal
	jdi026_send_cmds_date(0x21, 0xC9);//assign right GOA signal
	jdi026_send_cmds_date(0x22, 0x59);//assign right GOA signal
	jdi026_send_cmds_date(0x23, 0x4B);//assign right GOA signal
	jdi026_send_cmds_date(0x24, 0x4C);//assign right GOA signal
	jdi026_send_cmds_date(0x25, 0x4F);//assign right GOA signal
	jdi026_send_cmds_date(0x27, 0x57);//assign right GOA signal
	jdi026_send_cmds_date(0x28, 0x41);//assign right GOA signal
	jdi026_send_cmds_date(0x00, 0x88);//P8
	jdi026_send_cmds_date(0x05, 0x01);//DCLK>20MHz, TR=100Ohm
	jdi026_send_cmds_date(0x00, 0x28);//P2 Red Gamma
	jdi026_send_cmds_date(0x01, 0x00);
	jdi026_send_cmds_date(0x02, 0x00);//digital gamma code of R255; the larger, the dimmer.
	jdi026_send_cmds_date(0x03, 0x00);
	jdi026_send_cmds_date(0x04, 0x8C);//digital gamma code of R251; the larger, the dimmer.
	jdi026_send_cmds_date(0x05, 0x00);
	jdi026_send_cmds_date(0x06, 0xCE);//digital gamma code of R247; the larger, the dimmer.
	jdi026_send_cmds_date(0x07, 0x00);
	jdi026_send_cmds_date(0x08, 0xFC);//digital gamma code of R243; the larger, the dimmer.
	jdi026_send_cmds_date(0x09, 0x01);
	jdi026_send_cmds_date(0x0A, 0x21);//digital gamma code of R239; the larger, the dimmer.
	jdi026_send_cmds_date(0x0B, 0x01);
	jdi026_send_cmds_date(0x0C, 0x82);//digital gamma code of R223; the larger, the dimmer.
	jdi026_send_cmds_date(0x0D, 0x01);
	jdi026_send_cmds_date(0x0E, 0xBF);//digital gamma code of R207; the larger, the dimmer.
	jdi026_send_cmds_date(0x0F, 0x01);
	jdi026_send_cmds_date(0x10, 0xED);//digital gamma code of R191; the larger, the dimmer.
	jdi026_send_cmds_date(0x11, 0x02);
	jdi026_send_cmds_date(0x12, 0x34);//digital gamma code of R159; the larger, the dimmer.
	jdi026_send_cmds_date(0x13, 0x02);
	jdi026_send_cmds_date(0x14, 0x6C);//digital gamma code of R127; the larger, the dimmer.
	jdi026_send_cmds_date(0x15, 0x02);
	jdi026_send_cmds_date(0x16, 0x9E);//digital gamma code of R95; the larger, the dimmer.
	jdi026_send_cmds_date(0x17, 0x02);
	jdi026_send_cmds_date(0x18, 0xD9);//digital gamma code of R63; the larger, the dimmer.
	jdi026_send_cmds_date(0x19, 0x02);
	jdi026_send_cmds_date(0x1A, 0xFE);//digital gamma code of R47; the larger, the dimmer.
	jdi026_send_cmds_date(0x1B, 0x03);
	jdi026_send_cmds_date(0x1C, 0x2F);//digital gamma code of R31; the larger, the dimmer.
	jdi026_send_cmds_date(0x1D, 0x03);
	jdi026_send_cmds_date(0x1E, 0x77);//digital gamma code of R15; the larger, the dimmer.
	jdi026_send_cmds_date(0x1F, 0x03);
	jdi026_send_cmds_date(0x20, 0x91);//digital gamma code of R11; the larger, the dimmer.
	jdi026_send_cmds_date(0x21, 0x03);
	jdi026_send_cmds_date(0x22, 0xB1);//digital gamma code of R7; the larger, the dimmer.
	jdi026_send_cmds_date(0x23, 0x03);
	jdi026_send_cmds_date(0x24, 0xDD);//digital gamma code of R3; the larger, the dimmer.
	jdi026_send_cmds_date(0x25, 0x03);
	jdi026_send_cmds_date(0x26, 0xFF);//digital gamma code of R0; the larger, the dimmer.
	jdi026_send_cmds_date(0x00, 0x38);//P3 Green Gamma
	jdi026_send_cmds_date(0x01, 0x00);
	jdi026_send_cmds_date(0x02, 0x00);//digital gamma code of G255; the larger, the dimmer.
	jdi026_send_cmds_date(0x03, 0x00);
	jdi026_send_cmds_date(0x04, 0x8C);//digital gamma code of G251; the larger, the dimmer.
	jdi026_send_cmds_date(0x05, 0x00);
	jdi026_send_cmds_date(0x06, 0xCE);//digital gamma code of G247; the larger, the dimmer.
	jdi026_send_cmds_date(0x07, 0x00);
	jdi026_send_cmds_date(0x08, 0xFC);//digital gamma code of G243; the larger, the dimmer.
	jdi026_send_cmds_date(0x09, 0x01);
	jdi026_send_cmds_date(0x0A, 0x21);//digital gamma code of G239; the larger, the dimmer.
	jdi026_send_cmds_date(0x0B, 0x01);
	jdi026_send_cmds_date(0x0C, 0x82);//digital gamma code of G223; the larger, the dimmer.
	jdi026_send_cmds_date(0x0D, 0x01);
	jdi026_send_cmds_date(0x0E, 0xBF);//digital gamma code of G207; the larger, the dimmer.
	jdi026_send_cmds_date(0x0F, 0x01);
	jdi026_send_cmds_date(0x10, 0xED);//digital gamma code of G191; the larger, the dimmer.
	jdi026_send_cmds_date(0x11, 0x02);
	jdi026_send_cmds_date(0x12, 0x34);//digital gamma code of G159; the larger, the dimmer.
	jdi026_send_cmds_date(0x13, 0x02);
	jdi026_send_cmds_date(0x14, 0x6C);//digital gamma code of G127; the larger, the dimmer.
	jdi026_send_cmds_date(0x15, 0x02);
	jdi026_send_cmds_date(0x16, 0x9E);//digital gamma code of G95; the larger, the dimmer.
	jdi026_send_cmds_date(0x17, 0x02);
	jdi026_send_cmds_date(0x18, 0xD9);//digital gamma code of G63; the larger, the dimmer.
	jdi026_send_cmds_date(0x19, 0x02);
	jdi026_send_cmds_date(0x1A, 0xFE);//digital gamma code of G47; the larger, the dimmer.
	jdi026_send_cmds_date(0x1B, 0x03);
	jdi026_send_cmds_date(0x1C, 0x2F);//digital gamma code of G31; the larger, the dimmer.
	jdi026_send_cmds_date(0x1D, 0x03);
	jdi026_send_cmds_date(0x1E, 0x77);//digital gamma code of G15; the larger, the dimmer.
	jdi026_send_cmds_date(0x1F, 0x03);
	jdi026_send_cmds_date(0x20, 0x91);//digital gamma code of G11; the larger, the dimmer.
	jdi026_send_cmds_date(0x21, 0x03);
	jdi026_send_cmds_date(0x22, 0xB1);//digital gamma code of G7; the larger, the dimmer.
	jdi026_send_cmds_date(0x23, 0x03);
	jdi026_send_cmds_date(0x24, 0xDD);//digital gamma code of G3; the larger, the dimmer.
	jdi026_send_cmds_date(0x25, 0x03);
	jdi026_send_cmds_date(0x26, 0xFF);//digital gamma code of G0; the larger, the dimmer.
	jdi026_send_cmds_date(0x00, 0x48);//P4 Blue Gamma
	jdi026_send_cmds_date(0x01, 0x00);
	jdi026_send_cmds_date(0x02, 0x00);//digital gamma code of B255; the larger, the dimmer.
	jdi026_send_cmds_date(0x03, 0x00);
	jdi026_send_cmds_date(0x04, 0x8C);//digital gamma code of B251; the larger, the dimmer.
	jdi026_send_cmds_date(0x05, 0x00);
	jdi026_send_cmds_date(0x06, 0xCE);//digital gamma code of B247; the larger, the dimmer.
	jdi026_send_cmds_date(0x07, 0x00);
	jdi026_send_cmds_date(0x08, 0xFC);//digital gamma code of B243; the larger, the dimmer.
	jdi026_send_cmds_date(0x09, 0x01);
	jdi026_send_cmds_date(0x0A, 0x21);//digital gamma code of B239; the larger, the dimmer.
	jdi026_send_cmds_date(0x0B, 0x01);
	jdi026_send_cmds_date(0x0C, 0x82);//digital gamma code of B223; the larger, the dimmer.
	jdi026_send_cmds_date(0x0D, 0x01);
	jdi026_send_cmds_date(0x0E, 0xBF);//digital gamma code of B207; the larger, the dimmer.
	jdi026_send_cmds_date(0x0F, 0x01);
	jdi026_send_cmds_date(0x10, 0xED);//digital gamma code of B191; the larger, the dimmer.
	jdi026_send_cmds_date(0x11, 0x02);
	jdi026_send_cmds_date(0x12, 0x34);//digital gamma code of B159; the larger, the dimmer.
	jdi026_send_cmds_date(0x13, 0x02);
	jdi026_send_cmds_date(0x14, 0x6C);//digital gamma code of B127; the larger, the dimmer.
	jdi026_send_cmds_date(0x15, 0x02);
	jdi026_send_cmds_date(0x16, 0x9E);//digital gamma code of B95; the larger, the dimmer.
	jdi026_send_cmds_date(0x17, 0x02);
	jdi026_send_cmds_date(0x18, 0xD9);//digital gamma code of B63; the larger, the dimmer.
	jdi026_send_cmds_date(0x19, 0x02);
	jdi026_send_cmds_date(0x1A, 0xFE);//digital gamma code of B47; the larger, the dimmer.
	jdi026_send_cmds_date(0x1B, 0x03);
	jdi026_send_cmds_date(0x1C, 0x2F);//digital gamma code of B31; the larger, the dimmer.
	jdi026_send_cmds_date(0x1D, 0x03);
	jdi026_send_cmds_date(0x1E, 0x77);//digital gamma code of B15; the larger, the dimmer.
	jdi026_send_cmds_date(0x1F, 0x03);
	jdi026_send_cmds_date(0x20, 0x91);//digital gamma code of B11; the larger, the dimmer.
	jdi026_send_cmds_date(0x21, 0x03);
	jdi026_send_cmds_date(0x22, 0xB1);//digital gamma code of B7; the larger, the dimmer.
	jdi026_send_cmds_date(0x23, 0x03);
	jdi026_send_cmds_date(0x24, 0xDD);//digital gamma code of B3; the larger, the dimmer.
	jdi026_send_cmds_date(0x25, 0x03);
	jdi026_send_cmds_date(0x26, 0xFF);//digital gamma code of B0; the larger, the dimmer.
	lcd_stbyb_normal();
	return 0;
}

static void gpio_spi_set_mosi(unsigned char data)
{
    lcd_gpio_set_output(jdi026dev.spi_mosi_num,(bool)data);
}

static void gpio_spi_generate_clk(void)
{   
    if(jdi026dev.spi_clk_vaild_edge == 1)//1 
		lcd_gpio_set_output(jdi026dev.spi_clk_num,1);
	else
		lcd_gpio_set_output(jdi026dev.spi_clk_num,0);
	usleep(2);
	if(jdi026dev.spi_clk_vaild_edge == 1)//1 
		lcd_gpio_set_output(jdi026dev.spi_clk_num,0);
	else
		lcd_gpio_set_output(jdi026dev.spi_clk_num,1);
}
static void gpio_spi_enable_cs(void)//==0
{
	if(jdi026dev.spi_cs_polar == 0)	
		lcd_gpio_set_output(jdi026dev.spi_cs_num,0);
	else
		lcd_gpio_set_output(jdi026dev.spi_cs_num,1);
}

static void gpio_spi_disable_cs(void)
{
	if(jdi026dev.spi_cs_polar == 0)
		lcd_gpio_set_output(jdi026dev.spi_cs_num,1);//cs
	else
		lcd_gpio_set_output(jdi026dev.spi_cs_num,0);//cs
}

static void gpio_spi_init_clk(void)
{
	if(jdi026dev.spi_clk_vaild_edge == 1)//1 //==1
		lcd_gpio_set_output(jdi026dev.spi_clk_num,0);//sck
	else
		lcd_gpio_set_output(jdi026dev.spi_clk_num,1);//sck
		
}

static void jdi026_write_data(unsigned char data)
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

static void jdi026_write_command(unsigned short cmds)
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
	lcd_gpio_set_output(jdi026dev.lcd_stbyb_num,!jdi026dev.lcd_stbyb_polar);
	usleep(200*500);
	lcd_gpio_set_output(jdi026dev.lcd_stbyb_num,jdi026dev.lcd_stbyb_polar);
	usleep(200*500);
}

static void lcd_stbyb_normal(void)
{
	usleep(10*1000);
	lcd_gpio_set_output(jdi026dev.lcd_stbyb_num,!jdi026dev.lcd_stbyb_polar);
}

static struct lcd_map_list jdi026_map = {
	.map = {
		.lcd_init = jdi026_display_init,
		.name = "lcd-jdi026",
	}
};

static int jdi026_probe(const char *node)
{
	int np = fdt_node_probe_by_path(node);

	if(np < 0){
		goto error;
	}

	memset(&jdi026dev,0,sizeof(struct jdi026_dev));

	jdi026dev.spi_clk_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.sck;
	jdi026dev.spi_clk_vaild_edge=1;
	jdi026dev.spi_cs_polar=0;
	jdi026dev.spi_is_9bit=1;
	jdi026dev.spi_cs_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.cs;
	jdi026dev.spi_mosi_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.mosi;
	jdi026dev.spi_miso_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.miso;
	jdi026dev.lcd_stbyb_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.stbyb;
	jdi026dev.lcd_stbyb_polar = 0;

#if 0
	jdi026dev.spi_clk_num=PINPAD_T01;
	jdi026dev.spi_clk_vaild_edge=1;
	jdi026dev.spi_cs_polar=0;
	jdi026dev.spi_is_9bit=1;
	jdi026dev.spi_cs_num=PINPAD_T00;
	jdi026dev.spi_mosi_num=PINPAD_T02;
	jdi026dev.lcd_stbyb_num=PINPAD_T03;
	jdi026dev.lcd_stbyb_polar=0;
#endif

	fdt_get_property_u_32_index(np, "spi-gpio-sck", 	0, &jdi026dev.spi_clk_num);
	fdt_get_property_u_32_index(np, "spi-gpio-mosi", 	0, &jdi026dev.spi_mosi_num);
	fdt_get_property_u_32_index(np, "spi-gpio-miso", 	0, &jdi026dev.spi_miso_num);
	fdt_get_property_u_32_index(np, "spi-gpio-cs", 		0, &jdi026dev.spi_cs_num);
	fdt_get_property_u_32_index(np, "spi-gpio-stbyb", 	0, &jdi026dev.lcd_stbyb_num);
	log_d("jdi026dev.lcd_stbyb_num = %d %d %d %d\n",jdi026dev.spi_clk_num,jdi026dev.spi_mosi_num,jdi026dev.spi_cs_num,jdi026dev.lcd_stbyb_num);

	int default_off = 0;
	fdt_get_property_u_32_index(np, "default-off", 	0, &default_off);
	if(default_off ==0)
		jdi026_display_init();

	jdi026_map.map.default_off_val = default_off;
	lcd_map_register(&jdi026_map);
error:
	return 0;
}

static int jdi026_init(void)
{
	jdi026_probe("/hcrtos/lcd-jdi026");
	return 0;
}

module_driver(jdi026, jdi026_init, NULL, 2)
