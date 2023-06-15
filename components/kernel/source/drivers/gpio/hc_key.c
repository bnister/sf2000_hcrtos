#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <kernel/lib/console.h>
#include <kernel/module.h>

#include <freertos/FreeRTOS.h>
#include <kernel/lib/console.h>
#include <sys/mman.h>
#include <hcuapi/pinpad.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>

#include <kernel/lib/libfdt/libfdt.h>
#include <kernel/lib/fdt_api.h>
#include <linux/slab.h>
#include <kernel/io.h>
#include <kernel/ld.h>

#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define KEY_UPDOWN_SAMPLE	3
#define KEY_SAMPLE		18
#define  KEY_TIME_OUT  20*1000

typedef struct {
	uint32_t  padctl;
	uint32_t  value_status;
	uint32_t  key_code;
} hc_gpiokey_param_s;

typedef struct hc_key_priv
{
	int				flag;
	uint32_t			key_down_cnt;
	uint32_t			key_up_cnt;
	uint32_t			timeout;
	unsigned long			key_jiffies;
	struct timer_list		timer_key;	
	struct input_dev 		*input;
	hc_gpiokey_param_s *key_map;
}hc_key_priv_s;

static void hc_timer_key(struct timer_list *param)
{
	struct hc_key_priv *priv = from_timer(priv, param, timer_key);
	if(priv->flag == 0)
	{
		if (priv->key_map->value_status == GPIO_ACTIVE_LOW) {
			if (!gpio_get_input(priv->key_map->padctl))
				priv->flag = 1;
		} else {
			if (gpio_get_input(priv->key_map->padctl))
				priv->flag = 1;
		}
	}

	if (priv->flag == 1)
	{
		if (priv->key_map->value_status == GPIO_ACTIVE_LOW)
		{
			if(!gpio_get_input(priv->key_map->padctl))
				priv->key_down_cnt++;
			else
				priv->key_up_cnt++;

			if((priv->key_up_cnt != KEY_SAMPLE/KEY_UPDOWN_SAMPLE))
			{
				priv->timeout = KEY_TIME_OUT;
				priv->key_jiffies = jiffies + usecs_to_jiffies(priv->timeout);
				mod_timer(&priv->timer_key, priv->key_jiffies);
			}

			if(priv->key_down_cnt == (KEY_SAMPLE/KEY_UPDOWN_SAMPLE))
			{
				priv->key_down_cnt++; 
				input_event(priv->input, EV_MSC, MSC_SCAN,priv->key_map->key_code);	
				input_report_key(priv->input, priv->key_map->key_code, 1);
			}
			else if(priv->key_down_cnt == (KEY_SAMPLE))
			{
				input_event(priv->input, EV_MSC, MSC_SCAN,priv->key_map->key_code);	
				input_sync(priv->input);
				priv->key_down_cnt =( KEY_SAMPLE/KEY_UPDOWN_SAMPLE + 1);
				priv->key_up_cnt = 0;
			}
			else if(priv->key_up_cnt == KEY_SAMPLE/KEY_UPDOWN_SAMPLE)
			{
				input_report_key(priv->input, priv->key_map->key_code, 0);
				input_sync(priv->input);
				priv->key_up_cnt = 0;
				priv->key_down_cnt = 0;
				priv->flag = 0;
			}
		}
		else
		{
			if(gpio_get_input(priv->key_map->padctl))
				priv->key_down_cnt++;
			else
				priv->key_up_cnt++;

			if((priv->key_up_cnt != KEY_SAMPLE/KEY_UPDOWN_SAMPLE))
			{
				priv->timeout = KEY_TIME_OUT;
				priv->key_jiffies = jiffies + usecs_to_jiffies(priv->timeout);
				mod_timer(&priv->timer_key, priv->key_jiffies);
			}

			if(priv->key_down_cnt == (KEY_SAMPLE/KEY_UPDOWN_SAMPLE))
			{
				priv->key_down_cnt++; 
				input_event(priv->input, EV_MSC, MSC_SCAN,priv->key_map->key_code);	
				input_report_key(priv->input, priv->key_map->key_code, 1);
			}
			else if(priv->key_down_cnt == (KEY_SAMPLE))
			{
				input_event(priv->input, EV_MSC, MSC_SCAN,priv->key_map->key_code);	
				input_sync(priv->input);
				priv->key_down_cnt =( KEY_SAMPLE/KEY_UPDOWN_SAMPLE + 1);
				priv->key_up_cnt = 0;
			}
			else if(priv->key_up_cnt == KEY_SAMPLE/KEY_UPDOWN_SAMPLE)
			{
				input_report_key(priv->input, priv->key_map->key_code, 0);
				input_sync(priv->input);
				priv->key_up_cnt = 0;
				priv->key_down_cnt = 0;
				priv->flag = 0;
			}
		}
	}
	return;
}

static void key_gpio_irq(uint32_t param)
{
	struct hc_key_priv *priv = (struct hc_key_priv *)param; 

	priv->timeout = KEY_TIME_OUT;
	priv->key_jiffies = jiffies + usecs_to_jiffies(priv->timeout);
	mod_timer(&priv->timer_key, priv->key_jiffies);

	return ;
}

static int key_probe(const char *path)
{
	const char *status;
	int ret = 0;
	gpio_pinset_t int_pinset = GPIO_DIR_INPUT;
	u32 key_num;
	struct pinmux_setting *active_state;

	int np = fdt_node_probe_by_path(path);
	if (np < 0)
		return -1;

	ret = fdt_get_property_string_index(np, "status", 0, &status);
	if(ret != 0)
		return -1;

	if(strcmp(status, "okay"))
		return -1;

	active_state = fdt_get_property_pinmux(np, "active");
	if (active_state != NULL)
		pinmux_select_setting(active_state);
	else
		return -1;

	ret = fdt_get_property_u_32_index(np, "key-num", 0, &key_num);

	hc_gpiokey_param_s *key_map = kzalloc((sizeof( hc_gpiokey_param_s) * key_num ), GFP_KERNEL);

	fdt_get_property_u_32_array(np, "gpio-key-map",(u32 *)key_map, (key_num) * 3);

	struct hc_key_priv *priv[key_num];

	for (u32 i = 0; i < key_num; i++) {
		priv[i] = kzalloc(sizeof(struct hc_key_priv), GFP_KERNEL);

		priv[i]->key_map = (key_map + i);
		priv[i]->key_down_cnt = 0;
		priv[i]->key_up_cnt = 0;
		priv[i]->flag = 0;

		priv[i]->input = input_allocate_device();
		input_set_drvdata(priv[i]->input, priv[i]);
		ret = input_register_device(priv[i]->input);

		if (priv[i]->key_map->padctl != PINPAD_INVALID) {
			if (priv[i]->key_map->value_status == GPIO_ACTIVE_LOW)
				gpio_configure(priv[i]->key_map->padctl,
					       int_pinset | GPIO_IRQ_FALLING);
			else
				gpio_configure(priv[i]->key_map->padctl,
					       int_pinset | GPIO_IRQ_RISING);
		}

		ret = gpio_irq_request(priv[i]->key_map->padctl, key_gpio_irq, (uint32_t)priv[i]);
		if (ret < 0)
			return -1;

		timer_setup(&priv[i]->timer_key, hc_timer_key, 0);

		key_gpio_irq(priv[i]);
	}

	return 0;
}

static int hc_key_init(void)
{
	int ret = 0;

	ret |= key_probe("/hcrtos/gpio_key");

	return ret;
}


module_driver(hc_key,hc_key_init,NULL,2)
