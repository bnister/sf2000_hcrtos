#define LOG_TAG "lcd_jd9168"
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
*	support: TPR W2 BOARD
*	PINPAD_T01 clk
*	PINPAD_T02 mosi
*	PINPAD_T00 cs
*	PINPAD_T03 STBYB
*	PINPAD_T04 backlight

	lcd-jd9168{
		spi-gpio-sck	= <PINPAD_T01>;
		spi-gpio-mosi	= <PINPAD_T02>;
		spi-gpio-cs		= <PINPAD_T00>;
		spi-gpio-stbyb	= <PINPAD_T03>;
		default-off;
		status = "okay";
	};
	lcd{
		lcd-map-name = "lcd-jd9168";
		default-off;
		status = "okay";
	};
*/

typedef struct jd9168_dev{
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
}jd9168_dev_t;


#define SET_DWORD(i, d)         (*(volatile unsigned long *)(i)) = (d)
#define GET_DWORD(i)            (*(volatile unsigned long *)(i))
#define LVDS_GPIO_SUPPORT 0

static void lcd_stbyb_start(void);
static void lcd_stbyb_normal(void);
static void jd9168_write_data(unsigned char cmds);
void LCD_SCS_CLEAR(void);

static jd9168_dev_t jd9168dev;

static int jd9168_display_init(void)
{
	// gpio_configure(PINPAD_T04,GPIO_DIR_OUTPUT);//backlight
	// lcd_gpio_set_output(PINPAD_T04,1);
	gpio_configure(jd9168dev.spi_clk_num,GPIO_DIR_OUTPUT);//clk
	gpio_configure(jd9168dev.spi_mosi_num,GPIO_DIR_OUTPUT);//mosi
	gpio_configure(jd9168dev.spi_cs_num,GPIO_DIR_OUTPUT);//cs
	gpio_configure(jd9168dev.lcd_stbyb_num,GPIO_DIR_OUTPUT);//STBYB
	gpio_configure(jd9168dev.spi_miso_num,GPIO_DIR_OUTPUT);//miso

	lcd_stbyb_start();
	printf("%s %d\n",__FUNCTION__,__LINE__);
	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x03);
	jd9168_write_data(0xDF);
	jd9168_write_data(0x91);
	jd9168_write_data(0x68);
	jd9168_write_data(0xF9);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x01);
	jd9168_write_data(0xDE);
	jd9168_write_data(0x00);
	LCD_SCS_CLEAR();
	/*
	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x01);
	jd9168_write_data(0xC2);
	jd9168_write_data(0x10);
	LCD_SCS_CLEAR();
	*/
	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x02);
	jd9168_write_data(0xB2);
	jd9168_write_data(0x00);
	jd9168_write_data(0x60);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x02);
	jd9168_write_data(0xB3);
	jd9168_write_data(0x00);
	jd9168_write_data(0x4E);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x06);
	jd9168_write_data(0xC1);
	jd9168_write_data(0x00);
	jd9168_write_data(0x14);
	jd9168_write_data(0x40);
	jd9168_write_data(0x80);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x07);
	jd9168_write_data(0xBB);
	jd9168_write_data(0x00);
	jd9168_write_data(0x14);
	jd9168_write_data(0x07);
	jd9168_write_data(0x5C); 
	jd9168_write_data(0x14);
	jd9168_write_data(0x44);
	jd9168_write_data(0x44);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x02);
	jd9168_write_data(0xBE);
	jd9168_write_data(0x1A);
	jd9168_write_data(0xF2);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x14);
	jd9168_write_data(0xC3);
	jd9168_write_data(0x10);
	jd9168_write_data(0x74);
	jd9168_write_data(0x73);
	jd9168_write_data(0x74);
	jd9168_write_data(0x73);
	jd9168_write_data(0x05);
	jd9168_write_data(0x05);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x15);
	jd9168_write_data(0x15);
	jd9168_write_data(0x31);
	jd9168_write_data(0x85);
	jd9168_write_data(0x17);
	jd9168_write_data(0x05);
	jd9168_write_data(0x6C);
	jd9168_write_data(0x0A);
	jd9168_write_data(0x10);
	jd9168_write_data(0x0A);
	jd9168_write_data(0x10);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x07);
	jd9168_write_data(0xC4);
	jd9168_write_data(0x13);
	jd9168_write_data(0x2C);
	jd9168_write_data(0x00);
	jd9168_write_data(0x13);
	jd9168_write_data(0x10);
	jd9168_write_data(0x07);
	jd9168_write_data(0x14);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x09);
	jd9168_write_data(0xCF);
	jd9168_write_data(0x00);
	jd9168_write_data(0x70);
	jd9168_write_data(0x73);
	jd9168_write_data(0x01);
	jd9168_write_data(0x79);
	jd9168_write_data(0x73);
	jd9168_write_data(0x73);
	jd9168_write_data(0x01);
	jd9168_write_data(0x7C);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x17);
	jd9168_write_data(0xD0);
	jd9168_write_data(0x00);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x03);
	jd9168_write_data(0x01);
	jd9168_write_data(0x0B);
	jd9168_write_data(0x05);
	jd9168_write_data(0x07);
	jd9168_write_data(0x09);
	jd9168_write_data(0x21);
	jd9168_write_data(0x23);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x17);
	jd9168_write_data(0xD1);
	jd9168_write_data(0x00);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x02);
	jd9168_write_data(0x00);
	jd9168_write_data(0x0A);
	jd9168_write_data(0x04);
	jd9168_write_data(0x06);
	jd9168_write_data(0x08);
	jd9168_write_data(0x20);
	jd9168_write_data(0x22);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x17);
	jd9168_write_data(0xD2);
	jd9168_write_data(0x00);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x20);
	jd9168_write_data(0x22);
	jd9168_write_data(0x04);
	jd9168_write_data(0x0A);
	jd9168_write_data(0x08);
	jd9168_write_data(0x06);
	jd9168_write_data(0x02);
	jd9168_write_data(0x00);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x17);
	jd9168_write_data(0xD3);
	jd9168_write_data(0x00);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x21);
	jd9168_write_data(0x23);
	jd9168_write_data(0x05);
	jd9168_write_data(0x0B);
	jd9168_write_data(0x09);
	jd9168_write_data(0x07);
	jd9168_write_data(0x03);
	jd9168_write_data(0x01);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	jd9168_write_data(0x1F);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x25);
	jd9168_write_data(0xD4);
	jd9168_write_data(0x30);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x09);
	jd9168_write_data(0x00);
	jd9168_write_data(0x0B);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x04);
	jd9168_write_data(0x04);
	jd9168_write_data(0x00);
	jd9168_write_data(0x11);
	jd9168_write_data(0x00);
	jd9168_write_data(0x01);
	jd9168_write_data(0xC0);
	jd9168_write_data(0x04);
	jd9168_write_data(0x01);
	jd9168_write_data(0x01);
	jd9168_write_data(0x00);
	jd9168_write_data(0x84);
	jd9168_write_data(0xBF);
	jd9168_write_data(0xC4);
	jd9168_write_data(0xC1);
	jd9168_write_data(0x04);
	jd9168_write_data(0x04);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	jd9168_write_data(0x00);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x08);
	jd9168_write_data(0xD5);
	jd9168_write_data(0x68);
	jd9168_write_data(0x74);
	jd9168_write_data(0x00);
	jd9168_write_data(0x05);
	jd9168_write_data(0x10);
	jd9168_write_data(0x00);
	jd9168_write_data(0x03);
	jd9168_write_data(0x00);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x06);
	jd9168_write_data(0xB7);
	jd9168_write_data(0x10);
	jd9168_write_data(0x0F);//07
	jd9168_write_data(0x00);
	jd9168_write_data(0x10);
	jd9168_write_data(0x0F);//07
	jd9168_write_data(0x00);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x26);
	jd9168_write_data(0xC8);
	jd9168_write_data(0x79);
	jd9168_write_data(0x6B);
	jd9168_write_data(0x61);
	jd9168_write_data(0x57); // 56
	jd9168_write_data(0x57); // 56
	jd9168_write_data(0x4A); // 49
	jd9168_write_data(0x50); // 4f
	jd9168_write_data(0x3B); // 3a
	jd9168_write_data(0x53);
	jd9168_write_data(0x4E); // 52
	jd9168_write_data(0x46); // 51
	jd9168_write_data(0x5E); // 6f
	jd9168_write_data(0x4B); // 5d
	jd9168_write_data(0x56); // 65
	jd9168_write_data(0x4A); // 57
	jd9168_write_data(0x48); // 56
	jd9168_write_data(0x39); // 4c
	jd9168_write_data(0x24); // 40
	jd9168_write_data(0x0F); 
	jd9168_write_data(0x79);
	jd9168_write_data(0x6B);
	jd9168_write_data(0x61);
	jd9168_write_data(0x57); // 56
	jd9168_write_data(0x57); // 56
	jd9168_write_data(0x4A); // 49
	jd9168_write_data(0x50); // 4f
	jd9168_write_data(0x3B); // 3a
	jd9168_write_data(0x53);
	jd9168_write_data(0x4E); // 52
	jd9168_write_data(0x46); // 51
	jd9168_write_data(0x5E); // 6f
	jd9168_write_data(0x4B); // 5d
	jd9168_write_data(0x56); // 65
	jd9168_write_data(0x4A); // 57
	jd9168_write_data(0x48); // 56
	jd9168_write_data(0x39); // 4c
	jd9168_write_data(0x24); // 40
	jd9168_write_data(0x0F);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x01);
	jd9168_write_data(0xDE);
	jd9168_write_data(0x03);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x01);
	jd9168_write_data(0xD5);
	jd9168_write_data(0x20);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x01);
	jd9168_write_data(0xDE);
	jd9168_write_data(0x02);
	LCD_SCS_CLEAR();


	 jd9168_write_data(0xF1);
	 jd9168_write_data(0x01);
	 jd9168_write_data(0x01);
	 jd9168_write_data(0xD2);
	 jd9168_write_data(0x0C);
	 LCD_SCS_CLEAR();


	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x02);
	jd9168_write_data(0xBB);
	jd9168_write_data(0x00);
	jd9168_write_data(0x41);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x01);
	jd9168_write_data(0xE7);
	jd9168_write_data(0x01);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x01);
	jd9168_write_data(0xDE);
	jd9168_write_data(0x00);
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x01);
	jd9168_write_data(0x36);
	jd9168_write_data(0xe0);//0x02
	LCD_SCS_CLEAR();

	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x00);
	jd9168_write_data(0x11);
	LCD_SCS_CLEAR();
	// mdelay (120);
	usleep(120*1000);
	jd9168_write_data(0xF1);
	jd9168_write_data(0x01);
	jd9168_write_data(0x00);
	jd9168_write_data(0x29);
	LCD_SCS_CLEAR();

	lcd_stbyb_normal();
	return 0;
}

static void gpio_spi_set_mosi(unsigned char data)
{
    lcd_gpio_set_output(jd9168dev.spi_mosi_num,(bool)data);
}

static void gpio_spi_generate_clk(void)
{   
    if(jd9168dev.spi_clk_vaild_edge == 1)//1 
		lcd_gpio_set_output(jd9168dev.spi_clk_num,1);
	else
		lcd_gpio_set_output(jd9168dev.spi_clk_num,0);
	usleep(2);
	if(jd9168dev.spi_clk_vaild_edge == 1)//1 
		lcd_gpio_set_output(jd9168dev.spi_clk_num,0);
	else
		lcd_gpio_set_output(jd9168dev.spi_clk_num,1);
}
static void gpio_spi_enable_cs(void)//==0
{
	if(jd9168dev.spi_cs_polar == 0)	
		lcd_gpio_set_output(jd9168dev.spi_cs_num,0);
	else
		lcd_gpio_set_output(jd9168dev.spi_cs_num,1);
}

static void gpio_spi_disable_cs(void)
{
	if(jd9168dev.spi_cs_polar == 0)
		lcd_gpio_set_output(jd9168dev.spi_cs_num,1);//cs
	else
		lcd_gpio_set_output(jd9168dev.spi_cs_num,0);//cs
}

static void gpio_spi_init_clk(void)
{
	if(jd9168dev.spi_clk_vaild_edge == 1)//1 //==1
		lcd_gpio_set_output(jd9168dev.spi_clk_num,0);//sck
	else
		lcd_gpio_set_output(jd9168dev.spi_clk_num,1);//sck
		
}

static void jd9168_write_data(unsigned char cmds)
{
	int i=0;
	unsigned char cmd_val = 0;
	usleep(2);
	for(i=8;i>0;i--){
		cmd_val = (cmds>>(i-1))&0x1;
		gpio_spi_set_mosi(cmd_val);
		usleep(2);
		gpio_spi_generate_clk();
	}
	
}

void LCD_SCS_CLEAR(void)
{
	usleep(2);
	gpio_spi_disable_cs();
	gpio_spi_set_mosi(0);
	usleep(10);

	gpio_spi_disable_cs();//cs
	usleep(10);
	gpio_spi_init_clk();//sck
	gpio_spi_enable_cs();
	
}

static void lcd_stbyb_start(void)
{
	lcd_gpio_set_output(jd9168dev.lcd_stbyb_num,!jd9168dev.lcd_stbyb_polar);
	usleep(200*500);
	lcd_gpio_set_output(jd9168dev.lcd_stbyb_num,jd9168dev.lcd_stbyb_polar);
	usleep(200*500);
}

static void lcd_stbyb_normal(void)
{
	usleep(10*1000);
	lcd_gpio_set_output(jd9168dev.lcd_stbyb_num,!jd9168dev.lcd_stbyb_polar);
}


static struct lcd_map_list jd9168_map = {
	.map = {
		.lcd_init = jd9168_display_init,
		.name = "lcd-jd9168",
	}
};

static int jd9168_probe(const char *node)
{
	int np = fdt_node_probe_by_path(node);

	if(np < 0){
		goto error;
	}

	memset(&jd9168dev,0,sizeof(struct jd9168_dev));

	jd9168dev.spi_clk_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.sck;
	jd9168dev.spi_clk_vaild_edge=1;
	jd9168dev.spi_cs_polar=0;
	jd9168dev.spi_is_9bit=1;
	jd9168dev.spi_cs_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.cs;
	jd9168dev.spi_mosi_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.mosi;
	jd9168dev.spi_miso_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.miso;
	jd9168dev.lcd_stbyb_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.stbyb;
	jd9168dev.lcd_stbyb_polar = 0;

#if 0
	jd9168dev.spi_clk_num=PINPAD_T01;
	jd9168dev.spi_clk_vaild_edge=1;
	jd9168dev.spi_cs_polar=0;
	jd9168dev.spi_is_9bit=1;
	jd9168dev.spi_cs_num=PINPAD_T00;
	jd9168dev.spi_mosi_num=PINPAD_T02;
	jd9168dev.lcd_stbyb_num=PINPAD_T03;
	jd9168dev.lcd_stbyb_polar=0;
#endif
	fdt_get_property_u_32_index(np, "reset", 			0, &jd9168dev.lcd_reset_num);
	fdt_get_property_u_32_index(np, "spi-gpio-sck", 	0, &jd9168dev.spi_clk_num);
	fdt_get_property_u_32_index(np, "spi-gpio-mosi", 	0, &jd9168dev.spi_mosi_num);
	fdt_get_property_u_32_index(np, "spi-gpio-miso", 	0, &jd9168dev.spi_miso_num);
	fdt_get_property_u_32_index(np, "spi-gpio-cs", 		0, &jd9168dev.spi_cs_num);
	fdt_get_property_u_32_index(np, "spi-gpio-stbyb", 	0, &jd9168dev.lcd_stbyb_num);
	log_d("jd9168dev.lcd_stbyb_num = %d %d %d %d\n",jd9168dev.spi_clk_num,jd9168dev.spi_mosi_num,jd9168dev.spi_cs_num,jd9168dev.lcd_stbyb_num);

	int default_off = 0;
	fdt_get_property_u_32_index(np, "default-off", 	0, &default_off);
	if(default_off ==0)
		jd9168_display_init();

	jd9168_map.map.default_off_val = default_off;
	lcd_map_register(&jd9168_map);
error:
	return 0;
}

static int jd9168_init(void)
{
	jd9168_probe("/hcrtos/lcd-jd9168");
	return 0;
}

module_driver(jd9168, jd9168_init, NULL, 2)
