#define LOG_TAG "lcd_nt35510"
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
*	PINPAD_L03 clk
*	PINPAD_L02 mosi
*	PINPAD_L01 cs

	lcd-nt35510{
		spi-gpio-sck	= <PINPAD_L03>;
		spi-gpio-mosi	= <PINPAD_L02>;
		spi-gpio-cs		= <PINPAD_L01>;
		default-off;
		status = "okay";
	};
	lcd{
		lcd-map-name = "lcd-nt35510";
		default-off;
		status = "okay";
	};
*/

#define	W_CMD_H		(0x20)
#define	W_CMD_L		(0X00)
#define W_DATA_CMD 	(0x40)
#define R_DATA_CMD	(0xc0)
#define SET_DWORD(i, d)         (*(volatile unsigned long *)(i)) = (d)
#define GET_DWORD(i)            (*(volatile unsigned long *)(i))
static void nt35510_write_command(unsigned short data);
static void nt35510_write_data(unsigned char data);
static int nt35510_display_init(void);
static void lcd_reset(void);

typedef struct nt35510_dev{
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
}nt35510_dev_t;

static nt35510_dev_t nt35510dev;

static int nt35510_display_init(void)
{
    log_d("lcd init\n");
    //LV2 Page 1 enable
	 //NT35510-CPT40-TRADE
    nt35510_write_command(0x1100);
	//TFT_PIN_SCS_HIGH();
	usleep(1000*300);
    nt35510_write_command(0xF000);	nt35510_write_data(0x55);
    nt35510_write_command(0xF001);	nt35510_write_data(0xAA);
    nt35510_write_command(0xF002);	nt35510_write_data(0x52);
    nt35510_write_command(0xF003);	nt35510_write_data(0x08);
    nt35510_write_command(0xF004);	nt35510_write_data(0x01);
//************* Start Initial Sequence **********//

	nt35510_write_command(0xBC00);	nt35510_write_data(0x00);  //VGMP/VGMN/VOOM Setting, VGMP=5.8V   #VGSP=0V  
	nt35510_write_command(0xBC01);	nt35510_write_data(0xA0);
	nt35510_write_command(0xBC02);	nt35510_write_data(0x00);

	nt35510_write_command(0xBD00);   nt35510_write_data(0x00); //VGMN=-5.8V  #VGSN=0V   
	nt35510_write_command(0xBD01);   nt35510_write_data(0xA0);
	nt35510_write_command(0xBD02);   nt35510_write_data(0x00);

	nt35510_write_command(0xBE00);   nt35510_write_data(0x00); //VCOM= -1V 
	nt35510_write_command(0xBE01);   nt35510_write_data(0x34);

	nt35510_write_command(0xB000);   nt35510_write_data(0x03);  //AVDD, Set AVDD 6.2V
	nt35510_write_command(0xB001);   nt35510_write_data(0x03);
	nt35510_write_command(0xB002);   nt35510_write_data(0x03);

	nt35510_write_command(0xB100);   nt35510_write_data(0x00); //AVEE voltage, Set AVEE -6.5V  
	nt35510_write_command(0xB101);   nt35510_write_data(0x00);
	nt35510_write_command(0xB102);   nt35510_write_data(0x00);

	nt35510_write_command(0xB900);   nt35510_write_data(0x24);   //VGH, Set VGH 18V  
	nt35510_write_command(0xB901);   nt35510_write_data(0x24);
	nt35510_write_command(0xB902);   nt35510_write_data(0x24);

	nt35510_write_command(0xBA00);   nt35510_write_data(0x14); //VGLX, Set VGL -12V   
	nt35510_write_command(0xBA01);   nt35510_write_data(0x14);
	nt35510_write_command(0xBA02);   nt35510_write_data(0x14);
// ********************  EABLE PAGE 0 **************//
	nt35510_write_command(0xF000);	nt35510_write_data(0x55);
	nt35510_write_command(0xF001);	nt35510_write_data(0xAA);     
	nt35510_write_command(0xF002);	nt35510_write_data(0x52);
	nt35510_write_command(0xF003);	nt35510_write_data(0x08);
	nt35510_write_command(0xF004);	nt35510_write_data(0x00);

	nt35510_write_command(0xB400);   nt35510_write_data(0x10); //Vivid Color
	 
	nt35510_write_command(0xB600);   nt35510_write_data(0x02);//SDT  

	nt35510_write_command(0xB700);   nt35510_write_data(0x70);//Set Gate EQ   
	nt35510_write_command(0xB701);   nt35510_write_data(0x70);

	nt35510_write_command(0xB800);   nt35510_write_data(0x01); //Set Source EQ   
	nt35510_write_command(0xB801);   nt35510_write_data(0x07);
	nt35510_write_command(0xB802);   nt35510_write_data(0x07);
	nt35510_write_command(0xB803);   nt35510_write_data(0x07);

	nt35510_write_command(0xBC00);   nt35510_write_data(0x05);//Inversion Control     
	nt35510_write_command(0xBC01);   nt35510_write_data(0x05);
	nt35510_write_command(0xBC02);   nt35510_write_data(0x05);

	nt35510_write_command(0xBD00);   nt35510_write_data(0x01); //Porch Adjust  
	nt35510_write_command(0xBD01);   nt35510_write_data(0x84);
	nt35510_write_command(0xBD02);   nt35510_write_data(0x07);
	nt35510_write_command(0xBD03);   nt35510_write_data(0x31);
	nt35510_write_command(0xBD04);   nt35510_write_data(0x00);

	nt35510_write_command(0xBE00);   nt35510_write_data(0x01);   
	nt35510_write_command(0xBE01);   nt35510_write_data(0x84);
	nt35510_write_command(0xBE02);   nt35510_write_data(0x07);
	nt35510_write_command(0xBE03);   nt35510_write_data(0x31);
	nt35510_write_command(0xBE04);   nt35510_write_data(0x00);

	nt35510_write_command(0xBF00);   nt35510_write_data(0x01);  
	nt35510_write_command(0xBF01);   nt35510_write_data(0x84);
	nt35510_write_command(0xBF02);   nt35510_write_data(0x07);
	nt35510_write_command(0xBF03);   nt35510_write_data(0x31);
	nt35510_write_command(0xBF04);   nt35510_write_data(0x00);
//******************* ENABLE CMD2 PAGE0 **************//  
	nt35510_write_command(0xF000);	nt35510_write_data(0x55);
	nt35510_write_command(0xF001);	nt35510_write_data(0xAA);     
	nt35510_write_command(0xF002);	nt35510_write_data(0x52);
	nt35510_write_command(0xF003);	nt35510_write_data(0x08);
	nt35510_write_command(0xF004);	nt35510_write_data(0x00);
	nt35510_write_command(0xC700);   nt35510_write_data(0x02);   
	nt35510_write_command(0xC900);   nt35510_write_data(0x11);   
	nt35510_write_command(0xC901);   nt35510_write_data(0x00);
	nt35510_write_command(0xC902);   nt35510_write_data(0x00);
	nt35510_write_command(0xC903);   nt35510_write_data(0x00);
	nt35510_write_command(0x3A00);   nt35510_write_data(0x77); //Data format 16-Bits
	nt35510_write_command(0x3500);   nt35510_write_data(0x00); //TE ON 
	nt35510_write_command(0x2100);   nt35510_write_data(0x00);
	nt35510_write_command(0x3600);   nt35510_write_data(0x00);
	nt35510_write_command(0x1100);   nt35510_write_data(0x00);//StartUp  
	usleep(120*1000);
	nt35510_write_command(0x2900);   nt35510_write_data(0x00); //Display On 
	usleep(120*1000);
}

static void gpio_spi_set_mosi(unsigned char data)
{
    lcd_gpio_set_output(nt35510dev.spi_mosi_num,(bool)data);
}

static void gpio_spi_generate_clk(void)
{   
    if(nt35510dev.spi_clk_vaild_edge == 1)//1 
		lcd_gpio_set_output(nt35510dev.spi_clk_num,1);
	else
		lcd_gpio_set_output(nt35510dev.spi_clk_num,0);
	usleep(2);
	if(nt35510dev.spi_clk_vaild_edge == 1)//1 
		lcd_gpio_set_output(nt35510dev.spi_clk_num,0);
	else
		lcd_gpio_set_output(nt35510dev.spi_clk_num,1);
}
static void gpio_spi_enable_cs(void)//==0
{
	if(nt35510dev.spi_cs_polar == 0)	
		lcd_gpio_set_output(nt35510dev.spi_cs_num,0);
	else
		lcd_gpio_set_output(nt35510dev.spi_cs_num,1);
	// printf("%s %d val = %08lx\n",__FUNCTION__,__LINE__,GET_DWORD(0xb8800054));
}

static void gpio_spi_disable_cs(void)
{
	if(nt35510dev.spi_cs_polar == 0)
		lcd_gpio_set_output(nt35510dev.spi_cs_num,1);//cs
	else
		lcd_gpio_set_output(nt35510dev.spi_cs_num,0);//cs
	// printf("%s %d val = %08lx\n",__FUNCTION__,__LINE__,GET_DWORD(0xb8800054));
}

static void gpio_spi_init_clk(void)
{
	if(nt35510dev.spi_clk_vaild_edge == 1)//1 //==1
		lcd_gpio_set_output(nt35510dev.spi_clk_num,0);//sck
	else
		lcd_gpio_set_output(nt35510dev.spi_clk_num,1);//sck
		
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
	if(nt35510dev.spi_is_9bit == 1)
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

static unsigned char lcd_gpio_spi_config_read(unsigned char bit_9,unsigned char cmd,unsigned char len)
{
	int i=0;
	unsigned char cmd_val = 0;
	unsigned char get_reg_val = 0;
	unsigned char len_t = len;
	gpio_spi_disable_cs();//cs
	usleep(10);
	gpio_spi_init_clk();//sck
	gpio_spi_enable_cs();
	usleep(2);
	if(nt35510dev.spi_is_9bit == 1)
	{
		gpio_spi_set_mosi(bit_9);//sda dat=0
		usleep(3);
		gpio_spi_generate_clk();
	}
	gpio_configure(nt35510dev.spi_mosi_num, GPIO_DIR_OUTPUT);//sda
	for(i=8;i>0;i--){
		cmd_val = (cmd>>(i-1))&0x1;
		gpio_spi_set_mosi(cmd_val);
		usleep(2);
		gpio_spi_generate_clk();
	}
	usleep(2);
	gpio_spi_set_mosi(0);
	while(len_t--)
	{
		if(nt35510dev.spi_mode == 0)
			gpio_configure(nt35510dev.spi_mosi_num, GPIO_DIR_INPUT);//sda
		for(i=8;i>0;i--)
		{
			if(nt35510dev.spi_mode == 0)
				cmd_val = gpio_get_input(nt35510dev.spi_mosi_num);
			get_reg_val |= cmd_val<<(i-1);
			gpio_spi_generate_clk();
			usleep(2);
		}
		if(nt35510dev.spi_mode == 0)
			gpio_configure(nt35510dev.spi_mosi_num, GPIO_DIR_OUTPUT);//sda
		gpio_spi_disable_cs();
	}
	gpio_spi_set_mosi(0);
	usleep(10);
	return get_reg_val;
}

static void lcd_gpio_spi_config_write_16bit(unsigned short cmd)
{
	int i=0;
	unsigned char cmd_val = 0;
	gpio_spi_disable_cs();//cs
	usleep(10);
	gpio_spi_init_clk();//sck
	gpio_spi_enable_cs();
	usleep(2);

	for(i=16;i>0;i--){
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

static void lcd_gpio_spi_config_send_cmds(unsigned char cmds)
{
	lcd_gpio_spi_config_write(0,cmds);
}

static void lcd_gpio_spi_config_send_data(unsigned char data)
{
	lcd_gpio_spi_config_write(1,data);
}

static void lcd_spi_send_cmds(unsigned char cmds)
{
	lcd_gpio_spi_config_write(0,cmds);
}

static void lcd_reset(void)
{
	if(nt35510dev.lcd_reset_num!=0xff)
	{
		lcd_gpio_set_output(nt35510dev.lcd_reset_num,1);
		usleep(500*1000);
		lcd_gpio_set_output(nt35510dev.lcd_reset_num,0);
		usleep(500*1000);
		lcd_gpio_set_output(nt35510dev.lcd_reset_num,1);
		usleep(500*1000);
	}
}
static unsigned char lcd_gpio_spi_config_read_16bit(unsigned char cmd)
{
	
	int i=0;
	unsigned char cmd_val = 0;
	unsigned char get_reg_val = 0;
	gpio_spi_disable_cs();//cs
	usleep(10);
	gpio_spi_init_clk();//sck
	gpio_spi_enable_cs();
	usleep(2);
	gpio_configure(nt35510dev.spi_mosi_num, GPIO_DIR_OUTPUT);//sda
	for(i=8;i>0;i--){
		cmd_val = (cmd>>(i-1))&0x1;
		gpio_spi_set_mosi(cmd_val);
		usleep(2);
		gpio_spi_generate_clk();
	}
	usleep(2);
	gpio_spi_set_mosi(0);
	if(nt35510dev.spi_mode == 0)
		gpio_configure(nt35510dev.spi_mosi_num, GPIO_DIR_INPUT);//sda
	for(i=8;i>0;i--)
	{
		if(nt35510dev.spi_mode == 0)
			cmd_val = gpio_get_input(nt35510dev.spi_mosi_num);
		else
			cmd_val = gpio_get_input(nt35510dev.spi_miso_num);
		get_reg_val |= cmd_val<<(i-1);
		gpio_spi_generate_clk();
		usleep(2);
	}
	gpio_spi_disable_cs();
	gpio_spi_set_mosi(0);
	usleep(10);
	return get_reg_val;
}

static void nt35510_write_command(unsigned short data)
{
	unsigned short send_data = 0;
	unsigned char command_data = 0;
    command_data = (data>>8)&0xff;
    send_data = command_data|(W_CMD_H<<8);
    lcd_gpio_spi_config_write_16bit(send_data);
    command_data= data&0xff;
    send_data = command_data|(W_CMD_L<<8);
    lcd_gpio_spi_config_write_16bit(send_data);
}

static void nt35510_write_data(unsigned char data)
{
	unsigned short send_data = 0;
	unsigned char command_data = 0;
    command_data = (data)&0xff;
    send_data = command_data|(W_DATA_CMD<<8);
    lcd_gpio_spi_config_write_16bit(send_data);
}

static struct lcd_map_list nt35510_map = {
	.map = {
		.lcd_init = nt35510_display_init,
		.name = "lcd-nt35510",
	}
};

static int nt35510_probe(const char *node)
{
	int np = fdt_node_probe_by_path(node);

	if(np < 0){
		goto error;
	}

	memset(&nt35510dev,0,sizeof(struct nt35510_dev));

	nt35510dev.spi_clk_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.sck;
	nt35510dev.spi_clk_vaild_edge=1;
	nt35510dev.spi_cs_polar=0;
	nt35510dev.spi_is_9bit=1;
	nt35510dev.spi_cs_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.cs;
	nt35510dev.spi_mosi_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.mosi;
	nt35510dev.spi_miso_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.miso;
	nt35510dev.lcd_stbyb_num = PINPAD_INVALID;//lcddrv->gpio_spi_config.stbyb;
	nt35510dev.lcd_stbyb_polar = 0;
	nt35510dev.lcd_reset_num = PINPAD_INVALID;
#if 0
// gpio_configure(PINPAD_T23,GPIO_DIR_OUTPUT);//reset
	nt35510dev.spi_clk_num=PINPAD_L03;
	nt35510dev.spi_clk_vaild_edge=1;
	nt35510dev.spi_cs_polar=0;
	nt35510dev.spi_is_9bit=1;
	nt35510dev.spi_cs_num=PINPAD_L01;
	nt35510dev.spi_mosi_num=PINPAD_L02;
	nt35510dev.lcd_reset_num=PINPAD_INVALID;
	nt35510dev.lcd_reset_polar=0;
#endif
	fdt_get_property_u_32_index(np, "reset", 			0, &nt35510dev.lcd_reset_num);
	fdt_get_property_u_32_index(np, "spi-gpio-sck", 	0, &nt35510dev.spi_clk_num);
	fdt_get_property_u_32_index(np, "spi-gpio-mosi", 	0, &nt35510dev.spi_mosi_num);
	fdt_get_property_u_32_index(np, "spi-gpio-miso", 	0, &nt35510dev.spi_miso_num);
	fdt_get_property_u_32_index(np, "spi-gpio-cs", 		0, &nt35510dev.spi_cs_num);
	fdt_get_property_u_32_index(np, "spi-gpio-stbyb", 	0, &nt35510dev.lcd_stbyb_num);
	log_d("nt35510dev.lcd_stbyb_num = %d %d %d %d\n",nt35510dev.spi_clk_num,nt35510dev.spi_mosi_num,nt35510dev.spi_cs_num,nt35510dev.lcd_stbyb_num);

	int default_off = 0;
	fdt_get_property_u_32_index(np, "default-off", 	0, &default_off);
	if(default_off ==0)
		nt35510_display_init();

	nt35510_map.map.default_off_val = default_off;
	lcd_map_register(&nt35510_map);
error:
	return 0;
}

static int nt35510_init(void)
{
	nt35510_probe("/hcrtos/lcd-nt35510");
	return 0;
}

module_driver(nt35510, nt35510_init, NULL, 2)

