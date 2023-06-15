#ifndef __I2C_SLAVE_H
#define __I2C_SLAVE_H

#include <nuttx/i2c/i2c_slave.h>

extern struct i2c_slave_s *hc_hdmi_i2cs_initialize(int port, void *param);
extern int hc_hdmi_i2cs_deinit(void *priv);

#endif
