THIRDPARTYNUTTXMTD=../../third_party/nuttx/drivers/i2c

_i2c-y := i2c_read.o i2c_write.o i2c_writeread.o

_i2c-$(CONFIG_I2C_DRIVER) += i2c_driver.o
_i2c-$(CONFIG_I2C_GPIO_MASTER) += i2c_bitbang.o

i2c-y := $(addprefix $(THIRDPARTYNUTTXMTD)/,$(_i2c-y))
i2c-$(CONFIG_I2C_SCB_MASTER) += hc-i2c.o
i2c-$(CONFIG_I2C_GPIO_MASTER) += hc_i2c_bitbang.o
i2c-$(CONFIG_I2C_SLAVE_HDMI) += hdmi_i2c_slave.o

obj-y += i2c.o
