#ifndef __USB_CMDS_MAIN_H__
#define __USB_CMDS_MAIN_H__

#include <generated/br2_autoconf.h>
#include <kernel/lib/console.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// #include <pthread.h>
#include <dirent.h>
#include <sys/poll.h>

// extern int setup_usbd_mass_storage(int argc, char **argv);
// extern int setup_usbd_serial(int argc, char **argv);
// extern int setup_usbd_serial_testing(int argc, char **argv);
// extern int setup_usbd_ncm(int argc, char **argv);
// extern int setup_usbd_zero(int argc, char **argv);
// extern int setup_usbd_hid(int argc, char **argv);



__attribute__((weak)) int setup_usbd_serial(int argc, char **argv)
{
    return -1;
}

__attribute__((weak)) int setup_usbd_serial_testing(int argc, char **argv)
{
    return -1;
}

__attribute__((weak)) int setup_usbd_ncm(int argc, char **argv)
{
    return -1;
}

__attribute__((weak)) int setup_usbd_mass_storage(int argc, char **argv)
{
    return -1;
}

__attribute__((weak)) int setup_usbd_zero(int argc, char **argv)
{
    return -1;
}

__attribute__((weak)) int setup_usbd_hid(int argc, char **argv)
{
    return -1;
}

__attribute__((weak)) int hid_gadget_test(int argc, char **argv)
{
    return -1;
}

__attribute__((weak)) int hid_test_main(int argc, char **argv)
{
    return -1;
}

__attribute__((weak)) int hid_kbd_demo(int argc, char **argv)
{
    return -1;
}

__attribute__((weak)) int hid_mouse_demo(int argc, char **argv)
{
    return -1;
}

__attribute__((weak)) int hid_dev_demo_main(int argc, char **argv)
{
    return -1;
}

extern int libusb_helloworld_demo(int argc, char **argv);
extern int testlibusb(int argc, char **argv);
extern int xusb(int argc, char** argv);
extern int hotplug(int argc, char** argv);

#endif // !__USB_CMDS_MAIN_H__
