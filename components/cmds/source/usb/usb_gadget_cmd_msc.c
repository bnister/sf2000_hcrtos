#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <kernel/lib/console.h>
#include <linux/bitops.h>
#include <linux/bits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <nuttx/fs/dirent.h>
#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>
#include <string.h>

#include <kernel/drivers/hcusb.h>

static void usbd_msg_help_info(void)
{
    printf("\nMass Storage Gadget (or MSG) acts as a USB Mass Storage device\n");
    printf("It supports multiple logical units (LUNs).\n");
    printf("usage:\n");
    printf("<1> -h == print help information :\n");
    printf("\tg_mass_storage -h\n");
    printf("<2> -s == shut down usb mass-storage device :\n");
    printf("\tg_mass_storage -s\n");
    printf("\tg_mass_storage -s -p 0\n");
    printf("\tg_mass_storage -s -p 1\n");
    printf("<3> -p == Specify the usb port(usb#0 or usb#1), default usb#0 :\n");
    printf("<4> setup block file or character file as logic unit:\n");
    printf("\tg_mass_storage <path>\n");
    printf("    examples:\n");
    printf("\tg_mass_storage /dev/mmcblk0\n");
    printf("\tg_mass_storage /dev/ram0\n");
    printf("\tg_mass_storage -p 1 /dev/ram0\n");
    printf("\tg_mass_storage -p 0 /dev/ram0 /dev/mmcblk0\n");
}


int setup_usbd_mass_storage(int argc, char **argv)
{
    char ch;
    int i, luns, start;
    const char *udc_name = NULL;
    int usb_port = 0;
    bool is_deinit = false;

    opterr = 0;
    optind = 0;

    while ((ch = getopt(argc, argv, "hHsSp:P:")) != EOF) {
        switch(ch) {
            case 'h' :
            case 'H' :
                usbd_msg_help_info();
                return 0;
            case 'p' :
            case 'P' :
                usb_port = atoi(optarg);
                udc_name = get_udc_name(usb_port);
                if(udc_name == NULL){
                    printf("[error] parameter(-p {usb_port}) error,"
                        "please check for help information(cmd: g_mass_storage -h)\n");
                    return -1;
                }
                break;
            case 's' :
            case 'S' :
                hcusb_gadget_msg_deinit();
                hcusb_set_mode(usb_port, MUSB_HOST);
                is_deinit = true;
                break;
            default:
                break;
        }
    }

    if(is_deinit)
        return 0;

    if(argc < 2){
        printf("[error] parameter error, please check for help information(cmd: g_mass_storage -h)\n");
        return -1;
    }

    luns = udc_name ? (argc - 3) : (argc -1);
    start = udc_name ? 3 : 1;
    printf("logic unit numbers: %u\n", luns);
    for(i = start; i < argc; i++)
        printf("logic unit(LUN:%u) path: %s\n", i - start, argv[i]);

    if(!udc_name){
        hcusb_set_mode(0, MUSB_PERIPHERAL);  // using usb0 as usb gadget
        hcusb_gadget_msg_init(&argv[1], luns);
    }else{
        hcusb_set_mode(usb_port, MUSB_PERIPHERAL);
        hcusb_gadget_msg_specified_init(get_udc_name(usb_port), &argv[3], luns);
    }
    return 0;
}


// CONSOLE_CMD(usb, NULL, NULL, CONSOLE_CMD_MODE_SELF, "usb configuration")
// CONSOLE_CMD(g_mass_storage, "usb", setup_usbd_mass_storage, CONSOLE_CMD_MODE_SELF, "setup USB as mass-storage device")
