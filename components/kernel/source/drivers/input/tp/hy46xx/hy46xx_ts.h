#ifndef __HY46XX_TS_H
#define __HY46XX_TS_H

#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/timer.h>

#define HY46XX_NAME	"hy46xx_ts"
#define CFG_MAX_TOUCH_POINTS	5
#define RESOLUTION_X			800
#define RESOLUTION_Y			1280

#define HY_PRESS				0x08
#define HY_MAX_ID				0x0F
#define HY_TOUCH_STEP			6
#define HY_TOUCH_X_H_POS		3
#define HY_TOUCH_X_L_POS		4
#define HY_TOUCH_Y_H_POS		5
#define HY_TOUCH_Y_L_POS		6
#define HY_TOUCH_EVENT_POS		3
#define HY_TOUCH_ID_POS			5

#define POINT_READ_BUF			(3 + HY_TOUCH_STEP * CFG_MAX_TOUCH_POINTS)

#define HY46XX_REG_FW_VER		0xA6
#define HY46XX_REG_WORK_MODE	0xA5

struct hy46xx_platform_data {
	unsigned int x_max;
	unsigned int y_max;
	unsigned long irqflags;	/*default:IRQF_TRIGGER_FALLING*/
};

struct ts_event {
	unsigned short au16_x[CFG_MAX_TOUCH_POINTS];	/*x coordinate */
	unsigned short au16_y[CFG_MAX_TOUCH_POINTS];	/*y coordinate */
	unsigned char au8_touch_event[CFG_MAX_TOUCH_POINTS];	/*touch event:
					0 -- down; 1-- contact; 2 -- contact */
	unsigned char au8_finger_id[CFG_MAX_TOUCH_POINTS];	/*touch ID */
	unsigned short pressure;
	unsigned char touch_point;
};

struct hy46xx_ts_data {
	unsigned int irq;
	unsigned int x_max;
	unsigned int y_max;
//	struct i2c_client *client;
//	struct input_dev *input_dev;
	struct ts_event event;
	struct hy46xx_platform_data *pdata;
	unsigned int reset;
	const char *i2c_devpath;
	struct input_dev		*input_dev;
	unsigned int addr;
	struct work_s           work;
};

#endif
