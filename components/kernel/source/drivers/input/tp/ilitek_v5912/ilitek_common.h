#ifndef __ILITEK_COMMON_H
#define __ILITEK_COMMON_H

#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>

#include "ilitek_protocol.h"
#include "ilitek_ts.h"

/* Extern define ------------------------------------------------------------*/
//driver information

#define DRIVER_VERSION_0 5
#define DRIVER_VERSION_1 9
#define DRIVER_VERSION_2 1
#define DRIVER_VERSION_3 2
#define CUSTOMER_H_ID 0
#define CUSTOMER_L_ID 0
#define TEST_VERSION 0

#define ILITEK_ERR_LOG_LEVEL 1
#define ILITEK_INFO_LOG_LEVEL 2
#define ILITEK_DEBUG_LOG_LEVEL 3
#define ILITEK_DEFAULT_LOG_LEVEL 4

#define ILITEK_IOCTL_MAX_TRANSFER 5000

#define debug_level(level, fmt, arg...)                                        \
	do {                                                                   \
		if (level > ilitek_log_level_value)                            \
			break;                                                 \
		if (level == ILITEK_ERR_LOG_LEVEL)                             \
			printk("[ILITEK][ERR][%s:%d] " fmt, __func__,          \
			       __LINE__, ##arg);                               \
		else if (level == ILITEK_INFO_LOG_LEVEL)                       \
			printk("[ILITEK][MSG][%s:%d] " fmt, __func__,          \
			       __LINE__, ##arg);                               \
		else if (level == ILITEK_DEBUG_LOG_LEVEL)                      \
			printk("[ILITEK][DBG][%s:%d] " fmt, __func__,          \
			       __LINE__, ##arg);                               \
	} while (0)

#define tp_err(fmt, arg...) debug_level(ILITEK_ERR_LOG_LEVEL, fmt, ##arg)
#define tp_msg(fmt, arg...) debug_level(ILITEK_INFO_LOG_LEVEL, fmt, ##arg)
#define tp_dbg(fmt, arg...) debug_level(ILITEK_DEBUG_LOG_LEVEL, fmt, ##arg)

#define set_arr(arr, idx, val)                                                 \
	do {                                                                   \
		if ((unsigned int)idx < ARRAY_SIZE(arr))                       \
			arr[idx] = val;                                        \
	} while (0)
enum ilitek_irq_handle_type {
	irq_type_normal = 0,
	irq_type_debug,
	irq_type_c_model,
};

struct ilitek_touch_info {
	uint16_t id;
	uint16_t x;
	uint16_t y;
	uint16_t p;
	uint16_t w;
	uint16_t h;
	uint16_t status;
};

struct ilitek_ts_data {
	uint8_t buf[512];
	int fd;
	int irq_gpio;
	int reset_gpio;
	unsigned int addr;
	const char *i2c_devpath;
	struct work_s work;

	bool system_suspend;
	bool is_touched;
	bool touch_key_hold_press;
	int touch_flag[40];
	struct ilitek_touch_info tp[40];

	struct ilitek_ts_device *dev;
	struct input_dev *input_dev;
	struct input_dev *pen_input_dev;
	uint8_t irq_trigger_type;
	int (*process_and_report)(void);

	struct ilitek_screen_info screen_info;

	unsigned int irq_handle_type;
	unsigned int irq_read_len;

	uint8_t gesture_status;
	uint8_t low_power_status;
};
/* Extern macro -------------------------------------------------------------*/
#define CEIL(n, d) ((n % d) ? (n / d) + 1 : (n / d))
/* Extern variables ---------------------------------------------------------*/

extern int ilitek_log_level_value;

/* input_dev */
#define INPUT_MT_INIT_SLOTS(dev, num)                                          \
	input_mt_init_slots((dev), (num), INPUT_MT_DIRECT)

#endif
