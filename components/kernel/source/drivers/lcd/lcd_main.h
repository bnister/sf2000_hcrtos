#ifndef _LCD_MAIN_H_
#define _LCD_MAIN_H_
#include <linux/kconfig.h>
#include <kernel/list.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include "lcd.h"

// #include <hcuapi/lcd.h>
#define LCD_RET_SUCCESS			0
#define LCD_RET_ERROR			(-1)
#define LCD_PWM_SET_DEFAULT		0xFFFFFFFF

struct lcd_map
{
	char *name;
	int (*lcd_reset)(void);
	int (*lcd_power_onoff)(u32 val);
	int (*lcd_read_data)(u32 cmd);
	int (*lcd_write_cmds)(u32 *cmd,u32 len);
	int (*lcd_write_data)(u32 *data,u32 len);
	int (*lcd_rorate)(lcd_rotate_type_e direct);
	int (*lcd_onoff)(u32 onoff);
	int (*lcd_init)(void);
	int default_off_val;
};

struct lcd_map_list {
	struct list_head	 list;
	struct lcd_map map;
};

struct lcd_map *lcd_map_get(const char *name);

int lcd_map_register(struct lcd_map_list *map);

void lcd_gpio_set_output(uint8_t padctl, bool value);
#endif
