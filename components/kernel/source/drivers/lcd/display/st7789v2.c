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

void lcd_set_gpio_output() {
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

static void vsync_irq(uint32_t param) {
    lcd_pinmux_rgb(0);
    lcd_set_gpio_output();
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
    st7789v2_write_command( 0x2C); //memory write
    lcd_pinmux_rgb(1);
}

static int st7789v2_display_init(void)
{
    lcd_pinmux_rgb(0);
    printf("%s %d\n", __FUNCTION__,__LINE__);


    gpio_configure(PINPAD_L08,GPIO_DIR_INPUT | GPIO_IRQ_RISING);
    lcd_set_gpio_output();

	printf("%s %d\n", __FUNCTION__,__LINE__);

	lcd_reset();

    printf("%s %d\n", __FUNCTION__,__LINE__);

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
	//driver default
	#if 0
    printf("%s %d\n", __FUNCTION__,__LINE__);
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

	// sf2000 original init 10xx are commands, 20xx are delays, 00xx are data
	/*
	 * 1011h,  2063h,  1036h,    60h,
103Ah,    55h,  10B1h,    40h,
   4h,    14h,  10B2h,     Ch,
   Ch,     0h,    33h,    33h,
10B7h,    71h,  10BBh,    3Bh,
10C0h,    2Ch,  10C2h,     1h,
10C3h,    13h,  10C4h,    20h,
10C6h,     Fh,  10D0h,    A4h,
  A1h,  10D6h,    A1h,  10E0h,
  D0h,     6h,     6h,     Eh,
   Dh,     6h,    2Fh,    3Ah,
  47h,     8h,    15h,    14h,
  2Ch,    33h,  10E1h,    D0h,
   6h,     6h,     Eh,     Dh,
   6h,    2Fh,    3Bh,    47h,
   8h,    15h,    14h,    2Ch,
  33h,  102Ah,     0h,     0h,
   1h,    3Fh,  102Bh,     0h,
   0h,     0h,    EFh,  1021h,
1029h,  FFFFh
	 */
#if 1
	//1011h // sleep out
	st7789v2_write_command( 0x11);
	//2063h
	msleep(120);

	// 1036h //memory data access controll (order setup)
	st7789v2_write_command( 0x36);
	// 60h
	st7789v2_write_data(0x60);

	//103Ah // interface pizel format
	st7789v2_write_command( 0x3A);
	//55h
	st7789v2_write_data(0x55); // 16 bit / 65k of rgb

	//10B1h
	st7789v2_write_command(0xB1); // RGB Control
	//40h,	4h,    14h,
	st7789v2_write_data(0x40);
	st7789v2_write_data(0x04);
	st7789v2_write_data(0x14);

	//10B2h
	st7789v2_write_command( 0xB2);  //PORCTRL
	//Ch,	 Ch,     0h,    33h,    33h, //Default Value
	st7789v2_write_data(0x0C);
	st7789v2_write_data(0x0C);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x33);
	st7789v2_write_data(0x33);

	//10B7h
	st7789v2_write_command( 0xB7); // Gate Controll
	//71h
	st7789v2_write_data(0x71);

	//10BBh
	st7789v2_write_command( 0xBB); //vcom setting
	//3Bh
	st7789v2_write_data(0x3B);

	//10C0h
	st7789v2_write_command( 0xC0); //lcom setting
	//2Ch
	st7789v2_write_data(0x2C);

	//10C2h
	st7789v2_write_command( 0xC2);  //vdv and vrh command enable
	//1h
	st7789v2_write_data(0x01);

	//10C3h
	st7789v2_write_command( 0xC3); //vrh set
	//13h
	st7789v2_write_data(0x13);//4.5V

	//10C4h
	st7789v2_write_command( 0xC4); //vdv set
	//20h
	st7789v2_write_data(0x20);

	//10C6h
	st7789v2_write_command( 0xC6); //framerate control
	//Fh
	st7789v2_write_data(0x0F); //60hz

	//10D0h,
	st7789v2_write_command( 0xD0); // Power controll
	//A4h,A1h,
	st7789v2_write_data(0xA4);
	st7789v2_write_data(0xA1);

	//10D6h,
	st7789v2_write_command( 0xD6); // unknown command
	//A1h
	st7789v2_write_data(0xA1);

	//10E0h,
	st7789v2_write_command( 0xE0); // positive voltage gama controll
	//  D0h,     6h,     6h,     Eh,
	//   Dh,     6h,    2Fh,    3Ah,
	//  47h,     8h,    15h,    14h,
	//  2Ch,    33h,
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

	//10E1h,
	st7789v2_write_command( 0xE1); // negative voltage gama control
	//  D0h,
	//   6h,     6h,     Eh,     Dh,
	//   6h,    2Fh,    3Bh,    47h,
	//   8h,    15h,    14h,    2Ch,
	//  33h,
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

	//102Ah,
	st7789v2_write_command(0x2A); //CASET Colum adress Set
	// 0h,     0h,
	//   1h,    3Fh,
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x01);
	st7789v2_write_data(0x3F);

	//102Bh,
	st7789v2_write_command(0x2B); //RASET // Row adress Set
	// 0h,
	//   0h,     0h,    EFh,
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0x00);
	st7789v2_write_data(0xEF);

	//1021h,
	st7789v2_write_command( 0x20); // display inversion on(21) off (20)

	//1029h
	st7789v2_write_command( 0x29); // display on

	//Memory Write
	st7789v2_write_command( 0x2C);

    //TestImage
    for(int x = 0; x < 320;x++)
    {
        for(int y = 0; x < 240;x++)
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

    usleep(2 * 1000 * 1000); // 2 seconds delay;
#endif

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

static void* LREG = (void*)&GPIOLCTRL + OUTPUT_VAL_REG;
static void* TREG = (void*)&GPIOTCTRL + OUTPUT_VAL_REG;

static void lcd_write_data(unsigned short cmd)
{
    REG32_WRITE(LREG,REG32_READ(LREG) & 0xffffff83 | (cmd & 0x1f) << 2);
    REG32_WRITE(TREG,REG32_READ(TREG) & 0xffff8183 | (cmd & 0x7e0) << 4 | cmd >> 9 & 0x7c);
}

static void lcd_gpio_spi_config_write(unsigned char RS, unsigned short cmd)
{
    gpio_set_rs(RS);
    gpio_set_cs(0);
    lcd_write_data(cmd);
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
        gpio_set_wr(0);
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
