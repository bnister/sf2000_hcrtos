#ifndef __UAPI_LCD_H_H
#define __UAPI_LCD_H_H
#include <hcuapi/iocbase.h>

#define LCD_IOCBASE								(0x100)
#define LCD_INIT_ALL							_IO (LCD_IOCBASE, 1)
#define LCD_DRIVER_INIT							_IO (LCD_IOCBASE, 2)
#define LCD_DRIVER_GET_INIT_STATUS				_IOR (LCD_IOCBASE, 3, enum LCD_CURRENT_INIT_STATUS)
#define LCD_SET_ROTATE							_IO (LCD_IOCBASE, 4)	//!< param: lcd_rotate_type_e
#define LCD_SEND_DATE							_IOW (LCD_IOCBASE, 5,struct hc_lcd_master_dev)
#define LCD_SEND_CMDS							_IOW (LCD_IOCBASE, 6,struct hc_lcd_master_dev)
#define LCD_READ_DATE							_IOWR (LCD_IOCBASE, 7,struct hc_lcd_spi_read)
#define LCD_SET_ONOFF							_IO (LCD_IOCBASE, 8)	//!< param: value: 0 1
#define LCD_SET_PWM_VCOM						_IO (LCD_IOCBASE, 9)	//!< param: Duty cycle: 0~100 0xFFFFFFFF
#define LCD_SET_GPIO_POWER						_IO (LCD_IOCBASE, 10)  	//!< param: value: 0 1
#define LCD_SET_GPIO_RESET						_IO (LCD_IOCBASE, 11)  	//!< param: value: 0 1

#define LCD_GPIO_ENABLE			1
#define LCD_GPIO_DISABLE		0

#define LCD_MAX_DATA_SIZE		32
#define LCD_MAX_COMMAND_SIZE	32
#define LCD_MAX_RECEIVED_SIZE	32

struct hc_lcd_master_dev {
	unsigned int count;
	unsigned int packet[LCD_MAX_DATA_SIZE];
};

struct hc_lcd_spi_read {
	unsigned int command_size;
	unsigned int command_data[LCD_MAX_COMMAND_SIZE];
	unsigned int received_size;
	unsigned int received_data[LCD_MAX_RECEIVED_SIZE];
};

typedef enum LCD_ROTATE_TYPE
{
	LCD_ROTATE_0=0,
	LCD_ROTATE_180,
	LCD_H_MIRROR,
	LCD_V_MIRROR,
}lcd_rotate_type_e;

typedef enum LCD_CURRENT_INIT_STATUS
{
	LCD_CURRENT_NOT_INIT,
	LCD_CURRENT_IS_INIT,
}lcd_current_init_status_e;

#endif

