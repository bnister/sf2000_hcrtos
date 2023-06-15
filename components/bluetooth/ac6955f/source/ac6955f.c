#define LOG_TAG "BLUETOOTH"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/module.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <hcuapi/sci.h>
#include <time.h>
#include <signal.h>
#include <kernel/completion.h>
#include "bluetooth.h"

#include <hcuapi/input-event-codes.h>
static bluetooth_ir_control_t bt_control;
int bluetooth_ir_key_init(bluetooth_ir_control_t control)
{
	if(control == NULL)
		return BT_RET_ERROR;

	bt_control = control;
	log_d("%s %d\n",__func__,__LINE__);
	return BT_RET_SUCCESS;
}

/*
function: Bluetooth sending infrared button
examples: bluetooth_ir_key_send(KEY_DOWN);
*/
int bluetooth_ir_key_send(unsigned short code)
{
	struct input_event_bt event_key = {0};
	if(bt_control == NULL)
		return BT_RET_ERROR;
	event_key.type = EV_KEY;
	event_key.value = 1;
	event_key.code = code;
	bt_control(event_key);
	log_d("%s %d\n",__func__,__LINE__);
	return BT_RET_SUCCESS;
}

int bluetooth_del_all_device(void)
{
    return 0;
}

int bluetooth_del_list_device(void)
{
    return 0;
}

int bluetooth_memory_connection(unsigned char value)
{
    return 0;
}

#ifdef BR2_PACKAGE_BLUETOOTH_FAKE
int bluetooth_init(const char *uart_path, bluetooth_callback_t callback)
{
    return 0;
}
int bluetooth_deinit(void)
{
    return 0;
}
int bluetooth_poweron(void)
{
    return 0;
}
int bluetooth_poweroff(void)
{
    return 0;
}
int bluetooth_scan(void)
{
    return 0;
}
int bluetooth_stop_scan(void)
{
    return 0;
}
int bluetooth_connect(unsigned char *mac)
{
    return 0;
}
int bluetooth_is_connected(void)
{
    return 0;
}
int bluetooth_disconnect(void)
{
    return 0;
}

int bluetooth_set_music_vol(unsigned char val)
{
    return 0;
}

int bluetooth_set_gpio_backlight(unsigned char value)
{
    return 0;
}
int bluetooth_set_gpio_mutu(unsigned char value)
{
    return 0;
}
int bluetooth_set_cvbs_aux_mode(void)
{
    return 0;
}

int bluetooth_set_cvbs_fiber_mode(void)
{
    return 0;
}
#else
#define BT_RET_EXIT     1
#define UART_RX_RECEIVE_MAX_BUFF 1024
#define UART_TX_WRITE_MAX_BUFF 256

#define AC6955F_MAX_WRITE_SIZE          128
#define BT_GPIO_BACKLIGHT_ACTIVE   1
#define BT_GPIO_MUTE_ACTIVE   0

typedef enum _E_BLUETOOTH_CMD_SENDS_{
    BT_CMD_SET_BT_POWER_ON=0,              //1 means the device exists, 0 means the device does not exist
    BT_CMD_SET_INQUIRY_START,                  //Read the obtained Bluetooth name and MAC address
    BT_CMD_SET_INQUIRY_STOP,                         
    BT_CMD_SET_CONNECT,                              //Write the MAC address of the Bluetooth device to be connected
    BT_CMD_GET_CONNECT_INFO,                         
    BT_CMD_SET_DISCONNECT,                           
    BT_CMD_SET_BT_POWER_OFF,                         
    BT_CMD_GET_BT_CONNECT_STATE,                      // 1 connect, other disconnect
                                                           /*0 success; other : error*/
    BT_CMD_SET_PC7_GPIO_OUT,
    BT_CMD_SET_PB11_GPIO_OUT,
    BT_CMD_SET_CVBS_MODE,
    BT_CMD_SET_MUSIC_VOL_VALUE,
    BT_CMD_SET_CONNECTION_CVBS_MODE,
}bt_ac6955f_cmd_sends_e;

typedef enum _E_BT_DEVICE_STATUS_
{
    EBT_DEVICE_STATUS_NOWORKING_DEFAULT=0,
    EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR,
    EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF,
    EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT,
    EBT_DEVICE_STATUS_CMD_ACK_OK,
    EBT_DEVICE_STATUS_WORKING_EXISTENT,
    EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED,
    EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING,
    EBT_DEVICE_STATUS_WORKING_DISCONNECTED,
    EBT_DEVICE_STATUS_WORKING_CONNECTED,
    EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO,
}bt_device_status_e;

struct AC6955F_MessageBody{
    unsigned short  frame_head;
    unsigned short  frame_len;
    unsigned char   frame_id;
    unsigned char   cmd_id;
    unsigned char   cmd_len;
    unsigned char   cmd_value[AC6955F_MAX_WRITE_SIZE];
    unsigned short  Frame_checksum;
};

struct bt_ac6955f_priv {
	int uartfd;
	int cref;
	bluetooth_callback_t callback;
    struct bluetooth_slave_dev inquiry_info;
    struct bluetooth_slave_dev connet_info;
    bt_device_status_e get_dev_status;
    bt_ac6955f_cmd_sends_e bt_cmds;
};

static void bt_ac6955f_cp_connet_info(struct bluetooth_slave_dev *connet_cmds,struct AC6955F_MessageBody *obj);
static int bt_ac6955f_messagebody_switch_str(struct AC6955F_MessageBody *body,unsigned char *buf);
static void bt_ac6955f_read_thread(void *args);
static void bt_ac6955f_set_poll_timeout(int cmds,int *poll_t);
static void bt_ac6955f_serial_data_judg(bt_ac6955f_cmd_sends_e cmd,char *buf,unsigned int count);
static int bt_ac6955f_set_action(bt_ac6955f_cmd_sends_e cmd,unsigned char *send_buf);
static int str_switch_bt_ac6955f_messagebody(char *buf,unsigned int *count,struct AC6955F_MessageBody *body);
static void printf_bt_ac6955f_dev_status(bt_ac6955f_cmd_sends_e status);
static bt_device_status_e bt_get_device_sys_status(void);
static struct completion bt_ac6955f_task_completion;
static int bt_ac6955f_task_start_flag = 0;
static struct bt_ac6955f_priv *gbt = NULL;
static int bt_get_device_sys_connet_info(struct bluetooth_slave_dev *con_data);
int bluetooth_init(const char *uart_path, bluetooth_callback_t callback)
{
	int fd;
    struct sci_setting bt_ac6955f_sci;
    int ret = BT_RET_SUCCESS;
    if(gbt==NULL)
    {
        gbt = (struct bt_ac6955f_priv *)malloc(sizeof(struct bt_ac6955f_priv));
        if(gbt==NULL)goto error;
        memset(gbt,0,sizeof(struct bt_ac6955f_priv));
    }
	if (gbt->cref == 0) {
		/* init bt */
        if(uart_path==NULL)
        {
            goto null_error;
        }

        fd = open(uart_path,O_RDWR);
        if(fd<0) 
        {
            log_e("open %s error %d\n",uart_path,fd);
            goto uart_error;
        }
        bt_ac6955f_sci.parity_mode=PARITY_NONE;
        bt_ac6955f_sci.bits_mode=bits_mode_default;
        ioctl(fd, SCIIOC_SET_BAUD_RATE_9600, NULL);
        
        ioctl(fd, SCIIOC_SET_SETTING, &bt_ac6955f_sci);
        
		/* create task */
        log_d("gbt->uartfd = %d\n",fd);
        gbt->uartfd = fd;
        bt_ac6955f_task_start_flag = 0;
        init_completion(&bt_ac6955f_task_completion);
        ret = xTaskCreate(bt_ac6955f_read_thread , "bt_ac6955f_read_thread" ,
                          0x1000 , &gbt->bt_cmds , portPRI_TASK_NORMAL , NULL);

        if(ret != pdTRUE)
        {
            log_e("kshm recv thread create failed\n");
            goto taskcreate_error;
        }
		/* init success */
        gbt->callback = callback;
		gbt->cref++;
	} else {
		gbt->cref++;
	}

	/* SUCCESS */
	return BT_RET_SUCCESS;

taskcreate_error:
    bt_ac6955f_task_start_flag=1;
    wait_for_completion_timeout(&bt_ac6955f_task_completion , 3000);
    close(fd);
uart_error:
    gbt->uartfd=-1;
null_error:
    free(gbt);
    gbt = NULL;
error:
    return BT_RET_ERROR;
}

int bluetooth_deinit(void)
{
    if(gbt==NULL)
        return BT_RET_ERROR;

	if (gbt->cref == 0)
		return BT_RET_ERROR;

	gbt->cref--;
	if (gbt->cref == 0) {
        // bluetooth_poweroff();
		/* delete task */
        bt_ac6955f_task_start_flag=1;
        wait_for_completion_timeout(&bt_ac6955f_task_completion , 3000);
		/* deinit bt */
		close(gbt->uartfd);
        free(gbt);
        gbt = NULL;
	}

	return BT_RET_SUCCESS;
}

int bluetooth_poweron(void)
{
    bt_device_status_e read_state=0;
    int count=0;
    if(bt_ac6955f_set_action(BT_CMD_SET_BT_POWER_ON,0)==0)
    {
        while(1)
        {
            read_state = bt_get_device_sys_status();
            if(read_state!=EBT_DEVICE_STATUS_NOWORKING_DEFAULT)
            {
                if(read_state<EBT_DEVICE_STATUS_WORKING_EXISTENT)
                    return BT_RET_ERROR;
                break;
            }
            if(count++>200)return BT_RET_ERROR;
            usleep(20 * 1000);
        }
    }
    else
        return BT_RET_ERROR;
    // return BT_RET_SUCCESS;
}

int bluetooth_poweroff(void)
{
    if(bt_ac6955f_set_action(BT_CMD_SET_BT_POWER_OFF,0)==0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
    // return BT_RET_SUCCESS;
}

int bluetooth_scan(void)
{
    if(bt_ac6955f_set_action(BT_CMD_SET_INQUIRY_START,0)==0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_stop_scan(void)
{
    if(bt_ac6955f_set_action(BT_CMD_SET_INQUIRY_STOP,0)==0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_connect(unsigned char *mac)
{
    if(bt_ac6955f_set_action(BT_CMD_SET_CONNECT,mac)==0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_get_connect_info(unsigned char *mac)
{
    // int count=0;
    if(bt_ac6955f_set_action(BT_CMD_GET_CONNECT_INFO,mac)==0)
    {
        // while(1)
        // {
        //     if(bt_get_device_sys_connet_info(info)==0)return BT_RET_SUCCESS;
        //     if(count>200)return BT_RET_ERROR;
        //     usleep(20 * 1000);
        // }
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_is_connected(void)
{
    bt_device_status_e read_state=0;
    int count=0;
    if(bt_ac6955f_set_action(BT_CMD_GET_BT_CONNECT_STATE,0)==0)
    {
        while(1)
        {
            count++;
            read_state = bt_get_device_sys_status();
            if(read_state>EBT_DEVICE_STATUS_NOWORKING_DEFAULT)
            {
                if(read_state==EBT_DEVICE_STATUS_WORKING_CONNECTED)
                {
                    log_d("Device exists\n");
                }
                else if(read_state==EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO)
                {
                    log_d("Device exist\n");
                }
                else if(read_state==EBT_DEVICE_STATUS_WORKING_DISCONNECTED)
                {
                    log_d("Device does not exist\n");
                    return BT_RET_ERROR;
                }
                break;
            }
            if(count>200)return BT_RET_ERROR;
                usleep(20 * 1000);
        }
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_disconnect(void)
{
    if(bt_ac6955f_set_action(BT_CMD_SET_DISCONNECT,0)==0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_set_music_vol(unsigned char val)
{
    bt_device_status_e read_state=0;
    if(bt_ac6955f_set_action(BT_CMD_SET_MUSIC_VOL_VALUE,&val)==0)
    {
        int count=0;
        while(1)
        {
            read_state = bt_get_device_sys_status();
            if(read_state==EBT_DEVICE_STATUS_CMD_ACK_OK)
                break;
            if(count++>10)return BT_RET_ERROR;
            usleep(20 * 1000);
        }
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_set_gpio_backlight(unsigned char value)
{
    // if(BT_GPIO_BACKLIGHT_ACTIVE)
    //     value=value;
    // else
    //     value=(~value)&0x01;

    // bt_device_status_e read_state=0;
    // if(bt_ac6955f_set_action(BT_CMD_SET_PC7_GPIO_OUT,&value)==0)
    // {
    //     int count=0;
    //     while(1)
    //     {
    //         read_state = bt_get_device_sys_status();
    //         if(read_state==EBT_DEVICE_STATUS_CMD_ACK_OK)
    //             break;
    //         if(count++>20)return BT_RET_ERROR;
    //         usleep(20 * 1000);
    //     }
    //     return BT_RET_SUCCESS;
    // }
    // else
    //     return BT_RET_ERROR;
    return BT_RET_SUCCESS;
}

int bluetooth_set_gpio_mutu(unsigned char value)
{
    // if(BT_GPIO_MUTE_ACTIVE)
    //     value=value;
    // else
    //     value=(~value)&0x01;

    // bt_device_status_e read_state=0;
    // if(bt_ac6955f_set_action(BT_CMD_SET_PB11_GPIO_OUT,&value)==0)
    // {
    //     int count=0;
    //     while(1)
    //     {
    //         read_state = bt_get_device_sys_status();
    //         if(read_state==EBT_DEVICE_STATUS_CMD_ACK_OK)
    //             break;
    //         if(count++>10)return BT_RET_ERROR;
    //         usleep(20 * 1000);
    //     }
    //     return BT_RET_SUCCESS;
    // }
    // else
    //     return BT_RET_ERROR;
    return BT_RET_SUCCESS;
}

int bluetooth_set_cvbs_aux_mode(void)
{
    unsigned char value = 0;
    bt_device_status_e read_state=0;
    if(bt_ac6955f_set_action(BT_CMD_SET_CVBS_MODE,&value)==0)
    {
        int count=0;
        while(1)
        {
            read_state = bt_get_device_sys_status();
            if(read_state==EBT_DEVICE_STATUS_CMD_ACK_OK)
                break;
            if(count++>20)return BT_RET_ERROR;
            usleep(20 * 1000);
        }
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

/*Optical fiber mode*/
int bluetooth_set_cvbs_fiber_mode(void)
{
    unsigned char value = 1;
    bt_device_status_e read_state=0;
    if(bt_ac6955f_set_action(BT_CMD_SET_CVBS_MODE,&value)==0)
    {
        int count=0;
        while(1)
        {
            read_state = bt_get_device_sys_status();
            if(read_state==EBT_DEVICE_STATUS_CMD_ACK_OK)
                break;
            if(count++>20)return BT_RET_ERROR;
            usleep(20 * 1000);
        }
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_set_connection_cvbs_mode(unsigned char value)
{
    bt_device_status_e read_state=0;
    if(bt_ac6955f_set_action(BT_CMD_SET_CONNECTION_CVBS_MODE,&value)==0)
    {
        int count=0;
        while(1)
        {
            read_state = bt_get_device_sys_status();
            if(read_state==EBT_DEVICE_STATUS_CMD_ACK_OK)
                break;
            if(count++>20)return BT_RET_ERROR;
            usleep(20 * 1000);
        }
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}
int bluetooth_set_connection_cvbs_aux_mode(void)
{
    return bluetooth_set_connection_cvbs_mode(0);
}
int bluetooth_set_connection_cvbs_fiber_mode(void)
{
    return bluetooth_set_connection_cvbs_mode(1);
}
static void bt_ac6955f_read_thread(void *args)
{
    bt_ac6955f_cmd_sends_e *cmds=(bt_ac6955f_cmd_sends_e *)args;
    bt_ac6955f_cmd_sends_e cmds_t=*cmds;
    char *rx_buf = (char *)malloc(1 * UART_RX_RECEIVE_MAX_BUFF);
    char byte = 0;
    struct pollfd fds[1];
    nfds_t nfds = 1;
    int count=0;
    int ret = BT_RET_SUCCESS;
    int poll_time = 100;

    if(gbt==NULL) return;

    fds[0].fd = gbt->uartfd;
    fds[0].events  = POLLIN|POLLRDNORM;;
    fds[0].revents = 0;
    // log_d("fds[0].fd =%d \n",fds[0].fd);
    // log_d("init \n");
    while(!bt_ac6955f_task_start_flag)
    {
        if(gbt->uartfd<0)break;
        bt_ac6955f_set_poll_timeout(cmds_t,&poll_time);
 poll_loop: 
/*Waiting for serial port data return*/
        ret = poll(fds, nfds, poll_time);
        if (ret > 0)
        {
            if (fds[0].revents & (POLLRDNORM | POLLIN))
            {
                if(read(gbt->uartfd, &byte, 1))
                {
                    rx_buf[count++]=byte;
                    if(count==UART_RX_RECEIVE_MAX_BUFF)
                    {
                        // log_d("Data exceeds\n");
                    }
                    else
                    {
                        // log_d("byte %02x\n",byte);
                        goto poll_loop;
                    }
                }
            }
        }

/*Serial port returns data for judgment*/
        if(count > 0)
        {
            if(cmds_t!=*cmds)
            cmds_t=*cmds;
            log_d("time out count =%d\n",count);
            bt_ac6955f_serial_data_judg(cmds_t,rx_buf,count);
            memset(rx_buf,0,count);
            count =0;
        }
        usleep(20 * 1000);
    }
    free(rx_buf);
    usleep(1000);
    complete(&bt_ac6955f_task_completion);
    vTaskDelete(NULL);
}


static bt_device_status_e bt_get_device_sys_status(void)
{
    if(gbt==NULL)
        return EBT_DEVICE_STATUS_NOWORKING_DEFAULT;
    return gbt->get_dev_status;
}

static int bt_get_device_sys_connet_info(struct bluetooth_slave_dev *con_data)
{
    bt_device_status_e sys_status=bt_get_device_sys_status();
    if(gbt==NULL||con_data==NULL) return BT_RET_ERROR;
    if(sys_status==EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO)
    {
        memcpy(con_data,&gbt->connet_info,sizeof(struct bluetooth_slave_dev));
        return BT_RET_SUCCESS;
    }
    else if(sys_status<EBT_DEVICE_STATUS_WORKING_CONNECTED)
        return 1;

    return BT_RET_ERROR;
}

static int bt_ac6955f_set_action(bt_ac6955f_cmd_sends_e cmd,unsigned char *send_buf)
{
    struct AC6955F_MessageBody body_m={0};
    char buf[UART_TX_WRITE_MAX_BUFF]={0};
    int ret = BT_RET_SUCCESS;
    struct bluetooth_slave_dev *acq_dev_info=NULL;

    bt_ac6955f_cmd_sends_e *dev_status=NULL;
    if(gbt==NULL)
    {
        log_e("gbt = NULL \n");
        return BT_RET_ERROR;
    }
        

    if(gbt->uartfd<0)
    {
        log_e("gbt->uartfd  =%d\n",gbt->uartfd);
        return BT_RET_ERROR;
    }

    if(cmd!=BT_CMD_SET_BT_POWER_ON||cmd!=BT_CMD_SET_PC7_GPIO_OUT||cmd!=BT_CMD_SET_PB11_GPIO_OUT)
    {
        gbt->get_dev_status=EBT_DEVICE_STATUS_NOWORKING_DEFAULT;
    }
    switch(cmd)
    {
        case BT_CMD_SET_BT_POWER_ON:
            body_m.cmd_id=0x02;
            body_m.cmd_len=0x00;
            if(gbt->get_dev_status<EBT_DEVICE_STATUS_WORKING_CONNECTED)
            {
                gbt->get_dev_status=EBT_DEVICE_STATUS_NOWORKING_DEFAULT;
            }
            break;
        case BT_CMD_SET_INQUIRY_START:
            memset(&gbt->inquiry_info,0,sizeof(struct bluetooth_slave_dev));
            body_m.cmd_id=0x06;
            body_m.cmd_len=0x00;
            break;
        case BT_CMD_SET_INQUIRY_STOP:
            body_m.cmd_id=0x07;
            body_m.cmd_len=0x00;
            break;
        case BT_CMD_SET_CONNECT:
        case BT_CMD_GET_CONNECT_INFO:
            if(send_buf==NULL)goto error;
            memcpy(body_m.cmd_value,send_buf,6);
            body_m.cmd_id=0x08;
            body_m.cmd_len=0x06;
            break;
        case BT_CMD_GET_BT_CONNECT_STATE:
            body_m.cmd_id=0x44;
            body_m.cmd_len=0x00;
            break;
        case BT_CMD_SET_DISCONNECT:
            body_m.cmd_id=0x09;
            body_m.cmd_len=0x00;
            break;
        case BT_CMD_SET_BT_POWER_OFF:
            body_m.cmd_id=0x03;
            body_m.cmd_len=0x00;
            gbt->get_dev_status=EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT;
            break;
        case BT_CMD_SET_PC7_GPIO_OUT:
            body_m.cmd_id=0xC6;
            body_m.cmd_len=0x01;
            if(send_buf==NULL)goto error;
            memcpy(body_m.cmd_value,send_buf,body_m.cmd_len);
            break;
        case BT_CMD_SET_PB11_GPIO_OUT:
            body_m.cmd_id=0xC7;
            body_m.cmd_len=0x01;
            if(send_buf==NULL)goto error;
            memcpy(body_m.cmd_value,send_buf,body_m.cmd_len);
            break;
        case BT_CMD_SET_CVBS_MODE:
            body_m.cmd_id=0xC8;
            body_m.cmd_len=0x01;
            if(send_buf==NULL)goto error;
            memcpy(body_m.cmd_value,send_buf,body_m.cmd_len);
            break;
        case BT_CMD_SET_MUSIC_VOL_VALUE:
            body_m.cmd_id=0x14;
            body_m.cmd_len=0x01;
            if(send_buf==NULL)goto error;
            memcpy(body_m.cmd_value,send_buf,body_m.cmd_len);
            break;
        case BT_CMD_SET_CONNECTION_CVBS_MODE:
            body_m.cmd_id=0x11;
            body_m.cmd_len=0x01;
            if(send_buf==NULL)goto error;
            memcpy(body_m.cmd_value,send_buf,body_m.cmd_len);
            break;
        default:
            ret = BT_RET_ERROR;
            break;
    }
    if(ret!=0)goto error;
    gbt->bt_cmds=cmd;
/*uart send data*/
    body_m.frame_id=0x00;
    body_m.frame_head=0xAC69;
    body_m.frame_len=9+body_m.cmd_len;
    memset(buf,0,UART_TX_WRITE_MAX_BUFF);
    if(bt_ac6955f_messagebody_switch_str(&body_m,buf)==BT_RET_ERROR)goto error;
    ret=write(gbt->uartfd,buf,body_m.frame_len);

    if(ret<0)goto error;
    ret = BT_RET_SUCCESS;
exit:
    return ret;
error:
    log_e("data error\n");
    ret = BT_RET_ERROR;
    return ret;
}

static void bt_ac6955f_serial_data_judg(bt_ac6955f_cmd_sends_e cmd,char *buf,unsigned int count)
{
    unsigned int i=0;
    int ret = BT_RET_SUCCESS;
    struct AC6955F_MessageBody body_m={0};
    struct bluetooth_slave_dev *acq_dev_info=&gbt->inquiry_info;
    struct bluetooth_slave_dev *connet_data=&gbt->connet_info;
    unsigned char name_cnt=0;
    i=count;
    if(i<9)
    {
        switch(cmd)
        {
            case BT_CMD_SET_INQUIRY_STOP:
                gbt->get_dev_status=EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING;
                break;
            case BT_CMD_SET_DISCONNECT:
                gbt->get_dev_status=EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                break;
            case BT_CMD_SET_BT_POWER_OFF:
                gbt->get_dev_status=EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT;
                break;
            default:
                break;
        }
    }
    else
    {
        while(i>=9)
        {
            if(ret !=0 ) {
                ret = BT_RET_SUCCESS;
                break;
            }
            if(str_switch_bt_ac6955f_messagebody(&buf[count-i],&i,&body_m)==BT_RET_SUCCESS)
            {
                if(body_m.frame_head != 0xac69) 
                {
                    gbt->get_dev_status = EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR;
                    break;
                }
                switch(cmd){
                case BT_CMD_SET_PC7_GPIO_OUT:
                case BT_CMD_SET_PB11_GPIO_OUT:
                case BT_CMD_SET_CVBS_MODE:
                case BT_CMD_SET_CONNECTION_CVBS_MODE:
                    if(body_m.cmd_id==0x00&&body_m.cmd_value[0]==0x00)
                    {
                        gbt->get_dev_status = EBT_DEVICE_STATUS_CMD_ACK_OK;
                    }
                    break;
                case BT_CMD_SET_BT_POWER_ON:
                        if(gbt->get_dev_status !=EBT_DEVICE_STATUS_WORKING_CONNECTED)
                        {
                            gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_EXISTENT;
                        }
                        // ret = BT_RET_EXIT;
                    break;
                case BT_CMD_SET_INQUIRY_START:
                    if(body_m.cmd_id==0x0c)
                    {
                        memset(acq_dev_info,0,sizeof(struct bluetooth_slave_dev));
                        name_cnt=body_m.cmd_len-BLUETOOTH_MAC_LEN;
                        memcpy(acq_dev_info->mac,body_m.cmd_value,BLUETOOTH_MAC_LEN);
                        if(name_cnt<BLUETOOTH_NAME_LEN)
                        {
                            memcpy(acq_dev_info->name,&body_m.cmd_value[BLUETOOTH_MAC_LEN], name_cnt);
                        }
                        else
                        {
                            memcpy(acq_dev_info->name,&body_m.cmd_value[BLUETOOTH_MAC_LEN], BLUETOOTH_NAME_LEN);
                            acq_dev_info->name[BLUETOOTH_NAME_LEN-1] = 0;
                        }
                        gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED;
                        gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_SCANNED,(unsigned long)acq_dev_info);
                    }
                    else if(body_m.cmd_id==0x0B)
                    {
                        gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING;
                        gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED,(unsigned long)acq_dev_info);
                    }
                    break;
                case BT_CMD_SET_CONNECT:
                    break;
                case BT_CMD_GET_BT_CONNECT_STATE:
                    if(body_m.cmd_id==0X45)
                    {
                        if(body_m.cmd_value[0]==1)
                        {
                            gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_CONNECTED;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED,0);
                        }
                        else
                        {
                            gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED,0);
							ret = BT_RET_EXIT;
                        }                      
                    }
                    break;
                case BT_CMD_SET_INQUIRY_STOP:
                    if(body_m.cmd_id==0x0B)
                    {
                        gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING;
                        gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED,0);
                    }
                    break;
                case BT_CMD_SET_DISCONNECT:
                    gbt->get_dev_status=EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                    gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED,0);
                    ret = BT_RET_EXIT;
                    break;
                case BT_CMD_SET_BT_POWER_OFF:
                    gbt->get_dev_status=EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF;
                    ret = BT_RET_EXIT;
                    break;
                default:
                    ret = BT_RET_EXIT;
                    break;
                }
                if(cmd!=BT_CMD_SET_INQUIRY_START&&ret !=BT_RET_EXIT)
                {
                    switch (body_m.cmd_id)
                    {
                        case 0x0A:
                            if(body_m.cmd_value[0]==0x01)
                            {
                                gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_CONNECTED;
                                gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED,0);
                                ret = BT_RET_EXIT;
                            }
                            else
                            {
                                memset(connet_data,0,sizeof(struct bluetooth_slave_dev));
                                gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                                gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED,0);
                                // ret = BT_RET_EXIT;
                            }
                            break;
                        case 0x04:
                            memset(connet_data,0,sizeof(struct bluetooth_slave_dev));
                            memcpy(connet_data->mac,body_m.cmd_value,BLUETOOTH_MAC_LEN);
                            if(body_m.cmd_len-6<BLUETOOTH_NAME_LEN)
                            {
                                memcpy(connet_data->name,&body_m.cmd_value[BLUETOOTH_MAC_LEN],body_m.cmd_len-6);
                            }
                            else
                            {
                                memcpy(connet_data->name,&body_m.cmd_value[BLUETOOTH_MAC_LEN], BLUETOOTH_NAME_LEN);
                                connet_data->name[BLUETOOTH_NAME_LEN-1] = 0;
                            }
                            gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO,(unsigned long)connet_data);
                            break;
                        case 0x05:
                            gbt->get_dev_status=EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED,0);
                            break;
                        case 0x08:
                            gbt->get_dev_status=BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED,0);
                            break;
                        case 0x09:
                            gbt->get_dev_status=EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED,0);
                            break;
                        default:
                            break;
                    }
                }
            }
            else
            {
                gbt->get_dev_status = EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR;
                ret = BT_RET_ERROR;
            }
        }
    }
    printf_bt_ac6955f_dev_status(gbt->get_dev_status);
}

static void bt_ac6955f_set_poll_timeout(int cmds,int *poll_t)
{
    switch(cmds)
    {
        case BT_CMD_SET_BT_POWER_ON:
            *poll_t=500;
            break;
        case BT_CMD_SET_CONNECT:
            *poll_t=1000;
            break;
        case BT_CMD_SET_INQUIRY_START:
        case BT_CMD_SET_INQUIRY_STOP:
        case BT_CMD_SET_DISCONNECT:
        case BT_CMD_SET_BT_POWER_OFF:
        case BT_CMD_GET_BT_CONNECT_STATE:
            *poll_t=200;
            break;
        default:
            *poll_t=100;
            break;
    }
}

static void printf_bt_ac6955f_dev_status(bt_ac6955f_cmd_sends_e status)
{
    switch(status)
    {
        case EBT_DEVICE_STATUS_NOWORKING_DEFAULT:log_d("EBT_DEVICE_STATUS_NOWORKING_DEFAULT\n");break;
        case EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR:log_d("EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR\n");break;
        case EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT:log_d("EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT\n");break;
        case EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF:log_d("EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF\n");break;
        case EBT_DEVICE_STATUS_WORKING_EXISTENT:log_d("EBT_DEVICE_STATUS_WORKING_EXISTENT\n");break;
        case EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED:log_d("EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED\n");break;
        case EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING:log_d("EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING\n");break;
        case EBT_DEVICE_STATUS_WORKING_DISCONNECTED:log_d("EBT_DEVICE_STATUS_WORKING_DISCONNECTED\n");break;
        case EBT_DEVICE_STATUS_WORKING_CONNECTED:log_d("EBT_DEVICE_STATUS_WORKING_CONNECTED\n");break;
        case EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO:log_d("EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO\n");break;
        default:
            log_d("other\n");break;
            break;
    }
}

static void checksum_calculate(struct AC6955F_MessageBody *body)
{
    unsigned short sum=0;
    if(body==NULL)return;
    sum += (body->frame_head&0x00ff);
    sum += (body->frame_head >>8);
    sum += (body->frame_len&0x00ff);
    sum +=  (body->frame_len >>8);
    sum +=  body->frame_id;
    sum +=  body->cmd_id;
    sum +=  body->cmd_len;
    for(int i=0;i<body->cmd_len;i++)
    {
        sum += body->cmd_value[i];
    }
    body->Frame_checksum =sum;
}

static void bt_ac6955f_mes_printf(struct AC6955F_MessageBody *body)
{
    if(body==NULL)return;
    log_d("ad6956 a cmds : %02x %02x %02x %02x %02x %02x %02x\n",(body->frame_head>>8),(body->frame_head&0x00ff),(body->frame_len&0x00ff),(body->frame_len>>8),body->frame_id,body->cmd_id,body->cmd_len);
    elog_hexdump("data",16,body->cmd_value,body->cmd_len);
    log_d("%02x %02x\n",(body->Frame_checksum&0x00ff),(body->Frame_checksum>>8));
}

static int bt_ac6955f_messagebody_switch_str(struct AC6955F_MessageBody *body,unsigned char *buf)
{
    unsigned char *offset=buf;
    if(body==NULL)return BT_RET_ERROR;
    if(body->cmd_len+9>=UART_RX_RECEIVE_MAX_BUFF)
    {
        log_e("Buf is too long\n");
        return BT_RET_ERROR;
    }
    checksum_calculate(body);
    *offset++=body->frame_head >>8;
    *offset++=body->frame_head&0x00ff;
    *offset++=body->frame_len&0x00ff;
    *offset++=body->frame_len >>8;
    *offset++=body->frame_id;
    *offset++=body->cmd_id;
    *offset++=body->cmd_len;
    memcpy(offset,body->cmd_value,body->cmd_len);
    offset+=body->cmd_len;
    *offset++=body->Frame_checksum&0x00ff;
    *offset=body->Frame_checksum >>8;
    bt_ac6955f_mes_printf(body);
	return BT_RET_SUCCESS;
}

static int str_switch_bt_ac6955f_messagebody(char *buf,unsigned int *count,struct AC6955F_MessageBody *body)
{
    unsigned char *offset=(unsigned char*)buf;
    unsigned int temp=0,temp1=0;
    if(body==NULL||buf==NULL||*count<=0)return BT_RET_ERROR;
    
    // printf("buf : ");
    // for(int i=0;i<*count;i++)
    // {
    //     printf("%02x ",buf[i]);
    // }
    // printf("\n");
    body->frame_head=*offset++;
    body->frame_head<<=8;
    body->frame_head+=(*offset++);
    
    if(body->frame_head!=0xac69)
    {
        *count -= 2;
        return BT_RET_ERROR;
    }
    body->frame_len=*offset++;
    temp=*offset++;
    body->frame_len|=(temp>>8);
    body->frame_id=*offset++;
    body->cmd_id=*offset++;
    body->cmd_len=*offset++;
    if(body->cmd_len+9>=UART_RX_RECEIVE_MAX_BUFF||body->cmd_len>=AC6955F_MAX_WRITE_SIZE)
    {
        log_e("Buf is too long\n");
        return BT_RET_ERROR;
    }
    memcpy(body->cmd_value,offset,body->cmd_len);
    offset+=body->cmd_len;
    checksum_calculate(body);
    temp=0;
    temp1 = (*offset++)&0x00ff;
    temp = *offset;
    temp <<= 8;
    temp += temp1;
    bt_ac6955f_mes_printf(body);
    *count -= (body->cmd_len+9);
    log_d("body->Frame_checksum =%04x temp= %04x\n",body->Frame_checksum,temp);
    if(body->Frame_checksum==temp)
        return BT_RET_SUCCESS;
    else
        return BT_RET_ERROR;
}


static void bt_ac6955f_cp_connet_info(struct bluetooth_slave_dev *connet_cmds,struct AC6955F_MessageBody *obj)
{
    if(connet_cmds==NULL||obj==NULL)return;
    memcpy(obj->cmd_value,connet_cmds->mac,BLUETOOTH_MAC_LEN);
    obj->cmd_len=BLUETOOTH_MAC_LEN;
}

// typedef enum _E_BT_SCAN_STATUS_
// {
//     BT_SCAN_STATUS_DEFAULT=0,
//     BT_SCAN_STATUS_GET_DATA_SEARCHED=1,
//     BT_SCAN_STATUS_GET_DATA_FINISHED=2,
// }bt_scan_status;

// bt_scan_status bt_get_device_sys_inquiry_info(struct bluetooth_slave_dev *inq_data)
// {
//     bt_device_status_e sys_status=bt_get_device_sys_status();
//     if(gbt==NULL||inq_data==NULL) return BT_SCAN_STATUS_DEFAULT;

//     if(sys_status==EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED)
//     {
//         memcpy(inq_data,&gbt->inquiry_info,sizeof(struct bluetooth_slave_dev));
//         return BT_SCAN_STATUS_GET_DATA_SEARCHED;
//     }
//     else if(sys_status==EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING)
//     {
//         memcpy(inq_data,&gbt->inquiry_info,sizeof(struct bluetooth_slave_dev));
//         return BT_SCAN_STATUS_GET_DATA_FINISHED;
//     }
//     return BT_SCAN_STATUS_DEFAULT;
// }
#endif
