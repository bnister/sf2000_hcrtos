#include <nuttx/i2c/i2c_master.h>
#include <nuttx/i2c/i2c_bitbang.h>
#include <nuttx/kmalloc.h>
#include <hcuapi/gpio.h>
#include <kernel/lib/fdt_api.h>
#include <stdio.h>
#include <kernel/module.h>
#include <linux/spinlock.h>

static void i2c_bb_set_scl(FAR struct i2c_bitbang_lower_dev_s *lower, bool high);
static void i2c_bb_set_sda(FAR struct i2c_bitbang_lower_dev_s *lower, bool high);
static bool i2c_bb_get_scl(FAR struct i2c_bitbang_lower_dev_s *lower);
static bool i2c_bb_get_sda(FAR struct i2c_bitbang_lower_dev_s *lower);
static void i2c_bb_initialize(FAR struct i2c_bitbang_lower_dev_s *lower);

struct i2c_bitbang_lower_ops_s hc_ops = {
	.initialize	= i2c_bb_initialize,
	.set_scl	= i2c_bb_set_scl,
	.set_sda	= i2c_bb_set_sda,
	.get_scl	= i2c_bb_get_scl,
	.get_sda	= i2c_bb_get_sda,
};

static void i2c_bb_set_scl(FAR struct i2c_bitbang_lower_dev_s *lower, bool high)
{
	struct hc_i2c_bitbang_dev_s *dev = lower->priv;
	
	return gpio_set_output(dev->scl_pin, high);
}

static void i2c_bb_set_sda(FAR struct i2c_bitbang_lower_dev_s *lower, bool high)
{
	struct hc_i2c_bitbang_dev_s *dev = lower->priv;
	
	return gpio_set_output(dev->sda_pin, high);
}


static bool i2c_bb_get_scl(FAR struct i2c_bitbang_lower_dev_s *lower)
{
	struct hc_i2c_bitbang_dev_s *dev = lower->priv;

	return gpio_get_input(dev->sda_pin);
}

static bool i2c_bb_get_sda(FAR struct i2c_bitbang_lower_dev_s *lower)
{
	struct hc_i2c_bitbang_dev_s *dev = lower->priv;

	return gpio_get_input(dev->scl_pin);
}

static void i2c_bb_initialize(FAR struct i2c_bitbang_lower_dev_s *lower)
{
	struct hc_i2c_bitbang_dev_s *dev = lower->priv;

	gpio_configure(dev->sda_pin, GPIO_DIR_OUTPUT);
	gpio_configure(dev->scl_pin, GPIO_DIR_OUTPUT);
	gpio_set_output(dev->sda_pin, 1);
	gpio_set_output(dev->scl_pin, 1);
}

static struct i2c_master_s *hc_i2c_bitbang_initialize(pinpad_e sda_pin, pinpad_e scl_pin, uint32_t baudrate)
{
	struct hc_i2c_bitbang_dev_s *dev = 
		(struct hc_i2c_bitbang_dev_s *)kmm_zalloc(sizeof(struct hc_i2c_bitbang_dev_s));

	DEBUGASSERT(dev);

	dev->lower.ops = &hc_ops;
	dev->lower.priv = dev;
	dev->scl_pin = scl_pin;
	dev->sda_pin = sda_pin;
	dev->delay = 3;


	return i2c_bitbang_initialize(&dev->lower);
}

static int i2cbus_uninitialize(struct i2c_master_s *dev)
{
	return 0;
}

static int gpio_i2c_probe(const char *node, int id)
{
	struct i2c_master_s *i2c;
	int np;
	unsigned int baudrate;
	pinpad_e scl, sda;
	int ret = 0;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	if (fdt_get_property_u_32_index(np, "sda-pinmux", 0, &sda) < 0)
		return 0;
	if (fdt_get_property_u_32_index(np, "scl-pinmux", 0, &scl) < 0)
		return 0;

	i2c = hc_i2c_bitbang_initialize(sda, scl, baudrate);

	if (i2c == NULL)
		return 0;

	i2c->simulate = 1;
	
	ret = i2c_register(i2c, id);
	if (ret < 0) {
		printf("ERROR: Failed to register GPIO I2C%d driver:%d\n",id,ret);
		i2cbus_uninitialize(i2c);
	}

	return ret;
}

static int i2c_bitbang_init(void)
{
	int rc = 0;

	rc |= gpio_i2c_probe("/hcrtos/gpio-i2c@0", 0);
	rc |= gpio_i2c_probe("/hcrtos/gpio-i2c@1", 1);
	rc |= gpio_i2c_probe("/hcrtos/gpio-i2c@2", 2);
	rc |= gpio_i2c_probe("/hcrtos/gpio-i2c@3", 3);

	return rc;
}

module_driver(gpio_i2c, i2c_bitbang_init, NULL, 0)
