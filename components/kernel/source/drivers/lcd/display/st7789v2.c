#define LOG_TAG "lcd_st7789v2"
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
#include <kernel/delay.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../lcd_main.h"

/*
*	TIME: 2023 04 23
*	support: TPR yt300 BOARD

	lcd-st7789v2{
		spi-gpio-sck	= <PINPAD_L20>;
		spi-gpio-mosi	= <PINPAD_L21>;
		spi-gpio-cs		= <PINPAD_L19>;
		reset = <PINPAD_L20>;
		default-off;
		status = "okay";
	};
	lcd{
		lcd-map-name = "lcd-st7789v2";
		default-off;
		status = "okay";
	};
*/


static void st7789v2_spi_sends_data(unsigned char *data,unsigned char len);
static void st7789v2_write_command(unsigned short cmds);
static void st7789v2_write_data(unsigned char data);
static int st7789v2_display_init(void);
static int st7789v2_rorate(lcd_rotate_type_e dir);
static void lcd_reset(void);

typedef struct st7789v2_dev{
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
}st7789v2_dev_t;
static st7789v2_dev_t st7789v2dev;

static int st7789v2_rorate(lcd_rotate_type_e dir)
{
	unsigned char data = 0x0+0x10;
	printf("dir = %d\n",dir);
	switch(dir)
	{
		case LCD_ROTATE_0:
			data = 0x80+0x10;//0xc0+0x10;//0xc0
			break;
		case LCD_H_MIRROR:
			data = 0xc0+0x10;//0x80+0x10;//0x00
			break;
		case LCD_ROTATE_180:
			data = 0x00+0x10;//0x40+0x10;
			break;
		case LCD_V_MIRROR:
			data = 0x40+0x10;//0x40//0x00+0x10;//0x80
			break;
	}
	st7789v2_write_command(0x36);	  st7789v2_write_data(data);//(data); //v
	return 0;
}
static int st7789v2_display_init(void)
{
	gpio_configure(st7789v2dev.spi_clk_num,GPIO_DIR_OUTPUT);//clk
	gpio_configure(st7789v2dev.spi_cs_num,GPIO_DIR_OUTPUT);//mosi
	gpio_configure(st7789v2dev.spi_mosi_num,GPIO_DIR_OUTPUT);//cs
	gpio_configure(st7789v2dev.lcd_reset_num,GPIO_DIR_OUTPUT);//reset
	printf("%s %d\n", __FUNCTION__,__LINE__);

	lcd_reset();

	msleep(120);
#if 0//old spi 	
	st7789v2_write_command(0x11);
	msleep(120);
	st7789v2_write_command(0x36);
	st7789v2_write_data(0x90);
	st7789v2_write_command(0x3A);
	st7789v2_write_data(0x06);
	st7789v2_write_command(0xB2);
	st7789v2_write_data(0x0C);
	st7789v2_write_data(0x0C);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x33);
	st7789v2_write_data(0x33);
	st7789v2_write_command(0xB7);
	st7789v2_write_data(0x35);
	st7789v2_write_command(0xBB);
	st7789v2_write_data(0x1E);
	st7789v2_write_command(0xC0);
	st7789v2_write_data(0x2C);
	st7789v2_write_command(0xC2);
	st7789v2_write_data(0x01);
	st7789v2_write_command(0xC3);
	st7789v2_write_data(0x27);
	st7789v2_write_command(0xC4);
	st7789v2_write_data(0x20);
	st7789v2_write_command(0xC6);
	st7789v2_write_data(0x0F);
	st7789v2_write_command(0xD0);
	st7789v2_write_data(0xA4);
	st7789v2_write_data(0xA1);
	st7789v2_write_command(0xE0);
	st7789v2_write_data(0xD0);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x05);
	st7789v2_write_data(0x03);
	st7789v2_write_data(0x02);
	st7789v2_write_data(0x07);
	st7789v2_write_data(0x3F);
	st7789v2_write_data(0x55);
	st7789v2_write_data(0x50);
	st7789v2_write_data(0x09);
	st7789v2_write_data(0x14);
	st7789v2_write_data(0x15);
	st7789v2_write_data(0x22);
	st7789v2_write_data(0x25);
	st7789v2_write_command(0xE1);
	st7789v2_write_data(0xD0);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x05);
	st7789v2_write_data(0x03);
	st7789v2_write_data(0x02);
	st7789v2_write_data(0x07);
	st7789v2_write_data(0x3F);
	st7789v2_write_data(0x55);
	st7789v2_write_data(0x54);
	st7789v2_write_data(0x0C);
	st7789v2_write_data(0x18);
	st7789v2_write_data(0x14);
	st7789v2_write_data(0x22);
	st7789v2_write_data(0x25);
	st7789v2_write_command(0xB0);
	st7789v2_write_data(0x11);
	st7789v2_write_data(0xF0);
	st7789v2_write_command(0xB1);
	st7789v2_write_data(0x42);
	st7789v2_write_data(0x08);
	st7789v2_write_data(0x14);
	st7789v2_write_command(0xB2);
	st7789v2_write_data(0x14);
	st7789v2_write_data(0x08);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x44);
	st7789v2_write_data(0x44);
	st7789v2_write_command(0x2B);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x01);
	st7789v2_write_data(0x3F);
	st7789v2_write_command(0x2A);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0xEF);
	st7789v2_write_command(0x51);
	st7789v2_write_data(0xFF);
	st7789v2_write_command(0x55);
	st7789v2_write_data(0x11);
	st7789v2_write_command(0x29);
#endif 
	#if 1
	st7789v2_write_command( 0x11); 

	msleep(120);
	st7789v2_write_command(0xB0);
	st7789v2_write_data(0x11);
	st7789v2_write_data(0xF0);
	st7789v2_write_command(0xB1);
	st7789v2_write_data(0x42);
	st7789v2_write_data(0x08);
	st7789v2_write_data(0x14);

	st7789v2_write_command( 0xB2); 
	st7789v2_write_data(0x0C); 
	st7789v2_write_data(0x0C); 
	st7789v2_write_data(0x00); 
	st7789v2_write_data(0x33); 
	st7789v2_write_data(0x33);

	st7789v2_write_command( 0x35); 
	st7789v2_write_data(0x00);

	st7789v2_write_command( 0x36); 
	st7789v2_write_data(0x00); 

	st7789v2_write_command( 0x3A); 
	st7789v2_write_data(0x66); 
	//st7789v2_write_data(0x05); 

	st7789v2_write_command( 0xB7); 
	st7789v2_write_data(0x35); 

	st7789v2_write_command( 0xBB); 
	st7789v2_write_data(0x34); 

	st7789v2_write_command( 0xC0); 
	st7789v2_write_data(0x2C); 

	st7789v2_write_command( 0xC2); 
	st7789v2_write_data(0x01); 

	st7789v2_write_command( 0xC3); 
	st7789v2_write_data(0x13);//4.5V 

	st7789v2_write_command( 0xC4); 
	st7789v2_write_data(0x20);

	st7789v2_write_command( 0xC6); 
	st7789v2_write_data(0x0F); 

	st7789v2_write_command( 0xD0); 
	st7789v2_write_data(0xA4); 
	st7789v2_write_data(0xA1); 

	st7789v2_write_command( 0xD6); 
	st7789v2_write_data(0xA1); 

	st7789v2_write_command( 0xE0);
	st7789v2_write_data(0xD0);
	st7789v2_write_data(0x0A);
	st7789v2_write_data(0x10);
	st7789v2_write_data(0x0C);
	st7789v2_write_data(0x0C);
	st7789v2_write_data(0x18);
	st7789v2_write_data(0x35);
	st7789v2_write_data(0x43);
	st7789v2_write_data(0x4D);
	st7789v2_write_data(0x39);
	st7789v2_write_data(0x13);
	st7789v2_write_data(0x13);
	st7789v2_write_data(0x2D);
	st7789v2_write_data(0x34);

	st7789v2_write_command( 0xE1);
	st7789v2_write_data(0xD0);
	st7789v2_write_data(0x05);
	st7789v2_write_data(0x0B);
	st7789v2_write_data(0x06);
	st7789v2_write_data(0x05);
	st7789v2_write_data(0x02);
	st7789v2_write_data(0x35);
	st7789v2_write_data(0x43);
	st7789v2_write_data(0x4D);
	st7789v2_write_data(0x16);
	st7789v2_write_data(0x15);
	st7789v2_write_data(0x15);
	st7789v2_write_data(0x2E);
	st7789v2_write_data(0x32);

	st7789v2_write_command( 0x21);

	st7789v2_write_command( 0x29); 

	st7789v2_write_command( 0x2C); 
	#endif
	return 0;
}

static void gpio_spi_set_mosi(unsigned char data)
{
    lcd_gpio_set_output(st7789v2dev.spi_mosi_num,(bool)data);
}

static void gpio_spi_generate_clk(void)
{   
    if(st7789v2dev.spi_clk_vaild_edge == 1)//1 
		lcd_gpio_set_output(st7789v2dev.spi_clk_num,1);
	else
		lcd_gpio_set_output(st7789v2dev.spi_clk_num,0);
	usleep(2);
	if(st7789v2dev.spi_clk_vaild_edge == 1)//1 
		lcd_gpio_set_output(st7789v2dev.spi_clk_num,0);
	else
		lcd_gpio_set_output(st7789v2dev.spi_clk_num,1);
}
static void gpio_spi_enable_cs(void)//==0
{
	if(st7789v2dev.spi_cs_polar == 0)	
		lcd_gpio_set_output(st7789v2dev.spi_cs_num,0);
	else
		lcd_gpio_set_output(st7789v2dev.spi_cs_num,1);
}

static void gpio_spi_disable_cs(void)
{
	if(st7789v2dev.spi_cs_polar == 0)
		lcd_gpio_set_output(st7789v2dev.spi_cs_num,1);//cs
	else
		lcd_gpio_set_output(st7789v2dev.spi_cs_num,0);//cs
}

static void gpio_spi_init_clk(void)
{
	if(st7789v2dev.spi_clk_vaild_edge == 1)//1 //==1
		lcd_gpio_set_output(st7789v2dev.spi_clk_num,0);//sck
	else
		lcd_gpio_set_output(st7789v2dev.spi_clk_num,1);//sck
		
}

static void lcd_gpio_spi_config_write(unsigned char bit_9,unsigned char cmd)
{
	int i=0;
	unsigned char cmd_val = 0;
	gpio_spi_disable_cs();//cs
	usleep(10);
	gpio_spi_init_clk();//sck
	gpio_spi_enable_cs();
	usleep(2);
	if(st7789v2dev.spi_is_9bit == 1)
	{
		gpio_spi_set_mosi(bit_9);//sda dat=0
		usleep(3);
		gpio_spi_generate_clk();
	}
	for(i=8;i>0;i--){
		cmd_val = (cmd>>(i-1))&0x1;
		gpio_spi_set_mosi(cmd_val);
		usleep(2);
		gpio_spi_generate_clk();
	}
	usleep(2);
	gpio_spi_disable_cs();
	gpio_spi_set_mosi(0);
	usleep(10);
}

static void st7789v2_write_data(unsigned char data)
{
	lcd_gpio_spi_config_write(1,data);
}

static void st7789v2_spi_sends_data(unsigned char *data,unsigned char len)
{
	int i=len;
	do{
		st7789v2_write_data(*data++);
	}
	while(i--);
}
static void st7789v2_write_command(unsigned short cmds)
{
	lcd_gpio_spi_config_write(0,(unsigned char)cmds);
}

static void lcd_reset(void)
{
	if(st7789v2dev.lcd_reset_num!=PINPAD_INVALID)
	{
		lcd_gpio_set_output(st7789v2dev.lcd_reset_num,!st7789v2dev.lcd_reset_polar);
		usleep(500*1000);
		lcd_gpio_set_output(st7789v2dev.lcd_reset_num,st7789v2dev.lcd_reset_polar);
		usleep(500*1000);
		lcd_gpio_set_output(st7789v2dev.lcd_reset_num,!st7789v2dev.lcd_reset_polar);
		usleep(500*1000);
	}
}

static struct lcd_map_list st7789v2_map = {
	.map = {
		.lcd_init = st7789v2_display_init,
		.lcd_rorate = st7789v2_rorate,
		.name = "lcd-st7789v2",
	}
};

static int st7789v2_probe(const char *node)
{
	int np = fdt_node_probe_by_path(node);

	if(np < 0){
		goto error;
	}

	memset(&st7789v2dev,0,sizeof(struct st7789v2_dev));

	st7789v2dev.spi_clk_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.sck;
	st7789v2dev.spi_clk_vaild_edge=1;
	st7789v2dev.spi_cs_polar=0;
	st7789v2dev.spi_is_9bit=1;
	st7789v2dev.spi_cs_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.cs;
	st7789v2dev.spi_mosi_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.mosi;
	st7789v2dev.spi_miso_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.miso;
	st7789v2dev.lcd_stbyb_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.stbyb;
	st7789v2dev.lcd_stbyb_polar = 0;
	st7789v2dev.lcd_reset_num = PINPAD_INVALID;

#if 0
	st7789v2dev.spi_clk_num=PINPAD_L20;
	st7789v2dev.spi_clk_vaild_edge=1;
	st7789v2dev.spi_cs_polar=0;
	st7789v2dev.spi_is_9bit=1;
	st7789v2dev.spi_cs_num=PINPAD_L19;
	st7789v2dev.spi_mosi_num=PINPAD_L21;
	st7789v2dev.lcd_reset_num=PINPAD_L00;
	st7789v2dev.lcd_reset_polar=0;
#endif

	fdt_get_property_u_32_index(np, "reset", 			0, &st7789v2dev.lcd_reset_num);
	fdt_get_property_u_32_index(np, "spi-gpio-sck", 	0, &st7789v2dev.spi_clk_num);
	fdt_get_property_u_32_index(np, "spi-gpio-mosi", 	0, &st7789v2dev.spi_mosi_num);
	fdt_get_property_u_32_index(np, "spi-gpio-miso", 	0, &st7789v2dev.spi_miso_num);
	fdt_get_property_u_32_index(np, "spi-gpio-cs", 		0, &st7789v2dev.spi_cs_num);
	fdt_get_property_u_32_index(np, "spi-gpio-stbyb", 	0, &st7789v2dev.lcd_stbyb_num);
	log_d("st7789v2dev.lcd_stbyb_num = %d %d %d %d\n",st7789v2dev.spi_clk_num,st7789v2dev.spi_mosi_num,st7789v2dev.spi_cs_num,st7789v2dev.lcd_stbyb_num);

	int default_off = 0;
	fdt_get_property_u_32_index(np, "default-off", 	0, &default_off);
	if(default_off ==0)
		st7789v2_display_init();

	st7789v2_map.map.default_off_val = default_off;

	lcd_map_register(&st7789v2_map);
error:
	return 0;
}

static int st7789v2_init(void)
{
	st7789v2_probe("/hcrtos/lcd-st7789v2");
	return 0;
}

module_driver(st7789v2, st7789v2_init, NULL, 2)
