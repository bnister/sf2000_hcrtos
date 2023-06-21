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
		default-off;wait no
		status = "okay";
	};
	lcd{
		lcd-map-name = "lcd-st7789v2";
		default-off;
		status = "okay";
	};
*/

/* SF2000
24 D15   106 T06
23 D14   105 T05
22 D13   104 T04
21 D12   103 T03
20 D11   102 T02
19 D10   114 T14
18 D9    113 T13
17 D8    112 T12
16 D7    111 T11
15 D6    110 T10
14 D5    109 T09
13 D4    5   L06
12 D3    4   L05
11 D2    3   L04
10 D1    2   L03
9  D0    1   L02
8  RD    100 T00
7  WR    6   L07
6  RS    101 T01
5  CS    9   L10
4  RESET 128 L01
 */


static void st7789v2_spi_sends_data(unsigned char *data,unsigned char len);
static void st7789v2_write_command(unsigned short cmds);
static void st7789v2_write_data(unsigned char data);
static int st7789v2_display_init(void);
static int st7789v2_rorate(lcd_rotate_type_e dir);
static void lcd_reset(void);

typedef struct st7789v2_dev{
	u32 lcd_cs_num;
    u32 lcd_rs_num;
    u32 lcd_wr_num;
    u32 lcd_rd_num;
    u32 lcd_reset_num;
    u32 lcd_d0_num;
    u32 lcd_d1_num;
    u32 lcd_d2_num;
    u32 lcd_d3_num;
    u32 lcd_d4_num;
    u32 lcd_d5_num;
    u32 lcd_d6_num;
    u32 lcd_d7_num;
    u32 lcd_d8_num;
    u32 lcd_d9_num;
    u32 lcd_d10_num;
    u32 lcd_d11_num;
    u32 lcd_d12_num;
    u32 lcd_d13_num;
    u32 lcd_d14_num;
    u32 lcd_d15_num;
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
    gpio_configure(st7789v2dev.lcd_cs_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_rs_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_wr_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_rd_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d0_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d1_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d2_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d3_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d4_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d5_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d6_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d7_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d8_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d9_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d10_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d11_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d12_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d13_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d14_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d15_num,GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_reset_num,GPIO_DIR_OUTPUT);
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

static void gpio_set_cs(bool state)
{
    lcd_gpio_set_output(st7789v2dev.spi_cs_num,state);
}

static void gpio_set_rs(bool state)
{
    lcd_gpio_set_output(st7789v2dev.spi_rs_num,state);
}

static void gpio_set_wr(bool state)
{
    lcd_gpio_set_output(st7789v2dev.spi_wr_num,state);
}

static void gpio_set_data(int pin, bool state)
{
    switch (pin)
    {
        case 0:
		lcd_gpio_set_output(st7789v2dev.lcd_d0_num,state);
		break;
	case 1:
		lcd_gpio_set_output(st7789v2dev.lcd_d1_num,state);
		break;
	case 2:
		lcd_gpio_set_output(st7789v2dev.lcd_d2_num,state);
		break;
	case 3:
		lcd_gpio_set_output(st7789v2dev.lcd_d3_num,state);
		break;
	case 4:
		lcd_gpio_set_output(st7789v2dev.lcd_d4_num,state);
		break;
	case 5:
		lcd_gpio_set_output(st7789v2dev.lcd_d5_num,state);
		break;
	case 6:
		lcd_gpio_set_output(st7789v2dev.lcd_d6_num,state);
		break;
	case 7:
		lcd_gpio_set_output(st7789v2dev.lcd_d7_num,state);
		break;
	case 8:
		lcd_gpio_set_output(st7789v2dev.lcd_d8_num,state);
		break;
	case 9:
		lcd_gpio_set_output(st7789v2dev.lcd_d9_num,state);
		break;
	case 10:
		lcd_gpio_set_output(st7789v2dev.lcd_d10_num,state);
		break;
	case 11:
		lcd_gpio_set_output(st7789v2dev.lcd_d11_num,state);
		break;
	case 12:
		lcd_gpio_set_output(st7789v2dev.lcd_d12_num,state);
		break;
	case 13:
		lcd_gpio_set_output(st7789v2dev.lcd_d13_num,state);
		break;
	case 14:
		lcd_gpio_set_output(st7789v2dev.lcd_d14_num,state);
		break;
	case 15:
		lcd_gpio_set_output(st7789v2dev.lcd_d15_num,state);
		break;
    }

}

/*
void LCD_cmd_80290500(int cmd)
{
  LCS_rs_lo_8029045c();
  LCD_cs_lo_802903e4();
  LCD_bus_write_80290498(cmd);
  LCD_wr_lo_80290420();
  LCD_wr_hi_80290440();
  LCD_cs_hi_80290404();
  LCD_rs_hi_8029047c();
}
 */

static void lcd_gpio_spi_config_write(unsigned char RS, unsigned char cmd)
{
	int i=0;
	unsigned char cmd_val = 0;
	gpio_set_cs(0);
    	gpio_set_rs(RS);
    	gpio_set_wr(0);
	//usleep(2);
	for(i=7;i>=0;i--){
		cmd_val = (cmd>>(i))&0x1;
		gpio_set_data(i, cmd_val);
	}
	usleep(10); //data setup time
	gpio_set_wr(1);
	//usleep(10); //Address hold time
	usleep(15); //control pulse duration
	gpio_set_wr(0);
	gpio_set_cs(1);
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
		lcd_gpio_set_output(st7789v2dev.lcd_reset_num,1);
		usleep(500*1000);
		lcd_gpio_set_output(st7789v2dev.lcd_reset_num,0);
		usleep(500*1000);
		lcd_gpio_set_output(st7789v2dev.lcd_reset_num,1);
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

	st7789v2dev.lcd_cs_num = PINPAD_INVALID;
	st7789v2dev.lcd_rs_num = PINPAD_INVALID;
	st7789v2dev.lcd_wr_num = PINPAD_INVALID;
	st7789v2dev.lcd_rd_num = PINPAD_INVALID;
	st7789v2dev.lcd_reset_num = PINPAD_INVALID;
	st7789v2dev.lcd_d0_num = PINPAD_INVALID;
	st7789v2dev.lcd_d1_num = PINPAD_INVALID;
	st7789v2dev.lcd_d2_num = PINPAD_INVALID;
	st7789v2dev.lcd_d3_num = PINPAD_INVALID;
	st7789v2dev.lcd_d4_num = PINPAD_INVALID;
	st7789v2dev.lcd_d5_num = PINPAD_INVALID;
	st7789v2dev.lcd_d6_num = PINPAD_INVALID;
	st7789v2dev.lcd_d7_num = PINPAD_INVALID;
	st7789v2dev.lcd_d8_num = PINPAD_INVALID;
	st7789v2dev.lcd_d9_num = PINPAD_INVALID;
	st7789v2dev.lcd_d10_num = PINPAD_INVALID;
	st7789v2dev.lcd_d11_num = PINPAD_INVALID;
	st7789v2dev.lcd_d12_num = PINPAD_INVALID;
	st7789v2dev.lcd_d13_num = PINPAD_INVALID;
	st7789v2dev.lcd_d14_num = PINPAD_INVALID;
	st7789v2dev.lcd_d15_num = PINPAD_INVALID;

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

	fdt_get_property_u_32_index(np, "lcd_cs", 0, st7789v2dev.lcd_cs_num);
	fdt_get_property_u_32_index(np, "lcd_rs", 0, st7789v2dev.lcd_rs_num);
	fdt_get_property_u_32_index(np, "lcd_wr", 0, st7789v2dev.lcd_wr_num);
	fdt_get_property_u_32_index(np, "lcd_rd", 0, st7789v2dev.lcd_rd_num);
	fdt_get_property_u_32_index(np, "lcd_reset", 0, st7789v2dev.lcd_reset_num);
	fdt_get_property_u_32_index(np, "lcd_d0", 0, st7789v2dev.lcd_d0_num);
	fdt_get_property_u_32_index(np, "lcd_d1", 0, st7789v2dev.lcd_d1_num);
	fdt_get_property_u_32_index(np, "lcd_d2", 0, st7789v2dev.lcd_d2_num);
	fdt_get_property_u_32_index(np, "lcd_d3", 0, st7789v2dev.lcd_d3_num);
	fdt_get_property_u_32_index(np, "lcd_d4", 0, st7789v2dev.lcd_d4_num);
	fdt_get_property_u_32_index(np, "lcd_d5", 0, st7789v2dev.lcd_d5_num);
	fdt_get_property_u_32_index(np, "lcd_d6", 0, st7789v2dev.lcd_d6_num);
	fdt_get_property_u_32_index(np, "lcd_d7", 0, st7789v2dev.lcd_d7_num);
	fdt_get_property_u_32_index(np, "lcd_d8", 0, st7789v2dev.lcd_d8_num);
	fdt_get_property_u_32_index(np, "lcd_d9", 0, st7789v2dev.lcd_d9_num);
	fdt_get_property_u_32_index(np, "lcd_d10", 0, st7789v2dev.lcd_d10_num);
	fdt_get_property_u_32_index(np, "lcd_d11", 0, st7789v2dev.lcd_d11_num);
	fdt_get_property_u_32_index(np, "lcd_d12", 0, st7789v2dev.lcd_d12_num);
	fdt_get_property_u_32_index(np, "lcd_d13", 0, st7789v2dev.lcd_d13_num);
	fdt_get_property_u_32_index(np, "lcd_d14", 0, st7789v2dev.lcd_d14_num);
	fdt_get_property_u_32_index(np, "lcd_d15", 0, st7789v2dev.lcd_d15_num);

	st7789v2_display_init();

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
