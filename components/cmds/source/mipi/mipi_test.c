#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/mipi.h>
#include <kernel/lib/console.h>
#include <string.h>
#include <hcuapi/lvds.h>
#include <hcuapi/gpio.h>

#define MIPIDEV_PATH  "/dev/mipi"
unsigned char mipi_buf[100];

static int mipi_test_enter(int argc, char *argv[])
{
	return 0;
}

static int mipi_dsi_on_cb(int argc, char *argv[])
{
    int fb = 0;
    fb = open(MIPIDEV_PATH, O_RDWR);
    if( fb < 0){
        printf("open %s failed, ret=%d\n", MIPIDEV_PATH,fb);
        return -1;
    }
    ioctl(fb,MIPI_DSI_SET_ON,NULL);
    close(fb);
    return 0;  
}

static int mipi_dsi_off_cb(int argc, char *argv[])
{
    int fb = 0;
    fb = open(MIPIDEV_PATH, O_RDWR);
    if( fb < 0){
        printf("open %s failed, ret=%d\n", MIPIDEV_PATH,fb);
        return -1;
    }
    ioctl(fb,MIPI_DSI_SET_OFF,NULL);
    close(fb);
    return 0;    
}

static int mipi_set_dsi_format_cb(int argc, char *argv[])
{
    int fb = 0;
    unsigned char arg_temp=0;
    unsigned long arg=0;
    char *ptr=NULL;
    int argc_temp=0;
    fb = open(MIPIDEV_PATH, O_RDWR);
    if( fb < 0){
        printf("open %s failed, ret=%d\n", MIPIDEV_PATH,fb);
        return -1;
    }
    argc_temp=argc;
    while(argc_temp)
    printf("%s\n",argv[--argc_temp]);
    if(argc>1)
    {
        arg_temp=(unsigned char)strtol(argv[1],&ptr,16);
        arg=arg_temp;
        printf("arg=%d\n",arg_temp);
        ioctl(fb,MIPI_DSI_SET_FORMAT,arg);
    }
    close(fb);
    return 0;
}

static int mipi_set_dsi_cfg_cb(int argc, char *argv[])
{
    int fb = 0;
    unsigned char arg=0;
    char *ptr=NULL;
    int argc_temp=0;
    fb = open(MIPIDEV_PATH, O_RDWR);
    if( fb < 0){
        printf("open %s failed, ret=%d\n", MIPIDEV_PATH,fb);
        return -1;
    }
    argc_temp=argc;
    while(argc_temp)
        printf("%s\n",argv[--argc_temp]);
    if(argc>1)
    {
        arg=(unsigned char)strtol(argv[1],&ptr,16);
        printf("arg=%d\n",arg);
        ioctl(fb,MIPI_DSI_SET_CFG,(unsigned char)arg);
    }
    close(fb);
    return 0;
}

static int mipi_dsi_init_cb(int argc, char *argv[])
{
    int fb = 0;
    fb = open(MIPIDEV_PATH, O_RDWR);
    if( fb < 0){
        printf("open %s failed, ret=%d\n", MIPIDEV_PATH,fb);
        return -1;
    }
    ioctl(fb,MIPI_DSI_INIT,NULL);
    close(fb);
    return 0;
}

static int mipi_sdi_send_cmds_cb(int argc, char *argv[])
{
   int fb = 0;
    unsigned long arg=0;
    struct hc_dcs_cmds cmds;
    unsigned char mipi_data_buf[3]={0};
    int len=0,mipi_add=0,count=0,i=1,j=0;
    char *ptr=NULL;
    count=argc;
    fb = open(MIPIDEV_PATH, O_RDWR);
    if( fb < 0){
        printf("open %s failed, ret=%d\n", MIPIDEV_PATH,fb);
        return -1;
    }
    if(count>1)
    {
        cmds.count=count-1;
        memset(cmds.packet,0,sizeof(cmds.packet));
        printf("mipi_send_cmds :");
        // ioctl(fb,MIPI_DSI_SET_OFF,NULL);
        while(count--)
        {
            len=strlen(argv[i]);
            if(len>2)
                printf("len >2 %d %s",i,argv[i]);
            else
            {
                memcpy(mipi_data_buf,argv[i],2);
                cmds.packet[i-1]=(unsigned char)strtol(mipi_data_buf,&ptr,16);
            }
            printf("0x%02x ",cmds.packet[i-1]);
            i++;
            if(count==1)break;
        }
        printf("cmds.count=%d \n",cmds.count);
        arg = (unsigned long)&cmds;
        ioctl(fb,MIPI_DSI_SEND_DCS_CMDS,arg);
        // ioctl(fb,MIPI_DSI_SET_ON,NULL);
    }
    close(fb);
    return 0;
}

static int mipi_sdi_send_dcs_init_sequence_cb(int argc, char *argv[])
{
    int fb = 0;
    fb = open(MIPIDEV_PATH, O_RDWR);
    if( fb < 0){
        printf("open %s failed, ret=%d\n", MIPIDEV_PATH,fb);
        return -1;
    }
    ioctl(fb,MIPI_DSI_SEND_DCS_INIT_SEQUENCE,NULL);
    close(fb);
    return 0;
}

void io_lvds_gpio_set_output(unsigned int pad,unsigned char value)
{
    int fb = 0;
    fb = open("/dev/lvds", O_RDWR);
    if( fb < 0){
        printf("open /dev/lvds failed, ret=%d\n",fb);
        return;
    }
    struct lvds_set_gpio lvds_pad;
    lvds_pad.padctl=pad;
    lvds_pad.value=value;
    printf("pad= %d value=%d\n",lvds_pad.padctl,lvds_pad.value);
    ioctl(fb,LVDS_SET_GPIO_OUT,&lvds_pad);
    close(fb);
}

static int lvds_gpio_test(int argc, char *argv[])
{
    char *ptr=NULL;
    unsigned char gpio_pad_buf[3]={0};
    unsigned char gpio_value_buf[3]={0};
    struct lvds_set_gpio lvds_pad;
    printf("%s %s argc=%d\n",__FUNCTION__,__FILE__,argc);
    if(argc>2)
    {
        memcpy(gpio_pad_buf,argv[1],2);
        lvds_pad.padctl=(unsigned int)strtol(gpio_pad_buf,&ptr,10);
        memcpy(gpio_value_buf,argv[2],2);
        lvds_pad.value=(unsigned char)strtol(gpio_value_buf,&ptr,10);
        io_lvds_gpio_set_output(lvds_pad.padctl,lvds_pad.value);
    }
    return 0;
}

static int gpio_set_out(int argc, char *argv[])
{
    int count = argc;
    char *endptr=NULL;
    char write_buf[4]={0};
    int gpio_pin;
    int val;
    if(count > 1)
    {
        memcpy(write_buf,argv[1],3);
        gpio_pin=strtol(write_buf, &endptr, 10);
        gpio_configure(gpio_pin,1);
        memset(write_buf,0,3);
        memcpy(write_buf,argv[2],2);
        val=strtol(write_buf, &endptr, 10);
        gpio_set_output(gpio_pin,(bool)val);

    }
    return 0;
}

// static int mipi_dsi_dcs_wr_cb(int argc, char *argv[])
// {
//     int fb = 0;
//     unsigned long arg=0;
//     struct  hc_dcs_read rd_data={0};
//     struct  hc_dcs_write wd_data={0};
//     int i=0,ret=0;

// 	fb = open(MIPIDEV_PATH, O_RDWR);
//     if( fb < 0){
//         printf("open %s failed, ret=%d\n", MIPIDEV_PATH,fb);
//         return -1;
//     }
//     // ioctl(fb,MIPI_DSI_SET_OFF,NULL);

//     wd_data.type=0x37;
//     wd_data.delay_ms=0x01;
//     wd_data.len=2;
//     wd_data.payload[0]=1;
//     wd_data.payload[1]=0;
//     arg = (unsigned long)&wd_data;
//     ioctl(fb,MIPI_DSI_DCS_WRITE,arg);

//     rd_data.type=0x14;
//     rd_data.delay_ms=0x00;
//     rd_data.received_size=0x01;
//     rd_data.command[0]=0x0a;
//     rd_data.command[1]=0x00;
//     arg = (unsigned long)&rd_data;
//     ret=ioctl(fb,MIPI_DSI_DCS_READ,arg);
//     if(ret==0)
//     {
//         for(i=0;i<3;i++)
//         printf("mipi_rd data[%d]=%d\n",i,rd_data.received_data[i]);
//     }
//     // ioctl(fb,MIPI_DSI_SET_ON,NULL);
//     close(fb);
//     return 0;
// }

CONSOLE_CMD(mipi_test, NULL, mipi_test_enter, CONSOLE_CMD_MODE_SELF, "enter mipi test")
CONSOLE_CMD(on, "mipi_test", mipi_dsi_on_cb, CONSOLE_CMD_MODE_SELF, "mipi hal on send")
CONSOLE_CMD(off, "mipi_test", mipi_dsi_off_cb, CONSOLE_CMD_MODE_SELF, "mipi hal off send")
CONSOLE_CMD(color, "mipi_test", mipi_set_dsi_format_cb, CONSOLE_CMD_MODE_SELF, "mipi dsi color set,0<vlaue<5,example:color 5")
CONSOLE_CMD(cfg, "mipi_test", mipi_set_dsi_cfg_cb, CONSOLE_CMD_MODE_SELF, "mipi dsi cfg set,0<vlaue<3f,example:cfg 1c")
CONSOLE_CMD(init, "mipi_test", mipi_dsi_init_cb, CONSOLE_CMD_MODE_SELF, "Re execute initialization Mipi")
CONSOLE_CMD(cmds, "mipi_test", mipi_sdi_send_cmds_cb, CONSOLE_CMD_MODE_SELF, "mipi d0 send cmds,example:cmds 05 00 01 29")
CONSOLE_CMD(init_seq, "mipi_test", mipi_sdi_send_dcs_init_sequence_cb, CONSOLE_CMD_MODE_SELF, "mipi send panel-init-sequence data")
CONSOLE_CMD(lvds_gpio, "mipi_test", lvds_gpio_test, CONSOLE_CMD_MODE_SELF, "lvds_gpio_test,example:lvds_gpio 10 1")
CONSOLE_CMD(gpio, "mipi_test", gpio_set_out, CONSOLE_CMD_MODE_SELF, "gpio_set_out")
// CONSOLE_CMD(wr, "mipi_test", mipi_dsi_dcs_wr_cb, CONSOLE_CMD_MODE_SELF, "Mipi read / write example")
