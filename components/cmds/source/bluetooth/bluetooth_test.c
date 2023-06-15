#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/lvds.h>
#include <kernel/lib/console.h>
#include <string.h>
#include <hcuapi/gpio.h>
#include <hcuapi/sci.h>
#include <kernel/completion.h>
#include <time.h>
#include <bluetooth.h>
#include <kernel/lib/fdt_api.h>
typedef enum _E_BT_SCAN_STATUS_
{
    BT_SCAN_STATUS_DEFAULT=0,
    BT_SCAN_STATUS_GET_DATA_SEARCHED=1,
    BT_SCAN_STATUS_GET_DATA_FINISHED=2,
}bt_scan_status;

typedef enum _E_BT_CONNECT_STATUS_
{
    BT_CONNECT_STATUS_DEFAULT=0,
    BT_CONNECT_STATUS_DISCONNECTED,
    BT_CONNECT_STATUS_CONNECTED,
    BT_CONNECT_STATUS_GET_CONNECTED_INFO,
}bt_connect_status_e;

typedef enum _BT_DEV_POWER_STATUS_
{
    BT_DEV_POWER_STATUS_DEFAULT=0,
    BT_DEV_POWER_STATUS_WORK,
}bt_dev_power_status;

static bt_dev_power_status power_status;
static bt_scan_status scan_status;
static bt_connect_status_e connet_status;
unsigned char bluetooth_connect_mac[6]={0x88,0xC4,0x3F,0x89,0x9C,0x52};
void bluetooth_inquiry_printf(struct bluetooth_slave_dev *data)
{
    printf("dev mac : ");
    for(int i=0; i<6; i++)
    printf("%02x ", data->mac[i]);
    printf("\n");
    printf("dev name %s\n",data->name);
}

int bluetooth_callback_test(unsigned long event, unsigned long param)
{
    struct bluetooth_slave_dev dev_info={0};
    struct bluetooth_slave_dev *dev_info_t=NULL;
    struct bluetooth_slave_dev bluetooth_connect={0};
    switch (event)
    {
        case BLUETOOTH_EVENT_SLAVE_DEV_SCANNED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_SCANNED\n");
            scan_status=BT_SCAN_STATUS_GET_DATA_SEARCHED;
            connet_status = BT_CONNECT_STATUS_DISCONNECTED;
            if(param==0)break;
            dev_info_t=(struct bluetooth_slave_dev*)param;
            memcpy(&dev_info, dev_info_t,sizeof(struct bluetooth_slave_dev));
            bluetooth_inquiry_printf(&dev_info);
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED\n");
            scan_status=BT_SCAN_STATUS_GET_DATA_FINISHED;
            connet_status = BT_CONNECT_STATUS_DISCONNECTED;
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED\n");
            connet_status = BT_CONNECT_STATUS_DISCONNECTED;
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED\n");
            connet_status = BT_CONNECT_STATUS_CONNECTED;
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO\n");
            if(param==0)break;
            memcpy(&bluetooth_connect, (struct bluetooth_slave_dev*)param,sizeof(struct bluetooth_slave_dev));
            bluetooth_inquiry_printf(&bluetooth_connect);
            connet_status = BT_CONNECT_STATUS_GET_CONNECTED_INFO;
            break;
    }
    // printf("bluetooth_callback_test exit\n");
}

static int bluetooth_test_enter(int argc, char *argv[])
{
	return 0;
}

static int bluetooth_init_cb(int argc, char *argv[])
{
    const char *devpath=NULL;
	int np=fdt_node_probe_by_path("/hcrtos/bluetooth");
    if(np>0)
    {
        if(!fdt_get_property_string_index(np, "devpath", 0, &devpath))
        {
            if(bluetooth_init(devpath, bluetooth_callback_test) == 0){
                printf("%s %d bluetooth_init ok\n",__FUNCTION__,__LINE__);
            }else{
                printf("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);
            }
        }
    }
    else
        printf("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);

	return 0;
}

static int bluetooth_deinit_cb(int argc, char *argv[])
{
    int ret =0;
    if(bluetooth_deinit()==0)
        printf("deinit success\n");
    else
        printf("deinit error\n");
	return 0;
}

static int bluetooth_power_on_cb(int argc, char *argv[])
{
    connet_status = BT_CONNECT_STATUS_DEFAULT;
    if(bluetooth_poweron()==0)
    {
        printf("Device exists\n");
    }
    else
        printf("Device does not exist\n");
	return 0;
}



static int bluetooth_scan_cb(int argc, char *argv[])
{
    struct bluetooth_slave_dev inq_data={0};
    int read_state=0;
    scan_status=BT_SCAN_STATUS_DEFAULT;
    int count=0;
    bluetooth_scan();
	return 0;
}

static int bluetooth_bt_stop_scan_cb(int argc, char *argv[])
{
    bluetooth_stop_scan();
	return 0;
}

static int bluetooth_connect_cb(int argc, char *argv[])
{
    int count=0;
    unsigned char buff[7];
    unsigned char temp[3];
    char *ptr;
    long ret;
    int argc_t=argc-1;
    int i=0;
    connet_status = BT_CONNECT_STATUS_DEFAULT;
    if(argc_t == 0)
    {
        bluetooth_connect(bluetooth_connect_mac);
    }
    else if(argc_t==6)
    {
        while(argc_t--)
        {
            memcpy(temp,argv[i+1],2);
            buff[i++] = (unsigned)strtol(temp, &ptr, 16);
        }
        bluetooth_connect(buff);
    }
	return 0;
}

static int bluetooth_disconnect_cb(int argc, char *argv[])
{
    int count=0;
    bluetooth_disconnect();
    connet_status = BT_CONNECT_STATUS_DISCONNECTED;
	return 0;
}

static int bluetooth_bt_power_off_cb(int argc, char *argv[])
{
    bluetooth_poweroff();
    usleep(2000* 1000);
	return 0;
}

static int bluetooth_bt_get_connect_state_cb(int argc, char *argv[])
{
    int read_state=0,ret=0;
    // struct bluetooth_slave_dev bluetooth_connect={0};
    int count=0;

    if(bluetooth_is_connected()==0)
    {
        printf("The device is connected to a Bluetooth speaker \n");
    }
    else
    {
        printf("The device is not connected to a Bluetooth speaker\n");
    }

	return 0;
}

static int bluetooth_bt_set_gpio_power_cb(int argc, char *argv[])
{
    unsigned char val;
    unsigned char temp[3];
    char *ptr;
    int argc_t=argc-1;
    if(argc > 1)
    {
        memcpy(temp,argv[1],1);
        val = (unsigned)strtol(temp, &ptr, 16);
        bluetooth_set_gpio_backlight(val);
        printf("val : %d\n",val);
    }
    return 0;
}

static int bluetooth_bt_set_mute_cb(int argc, char *argv[])
{
    unsigned char val;
    unsigned char temp[3];
    char *ptr;
    int argc_t=argc-1;
    if(argc > 1)
    {
        memcpy(temp,argv[1],1);
        val = (unsigned)strtol(temp, &ptr, 16);
        bluetooth_set_gpio_mutu(val);
        printf("val : %d\n",val);
    }
    return 0;
}

static int bluetooth_set_cvbs_aux_mode_cb(int argc, char *argv[])
{
    return bluetooth_set_cvbs_aux_mode();
}

static int bluetooth_set_cvbs_fiber_mode_cb(int argc, char *argv[])
{
    return bluetooth_set_cvbs_fiber_mode();
}

static int bluetooth_set_music_vol_cb(int argc, char *argv[])
{
    unsigned char val;
    unsigned char temp[5]={0};
    char *ptr;
    int argc_t=argc-1;
    if(argc > 1)
    {
        memcpy(temp,argv[1],strlen(argv[1]));
        val = (unsigned)strtol(temp, &ptr, 16);
        bluetooth_set_music_vol(val);
        printf("val : %d\n",val);
    }
    return 0;
}

static int bluetooth_set_connection_cvbs_aux_mode_cb(int argc, char *argv[])
{
    return bluetooth_set_connection_cvbs_aux_mode();
}

static int bluetooth_set_connection_cvbs_fiber_mode_cb(int argc, char *argv[])
{
    return bluetooth_set_connection_cvbs_fiber_mode();
}

CONSOLE_CMD(bluetooth, NULL, bluetooth_test_enter, CONSOLE_CMD_MODE_SELF, "enter Bluetooth test")
CONSOLE_CMD(init, "bluetooth", bluetooth_init_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth serial port and task initialization")
CONSOLE_CMD(deinit, "bluetooth", bluetooth_deinit_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth serial port and task initialization")
CONSOLE_CMD(on, "bluetooth", bluetooth_power_on_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth sending power on")
CONSOLE_CMD(scan, "bluetooth", bluetooth_scan_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth search device")
CONSOLE_CMD(stop_scan, "bluetooth", bluetooth_bt_stop_scan_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth stop scan")
CONSOLE_CMD(connet, "bluetooth", bluetooth_connect_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth connected devices, connet [mac addr], for example: connet 88 C4 3F 89 9C 52")
CONSOLE_CMD(disconnet, "bluetooth", bluetooth_disconnect_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth disconnect_cb")
CONSOLE_CMD(off, "bluetooth", bluetooth_bt_power_off_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth power off")
CONSOLE_CMD(get_state, "bluetooth", bluetooth_bt_get_connect_state_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth disconnection")
CONSOLE_CMD(backlight, "bluetooth", bluetooth_bt_set_gpio_power_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth settings backlight,for example: backlight 0")
CONSOLE_CMD(mute, "bluetooth", bluetooth_bt_set_mute_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth settings gpio mute,for example: mute 0")
CONSOLE_CMD(aux, "bluetooth", bluetooth_set_cvbs_aux_mode_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth enters the aux mode")
CONSOLE_CMD(fiber, "bluetooth", bluetooth_set_cvbs_fiber_mode_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth enters fiber mode")
CONSOLE_CMD(c_aux, "bluetooth", bluetooth_set_connection_cvbs_aux_mode_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth enters the aux mode")
CONSOLE_CMD(c_fiber, "bluetooth", bluetooth_set_connection_cvbs_fiber_mode_cb, CONSOLE_CMD_MODE_SELF, "Bluetooth enters fiber mode")
CONSOLE_CMD(vol, "bluetooth", bluetooth_set_music_vol_cb, CONSOLE_CMD_MODE_SELF, "bluetooth_set_music_vol_cb")
