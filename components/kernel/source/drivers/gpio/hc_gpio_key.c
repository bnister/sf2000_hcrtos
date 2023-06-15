#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <hcuapi/gpio.h>
#include <kernel/delay.h>
#include <stdio.h>

static TaskHandle_t gpio_key_thread = NULL;
static int register_num = 0;
struct hc_gpio_key_priv {
        struct input_dev                *input;
        struct device                   *dev;
        int                             keymap_len;
        int                             *key_map;
	int				*pin_map;
        struct gpio_descs *gpiods;
        int                             enable;
};

static void gpio_key_kthread(void *pvParameters);
static int hc_gpio_key_open(struct input_dev *dev)
{
        struct hc_gpio_key_priv *priv = input_get_drvdata(dev);

        priv->enable = 1;
	
	if (register_num == 0) {
		xTaskCreate(gpio_key_kthread, (const char *)"gpio_key_kthread", configTASK_STACK_DEPTH,
				priv, portPRI_TASK_NORMAL, &gpio_key_thread);
	}	

	register_num++;
        return 0;
}

static void hc_gpio_key_close(struct input_dev *dev)
{
        struct hc_gpio_key_priv *priv = input_get_drvdata(dev);

	if (register_num == 0) {
		priv->enable = 0;
		vTaskDelete(gpio_key_thread);
	} else 
		register_num--;

        return;
}

static void input_init(struct hc_gpio_key_priv *priv)
{
	priv->input->open = hc_gpio_key_open;
	priv->input->close = hc_gpio_key_close;
}

static int hc_gpio_key_get(struct hc_gpio_key_priv *priv)
{
	int i, temp = 0;

	gpio_configure(priv->pin_map[0], GPIO_DIR_INPUT);
	msleep(10);
	for (i = 0; i < priv->keymap_len; i++) {
		if (gpio_get_input(priv->pin_map[i]) == 0) 
			temp = i+1;
	}

	gpio_configure(priv->pin_map[0], GPIO_DIR_OUTPUT);
	gpio_set_output(priv->pin_map[0], 0);
	msleep(10);

	for (i = 1; i < priv->keymap_len; i++) {
		if (gpio_get_input(priv->pin_map[i]) == 0) {
			if (temp != 0)
				break;
			temp = i+3;
		}
	}

	return temp;
}

static void gpio_key_kthread(void *pvParameters)
{
	int i, src_code = 0, cmp_code = 0, re_code = 0, num = 0;
	struct hc_gpio_key_priv *priv = (struct hc_gpio_key_priv *)pvParameters;

	for (i = 0; i < priv->keymap_len; i++)
		gpio_configure(priv->pin_map[0], GPIO_DIR_INPUT);

	while (1) {
		if (priv->enable == 0)
			usleep(100000);
		else {
			src_code = hc_gpio_key_get(priv);

			if (src_code) {
				if (src_code == cmp_code)
					num++;
				else
					cmp_code = src_code;
			}

			if (num >= 3) {
				if (src_code != 0) {
					if (num == 3) {
						input_event(priv->input, EV_MSC, MSC_SCAN, priv->key_map[src_code - 1]);
						input_report_key(priv->input, priv->key_map[src_code - 1], 1);
						input_sync(priv->input);
						re_code = src_code;

					} else if (num % 3 == 0) {
						input_event(priv->input, EV_MSC, MSC_SCAN, priv->key_map[src_code - 1]);
						input_sync(priv->input);
						re_code = src_code;
					} 
				} else {
					input_report_key(priv->input, priv->key_map[re_code - 1], 0);
					input_sync(priv->input);
					num = 0;
				} 
			} 
		}
	}

	return;
}

static int hc_gpiokey_probe(const char *node)
{
	int np, ret;
	u32 i, pin, num_pins;
	struct hc_gpio_key_priv *priv;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	priv = kzalloc(sizeof(struct hc_gpio_key_priv), GFP_KERNEL);
	if (!priv)
		return 0;

	if (fdt_get_property_data_by_name(np, "key-gpios", &num_pins) == NULL)
		num_pins = 0;

	num_pins >>= 3;

	if (num_pins == 0)
		return 0;

	priv->keymap_len = num_pins;
	priv->pin_map = malloc(sizeof(int) * num_pins);
	for (i = 0; i < num_pins; i++) {
		fdt_get_property_u_32_index(np, "key-gpios", i*2, &pin);

		pinmux_configure(pin, 0);
		priv->pin_map[i] = pin;
	}
	
	if (fdt_get_property_data_by_name(np, "key-map", &num_pins) == NULL)
		num_pins = 0;

	if (num_pins == 0)
		return 0;

	num_pins >>= 2;

	priv->key_map = malloc(sizeof(int) * num_pins);
	fdt_get_property_u_32_array(np, "key-map", (u32 *)priv->key_map, num_pins);

	priv->input = input_allocate_device();
	if (!priv->input)
		goto err;

	input_init(priv);

	input_set_drvdata(priv->input, priv);

	ret = input_register_device(priv->input);

	return ret;

err:
	input_free_device(priv->input);
	kfree(priv);
	return 0;
}

static int gpio_key_init(void)
{
	int rc = 0;

	rc |= hc_gpiokey_probe("/hcrtos/gpio-key-group");

	return rc;
}

module_driver(gpio_key, gpio_key_init, NULL, 3)
