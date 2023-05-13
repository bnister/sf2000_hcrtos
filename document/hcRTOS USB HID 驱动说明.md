# hcRTOS USB HID 驱动说明

## 一. 概述

目前hcRTOS 支持USB HID host/gadget两种模式, host模式支持连接键盘和鼠标, 而gadget模式支持将任意一个usb口模拟成鼠标和键盘(两个interface)



## 二. 简单演示

这里有个演示的demo, **usb#0** 连接键盘或者鼠标, 而**usb#1** 则连接到PC上. 然后**通过对usb#0 连接的键盘和鼠标进行操作, 就能在PC上看到对应的键盘或者鼠标操作**

### menuconfig配置

>   *   BR2_PACKAGE_PREBUILTS_USBDRIVER_HID
>   *   BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_HID
>   *   CONFIG_USB_GADGET_HID_VENDOR
>   *   CONFIG_CMDS_USB
>   *   CONFIG_CMDS_USB_GADGET_HID
>   *   CONFIG_CMDS_USB_HID

配置上上述menuconfig后, 输入以下命令进行编译

```shell
make kernel-rebuild cmds-rebuild all
```

### 上电后的操作

上电后,  操作如下

1.   **usb#0** 连接键盘或者鼠标
2.   **usb#1** 通过USB线材连接到PC主机上
3.   在串口终端上输入命令 `usb g_hid -p 1`, 此时 PC主机就会显示有新的USB设备接入
     *   这里的`-p 1` 表示选择**usb#1**, 如果连接到PC主机的是 **usb#0**, 命令则改为 `usb g_hid -p 0`
4.   在串口中断输入命令
     *   如果**usb#1** 连接的是**鼠标**, 则输入命令 `usb hid_mouse_demo /dev/input/mouse0`
     *   如果**usb#1** 连接的是**键盘**, 则输入命令 `usb hid_kbd_demo /dev/input/kbd0`
5.   此时操作键盘或者鼠标, 
     *   串口终端会显示应用所收到的 input event 打印
     *   PC主机也会做出对应的键盘或者鼠标操作



## 三. USB Host HID 驱动说明

### menuconfig配置

*   **BR2_PACKAGE_PREBUILTS_USBDRIVER_HID **  (必须选择)
*   **CONFIG_CMDS_USB** (仅作测试命令, 并非必须)
*   **CONFIG_CMDS_USB_HID** (仅作测试命令, 并非必须)

配置上上述menuconfig后, 输入以下命令进行编译

```shell
make kernel-rebuild cmds-rebuild all
```

### usb hid host 说明

**目前host 仅支持键盘和鼠标**, 接上之后, 

*   键盘: 自动生成input event节点 `/dev/input/kbd0`
*   鼠标: 自动生成input event节点 `/dev/input/mouse0`

应用层就能通过 `open`和`read` `poll` 就能读取对应的event事件值, 具体参考可以见 `components\cmds\source\usb\usb_cmd_hid.c` 的函数 `hid_test_main`

#### 测试命令使用

测试方法:  
在终端输入命令 

*   键盘

```shell
usb hid /dev/input/kbd0
```

*   鼠标

```shell
usb hid /dev/input/mouse0
```

**然后操作所连接的键盘或者鼠标**, 就可以在串口中断上看到对应的 input event事件值



### 源码使用演示

```c
#include <kernel/drivers/input.h>
#include <hcuapi/usbhid.h>
#include <kernel/drivers/hcusb.h>

int hid_test_main(int argc, char **argv)
{
    int ret;
    int fd;
	struct pollfd pfd;
    struct input_event value;

	if(argc != 2){
		printf("Error command\n");
		return -1;
	}

    fd = open(argv[1], O_RDWR);   // 鼠标的话, 这里open的是 /dev/input/mouse0
    							  // 键盘的话, 输入open的是 /dev/input/kbd0
    if (fd < 0) {
         printf("open %s failed %d\n", argv[1], fd);
         return 0;
    }
	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDNORM;    

    while(1) {
		if (poll(&pfd, 1, -1) <= 0)   // 通过poll 来实现阻塞
            continue;

        ret = read(fd, &value, sizeof(value)); // 通用read 来读取具体的 input event
        if (ret <= 0){
            if(!ret)
                printf("read failed\n");
            continue;
        }
        printf("input type:%d code:%d value:%ld\n", value.type,
            value.code, value.value);   // 打印对应的input event值
     }
    return 0;
}
```



### usb hid hook配置

hcRTOS可以配置hook, 每次host hid 每次接收到一笔usb数据, 就会调用的这个hook
应用层可以通过ioctrl命令 (`USBHID_SET_HOOK`) 来配置

```c
// from hcuapi/usbhid.h
typedef int (*usbhid_hook_t)(char *data, int len);
#define USBHID_SET_HOOK			_IOW (USBHID_IOCBASE, 0, usbhid_hook_t)
#define USBHID_CLEAR_HOOK		_IO (USBHID_IOCBASE, 1)
```

其中键盘对应的配置hook的设备文件是 `/dev/usbkbd`
而鼠标对应的配置hook的设备 `/dev/usbmouse`

#### 使用示例

```c
#include <kernel/drivers/input.h>
#include <hcuapi/usbhid.h>
#include <kernel/drivers/hcusb.h>


static int usbmouse_hook(char *data, int len)
{
    int i;
    for(i = 0; i < len; i++)
        printf("%2.2x ", (unsigned char)data[i]);
    printf("\n");
}

void test(void)
{
    ...
    int config_fd = open(/dev/usbmouse);
    ioctl(config_fd, USBHID_SET_HOOK, usbmouse_hook);
	...
}

```





## 四. USB Gadget HID 驱动说明

### menuconfig配置

*   **BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER **  (必须选择)
*   **BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_HID**  (必须选择)
*   **CONFIG_USB_GADGET_HID_VENDOR** (必须选择)
*   **CONFIG_CMDS_USB** (仅作测试命令, 并非必须)
*   **CONFIG_CMDS_USB_GADGET_HID** (仅作测试命令, 并非必须)

配置上上述menuconfig后, 输入以下命令进行编译

```shell
make kernel-rebuild cmds-rebuild all
```

###  usb gadget hid 说明

这里就是就是将usb端口配置为 **usb gadget模式**, 然后再将此usb端口模拟成**一个同时包含键盘和鼠标功能的USB设备**

配置后会自动生成两个设备文件, 

*   代表键盘: **/dev/hidg0**
*   代表鼠标: **/dev/hidg1**

然后应用层就可以通过 `open` 和 `write` 接口将对应的input event 值写进去, 此时就会对应usb数据就会发出去

####  测试命令说明

*   模拟成键盘操作, 可以输入命令

```shell
usb hid_gadget_test /dev/hidg0 keyboard
```

此时就是每隔1秒, PC就会收到 字母`a` 到 `z` 的字符

*   模拟成鼠标, 可以输入命令

```shell
usb hid_gadget_test /dev/hidg1 mouse
```

此时每个一秒钟, PC就会看到鼠标移动一次

具体的源码示例, 可见 `components\cmds\source\usb\usb_gadget_cmd_hid.c` 的函数 `hid_gadget_test`

```c
{
    ...
    int fd = open("/dev/hidg0", O_RDWR, 0666);
    write(fd, report, 8);
    ...
}
```



###  代码说明

#### 配置 usb report 描述符内容

需要先在 menuconfig 中打开宏 

*   **CONFIG_USB_GADGET_HID_VENDOR**

`components\kernel\source\drivers\usb\gadget\legacy\hid.c` 这里配置所模拟的键盘和鼠标的 **report 描述符的内容**

```c
struct hidg_func_descriptor us_keyboard_data = {
    .subclass        = 0, /* No subclass */
    .protocol        = 1, /* Keyboard */
    .report_length        = 8,
    .report_desc_length    = 63,
    .report_desc        = {
        0x05, 0x01,    /* USAGE_PAGE (Generic Desktop)     */
        0x09, 0x06,    /* USAGE (Keyboard) */
        0xa1, 0x01,    /* COLLECTION (Application) */
        0x05, 0x07,    /* USAGE_PAGE (Keyboard) */
        0x19, 0xe0,    /* USAGE_MINIMUM (Keyboard LeftControl) */
        0x29, 0xe7,    /* USAGE_MAXIMUM (Keyboard Right GUI) */
        0x15, 0x00,    /* LOGICAL_MINIMUM (0) */
        0x25, 0x01,    /* LOGICAL_MAXIMUM (1) */
        0x75, 0x01,    /* REPORT_SIZE (1) */
        0x95, 0x08,    /* REPORT_COUNT (8) */
        0x81, 0x02,    /* INPUT (Data,Var,Abs) */
        0x95, 0x01,    /* REPORT_COUNT (1) */
        0x75, 0x08,    /* REPORT_SIZE (8) */
        0x81, 0x03,    /* INPUT (Cnst,Var,Abs) */
        0x95, 0x05,    /* REPORT_COUNT (5) */
        0x75, 0x01,    /* REPORT_SIZE (1) */
        0x05, 0x08,    /* USAGE_PAGE (LEDs) */
        0x19, 0x01,    /* USAGE_MINIMUM (Num Lock) */
        0x29, 0x05,    /* USAGE_MAXIMUM (Kana) */
        0x91, 0x02,    /* OUTPUT (Data,Var,Abs) */
        0x95, 0x01,    /* REPORT_COUNT (1) */
        0x75, 0x03,    /* REPORT_SIZE (3) */
        0x91, 0x03,    /* OUTPUT (Cnst,Var,Abs) */
        0x95, 0x06,    /* REPORT_COUNT (6) */
        0x75, 0x08,    /* REPORT_SIZE (8) */
        0x15, 0x00,    /* LOGICAL_MINIMUM (0) */
        0x25, 0x65,    /* LOGICAL_MAXIMUM (101) */
        0x05, 0x07,    /* USAGE_PAGE (Keyboard) */
        0x19, 0x00,    /* USAGE_MINIMUM (Reserved) */
        0x29, 0x65,    /* USAGE_MAXIMUM (Keyboard Application) */
        0x81, 0x00,    /* INPUT (Data,Ary,Abs) */
        0xc0        /* END_COLLECTION */
    }
};


/*hid descriptor for a mouse*/
struct hidg_func_descriptor us_mouse_data = {
	.subclass = 0,	/*NO SubClass*/
	.protocol = 2,	/*Mouse*/
	.report_length = 4,
	.report_desc_length = 52,
	.report_desc={
		0x05, 0x01, 
		0x09, 0x02, 
		0xA1, 0x01, 
		0x09, 0x01, 
		0xA1, 0x00, 
		0x05, 0x09, 
		0x19, 0x01, 
		0x29, 0x03, 
		0x15, 0x00, 
		0x25, 0x01, 
		0x95, 0x03, 
		0x75, 0x01, 
		0x81, 0x02, 
		0x95, 0x01, 
		0x75, 0x05, 
		0x81, 0x01, 
		0x05, 0x01, 
		0x09, 0x30, 
		0x09, 0x31,
		0x09, 0x38, 
		0x15, 0x81, 
		0x25, 0x7F, 
		0x75, 0x08, 
		0x95, 0x03, 
		0x81, 0x06, 
		0xC0,
		0xC0
		#endif
	}
};
```



####  配置usb端口为gadget模式

```c
/* 
** brief: setup USB mode as HOST or GADGET
** parm: 
**   usb_port -- 0:usb0, 1:usb1
**   mode -- usb mode (MUSB_HOST, MUSB_PERIPHERAL, MUSB_OTG)
** return:
**   0  -- successfully 
**   !0 -- failure
*/
int hcusb_set_mode(uint8_t usb_port, enum musb_mode mode);



/* 
** brief: return USB mode as HOST or GADGET
** parm: 
**   usb_port -- 0:usb0, 1:usb1
** return:
**   mode -- usb mode (MUSB_HOST, MUSB_PERIPHERAL, MUSB_OTG)
*/
enum musb_mode hcusb_get_mode(uint8_t usb_port);
```



##### 使用示例:

```c
#include <kernel/drivers/hcusb.h>

{
    ...
    hcusb_set_mode(0, MUSB_PERIPHERAL); //配置usb#0 为gadget模式
    hcusb_set_mode(1, MUSB_PERIPHERAL); //配置usb#1 为gadget模式
    ...
}
```



#### 配置usb 端口为对应的 hid 功能

```c
// from hcusb.h
int hcusb_gadget_hidg_init(void);
void hcusb_gadget_hidg_deinit(void);
int hcusb_gadget_hidg_specified_init(const char *udc_name);
```



##### 使用示例

```c
#include <kernel/drivers/hcusb.h>
{
...
    hcusb_gadget_hidg_specified_init(get_udc_name(0));   // 配置usb#0 的gadget为HID 
    hcusb_gadget_hidg_specified_init(get_udc_name(1));   // 配置usb#1 的gadget为HID 
...
}
```

