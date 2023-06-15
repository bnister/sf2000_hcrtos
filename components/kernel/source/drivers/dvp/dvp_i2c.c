#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <hcuapi/i2c-master.h>

static int dvp_i2c_fd = 0;
static uint32_t dvp_slave_addr = 0;

void dvp_i2c_set_slv_addr(uint32_t slv_addr)
{
    dvp_slave_addr = slv_addr;
}
void dvp_i2c_open(const char *devpath)
{
    dvp_i2c_fd = open(devpath , O_RDWR);
    if(dvp_i2c_fd < 0)
    {
        perror("Error: cannot open dvp_i2c");
        return;
    }
}

void dvp_i2c_close(void)
{
    printf("dvp_i2c_close i2c:%d\n", dvp_i2c_fd);
    if(dvp_i2c_fd >=0)
    {
        close(dvp_i2c_fd);
        dvp_i2c_fd = -1;
    }
}

int dvp_i2c_read(uint8_t *data , int len)
{
    struct i2c_transfer_s xfer;
    struct i2c_msg_s i2c_msg = { 0 };

    i2c_msg.addr = (uint8_t)dvp_slave_addr;
    i2c_msg.flags = I2C_M_READ;
    i2c_msg.buffer = data;
    i2c_msg.length = len;
    xfer.msgv = &i2c_msg;
    xfer.msgc = 1;

    if(ioctl(dvp_i2c_fd , I2CIOC_TRANSFER , &xfer) < 0)
    {
        printf("ioctl(set I2CIOC_TRANSFER dvp_i2c_fd=%d dvp_slave_addr=0x%lx)\n", 
               dvp_i2c_fd, dvp_slave_addr);
        return -1;
    }
    // printf("vin_dvp_i2c_read OK\n");

    return 0;
}

int dvp_i2c_write( uint8_t *data , int len)
{
    struct i2c_transfer_s xfer;
    struct i2c_msg_s i2c_msg = { 0 };

    i2c_msg.addr = (uint8_t)dvp_slave_addr;
    i2c_msg.flags = 0;
    i2c_msg.buffer = data;
    i2c_msg.length = len;

    xfer.msgv = &i2c_msg;
    xfer.msgc = 1;
    if(ioctl(dvp_i2c_fd , I2CIOC_TRANSFER , &xfer) < 0)
    {
        printf("ioctl(set I2CIOC_TRANSFER dvp_i2c_fd=%d dvp_slave_addr=0x%lx)\n" ,
               dvp_i2c_fd , dvp_slave_addr);
        return -1;
    }
    // printf("vin_dvp_i2c_write OK\n");
    return 0;
}
