#ifndef __LINUX_GPIO_H
#define __LINUX_GPIO_H

#include <linux/errno.h>

/* make these flag values available regardless of GPIO kconfig options */
#define GPIOF_DIR_OUT	(0 << 0)
#define GPIOF_DIR_IN	(1 << 0)

#define GPIOF_INIT_LOW	(0 << 1)
#define GPIOF_INIT_HIGH	(1 << 1)

#define GPIOF_IN		(GPIOF_DIR_IN)
#define GPIOF_OUT_INIT_LOW	(GPIOF_DIR_OUT | GPIOF_INIT_LOW)
#define GPIOF_OUT_INIT_HIGH	(GPIOF_DIR_OUT | GPIOF_INIT_HIGH)

/* Gpio pin is active-low */
#define GPIOF_ACTIVE_LOW        (1 << 2)

/* Gpio pin is open drain */
#define GPIOF_OPEN_DRAIN	(1 << 3)

/* Gpio pin is open source */
#define GPIOF_OPEN_SOURCE	(1 << 4)

#define GPIOF_EXPORT		(1 << 5)
#define GPIOF_EXPORT_CHANGEABLE	(1 << 6)
#define GPIOF_EXPORT_DIR_FIXED	(GPIOF_EXPORT)
#define GPIOF_EXPORT_DIR_CHANGEABLE (GPIOF_EXPORT | GPIOF_EXPORT_CHANGEABLE)

struct device;
int devm_gpio_request_one(struct device *dev, unsigned gpio,
			  unsigned long flags, const char *label);

#define gpio_set_debounce(gpio, debounce) (-ENOSYS)

#endif /* __LINUX_GPIO_H */
