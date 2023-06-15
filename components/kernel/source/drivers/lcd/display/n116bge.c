#define LOG_TAG "lcd_n116bge"
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
#include <hcuapi/i2c-master.h>
#include <kernel/delay.h>
#include "../lcd_main.h"
/*
	gpio-i2c@0 {
		sda-pinmux = <PINPAD_L28>;
		scl-pinmux = <PINPAD_L29>;
        status = "okay";
		simulate;
	};
	lcd-n116bge
	{
		i2c-devpath = "/dev/gpio-i2c0";
		rstb = <PINPAD_L21>;
		pwd = <PINPAD_L20>;
		enbaled = <PINPAD_B02>;
		status = "okay";
	};
	lcd{
		lcd-map-name = "lcd-n116bge";
		status = "okay";
	};
*/

struct n116bge_dev
{
	// const char *name;
	const char *i2c_devpath;
	unsigned int rstb;
	unsigned int pwd;
	unsigned int enbaled;
	int default_off;
};

static struct n116bge_dev n116bgedev;

static uint32_t edp_in_i2c_write(uint8_t i2c_id,uint8_t cmd,uint8_t data)
{
	uint8_t array[2];
	int ret = 0;
	array[0] = cmd;
	array[1] = data;

	static int fd = 0;
	if(fd == 0)
		fd = open(n116bgedev.i2c_devpath,O_RDWR);

	if(fd > 0)
	{
		
		struct i2c_transfer_s xfer_write;
		struct i2c_msg_s i2c_msg_write;

		i2c_msg_write.addr = (i2c_id >> 1);
		i2c_msg_write.flags = 0x0;
		i2c_msg_write.buffer = array;
		i2c_msg_write.length = 2;

		xfer_write.msgv = &i2c_msg_write;
		xfer_write.msgc = 1;

		ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_write);
		if (ret < 0)
			log_e("%s:i2c read error.\n", __func__);
		// printf("I2c open ok\n");
		// close(fd);
	}
	
}

static int n116bge_display_init(void)
{
	gpio_configure(n116bgedev.rstb,GPIO_DIR_OUTPUT);//
	gpio_set_output(n116bgedev.rstb,false);
	//HAL_GPIO_BIT_SET(NCS8801_STB,0);
	msleep(1);
	//HAL_GPIO_BIT_SET(NCS8801_PWR,0);
	gpio_configure(n116bgedev.pwd,GPIO_DIR_OUTPUT);//
	gpio_set_output(n116bgedev.pwd,false);
	msleep(1);
	printf("%s %d OK\n",__func__,__LINE__);
   	//HAL_GPIO_BIT_SET(PIN_MUX_L21,1);
	gpio_configure(n116bgedev.rstb,GPIO_DIR_OUTPUT);//
	gpio_set_output(n116bgedev.rstb,true);

	edp_in_i2c_write(0xe0,0x0f,0x01);  ///reset enable 
	edp_in_i2c_write(0xe0,0x00,0x04);  ///rgb input 
	edp_in_i2c_write(0xe0,0x02,0x07);  ///tx rx enable 
	edp_in_i2c_write(0xe0,0x06,0x04);  //0x04 
	edp_in_i2c_write(0xe0,0x07,0xc1);  //0xc2 
	edp_in_i2c_write(0xe0,0x09,0x01);  ///edp tx port number 
	edp_in_i2c_write(0xe0,0x0b,0x00);  ///b0:1:bist enable 
	edp_in_i2c_write(0xe0,0x60,0x00); 
	edp_in_i2c_write(0xe0,0x70,0x00); 
	edp_in_i2c_write(0xe0,0x71,0x09);  //{0xe0,0x71,0x01}0x09 
	edp_in_i2c_write(0xe0,0x73,0x80);  //0x80 
	edp_in_i2c_write(0xe0,0x74,0x20); 
	edp_in_i2c_write(0xea,0x00,0xb0); 
	edp_in_i2c_write(0xea,0x84,0x10); 
	edp_in_i2c_write(0xea,0x85,0x32); 
	edp_in_i2c_write(0xea,0x01,0x00); 
	edp_in_i2c_write(0xea,0x02,0x5c); 
	edp_in_i2c_write(0xea,0x0b,0x47); 
	edp_in_i2c_write(0xea,0x0e,0x06); 
	edp_in_i2c_write(0xea,0x0f,0x06); 
	edp_in_i2c_write(0xea,0x11,0x88); 
	edp_in_i2c_write(0xea,0x22,0x00); 
	edp_in_i2c_write(0xea,0x00,0xb1); 
	edp_in_i2c_write(0xe0,0x0f,0x00);

	gpio_configure(n116bgedev.enbaled,GPIO_DIR_OUTPUT);//
	gpio_set_output(n116bgedev.enbaled,true);
	return LCD_RET_SUCCESS;
}

static struct lcd_map_list n116bge_map = {
	.map = {
		.lcd_init = n116bge_display_init,
		.name     = "lcd-n116bge",
	}
};

static int n116bge_probe(const char *node)
{
	int np = fdt_node_probe_by_path(node);

	if(np < 0){
		goto error;
	}

	memset(&n116bgedev,0,sizeof(struct n116bge_dev));

	// fdt_get_property_string_index(np, "name", 0, &n116bgedev.name);
	// if(n116bgedev.name == NULL)
	// {
	// 	log_e("n116bgedev.name == NULL\n");
	// 	goto error;
	// }
	fdt_get_property_string_index(np, "i2c-devpath", 0, &n116bgedev.i2c_devpath);
	if(n116bgedev.i2c_devpath == NULL)
	{
		log_e("n116bgedev.i2c_devpath == NULL\n");
		goto error;
	}

	n116bgedev.rstb = PINPAD_INVALID;
	n116bgedev.pwd = PINPAD_INVALID;
	n116bgedev.enbaled = PINPAD_INVALID;
	fdt_get_property_u_32_index(np, "rstb", 0, &n116bgedev.rstb);
	fdt_get_property_u_32_index(np, "pwd", 	0, &n116bgedev.pwd);
	fdt_get_property_u_32_index(np, "enbaled", 	0, &n116bgedev.enbaled);
	printf("%s %d rstb = %d pwd = %d enbaled =%d n116bgedev.i2c_devpath = %s\n",__func__,__LINE__,n116bgedev.rstb,n116bgedev.pwd,n116bgedev.enbaled,n116bgedev.i2c_devpath);

	int default_off = 0;
	fdt_get_property_u_32_index(np, "default-off", 	0, &default_off);
	if(default_off ==0)
		n116bge_display_init();

	n116bge_map.map.default_off_val = default_off;
	lcd_map_register(&n116bge_map);
error:
	return 0;	
}

static int n116bge_init(void)
{
	n116bge_probe("/hcrtos/lcd-n116bge");
	return 0;
}

module_driver(n116bge, n116bge_init, NULL, 2)
