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
#include <hcuapi/pinmux.h>
#include <hcuapi/gpio.h>
#include <kernel/ld.h>


#define OUTPUT_VAL_REG                  0x10

//#define GPIO_HARDCODED
//#defien PINMUX_HARDCODED
#define GPIO_FAST_WRITE
//#define X60_INIT


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
static void st7789v2_write_data(unsigned short data);
static int st7789v2_display_init(void);
static int st7789v2_rorate(lcd_rotate_type_e dir);
static void lcd_reset(void);
static void lcd_write_rgb(unsigned int cmd);


typedef struct st7789v2_dev{
	u32 lcd_cs_num;
    u32 lcd_rs_num;
    u32 lcd_wr_num;
    u32 lcd_rd_num;
    u32 lcd_reset_num;
    u32 lcd_vsync_num;
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
static struct pinmux_setting *control_state;
static struct pinmux_setting *rgb_state;

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

#ifdef PINMUX_HARDCODED
// Extracted from original firmware
void lcd_pinmux_rgb(bool pinmux_rgb) {
    if (pinmux_rgb) {
        REG32_WRITE((void*)0xb8808084,REG32_READ(0xb8808084) & 0xfffffeff);
        REG32_WRITE((void*)0xb8808000,REG32_READ(0xb8808000) & 0xffffff00 | 0x15);
        REG32_WRITE((void*)0xb88004a4,0xb6060606);
        REG32_WRITE((void*)0xb88004a0,REG32_READ(0xb88004a0) & 0xffff | 0x6060000);
        REG32_WRITE((void*)0xb8800508,REG32_READ(0xb8800508) & 0xff | 0x6060600);
        REG32_WRITE((void*)0xb880050c,REG32_READ(0xb880050c) & 0xff000000 | 0x60606);
        REG32_WRITE((void*)0xb8800500,REG32_READ(0xb8800500) & 0xffff | 0x6060000);
        REG32_WRITE((void*)0xb8800504,REG32_READ(0xb8800504) & 0xff000000 | 0x60606);
        REG32_WRITE((void*)0xb88004a8,REG32_READ(0xb88004a8) & 0xff00ffff | 0x60000);
    }
    else {
        REG32_WRITE((void*)0xb88004a4,0x0);
        REG32_WRITE((void*)0xb88004a8,REG32_READ(0xb88004a8) & 0xff00ffff);
        REG32_WRITE((void*)0xb88004a0,REG32_READ(0xb88004a0) & 0xffff);
        REG32_WRITE((void*)0xb8800508,REG32_READ(0xb8800508) & 0xff);
        REG32_WRITE((void*)0xb880050c,REG32_READ(0xb880050c) & 0xff000000);
        REG32_WRITE((void*)0xb8800500,REG32_READ(0xb8800500) & 0xffff);
        REG32_WRITE((void*)0xb8800504,REG32_READ(0xb8800504) & 0xff000000);
    }
}
#else
void lcd_pinmux_rgb(bool pinmux_rgb) {
    if (pinmux_rgb) {
        if (rgb_state) {
            pinmux_select_setting(rgb_state);
        }
    }
    else {
        if (control_state) {
            pinmux_select_setting(control_state);
        }
    }
}
#endif

#ifdef GPIO_HARDCODED
static void lcd_configure_gpio_output(void) {
    gpio_configure(PINPAD_L10,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T01,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_L07,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T00,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_L01,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_L02,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_L03,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_L04,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_L05,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_L06,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T09,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T10,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T11,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T12,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T13,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T14,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T02,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T03,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T04,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T05,GPIO_DIR_OUTPUT);
    gpio_configure(PINPAD_T06,GPIO_DIR_OUTPUT);
}
#else
static void lcd_configure_gpio_output(void) {
    gpio_configure(st7789v2dev.lcd_cs_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_rs_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_wr_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_rd_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d0_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d1_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d2_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d3_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d4_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d5_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d6_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d7_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d8_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d9_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d10_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d11_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d12_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d13_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d14_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_d15_num, GPIO_DIR_OUTPUT);
    gpio_configure(st7789v2dev.lcd_reset_num, GPIO_DIR_OUTPUT);
}
#endif
static void vsync_irq(uint32_t param) {
    lcd_pinmux_rgb(0);
    lcd_configure_gpio_output();
    st7789v2_write_command(0x2A);
    st7789v2_write_data(0x00);
    st7789v2_write_data(0x00);
    st7789v2_write_data(0x01);
    st7789v2_write_data(0x3F);
    st7789v2_write_command(0x2B);
    st7789v2_write_data(0x00);
    st7789v2_write_data(0x00);
    st7789v2_write_data(0x00);
    st7789v2_write_data(0xEF);
    st7789v2_write_command( 0x2C); //memory write
    //*(volatile unsigned *)0xb8808084 &= 0xfffffeff;
    //*(volatile unsigned *)0xb8808000 = *(volatile unsigned *)0xb8808000 & 0xffffff00 | 0x15;
    lcd_pinmux_rgb(1);
}

static int st7789v2_display_init(void)
{
    lcd_pinmux_rgb(0);
    printf("%s %d\n", __FUNCTION__,__LINE__);

#ifdef GPIO_HARDCODED
    gpio_configure(PINPAD_L08,GPIO_DIR_INPUT | GPIO_IRQ_RISING);
#else
    gpio_configure(st7789v2dev.lcd_vsync_num, GPIO_DIR_INPUT | GPIO_IRQ_RISING);
#endif
    lcd_configure_gpio_output();
    lcd_gpio_set_output(st7789v2dev.lcd_cs_num,1);
    lcd_gpio_set_output(st7789v2dev.lcd_rs_num,1);
    lcd_gpio_set_output(st7789v2dev.lcd_wr_num,1);
    lcd_gpio_set_output(st7789v2dev.lcd_rd_num,1);

	printf("%s %d\n", __FUNCTION__,__LINE__);

	//lcd_reset();
    lcd_gpio_set_output(st7789v2dev.lcd_reset_num,1);

    printf("%s %d\n", __FUNCTION__,__LINE__);

	msleep(120);

	st7789v2_write_command( 0x11);// sleep out
	msleep(120);
#ifdef X60_INIT
	st7789v2_write_command(0xcf);
	st7789v2_write_data(0);
	st7789v2_write_data(0xa1);

	st7789v2_write_command(0xb1);
	st7789v2_write_data(0);
	st7789v2_write_data(0x1e);

	st7789v2_write_command(0xb4);
	st7789v2_write_data(2);

	st7789v2_write_command(0xb6);
	st7789v2_write_data(2);

	st7789v2_write_command(0xc0);
	st7789v2_write_data(0x0f);
	st7789v2_write_data(0x0d);

	st7789v2_write_command(0xc1);
	st7789v2_write_data(0);

	st7789v2_write_command(0xc5);
	st7789v2_write_data(0xe7);

	st7789v2_write_command(0xe0);
	st7789v2_write_data(0x05);
	st7789v2_write_data(0x08);
	st7789v2_write_data(0x0d);
	st7789v2_write_data(0x07);
	st7789v2_write_data(0x10);
	st7789v2_write_data(0x08);
	st7789v2_write_data(0x33);
	st7789v2_write_data(0x35);
	st7789v2_write_data(0x45);
	st7789v2_write_data(0x04);
	st7789v2_write_data(0x0b);
	st7789v2_write_data(0x08);
	st7789v2_write_data(0x1a);
	st7789v2_write_data(0x1d);
	st7789v2_write_data(0x0f);

	st7789v2_write_command(0xe1);
	st7789v2_write_data(0x06);
	st7789v2_write_data(0x23);
	st7789v2_write_data(0x26);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x0c);
	st7789v2_write_data(0x01);
	st7789v2_write_data(0x39);
	st7789v2_write_data(0x02);
	st7789v2_write_data(0x4a);
	st7789v2_write_data(0x02);
	st7789v2_write_data(0x0c);
	st7789v2_write_data(0x07);
	st7789v2_write_data(0x31);
	st7789v2_write_data(0x36);
	st7789v2_write_data(0x0f);

	st7789v2_write_command(0x3a);
	st7789v2_write_data(0x55);

	st7789v2_write_command(0x36);
	st7789v2_write_data(0xa8);

	st7789v2_write_command(0x2a);
	st7789v2_write_data(0);
	st7789v2_write_data(0);
	st7789v2_write_data(0x01);
	st7789v2_write_data(0x3F);

	st7789v2_write_command(0x2b);
	st7789v2_write_data(0);
	st7789v2_write_data(0);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0xef);
#else
	st7789v2_write_command( 0x36); //memory data access controll (order setup)
	st7789v2_write_data(0x60);

	st7789v2_write_command( 0x3A);
	st7789v2_write_data(0x55); // 16 bit / 65k of rgb

	st7789v2_write_command(0xB1); // RGB Control
	st7789v2_write_data(0x40);
	st7789v2_write_data(0x04);
	st7789v2_write_data(0x14);

	st7789v2_write_command( 0xB2);  //PORCTRL
	st7789v2_write_data(0x0C);
	st7789v2_write_data(0x0C);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x33);
	st7789v2_write_data(0x33);

	st7789v2_write_command( 0xB7); // Gate Controll
	st7789v2_write_data(0x71);

	st7789v2_write_command( 0xBB); //vcom setting
	st7789v2_write_data(0x3B);

	st7789v2_write_command( 0xC0); //lcom setting
	st7789v2_write_data(0x2C);

	st7789v2_write_command( 0xC2);  //vdv and vrh command enable
	st7789v2_write_data(0x01);

	st7789v2_write_command( 0xC3); //vrh set
	st7789v2_write_data(0x13);//4.5V

	st7789v2_write_command( 0xC4); //vdv set
	st7789v2_write_data(0x20);

	st7789v2_write_command( 0xC6); //framerate control
	st7789v2_write_data(0x0F); //60hz

	st7789v2_write_command( 0xD0); // Power controll
	st7789v2_write_data(0xA4);
	st7789v2_write_data(0xA1);

	st7789v2_write_command( 0xD6); // unknown command
	st7789v2_write_data(0xA1);

	st7789v2_write_command( 0xE0); // positive voltage gama controll
	st7789v2_write_data(0xD0);
	st7789v2_write_data(0x06);
	st7789v2_write_data(0x06);
	st7789v2_write_data(0x0E);
	st7789v2_write_data(0x0D);
	st7789v2_write_data(0x06);
	st7789v2_write_data(0x2F);
	st7789v2_write_data(0x3A);
	st7789v2_write_data(0x47);
	st7789v2_write_data(0x08);
	st7789v2_write_data(0x15);
	st7789v2_write_data(0x14);
	st7789v2_write_data(0x2C);
	st7789v2_write_data(0x33);

	st7789v2_write_command( 0xE1); // negative voltage gama control
	st7789v2_write_data(0xD0);
	st7789v2_write_data(0x06);
	st7789v2_write_data(0x06);
	st7789v2_write_data(0x0E);
	st7789v2_write_data(0x0D);
	st7789v2_write_data(0x06);
	st7789v2_write_data(0x2F);
	st7789v2_write_data(0x3B);
	st7789v2_write_data(0x47);
	st7789v2_write_data(0x08);
	st7789v2_write_data(0x15);
	st7789v2_write_data(0x14);
	st7789v2_write_data(0x2C);
	st7789v2_write_data(0x33);

	st7789v2_write_command(0x2A); //CASET Colum adress Set
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x01);
	st7789v2_write_data(0x3F);

	st7789v2_write_command(0x2B); //RASET // Row adress Set
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0xEF);

	st7789v2_write_command( 0x21); // display inversion on(21) off (20)
#endif

	st7789v2_write_command( 0x29); // display on

	st7789v2_write_command( 0x2C); //Memory Write

    //TestImage
    for(int y = 0; y < 240;y++)
    {
        for(int x = 0; x < 320;x++)
        {
            int r = x;
            int g = y;
            int b = 320-x;
            int data = ((r&0b11111000) << 8)
                    | ((g&0b11111100) << 3)
                    | ((b&0b11111000) >> 3);
            st7789v2_write_data(data); //r565
        }
    }

    usleep(5 * 1000 * 1000); // 5 seconds delay;

    printf("%s %d\n", __FUNCTION__,__LINE__);

    int ret = gpio_irq_request(st7789v2dev.lcd_vsync_num, vsync_irq, (uint32_t)0x0); //param is not needed, but I dont know if I have to pass it.
    if (ret < 0)
        return -1;

    printf("%s %d\n", __FUNCTION__,__LINE__);

    lcd_pinmux_rgb(1);
	return 0;
}

static void gpio_set_cs(bool state)
{
    lcd_gpio_set_output(st7789v2dev.lcd_cs_num,state);
}

static void gpio_set_rs(bool state)
{
    lcd_gpio_set_output(st7789v2dev.lcd_rs_num,state);
}

static void gpio_set_wr(bool state)
{
    lcd_gpio_set_output(st7789v2dev.lcd_wr_num,state);
}

#ifdef GPIO_FAST_WRITE
static void* LREG = (void*)&GPIOLCTRL + OUTPUT_VAL_REG;
static void* TREG = (void*)&GPIOTCTRL + OUTPUT_VAL_REG;

static void lcd_write_data(unsigned short cmd)
{
    REG32_WRITE(LREG,REG32_READ(LREG) & 0xffffff83 | (cmd & 0x1f) << 2);
    REG32_WRITE(TREG,REG32_READ(TREG) & 0xffff8183 | (cmd & 0x7e0) << 4 | cmd >> 9 & 0x7c);
}
#else
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
#endif

static void lcd_gpio_spi_config_write(unsigned char RS, unsigned short cmd)
{
    gpio_set_rs(RS);
    gpio_set_cs(0);

#ifdef GPIO_FAST_WRITE
    lcd_write_data(cmd);
#else
    int i = 0;
    int cmd_val = 0;
    for(i=15;i>=0;i--){
        cmd_val = (cmd>>(i))&0x1;
        gpio_set_data(i, cmd_val);
    }
#endif
    gpio_set_wr(0);
    gpio_set_wr(1);
    gpio_set_cs(1);
    gpio_set_rs(1);
}

static void st7789v2_write_data(unsigned short data)
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
        gpio_set_rs(1);
        gpio_set_wr(1);
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
    printf("%s %d\n", __FUNCTION__,__LINE__);
	if(np < 0){
		goto error;
	}

    control_state = fdt_get_property_pinmux(np, "control");
    rgb_state = fdt_get_property_pinmux(np, "rgb");

    printf("%s %d\n", __FUNCTION__,__LINE__);
	memset(&st7789v2dev,0,sizeof(struct st7789v2_dev));

	st7789v2dev.lcd_cs_num = PINPAD_INVALID;
	st7789v2dev.lcd_rs_num = PINPAD_INVALID;
	st7789v2dev.lcd_wr_num = PINPAD_INVALID;
	st7789v2dev.lcd_rd_num = PINPAD_INVALID;
	st7789v2dev.lcd_reset_num = PINPAD_INVALID;
    st7789v2dev.lcd_vsync_num = PINPAD_INVALID;
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

	fdt_get_property_u_32_index(np, "lcd_cs", 0, &st7789v2dev.lcd_cs_num);
	fdt_get_property_u_32_index(np, "lcd_rs", 0, &st7789v2dev.lcd_rs_num);
	fdt_get_property_u_32_index(np, "lcd_wr", 0, &st7789v2dev.lcd_wr_num);
	fdt_get_property_u_32_index(np, "lcd_rd", 0, &st7789v2dev.lcd_rd_num);
	fdt_get_property_u_32_index(np, "lcd_reset", 0, &st7789v2dev.lcd_reset_num);
    fdt_get_property_u_32_index(np, "lcd_vsync", 0, &st7789v2dev.lcd_vsync_num);
	fdt_get_property_u_32_index(np, "lcd_d0", 0, &st7789v2dev.lcd_d0_num);
	fdt_get_property_u_32_index(np, "lcd_d1", 0, &st7789v2dev.lcd_d1_num);
	fdt_get_property_u_32_index(np, "lcd_d2", 0, &st7789v2dev.lcd_d2_num);
	fdt_get_property_u_32_index(np, "lcd_d3", 0, &st7789v2dev.lcd_d3_num);
	fdt_get_property_u_32_index(np, "lcd_d4", 0, &st7789v2dev.lcd_d4_num);
	fdt_get_property_u_32_index(np, "lcd_d5", 0, &st7789v2dev.lcd_d5_num);
	fdt_get_property_u_32_index(np, "lcd_d6", 0, &st7789v2dev.lcd_d6_num);
	fdt_get_property_u_32_index(np, "lcd_d7", 0, &st7789v2dev.lcd_d7_num);
	fdt_get_property_u_32_index(np, "lcd_d8", 0, &st7789v2dev.lcd_d8_num);
	fdt_get_property_u_32_index(np, "lcd_d9", 0, &st7789v2dev.lcd_d9_num);
	fdt_get_property_u_32_index(np, "lcd_d10", 0, &st7789v2dev.lcd_d10_num);
	fdt_get_property_u_32_index(np, "lcd_d11", 0, &st7789v2dev.lcd_d11_num);
	fdt_get_property_u_32_index(np, "lcd_d12", 0, &st7789v2dev.lcd_d12_num);
	fdt_get_property_u_32_index(np, "lcd_d13", 0, &st7789v2dev.lcd_d13_num);
	fdt_get_property_u_32_index(np, "lcd_d14", 0, &st7789v2dev.lcd_d14_num);
	fdt_get_property_u_32_index(np, "lcd_d15", 0, &st7789v2dev.lcd_d15_num);

    printf("%s %d\n", __FUNCTION__,__LINE__);
	st7789v2_display_init();
    printf("%s %d\n", __FUNCTION__,__LINE__);

	lcd_map_register(&st7789v2_map);
    printf("%s %d\n", __FUNCTION__,__LINE__);
error:
	return 0;
}

static int st7789v2_init(void)
{
	st7789v2_probe("/hcrtos/lcd-st7789v2");
	return 0;
}

module_driver(st7789v2, st7789v2_init, NULL, 2)
