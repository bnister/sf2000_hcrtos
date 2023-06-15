#ifndef __LINUX_GPIO_CONSUMER_H
#define __LINUX_GPIO_CONSUMER_H

#include <hcuapi/gpio.h>
#include <kernel/io.h>
#include <linux/bug.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/compiler.h>

struct device;
struct gpio_desc;

/**
 * Struct containing an array of descriptors that can be obtained using
 * gpiod_get_array().
 */
struct gpio_descs {
	unsigned int ndescs;
	struct gpio_desc *desc[];
};

#define GPIOD_FLAGS_BIT_DIR_SET		BIT(0)
#define GPIOD_FLAGS_BIT_DIR_OUT		BIT(1)
#define GPIOD_FLAGS_BIT_DIR_VAL		BIT(2)

/**
 * Optional flags that can be passed to one of gpiod_* to configure direction
 * and output value. These values cannot be OR'd.
 */
enum gpiod_flags {
	GPIOD_ASIS	= 0,
	GPIOD_IN	= GPIOD_FLAGS_BIT_DIR_SET,
	GPIOD_OUT_LOW	= GPIOD_FLAGS_BIT_DIR_SET | GPIOD_FLAGS_BIT_DIR_OUT,
	GPIOD_OUT_HIGH	= GPIOD_FLAGS_BIT_DIR_SET | GPIOD_FLAGS_BIT_DIR_OUT |
			  GPIOD_FLAGS_BIT_DIR_VAL,
};

int gpiod_to_irq(const struct gpio_desc *desc);
int gpiod_get_raw_value_cansleep(const struct gpio_desc *desc);
int gpiod_get_value_cansleep(const struct gpio_desc *desc);
struct gpio_desc *gpio_to_desc(unsigned gpio);
int gpiod_direction_output(struct gpio_desc *desc, int value);
struct gpio_desc *__must_check devm_gpiod_get_index(struct device *dev,
						    const char *con_id,
						    unsigned int idx,
						    enum gpiod_flags flags);
int gpiod_is_active_low(const struct gpio_desc *desc);
void gpiod_set_array_value_cansleep(unsigned int array_size,
				    struct gpio_desc **desc_array,
				    int *value_array);
void gpiod_put_array(struct gpio_descs *descs);
struct gpio_descs *__must_check gpiod_get_array(struct device *dev,
						const char *con_id,
						enum gpiod_flags flags);
void gpiod_set_value(struct gpio_desc *desc, int value);
struct gpio_desc *__must_check gpiod_get(struct device *dev,
					 const char *con_id,
					 enum gpiod_flags flags);
void gpiod_put(struct gpio_desc *desc);
#define gpiod_set_debounce(desc, debounce) (-ENOSYS)

#endif
