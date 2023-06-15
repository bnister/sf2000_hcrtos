#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/module.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <kernel/lib/console.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/list.h>
#include <nuttx/wqueue.h>
#include <hcuapi/virtuart.h>
#include <hcuapi/tvtype.h>
#include <hcuapi/dvpdevice.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>
#include <kernel/ld.h>
#include <linux/io.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include "../dvp_i2c.h"

#include "rn6752V1_720P_25.h"
#include "rn6752V1_720P_30.h"
#include "rn6752V1_cvbs_pal.h"
#include "rn6752V1_cvbs_ntsc.h"
#include "rn6752V1_1080P_25.h"
#include "rn6752V1_1080P_30.h"

typedef enum _E_VIDEO_CLOCK_
{
    E_VIDEO_CLK_27M = 0 ,
    E_VIDEO_CLK_13_5M ,
    E_VIDEO_CLK_36M ,
    E_VIDEO_CLK_72M ,
    E_VIDEO_CLK_18M ,
}E_VIDEO_CLOCK;

typedef enum _E_VIDEO_FORMAT_SEL_
{
    E_VIDEO_FORMAT_SEL_720P_1080N = 0 ,
    E_VIDEO_FORMAT_SEL_D1_960H ,
}E_VIDEO_FORMAT_SEL;

typedef enum _E_VIDEO_RES_FORMAT_
{
    E_VIDEO_RES_FORMAT_720 = 0 ,
    E_VIDEO_RES_FORMAT_960 ,
    E_VIDEO_RES_FORMAT_1280 ,
}E_VIDEO_RES_FORMAT;

typedef enum _E_HD_MODE_SEL_
{
    E_HD_MODE_SEL_720P25 = 2 ,
    E_HD_MODE_SEL_720P30 = 4 ,
    E_HD_MODE_SEL_1080P25 = 5 ,
    E_HD_MODE_SEL_1080P30 = 6 ,
}E_HD_MODE_SEL;

typedef enum _E_VIDEO_OUTPUT_SRC_FORMAT_
{
    E_VIDEO_OUTPUT_SRC_FORMAT_BT656 = 0 ,
    E_VIDEO_OUTPUT_SRC_FORMAT_BT1302 ,
}E_VIDEO_OUTPUT_SRC_FORMAT;

struct rn6752v1_chrdev
{
    const char *i2c_devpath;
    uint32_t    i2c_addr;
    uint32_t    b_support_input_detect;
    uint32_t    reset_gpio;
    uint32_t    reset_active;
};

static void rn6752V1_dts_init(struct rn6752v1_chrdev *chrdev)
{
    int np;

    np = fdt_get_node_offset_by_path("/hcrtos/dvp-rn67521V1");

    chrdev->reset_gpio = 0xffffffff;
    chrdev->i2c_addr = 0xffffffff;
    chrdev->reset_active = 0xffffffff;

    if(np >= 0)
    {
        if(chrdev != NULL)
        {
            fdt_get_property_string_index(np , "i2c-devpath" , 0 , &chrdev->i2c_devpath);
            fdt_get_property_u_32_index(np , "i2c-addr" , 0 , (u32 *)(&chrdev->i2c_addr));
            fdt_get_property_u_32_index(np , "reset-gpio" , 0 , (u32 *)(&chrdev->reset_gpio));
            fdt_get_property_u_32_index(np , "reset-gpio" , 1 , (u32 *)(&chrdev->reset_active));
            if(chrdev->reset_active == GPIO_ACTIVE_LOW)
            {
                chrdev->reset_active = 0;
            }
            else
            {
                chrdev->reset_active = 1;
            }
        }
    }
}

static void rn6752V1_hw_reset(struct rn6752v1_chrdev *chrdev)
{
    if(chrdev->reset_gpio != 0xffffffff)
    {
        printf("chrdev->reset_gpio=%ld chrdev->reset_active=%ld\n" ,
               chrdev->reset_gpio , chrdev->reset_active);
        gpio_configure(chrdev->reset_gpio , GPIO_DIR_OUTPUT);
        gpio_set_output(chrdev->reset_gpio , chrdev->reset_active);
        msleep(5);
        gpio_set_output(chrdev->reset_gpio , 1 - chrdev->reset_active);
        msleep(100);
    }
}

static int rn6752V1_read_reg(uint8_t cmd , uint8_t *p_data , uint32_t len)
{
    int ret = 0;

    ret = dvp_i2c_write(&cmd , 1);

    if(ret < 0)
    {
        printk("%s %d fail\n" , __FUNCTION__ , __LINE__);
        return ret;
    }

    ret = dvp_i2c_read(p_data , len);
    if(ret < 0)
    {
        return ret;
    }

    return ret;
}

static int rn6752V1_write_reg(uint8_t cmd , uint8_t *p_data , uint32_t len)
{
    int ret = 0;

    uint8_t *array = NULL;
    uint8_t i = 0;
    //uint8_t value = 0;

    array = malloc(len + 1);
    if(NULL == array)
    {
        return -1;
    }

    array[0] = cmd;
    for(i = 0;i < len;i++)
    {
        array[i + 1] = p_data[i];
    }
    ret = dvp_i2c_write(array , len + 1);

    free(array);
    if(ret < 0)
    {
        printk("%s %d fail\n" , __FUNCTION__ , __LINE__);
        return -1;
    }
    //rn6752V1_read_reg(cmd , &value , 1);

    return ret;
}

static int rn6752V1_write_reg_byte(uint8_t cmd , uint8_t data)
{
    int ret = 0;
    uint8_t array[2];

    array[0] = cmd;
    array[1] = data;

    ret = dvp_i2c_write(array , 2);
    if(ret < 0)
    {
        printk("%s %d fail\n" , __FUNCTION__ , __LINE__);
    }
    //printk("%s cmd = 0x%x data=0x%x\n" , __FUNCTION__ , cmd , data);
    return ret;
}

static bool rn6752V1_get_input_signal_status(uint8_t reg_value)
{
    bool ret = false;

    ret = ((reg_value >> 4) & 0x1) == 0 ? true : false;

    return ret;
}

static bool rn6752V1_get_input_signal_format_is_720p(uint8_t reg_value)
{
    bool ret = false;

    ret = (reg_value >> 5) & 0x1;
    return ret;
}

static bool rn6752V1_get_input_signal_format_is_1080p(uint8_t reg_value)
{
    bool ret = false;

    ret = (reg_value >> 6) & 0x1;
    return ret;
}

static uint32_t rn6752V1_get_input_signal_frame_rate(uint8_t reg_value)
{
    uint32_t ret = 25;

    ret = (reg_value & 0x1) == 1 ? 30 : 25;
    return ret;
}

static bool rn6752V1_get_input_signal_format(tvtype_e *tv_sys , bool *bProgressive)
{
    bool input_on = false;
    uint32_t frame_rate = 0;
    uint8_t reg_0_value = 0;
    int ret = 0;

    rn6752V1_write_reg_byte(0xff , 0);
    rn6752V1_write_reg_byte(0x49 , 0x81);
    rn6752V1_write_reg_byte(0x19 , 0x0a);

    ret = rn6752V1_read_reg(0x00 , &reg_0_value , 1);
    if(ret<0)
    {
        return false;
    }
    //printf("reg_0_value = 0x%x\n", reg_0_value);
    input_on = rn6752V1_get_input_signal_status(reg_0_value);
    if(input_on == true)
    {
        if(tv_sys == NULL || bProgressive == NULL)
        {
            return false;
        }
        msleep(300);
        rn6752V1_read_reg(0x00 , &reg_0_value , 1);
        frame_rate = rn6752V1_get_input_signal_frame_rate(reg_0_value);
        if(rn6752V1_get_input_signal_format_is_720p(reg_0_value))
        {
            if(frame_rate == 25)
            {
                *tv_sys = TV_LINE_720_25;
            }
            else
            {
                *tv_sys = TV_LINE_720_30;
            }
            *bProgressive = true;
        }
        else if(rn6752V1_get_input_signal_format_is_1080p(reg_0_value))
        {
            if(frame_rate == 25)
            {
                *tv_sys = TV_LINE_1080_25;
            }
            else
            {
                *tv_sys = TV_LINE_1080_30;
            }
            *bProgressive = true;
        }
        else
        {
            if(frame_rate == 25)
            {
                *tv_sys = TV_PAL;
            }
            else
            {
                *tv_sys = TV_NTSC;
            }
            *bProgressive = false;
        }
        return true;
    }
    else
    {
        return false;
    }
}

static int rn6752V1_init_config(struct regval_list *reg_list)
{
    struct regval_list *p_reg_list = reg_list;
    int ret = -1;

    for(;;)
    {
        if(p_reg_list->reg_num == 0xFF && p_reg_list->value == 0xFF)
        {
            break;
        }

        ret = rn6752V1_write_reg(p_reg_list->reg_num , &(p_reg_list->value) , 1);
        p_reg_list++;
    }

    return ret;
}

int rn6752V1_pre_initial(void)
{
    int ret = -1;

    unsigned char rom_byte5 , rom_byte6;
    unsigned char rom_bype[6];

    ret = rn6752V1_write_reg_byte(0xE1 , 0x80);
    if(ret < 0)
    {
        return ret;
    }
    ret = rn6752V1_write_reg_byte(0xFA , 0x81);

    ret = rn6752V1_read_reg(0xFB , &rom_bype[0] , 1);
    ret = rn6752V1_read_reg(0xFB , &rom_bype[1] , 1);
    ret = rn6752V1_read_reg(0xFB , &rom_bype[2] , 1);
    ret = rn6752V1_read_reg(0xFB , &rom_bype[3] , 1);
    ret = rn6752V1_read_reg(0xFB , &rom_bype[4] , 1);
    ret = rn6752V1_read_reg(0xFB , &rom_bype[5] , 1);

    ret = rom_byte6 = rom_bype[5];
    ret = rom_byte5 = rom_bype[4];
    // config. decoder accroding to rom_byte5 and rom_byte6
    if((rom_byte6 == 0x00) && (rom_byte5 == 0x00))
    {
        ret = rn6752V1_write_reg_byte(0xEF , 0xAA);
        ret = rn6752V1_write_reg_byte(0xE7 , 0xFF);
        ret = rn6752V1_write_reg_byte(0xFF , 0x09);
        ret = rn6752V1_write_reg_byte(0x03 , 0x0C);
        ret = rn6752V1_write_reg_byte(0xFF , 0x0B);
        ret = rn6752V1_write_reg_byte(0x03 , 0x0C);
    }
    else if(((rom_byte6 == 0x34) && (rom_byte5 == 0xA9)) ||
            ((rom_byte6 == 0x2C) && (rom_byte5 == 0xA8)))
    {
        ret = rn6752V1_write_reg_byte(0xEF , 0xAA);
        ret = rn6752V1_write_reg_byte(0xE7 , 0xFF);
        ret = rn6752V1_write_reg_byte(0xFC , 0x60);
        ret = rn6752V1_write_reg_byte(0xFF , 0x09);
        ret = rn6752V1_write_reg_byte(0x03 , 0x18);
        ret = rn6752V1_write_reg_byte(0xFF , 0x0B);
        ret = rn6752V1_write_reg_byte(0x03 , 0x18);
    }
    else
    {
        ret = rn6752V1_write_reg_byte(0xEF , 0xAA);
        ret = rn6752V1_write_reg_byte(0xFC , 0x60);
        ret = rn6752V1_write_reg_byte(0xFF , 0x09);
        ret = rn6752V1_write_reg_byte(0x03 , 0x18);
        ret = rn6752V1_write_reg_byte(0xFF , 0x0B);
        ret = rn6752V1_write_reg_byte(0x03 , 0x18);
    }

    return ret;
}

static void rn6752V1_reset(void)
{
    uint8_t value = 0;

    printk("rn6752V1_reset\n");
    rn6752V1_read_reg(0x80 , &value , 1);
    value |= 0x1;
    rn6752V1_write_reg(0x80 , &value , 1);
    msleep(30);
    value &= 0xFE;
    rn6752V1_write_reg(0x80 , &value , 1);
    msleep(30);
}

static int rn6752V1_set_tv_sys(tvtype_e tv_sys , bool bProgressive)
{
    int ret = 0;

    // rn6752V1_reset();
    ret = rn6752V1_pre_initial();

    if(ret == -1)
    {
        return ret;
    }

    switch(tv_sys)
    {
        case TV_PAL:
            ret = rn6752V1_init_config(RN675V1_init_cfg_cvbs_pal);
            break;
        case TV_NTSC:
            ret = rn6752V1_init_config(RN675V1_init_cfg_cvbs_ntsc);
            break;
        case TV_LINE_720_25:
            ret = rn6752V1_init_config(RN6752V1_init_cfg_720P25);
            break;
        case TV_LINE_720_30:
            ret = rn6752V1_init_config(RN675V1_init_cfg_720P30);
            break;
        case TV_LINE_1080_25:
            ret = rn6752V1_init_config(RN6752V1_init_cfg_1080P25);
            break;
        case TV_LINE_1080_30:
            ret = rn6752V1_init_config(RN675V1_init_cfg_1080P30);
            break;
        default:
            ret = -1;
            break;
    }

    return ret;
}

static void rn6752V1_set_input_port_num(uint8_t port_num)
{
    rn6752V1_write_reg(0xD3 , &port_num , 1);
}

static int rn6752V1_open(struct file *file)
{
    struct rn6752v1_chrdev *chrdev = NULL;

    chrdev = malloc(sizeof(struct rn6752v1_chrdev));
    memset(chrdev , 0 , sizeof(struct rn6752v1_chrdev));
    file->f_priv = chrdev;

    rn6752V1_dts_init(chrdev);
    rn6752V1_hw_reset(chrdev);
    dvp_i2c_open(chrdev->i2c_devpath);
    dvp_i2c_set_slv_addr(chrdev->i2c_addr);
    chrdev->b_support_input_detect = true;

    return 0;
}

static int rn6752V1_close(struct file *file)
{
    struct rn6752v1_chrdev *chrdev = file->f_priv;

    printk("rn6752V1_close\n");
    dvp_i2c_close();
    if(chrdev)
    {
        free(chrdev);
    }
    file->f_priv = NULL;
    printk("rn6752V1_close end\n");

    return 0;
}

static int rn6752V1_ioctl(struct file *file , int cmd , unsigned long arg)
{
    int ret = -1;

    struct rn6752v1_chrdev *chrdev = file->f_priv;

    switch(cmd)
    {
        case DVPDEVICE_SET_INPUT_TVTYPE:
        {
            struct dvp_device_tvtype *device_tvtype = (struct dvp_device_tvtype *)arg;
            ret = rn6752V1_set_tv_sys(device_tvtype->tv_sys , device_tvtype->b_progressive);
            break;
        }
        case DVPDEVICE_GET_INPUT_DETECT_SUPPORT:
        {
            uint32_t *b_support_input_detect;
            b_support_input_detect = (uint32_t *)arg;
            *b_support_input_detect = chrdev->b_support_input_detect;
            ret = 0;
            break;
        }
        case DVPDEVICE_GET_INPUT_TVTYPE:
        {
            struct dvp_device_tvtype *device_tvtype = (struct dvp_device_tvtype *)arg;
            if(rn6752V1_get_input_signal_format(&device_tvtype->tv_sys , &device_tvtype->b_progressive) == true)
            {
                ret = 0;
            }
            else
            {
                ret = -1;
            }
            break;
        }
        case DVPDEVICE_SET_INPUT_PORT:
        {
            uint8_t port = (uint8_t)arg;
            rn6752V1_set_input_port_num(port);
            ret = 0;
            break;
        }
        default:
            ret = -1;
            break;
    }

    return ret;
}

static const struct file_operations rn6752v1_fops = {
    .open = rn6752V1_open,
    .close = rn6752V1_close,
    .read = dummy_read,
    .write = dummy_write,
    .ioctl = rn6752V1_ioctl,
};

static int rn6752V1_init(void)
{
    int np;
    const char *status;

    np = fdt_node_probe_by_path("/hcrtos/dvp-rn67521V1");
    if(np < 0)
    {
        log_w("cannot found efsue in DTS\n");
        return 0;
    }

    if(!fdt_get_property_string_index(np , "status" , 0 , &status) &&
       !strcmp(status , "disabled"))
        return 0;

    register_driver("/dev/dvp-rn67521V1" , &rn6752v1_fops , 0666 , NULL);

    return 0;
}

module_driver(rn6752V1 , rn6752V1_init , NULL , 0)
