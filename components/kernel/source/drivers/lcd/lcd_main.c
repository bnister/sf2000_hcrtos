#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <kernel/module.h>
#include <kernel/drivers/input.h>
#include <kernel/io.h>

#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/export.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/log2.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/limits.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <kernel/elog.h>
#include <hcuapi/lvds.h>
#include <dt-bindings/gpio/gpio.h>
#include <hcuapi/pinpad.h>
#include <hcuapi/pinmux.h>
#include <hcuapi/gpio.h>
#include "lcd_main.h"

static LIST_HEAD(lcd_map_list);
static DEFINE_SPINLOCK(lcd_map_lock);

volatile uint32_t *sys_lvds_gpio = (void *)0xb8800174;
static struct lcd_map_list *seek_lcd_map(const char *name)
{
	struct lcd_map_list *map = NULL;

	spin_lock(&lcd_map_lock);
	list_for_each_entry(map, &lcd_map_list, list) {
		if (!strcmp(name, map->map.name)) {
			spin_unlock(&lcd_map_lock);
			return map;
		}
	}
	spin_unlock(&lcd_map_lock);

	return NULL;
}

struct lcd_map *lcd_map_get(const char *name)
{

	struct lcd_map_list *map;
	if(name == NULL)
		return NULL;
	map = seek_lcd_map(name);
	if (!map) {
		log_e("lcd dev %s not found\n", name);
		return NULL;
	}

	log_d("Registered IR keymap %s\n", map->map.name);

	return &map->map;
}
EXPORT_SYMBOL_GPL(lcd_map_get);

int lcd_map_register(struct lcd_map_list *map)
{
	spin_lock(&lcd_map_lock);
	list_add_tail(&map->list, &lcd_map_list);
	spin_unlock(&lcd_map_lock);
	return 0;
}

static void lvds_gpio_set_output_high(unsigned int padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg= (void *)((uint32_t)sys_lvds_gpio);
	REG32_SET_BIT(reg,bit);
}

static void lvds_gpio_set_output_low(unsigned int padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg= (void *)((uint32_t)sys_lvds_gpio);
	REG32_CLR_BIT(reg,bit);
}

static void lvds_set_gpio_output(struct lvds_set_gpio *pad)
{
	if(pad==NULL)
	{
		log_e("error PAD NULL\n");
		return;
	}
	if(pad->padctl>PINPAD_LVDS_RESERVED12)
	{
		log_e("padctl Maximum exceeded\n");
		return;
	}
	pad->padctl = pad->padctl - PINPAD_LVDS_DP0;
	if(pad->value)
		lvds_gpio_set_output_high(pad->padctl);
	else
		lvds_gpio_set_output_low(pad->padctl);
}

void lcd_gpio_set_output(uint8_t padctl, bool value)
{
	struct lvds_set_gpio pad;
	if(padctl >= PINPAD_LVDS_DP0 && padctl < PINPAD_LVDS_MAX)
	{
		pad.padctl=padctl;
		pad.value=value;
		lvds_set_gpio_output(&pad);
	}
	else
		gpio_set_output(padctl,value);
}
