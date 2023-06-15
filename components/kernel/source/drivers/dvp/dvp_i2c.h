#ifndef _DVP_I2C_H_
#define _DVP_I2C_H_

void dvp_i2c_set_slv_addr(uint32_t slv_addr);
void dvp_i2c_open(const char *devpath);
void dvp_i2c_close(void);
int dvp_i2c_read(uint8_t *data , int len);
int dvp_i2c_write( uint8_t *data , int len);
#endif
