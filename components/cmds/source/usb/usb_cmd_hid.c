#define LOG_TAG "hid_test"
#define ELOG_OUTPUT_LVL ELOG_LVL_INFO

#include <kernel/elog.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <kernel/drivers/input.h>
#include <hcuapi/usbhid.h>

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

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
         printf("open %s failed %d\n", argv[1], fd);
         return 0;
    }
	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDNORM;    

    while(1) {
		if (poll(&pfd, 1, -1) <= 0)
            continue;

        ret = read(fd, &value, sizeof(value));
        if (ret <= 0){
            if(!ret)
                printf("read failed\n");
            continue;
        }
        printf("input type:%d code:%d value:%ld\n", value.type,
            value.code, value.value);
     }
    return 0;
}



static int usb_hidg_kbd_fd = -1;

static int usbkbd_hook(char *data, int len)
{
    int ret = 0;
    if(len){
        // int index;
        // char str[128] = {0};
        // char *pstr = &str[0];
        // pstr += sprintf(pstr, "hook(len:%d): ", len);
        // for(index = 0; index < len; index++)
        //     pstr += sprintf(pstr, " %2.2x", data[index]);
        // pstr += sprintf(pstr, "\n");
        // log_i("%s", str);

        if(usb_hidg_kbd_fd < 0)
            return 0;

        ret = write(usb_hidg_kbd_fd, data, len);
    }
    return ret;
}


int hid_kbd_demo(int argc, char **argv)
{
    int ret;
    int fd, kbd_fd;
	struct pollfd pfd;
    struct input_event event;

    elog_set_filter_tag_lvl("hid_test", ELOG_LVL_INFO);

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        printf("open %s failed %d\n", argv[1], fd);
        return 0;
    }

    usb_hidg_kbd_fd = open("/dev/hidg0", O_RDWR);
    if(usb_hidg_kbd_fd < 0){
        printf("open /dev/hidg0 fail ! Please connect usb to PC\n");
        return 0;
    }

    kbd_fd = open("/dev/usbkbd", O_RDWR);
    if(kbd_fd < 0){
        printf("open /dev/usbkbd fail ! Please connect usb keyboard\n");
        return 0;
    }
    ioctl(kbd_fd, USBHID_SET_HOOK, usbkbd_hook);
    close(kbd_fd);

	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDNORM;

    while(1) {
		if (poll(&pfd, 1, -1) <= 0)
            continue;

        ret = read(fd, &event, sizeof(event));
        if (ret <= 0){
            if(!ret)
                printf("read failed\n");
            continue;
        }

        log_i("input type:%2.2x code:%2.2x value:%2.2lx\n", event.type,
            event.code, event.value);
     }
    return 0;
}





static int usb_hidg_mouse_fd = -1;

static int usbmouse_hook(char *data, int len)
{
    int ret = 0;
    if(len){
        // int index;
        // char str[128] = {0};
        // char *pstr = &str[0];
        // pstr += sprintf(pstr, "hook(len:%d): ", len);
        // for(index = 0; index < len; index++)
        //     pstr += sprintf(pstr, " %2.2x", data[index]);
        // pstr += sprintf(pstr, "\n");
        // log_i("%s", str);

        if(usb_hidg_mouse_fd < 0)
            return 0;

        ret = write(usb_hidg_mouse_fd, data, len);
    }
    return ret;
}


int hid_mouse_demo(int argc, char **argv)
{
    int ret;
    int fd, mouse_fd;
	struct pollfd pfd;
    struct input_event event;

    elog_set_filter_tag_lvl("hid_test", ELOG_LVL_INFO);
    // elog_set_filter_tag_lvl("musbg", ELOG_LVL_DEBUG);
    // elog_set_filter_tag_lvl("hidg", ELOG_LVL_DEBUG);

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        printf("open %s failed %d\n", argv[1], fd);
        return 0;
    }

    usb_hidg_mouse_fd = open("/dev/hidg1", O_RDWR);
    if(usb_hidg_mouse_fd < 0){
        printf("open /dev/hidg1 fail ! Please connect usb to PC\n");
        return 0;
    }

    mouse_fd = open("/dev/usbmouse", O_RDWR);
    if(mouse_fd < 0){
        printf("open /dev/usbmouse fail ! Please connect usb keyboard\n");
        return 0;
    }
    ioctl(mouse_fd, USBHID_SET_HOOK, usbmouse_hook);
    close(mouse_fd);

	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDNORM;

    while(1) {
		if (poll(&pfd, 1, -1) <= 0)
            continue;

        ret = read(fd, &event, sizeof(event));
        if (ret <= 0){
            if(!ret)
                printf("read failed\n");
            continue;
        }

        log_i("input type:%2.2x code:%2.2x value:%2.2lx\n", event.type,
            event.code, event.value);
     }
    return 0;
}
